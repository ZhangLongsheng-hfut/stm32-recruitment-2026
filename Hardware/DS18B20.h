#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

/* 初始化 PB10，并检测传感器：1 成功，0 失败。 */
uint8_t DS18B20_Init(void);

/* 配置为 11 位并启动转换：1 成功，0 失败。 */
uint8_t DS18B20_StartConvert(void);

/*
 * 启动转换后至少等待 380ms，再调用此函数。
 * 返回 1：读取成功，Temperature10 单位为 0.1 摄氏度。
 * 返回 0：读取失败，输出参数保持原值。
 */
uint8_t DS18B20_ReadTemperature(int16_t *Temperature10);

#endif
