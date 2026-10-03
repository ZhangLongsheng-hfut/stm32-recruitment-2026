#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

extern char Serial_RxPacket[100];
extern volatile uint8_t Serial_RxFlag;

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendString(char *String);

/* 返回0～180表示合法角度，返回-1表示命令错误 */
int16_t Serial_ParseAngle(const char *Command);

#endif
