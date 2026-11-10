#include "stm32f1xx_hal.h"
#include <can_device.h>
#include "string.h"
#include <stdint.h>
#include <oled_device.h>
#include "button_mid.h"
#include "task.h"
#include <stdio.h>
#include "can_app.h"


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

void vtask(void)
{
    vBtnExecute(); // 统一处理物理按键状态机

#if (CAN_NODE_ROLE == CAN_NODE_SENDER)
    vCanAppSenderTask();
#else
    vCanAppReceiverTask();
#endif
}
