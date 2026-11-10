#include "can_test.h"
#include "can_device.h"
#include "project.h"             // 为了 CAN_TEST_PROFILE 等宏
#include "stm32f1xx_hal.h"       // 为了 CAN_ID_EXT、CAN_RTR_DATA 等宏
#include <stdint.h>

// 测试专用发送缓存
static uint8_t s_aucTestData[8] = {0x55, 0x66, 0x48, 0x77};

// 发送帧表：按 project.h 里的 CAN_TEST_PROFILE 选一张表
#if (CAN_TEST_PROFILE == CAN_TEST_ALL_PASS)
	stCanTxParamTdf stTxMsgArray[] =
	{
		{.ulID    = 0x12345600,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x12345601,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x12345603,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x123456FF,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},

		{.ulID    = 0x0789AB00,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x0789AB01,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x0789AB03,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x0789AB0F,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	};

#elif (CAN_TEST_PROFILE == CAN_TEST_32BIT_MASK)
	stCanTxParamTdf stTxMsgArray[] =
	{
		{.ulID    = 0x12345600,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x12345601,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x12345603,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x123456FF,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},

		{.ulID    = 0x0789AB00,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x0789AB01,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x0789AB03,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x0789AB0F,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_EXT,.ulRTR   = CAN_RTR_DATA,},
	};

#elif (CAN_TEST_PROFILE == CAN_TEST_32BIT_LIST)
	stCanTxParamTdf stTxMsgArray[] =
	{
		{.ulID    = 0x123,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x234,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x345,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x456,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x12345678,.ucDLC = 4,.pucData = s_aucTestData,.ulIDE = CAN_ID_EXT,.ulRTR = CAN_RTR_DATA,},
		{.ulID    = 0x0789ABCD,.ucDLC = 4,.pucData = s_aucTestData,.ulIDE = CAN_ID_EXT,.ulRTR = CAN_RTR_DATA,},
	};

#elif (CAN_TEST_PROFILE == CAN_TEST_16BIT_MASK)
	stCanTxParamTdf stTxMsgArray[] =
	{
		{.ulID    = 0x100,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x101,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_REMOTE,},
		{.ulID    = 0x1FE,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x1FF,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},

		{.ulID    = 0x200,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x201,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_REMOTE,},
		{.ulID    = 0x2FE,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x2FF,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},

		{.ulID    = 0x300,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x301,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_REMOTE,},
		{.ulID    = 0x3FE,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x3FF,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},

		{.ulID    = 0x320,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x321,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_REMOTE,},
		{.ulID    = 0x32E,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x32F,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
	};

#else /* CAN_TEST_PROFILE == CAN_TEST_16BIT_LIST */
	stCanTxParamTdf stTxMsgArray[] =
	{
		{.ulID    = 0x123,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x234,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_REMOTE,},
		{.ulID    = 0x345,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x456,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x567,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
		{.ulID    = 0x678,.ucDLC   = 4,.pucData = s_aucTestData,.ulIDE   = CAN_ID_STD,.ulRTR   = CAN_RTR_DATA,},
	};
#endif

#define CAN_TXMSG_NUM 				(sizeof(stTxMsgArray) / sizeof(stTxMsgArray[0]))

void vCanTestSendOneByIndex(uint8_t ucIndex)
{
    if (ucIndex >= CAN_TXMSG_NUM) return;
    
    vMyCan_Transmit(&stTxMsgArray[ucIndex]);
    s_aucTestData[0]++; // 数据首字节自增，用于接收端检测丢帧
}

uint8_t ucCanTestGetFrameNum(void)
{
    return CAN_TXMSG_NUM;
}