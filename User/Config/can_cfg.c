#include "can_cfg.h"

/* 连续触发 2 次 Bus-Off，进入 L2 慢恢复 */
const uint8_t CanSMBorCounterL1ToL2 = 2U; 

/* L1 快恢复延时（毫秒） */
const uint32_t CanSMBorTimeL1 = 100U;      

/* L2 慢恢复延时（毫秒） */
const uint32_t CanSMBorTimeL2 = 2000U;    

const uint32_t CanSMBorTimeTxEnsured = 300U;

/* ============ 报文超时监控表（只读配置，与 can_timeout_mid.c 的运行状态一一对应） ============ */
const stCanTimeoutConfigTdf CanTimeoutConfigList[CAN_TIMEOUT_MONITOR_NUM] =
{
    { CAN_ID_TIMING_MSG, CAN_TIMEOUT_TIMING_MS },   // 周期报文 0x0789AB0F
};
