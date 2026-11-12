/**
 * @brief 		CAN 状态管理（CanSM）服务层
 * @version 	DingDa-H
 * @data 		20261111
 * @note 		Bus-Off 检测与恢复状态机，只依赖驱动层（can_device）和配置层（can_cfg）参数
 */

#ifndef __CAN_SM_MID_H
#define __CAN_SM_MID_H

/**
 * @brief   Bus-Off状态机
 * @note    状态枚举
 */
typedef enum {
    emBusoffIdle = 0,          // 正常，无 Bus-Off
    emBusoffWaitRecoveryL1,    // 已确认 Bus-Off，正在等 L1 快恢复时长
    emBusoffWaitRecoveryL2,    // 已确认 Bus-Off，正在等 L2 慢恢复时长
    emBusoffRecovering,        // 正在执行恢复序列（进/出初始化模式）
    emBusoffObserving          // 恢复已发起，在判定窗口内观察是否再次 Bus-Off
} emBusoffState;

/**
 * @brief   Bus-Off 监控与恢复主函数
 * @note    由主循环任务周期调用，对应 AUTOSAR CanSM_MainFunction
 */
void vCanSmMainFunction(void);

#endif /* __CAN_SM_MID_H */
