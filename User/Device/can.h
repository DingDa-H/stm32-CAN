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



void vMyCan_Transmit(uint32_t ID,uint8_t Length,uint8_t *Data,uint32_t IDE,uint32_t RTR);
uint8_t ucMyCan_ReceiveFlag(void);
void vMyCan_Receive(CAN_RxHeaderTypeDef *pHeader, uint8_t *RxData);
#endif
