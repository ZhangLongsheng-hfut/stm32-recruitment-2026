#ifndef __INPUT_H
#define __INPUT_H

#include "stm32f10x.h"

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
