#include "stm32f10x.h"
#include "Buzzer.h"

#define BUZZER_PORT   GPIOB
#define BUZZER_PIN    GPIO_Pin_11
#define BUZZER_CLOCK  RCC_APB2Periph_GPIOB

void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(BUZZER_CLOCK, ENABLE);

    /* 先置高，再配置输出模式，避免初始化时误响。 */
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);

    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
}

void Buzzer_On(void)
{
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Off(void)
{
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}
