#include "can_app.h"
#include "can_device.h"
#include "can_test.h"
#include "tim_device.h"
#include "button_mid.h"
#include "oled_device.h"
#include "project.h"             							// 为了 CAN_TEST_PROFILE 等宏
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include "can_timeout_mid.h"
// 业务层发送缓存（与测试层隔离开）：三类发送帧各用独立缓冲，Tim/Tri/Req 各自递增、互不影响
static uint8_t s_aucAppTxData_Timing[8]  = {0x11, 0x22, 0x33, 0x44};
static uint8_t s_aucAppTxData_Trigger[8] = {0x11, 0x22, 0x33, 0x44};
static uint8_t s_aucAppTxData_Request[8] = {0x11, 0x22, 0x33, 0x44};

// 触发发送标志位
static uint8_t s_ucTriggerFlag;
// 请求发送标志位
static uint8_t s_ucRequestFlag;

// 定时发送帧数据
static stCanTxParamTdf stTxMsg_Timing =
{
	.ulID    = CAN_ID_TIMING_MSG,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData_Timing,
	.ulIDE   = CAN_ID_EXT,
	.ulRTR   = CAN_RTR_DATA,
};

// 触发发送帧数据
static stCanTxParamTdf stTxMsg_Trigger =
{
	.ulID    = 0x555,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData_Trigger,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};

// 请求数据帧（请求方自身为 0x777 数据帧，仅作触发信号，对齐江协原版 0x3FF 的做法）
static stCanTxParamTdf stTxMsg_Request =
{
	.ulID    = 0x777,
	.ucDLC   = 0,
	.pucData = s_aucAppTxData_Request,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};

// 请求遥控帧（0x789 标准遥控帧，对齐江协原版 0x300 遥控帧请求的做法，无数据场）
static stCanTxParamTdf stTxMsg_Request_REMOTE =
{
	.ulID    = 0x789,
	.ucDLC   = 0,
	.pucData = s_aucAppTxData_Request,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_REMOTE,
};

// 请求发送帧数据（数据）
static stCanTxParamTdf stTxMsg_Request_DATA =
{
	.ulID    = 0x666,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData_Request,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};

/**
 * @brief   CAN 应用层初始化
 * @note    根据 project.h 中的宏配置，计算滤波参数并调用底层初始化
 */
// can_app.c 中修改 vCanAppInit 函数
// can_app.c
void vCanAppInit(void)
{
    CAN_FilterTypeDef stFilter = {0};
    
    // 通用配置
    stFilter.FilterBank = 0;
    stFilter.FilterActivation = ENABLE;
    stFilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;

#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
    stFilter.FilterIdHigh = 0;
    stFilter.FilterIdLow = 0;
    stFilter.FilterMaskIdHigh = 0;
    stFilter.FilterMaskIdLow = 0;
    stFilter.FilterMode = CAN_FILTERMODE_IDMASK;
    stFilter.FilterScale = CAN_FILTERSCALE_32BIT;

#elif (CAN_TEST_PROFILE == CAN_TEST_16BIT_MASK)
    stFilter.FilterIdHigh = 0x200 << 5;
    stFilter.FilterIdLow = 0x320 << 5;
    stFilter.FilterMaskIdHigh = (0x700 << 5) | 0x10 | 0x08;
    stFilter.FilterMaskIdLow = (0x7F0 << 5) | 0x10 | 0x08;
    stFilter.FilterMode = CAN_FILTERMODE_IDMASK;
    stFilter.FilterScale = CAN_FILTERSCALE_16BIT;

#elif (CAN_TEST_PROFILE == CAN_TEST_16BIT_LIST)
    stFilter.FilterIdHigh = 0x234 << 5;
    stFilter.FilterIdLow = 0x345 << 5;
    stFilter.FilterMaskIdHigh = 0x567 << 5;
    stFilter.FilterMaskIdLow = 0x000 << 5;
    stFilter.FilterMode = CAN_FILTERMODE_IDLIST;
    stFilter.FilterScale = CAN_FILTERSCALE_16BIT;

#elif (CAN_TEST_PROFILE == CAN_TEST_32BIT_MASK)
    // 在常量后加 u，并先把常量强转为 uint32_t，再做移位
    uint32_t ulId = ((uint32_t)0x12345600u << 3) | 0x4;
    
    // 加上 0x4 掩码位，严格限制“必须是扩展帧” + “必须是数据帧”
    uint32_t ulMask = ((uint32_t)0x1FFFFF00u << 3) | 0x4 | 0x2; 
    
    stFilter.FilterIdHigh = ulId >> 16;
    stFilter.FilterIdLow  = ulId & 0xFFFF;
    stFilter.FilterMaskIdHigh = ulMask >> 16;
    stFilter.FilterMaskIdLow  = ulMask & 0xFFFF;
    stFilter.FilterMode = CAN_FILTERMODE_IDMASK;
    stFilter.FilterScale = CAN_FILTERSCALE_32BIT;

#else /* CAN_TEST_PROFILE == CAN_TEST_32BIT_LIST */
    // 这里同样要防止溢出警告
    uint32_t ulId1 = ((uint32_t)0x123u << 21);
    uint32_t ulId2 = ((uint32_t)0x12345678u << 3) | 0x4;
    
    stFilter.FilterIdHigh = ulId1 >> 16;
    stFilter.FilterIdLow  = ulId1 & 0xFFFF;
    stFilter.FilterMaskIdHigh = ulId2 >> 16;
    stFilter.FilterMaskIdLow  = ulId2 & 0xFFFF;
    stFilter.FilterMode = CAN_FILTERMODE_IDLIST;
    stFilter.FilterScale = CAN_FILTERSCALE_32BIT;
#endif
	//初始化需要监控的报文列表
	vCanTimeoutInit();
    // 调用底层初始化，把算好的结构体传过去
    vCanDeviceInit(&stFilter);
}

/**
 * @brief   发送板业务任务
 */
void vCanAppSenderTask(void)
{
	// 用于接收队列的数据
	stCanRxFrameTdf stFrame; 	
#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
	// 1. 定时发送
	if(ucTimDeviceGetFlag())
	{
		vTimDeviceClearFlag();
		
		vOledWriteStringToBuffer(6,0,(uint8_t*)"Tx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		s_aucAppTxData_Timing[0]++;
		s_aucAppTxData_Timing[1]++;
		s_aucAppTxData_Timing[2]++;
		s_aucAppTxData_Timing[3]++;
		if (ucDeviceCan_Transmit(&stTxMsg_Timing) == 0U)
		{
			vOledWriteHexToBuffer(30,12,stTxMsg_Timing.pucData,stTxMsg_Timing.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		}
		vOledRefreshFromBuffer(OLED);
	}
	
	// 2. 按键触发发送
	emButEventTdf emEventUp = emBtnGetCurEvent(BUTTON_UP);
	if (emEventUp == emBtnEvent_Click)
	{
		/* 数据只在事件到达时自增一次，失败重发的是同一帧 */
		s_aucAppTxData_Trigger[0]++;
		s_aucAppTxData_Trigger[1]++;
		s_aucAppTxData_Trigger[2]++;
		s_aucAppTxData_Trigger[3]++;
		s_ucTriggerFlag = 2U;			/* 2 = 数据已备好，待发送 */
		vBtnEventClear(BUTTON_UP);
	}
	if(s_ucTriggerFlag == 2U)
	{
		if (ucDeviceCan_Transmit(&stTxMsg_Trigger) == 0U)
		{
			s_ucTriggerFlag = 0U;
			vOledWriteHexToBuffer(30,36,stTxMsg_Trigger.pucData,stTxMsg_Trigger.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			vOledRefreshFromBuffer(OLED);
		}
	}
	
	// 3. 请求接收处理
	while(ucDeviceCan_ReadRxFrame(&stFrame) == 1U)		// == 1表示队列里面有数据
	{
		/* 请求遥控帧：收到 0x789 标准遥控帧 */
		if(stFrame.stHeader.IDE == CAN_ID_STD && stFrame.stHeader.RTR == CAN_RTR_REMOTE && stFrame.stHeader.StdId == 0x789)
		{
			s_ucRequestFlag = 1;
		}
		/* 请求数据帧：收到 0x777 标准数据帧（仅作触发，不看数据） */
		if(stFrame.stHeader.IDE == CAN_ID_STD && stFrame.stHeader.RTR == CAN_RTR_DATA && stFrame.stHeader.StdId == 0x777)
		{
			s_ucRequestFlag = 1;
		}
	}
	if (s_ucRequestFlag == 1U)			/* 1 = 收到请求，数据尚未准备 */
	{
		s_aucAppTxData_Request[0]++;
		s_aucAppTxData_Request[1]++;
		s_aucAppTxData_Request[2]++;
		s_aucAppTxData_Request[3]++;
		s_ucRequestFlag = 2U;			/* 2 = 数据已备好，待发送 */
	}
	if (s_ucRequestFlag == 2U)
	{
		if (ucDeviceCan_Transmit(&stTxMsg_Request_DATA) == 0U)
		{
			s_ucRequestFlag = 0U;		/* 发送成功才清标志 */
			vOledWriteHexToBuffer(30,24,stTxMsg_Request_DATA.pucData,stTxMsg_Request_DATA.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			vOledRefreshFromBuffer(OLED);
		}
	}

#else
	// 发送板 + 滤波档：按一下按钮发一条，调用测试层接口
	static uint32_t s_ulSent = 0;
	char acLine[24];

	/* 滤波档发送板不处理接收业务：清空队列，避免溢出计数持续增长 */
	while (ucDeviceCan_ReadRxFrame(&stFrame) == 1U)
	{
	}

	if (emBtnGetCurEvent(BUTTON_UP) == emBtnEvent_Click)
	{
		vBtnEventClear(BUTTON_UP);

		static uint8_t s_ucTestIdx = 0;
		vCanTestSendOneByIndex(s_ucTestIdx);
		s_ulSent++;

		snprintf(acLine, sizeof(acLine), "Tx %u/%u N:%lu",
					(unsigned)(s_ucTestIdx + 1U), (unsigned)ucCanTestGetFrameNum(), (unsigned long)s_ulSent);
		vOledWriteStringToBuffer(0,0,(uint8_t*)acLine,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		vOledRefreshFromBuffer(OLED);

		s_ucTestIdx++;
		if (s_ucTestIdx >= ucCanTestGetFrameNum())
		{
			s_ucTestIdx = 0;
		}
	}
#endif
}
/**
 * @brief   接收板业务任务
 */
void vCanAppReceiverTask(void)
{
	stCanRxFrameTdf stFrame;
#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
	// 1. 按键请求部分
	emButEventTdf emEventUp   = emBtnGetCurEvent(BUTTON_UP);
    emButEventTdf emEventDown = emBtnGetCurEvent(BUTTON_DOWN);
	if (emEventUp == emBtnEvent_Click)
	{
		if (ucDeviceCan_Transmit(&stTxMsg_Request_REMOTE) == 0U) {
			vBtnEventClear(BUTTON_UP);
		}
	}
	if (emEventDown == emBtnEvent_Click)
	{
		if (ucDeviceCan_Transmit(&stTxMsg_Request) == 0U) {
			vBtnEventClear(BUTTON_DOWN);
		}
	}
	
	// 2. 接收显示部分
	vOledWriteStringToBuffer(6,0,(uint8_t*)"Rx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

	while (ucDeviceCan_ReadRxFrame(&stFrame) == 1U)
	{
		if(stFrame.stHeader.RTR == CAN_RTR_DATA)
		{
			if(stFrame.stHeader.IDE == CAN_ID_EXT && stFrame.stHeader.ExtId == CAN_ID_TIMING_MSG)
			{
				vCanTimeoutUpdate(CAN_ID_TIMING_MSG);
				vOledWriteHexToBuffer(30,12,stFrame.aucData,stFrame.stHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
			if(stFrame.stHeader.StdId == 0x555 && stFrame.stHeader.IDE == CAN_ID_STD)
			{
				vOledWriteHexToBuffer(30,36,stFrame.aucData,stFrame.stHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
			if(stFrame.stHeader.StdId == 0x666 && stFrame.stHeader.IDE == CAN_ID_STD)
			{
				vOledWriteHexToBuffer(30,24,stFrame.aucData,stFrame.stHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
		}
	}
	if (ucCanTimeoutGetFlag(CAN_ID_TIMING_MSG) == 1U)
	{
		vOledWriteStringToBuffer(6, 48, (uint8_t*)"TIMEOUT!", emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);
	}
	else
	{
		vOledWriteStringToBuffer(6, 48, (uint8_t*)"        ", emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);   /* 清除残留 */
	}
	vOledRefreshFromBuffer(OLED);

#else
	// 接收板 + 滤波档
	static uint32_t s_ulRxCnt = 0;
	char acLine[24];

	// 【修改】统一用 while 循环读取队列
	while (ucDeviceCan_ReadRxFrame(&stFrame) == 1U)
	{
		const char *pcType;
		uint32_t ulId;

		s_ulRxCnt++;

		if (stFrame.stHeader.IDE == CAN_ID_EXT)
		{
			pcType = (stFrame.stHeader.RTR == CAN_RTR_REMOTE) ? "Ext-R" : "Ext-D";
			ulId   = stFrame.stHeader.ExtId;
		}
		else
		{
			pcType = (stFrame.stHeader.RTR == CAN_RTR_REMOTE) ? "Std-R" : "Std-D";
			ulId   = stFrame.stHeader.StdId;
		}

		snprintf(acLine, sizeof(acLine), "%s 0x%08lX", pcType, (unsigned long)ulId);
		vOledWriteStringToBuffer(0,0,(uint8_t*)acLine,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		// 【修改】用 stFrame.aucData
		vOledWriteHexToBuffer(0,12,stFrame.aucData,stFrame.stHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		snprintf(acLine, sizeof(acLine), "DLC:%lu Cnt:%lu",
					(unsigned long)stFrame.stHeader.DLC, (unsigned long)s_ulRxCnt);
		vOledWriteStringToBuffer(0,24,(uint8_t*)acLine,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	}

	vOledRefreshFromBuffer(OLED);
#endif
}
