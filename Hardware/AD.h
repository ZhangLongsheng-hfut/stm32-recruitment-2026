#ifndef __AD_H
#define __AD_H

#include <stdint.h>

void AD_Init(void);             /* 初始化PA0和ADC1 */
uint16_t AD_GetValue(void);     /* 获取ADC原始值 */
uint16_t AD_GetVoltage(void);   /* 获取电压，单位为毫伏 */

#endif
