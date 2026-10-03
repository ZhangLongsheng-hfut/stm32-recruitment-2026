#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
/**
 * 显示16x16中文，兼容UTF-8和GBK；可混合8x16的ASCII字符。
 * Line：1~4；Column：起始汉字列1~8（与ShowString的ASCII列1~16不同）。
 * 支持：你 好 温 度 设 置 菜 单 欢 迎 加 入 电 压 舵 机 角 ！。
 * 示例：OLED_ShowChinese(1, 1, "欢迎加入！");
 * 每行最多8个汉字；到达右边界停止，不自动换行；未收录字符跳过。
 */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, const char *String);

#endif
