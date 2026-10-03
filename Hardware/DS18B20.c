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

static uint8_t Converting = 0;

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

    return (DS18B20_CRC8(data, 8) == data[8]) ? 1U : 0U;
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
    return DS18B20_Reset();
}

uint8_t DS18B20_StartConvert(void)
{
    uint8_t data[9];

    Converting = 0;

    /* 每次启动都配置分辨率，兼顾拔下后重新接入的情况。 */
    if (DS18B20_ReadScratchpad(data) == 0U)
    {
        return 0;
    }

    if (DS18B20_Reset() == 0U)
    {
        return 0;
    }

    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x4E);     /* Write Scratchpad。 */
    DS18B20_WriteByte(data[2]);  /* 保留传感器原有的 TH。 */
    DS18B20_WriteByte(data[3]);  /* 保留传感器原有的 TL。 */
    DS18B20_WriteByte(0x5F);     /* 11 位分辨率，不写 EEPROM。 */

    if (DS18B20_Reset() == 0U)
    {
        return 0;
    }

    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);     /* Convert T：启动转换。 */

    Converting = 1;
    return 1;
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
        return 0;
    }

    if (DS18B20_ReadScratchpad(data) == 0U)
    {
        return 0;
    }

    /* 确认此次数据确实使用 11 位分辨率。 */
    if ((data[4] & 0x60U) != 0x40U)
    {
        return 0;
    }

    raw_bits = (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
    raw_bits &= 0xFFFEU;  /* 11 位模式下 bit0 未定义。 */
    raw = (int16_t)raw_bits;

    if (raw < -880 || raw > 2000)
    {
        return 0;
    }

    /* 原始单位为 1/16 摄氏度，转换成 0.1 摄氏度。 */
    value = (int32_t)raw * 10;
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
