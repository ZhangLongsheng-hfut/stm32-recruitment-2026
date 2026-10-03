#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

void Key_Init(void);
/* 主循环反复传入毫秒时间，用连续稳定 20ms 的电平确认按键。 */
void Key_Scan(uint32_t Now);
/* 取走一个按下事件：0 无事件，1 ENTER，2 BACK；长按不连续产生事件。 */
uint8_t Key_GetNum(void);

#endif
