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

#endif /* __CAN_CFG_H */
