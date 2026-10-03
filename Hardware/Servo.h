#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

void PWM_Init(void);
/* 当前配置每计数 1us；Compare 直接写入 TIM2_CH2 比较寄存器。 */
void PWM_SetCompare2(uint16_t Compare);
/* Angle 为整数角度 0~180，内部换算为 500~2500us 脉宽。 */
void Servo_SetAngle(uint16_t Angle);

#endif
