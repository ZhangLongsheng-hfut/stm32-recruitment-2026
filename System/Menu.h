#ifndef __MENU_H
#define __MENU_H

#include <stdint.h>
#include "Input.h"

typedef enum
{
    ALARM_WAIT = 0,    /* 等待首次有效温度 */
    ALARM_NORMAL,      /* 温度正常 */
    ALARM_LOW,         /* 低温报警 */
    ALARM_HIGH,        /* 高温报警 */
    ALARM_FAULT        /* 传感器故障 */
} Alarm_State_t;

/* 初始化并显示根菜单；恢复 RAM 中的上电默认设置。 */
void Menu_Init(void);
void Menu_Process(Input_Event_t Event);

void Menu_SetVoltage(uint16_t Millivolts);
/* 温度参数单位为 0.1℃，Valid=0 显示占位符；不在此接口中进行报警判定。 */
void Menu_SetTemperature(int16_t Temperature10, uint8_t Valid);

uint8_t Menu_GetLEDEnabled(void);
uint16_t Menu_GetServoAngle(void);
/* 串口等外部输入调用；0 表示角度越界，1 表示接受并更新菜单中的目标角度。 */
uint8_t Menu_SetServoAngle(uint16_t Angle);

/* 报警阈值的单位为整数℃，main.c 比较前会乘 10 与温度采样值统一单位。 */
int16_t Menu_GetLowLimit(void);
int16_t Menu_GetHighLimit(void);

void Menu_SetAlarmState(Alarm_State_t State);
void Menu_ShowAlarmIndicator(void);

#endif
