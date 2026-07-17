
#include "stm32f1xx_hal.h"
#include <can.h>
	
extern CAN_HandleTypeDef hcan;

static uint8_t ucRxFlag = 0;						// 新增接收标志：中断置1，主循环读取后置0

static CAN_RxHeaderTypeDef s_rxHeader;				// 接收报文的帧头信息

static uint8_t s_aucRxBuf[8];						// 全局接收缓存

/**
 * @brief   发送一帧 CAN 报文
 * @param   pstTxMsg    CAN 发送帧参数结构体指针
 * @note	仅进行单次发送，发送失败需要重复发送的处理放在应用层
 */
void vMyCan_Transmit(const stCanTxParamTdf *pstTxMsg)
{
	CAN_TxHeaderTypeDef CAN_TxHeader;

	CAN_TxHeader.IDE = pstTxMsg->ulIDE;
	CAN_TxHeader.DLC = pstTxMsg->ucDLC;
	CAN_TxHeader.RTR = pstTxMsg->ulRTR;
	CAN_TxHeader.TransmitGlobalTime = DISABLE;

	if (pstTxMsg->ulIDE == CAN_ID_STD) {
        CAN_TxHeader.StdId = pstTxMsg->ulID;
    } else {
        CAN_TxHeader.ExtId = pstTxMsg->ulID;
    }
	uint32_t ulTxMailbox;					//返回发送所使用的邮箱号
	HAL_CAN_AddTxMessage(&hcan, &CAN_TxHeader, pstTxMsg->pucData, &ulTxMailbox);
}

/**
 * @brief   查询当前空闲邮箱数量函数
 * @note	用于轮询是否接收到数据，接收到返回1
 */
//uint8_t ucMyCan_ReceiveFlag(void)
//{
//	if(HAL_CAN_GetRxFifoFillLevel(&hcan,CAN_RX_FIFO0) > 0)
//	{
//		return 1;
//	}
//	return 0;
//}
/**
 * @brief 		接收中断处理函数
 * @param 	
 * @retval 		
 * @note 		FIFO0收到报文自动执行
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 判断是不是CAN1的中断
    if(hcan->Instance == CAN1)
    {
        // 读取FIFO0整帧报文
        HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &s_rxHeader, s_aucRxBuf);
        // 置标志，主循环处理数据，禁止在这里刷屏、延时
        ucRxFlag = 1;
    }
}

void vMyCan_Receive(CAN_RxHeaderTypeDef *pHeader, uint8_t *RxData)
{
	HAL_CAN_GetRxMessage(&hcan,CAN_RX_FIFO0,pHeader,RxData);
}


// 获取接收标志
uint8_t ucBspCan_GetRxFlag(void)
{
    return ucRxFlag;
}
// 清除接收标志
void vBspCan_ClearRxFlag(void)
{
    ucRxFlag = 0;
}
// 读取帧头数据
void vBspCan_GetRxHeader(CAN_RxHeaderTypeDef *pHeader)
{
    *pHeader = s_rxHeader;
}
// 读取接收数据数组
void vBspCan_GetRxData(uint8_t *pBuf)
{
    uint8_t len = s_rxHeader.DLC;
    for(uint8_t i = 0; i < len; i++)
    {
        pBuf[i] = s_aucRxBuf[i];
    }
}