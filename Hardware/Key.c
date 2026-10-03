#include "stm32f10x.h"
#include "Key.h"

static uint8_t LastRaw = 0;
static uint8_t Stable = 0;
static uint8_t Pending = 0;
static uint32_t ChangedAt = 0;

/* bit0：ENTER，bit1：BACK。 */
static uint8_t Key_ReadRaw(void)
{
    uint8_t value = 0;

    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == 0)
    {
        value |= 1U;
    }

    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3) == 0)
    {
        value |= 2U;
    }

    return value;
}

void Key_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    LastRaw = Key_ReadRaw();
    Stable = LastRaw;
    Pending = 0;
    ChangedAt = 0;
}

/* 主循环持续调用，Now为当前毫秒数。 */
void Key_Scan(uint32_t Now)
{
    uint8_t raw = Key_ReadRaw();

    if (raw != LastRaw)
    {
        LastRaw = raw;
        ChangedAt = Now;
    }

    /* 连续稳定20ms，才确认状态变化。 */
    if ((uint32_t)(Now - ChangedAt) >= 20U &&
        raw != Stable)
    {
        /* 稳定按下时只产生一次事件，长按不重复。 */
        Pending |= (uint8_t)(raw & (uint8_t)~Stable);
        Stable = raw;
    }
}

/* 保持原接口，Input.c可以继续调用它。 */
uint8_t Key_GetNum(void)
{
    if ((Pending & 1U) != 0U)
    {
        Pending &= (uint8_t)~1U;
        return 1;
    }

    if ((Pending & 2U) != 0U)
    {
        Pending &= (uint8_t)~2U;
        return 2;
    }

    return 0;
}
