#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

void PWM_Init(void);
void PWM_SetCompare2(uint16_t Compare);
void Servo_SetAngle(uint16_t Angle);

#endif
