
#include "stm32f1xx_hal.h"
#include <can_device.h>
	
extern CAN_HandleTypeDef hcan;

static stCanRxFrameTdf s_astRxQueue[RX_QUEUE_SIZE]; // 真正的存储池(环形队列)

// s_ucRxHead == s_ucRxTail为空、s_ucRxHead的下一个位置等于s_ucRxTail为满
static volatile uint8_t s_ucRxHead = 0;             // 中断写的位置
static volatile uint8_t s_ucRxTail = 0;             // 任务读的位置

static volatile uint32_t s_ulRxOverflowCount = 0U;	// 队列满、来数据s_ulRxOverflowCount++表示溢出
static volatile uint32_t s_ulTxFailCount = 0U;		// 发送失败累计：邮箱满或控制器未就绪
static volatile uint8_t s_ucBusOffFlag = 0U;		// BusOff中断标志

/**
 * @brief   发送一帧 CAN 报文
 * @param   pstTxMsg    CAN 发送帧参数结构体指针
 * @retval 	返回 0=已入邮箱、1=发送失败（邮箱满或控制器未就绪）
 * @note	仅进行单次发送，发送失败需要重复发送的处理放在应用层
 */
uint8_t ucDeviceCan_Transmit(const stCanTxParamTdf *pstTxMsg)
{
	CAN_TxHeaderTypeDef CAN_TxHeader = {0};

	CAN_TxHeader.IDE = pstTxMsg->ulIDE;
	CAN_TxHeader.DLC = pstTxMsg->ucDLC;
	CAN_TxHeader.RTR = pstTxMsg->ulRTR;
	CAN_TxHeader.TransmitGlobalTime = DISABLE;

	if (pstTxMsg->ulIDE == CAN_ID_STD) {
        CAN_TxHeader.StdId = pstTxMsg->ulID;
    } else {
        CAN_TxHeader.ExtId = pstTxMsg->ulID;
    }
	uint32_t ulTxMailbox = 0U;				//返回发送所使用的邮箱号

	/* HAL 内部同时检查空闲邮箱和控制器状态，返回值即为本次发送结果 */
	if (HAL_CAN_AddTxMessage(&hcan, &CAN_TxHeader, pstTxMsg->pucData, &ulTxMailbox) != HAL_OK)
	{
		s_ulTxFailCount++;
		return 1U;
	}
	return 0U;
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
 * @note 		FIFO0收到报文自动执行、入队
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 判断是不是CAN1的中断
    if(hcan->Instance == CAN1)
    {
		uint8_t ucNextHead = (uint8_t)((s_ucRxHead + 1U) % RX_QUEUE_SIZE);//ucNextHead指下一次写入的位置，不是当前写入位置
        //队列未满
		if (ucNextHead != s_ucRxTail)					
		{
			// 读取FIFO0整帧报文入队
			HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &s_astRxQueue[s_ucRxHead].stHeader, s_astRxQueue[s_ucRxHead].aucData);
			// 置标志，主循环处理数据，禁止在这里刷屏、延时
			s_ucRxHead = ucNextHead;
		}
		else
		{
			// 4. 队列满了，丢弃最新帧。
			// 注意：此时硬件 FIFO 里依然有这帧数据，如果不读出来，中断标志不会清除。
			// 所以我们需要假装读一下，或者用一个临时的空结构体把它读走丢掉。
			CAN_RxHeaderTypeDef stDummyHeader;
			uint8_t aucDummyData[8];
			HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &stDummyHeader, aucDummyData);
			
			s_ulRxOverflowCount++; 
		}
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
/**
 * @brief 		CAN 接收帧出队
 * @param 		pstFrame 应用层传入的空结构体指针，用于接收出队的数据
* @retval 		1U 表示队列不为空， 0U 表示队列为空
 * @note 		非阻塞接口。从硬件接收环形队列中取出最旧的一帧，
 * 				通过指针直接将数据拷贝给应用层，并推进读指针（Tail）。
 */
uint8_t ucDeviceCan_ReadRxFrame(stCanRxFrameTdf *pstFrame)
{
    // 0. 入参检查：空指针直接返回“无数据”
    if (pstFrame == NULL)
    {
        return 0U;
    }

    // 1. 判空：如果头指针和尾指针相等，说明里面没东西
    if (s_ucRxHead == s_ucRxTail)
    {
        return 0U; // 返回 0，告诉应用层“没数据了”
    }

    // 2. 取数据：把 Tail 指向的那一帧，整体拷贝给应用层传进来的空盒子
    // 直接把结构体赋值给指针指向的内存（C语言支持整体赋值，相当于 memcpy）
    *pstFrame = s_astRxQueue[s_ucRxTail];

    // 3. 移动尾指针：数据被取走了，Tail 往前走一步（遇到末尾绕回头部）
    s_ucRxTail = (uint8_t)((s_ucRxTail + 1U) % RX_QUEUE_SIZE);

    // 4. 返回 1，不为空，有数据
    return 1U;
}

/**
 * @brief 		读取接收队列溢出累计次数
 * @retval 		队列满时被丢弃的帧数
 * @note 		供台架验证与调试观察，只读接口
 */
uint32_t ulDeviceCan_GetRxOverflowCount(void)
{
    return s_ulRxOverflowCount;
}

/**
 * @brief 		读取发送失败累计次数
 * @retval 		发送失败（邮箱满或控制器未就绪）的次数
 * @note 		供台架验证与调试观察，只读接口
 */
uint32_t ulDeviceCan_GetTxFailCount(void)
{
    return s_ulTxFailCount;
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
	HAL_CAN_AbortTxRequest(&hcan, CAN_TX_MAILBOX0|CAN_TX_MAILBOX1|CAN_TX_MAILBOX2);
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
