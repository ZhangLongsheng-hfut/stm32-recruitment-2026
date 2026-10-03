#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

/* 初始化 PB10 并检测传感器：1 存在，0 失败。 */
uint8_t DS18B20_Init(void);

/* 优先配置 11 位；未生效时使用读回的合法分辨率。1 成功，0 失败。 */
uint8_t DS18B20_StartConvert(void);

/* 当前转换的超时上限（ms），根据实际分辨率确定。 */
uint16_t DS18B20_GetConversionTimeoutMs(void);

/* 轮询转换完成状态：1 完成，0 未完成或未启动；不会结束本次转换。 */
uint8_t DS18B20_IsConversionDone(void);

/*
 * 启动后轮询完成状态，或达到转换超时上限时，再调用此函数。
 * 返回 1：读取成功，Temperature10 单位为 0.1 摄氏度。
 * 返回 0：读取失败，输出参数保持原值。
 */
uint8_t DS18B20_ReadTemperature(int16_t *Temperature10);

#endif
