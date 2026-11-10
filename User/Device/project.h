/**
 * @file 	project.h
 * @author	
 * @version 
 * @data 	20260507
 * @brief 	工程配置文件
 */
 
#ifndef __PROJECT_H_
#define __PROJECT_H_


			
#define STM32_IC_HAL				"stm32f1xx_hal.h"
		
#define BUTTON_NUM 					4								//按钮设备数量
#define BUTTON_UP 					emBtnDevNum0					//按钮设备对应功能
#define BUTTON_DOWN 				emBtnDevNum1					
#define BUTTON_ENABLE 				emBtnDevNum2					
#define BUTTON_CANCEL 				emBtnDevNum3					

#define LED_NUM 					4								//led设备数量

#define LED0						emLedDevNum0
#define LED1						emLedDevNum1
#define LED2						emLedDevNum2
#define LED3						emLedDevNum3

#define EM_FONT_SIZE 				emOledFontSize_8x16				//字号大小

#define OLED_DEV_NUM				1								// OLED设备数量
#define OLED                        emOledDevNum0					// OLED设备号
#define OLED_POINT_WIDTH			128								//像素宽度
#define OLED_POINT_HIGH				64								//像素高度
#define OLED_BUFFER_WIDTH			OLED_POINT_WIDTH				//OLED一帧数据的行宽度，等于屏幕一行的像素数量
#define OLED_BUFFER_HIGH			OLED_POINT_HIGH/8				//OLED一帧数据的列高度，等于屏幕一列的像素数量/8

#define MENU_VISIBLE_LINES      	4U								// 菜单一屏可见行数，和 menu.c 滚动逻辑对应

/* ============ CAN 测试配置 ============ */
/* 一份测试配置 = 一组配套参数：过滤器配置 + 对应的发送帧表 + 预期结果，切换配置即整套更换 */
#define CAN_TEST_ALL_PASS      		0U
#define CAN_TEST_16BIT_LIST    		1U
#define CAN_TEST_16BIT_MASK    		2U
#define CAN_TEST_32BIT_LIST    		3U
#define CAN_TEST_32BIT_MASK    		4U

#define CAN_TEST_PROFILE       		CAN_TEST_ALL_PASS   			/* 当前测试配置 */

/* ============ 节点类型配置 ============ */
/* 三节点联调（1 发 2 收菊花链，同一工程分别编译三份固件）：                       */
/* 发送节点（SENDER）  ：按当前测试配置的帧表循环发包（帧表内故意含应被过滤的帧）；  */
/* 接收节点（RECEIVER）：启用当前测试配置的硬件过滤器，显示所有通过的帧（帧类型解码） */
/* 滤波对照实验：接收节点 B 用目标滤波配置、接收节点 C 用 ALL_PASS，同刻对比两端收到的报文 */
#define CAN_NODE_RECEIVER      		0U								// 接收节点（RECEIVER）
#define CAN_NODE_SENDER        		1U								// 发送节点（SENDER）

#define CAN_NODE_ROLE          		CAN_NODE_RECEIVER   			/* 当前节点类型 */



#endif
