# STM32-CAN 总线驱动验证与多节点通信

基于 STM32F103C8T6（bxCAN）+ TJA1050 的 CAN 2.0B 三节点通信验证项目（1 发 2 收）。同一份源码通过两个编译期宏生成发送板或接收板固件，支持全通接收和四种硬件滤波器测试配置，覆盖标准帧、扩展帧、遥控帧，以及定时发送、按键触发发送、请求—应答、遥控帧发送四类收发行为，配套 128×64 OLED 显示与按键交互。

[![Platform](https://img.shields.io/badge/MCU-STM32F103C8T6-blue)]()
[![Bus](https://img.shields.io/badge/CAN-500kbps-orange)]()
[![Toolchain](https://img.shields.io/badge/Keil%20MDK-ARM%20V5-green)]()

> 仓库地址：https://github.com/DingDa-H/stm32-CAN

---

## 1. 硬件平台

| 项 | 配置 |
|---|---|
| MCU | STM32F103C8T6（Cortex-M3，72MHz） |
| 时钟 | HSE 8MHz → PLL×9 → SYSCLK 72MHz，APB1 36MHz |
| CAN 收发器 | TJA1050（5V 供电，3.3V IO） |
| CAN 引脚 | PA11 = CAN_RX，PA12 = CAN_TX |
| OLED | 128×64，软件 I2C：PB8 = SCL，PB9 = SDA |
| 按键 | PB11 = KEY_UP，PB1 = KEY_DOWN，低电平有效；PB12/PB13 已在按键模块注册，当前 CAN 业务不使用 |
| 调试 | SWD（PA13/PA14） |

### CAN 位时序（500kbps）

```
量化时间 tq = Prescaler / F_APB1 = 6 / 36MHz
位时间     = (1 SYNC + 8 BS1 + 3 BS2) tq = 12 tq
波特率     = F_APB1 / (12 × 6) = 500kbps
采样点     = (1 + 8) / 12 ≈ 75%
SJW        = 2 tq
```

`Core/Src/main.c` 的 `MX_CAN_Init` 关键项：

- 工作模式 `CAN_MODE_NORMAL`
- `AutoBusOff = DISABLE`：控制器不自动退出 Bus-Off，为后续软件恢复流程预留
- `AutoRetransmission = DISABLE`：单次发送语义，仲裁失败或出错后不自动重发；当前应用层没有补发逻辑
- `Prescaler = 6`，`SJW = 2TQ`，`BS1 = 8TQ`，`BS2 = 3TQ`

---

## 2. 软件架构

按设备驱动层 / 中间件 / 应用层组织，接口边界参照 AUTOSAR 的分层思路，但不是完整的 AUTOSAR 协议栈：CAN 驱动之上直接接应用层，没有独立的 CanIf、PduR、Com 模块。

```
STM32-CAN/
├── Core/                            CubeMX 生成的启动、时钟、外设初始化
├── Drivers/                         HAL 库与 CMSIS
├── MDK-ARM/                         Keil 工程与编译产物
└── User/
    ├── Device/                      设备驱动层
    │   ├── can_device.c/h           CAN 发送、滤波器下发、FIFO0 接收中断缓存、接收标志与取数接口
    │   ├── tim_device.c/h           TIM2 100ms 周期标志
    │   ├── oled_device.c/h          OLED 驱动（软件 I2C + 显存缓冲）
    │   ├── oled_font.c/h            字库
    │   ├── botton.c/h               按键 GPIO 电平读取
    │   └── project.h                编译期配置：测试配置、节点类型、OLED/按键公共宏
    ├── Middle/
    │   └── button_mid.c/h           按键事件状态机（消抖、单击/双击/长按）
    └── APP/
        ├── can_app.c/h              CAN 应用层：滤波参数计算、发送板/接收板业务
        ├── can_test.c/h             测试帧表、按索引发送、帧表长度查询
        └── task.c/h                 任务入口：设备初始化、按键扫描、按节点类型分发
```

`*.bak` 是旧版本备份，不在 Keil 工程中，不参与编译。

### 初始化与运行流程

```text
main()
 ├─ MX_GPIO_Init / MX_CAN_Init / MX_TIM2_Init
 ├─ HAL_TIM_Base_Start_IT(&htim2)
 ├─ vOledInit / Button_Init
 └─ vCanAppInit()
      └─ 按 CAN_TEST_PROFILE 填写 CAN_FilterTypeDef
           └─ vCanDeviceInit()
                └─ HAL_CAN_ConfigFilter / HAL_CAN_Start / HAL_CAN_ActivateNotification

while (1) vtask()
 ├─ vBtnExecute()
 └─ 按 CAN_NODE_ROLE 分支
      ├─ CAN_NODE_SENDER   → vCanAppSenderTask()
      └─ CAN_NODE_RECEIVER → vCanAppReceiverTask()
```

接收链路：FIFO0 中断 `HAL_CAN_RxFifo0MsgPendingCallback` 只把帧头和 8 字节数据存入驱动层静态缓存并置接收标志；主循环读取、清标志后再做 ID 判断和 OLED 刷新。中断里不刷屏、不延时。

### 滤波参数的下发路径

```
project.h 的 CAN_TEST_PROFILE
  → can_app.c 的 vCanAppInit()：填写 CAN_FilterTypeDef（Bank 0、FIFO0、ID/掩码/模式/位宽）
  → can_device.c 的 vCanDeviceInit()：HAL_CAN_ConfigFilter / Start / ActivateNotification
```

四种滤波配置的 ID、掩码、模式和位宽全部集中在 `vCanAppInit()`；`vCanDeviceInit()` 只负责把结构体交给 HAL，不再参与参数拆分。

---

## 3. 编译期配置

`User/Device/project.h` 末尾有两个宏：

| 宏 | 取值 | 作用 |
|---|---|---|
| `CAN_TEST_PROFILE` | `CAN_TEST_ALL_PASS` / `CAN_TEST_16BIT_LIST` / `CAN_TEST_16BIT_MASK` / `CAN_TEST_32BIT_LIST` / `CAN_TEST_32BIT_MASK` | 选择滤波器配置和发送帧表 |
| `CAN_NODE_ROLE` | `CAN_NODE_SENDER` / `CAN_NODE_RECEIVER` | 选择节点类型 |

常用固件组合：

| 场景 | 板 | `CAN_TEST_PROFILE` | `CAN_NODE_ROLE` |
|---|---|---|---|
| 全通联调 | A（发送板） | `CAN_TEST_ALL_PASS` | `CAN_NODE_SENDER` |
| 全通联调 | B、C（接收板） | `CAN_TEST_ALL_PASS` | `CAN_NODE_RECEIVER` |
| 滤波实验 | A（发送板） | 目标测试配置 | `CAN_NODE_SENDER` |
| 滤波实验 | B（被测接收板） | 与 A 相同的测试配置 | `CAN_NODE_RECEIVER` |

工程默认值是 `CAN_TEST_ALL_PASS + CAN_NODE_RECEIVER`，直接编译得到全通接收板固件。改宏之后必须重新编译再烧录。

---

## 4. 全通模式：四种通信行为

全通配置下，发送板使用业务帧，不使用测试帧表；接收板按 ID 和帧类型分类显示。

| # | 行为 | 发起板 | 触发条件 | 发出的帧 | 对方动作与显示 |
|---|---|---|---|---|---|
| 1 | 定时发送 | A 板 | 上电自动，TIM2 每 100ms | `0x0789AB0F` 扩展数据帧，DLC=4，4 个数据字节每帧自增 | B 板 `Tim:` 行同步递增 |
| 2 | 触发发送 | A 板 | 单击 PB11（KEY_UP） | `0x555` 标准数据帧，DLC=4 | B 板 `Tri:` 行更新 |
| 3 | 请求—应答 | B 板 | 单击 PB1（KEY_DOWN） | `0x777` 标准遥控帧，DLC=4，不带数据 | A 板收到后自动回 `0x666` 标准数据帧；B 板 `Req:` 行更新 |
| 4 | 遥控帧发送 | B 板 | 单击 PB11（KEY_UP） | `0x789` 标准遥控帧，DLC=4 | 当前固件没有节点应答；板载界面不显示，需要 CAN 分析仪观察 |

显示细节：

- A 板发送后先自增数据再刷新屏幕，所以 A 板显示值比 B 板刚收到的值领先一帧；两板整体同步递增。
- `0x789` 遥控帧只有总线报文，全通接收分支只显示 `0x0789AB0F / 0x555 / 0x666` 三种数据帧，因此界面上看不到。

### OLED 布局

屏幕从上到下四行，数据十六进制显示在对应行右侧（x=30）：

```
Tx:  / Rx:      第 1 行 (y=0)
Tim:            第 2 行 (y=12)
Req:            第 3 行 (y=24)
Tri:            第 4 行 (y=36)
```

### 全通联调步骤

1. A 板烧 `SENDER + ALL_PASS`，B 板烧 `RECEIVER + ALL_PASS`，接好总线上电。
2. A 板开始定时发送，两板 `Tim:` 行递增。
3. 按 A 板 PB11，两板 `Tri:` 行更新。
4. 按 B 板 PB1，B 板发 `0x777`，A 板回 `0x666`，两板 `Req:` 行更新。
5. 按 B 板 PB11，B 板发 `0x789`，总线上有报文但没有应答。

---

## 5. 四种硬件滤波器测试配置

选中任一滤波配置后，发送板和接收板的行为一起切换：

- 发送板：按一次 PB11 发送测试帧表里的下一帧，测试数据首字节自增；OLED 显示 `Tx 序号/总数 累计次数`。
- 接收板：通用显示最近一帧通过滤波器的报文：第一行 `Std-D / Std-R / Ext-D / Ext-R 0xID`，第二行数据，第三行 `DLC` 和累计计数。
- `CAN_TEST_ALL_PASS` 仍保持第 4 节的业务行为，不使用测试帧表。

### 配置明细

| 配置 | 滤波器设置 | 测试帧表 | 按当前代码预期的通过帧 |
|---|---|---|---|
| `CAN_TEST_ALL_PASS` | 32 位掩码，ID=0，掩码=0，全部放行 | 8 帧（全通模式下不参与发送） | 全部通过 |
| `CAN_TEST_16BIT_LIST` | 16 位列表，Bank 0 的 4 个列表项：`0x345`、`0x000`、`0x234`、`0x567`（均为标准数据帧项） | 6 帧 | `0x345`、`0x567`（2/6） |
| `CAN_TEST_16BIT_MASK` | 16 位掩码，两组：`0x320~0x32F` 和 `0x200~0x2FF` 的标准数据帧 | 16 帧 | 两段范围内的 6 个数据帧（6/16） |
| `CAN_TEST_32BIT_LIST` | 32 位列表，两个列表项：标准 `0x123` 数据帧、扩展 `0x12345678` 数据帧 | 6 帧 | `0x123`、`0x12345678`（2/6） |
| `CAN_TEST_32BIT_MASK` | 32 位掩码，扩展 ID 高 21 位等于 `0x123456`，要求扩展帧、数据帧 | 8 帧 | `0x12345600/01/03/FF`（4/8） |

说明：

- `16BIT_MASK` 的掩码里包含 IDE 和 RTR 匹配位（`0x10 | 0x08`），要求标准数据帧。帧表里 `0x201`、`0x321` 是遥控帧，会被挡掉，所以是 6/16；如果希望这两个遥控帧也通过，需要清掉掩码里的 `0x10 | 0x08`。
- `16BIT_LIST` 的第 4 个列表项是 ID `0x000`，当前帧表没有 ID 0 的帧，不影响结果；帧表里的 `0x234` 是遥控帧，列表项是数据帧项，因此 `0x234` 不通过。
- `32BIT_LIST` 的两个列表项分别是标准数据帧和扩展数据帧，预期通过 2 帧。
- 以上通过数量是按当前代码和 RM0008 的寄存器位布局推导的，尚未做台面实测回填。

### 测试帧表概况

| 配置 | 条数 | 内容 |
|---|---|---|
| `ALL_PASS` | 8 | `0x12345600/01/03/FF`、`0x0789AB00/01/03/0F`，全部扩展数据帧 |
| `32BIT_MASK` | 8 | 与 `ALL_PASS` 相同 |
| `32BIT_LIST` | 6 | 标准 `0x123/0x234/0x345/0x456`、扩展 `0x12345678`、`0x0789ABCD` |
| `16BIT_MASK` | 16 | `0x100/0x101/0x1FE/0x1FF`、`0x200/0x201/0x2FE/0x2FF`、`0x300/0x301/0x3FE/0x3FF`、`0x320/0x321/0x32E/0x32F`；每段第 2 帧为遥控帧，其余为数据帧 |
| `16BIT_LIST` | 6 | 标准 `0x123`、`0x234`（遥控）、`0x345`、`0x456`、`0x567`、`0x678` |

---

## 6. 滤波实验方法

### A/B 对照

```
A 板（SENDER + 目标测试配置）
   │ 每按一次 PB11，按顺序发送测试帧表里的下一帧；数据首字节自增
   ▼
B 板（RECEIVER + 同一测试配置）
   通用显示最近一帧通过滤波器的报文，累计计数
```

操作步骤：

1. A 板烧 `SENDER + 目标测试配置`，B 板烧 `RECEIVER + 同一测试配置`。
2. 两块板接到同一条总线，上电。
3. 反复按 A 板 PB11，直到测试帧表轮完一遍。
4. 记录 B 板能显示的 ID 和累计计数，与第 5 节的预期通过集合对照。

### 关于全通对照板

当前 `CAN_TEST_ALL_PASS` 的接收分支只显示 `0x0789AB0F`、`0x555`、`0x666` 三种业务帧，不显示测试帧表里的 `0x123`、`0x234` 等帧。因此现在不能直接用一块 `ALL_PASS` 接收板做"显示全部帧"的对照。如果实验需要全通对照，有两种做法：

- 修改接收显示分支，让 `ALL_PASS` 模式也走通用显示；
- 使用 CAN 分析仪观察总线全量报文。

---

## 7. 编译与烧录

1. Keil MDK-ARM 打开 `MDK-ARM/STM32-CAN.uvprojx`。
2. 按第 3 节修改 `User/Device/project.h` 的两个宏。
3. F7 编译。当前代码全量编译为 0 Error，可能出现"文件末尾缺换行"等警告，不影响功能。
4. 连接 ST-Link / J-Link，F8 下载。
5. 换板重复第 2~4 步。

工具链与工程设置：

| 项 | 值 |
|---|---|
| IDE | Keil μVision V5.24.2 |
| 编译器 | ARMCC V5.06 update 5 |
| C 标准 | C99 |
| 优化等级 | -O1 |
| 运行库 | microlib |
| 全通接收板参考占用 | Code 10,844 字节，RO-data 6,392 字节，RW-data 148 字节，ZI-data 2,364 字节 |

其他：

- 编译产物在 `MDK-ARM/STM32-CAN/`：`STM32-CAN.hex`、`STM32-CAN.axf`。
- 新增 `.c/.h` 需要手动加入 Keil 工程分组。
- 工程由 CubeMX 生成，用户代码位于 `USER CODE BEGIN/END` 标记内；重新生成 `.ioc` 不会覆盖用户代码。

---

## 8. 已知限制与后续计划

- [ ] 接收缓存是单帧缓存，高总线负载时后到帧会覆盖未处理的帧 → 计划改为帧队列
- [ ] `HAL_CAN_AddTxMessage` 返回值未检查，3 个发送邮箱全忙时帧会静默丢弃 → 应用层补发送状态判断和重试
- [ ] 无 Bus-Off 恢复：`AutoBusOff = DISABLE`，已使能 `CAN_IT_BUSOFF` / `CAN_IT_ERROR`，但没有实现 `HAL_CAN_ErrorCallback` 和恢复流程 → 计划补 TEC/LEC 记录、延时等待和初始化模式恢复
- [ ] `tim_device.c` 的 100ms 标志尚未加 `volatile`（当前通过跨文件函数读取，-O1 下可用） → 建议补上
- [ ] 滤波配置的通过集合尚未台面实测回填，`16BIT_MASK` 的遥控帧匹配位和 `16BIT_LIST` 的 `0x234` 列表项需要实测确认
- [ ] 全通接收分支不能作为滤波实验的全量显示对照 → 需要补通用显示模式或使用 CAN 分析仪
- [ ] 帧数据完整性目前只校验 DLC，没有 CRC 或帧计数域 → 滤波实验已用"数据首字节自增"观察丢帧，可在此基础上增加帧计数
- [ ] PB12/PB13 已在按键模块注册，但没有在 GPIO 初始化中配置；当前 CAN 业务不使用这两个按键
- [ ] 滤波配置编译时会出现业务帧结构体的未引用警告（`#177-D`），属于条件编译的正常现象，不影响功能

---

## 9. 相关项目

- 多任务系统与分层架构实践：[DingDa-H/stm32f1_rtos](https://github.com/DingDa-H/stm32f1_rtos)（FreeRTOS + 四层架构，本项目接收端代码结构与其 Device/Middle/APP 分层同源）
