#include "stm32f1xx_hal.h"
#include <can.h>
#include "string.h"
#include <stdint.h>
#include <oled_device.h>
#include "button_mid.h"
#include "task.h"
#include <stdio.h>


extern CAN_HandleTypeDef hcan;
//static uint32_t s_ulRxId;
//static uint8_t  s_ucRxLength;

//定时触发标志位
static uint8_t s_ucTimFlag;

//触发发送标志位
static uint8_t s_ucTriggerFlag;

//请求发送标志位
static uint8_t s_ucRequestFlag;

// 全局发送缓存
static uint8_t s_Txdata[8] = {0x55,0x66,0x48,0x77};
static uint8_t s_Txdata_1[8] = {0x00};

// 全局发送结构体（使用指定初始化器，与 can.h 中字段顺序一致）
stCanTxParamTdf stTxMsgArray[] =
{
	{.ulID    = 0x12345600,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	{.ulID    = 0x12345601,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	{.ulID    = 0x12345603,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	{.ulID    = 0x123456FF,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},

	{.ulID    = 0x0789AB00,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	{.ulID    = 0x0789AB01,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	{.ulID    = 0x0789AB03,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	{.ulID    = 0x0789AB0F,.ucDLC   = 4,.pucData = s_Txdata,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
};

// 定时发送帧数据
stCanTxParamTdf stTxMsg_Timing =
{
	.ulID    = 0x0789AB0F,
	.ucDLC   = 4,
	.pucData = s_Txdata,
	.ulIDE   = CAN_ID_EXT,
	.ulRTR   = CAN_RTR_DATA,
};
// 触发发送帧数据
stCanTxParamTdf stTxMsg_Trigger =
{
	.ulID    = 0x555,
	.ucDLC   = 4,
	.pucData = s_Txdata,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};
// 请求发送帧数据
stCanTxParamTdf stTxMsg_Request =
{
	.ulID    = 0x777,
	.ucDLC   = 4,
	.pucData = s_Txdata,
	.ulIDE   = CAN_RTR_DATA,
	.ulRTR   = CAN_RTR_REMOTE,
};

// 请求发送帧数据
stCanTxParamTdf stTxMsg_Request_REMOTE =
{
	.ulID    = 0x789,
	.ucDLC   = 4,
	.pucData = s_Txdata_1,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_REMOTE,
};
stCanTxParamTdf stTxMsg_Request_DATA =
{
	.ulID    = 0x666,
	.ucDLC   = 4,
	.pucData = s_Txdata,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};
//static uint8_t i = 0;						//用来表示stTxMsgArray的第i项
/**
 * @brief 		can初始化函数
 * @version 	
 * @data 			
 * @note 		配置过滤器、开启接收中断
 */// service/task.c 应用层
//void vCanService_Init(void)
//{
//	CAN_FilterTypeDef filter;
//	// 202607010,配置为32位列表模式
//	filter.FilterBank 				=0;						//滤波器编号0-13
//	filter.FilterActivation 		=ENABLE;				//是否启用该滤波器
//	filter.FilterFIFOAssignment 	=CAN_FILTER_FIFO0;		// CAN_FILTER_FIFO0 或 CAN_FILTER_FIFO1
//	uint32_t ID = 0x12345600u<<3|(0x4);
//	filter.FilterIdHigh 			=ID>>16;				/* 32位滤波器的高 16 位（对应标准 ID 的 11 位或扩展 ID 的高 16 位） */
//	filter.FilterIdLow 				=ID;				/* 32位滤波器的低 16 位（对应扩展 ID 的低 18 位、IDE、RTR 位等） */
//	uint32_t MASK = (0x1FFFFF00u<<3)|(0x4)|(0x2);
//	filter.FilterMaskIdHigh 		=MASK>>16;				/* 与 FilterIdHigh 对应的掩码高 16 位（1 表示必须匹配，0 表示不关心） */
//	filter.FilterMaskIdLow 			=MASK; 			/* 与 FilterIdLow 对应的掩码低 16 位（1 表示必须匹配，0 表示不关心） */
//	filter.FilterScale 				=CAN_FILTERSCALE_32BIT;	/* 滤波器位宽：16 位或 32 位 */
//	filter.FilterMode 				=CAN_FILTERMODE_IDMASK;	/* 滤波器模式：列表模式（ID 完全匹配）或掩码模式（用掩码筛选） */
//	filter.SlaveStartFilterBank 	=0;
//	HAL_CAN_ConfigFilter(&hcan, &filter);
//	HAL_CAN_Start(&hcan);
//	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);	//激活中断的（根据参数不同指定不同中断）

//}
void vCanService_Init(void)
{
    CAN_FilterTypeDef filter = {0};   // 全部初始化为 0
    filter.FilterBank           = 0;
    filter.FilterActivation     = ENABLE;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;   // 掩码全 0 → 全部通过
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.SlaveStartFilterBank = 0;
    HAL_CAN_ConfigFilter(&hcan, &filter);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}
/**
 * @brief 		OLED初始化函数
 * @version 	
 * @data 			
 * @note 		
 */
void vOledInit(void)
{
	stOledStaticParamTdf stInit;
	stInit.pstSclGpioBase	= GPIOB;
	stInit.pstSdaGpioBase	= GPIOB;
	stInit.usSclGpioPin		= GPIO_PIN_8;
	stInit.usSdaGpioPin		= GPIO_PIN_9;
	
	vOledDeviceInit(&stInit, OLED);
	vOledClearBuffer(OLED);			//清屏
}

/**
 * @brief 		物理按键初始化
 * @param 	
 * @data 		
 * @note 		
 */
void Button_Init(void)
{
	stBtnStaticParamTdf stBtnInit;
	stBtnInit.emButLevel					= emBtnActiveLevel_Low;
	stBtnInit.pstGpioBase					= GPIOB;
	stBtnInit.ulDebounceMs 					= 20;
	stBtnInit.ulLongPressMs					= 100000;			//不允许长按和双击
	stBtnInit.ulDoubleClickMs				= 10;
	stBtnInit.usGpioPin						= GPIO_PIN_1;
	
	vBtnParamInit(&stBtnInit,BUTTON_DOWN);
	
	stBtnInit.emButLevel					= emBtnActiveLevel_Low;
	stBtnInit.pstGpioBase					= GPIOB;
	stBtnInit.ulDebounceMs 					= 20;
	stBtnInit.ulLongPressMs					= 100000;
	stBtnInit.ulDoubleClickMs				= 10;
	stBtnInit.usGpioPin						= GPIO_PIN_11;
	
	vBtnParamInit(&stBtnInit,BUTTON_UP);
	
	stBtnInit.emButLevel					= emBtnActiveLevel_High;
	stBtnInit.pstGpioBase					= GPIOB;
	stBtnInit.ulDebounceMs 					= 20;
	stBtnInit.ulLongPressMs					= 100000;
	stBtnInit.ulDoubleClickMs				= 10;
	stBtnInit.usGpioPin						= GPIO_PIN_12;
	
	vBtnParamInit(&stBtnInit,BUTTON_ENABLE);
	
	stBtnInit.emButLevel					= emBtnActiveLevel_High;
	stBtnInit.pstGpioBase					= GPIOB;
	stBtnInit.ulDebounceMs 					= 20;
	stBtnInit.ulLongPressMs					= 100000;
	stBtnInit.ulDoubleClickMs				= 200;
	stBtnInit.usGpioPin						= GPIO_PIN_13;
	
	vBtnParamInit(&stBtnInit,BUTTON_CANCEL);
}

/**
 * @brief 		接收处理函数
 * @param 	
 * @retval 		
 * @note 		需要轮询
 */
//uint8_t ucMyCanExecute(void)
//{
//    if (ucMyCan_ReceiveFlag()) {
//        vMyCan_Receive(&s_rxHeader, s_aucRxBuf);
//        return 1;   // 有新数据
//    }
//    return 0;       // 无数据
//}

/**
 * @brief 		实现多种发送
 * @param 	
 * @retval 		
 * @note 		多种数据传输策略的实现
 */
//void vtask(void)
//{
//	//定时发送
//	if(s_ucTimFlag)
//	{
//		s_ucTimFlag = 0;
//		
//		vOledWriteStringToBuffer(6,0,(uint8_t*)"Tx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

//		vMyCan_Transmit(&stTxMsg_Timing);
//		s_Txdata[0]++;
//		s_Txdata[1]++;
//		s_Txdata[2]++;
//		s_Txdata[3]++;
//			
//		vOledWriteHexToBuffer(30,12,stTxMsg_Timing.pucData,stTxMsg_Timing.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		vOledRefreshFromBuffer(OLED);
//	}
//	
//	//触发发送
//	
//	vBtnExecute(); 
//	
//	// 复用之前的代码，规定BUTTON_UP是发送。
//	emButEventTdf emEventUp  		 = emBtnGetCurEvent(BUTTON_UP);
//    emButEventTdf emEventDown		 = emBtnGetCurEvent(BUTTON_DOWN);
//	emButEventTdf emEventEnable 	 = emBtnGetCurEvent(BUTTON_ENABLE);
//	emButEventTdf emEventCancel   	 = emBtnGetCurEvent(BUTTON_CANCEL);
//	if (emEventUp == emBtnEvent_Click)
//	{
//		s_ucTriggerFlag = 1;
//		vBtnEventClear(BUTTON_UP);
//	}
//	if(s_ucTriggerFlag)
//	{
//		s_ucTriggerFlag = 0;		
//		vMyCan_Transmit(&stTxMsg_Trigger);
//		s_Txdata[0]++;
//		s_Txdata[1]++;
//		s_Txdata[2]++;
//		s_Txdata[3]++;
//			
//		vOledWriteHexToBuffer(30,36,stTxMsg_Trigger.pucData,stTxMsg_Trigger.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		vOledRefreshFromBuffer(OLED);
//	}
//	//请求发送
//	if(ucBspCan_GetRxFlag() == 1)
//	{
//		CAN_RxHeaderTypeDef stAppHeader;  		 	// 局部接收帧头
//        uint8_t aucAppData[8];            			// 局部接收数据缓冲区
//		
//		/* 通过驱动层接口获取数据副本 */
//        vBspCan_GetRxHeader(&stAppHeader); 			// 拷贝帧头
//        vBspCan_GetRxData(aucAppData);     			// 拷贝数据
//		
//		vBspCan_ClearRxFlag();						// 清除标志
//		
//		if(stAppHeader.IDE == CAN_ID_STD 
//			&& stAppHeader.RTR == CAN_RTR_REMOTE 
//			&& stAppHeader.StdId == 0x777)
//		{/* 如果接收的是标准格式遥控帧 */					// 这里可以自己规定
//			s_ucRequestFlag = 1;							// 自动发送一个数据
//		}
//	}
//	
//	if(s_ucRequestFlag)
//	{
//		s_ucRequestFlag = 0;
//		
//		vMyCan_Transmit(&stTxMsg_Request_DATA);
//		s_Txdata[0]++;
//		s_Txdata[1]++;
//		s_Txdata[2]++;
//		s_Txdata[3]++;
//			
//		vOledWriteHexToBuffer(30,24,stTxMsg_Request.pucData,stTxMsg_Request.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		vOledRefreshFromBuffer(OLED);
//	}
//}

/**
 * @brief 		实现接收
 * @param 	
 * @retval 		
 * @note 		接收设备的代码的数据传输策略的实现-
 */
void vtask(void)
{	
	// 请求部分
	vBtnExecute(); 
	
	// 复用之前的代码，规定BUTTON_UP是发送。
	emButEventTdf emEventUp  		 = emBtnGetCurEvent(BUTTON_UP);
    emButEventTdf emEventDown		 = emBtnGetCurEvent(BUTTON_DOWN);
	emButEventTdf emEventEnable 	 = emBtnGetCurEvent(BUTTON_ENABLE);
	emButEventTdf emEventCancel   	 = emBtnGetCurEvent(BUTTON_CANCEL);
	if (emEventUp == emBtnEvent_Click)
	{
		vMyCan_Transmit(&stTxMsg_Request_REMOTE);
		vBtnEventClear(BUTTON_UP);
	}
	if (emEventDown == emBtnEvent_Click)
	{
		vMyCan_Transmit(&stTxMsg_Request);
		vBtnEventClear(BUTTON_DOWN);
	}
	
	// 接收部分
	vOledWriteStringToBuffer(6,0,(uint8_t*)"Rx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

	uint8_t ucNewRx = ucBspCan_GetRxFlag();				
	static uint8_t s_aucStrBuf[16];
	if(ucNewRx)
	{
//		vOledClearBuffer(OLED);
		
		CAN_RxHeaderTypeDef stAppHeader;  		 	// 局部接收帧头
        uint8_t aucAppData[8];            			// 局部接收数据缓冲区
		
		/* 通过驱动层接口获取数据副本 */
        vBspCan_GetRxHeader(&stAppHeader); 			// 拷贝帧头
        vBspCan_GetRxData(aucAppData);     			// 拷贝数据
		
		vBspCan_ClearRxFlag();						// 清除标志
		
		if(stAppHeader.RTR == CAN_RTR_DATA)
		{
			if(stAppHeader.ExtId == 0x0789AB0F && stAppHeader.IDE == CAN_ID_EXT)
			{	//收到了定时传输的数据
				vOledWriteHexToBuffer(30,12,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
			if(stAppHeader.StdId == 0x555 && stAppHeader.IDE == CAN_ID_STD)
			{	//收到了触发传输的数据
				vOledWriteHexToBuffer(30,36,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
			if(stAppHeader.StdId == 0x666 && stAppHeader.IDE == CAN_ID_STD)
			{	//收到了请求传输的数据
				vOledWriteHexToBuffer(30,24,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
		}
		
	}
	vOledRefreshFromBuffer(OLED);
}

/**
 * @brief  TIM2更新中断回调，100ms进一次
 * @param  htim 定时器句柄
 * @retval 无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        s_ucTimFlag = 1;
    }
}

//void vtask(void)
//{
//	vBtnExecute(); 
//	
//	// 复用之前的代码，规定BUTTON_UP是发送。
//	emButEventTdf emEventUp  		 = emBtnGetCurEvent(BUTTON_UP);
//    emButEventTdf emEventDown		 = emBtnGetCurEvent(BUTTON_DOWN);
//	emButEventTdf emEventEnable 	 = emBtnGetCurEvent(BUTTON_ENABLE);
//	emButEventTdf emEventCancel   	 = emBtnGetCurEvent(BUTTON_CANCEL);
//	if (emEventUp == emBtnEvent_Click) {
//		if(i >= sizeof(stTxMsgArray) / sizeof(stTxMsgArray[0]))
//			i = 0;
//		vMyCan_Transmit(&stTxMsgArray[i++]);
//		s_Txdata[0]++;
//		s_Txdata[1]++;
//		s_Txdata[2]++;
//		s_Txdata[3]++;
//		
//		vBtnEventClear(BUTTON_UP);					// 仅在触发事件后清除，避免提前清除
//		
//	}
//	if (emEventDown == emBtnEvent_Click) {
//		
//		vBtnEventClear(BUTTON_DOWN);
//	}		

//	
//	vOledWriteStringToBuffer(6,0,(uint8_t*)"Tx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//	vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//	vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//	vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

//	// 接收处理部分
//	uint8_t ucNewRx = ucBspCan_GetRxFlag();				
//	static uint8_t s_aucStrBuf[16];
//	if(ucNewRx)
//	{
//		vOledClearBuffer(OLED);
//		
//		CAN_RxHeaderTypeDef stAppHeader;  		 	// 局部接收帧头
//        uint8_t aucAppData[8];            			// 局部接收数据缓冲区
//		
//		/* 通过驱动层接口获取数据副本 */
//        vBspCan_GetRxHeader(&stAppHeader); 			// 拷贝帧头
//        vBspCan_GetRxData(aucAppData);     			// 拷贝数据
//		
//		vBspCan_ClearRxFlag();						// 清除标志
//		if(stAppHeader.IDE == CAN_ID_STD)
//		{
//			s_ulRxId = stAppHeader.StdId;
//			vOledWriteStringToBuffer(24,0,(uint8_t*)"Std",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		}
//		else
//		{
//			s_ulRxId = stAppHeader.ExtId;
//			vOledWriteStringToBuffer(24,0,(uint8_t*)"Exd",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		}
//		if(stAppHeader.RTR == CAN_RTR_DATA)
//		{
//			s_ucRxLength = stAppHeader.DLC;
//			vOledWriteStringToBuffer(48,0,(uint8_t*)"Data",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//			
//			/* 显示 Length */
//			snprintf((char *)s_aucStrBuf, sizeof(s_aucStrBuf), "%d", s_ucRxLength);
//			vOledWriteStringToBuffer(36, 24, s_aucStrBuf, emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);
//			vOledWriteHexToBuffer(36, 36, aucAppData, s_ucRxLength, emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);
//			
//		}
//		else
//		{
//			vOledWriteStringToBuffer(6,24,(uint8_t*)"Len: 0",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//			vOledWriteStringToBuffer(48,0,(uint8_t*)"Remote",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
//		}
//		/* 显示 RxID */
//		snprintf((char *)s_aucStrBuf, sizeof(s_aucStrBuf), "0x%03lX", (unsigned long)s_ulRxId);
//		vOledWriteStringToBuffer(36, 12, s_aucStrBuf, emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);

//	}
//	vOledRefreshFromBuffer(OLED);
//}
