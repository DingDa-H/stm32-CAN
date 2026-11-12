#include "can_cfg.h"

/* 连续触发 2 次 Bus-Off，进入 L2 慢恢复 */
const uint8_t CanSMBorCounterL1ToL2 = 2U; 

/* L1 快恢复延时（毫秒） */
const uint32_t CanSMBorTimeL1 = 100U;      

/* L2 慢恢复延时（毫秒） */
const uint32_t CanSMBorTimeL2 = 2000U;    

const uint32_t CanSMBorTimeTxEnsured = 300U;
