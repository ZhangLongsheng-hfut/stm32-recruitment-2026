# STM32 电创招新项目

基于 STM32F103C8T6 的多功能嵌入式项目，用于 2026 年电创招新考核。

## 主要功能

- OLED 菜单显示，支持旋转编码器和按键操作。
- DS18B20 温度采集与显示。
- 温度上下限设置、超限报警及传感器故障提示。
- 蜂鸣器报警、LED 流水灯控制。
- ADC 电压采集与显示。
- 舵机角度调节，支持串口控制。

## 开发环境

- 主控：STM32F103C8T6
- 开发工具：Keil µVision 5
- 编译器：ARM Compiler 5.06 update 5
- 固件库：STM32F10x 标准外设库

## 项目结构

```text
stm32-recruitment-2026/
├── Hardware/                       硬件驱动与输入处理
│   ├── AD.c / AD.h                 ADC 初始化、电压采集与换算
│   ├── Buzzer.c / Buzzer.h         蜂鸣器初始化与开关控制
│   ├── DS18B20.c / DS18B20.h       温度转换、完成检测与温度读取
│   ├── Encoder.c / Encoder.h       旋转编码器初始化与外部中断处理
│   ├── Input.c / Input.h           将按键和编码器操作转换为菜单事件
│   ├── Key.c / Key.h               按键扫描、消抖与按下事件读取
│   ├── LED.c / LED.h               LED 初始化、流水灯与关闭控制
│   ├── OLED.h                      OLED 显示接口声明
│   ├── OLED_ChineseSupport.c       OLED 驱动实现，含模拟 I2C 和字符绘制
│   ├── OLED_Font.h                 OLED 字模数据
│   ├── Serial.c / Serial.h         串口收发、命令解析与舵机角度指令处理
│   └── Servo.c / Servo.h           PWM 输出与舵机角度控制
│
├── Library/                        STM32F10x 标准外设库
│
├── Start/                          启动文件与系统基础配置
│
├── System/                         菜单交互与通用功能
│   ├── Menu.c / Menu.h             多级菜单、参数调整与状态显示
│   └── Delay.c / Delay.h           微秒与毫秒延时
│
├── User/                           应用入口与工程配置
│   ├── main.c                      硬件初始化、主循环与各功能任务调度
│   ├── stm32f10x_it.c / .h
│   └── stm32f10x_conf.h
│
├── Project.uvprojx                 Keil 工程配置与源文件列表
├── Project.uvoptx                  Keil 调试等工程选项
├── STM32F103C8T6开发说明.docx       项目开发说明
└── README.md                       项目介绍与使用方法
```
