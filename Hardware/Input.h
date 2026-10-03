#ifndef __INPUT_H
#define __INPUT_H

#include "stm32f10x.h"

/* 菜单接收的统一事件类型；具体按键和编码器方向在 Input.c 中映射。 */
typedef enum
{
    INPUT_NONE = 0,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_ENTER,
    INPUT_BACK

} Input_Event_t;

Input_Event_t Input_GetEvent(void);

#endif
