/* DS18B20 单传感器驱动：PB10 为单总线数据脚，采用外部供电的转换轮询方式。
 * 调用流程：Init -> StartConvert -> IsConversionDone -> ReadTemperature。
 * 驱动只给出测量值和成功/失败，报警上下限由 Menu.c 与 main.c 管理。
 */
#include "stm32f10x.h"
#include "DS18B20.h"
#include "Delay.h"

#define DS18B20_PORT       GPIOB
#define DS18B20_PIN        GPIO_Pin_10
#define DS18B20_CLOCK      RCC_APB2Periph_GPIOB

/* PB10 开漏输出：写 1 释放总线，写 0 拉低总线。 */
#define DQ_LOW()           (DS18B20_PORT->BRR = DS18B20_PIN)
#define DQ_RELEASE()       (DS18B20_PORT->BSRR = DS18B20_PIN)
#define DQ_READ()          ((DS18B20_PORT->IDR & DS18B20_PIN) != 0U)

/* Converting 表示本次转换已启动；ConfigurationTried 避免每轮重复配置。
 * ConversionResolution 保存配置寄存器的位 6:5，右移 5 位后作为查表下标。
 */
static uint8_t Converting = 0;
static uint8_t ConfigurationTried = 0;
static uint8_t ConversionResolution = 0x60U;

/* 9/10/11/12 位转换时间上限加少量余量，单位 ms。 */
static const uint16_t ConversionTimeouts[4] = {100U, 190U, 380U, 760U};
static const uint16_t TemperatureMasks[4] = {0xFFF8U, 0xFFFCU, 0xFFFEU, 0xFFFFU};

/* 复位并检测应答，返回 1 表示传感器存在。 */
static uint8_t DS18B20_Reset(void)
{
    uint32_t primask;
    uint8_t present;

    DQ_RELEASE();
    Delay_us(10);
    if (!DQ_READ())
    {
        return 0;
    }

    /* 保存进入前的中断屏蔽状态，时序关键段结束后恢复，避免错误地开启原本关闭的中断。 */
    primask = __get_PRIMASK();
    __disable_irq();

    DQ_LOW();
    Delay_us(500);
    DQ_RELEASE();
    Delay_us(70);
    present = DQ_READ() ? 0U : 1U;

    __set_PRIMASK(primask);
    Delay_us(410);

    /* 应答结束后总线应回到高电平。 */
    return (present && DQ_READ()) ? 1U : 0U;
}

/* 写一位，短暂关闭中断以保证单总线时序。 */
static void DS18B20_WriteBit(uint8_t bit)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();

    DQ_LOW();
    if (bit != 0U)
    {
        Delay_us(6);
        DQ_RELEASE();
        Delay_us(64);
    }
    else
    {
        Delay_us(60);
        DQ_RELEASE();
        Delay_us(10);
    }

    __set_PRIMASK(primask);
}

/* 读一位，在读时隙开始后约 11us 采样。 */
static uint8_t DS18B20_ReadBit(void)
{
    uint32_t primask;
    uint8_t bit;

    primask = __get_PRIMASK();
    __disable_irq();

    DQ_LOW();
    Delay_us(3);
    DQ_RELEASE();
    Delay_us(8);
    bit = DQ_READ() ? 1U : 0U;
    Delay_us(59);

    __set_PRIMASK(primask);
    return bit;
}

/* 低位先发。 */
static void DS18B20_WriteByte(uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        DS18B20_WriteBit(data & 0x01U);
        data >>= 1;
    }
}

/* 低位先读。 */
static uint8_t DS18B20_ReadByte(void)
{
    uint8_t i;
    uint8_t data = 0;

    for (i = 0; i < 8; i++)
    {
        if (DS18B20_ReadBit() != 0U)
        {
            data |= (uint8_t)(1U << i);
        }
    }
    return data;
}

/* Dallas/Maxim CRC-8，反射形式多项式为 0x8C。 */
static uint8_t DS18B20_CRC8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;
    uint8_t i;
    uint8_t j;

    for (i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if ((crc & 0x01U) != 0U)
            {
                crc = (uint8_t)((crc >> 1) ^ 0x8CU);
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* 读取完整 9 字节暂存器，并校验数据。 */
static uint8_t DS18B20_ReadScratchpad(uint8_t data[9])
{
    uint8_t i;
    uint8_t nonzero = 0;

    if (DS18B20_Reset() == 0U)
    {
        return 0;
    }

    DS18B20_WriteByte(0xCC);  /* Skip ROM：总线上只有一个传感器。 */
    DS18B20_WriteByte(0xBE);  /* Read Scratchpad。 */

    for (i = 0; i < 9; i++)
    {
        data[i] = DS18B20_ReadByte();
        nonzero |= data[i];
    }

    /* 全零数据也能通过 CRC，必须单独排除。 */
    if (nonzero == 0U)
    {
        return 0;
    }

    if (DS18B20_CRC8(data, 8) != data[8])
    {
        return 0;
    }

    return 1;
}

uint8_t DS18B20_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(DS18B20_CLOCK, ENABLE);

    DQ_RELEASE();
    gpio.GPIO_Pin = DS18B20_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_PORT, &gpio);

    Converting = 0;
    ConfigurationTried = 0;
    ConversionResolution = 0x60U;

    return DS18B20_Reset();
}

uint8_t DS18B20_StartConvert(void)
{
    uint8_t data[9];
    uint8_t th;
    uint8_t tl;
    uint8_t attempt;

    Converting = 0;

    if (DS18B20_ReadScratchpad(data) == 0U)
    {
        ConfigurationTried = 0;
        return 0;
    }

    /* 配置变化或传感器重新接入后，重新尝试首选分辨率。 */
    if ((data[4] & 0x60U) != ConversionResolution)
    {
        ConfigurationTried = 0;
    }

    /* data[0:1] 为温度，data[2:3] 为芯片自身报警寄存器，data[4] 为配置。
     * 写分辨率时保留芯片原有 TH/TL；它们不等于本项目菜单中的软件报警阈值。
     */
    th = data[2];
    tl = data[3];

    /* 首次最多尝试三次；固定 12 位模块避免每轮反复写配置。 */
    if (ConfigurationTried == 0U)
    {
        ConfigurationTried = 1;
        for (attempt = 0;
             (data[4] & 0x60U) != 0x40U && attempt < 3U;
             attempt++)
        {
            if (DS18B20_Reset() == 0U)
            {
                ConfigurationTried = 0;
                return 0;
            }

            DS18B20_WriteByte(0xCC);
            DS18B20_WriteByte(0x4E);
            DS18B20_WriteByte(th);
            DS18B20_WriteByte(tl);
            /* 0x5F 请求 11 位分辨率；随后读回检查，最终按模块实际配置计时。 */
            DS18B20_WriteByte(0x5F);

            if (DS18B20_ReadScratchpad(data) == 0U)
            {
                ConfigurationTried = 0;
                return 0;
            }
        }
    }

    /* 合法配置为 1F/3F/5F/7F，按真实分辨率执行转换。 */
    if ((data[4] & 0x9FU) != 0x1FU)
    {
        ConfigurationTried = 0;
        return 0;
    }
    ConversionResolution = data[4] & 0x60U;

    if (DS18B20_Reset() == 0U)
    {
        ConfigurationTried = 0;
        return 0;
    }

    DS18B20_WriteByte(0xCC);
    /* Convert T 命令只启动测温，等待与读数安排在后续主循环中。 */
    DS18B20_WriteByte(0x44);

    Converting = 1;
    return 1;
}

uint16_t DS18B20_GetConversionTimeoutMs(void)
{
    return ConversionTimeouts[ConversionResolution >> 5];
}

uint8_t DS18B20_IsConversionDone(void)
{
    if (Converting == 0U)
    {
        return 0;
    }
    return DS18B20_ReadBit();
}

uint8_t DS18B20_ReadTemperature(int16_t *Temperature10)
{
    uint8_t data[9];
    uint16_t raw_bits;
    int16_t raw;
    int32_t value;

    if (Temperature10 == 0 || Converting == 0U)
    {
        return 0;
    }

    /* 本次读取尝试结束后，下次需要重新启动转换。 */
    Converting = 0;

    /* 外部供电时，读到 1 表示转换完成。 */
    if (DS18B20_ReadBit() == 0U)
    {
        ConfigurationTried = 0;
        return 0;
    }

    if (DS18B20_ReadScratchpad(data) == 0U)
    {
        ConfigurationTried = 0;
        return 0;
    }

    /* 检查配置合法，且分辨率与本次启动时一致。 */
    if ((data[4] & 0x9FU) != 0x1FU ||
        (data[4] & 0x60U) != ConversionResolution)
    {
        ConfigurationTried = 0;
        return 0;
    }

    /* 先拼接高低字节并屏蔽当前分辨率下无效的低位，再解释为有符号温度。
     * 例如 raw=400 表示 25℃；负温度由 int16_t 保留符号。
     */
    raw_bits = (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
    raw_bits &= TemperatureMasks[ConversionResolution >> 5];
    raw = (int16_t)raw_bits;

    /* -55*16=-880，125*16=2000；只接受传感器量程内的结果。 */
    if (raw < -880 || raw > 2000)
    {
        return 0;
    }

    /* 原始单位为 1/16 摄氏度，转换成 0.1 摄氏度。 */
    value = (int32_t)raw * 10;
    /* 加半个除数后再整除，实现正负温度到 0.1℃的四舍五入。 */
    if (value >= 0)
    {
        *Temperature10 = (int16_t)((value + 8) / 16);
    }
    else
    {
        *Temperature10 = (int16_t)(-((-value + 8) / 16));
    }

    return 1;
}
