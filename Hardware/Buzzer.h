#ifndef BUZZER_H
#define BUZZER_H

/* 初始化 PB11，默认关闭蜂鸣器。 */
void Buzzer_Init(void);

/* 开启蜂鸣器：PB11 输出低电平。 */
void Buzzer_On(void);

/* 关闭蜂鸣器：PB11 输出高电平。 */
void Buzzer_Off(void);

#endif
