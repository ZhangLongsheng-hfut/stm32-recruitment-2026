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

/* 每轮调用一次；要求主循环每轮保留 Delay_ms(100)。 */
static void Temperature_Task(void)
{
    static uint8_t waiting = 0;
    static uint8_t wait_count = 0;
    int16_t temperature10 = 0;

    if (waiting == 0U)
    {
        if (DS18B20_StartConvert() != 0U)
        {
            waiting = 1;
            wait_count = 0;
        }
        else
        {
            Menu_SetTemperature(0, 0);
        }
        return;
    }

    wait_count++;
    if (wait_count < 4U)
    {
        return;
    }

    if (DS18B20_ReadTemperature(&temperature10) != 0U)
    {
        Menu_SetTemperature(temperature10, 1);
    }
    else
    {
        Menu_SetTemperature(0, 0);
    }

    waiting = 0;
}

int main(void)
{
    Input_Event_t Event;
    uint16_t VoltageMv;
    uint16_t TargetAngle;
    int16_t SerialAngle;    /* -1表示串口命令错误 */

    /* 初始化蜂鸣器、显示、编码器和按键 */
	Buzzer_Init();
    OLED_Init();
    Encoder_Init();
    Key_Init();

    /* 初始化流水灯、ADC、舵机、菜单和温度传感器 */
    LED_Init();
    AD_Init();
    PWM_Init();
    Menu_Init();
	DS18B20_Init();

    /* 初始化USART1：9600波特率，8位数据，无校验，1位停止 */
    Serial_Init();

    /* 设置开机目标角度 */
    TargetAngle = Menu_GetServoAngle();
    Servo_SetAngle(TargetAngle);

    while (1)
    {
        /* 处理菜单输入 */
        Event = Input_GetEvent();
        Menu_Process(Event);

        /* 收到完整一行后，再解析串口命令 */
        if (Serial_RxFlag == 1)
        {
            SerialAngle = Serial_ParseAngle(Serial_RxPacket);

            if (SerialAngle >= 0)
            {
                /* 更新菜单中的目标角度，并同步页面 */
                Menu_SetServoAngle((uint16_t)SerialAngle);

                /* 让舵机执行新的目标角度 */
                Servo_SetAngle((uint16_t)SerialAngle);

                Serial_SendString("ACK\r\n");
            }
            else
            {
                /* 错误命令不修改目标角度 */
                Serial_SendString("ERR\r\n");
            }

            /* 处理并回复后，允许接收下一条命令 */
            Serial_RxFlag = 0;
        }

        /* 菜单控制流水灯 */
        if (Menu_GetLEDEnabled() == 1)
        {
            LED_Run();
        }
        else
        {
            LED_Off();
        }

        /* 更新电位器电压 */
        VoltageMv = AD_GetVoltage();
        Menu_SetVoltage(VoltageMv);

        /* 执行菜单保存的角度，保持两种控制方式同步 */
        TargetAngle = Menu_GetServoAngle();
		Servo_SetAngle(TargetAngle);

		/* 处理温度采集 */
		Temperature_Task();    
		Delay_ms(100);
    }
}
