# STM32-CAN 使用指南（分层改造后）

> 适用工程：`D:\STM32-Study\CubeMxTest\STM32-CAN`
> 工具链：Keil MDK-ARM + ARMCC V5.06（C99 已开启，优化等级 -O1）
> 工程当前默认：`CAN_TEST_ALL_PASS` + `CAN_NODE_SENDER`，即一块"全通发送板"；接收板需改宏后重新编译
> 本文档只描述改造后的工程；旧的 `can.c/can.h` 已不存在，对应功能在 `can_device.c/h`

---

## 1. 一块板烧什么固件：两个宏决定

`User/Config/project.h` 末尾有两个宏，编译前按板的用途设置：

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

- 工程默认值是 `ALL_PASS + SENDER`。直接按 F7 编出来的是发送板固件；烧接收板前必须把 `CAN_NODE_ROLE` 改成 `CAN_NODE_RECEIVER` 重新编译。
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
| ADC 输入 | PA0 = ADC1_IN0，输入 0~3.3V，不得超过 |
| 调试 | ST-Link / J-Link 通过 SWD（PA13/PA14）烧录 |
| OLED | PB8 = SCL，PB9 = SDA（软件 I2C） |
| 按键 | PB11 = 逻辑 KEY_UP，PB1 = 逻辑 KEY_DOWN，均为低电平有效 |

上电顺序没有特殊要求，但总线上至少要有两个正常工作的节点。只有一个节点时没有人回 ACK，发送会失败，这是 CAN 协议本身的要求。

---

## 3. 全通模式：五种通信行为

全通模式下，发送板不使用测试帧表，跑的是业务代码；接收板把收到的帧分类显示。

| # | 名称 | 哪块板发起 | 操作 | 发出的帧 | 对方动作与屏幕现象 |
|---|---|---|---|---|---|
| 1 | 定时发送 | A 板 | 上电后自动，TIM2 每 100ms 一次 | 扩展数据帧 `0x0789AB0F`，DLC=4，4 个数据字节每帧先自增后发送 | 两板 `Tim:` 行显示同一数值（显示的就是实际发出的帧数据） |
| 2 | 触发发送 | A 板 | 单击 PB11（KEY_UP） | 标准数据帧 `0x555`，DLC=4 | 两板 `Tri:` 行显示同一数值 |
| 3 | 请求—应答（遥控帧） | B 板 | 单击 PB11（KEY_UP） | 标准遥控帧 `0x789`，DLC=0，不带数据 | A 板收到后回标准数据帧 `0x666`，两板 `Req:` 行更新 |
| 4 | 请求—应答（数据帧） | B 板 | 单击 PB1（KEY_DOWN） | 标准数据帧 `0x777`，DLC=0，仅作触发信号 | A 板同样回 `0x666`，两板 `Req:` 行更新 |
| 5 | ADC 周期上报 | A 板 | 上电后自动，TIM2 每 100ms 一次 | 标准数据帧 `0x400`，DLC=3：Data[0..1] 为 12 位码值（小端），Data[2] 为帧序号 | B 板 `Adc:` 行显示十进制码值，A 板发送成功后也显示同一数值 |

屏幕从上到下的五行是：`Tx:`（或 `Rx:`，y=0）、`Tim:`（y=12）、`Req:`（y=24）、`Tri:`（y=36）、`Adc:`（y=48），数据从 x=30 起显示。

显示细节：

- `Tim:` / `Tri:` / `Req:` 三条发送流各用独立的数据缓冲、各自递增，互不影响；ADC 帧用独立缓冲，每次覆盖为本次采样值；
- 顺序固定为"先自增 → 再发送 → 再刷新"，发送板显示的就是实际发出的帧内容；
- 发送失败（邮箱满或控制器未就绪）时：定时帧和 ADC 帧丢弃该周期，下一周期发送新值；触发/请求帧保持待发状态，重试的是已经准备好的同一帧数据，计数不会随重试累加；
- ADC 码值用 `%5u` 定宽显示，12 位满量程 4095 也占 5 个字符，数值位数变化时不会残留旧数字；
- 接收板超时标记写在各自行的数据区：`Tim:` 行和 `Adc:` 行分别显示 `T-OUT`，恢复收帧后由正常数据完整覆盖。

### 全通模式操作步骤

1. A 板烧 `SENDER + ALL_PASS`，B 板烧 `RECEIVER + ALL_PASS`，接好总线上电。
2. A 板上电约 100ms 后开始定时发送，`Tim:` 行的数据持续递增；B 板 `Tim:` 行显示与 A 板完全一致的数值。
3. 观察 `Adc:` 行：两板显示同一个 0~4095 的十进制值，随 PA0 输入电压变化；发送板和接收板应逐值一致。
4. 按一下 A 板的 PB11，A、B 两板 `Tri:` 行更新为同一数值。
5. 按一下 B 板的 PB11，B 板发出 `0x789` 遥控帧，A 板自动回 `0x666`，两板 `Req:` 行更新。
6. 按一下 B 板的 PB1，B 板发出 `0x777` 数据帧（纯触发，无数据），A 板同样回 `0x666`，两板 `Req:` 行更新。
7. 超时实验：拔掉 A 板 CANH 线（或断电）→ 约 300ms 后 B 板 `Tim:` 和 `Adc:` 两行数据区都显示 `T-OUT`；恢复接线后两行恢复为最新数据。若只想验证 ADC 监控项，可单独停发 `0x400`，此时只有 `Adc:` 行超时。

---

## 4. 滤波测试配置：操作方法与预期

选中任一滤波配置后，发送板和接收板的行为都会切换：

- 发送板：按键每按一次，按顺序发送测试帧表里的一帧，并让测试数据首字节自增；OLED 第一行显示 `Tx 序号/总数 累计次数`。滤波档发送板不处理接收业务，收到帧直接丢弃（清空队列），避免溢出计数持续增长。
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
| `16BIT_LIST` | 6 | `0x345`、`0x567` 两帧通过（2/6）；`0x234` 是遥控帧、列表项是数据帧，不通过 |
| `16BIT_MASK` | 16 | `0x200~0x2FF` 与 `0x320~0x32F` 两段标准数据帧，共 6 帧通过；两个遥控帧被匹配位挡掉 |
| `32BIT_LIST` | 6 | `0x123` 标准数据帧和 `0x12345678` 扩展数据帧，共 2 帧 |
| `32BIT_MASK` | 8 | `0x12345600/01/03/FF` 扩展数据帧，共 4 帧 |

> ⚠ 上表按当前代码与 RM0008 寄存器位布局推导，尚未做台面实测回填。烧滤波配置前建议先按上面步骤跑一遍，把实际通过结果记录下来，再回填 README 第 5 节表格。

---

## 5. 编译与烧录

1. Keil 打开 `MDK-ARM/STM32-CAN.uvprojx`。
2. 按第 1 节修改 `User/Config/project.h` 的两个宏。
3. F7 编译。当前工程实测 0 Error、0 Warning；发送节点 + `ALL_PASS` + ADC 上报的程序占用为 Code 15,076 B、RO-data 6,440 B、RW-data 224 B、ZI-data 2,688 B。
4. 连接 ST-Link，F8 下载。
5. 换板重复第 2~4 步；接收板记得先改 `CAN_NODE_ROLE`。

编译产物在 `MDK-ARM/STM32-CAN/`：`STM32-CAN.hex`、`STM32-CAN.axf`。

---

## 6. 常见现象排查

| 现象 | 优先检查 |
|---|---|
| A 板上电后 `Tim:` 不递增 | A 板是否真的烧了 `CAN_NODE_SENDER`；确认没有烧错固件 |
| 两板都没有显示 | 120Ω 终端电阻、CANH/CANL 是否接反、是否共地、TJA1050 是否 5V 供电 |
| 只有一块板时发送失败 | 总线上必须至少两个正常节点才能回 ACK |
| `Adc:` 行一直不动或显示 0 | PA0 是否接了变化的电压；PA0 是否被其他外设占用；确认 `vAdcDeviceInit()` 已执行 |
| `Adc:` 行显示 `T-OUT` | 发送板的 `0x400` 帧没有到达：检查发送板是否在运行、是否烧了滤波档（滤波档不发 ADC）、总线是否断开 |
| `Tim:` 行显示 `T-OUT` | 发送板停发或总线断开；恢复后自动清除 |
| 接收板偶发丢帧 | 驱动层是 8 帧环形队列，短时突发可吸收；持续高负载时读 `ulDeviceCan_GetRxOverflowCount()` 判断是否溢出 |
| 拔插总线后通信中断 | 当前已实现 Bus-Off 恢复：ESR.BOFF → 连续计数 → L1 100ms / L2 2000ms → Stop/Start → 约 300ms 观察窗。若仍不恢复，检查 TEC 是否持续升高（总线短路或波特率不一致时恢复会反复失败） |
| 滤波接收板收到的帧比预期多 | 先确认是否烧错测试配置；再按第 4 节说明复测滤波器配置 |
| 按键无反应 | PB11/PB1 是否接对、是否低电平有效；PB12/PB13 未参与 CAN 业务 |

---

## 7. 分层后各文件的职责

只列 CAN 相关的文件；OLED、按键驱动等未改动文件不在本文档展开。

| 文件 | 层次 | 职责 |
|---|---|---|
| `User/Config/project.h` | 配置 | 全工程编译期配置入口：测试配置编号、节点类型、OLED/按键公共宏。不含代码 |
| `User/Config/can_cfg.c/h` | 配置 | CAN 运行期只读参数：Bus-Off 恢复策略（L1/L2 时间、连续次数阈值、观察窗）、超时监控表（ID/阈值/条目数）、接收队列深度 `RX_QUEUE_SIZE` |
| `User/Device/can_device.c/h` | 设备驱动层（MCAL 风格） | CAN 收发封装。`ucDeviceCan_Transmit` 把发送参数结构体转换成 HAL 帧头并调用 `HAL_CAN_AddTxMessage`，返回 0=成功、1=失败（邮箱满或控制器未就绪），失败计数可查询；FIFO0 接收中断把整帧写入 8 帧环形队列并推进写指针，应用层用 `ucDeviceCan_ReadRxFrame` 取帧；提供接收溢出计数、发送失败计数、Bus-Off 标志与 ESR 快照、进入/退出初始化模式。不做 ID 判断、不刷屏 |
| `User/Device/adc_device.c/h` | 设备驱动层 | ADC1 单次转换封装：上电校准 `HAL_ADCEx_Calibration_Start`；`usAdcDeviceGetValue` 启动转换、10ms 轮询超时，成功返回新值，失败返回上次有效值 |
| `User/Device/tim_device.c/h` | 设备驱动层 | TIM2 100ms 更新中断，置周期标志；提供 `ucTimDeviceGetFlag / vTimDeviceClearFlag`。应用层不需要知道 TIM2 |
| `User/Middle/button_mid.c/h` | 中间件 | 按键事件状态机（消抖、单击/双击/长按），供 `task.c` 和 `can_app.c` 调用 |
| `User/Middle/can_sm_mid.c/h` | 中间件（CanSM 思路） | Bus-Off 恢复状态机：任务上下文确认 ESR、连续计数、L1/L2 分级等待、调用驱动层进入/退出初始化模式、观察窗判定 |
| `User/Middle/can_timeout_mid.c/h` | 中间件 | 报文超时监控：配置表在 `can_cfg`，运行状态在本模块，按下标一一对应；`vCanTimeoutUpdate` 喂狗、`vCanTimeoutTask` 周期判断、`ucCanTimeoutGetFlag` 查询 |
| `User/APP/can_app.c/h` | 应用层 | `vCanAppInit` 根据 `CAN_TEST_PROFILE` 计算滤波器的 ID、掩码、模式和位宽；`vCanAppSenderTask` 实现发送板业务（定时帧、ADC 上报、触发发送、请求应答）；`vCanAppReceiverTask` 实现接收板业务（按键发请求帧、队列取帧、按 ID 显示、超时标记）。滤波配置的业务规则集中在这里 |
| `User/APP/can_test.c/h` | 测试数据 | 按 `CAN_TEST_PROFILE` 定义发送帧表和测试数据缓存；`vCanTestSendOneByIndex` 按索引发送一帧并让数据首字节自增；`ucCanTestGetFrameNum` 返回帧表长度。只被滤波实验的发送板使用 |
| `User/APP/task.c/h` | 任务入口 | `vOledInit`、`Button_Init` 完成设备初始化；`vtask` 是主循环入口：按键状态机、Bus-Off 恢复、超时判断，再按 `CAN_NODE_ROLE` 分发到发送任务或接收任务 |
| `Core/Src/main.c` | 启动与调度 | 初始化顺序：HAL → 时钟 → GPIO → CAN（500kbps、正常模式、关闭自动 Bus-Off、关闭自动重发）→ TIM2 → ADC1 → 启动定时中断 → OLED → 按键 → `vCanAppInit()` → `vAdcDeviceInit()` → 主循环 `vtask()` |
| `Core/Src/stm32f1xx_hal_msp.c` | 外设底层 | PA11/PA12 CAN 复用配置、PA0 模拟输入配置、TIM2/ADC 时钟使能等 CubeMX 生成的 MSP 代码 |

### 调用关系

```
main.c
 ├─ MX_CAN_Init / MX_TIM2_Init / MX_ADC1_Init    CubeMX 生成的外设参数
 ├─ vOledInit / Button_Init                     OLED、按键设备初始化
 ├─ vCanAppInit                                 can_app.c
 │    ├─ 计算滤波参数
 │    ├─ vCanTimeoutInit                        can_timeout_mid.c
 │    └─ vCanDeviceInit → HAL_CAN_ConfigFilter / Start / ActivateNotification
 └─ vAdcDeviceInit                              adc_device.c（ADC 校准）

while (1) vtask()                               task.c
 ├─ vBtnExecute()                               button_mid.c
 ├─ vCanSmMainFunction()                        can_sm_mid.c → ucCanDevice_Enter/ExitInitMode
 ├─ vCanTimeoutTask()                           can_timeout_mid.c
 └─ 按 CAN_NODE_ROLE 分发
      ├─ vCanAppSenderTask()                    can_app.c
      │    ├─ ucTimDeviceGetFlag / vTimDeviceClearFlag   tim_device.c
      │    ├─ usAdcDeviceGetValue                        adc_device.c
      │    ├─ ucDeviceCan_Transmit                       can_device.c
      │    ├─ ucDeviceCan_ReadRxFrame（请求帧轮询）        can_device.c
      │    └─ 滤波配置下：vCanTestSendOneByIndex          can_test.c
      └─ vCanAppReceiverTask()                  can_app.c
           ├─ ucDeviceCan_Transmit（发请求帧）             can_device.c
           ├─ ucDeviceCan_ReadRxFrame                     can_device.c
           ├─ vCanTimeoutUpdate / ucCanTimeoutGetFlag     can_timeout_mid.c
           └─ OLED 显示接口

中断：
 USB_LP_CAN1_RX0_IRQn → HAL_CAN_IRQHandler → HAL_CAN_RxFifo0MsgPendingCallback
                        （can_device.c：写环形队列、推进写指针、满则丢帧并计数）
 HAL_CAN_ErrorCallback（can_device.c：核验 ESR.BOFF，置 Bus-Off 标志）
 TIM2_IRQn            → HAL_TIM_IRQHandler → HAL_TIM_PeriodElapsedCallback
                        （tim_device.c：置 100ms 标志）
```

### 与 AUTOSAR 分层的对应关系

当前是精简版分层，不是完整的 AUTOSAR 协议栈：

- `can_device.c`、`tim_device.c`、`adc_device.c` 对应 MCAL 的 CAN 驱动、定时器驱动、ADC 驱动。
- `can_timeout_mid.c` 承担了 Com 模块的超时监控职责；`can_sm_mid.c` 对应 CanSM。
- `can_app.c` 同时承担了应用逻辑和一部分通信处理，相当于把 CanIf/PduR/Com 的职责合并了。
- `task.c` 对应任务映射（RTE/OS 任务）这一层，目前是裸机主循环。
- `project.h`、`can_cfg.c/h` 对应编译期与链接期配置，类似 BSW 的配置文件。
- 工程里没有 CanIf、PduR、Com、Nm 等独立模块；如果后续要继续贴近 AUTOSAR，可以在 `can_device` 和 `can_app` 之间拆出 CanIf（报文收发接口、ID 路由）和 PduR/Com（信号打包、超时监控）。

---

## 8. 使用前需要知道的限制

- 接收端是 8 帧环形队列，持续高负载时仍会溢出；溢出次数可查询 `ulDeviceCan_GetRxOverflowCount()`，但尚未显示到 OLED。
- 发送失败（邮箱满或控制器未就绪）会计入 `ulDeviceCan_GetTxFailCount()`，但尚未显示到 OLED。
- Bus-Off 恢复不重发故障期间丢弃的报文；故障期间的事件帧由应用层保持待发状态重试，定时帧和 ADC 帧丢弃该周期。
- ADC 帧的序号字节目前只发送、不统计，接收端还没有用序号计算丢帧数。
- ADC 为单次轮询转换，没有 DMA 或连续转换；转换超时（10ms）时返回上次有效值，不会阻塞主循环。
- 滤波配置的通过集合尚未台面实测回填，`16BIT_MASK` 的遥控帧匹配位和 `16BIT_LIST` 的 `0x234` 列表项需实测确认。
- 全通接收分支只显示四类业务帧，不能作为滤波实验的全量显示对照 → 补通用显示模式或使用 CAN 分析仪。
- PB12/PB13 已在按键模块注册，但 GPIO 初始化未配置；当前 CAN 业务不使用这两个按键。
