# STM32-CAN 总线驱动验证与多节点通信

基于 STM32F103C8T6（bxCAN）+ TJA1050 收发器的 CAN 2.0B 双节点通信验证项目。包含分层 BSP 驱动、四种硬件滤波器配置实验、标准帧/扩展帧/遥控帧收发，以及定时/触发/请求三种传输策略，配套 128×64 OLED 显示与按键交互。

[![Platform](https://img.shields.io/badge/MCU-STM32F103C8T6-blue)]()
[![Bus](https://img.shields.io/badge/CAN-500kbps-orange)]()
[![IDE](https://img.shields.io/badge/Toolchain-Keil%20MDK%205-green)]()

> 仓库地址：https://github.com/DingDa-H/stm32-CAN

---

## 1. 硬件平台

| 项 | 配置 |
|---|---|
| MCU | STM32F103C8T6（Cortex-M3，72MHz） |
| 时钟 | HSE 8MHz → PLL×9 → SYSCLK 72MHz，APB1 = 36MHz |
| CAN 收发器 | TJA1050（5V 供电，3.3V IO） |
| CAN 引脚 | PA11（CAN_RX）/ PA12（CAN_TX） |
| OLED | 128×64，软件模拟 I2C：PB8=SCL、PB9=SDA（开漏输出） |
| 按键 | PB1 / PB11（低有效）、PB12 / PB13（高有效），低电平触发 |
| 调试 | SWD（PA13/PA14） |

### CAN 位时序计算（500kbps）

```
量化时间 tq = Prescaler / F_APB1 = 6 / 36MHz
位时间     = (1 SYNC + 8 BS1 + 3 BS2) tq = 12 tq
波特率     = F_APB1 / (12 × 6) = 500kbps
采样点     = (1 + 8) / 12 ≈ 75%
SJW        = 2 tq
```

外设初始化关键项（`Core/Src/main.c` 的 `MX_CAN_Init`）：
- 工作模式 `CAN_MODE_NORMAL`（早期回环自测阶段使用 `CAN_MODE_LOOPBACK` 校验过波特率与时序，见提交 `6b747cc`、`688bc84`）
- `AutoRetransmission = DISABLE`：单次发送语义，重发策略交由应用层决定

---

## 2. 软件架构（三层）

```
User/
├── Device/                 # 驱动层：硬件抽象
│   ├── can.c/h             #   CAN 收发封装（发送结构体 + 中断接收缓存）
│   ├── oled_device.c/h     #   OLED 驱动（软件 I2C + 显存缓冲，BSRR 直接寄存器操作）
│   ├── botton.c/h          #   按键 GPIO 读取 + 非阻塞消抖
│   └── project.h           #   引脚/字号/设备枚举等全局配置
├── Middle/
│   └── button_mid.c        #   按键事件状态机（单击/双击/长按，HAL_GetTick 计时）
└── APP/
    └── task.c              #   CAN 服务初始化、发送帧表、主循环收发策略
```

### 接收链路：中断 + 标志位 + 主循环搬运

```
FIFO0 收到报文
  └─ HAL_CAN_RxFifo0MsgPendingCallback（ISR）
       ├─ HAL_CAN_GetRxMessage → 存入驱动层静态缓存 s_aucRxBuf / s_rxHeader
       └─ 置接收标志 ucRxFlag = 1
主循环 vtask()
  ├─ 读标志 → 拷贝帧头/数据副本 → 清标志
  ├─ 按 IDE/RTR/ID 分类处理
  └─ 写 OLED 显存 → 整屏刷新
```

设计原则：**中断里只取数、不刷屏**，总线负载高时不影响位时序响应。

### 发送封装

`stCanTxParamTdf` 统一描述一帧（ID / DLC / 数据 / IDE / RTR），`vMyCan_Transmit` 内部根据 `IDE` 自动落到 `StdId` 或 `ExtId`，调用方无需关心 HAL 帧头细节。

---

## 3. 帧类型验证矩阵

项目中实际收发过的帧组合（发送帧表见 `User/APP/task.c`）：

| 帧 ID | IDE | RTR | 用途 |
|---|---|---|---|
| 0x555 | 标准 | 数据 | 触发传输 |
| 0x666 | 标准 | 数据 | 请求-响应应答帧 |
| 0x777 | 标准 | **遥控** | 请求帧（收到后对端自动应答 0x666） |
| 0x789 | 标准 | 遥控 | 按键主动发起的请求帧 |
| 0x0789AB0F | **扩展** | 数据 | 定时上报帧 |
| 0x123456xx / 0x0789ABxx | 扩展 | 数据 | 滤波器实验用多 ID 帧组 |

接收端对 IDE / RTR / ID 三要素做分类识别后分发显示（`vtask()` 接收分支）。

---

## 4. 传输策略（双节点）

发送节点与接收节点为同一工程的两种编译形态（对应函数体内的注释切换，提交 `98a0c3b`、`8370e6c`）：

| 策略 | 触发方式 | 帧 | 对端行为 |
|---|---|---|---|
| **定时发送** | TIM2 100ms 更新中断置标志 | 0x0789AB0F 扩展数据帧，数据自增 | 接收端 OLED "Tim" 行显示 |
| **触发发送** | 按键单击（事件状态机识别） | 0x555 标准数据帧 | "Tri" 行显示 |
| **请求发送** | 收到 0x777 遥控帧 | 自动应答 0x666 数据帧 | "Req" 行显示 |

请求-响应流程完整演示了遥控帧（RTR）语义：请求帧只携带 ID，应答方收到后回送同 ID 数据。

---

## 5. 硬件滤波器实验记录

bxCAN 滤波器实验按"配置 → 双节点实测 → 提交"的流程逐个完成，每个提交保留一种完整配置。**当前 HEAD 生效的是全通滤波器**（掩码全 0，便于接收节点显示所有报文），四种筛选配置保存在历史提交中，可 checkout 复现：

| 模式 | Scale | Mode | 实验内容 | Commit |
|---|---|---|---|---|
| 16 位列表 | 16BIT | IDLIST | 指定 ID（0x234/0x345/0x567）+ 指定遥控帧过滤 | `f56a8cf` |
| 16 位掩码 | 16BIT | IDMASK | 高 5 位通配过滤一类 ID（0x2_0/0x3_0 族） | `37da406` |
| 32 位列表 | 32BIT | IDLIST | 扩展帧指定 ID（0x12345600 等，含 IDE/RTR 位编码） | `f3649a7` |
| 32 位掩码 | 32BIT | IDMASK | 扩展帧段过滤（0x0789ABxx 族，29 位高段比对） | `15d2af2` |

> ⚠️ 待复核项：16 位模式下滤波器寄存器的 IDE/RTR 位定义为 bit2/bit1（RM0008），历史实验代码中使用
> `|0x10`/`|0x08`（EXID 域）承载帧类型过滤，重新整理为可切换配置时需在台面上逐模式复测并记录结果。

### 仲裁机制实测

双节点同时发送不同 ID 报文（低 11/29 位更小的 ID 优先级更高），观测到高优先级报文无损赢得总线、低优先级报文自动重发，验证了 CAN 的非破坏性位仲裁机制。

---

## 6. 编译与使用

1. Keil MDK-ARM V5.32 打开 `MDK-ARM/STM32-CAN.uvprojx`，F7 编译
2. ST-Link / J-Link 通过 SWD 烧录
3. 双节点联调：两块板 PA11/PA12 各接 TJA1050，H 线并 H 线、L 线并 L 线，两端各 120Ω 终端电阻
4. CubeMX 重新生成：打开 `STM32-CAN.ioc`，用户代码全部位于 `USER CODE BEGIN/END` 标记内，不会丢失
5. 新增 `.c/.h` 需手动加入 Keil 工程分组

---

## 7. 已知限制与路线图

当前版本的诚实记录（也是下一步的输入）：

- [ ] 接收标志 `ucRxFlag` 建议补充 `volatile` 修饰（ISR 与主循环共享）
- [ ] 驱动层为单帧缓存，高总线负载下后到帧会覆盖未消费的帧 → 计划改为帧队列
- [ ] `HAL_CAN_AddTxMessage` 返回值未检查，3 个邮箱全忙时帧会静默丢弃 → 应用层补重试策略
- [ ] 无错误处理：未使能 AutoBusOff，未实现 `HAL_CAN_ErrorCallback`，未监控 TEC/REC
      → **下一步：Bus-Off 检测与退避恢复、报文超时监控、故障码记录**
- [ ] 四种滤波器配置散落在历史提交 → **下一步：宏开关整合为可切换测试剖面（过滤器 + 匹配发送帧表联动），逐模式复测留档**
- [ ] 帧数据完整性目前仅校验 DLC，无 CRC/计数域 → 计划为上报帧增加帧计数

---

## 8. 相关项目

- 多任务系统与分层架构实践：[DingDa-H/stm32f1_rtos](https://github.com/DingDa-H/stm32f1_rtos)（FreeRTOS + 四层架构，本项目接收端代码结构与其 Device/Middle/APP 分层同源）
