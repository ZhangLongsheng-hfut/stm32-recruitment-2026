#include "stm32f10x.h"                  // Device header
#include "Input.h"
#include "Encoder.h"
#include "Key.h"
#include "LED.h"


Input_Event_t Input_GetEvent(void)
{
    int16_t Encoder_Num;
    uint8_t KeyNum;

    Encoder_Num = Encoder_Get();
    KeyNum = Key_GetNum();

    /* ENTER按键 */
    if (KeyNum == 1)
    {
        return INPUT_ENTER;
    }

    /* BACK按键 */
    if (KeyNum == 2)
    {
        return INPUT_BACK;
    }

    /* 顺时针 */
    if (Encoder_Num > 0)
    {
        return INPUT_DOWN;
    }

    /* 逆时针 */
    if (Encoder_Num < 0)
    {
        return INPUT_UP;
    }

    /* 什么操作都没有 */
    return INPUT_NONE;
}

