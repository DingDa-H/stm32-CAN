/**
 * @brief 		can应用层
 * @version 	DingDa-H	
 * @data 		20261109	
 * @note 		优化分层架构，使其尽量符合AUTOSAR思想
 * @note 		专门处理can对应的业务
 */
 
#ifndef __CAN_APP_H
#define __CAN_APP_H

#include "project.h"

/**
 * @brief   CAN 应用层初始化
 * @note    负责计算滤波参数并调用驱动层进行硬件初始化
 */
void vCanAppInit(void);

/**
 * @brief   发送板业务任务
 * @note    处理定时发送、触发发送、请求应答等逻辑
 */
void vCanAppSenderTask(void);

/**
 * @brief   接收板业务任务
 * @note    处理接收报文解析、分发到 OLED 显示的逻辑
 */
void vCanAppReceiverTask(void);

#endif
