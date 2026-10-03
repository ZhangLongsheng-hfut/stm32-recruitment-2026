#ifndef __ENCODER_H
#define __ENCODER_H

void Encoder_Init(void);
/* 读取并清零旋转累计量，正数/负数表示两个旋转方向。 */
int16_t Encoder_Get(void);

#endif
