# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

基于 STM32F103C8T6 的 CAN 总线通信项目，配有 OLED 显示和按键交互。使用 STM32CubeMX 生成初始化代码，Keil MDK-ARM V5.32 编译，HAL 库驱动。

## 硬件平台

- **MCU**: STM32F103C8T6（Cortex-M3, 72MHz）
- **时钟**: HSE 8MHz 外部晶振 → PLL×9 → SYSCLK 72MHz
- **CAN**: PA11(RX) / PA12(TX), 500kbps (Prescaler=6, BS1=8TQ, BS2=3TQ, SJW=2TQ), 正常模式
- **OLED**: 128×64, I2C 接口, 软件模拟 I2C (PB8=SCL, PB9=SDA), 开漏输出
- **按键**: PB1 (低电平有效), PB11 (低电平有效), 另有 PB12/PB13 预留
- **调试**: SWD (PA13=SWDIO, PA14=SWCLK)

## 编译与烧录

- **IDE**: 用 Keil MDK-ARM V5.32 打开 `MDK-ARM/STM32-CAN.uvprojx`
- **编译**: Keil IDE 内 Build (F7)
- **烧录**: 通过 SWD 接口使用 ST-Link 或 J-Link 下载
- **CubeMX 重新生成**: 打开 `STM32-CAN.ioc`（CubeMX 6.17.0），修改配置后重新生成代码。注意 `KeepUserCode=true`，用户代码在 `USER CODE BEGIN/END` 标记内的部分会被保留

## 代码架构（3 层结构）

```
User/
├── Device/          # 设备驱动层 - 硬件抽象
│   ├── oled_device  # OLED 驱动（软件 I2C + 显存缓冲区）
│   ├── botton       # 按键驱动（GPIO 读取 + 去抖）
│   └── can          # CAN 收发封装
├── Middle/          # 中间层 - 事件处理
│   └── button_mid   # 按键事件检测（单击/双击/长按）
└── APP/             # 应用层
    └── task         # 主任务循环 + CAN 服务初始化
```

**关键设计模式**:

- **显存缓冲区**: OLED 所有绘制操作先写入 `aucOledBuffer[]`，最后调用 `vOledRefreshFromBuffer()` 整屏刷新
- **设备枚举模式**: 通过 `emOledDevNumTdf`/`emButDevNumTdf` 枚举实现多设备实例管理，各设备静态参数和运行参数存储在全局结构体数组中
- **按键事件状态机**: `button_mid.c` 实现单击/双击/长按的完整状态机，依赖 `HAL_GetTick()` 计时。使用 `vBtnEventClear()` 消费事件后必须手动清除
- **CAN 轮询模式**: 当前使用轮询方式检查 FIFO0 是否有数据 (`HAL_CAN_GetRxFifoFillLevel`)，未使用中断模式。滤波器当前配置为"全通"（掩码全 0）

## 核心配置文件

`User/Device/project.h` 是所有用户模块的全局配置文件，包含：
- 按键引脚定义、设备数量 (`BUTTON_NUM`)
- LED 设备定义
- OLED 分辨率 (`OLED_POINT_WIDTH`/`OLED_POINT_HIGH`)、缓冲区尺寸
- 字号配置 (`EM_FONT_SIZE`)
- 页面枚举 (`emAllMenuTdf`) 和贪吃蛇游戏状态枚举

## 添加新功能的注意事项

- 新增 `.c/.h` 文件需手动添加到 Keil 工程中（在 MDK-ARM 界面 Project → Manage → Project Items 添加）
- CubeMX 重新生成时，用户代码必须写在 `/* USER CODE BEGIN ... */` 和 `/* USER CODE END ... */` 之间，否则会被覆盖
- `main.c` 中的外设初始化顺序：`HAL_Init()` → `SystemClock_Config()` → `MX_GPIO_Init()` → `MX_CAN_Init()` → 用户初始化
- OLED 使用直接寄存器操作 (`BSRR`) 而非 HAL 函数来优化 I2C 时序速度
- CAN 在 `main.c` 的 `MX_CAN_Init()` 中已将模式从 CubeMX 默认的回环模式改为 `CAN_MODE_NORMAL`（正常通信模式）
