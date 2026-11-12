# STM32-CAN 总线驱动验证与多节点通信

基于 STM32F103C8T6（bxCAN）+ TJA1050 的 CAN 2.0B 多节点通信验证项目：定时/触发/请求—应答（遥控帧与数据帧两种请求）通信策略、四种硬件滤波器测试配置、ABOM=0 软件 Bus-Off 检测与恢复、报文超时监控，配套 128×64 OLED 显示与按键交互。代码按 配置层（Config）/ 设备驱动层（Device）/ 中间件（Middle）/ 应用层（APP）组织，接口边界参照 AUTOSAR 思路。

[![Platform](https://img.shields.io/badge/MCU-STM32F103C8T6-blue)]()
[![Bus](https://img.shields.io/badge/CAN-500kbps-orange)]()
[![IDE](https://img.shields.io/badge/Toolchain-Keil%20MDK%205-green)]()

> 仓库地址：https://github.com/DingDa-H/stm32-CAN
> 详细操作手册（接线、逐键实验步骤、排障）见 `USAGE.md`。

---

## 1. 硬件平台

| 项 | 配置 |
|---|---|
| MCU | STM32F103C8T6（Cortex-M3，72MHz） |
| 时钟 | HSE 8MHz → PLL×9 → SYSCLK 72MHz，APB1 = 36MHz |
| CAN 收发器 | TJA1050（5V 供电，3.3V IO） |
| CAN 引脚 | PA11（CAN_RX）/ PA12（CAN_TX） |
| OLED | 128×64，软件模拟 I2C：PB8=SCL、PB9=SDA（开漏输出） |
| 按键 | PB1 / PB11（低有效）、PB12 / PB13（高有效） |
| 调试 | SWD（PA13/PA14） |

### CAN 位时序（500kbps）

```
量化时间 tq = Prescaler / F_APB1 = 6 / 36MHz
位时间     = (1 SYNC + 8 BS1 + 3 BS2) tq = 12 tq
波特率     = F_APB1 / (12 × 6) = 500kbps
采样点     = (1 + 8) / 12 ≈ 75%
```

`Core/Src/main.c` 的 `MX_CAN_Init` 关键项：

- 工作模式 `CAN_MODE_NORMAL`
- `AutoBusOff = DISABLE`（ABOM=0）：控制器不自动退出 Bus-Off，恢复由软件状态机执行（见 §2）
- `AutoRetransmission = DISABLE`：单次发送语义，仲裁失败或出错后不自动重发
- `Prescaler = 6`，`SJW = 2TQ`，`BS1 = 8TQ`，`BS2 = 3TQ`

---

## 2. 软件架构

```
STM32-CAN/
├── Core/                            CubeMX 生成的启动、时钟、外设初始化
├── Drivers/                         HAL 库与 CMSIS
├── MDK-ARM/                         Keil 工程与编译产物
└── User/
    ├── Config/                      编译期配置层（只读参数）
    │   ├── project.h                测试配置、节点类型、OLED/按键公共宏
    │   └── can_cfg.c/h              Bus-Off 恢复参数、超时监控表（extern const，驻 Flash）
    ├── Device/                      设备驱动层
    │   ├── can_device.c/h           CAN 收发、滤波器下发、接收/Bus-Off 中断标志、
    │   │                            ESR 快照、进入/退出初始化模式
    │   ├── tim_device.c/h           TIM2 100ms 周期标志
    │   ├── oled_device.c/h          OLED 驱动（软件 I2C + 显存缓冲）
    │   ├── oled_font.c/h            字库
    │   └── botton.c/h               按键 GPIO 电平读取
    ├── Middle/                      中间件层
    │   ├── button_mid.c/h           按键事件状态机（消抖、单击/双击/长按）
    │   ├── can_sm_mid.c/h           Bus-Off 恢复状态机（CanSM 思路）
    │   └── can_timeout_mid.c/h      报文超时监控（运行状态与超时判断）
    └── APP/                         应用层
        ├── can_app.c/h              CAN 应用：滤波参数计算、发送端/接收端业务
        ├── can_test.c/h             测试帧表、按索引发送、帧表长度查询
        └── task.c/h                 任务入口：初始化、按键扫描、鲁棒性任务周期执行、按节点类型分发
```

`*.bak` 是旧版本备份，不在 Keil 工程中，不参与编译。

### 初始化与运行流程

```text
main()
 ├─ MX_GPIO_Init / MX_CAN_Init / MX_TIM2_Init
 ├─ HAL_TIM_Base_Start_IT(&htim2)
 ├─ vOledInit / Button_Init
 └─ vCanAppInit()
      ├─ 按 CAN_TEST_PROFILE 填写 CAN_FilterTypeDef
      ├─ vCanTimeoutInit()
      └─ vCanDeviceInit()
           └─ HAL_CAN_ConfigFilter / HAL_CAN_Start
              / HAL_CAN_ActivateNotification(RX_FIFO0 | BUSOFF | ERROR)

while (1) vtask()
 ├─ vBtnExecute()                    按键事件状态机
 ├─ vCanSmMainFunction()             Bus-Off 监控与恢复
 ├─ vCanTimeoutTask()                报文超时判断
 └─ 按 CAN_NODE_ROLE 分支
      ├─ CAN_NODE_SENDER   → vCanAppSenderTask()
      └─ CAN_NODE_RECEIVER → vCanAppReceiverTask()
```

中断与任务的分工是固定的：接收中断只把帧头与数据存入驱动层静态缓存并置标志；Bus-Off 中断（`HAL_CAN_ErrorCallback`）只核验 `ESR.BOFF` 并置标志。所有确认、计时、恢复、刷屏都发生在任务上下文，中断里不延时、不阻塞。

### 参数下发路径

```
project.h 的 CAN_TEST_PROFILE
  → can_app.c 的 vCanAppInit()：填写 CAN_FilterTypeDef（Bank 0、FIFO0、ID/掩码/模式/位宽）
  → can_device.c 的 vCanDeviceInit()：交给 HAL 下发并启动

can_cfg.c 的只读参数
  → can_sm_mid.c：L1/L2 恢复策略参数
  → can_timeout_mid.c：监控 ID 与超时阈值
```

四种滤波配置的参数全部集中在 `vCanAppInit()`；运行策略参数全部集中在 `can_cfg`，中间件与应用层不再硬编码数值。

### Bus-Off 检测与恢复（ABOM=0 软件策略）

1. **中断置标志**：`can_device.c` 的 `HAL_CAN_ErrorCallback` 判断 `CAN1` 且 `ESR.BOFF` 置位后写 volatile 标志；
2. **任务确认**：`vCanSmMainFunction()` 读取 ESR 快照（记录 TEC/LEC 供诊断），二次核验后连续计数 +1；
3. **分级等待**：连续次数 < `CanSMBorCounterL1ToL2` 走 L1 快恢复等待，否则 L2 慢等待；等待是任务内时间戳判断，不阻塞；
4. **恢复序列**：等待到期后调用驱动层 `ucCanDevice_EnterInitMode()` / `ucCanDevice_ExitInitMode()`（封装 `HAL_CAN_Stop` / `HAL_CAN_Start`，即请求进/出初始化模式；内含 HAL 状态锁 ERROR 时重跑 `HAL_CAN_Init` 解锁），退出初始化后硬件仍需检测到 128 个连续隐性位才真正完成 Bus-Off 恢复；
5. **判定**：观察窗口 `CanSMBorTimeTxEnsured` 内无新 Bus-Off → 连续计数清零、恢复成功；窗口内再次 Bus-Off → 恢复失败，计数保留并重新评估 L1/L2。

恢复期间不重发被丢弃的报文，也不做发送重试（见 §8）。

### 报文超时监控

`vCanTimeoutUpdate(ID)` 在收到目标报文时刷新最后接收时刻并清除超时标志（喂狗）；`vCanTimeoutTask()` 每圈判断"当前 tick − 最后接收 tick > 阈值"置超时标志；应用层 `ucCanTimeoutGetFlag(ID)` 查询并在 OLED 第 5 行显示 `TIMEOUT!`。判断使用无符号减法，对 tick 溢出安全；`ucIsStarted` 防止上电未收到首帧时误报。配置（ID/阈值）在 `can_cfg`，状态在 `can_timeout_mid`，两者按下标一一对应。

### 报文超时监控与扩监控项

扩一个监控项（例：给 ADC 周期上报帧 `0x400`，100ms 周期，加超时监控）：

1. `can_cfg.h`：新增 `#define CAN_ID_ADC_MSG 0x400U`、`#define CAN_TIMEOUT_ADC_MS 300U`，并把 `CAN_TIMEOUT_MONITOR_NUM` 从 `1U` 改为 `2U`；
2. `can_cfg.c`：在 `CanTimeoutConfigList[]` 末尾追加一行 `{ CAN_ID_ADC_MSG, CAN_TIMEOUT_ADC_MS },`；
3. `can_app.c` 接收任务：在 `if(stAppHeader.RTR == CAN_RTR_DATA)` 内补一条 ID 匹配并喂狗的分支；在现有 `TIMEOUT!` 检查块旁边补一段 `ucCanTimeoutGetFlag(CAN_ID_ADC_MSG)` 的显示（注意 else 分支用空格覆盖，否则文字残留）。

`can_timeout_mid.c` 不需要任何改动。表加了行但忘记同步 `CAN_TIMEOUT_MONITOR_NUM`，会在编译期报 "too many initializers"；只改 `NUM` 不补表则生成全零空条目，`ucIsStarted` 永不置 1，不会误报。

---

## 3. 编译期配置

`User/Config/project.h` 末尾有两个宏：

| 宏 | 取值 | 作用 |
|---|---|---|
| `CAN_TEST_PROFILE` | `ALL_PASS` / `16BIT_LIST` / `16BIT_MASK` / `32BIT_LIST` / `32BIT_MASK` | 选择滤波器配置和发送帧表 |
| `CAN_NODE_ROLE` | `SENDER` / `RECEIVER` | 选择节点类型 |

运行期策略参数在 `User/Config/can_cfg.c`（`extern const`，改后重编译即可，不涉及代码）：

| 参数 | 当前值 | 用途 |
|---|---|---|
| `CanSMBorCounterL1ToL2` | 2 | 连续 Bus-Off 次数达到该值转入 L2 慢恢复 |
| `CanSMBorTimeL1` | 100 ms | L1 快恢复等待时间 |
| `CanSMBorTimeL2` | 2000 ms | L2 慢恢复等待时间 |
| `CanSMBorTimeTxEnsured` | 300 ms | 恢复后观察窗口 |
| `CAN_ID_TIMING_MSG` / `CAN_TIMEOUT_TIMING_MS` | `0x0789AB0F` / 300 ms | 超时监控表条目（周期报文） |

常用固件组合：

| 场景 | 板 | `CAN_TEST_PROFILE` | `CAN_NODE_ROLE` |
|---|---|---|---|
| 全通联调 | A（发送板） | `ALL_PASS` | `SENDER` |
| 全通联调 | B、C（接收板） | `ALL_PASS` | `RECEIVER` |
| 滤波实验 | A（发送板） | 目标测试配置 | `SENDER` |
| 滤波实验 | B（被测接收板） | 与 A 相同的测试配置 | `RECEIVER` |

工程默认值是 `ALL_PASS + RECEIVER`，直接编译得到全通接收板固件。改宏之后必须重新编译再烧录。

---

## 4. 全通模式：四种通信行为

全通配置下，发送板使用业务帧，不使用测试帧表；接收板按 ID 和帧类型分类显示。

| # | 行为 | 发起板 | 触发条件 | 发出的帧 | 对方动作与显示 |
|---|---|---|---|---|---|
| 1 | 定时发送 | A 板 | 上电自动，TIM2 每 100ms | `0x0789AB0F` 扩展数据帧，DLC=4 | 两板 `Tim:` 显示一致、同步递增 |
| 2 | 触发发送 | A 板 | 单击 PB11（KEY_UP） | `0x555` 标准数据帧，DLC=4 | 两板 `Tri:` 显示一致 |
| 3 | 请求—应答（数据帧请求） | B 板 | 单击 PB1（KEY_DOWN） | `0x777` 标准数据帧，DLC=0，仅作触发 | A 板回 `0x666` 标准数据帧，两板 `Req:` 更新 |
| 4 | 请求—应答（遥控帧请求） | B 板 | 单击 PB11（KEY_UP） | `0x789` 标准遥控帧，DLC=0 | A 板同样回 `0x666`，两板 `Req:` 更新 |

显示细节：

- `Tim:`/`Tri:`/`Req:` 三条发送流各用独立数据缓冲、各自递增，互不影响；
- 顺序固定为"先自增 → 再发送 → 再刷新"，发送板显示的就是实际发出的帧内容，两板数值一致。

### OLED 布局

```
Tx:  / Rx:      第 1 行 (y=0)
Tim:            第 2 行 (y=12)
Req:            第 3 行 (y=24)
Tri:            第 4 行 (y=36)
（接收板）       第 5 行 (y=48)：x=6 起超时显示 "TIMEOUT!"，收到报文恢复后清除
```

### 全通联调步骤

1. A 板烧 `SENDER + ALL_PASS`，B 板烧 `RECEIVER + ALL_PASS`，接好总线上电。
2. A 板开始定时发送，两板 `Tim:` 行数值一致并递增。
3. 按 A 板 PB11，两板 `Tri:` 行更新。
4. 按 B 板 PB1（数据帧请求）或 PB11（遥控帧请求），A 板均回 `0x666`，两板 `Req:` 行更新。
5. 超时实验：拔掉 B 板 CANH 线（或停掉 A 板）→ 约 300ms 后 `Tim:` 停止、第 5 行出现 `TIMEOUT!`；恢复接线后文字清除、计数继续。

---

## 5. 四种硬件滤波器测试配置

选中任一滤波配置后，发送板和接收板的行为一起切换：

- 发送板：按一次 PB11 发送测试帧表里的下一帧，测试数据首字节自增；OLED 显示 `Tx 序号/总数 累计次数`。
- 接收板：通用显示最近一帧通过滤波器的报文：第一行 `Std-D / Std-R / Ext-D / Ext-R 0xID`，第二行数据，第三行 `DLC` 和累计计数。
- `ALL_PASS` 仍保持第 4 节的业务行为，不使用测试帧表。

### 配置明细

| 配置 | 滤波器设置 | 测试帧表 | 预期通过帧 |
|---|---|---|---|
| `ALL_PASS` | 32 位掩码，ID=0，掩码=0，全部放行 | 8 帧（不参与发送） | 全部通过 |
| `16BIT_LIST` | 16 位列表，Bank 0 的 4 个列表项：`0x345`、`0x000`、`0x234`、`0x567`（均为标准数据帧项） | 6 帧 | `0x345`、`0x567`（2/6） |
| `16BIT_MASK` | 16 位掩码，两组：`0x320~0x32F` 和 `0x200~0x2FF` 的标准数据帧（掩码含 IDE/RTR 匹配位 `0x10|0x08`） | 16 帧 | 两段范围内的 6 个数据帧（6/16） |
| `32BIT_LIST` | 32 位列表，两个列表项：标准 `0x123` 数据帧、扩展 `0x12345678` 数据帧 | 6 帧 | `0x123`、`0x12345678`（2/6） |
| `32BIT_MASK` | 32 位掩码，扩展 ID 高 21 位等于 `0x123456`，要求扩展帧、数据帧 | 8 帧 | `0x12345600/01/03/FF`（4/8） |

- `16BIT_MASK` 的掩码要求标准数据帧，帧表中 `0x201`、`0x321` 两个遥控帧被挡掉；若要放行需清掉 `0x10|0x08` 匹配位。
- `16BIT_LIST` 帧表中的 `0x234` 是遥控帧、列表项是数据帧项，因此不通过。
- 以上通过数量按当前代码与 RM0008 寄存器位布局推导，台面实测回填见 §8。

### 测试帧表概况

| 配置 | 条数 | 内容 |
|---|---|---|
| `ALL_PASS` | 8 | `0x12345600/01/03/FF`、`0x0789AB00/01/03/0F`，全部扩展数据帧 |
| `32BIT_MASK` | 8 | 与 `ALL_PASS` 相同 |
| `32BIT_LIST` | 6 | 标准 `0x123/0x234/0x345/0x456`、扩展 `0x12345678`、`0x0789ABCD` |
| `16BIT_MASK` | 16 | 四段 `0x100x/0x200x/0x300x/0x320x` 各 4 帧；每段第 2 帧为遥控帧 |
| `16BIT_LIST` | 6 | 标准 `0x123`、`0x234`（遥控）、`0x345`、`0x456`、`0x567`、`0x678` |

---

## 6. 滤波实验方法

```
A 板（SENDER + 目标测试配置）
   │ 每按一次 PB11，按顺序发送测试帧表里的下一帧；数据首字节自增
   ▼
B 板（RECEIVER + 同一测试配置）
   通用显示最近一帧通过滤波器的报文，累计计数
```

1. A 板烧 `SENDER + 目标测试配置`，B 板烧 `RECEIVER + 同一测试配置`，接同一条总线上电。
2. 反复按 A 板 PB11 直到帧表轮完一遍，记录 B 板显示的 ID 与计数。
3. 与 §5 的预期通过集合对照。

注意：`ALL_PASS` 接收分支只显示三种业务帧，不能当作"显示全部帧"的对照板。需要对照时用 CAN 分析仪，或给显示分支补通用模式。

---

## 7. 编译与烧录

1. Keil MDK-ARM 打开 `MDK-ARM/STM32-CAN.uvprojx`。
2. 按 §3 修改 `User/Config/project.h` 的两个宏。
3. F7 编译，F8 下载（ST-Link / J-Link）。换板重复第 2~3 步。

工具链：Keil μVision V5.24.2 + ARMCC V5.06 update 5，C99，-O1，microlib。编译产物在 `MDK-ARM/STM32-CAN/`。

- 新增 `.c/.h` 必须手动加入 Keil 工程分组（`User/Config` 已在头文件搜索路径）。
- 工程由 CubeMX 生成，用户代码位于 `USER CODE BEGIN/END` 标记内，重新生成 `.ioc` 不覆盖用户代码。
- 程序占用参考值待加入 Bus-Off 恢复与超时监控后以最新编译结果更新；编译可能出现文件末尾缺换行、条件编译下业务帧结构体未引用（`#177-D`）等警告，不影响功能。

---

## 8. 已知限制与后续计划

已实现：ABOM=0 软件 Bus-Off 检测与 L1/L2 恢复、ESR/TEC/LEC 快照、报文超时监控（覆盖周期报文 `0x0789AB0F`）。

- [ ] 接收缓存是单帧缓存，高总线负载时后到帧会覆盖未处理的帧 → 计划改为帧队列
- [ ] `HAL_CAN_AddTxMessage` 返回值未检查，3 个发送邮箱全忙时帧会静默丢弃 → 应用层补发送状态判断和重试
- [ ] Bus-Off 恢复不重发故障期间丢弃的报文（单次发送语义下重试归应用层），且退出初始化模式时挂起邮箱中的旧请求可能被重新发出 → 计划补发送队列
- [ ] 超时监控目前只覆盖 1 个 ID，扩监控项步骤见 §2 → ADC 上报帧接入时按流程添加
- [ ] `tim_device.c` 的 100ms 标志尚未加 `volatile`（当前通过跨文件函数读取，-O1 下可用）
- [ ] 滤波配置的通过集合尚未台面实测回填，`16BIT_MASK` 的遥控帧匹配位和 `16BIT_LIST` 的 `0x234` 列表项需实测确认
- [ ] 全通接收分支不能作为滤波实验的全量显示对照 → 补通用显示模式或使用 CAN 分析仪
- [ ] 帧数据完整性只校验 DLC，没有 CRC 或帧计数域
- [ ] PB12/PB13 已在按键模块注册，但 GPIO 初始化未配置；当前 CAN 业务不使用这两个按键

---

## 9. 相关项目

- 多任务系统与分层架构实践：[DingDa-H/stm32f1_rtos](https://github.com/DingDa-H/stm32f1_rtos)（FreeRTOS + 四层架构，本项目 Config/Device/Middle/APP 分层与其同源）
