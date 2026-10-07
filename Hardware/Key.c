#include "stm32f10x.h"
#include "Key.h"

/* PA5 为 ENTER，PA3 为 BACK，均为上拉输入、按下接地。*/
static uint8_t LastRaw = 0;  //保存最近观察到的按键状态，包括尚未确认的变化
static uint8_t Stable = 0;   //保存已经持续稳定至少 20 ms、确认可信的按键状态
static uint8_t Pending = 0;  //保存已确认的按下事件，等其他函数取走
static uint32_t ChangedAt = 0;  //保存原始状态最后一次变化时的毫秒计数

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
	/* 按键的GPIO口初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

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
    if ((uint32_t)(Now - ChangedAt) >= 20U && raw != Stable)
    {
        /* 稳定按下时只产生一次事件，长按不重复。 */
        /* raw & ~Stable 只保留本次新按下的位；松开按键不会生成事件。 */
        Pending |= (uint8_t)(raw & (uint8_t)~Stable);
        Stable = raw;
    }
}

/* 保持原接口，Input.c可以继续调用它。 */
/* 每次取走一个事件并清除对应位；同时有两个事件时，先返回 ENTER。 */
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
