#include "stm32f10x.h"
#include "LED.h"

/* 四个LED使用的引脚 */
#define LED_PINS (GPIO_Pin_12 | GPIO_Pin_13 | \
                  GPIO_Pin_14 | GPIO_Pin_15)

/*
 * 下一次要点亮的LED编号：
 * 0：PB12
 * 1：PB13
 * 2：PB14
 * 3：PB15
 */
static uint8_t LED_Index = 0;


/**
 * 函数：LED初始化
 * 接线：GPIO → 限流电阻 → LED正极，LED负极 → GND
 * 说明：高电平点亮，低电平熄灭
 */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启GPIOB时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 先将四个引脚的输出电平设为低电平 */
    GPIO_ResetBits(GPIOB, LED_PINS);

    /* 只配置PB12～PB15，不影响GPIOB上的其他设备 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = LED_PINS;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 流水灯从PB12开始 */
    LED_Index = 0;
}


/**
 * 函数：流水灯前进一步
 * 说明：每调用一次，只点亮其中一盏LED
 * 顺序：PB12 → PB13 → PB14 → PB15 → PB12……
 * 调用：流水灯启用时，每100ms调用一次
 */
void LED_Run(void)
{
    uint16_t LED_Pin;

    /*
     * GPIO_Pin_12左移LED_Index位，得到本次要点亮的引脚。
     *
     * 左移0位：GPIO_Pin_12
     * 左移1位：GPIO_Pin_13
     * 左移2位：GPIO_Pin_14
     * 左移3位：GPIO_Pin_15
     */
    LED_Pin = (uint16_t)(GPIO_Pin_12 << LED_Index);

    /* 先熄灭四个LED */
    GPIO_ResetBits(GPIOB, LED_PINS);

    /* 再点亮本次选中的LED */
    GPIO_SetBits(GPIOB, LED_Pin);

    /* 准备下一盏灯 */
    LED_Index++;

    /* 四盏灯走完后，重新从第一盏开始 */
    if (LED_Index >= 4)
    {
        LED_Index = 0;
    }
}


/**
 * 函数：关闭流水灯
 * 说明：熄灭全部LED，下次启动时从PB12开始
 */
void LED_Off(void)
{
    GPIO_ResetBits(GPIOB, LED_PINS);

    LED_Index = 0;
}
