#ifndef __MENU_H
#define __MENU_H

#include <stdint.h>
#include "Input.h"

/* 初始化菜单，显示主菜单。
   调用前需要完成 OLED_Init()。 */
void Menu_Init(void);

/* 处理一次输入。
   Event 使用现有 Input_GetEvent() 返回的枚举。 */
void Menu_Process(Input_Event_t Event);

/* 后续ADC采样后调用，更新电压显示。
   Millivolts单位为毫伏，例如1650表示1.65V。
   本项目电压范围为0～3300mV。 */
void Menu_SetVoltage(uint16_t Millivolts);

/* 后续DS18B20采样后调用，更新温度显示。
   Temperature10单位为0.1℃，例如253表示25.3℃。
   Valid：1表示数据有效，0表示无有效数据。 */
void Menu_SetTemperature(int16_t Temperature10, uint8_t Valid);

/* 后台流水灯程序读取此设定：
   返回1表示要求运行，返回0表示要求停止。 */
uint8_t Menu_GetLEDEnabled(void);

/* 后台舵机程序读取已确认的目标角度，范围0～180。 */
uint16_t Menu_GetServoAngle(void);

/* 后续串口收到合法角度命令时调用。
   返回1表示设置成功，0表示角度超出范围。 */
uint8_t Menu_SetServoAngle(uint16_t Angle);

/* 后台温度报警程序读取已确认的上下限，单位为℃。 */
int16_t Menu_GetLowLimit(void);
int16_t Menu_GetHighLimit(void);

#endif
