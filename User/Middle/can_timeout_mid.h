// can_timeout_mid.h
#ifndef __CAN_TIMEOUT_MID_H
#define __CAN_TIMEOUT_MID_H

#include <stdint.h>
#include "can_cfg.h"        // 只读配置表 CanTimeoutConfigList 与 CAN_TIMEOUT_MONITOR_NUM
// 配置数据在 can_cfg（Flash 只读），本模块只持有运行期状态（RAM）

// 对外接口
void vCanTimeoutInit(void);
void vCanTimeoutUpdate(uint32_t ulRxId);
void vCanTimeoutTask(void);
uint8_t ucCanTimeoutGetFlag(uint32_t ulRxId);

#endif
