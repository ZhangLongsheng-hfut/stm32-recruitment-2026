# STM32 电创招新项目

基于 STM32F103C8T6 的多功能嵌入式项目，用于 2026 年电子信息创新基地招新考核。

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
Hardware/          硬件驱动
Library/           STM32 标准外设库
Start/             启动文件与系统配置
System/            菜单与延时等模块
User/              主程序与中断配置
Project.uvprojx    Keil 工程文件
```
