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

void Menu_Init(void);
void Menu_Process(Input_Event_t Event);

void Menu_SetVoltage(uint16_t Millivolts);
void Menu_SetTemperature(int16_t Temperature10, uint8_t Valid);

uint8_t Menu_GetLEDEnabled(void);
uint16_t Menu_GetServoAngle(void);
uint8_t Menu_SetServoAngle(uint16_t Angle);

int16_t Menu_GetLowLimit(void);
int16_t Menu_GetHighLimit(void);

void Menu_SetAlarmState(Alarm_State_t State);
void Menu_ShowAlarmIndicator(void);

#endif
