# STM32-CAN 使用指南（分层改造后）

> 适用工程：`D:\STM32-Study\CubeMxTest\STM32-CAN`
> 工具链：Keil MDK-ARM + ARMCC V5.06（C99 已开启，优化等级 -O1）
> 默认编译结果：`CAN_TEST_ALL_PASS` + `CAN_NODE_RECEIVER`，即一块"全通接收板"
> 本文档只描述改造后的工程；旧的 `can.c/can.h` 已不存在，对应功能在 `can_device.c/h`

---

## 1. 一块板烧什么固件：两个宏决定

`User/Device/project.h` 末尾有两个宏，编译前按板的用途设置：

| 宏 | 可选值 | 作用 |
|---|---|---|
| `CAN_TEST_PROFILE` | `CAN_TEST_ALL_PASS`、`CAN_TEST_16BIT_LIST`、`CAN_TEST_16BIT_MASK`、`CAN_TEST_32BIT_LIST`、`CAN_TEST_32BIT_MASK` | 选择一套"滤波器配置 + 发送帧表 + 发送板行为" |
| `CAN_NODE_ROLE` | `CAN_NODE_SENDER`、`CAN_NODE_RECEIVER` | 这块板是发送板还是接收板 |

常用固件组合：

| 场景 | 板 | `CAN_TEST_PROFILE` | `CAN_NODE_ROLE` |
|---|---|---|---|
| 全通联调 | A（发送板） | `CAN_TEST_ALL_PASS` | `CAN_NODE_SENDER` |
| 全通联调 | B、C（接收板） | `CAN_TEST_ALL_PASS` | `CAN_NODE_RECEIVER` |
| 滤波实验 | A（发送板） | 目标测试配置 | `CAN_NODE_SENDER` |
| 滤波实验 | B（目标接收板） | 与 A 相同的测试配置 | `CAN_NODE_RECEIVER` |
| 滤波实验 | C（对照接收板） | `CAN_TEST_ALL_PASS` | `CAN_NODE_RECEIVER` |

注意：

- 工程默认值是 `ALL_PASS + RECEIVER`。直接按 F7 编出来的是接收板固件，烧到 A 板不会有定时发送。
- 改宏之后必须重新编译再烧录。Keil 会跟踪头文件依赖，正常 F7 即可。
- `CAN_TEST_PROFILE` 不只是改滤波器：发送板的行为也会一起切换（见第 3、4 节）。

---

## 2. 硬件连接（上电前检查）

| 项 | 连接 |
|---|---|
| CAN 引脚 | PA11 = CAN_RX，PA12 = CAN_TX，接 TJA1050 的 RXD / TXD |
| 收发器供电 | TJA1050 用 5V 供电，与 MCU 共地 |
| 总线 | 所有节点 CANH 并 CANH、CANL 并 CANL，接成一条直线型主干 |
| 终端电阻 | 120Ω 只接在总线两个物理末端，中间节点不接 |
| 分支线 | 每个节点到主干的支线尽量短，避免信号反射 |
| 调试 | ST-Link / J-Link 通过 SWD（PA13/PA14）烧录 |
| OLED | PB8 = SCL，PB9 = SDA（软件 I2C） |
| 按键 | PB11 = 逻辑 KEY_UP，PB1 = 逻辑 KEY_DOWN，均为低电平有效 |

上电顺序没有特殊要求，但总线上至少要有两个正常工作的节点。只有一个节点时没有人回 ACK，发送会失败，这是 CAN 协议本身的要求。

---

## 3. 全通模式：四种通信行为

全通模式下，发送板不使用测试帧表，跑的是业务代码；接收板把收到的帧分类显示。

| # | 名称 | 哪块板发起 | 操作 | 发出的帧 | 对方动作 | 屏幕现象 |
|---|---|---|---|---|---|---|
| 1 | 定时发送 | A 板 | 上电后自动，TIM2 每 100ms 一次 | 扩展数据帧 `0x0789AB0F`，DLC=4，4 个数据字节每帧自增 | B 板接收 | A、B 两板的 `Tim:` 行都在递增；A 板显示自增后的下一值，B 板显示刚收到的值，两板相差一帧 |
| 2 | 触发发送 | A 板 | 单击 PB11（KEY_UP） | 标准数据帧 `0x555`，DLC=4 | B 板接收 | A、B 两板的 `Tri:` 行更新，同样相差一帧 |
| 3 | 请求—应答 | B 板 | 单击 PB1（KEY_DOWN） | 标准遥控帧 `0x777`，DLC=4，不带数据 | A 板收到后自动回标准数据帧 `0x666` | B 板 `Req:` 行更新；A 板 `Req:` 行显示自增后的应答数据值 |
| 4 | 遥控帧发送（无应答） | B 板 | 单击 PB11（KEY_UP） | 标准遥控帧 `0x789`，DLC=4 | 当前固件没有节点应答 | 板载界面不显示（显示逻辑只处理 `0x0789AB0F / 0x555 / 0x666` 三种数据帧），需要 CAN 分析仪观察 |

屏幕从上到下的四行是：`Tx:`（或 `Rx:`）、`Tim:`、`Req:`、`Tri:`。数据十六进制显示在对应行右侧。

### 全通模式操作步骤

1. A 板烧 `SENDER + ALL_PASS`，B 板烧 `RECEIVER + ALL_PASS`，接好总线上电。
2. A 板上电约 100ms 后开始定时发送，`Tim:` 行的数据持续递增；B 板 `Tim:` 行应同步递增（A 板显示自增后的值，比 B 板领先一帧，这是先自增后刷新的显示顺序导致的，属于原有行为）。
3. 按一下 A 板的 PB11，A、B 两板 `Tri:` 行更新。
4. 按一下 B 板的 PB1，B 板发出 `0x777` 遥控帧，A 板自动回 `0x666`，两板 `Req:` 行更新。
5. 按一下 B 板的 PB11，B 板发出 `0x789` 遥控帧，总线上有这一帧，但没有节点应答，界面不会显示。

---

## 4. 滤波测试配置：操作方法与预期

选中任一滤波配置后，发送板和接收板的行为都会切换：

- 发送板：按键每按一次，按顺序发送测试帧表里的一帧，并让测试数据首字节自增；OLED 第一行显示 `Tx 序号/总数 累计次数`。
- 接收板：不再按 ID 分类显示，而是把"凡是通过本节点滤波器的帧"统一显示：第一行 `Std-D / Std-R / Ext-D / Ext-R 0xID`，第二行数据，第三行 `DLC` 和累计计数。

### 操作步骤

1. A 板烧 `SENDER + 目标测试配置`。
2. B 板烧 `RECEIVER + 同一测试配置`，作为被测节点。
3. C 板烧 `RECEIVER + ALL_PASS`，作为对照节点。
4. 三块板上电入网，反复按 A 板 PB11，直到测试帧表全部轮一遍。
5. 对比 B 板和 C 板的显示：C 板显示总线全部帧，B 板只显示通过滤波器的子集。

预期通过数量（与 `can_test.c` 的帧表对应）：

| 测试配置 | 帧表条数 | 预期现象 |
|---|---|---|
| `ALL_PASS` | 8（该表在全通模式下不参与发送） | 全部通过 |
| `16BIT_LIST` | 6 | 按 README 记录为 `0x345`、`0x567`；两处位布局待复测 ⚠ |
| `16BIT_MASK` | 16 | 预期 `0x200~0x2FF` 与 `0x320~0x32F` 两段标准数据帧，共 8 帧 |
| `32BIT_LIST` | 6 | 预期 `0x123` 标准数据帧和 `0x12345678` 扩展数据帧；README 记录为理论 1/6 ⚠ |
| `32BIT_MASK` | 8 | `0x123456xx` 扩展数据帧，共 4 帧 |

> ⚠ 滤波配置的通过集合目前没有台面复测数据。当前 `can_app.c` 每个配置只配置了 FilterBank 0，
> 16 位模式下部分列表项/第二组滤波器没有写入历史版本的全部内容，实际现象可能与上表不一致。
> 烧滤波配置前建议先按上面步骤跑一遍，把实际通过结果记录下来，再回填 README 第 5 节表格。

---

## 5. 编译与烧录

1. Keil 打开 `MDK-ARM/STM32-CAN.uvprojx`。
2. 按第 1 节修改 `User/Device/project.h` 的两个宏。
3. F7 编译。正常结果是 0 Error；可能出现"文件末尾缺换行"警告，不影响功能。
4. 连接 ST-Link，F8 下载。
5. 换板重复第 2~4 步。

编译产物在 `MDK-ARM/STM32-CAN/`：`STM32-CAN.hex`、`STM32-CAN.axf`。

---

## 6. 常见现象排查

| 现象 | 优先检查 |
|---|---|
| A 板上电后 `Tim:` 不递增 | A 板是否真的烧了 `CAN_NODE_SENDER`；工程默认是接收板 |
| 两板都没有显示 | 120Ω 终端电阻、CANH/CANL 是否接反、是否共地、TJA1050 是否 5V 供电 |
| 只有一块板时发送失败 | 总线上必须至少两个正常节点才能回 ACK |
| 接收板偶尔丢帧 | 驱动层是单帧缓存，后到帧会覆盖未处理的帧；100ms 周期下正常，提高发送频率会明显 |
| 拔插总线后通信中断 | `AutoBusOff = DISABLE` 且未实现 Bus-Off 恢复，节点进入 Bus-Off 后需要复位 |
| 滤波接收板收到的帧比预期多 | 先确认是否烧错测试配置；再按第 4 节说明复测滤波器配置 |
| 按键无反应 | PB11/PB1 是否接对、是否低电平有效；PB12/PB13 未参与 CAN 业务 |

---

## 7. 分层后各文件的职责

只列 CAN 相关的文件；OLED、按键驱动等未改动文件不在本文档展开。

| 文件 | 层次 | 职责 |
|---|---|---|
| `User/Device/project.h` | 配置 | 全工程编译期配置入口：测试配置编号、节点类型、OLED/按键的公共宏。不含代码 |
| `User/Device/can_device.c/h` | 设备驱动层（MCAL 风格） | CAN 收发封装。`vMyCan_Transmit` 把发送参数结构体转换成 HAL 帧头并调用发送；`vCanDeviceInit` 接收上层算好的滤波器参数，配置滤波器、启动 CAN、使能中断；FIFO0 接收中断把帧头和数据存入文件内静态缓存并置标志；提供 `ucBspCan_GetRxFlag / vBspCan_ClearRxFlag / vBspCan_GetRxHeader / vBspCan_GetRxData` 四个取数接口。不做 ID 判断、不刷屏 |
| `User/Device/tim_device.c/h` | 设备驱动层（MCAL 风格） | TIM2 100ms 更新中断，置周期标志；提供 `ucTimDeviceGetFlag / vTimDeviceClearFlag`。应用层不需要知道 TIM2 |
| `User/APP/can_app.c/h` | 应用层 | `vCanAppInit` 根据 `CAN_TEST_PROFILE` 计算滤波器的 ID、掩码、模式和位宽，调用驱动层初始化；`vCanAppSenderTask` 实现发送板业务（定时发送、触发发送、请求应答）；`vCanAppReceiverTask` 实现接收板业务（按键发遥控帧、接收解析显示）。滤波配置的业务规则集中在这里 |
| `User/APP/can_test.c/h` | 测试数据 | 按 `CAN_TEST_PROFILE` 定义 5 张发送帧表和测试数据缓存；`vCanTestSendOneByIndex` 按索引发送一帧并让数据首字节自增；`ucCanTestGetFrameNum` 返回帧表长度。只被滤波实验的发送板使用 |
| `User/APP/task.c/h` | 任务入口 | `vOledInit`、`Button_Init` 完成设备初始化；`vtask` 是主循环入口：先跑按键状态机 `vBtnExecute()`，再按 `CAN_NODE_ROLE` 分发到发送任务或接收任务 |
| `Core/Src/main.c` | 启动与调度 | 初始化顺序：HAL → 时钟 → GPIO → CAN 外设参数（500kbps、正常模式、关闭自动 Bus-Off、关闭自动重发）→ TIM2 → 启动定时中断 → OLED → 按键 → `vCanAppInit()` → 主循环 `vtask()` |
| `User/Middle/button_mid.c/h` | 中间件（未改动） | 按键事件状态机，输出单击等事件，供 `task.c` 和 `can_app.c` 调用 |

### 调用关系

```
main.c
 ├─ MX_CAN_Init / MX_TIM2_Init        CubeMX 生成的外设参数
 ├─ vOledInit / Button_Init           OLED、按键设备初始化
 └─ vCanAppInit                      can_app.c
      └─ 计算滤波参数 → vCanDeviceInit → HAL_CAN_ConfigFilter / Start / ActivateNotification

while (1) vtask()                    task.c
 ├─ vBtnExecute()                    button_mid.c
 └─ 按 CAN_NODE_ROLE 分发
      ├─ vCanAppSenderTask()         can_app.c
      │    ├─ ucTimDeviceGetFlag / vTimDeviceClearFlag   tim_device.c
      │    ├─ vMyCan_Transmit                             can_device.c
      │    ├─ 接收标志与取数接口                            can_device.c
      │    └─ 滤波配置下：vCanTestSendOneByIndex           can_test.c
      └─ vCanAppReceiverTask()       can_app.c
           ├─ vMyCan_Transmit（发遥控请求帧）
           ├─ 接收标志与取数接口                            can_device.c
           └─ OLED 显示接口

中断：
 USB_LP_CAN1_RX0_IRQn → HAL_CAN_IRQHandler → HAL_CAN_RxFifo0MsgPendingCallback（can_device.c，存缓存并置标志）
 TIM2_IRQn            → HAL_TIM_IRQHandler → HAL_TIM_PeriodElapsedCallback（tim_device.c，置 100ms 标志）
```

### 与 AUTOSAR 分层的对应关系

当前是精简版分层，不是完整的 AUTOSAR 协议栈：

- `can_device.c`、`tim_device.c` 对应 MCAL 的 CAN 驱动和定时器驱动。
- `can_app.c` 同时承担了应用逻辑和一部分通信处理，相当于把 CanIf/PduR/Com 的职责合并了。
- `task.c` 对应任务映射（RTE/OS 任务）这一层，目前是裸机主循环。
- `project.h` 对应编译期配置头，类似 BSW 的配置文件。
- 工程里没有 CanIf、PduR、Com、Nm 等独立模块；如果后续要继续贴近 AUTOSAR，可以在 `can_device` 和 `can_app` 之间拆出 CanIf（报文收发接口、ID 路由）和 PduR/Com（信号打包、超时监控）。

---

## 8. 使用前需要知道的限制

- 接收缓存是单帧缓存，高负载时后到帧会覆盖未处理的帧。
- `HAL_CAN_AddTxMessage` 的返回值没有检查，3 个发送邮箱全忙时这一帧会静默丢弃。
- `AutoBusOff = DISABLE`，且没有实现 `HAL_CAN_ErrorCallback` 和恢复流程；进入 Bus-Off 后只能复位恢复。
- `tim_device.c` 的 100ms 标志还没有加 `volatile`（跨文件通过函数读取，当前 -O1 下可用，建议补上）。
