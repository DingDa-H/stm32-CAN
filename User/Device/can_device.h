/**
 * @brief 		can通信实现-回环测试
 * @version 	DingDa-H
 * @data 		20260630
 * @note 		........
 */

#ifndef __CAN_DEVICE_H
#define __CAN_DEVICE_H

#include "stm32f1xx_hal.h"
#include "project.h"
#include "can_cfg.h"
/**
 * @brief 	CAN 发送帧参数结构体
 * @note 	封装一帧 CAN 报文所需的所有参数
 */
typedef struct
{
	uint32_t 	ulID;		/* 帧 ID（标准帧 11 位 / 扩展帧 29 位） */
	uint8_t 	ucDLC;		/* 数据长度 (0~8) */
	uint8_t 	*pucData;	/* 发送数据缓冲区指针 */
	uint32_t 	ulIDE;		/* ID 类型：CAN_ID_STD 或 CAN_ID_EXT */
	uint32_t 	ulRTR;		/* 帧类型：CAN_RTR_DATA 或 CAN_RTR_REMOTE */
} stCanTxParamTdf;
/**
 * @brief 	CAN 接收帧结构体
 * @note 	封装一帧完整的 CAN 接收报文（帧头信息 + 数据），用于环形队列的整体入队与出队
 */
typedef struct {
    CAN_RxHeaderTypeDef stHeader; // 帧信息
    uint8_t aucData[8];           // 数据
} stCanRxFrameTdf;

uint8_t ucDeviceCan_Transmit(const stCanTxParamTdf *pstTxMsg);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

uint8_t ucDeviceCan_ReadRxFrame(stCanRxFrameTdf *pstFrame);
uint32_t ulDeviceCan_GetRxOverflowCount(void);
uint32_t ulDeviceCan_GetTxFailCount(void);

void vCanDeviceInit(const CAN_FilterTypeDef *pstFilter);

uint8_t ucDeviceCan_GetBusOffFlag(void);
void vDeviceCan_ClearBusOffFlag(void);

// 新增：获取 Bus-Off 时esr里的信息
void vCanDeviceGetBusOffSnapshot(Can_BusOffSnapshotTypeDef *pstSnapshot);


uint8_t ucCanDevice_ExitInitMode(void);
uint8_t ucCanDevice_EnterInitMode(void);
#endif
