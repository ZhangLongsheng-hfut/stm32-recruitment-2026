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
│   ├── stm32f10x_gpio.c / .h       GPIO 配置与读写接口
│   ├── stm32f10x_rcc.c / .h        外设时钟与复位控制
│   ├── stm32f10x_tim.c / .h        定时器、PWM 与定时中断接口
│   ├── stm32f10x_adc.c / .h        ADC 配置与采样接口
│   ├── stm32f10x_usart.c / .h      USART 配置与收发接口
│   ├── stm32f10x_exti.c / .h       外部中断配置接口
│   ├── misc.c / misc.h             NVIC 中断优先级配置等接口
│   └── 其他标准外设库文件          DMA、I2C、SPI、Flash 等外设支持
│
├── Start/                          启动文件与系统基础配置
│   ├── startup_stm32f10x_md.s      当前工程使用的启动文件与中断向量表
│   ├── 其他 startup_*.s 文件       适用于其他 STM32F10x 型号的启动文件
│   ├── system_stm32f10x.c / .h     系统时钟初始化与时钟频率更新
│   ├── stm32f10x.h                 芯片寄存器、外设与中断编号定义
│   └── core_cm3.c / core_cm3.h     Cortex-M3 内核支持
│
├── System/                         菜单交互与通用功能
│   ├── Menu.c / Menu.h             多级菜单、参数调整与状态显示
│   └── Delay.c / Delay.h           微秒与毫秒延时
│
├── User/                           应用入口与工程配置
│   ├── main.c                      硬件初始化、主循环与各功能任务调度
│   ├── stm32f10x_it.c / .h         Cortex-M3 异常处理函数与声明
│   └── stm32f10x_conf.h            标准外设库头文件配置与断言设置
│
├── Project.uvprojx                 Keil 工程配置与源文件列表
├── Project.uvoptx                  Keil 调试等工程选项
├── STM32F103C8T6开发说明.docx       项目开发说明
└── README.md                       项目介绍与使用方法
```

建议从 `User/main.c` 开始阅读：主循环负责温度采集、报警判断、蜂鸣器节奏、电压采集、流水灯及舵机控制；`System/Menu.c` 负责页面显示和参数设置；`Hardware/` 中的模块负责具体硬件操作。

外设中断按功能放在对应文件中：`TIM4_IRQHandler` 位于 `User/main.c`，编码器的 `EXTI0_IRQHandler` 和 `EXTI1_IRQHandler` 位于 `Hardware/Encoder.c`，`USART1_IRQHandler` 位于 `Hardware/Serial.c`。

## 使用方法

1. 下载项目，按照《STM32F103C8T6开发说明.docx》及驱动代码中的引脚配置连接硬件。
2. 使用 Keil 打开 `Project.uvprojx`，配置对应的芯片支持包和编译器。
3. 编译并下载程序到开发板。
4. 通过旋转编码器和按键操作菜单，查看采集数据并调整参数。

## 当前进度

已完成第一版，包含温度传感器采集与报警功能，后续继续完善。
