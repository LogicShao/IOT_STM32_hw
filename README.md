# 直流电能表开发板实验

STM32F070CBT6TR + HT7017 直流电能表开发板，兰州大学 2026 春 IoT 课程实验。

## 环境

- **MCU**: STM32F070CBT6TR (Cortex-M0, 48MHz, 64KB Flash)
- **工具链**: arm-none-eabi-gcc + CMake + Ninja
- **IDE**: CLion + STM32CubeMX

## 构建

```bash
# Debug
cmake --preset Debug
cmake --build --preset Debug

# Release
cmake --preset Release
cmake --build --preset Release
```

## 实验模块

| 模块 | Demo 文件 | 说明 |
|---|---|---|
| 红绿灯 | `Core/Src/Demo/demo_trafficlight_main.c` | FSM 状态机，按键调时长，倒计时切换 |
| 模拟电梯 | `Core/Src/Demo/demo_elevator_main.c` | 8 状态 FSM，楼层选择，自动归位 |
| 直流电表 | `Core/Src/Demo/demo_energymeter_main.c` | HT7017 采样，数码管显示，RS485/蓝牙发送 |

使用方法：将对应 demo 文件复制到 `Core/Src/main.c` 即可编译运行。

## 外设

LED、数码管 (GN1650)、蜂鸣器 (PWM)、按键 (EXTI)、RS485、HC-05 蓝牙、EEPROM (M24256E)、HT7017 计量芯片。

## 实验要求

详见 `class_doc/lab_req.md`
