// can_timeout_mid.c
#include "can_timeout_mid.h"
#include "can_cfg.h"                // 监控配置表（ID/阈值，Flash 只读）
#include "stm32f1xx_hal.h"

/* 运行期状态（RAM）：与 CanTimeoutConfigList[i] 一一对应，配置与状态分家 */
typedef struct
{
    uint32_t ulLastRxTick;      // 最后一次收到该报文的时间戳
    uint8_t  ucIsStarted;       // 是否已开始监控（防止上电没收到就立刻报错）
    uint8_t  ucIsTimeout;       // 超时标志（1=超时，0=正常）
} stCanTimeoutStateTdf;

static stCanTimeoutStateTdf s_astTimeoutState[CAN_TIMEOUT_MONITOR_NUM];

/**
 * @brief 	初始化函数
 * @note	复位监控运行状态，防止刚上电、一帧未收时被误判
 */
void vCanTimeoutInit(void)
{
    for (uint8_t i = 0; i < CAN_TIMEOUT_MONITOR_NUM; i++)
    {
        s_astTimeoutState[i].ulLastRxTick = 0U;
        s_astTimeoutState[i].ucIsTimeout = 0U;
        s_astTimeoutState[i].ucIsStarted = 0U;
    }
}

// 收到报文时调用，匹配配置表 ID 则更新“喂狗”时间戳
void vCanTimeoutUpdate(uint32_t ulRxId)
{
    for (uint8_t i = 0; i < CAN_TIMEOUT_MONITOR_NUM; i++)
    {
        if (CanTimeoutConfigList[i].ulMonitorId == ulRxId)
        {
            s_astTimeoutState[i].ulLastRxTick = HAL_GetTick();
            s_astTimeoutState[i].ucIsStarted = 1U; // 收到了第一帧，开始正式监控
            s_astTimeoutState[i].ucIsTimeout = 0U; // 超时标志置0（表示正常）
        }
    }
}

// 周期任务里调用，检查是否超时
void vCanTimeoutTask(void)
{
    uint32_t ulNow = HAL_GetTick();
    for (uint8_t i = 0; i < CAN_TIMEOUT_MONITOR_NUM; i++)
    {
        // 只有开始监控了，才判断超时
        if (s_astTimeoutState[i].ucIsStarted == 1U)
        {
            // 无符号减法，对 tick 溢出免疫
            if ((ulNow - s_astTimeoutState[i].ulLastRxTick) > CanTimeoutConfigList[i].ulTimeoutMs)
            {
                s_astTimeoutState[i].ucIsTimeout = 1U;
            }
        }
    }
}

// 供应用层查询超时状态；不在监控表里的 ID 返回 0（正常）
uint8_t ucCanTimeoutGetFlag(uint32_t ulRxId)
{
    for (uint8_t i = 0; i < CAN_TIMEOUT_MONITOR_NUM; i++)
    {
        if (CanTimeoutConfigList[i].ulMonitorId == ulRxId)
        {
            return s_astTimeoutState[i].ucIsTimeout;
        }
    }
    return 0U;
}
