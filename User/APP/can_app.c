#include "can_app.h"
#include "can_device.h"
#include "can_test.h"
#include "tim_device.h"
#include "button_mid.h"
#include "oled_device.h"
#include "project.h"             // 为了 CAN_TEST_PROFILE 等宏
#include "stm32f1xx_hal.h"
#include <stdio.h>


// 业务层发送缓存（与测试层隔离开）
static uint8_t s_aucAppTxData[8] = {0x55, 0x66, 0x48, 0x77};
static uint8_t s_aucAppTxData_1[8] = {0x00};

// 触发发送标志位
static uint8_t s_ucTriggerFlag;
// 请求发送标志位
static uint8_t s_ucRequestFlag;

// 定时发送帧数据
static stCanTxParamTdf stTxMsg_Timing =
{
	.ulID    = 0x0789AB0F,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData,
	.ulIDE   = CAN_ID_EXT,
	.ulRTR   = CAN_RTR_DATA,
};

// 触发发送帧数据
static stCanTxParamTdf stTxMsg_Trigger =
{
	.ulID    = 0x555,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};

// 请求发送帧数据
static stCanTxParamTdf stTxMsg_Request =
{
	.ulID    = 0x777,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_REMOTE,
};

// 请求发送帧数据（遥控）
static stCanTxParamTdf stTxMsg_Request_REMOTE =
{
	.ulID    = 0x789,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData_1,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_REMOTE,
};

// 请求发送帧数据（数据）
static stCanTxParamTdf stTxMsg_Request_DATA =
{
	.ulID    = 0x666,
	.ucDLC   = 4,
	.pucData = s_aucAppTxData,
	.ulIDE   = CAN_ID_STD,
	.ulRTR   = CAN_RTR_DATA,
};

/**
 * @brief   CAN 应用层初始化
 * @note    根据 project.h 中的宏配置，计算滤波参数并调用底层初始化
 */
void vCanAppInit(void)
{
    uint32_t ulIdVal   = 0;
    uint32_t ulMaskVal = 0;
    uint32_t ulMode    = CAN_FILTERMODE_IDMASK; // 默认掩码模式
    uint32_t ulScale   = CAN_FILTERSCALE_32BIT; // 默认32位

#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
    ulIdVal   = 0;
    ulMaskVal = 0;

#elif (CAN_TEST_PROFILE == CAN_TEST_32BIT_MASK)
    ulIdVal   = (0x12345600u << 3) | 0x4;
    ulMaskVal = (0x1FFFFF00u << 3) | 0x4 | 0x2;

#elif (CAN_TEST_PROFILE == CAN_TEST_32BIT_LIST)
    ulIdVal   = 0x123u << 21;
    ulMaskVal = (0x12345678u << 3) | 0x4;
    ulMode    = CAN_FILTERMODE_IDLIST;

#elif (CAN_TEST_PROFILE == CAN_TEST_16BIT_MASK)
    ulIdVal   = 0x200u << 5; 
    ulMaskVal = (0x700u << 5) | 0x10 | 0x08;
    ulScale   = CAN_FILTERSCALE_16BIT;

#else /* CAN_TEST_PROFILE == CAN_TEST_16BIT_LIST */
    ulIdVal   = 0x234u << 5;
    ulMaskVal = 0x345u << 5; 
    ulMode    = CAN_FILTERMODE_IDLIST;
    ulScale   = CAN_FILTERSCALE_16BIT;
#endif

    // 调用驱动层，执行硬件初始化
    vCanDeviceInit(ulIdVal, ulMaskVal, ulMode, ulScale);
}


/**
 * @brief   发送板业务任务
 */
void vCanAppSenderTask(void)
{
#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
	// 1. 定时发送
	if(ucTimDeviceGetFlag())
	{
		vTimDeviceClearFlag();
		
		vOledWriteStringToBuffer(6,0,(uint8_t*)"Tx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		vMyCan_Transmit(&stTxMsg_Timing);
		s_aucAppTxData[0]++; // 注意这里改成了 s_aucAppTxData
		s_aucAppTxData[1]++;
		s_aucAppTxData[2]++;
		s_aucAppTxData[3]++;
			
		vOledWriteHexToBuffer(30,12,stTxMsg_Timing.pucData,stTxMsg_Timing.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledRefreshFromBuffer(OLED);
	}
	
	// 2. 按键触发发送
	emButEventTdf emEventUp = emBtnGetCurEvent(BUTTON_UP);
	if (emEventUp == emBtnEvent_Click)
	{
		s_ucTriggerFlag = 1;
		vBtnEventClear(BUTTON_UP);
	}
	if(s_ucTriggerFlag)
	{
		s_ucTriggerFlag = 0;		
		vMyCan_Transmit(&stTxMsg_Trigger);
		s_aucAppTxData[0]++;
		s_aucAppTxData[1]++;
		s_aucAppTxData[2]++;
		s_aucAppTxData[3]++;
			
		vOledWriteHexToBuffer(30,36,stTxMsg_Trigger.pucData,stTxMsg_Trigger.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledRefreshFromBuffer(OLED);
	}
	
	// 3. 请求接收处理
	if(ucBspCan_GetRxFlag() == 1)
	{
		CAN_RxHeaderTypeDef stAppHeader;
        uint8_t aucAppData[8];
		
        vBspCan_GetRxHeader(&stAppHeader);
        vBspCan_GetRxData(aucAppData);
		vBspCan_ClearRxFlag();
		
		if(stAppHeader.IDE == CAN_ID_STD 
			&& stAppHeader.RTR == CAN_RTR_REMOTE 
			&& stAppHeader.StdId == 0x777)
		{
			s_ucRequestFlag = 1;
		}
	}
	
	if(s_ucRequestFlag)
	{
		s_ucRequestFlag = 0;
		
		vMyCan_Transmit(&stTxMsg_Request_DATA);
		s_aucAppTxData[0]++;
		s_aucAppTxData[1]++;
		s_aucAppTxData[2]++;
		s_aucAppTxData[3]++;
			
		vOledWriteHexToBuffer(30,24,stTxMsg_Request.pucData,stTxMsg_Request.ucDLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
		vOledRefreshFromBuffer(OLED);
	}

#else
	// 发送板 + 滤波档：按一下按钮发一条，调用测试层接口
	static uint32_t s_ulSent = 0;
	char acLine[24];

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
#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
	// 1. 按键请求部分
	emButEventTdf emEventUp   = emBtnGetCurEvent(BUTTON_UP);
    emButEventTdf emEventDown = emBtnGetCurEvent(BUTTON_DOWN);
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
	
	// 2. 接收显示部分
	vOledWriteStringToBuffer(6,0,(uint8_t*)"Rx:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,12,(uint8_t*)"Tim:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,36,(uint8_t*)"Tri:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	vOledWriteStringToBuffer(6,24,(uint8_t*)"Req:",emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

	if(ucBspCan_GetRxFlag())
	{
		CAN_RxHeaderTypeDef stAppHeader;
        uint8_t aucAppData[8];
		
        vBspCan_GetRxHeader(&stAppHeader);
        vBspCan_GetRxData(aucAppData);
		vBspCan_ClearRxFlag();
		
		if(stAppHeader.RTR == CAN_RTR_DATA)
		{
			if(stAppHeader.ExtId == 0x0789AB0F && stAppHeader.IDE == CAN_ID_EXT)
			{
				vOledWriteHexToBuffer(30,12,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
			if(stAppHeader.StdId == 0x555 && stAppHeader.IDE == CAN_ID_STD)
			{
				vOledWriteHexToBuffer(30,36,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
			if(stAppHeader.StdId == 0x666 && stAppHeader.IDE == CAN_ID_STD)
			{
				vOledWriteHexToBuffer(30,24,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
			}
		}
	}
	vOledRefreshFromBuffer(OLED);

#else
	// 接收板 + 滤波档
	static uint32_t s_ulRxCnt = 0;
	char acLine[24];

	if (ucBspCan_GetRxFlag() == 1)
	{
		CAN_RxHeaderTypeDef stAppHeader;
		uint8_t aucAppData[8];
		const char *pcType;
		uint32_t ulId;

		vBspCan_GetRxHeader(&stAppHeader);
		vBspCan_GetRxData(aucAppData);
		vBspCan_ClearRxFlag();

		s_ulRxCnt++;

		if (stAppHeader.IDE == CAN_ID_EXT)
		{
			pcType = (stAppHeader.RTR == CAN_RTR_REMOTE) ? "Ext-R" : "Ext-D";
			ulId   = stAppHeader.ExtId;
		}
		else
		{
			pcType = (stAppHeader.RTR == CAN_RTR_REMOTE) ? "Std-R" : "Std-D";
			ulId   = stAppHeader.StdId;
		}

		snprintf(acLine, sizeof(acLine), "%s 0x%08lX", pcType, (unsigned long)ulId);
		vOledWriteStringToBuffer(0,0,(uint8_t*)acLine,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		vOledWriteHexToBuffer(0,12,aucAppData,stAppHeader.DLC,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);

		snprintf(acLine, sizeof(acLine), "DLC:%lu Cnt:%lu",
					(unsigned long)stAppHeader.DLC, (unsigned long)s_ulRxCnt);
		vOledWriteStringToBuffer(0,24,(uint8_t*)acLine,emOledFontSize_6x12,emOledPixelShowMode_Positive,OLED);
	}

	vOledRefreshFromBuffer(OLED);
#endif
}
