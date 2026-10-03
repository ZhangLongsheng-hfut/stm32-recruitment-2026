#include "stm32f10x.h"                  // Device header
#include "Input.h"
#include "Encoder.h"
#include "Key.h"
#include "LED.h"


/* 将硬件输入统一为 NONE/UP/DOWN/ENTER/BACK，菜单不需要关心具体引脚。
 * 每次只返回一个事件：按键优先；旋转累计量只取符号，不按累计步数连续返回。
 * Encoder_Get 在判断按键前已取走增量，同轮有按键时该次旋转不会留到下一轮。
 */
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

