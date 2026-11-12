
#include "stm32f1xx_hal.h"
#include <can_device.h>
	
extern CAN_HandleTypeDef hcan;

static volatile uint8_t ucRxFlag = 0;				// 接收标志：中断置1，主循环读取后清0（volatile：禁止编译器缓存）

static CAN_RxHeaderTypeDef s_rxHeader;				// 接收报文的帧头信息

static uint8_t s_aucRxBuf[8];						// 全局接收缓存

static volatile uint8_t s_ucBusOffFlag = 0U;		// BusOff中断标志

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
 * @brief 		can初始化函数
 * @version 	
 * @data 			
 * @note 		按 project.h 里的 CAN_TEST_PROFILE 配置滤波器、开启接收中断
 */
// can_device.c
void vCanDeviceInit(const CAN_FilterTypeDef *pstFilter)
{
    // 直接配底层
    HAL_CAN_ConfigFilter(&hcan, pstFilter);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_BUSOFF | CAN_IT_ERROR);
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
/**
 * @brief 		busoff中断处理函数
 * @param 	
 * @note 		判断是否busoff，如果是就置标志位
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	if (hcan->Instance == CAN1)			//防止进入can2的中断
	{
		if((hcan->Instance->ESR&CAN_ESR_BOFF)!=0U)
		{
			s_ucBusOffFlag = 1U;
		}
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

// 获取 BusOff 标志
uint8_t ucDeviceCan_GetBusOffFlag(void)
{
    return s_ucBusOffFlag;
}
// 清除 BusOff 标志
void vDeviceCan_ClearBusOffFlag(void)
{
    s_ucBusOffFlag = 0U;
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

/**
 * @brief 		获取 Bus-Off 现场快照
 * @param 		pstSnapshot 用于接收 ESR、TEC、LEC 的结构体指针
 * @note 		必须在 CAN 重新初始化恢复之前调用，否则寄存器会被清零
 */
void vCanDeviceGetBusOffSnapshot(Can_BusOffSnapshotTypeDef *pstSnapshot)
{
    if (pstSnapshot == NULL) return;

    // 读取错误状态寄存器 (ESR)
    uint32_t ulEsrVal = hcan.Instance->ESR;
    
    pstSnapshot->ulEsrVal = ulEsrVal;
    
    // TEC 位于 ESR bit 23:16
	// CAN_ESR_TEC_Msk 是 0x00FF0000，右移 16 位得到真实的计数器值
    pstSnapshot->ucTecVal = (uint8_t)((ulEsrVal & CAN_ESR_TEC_Msk) >> CAN_ESR_TEC_Pos);
    
    // LEC (Last Error Code) 位于 ESR bit 6:4
	// CAN_ESR_LEC_Msk 是 0x00000070，右移 4 位得到真实的错误代码
    pstSnapshot->ucLecVal = (uint8_t)((ulEsrVal & CAN_ESR_LEC_Msk) >> CAN_ESR_LEC_Pos);
}
/**
 * @brief 		CAN 控制器进入初始化模式
 * @data 		
 * @retval 		返回 0U 成功、1U 失败
 * @note		调用 HAL_CAN_Stop，硬件会置位 MCR.INRQ 并等待 MSR.INAK 确认，
 */
uint8_t ucCanDevice_EnterInitMode(void)
{
    if (HAL_CAN_Stop(&hcan) != HAL_OK)
    {
        /* HAL 状态机可能因上次超时锁在 ERROR，重走 Init 解锁后再试一次 */
        if (HAL_CAN_Init(&hcan) != HAL_OK)
        {
            return 1U;
        }
        if (HAL_CAN_Stop(&hcan) != HAL_OK)
        {
            return 1U;
        }
    }
    return 0U;
}
/**
 * @brief 		CAN 控制器退出初始化模式
 * @data 		
 * @retval 		返回 0U 成功、1U 失败
 * @note		调用 HAL_CAN_Start，硬件会清除 MCR.INRQ 位，并等待 MSR.INAK 确认退出初始化
 */
uint8_t ucCanDevice_ExitInitMode(void)
{
	if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        return 1U; // 失败
    }
    return 0U; // 成功
}