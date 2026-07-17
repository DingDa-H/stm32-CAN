/**
 * @brief 		can通信实现-回环测试
 * @version 	DingDa-H
 * @data 		20260630
 * @note 		........
 */

#ifndef __CAN_H
#define __CAN_H

#include "stm32f1xx_hal.h"
#include "project.h"
#include "oled_device.h"

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


void vMyCan_Transmit(const stCanTxParamTdf *pstTxMsg);
void vMyCan_Receive(CAN_RxHeaderTypeDef *pHeader, uint8_t *RxData);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
//uint8_t ucMyCan_ReceiveFlag(void);

uint8_t ucBspCan_GetRxFlag(void);
void vBspCan_ClearRxFlag(void);
void vBspCan_GetRxHeader(CAN_RxHeaderTypeDef *pHeader);
void vBspCan_GetRxData(uint8_t *pBuf);
#endif
