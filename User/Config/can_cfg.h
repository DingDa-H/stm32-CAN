#ifndef __CAN_CFG_H
#define __CAN_CFG_H

#include <stdint.h>

/* Bus-Off 恢复参数（只读常量，放 Flash） */
extern const uint8_t  CanSMBorCounterL1ToL2;			//恢复等级的阈值
extern const uint32_t CanSMBorTimeL1;					//等级1的等待时间
extern const uint32_t CanSMBorTimeL2;					//等级2的等待时间

extern const uint32_t CanSMBorTimeTxEnsured;			//恢复后的观察时间

/* BusOff 故障快照（esr），用于诊断日志 */
typedef struct
{
    uint32_t ulEsrVal;
    uint8_t  ucTecVal;
    uint8_t  ucLecVal;
} Can_BusOffSnapshotTypeDef;

/* ============ 报文超时监控配置 ============ */
#define CAN_ID_TIMING_MSG        0x0789AB0F    /* 被监控的周期报文 ID（收发两端共用唯一定义处） */
#define CAN_TIMEOUT_TIMING_MS    300U          /* 超时阈值（ms）：100ms 周期允许连丢 3 帧 */
#define CAN_TIMEOUT_MONITOR_NUM  1U            /* 监控表条目数量 */

/* 监控只读配置（Flash）；运行期状态留在 can_timeout_mid.c 的 RAM 数组，配置与状态分家 */
typedef struct
{
    uint32_t ulMonitorId;       /* 需要监控的 CAN ID */
    uint32_t ulTimeoutMs;       /* 超时阈值（ms） */
} stCanTimeoutConfigTdf;

extern const stCanTimeoutConfigTdf CanTimeoutConfigList[CAN_TIMEOUT_MONITOR_NUM];

#endif /* __CAN_CFG_H */
