#include "stm32f10x.h"
#include "OLED.h"
#include "Encoder.h"
#include "Key.h"
#include "Input.h"
#include "Menu.h"
#include "LED.h"
#include "AD.h"
#include "Servo.h"
#include "Serial.h"
#include "Delay.h"
#include "DS18B20.h"
#include "Buzzer.h"

static volatile uint32_t TickMs = 0;

static int16_t LatestTemperature10 = 0;
static uint8_t HaveTemperature = 0;
static uint32_t LastGoodAt = 0;
static uint32_t MonitorStartedAt = 0;

static Alarm_State_t AlarmState = ALARM_WAIT;
static uint32_t BeepStartedAt = 0;

/* TIM4提供毫秒计时，SysTick继续留给Delay_us。 */
static void Tick_Init(void)
{
    RCC_ClocksTypeDef clocks;
    TIM_TimeBaseInitTypeDef timer;
    NVIC_InitTypeDef nvic;
    uint32_t timerClock;

    RCC_GetClocksFreq(&clocks);
    timerClock = clocks.PCLK1_Frequency;

    /* APB1分频时，定时器时钟为PCLK1的两倍。 */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U)
    {
        timerClock *= 2U;
    }

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_Cmd(TIM4, DISABLE);

    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler =
        (uint16_t)(timerClock / 1000000U - 1U);
    timer.TIM_Period = 999;

    TIM_TimeBaseInit(TIM4, &timer);
    TIM_SetCounter(TIM4, 0);
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

    nvic.NVIC_IRQChannel = TIM4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TickMs = 0;

    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}

/* 本项目只定义这一份TIM4_IRQHandler。 */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        TickMs++;
    }
}

static uint32_t Tick_GetMs(void)
{
    return TickMs;
}

/* 每约500ms启动一轮；慢速转换完成后再开始下一轮。 */
static void Temperature_Task(void)
{
    static uint8_t waiting = 0;
    static uint8_t cycleStarted = 0;
    static uint32_t cycleAt = 0;
    static uint32_t convertAt = 0;

    uint32_t now = Tick_GetMs();
    int16_t value10;

    if (waiting != 0U)
    {
        /* 未完成且未超时则继续后台等待，不结束本次转换。 */
        if (DS18B20_IsConversionDone() == 0U &&
            (uint32_t)(now - convertAt) < DS18B20_GetConversionTimeoutMs())
        {
            return;
        }

        waiting = 0;

        if (DS18B20_ReadTemperature(&value10) != 0U)
        {
            LatestTemperature10 = value10;
            HaveTemperature = 1;
            LastGoodAt = Tick_GetMs();

            Menu_SetTemperature(value10, 1);
        }

        /*
         * 失败不更新时间。
         * 持续无有效数据由Alarm_Task判定故障。
         */
        return;
    }

    if (cycleStarted != 0U &&
        (uint32_t)(now - cycleAt) < 500U)
    {
        return;
    }

    cycleStarted = 1;
    cycleAt = now;

    if (DS18B20_StartConvert() != 0U)
    {
        /* 在启动函数返回后计时，避免提前读取。 */
        convertAt = Tick_GetMs();
        waiting = 1;
    }
}

/* 判断正常、低温、高温、故障状态。 */
static void Alarm_Task(void)
{
    static uint8_t limitsKnown = 0;
    static int16_t previousLow = 0;
    static int16_t previousHigh = 0;

    int16_t low = Menu_GetLowLimit();
    int16_t high = Menu_GetHighLimit();

    /* 菜单阈值单位为℃，采集值单位为0.1℃。 */
    int16_t low10 = (int16_t)(low * 10);
    int16_t high10 = (int16_t)(high * 10);

    uint32_t now = Tick_GetMs();
    uint8_t limitsChanged;
    Alarm_State_t previous;
    Alarm_State_t next;

    limitsChanged = (!limitsKnown ||
                     low != previousLow ||
                     high != previousHigh);

    limitsKnown = 1;
    previousLow = low;
    previousHigh = high;

    /*
     * 修改阈值后，用新阈值重新判断，
     * 不继承旧阈值下的回差状态。
     */
    previous = limitsChanged ? ALARM_NORMAL : AlarmState;

    if (HaveTemperature == 0U)
    {
        /* 开机2秒还未读到有效温度，判为故障。 */
        next = ((uint32_t)(now - MonitorStartedAt) >= 2000U)
               ? ALARM_FAULT : ALARM_WAIT;
    }
    else if ((uint32_t)(now - LastGoodAt) >= 2000U)
    {
        next = ALARM_FAULT;
    }
    else if (LatestTemperature10 > high10)
    {
        next = ALARM_HIGH;
    }
    else if (LatestTemperature10 < low10)
    {
        next = ALARM_LOW;
    }
    else if (previous == ALARM_HIGH &&
             LatestTemperature10 > high10 - 10)
    {
        /* 高温报警：降到上限减1℃才解除。 */
        next = ALARM_HIGH;
    }
    else if (previous == ALARM_LOW &&
             LatestTemperature10 < low10 + 10)
    {
        /* 低温报警：升到下限加1℃才解除。 */
        next = ALARM_LOW;
    }
    else
    {
        next = ALARM_NORMAL;
    }

    if (next != AlarmState)
    {
        AlarmState = next;
        BeepStartedAt = now;
    }

    if (next == ALARM_WAIT || next == ALARM_FAULT)
    {
        Menu_SetTemperature(0, 0);
    }

    Menu_SetAlarmState(next);
}

/* 根据报警状态控制节奏，不使用长延时。 */
static void Buzzer_Task(void)
{
    uint32_t phase =
        (uint32_t)(Tick_GetMs() - BeepStartedAt) % 1000U;

    uint8_t on = 0;

    if (AlarmState == ALARM_HIGH || AlarmState == ALARM_LOW)
    {
        /* 每秒响200ms，停800ms。 */
        on = (phase < 200U);
    }
    else if (AlarmState == ALARM_FAULT)
    {
        /* 每秒响两次，每次100ms。 */
        on = (phase < 100U ||
              (phase >= 200U && phase < 300U));
    }

    if (on != 0U)
    {
        Buzzer_On();
    }
    else
    {
        Buzzer_Off();
    }
}

/* 开机文字每100ms出现一个字符，全部显示后停留2秒。 */
static void Boot_Show(void)
{
    static const char studentID[] = "2025212098";
    static const char *const welcome[] = {"欢", "迎", "加", "入"};
    uint8_t i;

    OLED_Clear();
    /* 10个ASCII字符共80像素，从X=24（第4列）开始居中。 */
    for (i = 0; i < 10; i++)
    {
        OLED_ShowChar(1, 4 + i, studentID[i]);
        Delay_ms(100);
    }
    /* 4个汉字共64像素，从X=32（第3个汉字列）开始居中。 */
    for (i = 0; i < 4; i++)
    {
        OLED_ShowChinese(2, 3 + i, welcome[i]);
        if (i < 3) Delay_ms(100);
    }
    Delay_ms(2000);
    OLED_Clear();
}

int main(void)
{
    Input_Event_t event;
    int16_t serialAngle;
    uint16_t targetAngle;

    uint32_t now;
    uint32_t lastADC;
    uint32_t lastLED;
    uint8_t ledRunning = 0;

    Buzzer_Init();
    LED_Init();

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Tick_Init();

    Key_Init();
    Encoder_Init();
    OLED_Init();
    Boot_Show();

    AD_Init();
    PWM_Init();
    Serial_Init();

    Menu_Init();
    DS18B20_Init();

    MonitorStartedAt = Tick_GetMs();
    BeepStartedAt = MonitorStartedAt;

    /* 进入循环后立即采集一次电压。 */
    lastADC = MonitorStartedAt - 100U;
    lastLED = MonitorStartedAt;

    Servo_SetAngle(Menu_GetServoAngle());

    while (1)
    {
        /* 非阻塞按键扫描及菜单处理。 */
        Key_Scan(Tick_GetMs());
        event = Input_GetEvent();
        Menu_Process(event);

        /* 保留现有串口舵机控制。 */
        if (Serial_RxFlag == 1)
        {
            serialAngle = Serial_ParseAngle(Serial_RxPacket);

            if (serialAngle >= 0)
            {
                Menu_SetServoAngle((uint16_t)serialAngle);
                Servo_SetAngle((uint16_t)serialAngle);
                Serial_SendString("ACK\r\n");
            }
            else
            {
                Serial_SendString("ERR\r\n");
            }

            Serial_RxFlag = 0;
        }

        /* 更新采集值，再判断报警，再控制蜂鸣器。 */
        Temperature_Task();
        Alarm_Task();
        Buzzer_Task();

        now = Tick_GetMs();

        /* 电压每100ms更新一次。 */
        if ((uint32_t)(now - lastADC) >= 100U)
        {
            lastADC = now;
            Menu_SetVoltage(AD_GetVoltage());
        }

        /* 流水灯仍然每100ms推进一步。 */
        if (Menu_GetLEDEnabled() != 0U)
        {
            if (!ledRunning ||
                (uint32_t)(now - lastLED) >= 100U)
            {
                LED_Run();
                lastLED = now;
                ledRunning = 1;
            }
        }
        else if (ledRunning != 0U)
        {
            LED_Off();
            ledRunning = 0;
        }

        targetAngle = Menu_GetServoAngle();
        Servo_SetAngle(targetAngle);

        /* 切页可能清屏，每轮最后补绘报警标记。 */
        Menu_ShowAlarmIndicator();

        /* 取代原来的100ms长间隔。 */
        Delay_ms(5);
    }
}
