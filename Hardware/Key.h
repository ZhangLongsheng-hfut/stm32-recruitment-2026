#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

void Key_Init(void);
void Key_Scan(uint32_t Now);
uint8_t Key_GetNum(void);

#endif
