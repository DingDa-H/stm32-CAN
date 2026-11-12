/**
 * @brief 		CAN 状态管理（CanSM）服务层
 * @version 	DingDa-H
 * @data 		20261111
 * @note 		Bus-Off 检测与恢复状态机
 */

#include "can_sm_mid.h"
#include "can_device.h"              // Bus-Off 标志读写、ESR 快照原语
#include "can_cfg.h"                 // L1/L2 阈值与等待时间参数、Can_BusOffSnapshotTypeDef
#include "stm32f1xx_hal.h"           // HAL_GetTick、CAN_ESR_* 宏

// 全局调试变量（volatile 防止编译器优化，供 Watch 窗口查看 ESR/TEC/LEC）
volatile uint32_t g_ulDebugBusOffEsr = 0U;
volatile uint8_t  g_ucDebugBusOffTec = 0U;
volatile uint8_t  g_ucDebugBusOffLec = 0U;

static Can_BusOffSnapshotTypeDef s_stBusOffSnapshot;            // Bus-Off 现场快照（ESR/TEC/LEC），用于故障诊断记录
static volatile uint32_t s_ulBusOffCounter = 0U;               	// 连续触发次数（volatile 便于 Debug 观测）
static volatile emBusoffState emBusoffStateCur = emBusoffIdle; 	// 状态机当前状态（Watch 窗口可见）

// L1/L2 等待窗口的起始时刻（HAL_GetTick）：跨调用保持，必须是 static；只在任务上下文读写，不需要 volatile
static uint32_t ulBusoffTimestamp = 0U;
// 本次选定的等待时长（ms）：同样跨调用保持
static uint32_t ulWaitTime = 0U;

/**
 * @brief   Bus-Off 故障现场抓取与恢复状态机触发
 * @note    在业务主循环（任务）中周期性调用
 */
void vCanSmMainFunction(void)
{
	// 记录是否发生过busoff，并且每圈重置
	uint8_t ucValidEvent = 0U;
    // 1.检查是否发生Bus-Off事件->然后采集Esr、Tec、Lec信息
    if (ucDeviceCan_GetBusOffFlag() == 1U)
    {
        // 第一步：先抓取故障快照（寄存器）
        vCanDeviceGetBusOffSnapshot(&s_stBusOffSnapshot);
        // 第二步：立刻清除事件标志，防止重复触发
        vDeviceCan_ClearBusOffFlag();

        // 核心二次核验：只有快照里 BOFF 位确实置 1，才算有效 BusOff
        if ((s_stBusOffSnapshot.ulEsrVal & CAN_ESR_BOFF) != 0U)
        {
            g_ulDebugBusOffEsr = s_stBusOffSnapshot.ulEsrVal;
            g_ucDebugBusOffTec = s_stBusOffSnapshot.ucTecVal;
            g_ucDebugBusOffLec = s_stBusOffSnapshot.ucLecVal;
            s_ulBusOffCounter++;
			ucValidEvent = 1U;
        }
    }
	
	// 2. 状态流转：每圈都执行，不依赖事件 
	switch(emBusoffStateCur)
	{
		case emBusoffIdle:
		{
			if(ucValidEvent == 1U)
			{
				ulBusoffTimestamp = HAL_GetTick();
				// 计数小于阈值选 L1 快恢复
				if (s_ulBusOffCounter < CanSMBorCounterL1ToL2)
				{
					ulWaitTime = CanSMBorTimeL1;
					emBusoffStateCur = emBusoffWaitRecoveryL1;
				}
				else
				{
					ulWaitTime = CanSMBorTimeL2;
					emBusoffStateCur = emBusoffWaitRecoveryL2;
				}
			}
			
		}
			break;
		//等 L1 快恢复时长（计时，到时间进入下一状态）
		case emBusoffWaitRecoveryL1:
		//等 L2 慢恢复时长
		case emBusoffWaitRecoveryL2:
		{
			if((HAL_GetTick() - ulBusoffTimestamp) >= ulWaitTime)
			{
				emBusoffStateCur = emBusoffRecovering;
			}
		}
			break;
		//正在执行恢复序列（进/出初始化模式）
		case emBusoffRecovering:
		{
			if ((ucCanDevice_EnterInitMode() == 0U) && (ucCanDevice_ExitInitMode() == 0U))
			{
				emBusoffStateCur = emBusoffObserving;
				ulBusoffTimestamp = HAL_GetTick();		//记录当前时间用于稳定判定
			}
			/*
			这意味着：
			硬件层面能进入初始化模式（说明控制器本身没死）。
			但退出初始化、重新接入总线时失败了，可能是 HAL_CAN_Start 内部超时、
			INAK 位没按预期变化、或者物理层短路/断路导致控制器重新上线时立刻又触发错误。
			这说明故障不是“偶发干扰”，而是持续存在的硬故障。
			这种情况下，继续用 L1（50ms/100ms）快恢复是徒劳的，
			只会让节点在总线上反复“抽风”。
			*/
			else
			{
				ulWaitTime = CanSMBorTimeL2;          	/* 发起失败：退避到 L2 再试 */
				ulBusoffTimestamp = HAL_GetTick();
				emBusoffStateCur = emBusoffWaitRecoveryL2;
			}
		}
			break;
		//恢复已发起，在判定窗口内观察是否再次 Bus-Off
		case emBusoffObserving:
			if (ucValidEvent == 1U)
            {
                /* 窗口内再次 Bus-Off：失败，计数已在事件段 +1，重新评估 */
                ulBusoffTimestamp = HAL_GetTick();
                if (s_ulBusOffCounter < CanSMBorCounterL1ToL2)
                { ulWaitTime = CanSMBorTimeL1; emBusoffStateCur = emBusoffWaitRecoveryL1; }
                else
                { ulWaitTime = CanSMBorTimeL2; emBusoffStateCur = emBusoffWaitRecoveryL2; }
            }
            else if ((HAL_GetTick() - ulBusoffTimestamp) >= CanSMBorTimeTxEnsured)
            {
                s_ulBusOffCounter = 0U;               /* 成功：连续计数清零 */
                emBusoffStateCur = emBusoffIdle;
            }
            break;
		default:
			break;
	}
}
