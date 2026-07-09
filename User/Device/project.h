/**
 * @file 	project.h
 * @author	
 * @version 
 * @data 	20260507
 * @brief 	工程配置文件
 */
 
#ifndef __PROJECT_H_
#define __PROJECT_H_

//行：Row		薄膜按键，还需要优化

//列：Column

#define KEYPAD_Row01				GPIO_PIN_0		//行1，默认为GPIOA
#define KEYPAD_Row02				GPIO_PIN_1
#define KEYPAD_Row03				GPIO_PIN_2
#define KEYPAD_Row04				GPIO_PIN_3
										
#define KEYPAD_Col01				GPIO_PIN_4		//列1，默认为GPIOA
#define KEYPAD_Col02				GPIO_PIN_5
#define KEYPAD_Col03				GPIO_PIN_6
#define KEYPAD_Col04				GPIO_PIN_7
			
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


#define RX_BUF_SIZE 				50								//串口接收缓存大小
#define RING_BUFFER_DEV_NUM 		1								//环形缓冲区设备数
#define UART_RX_BUFFER 				emRingBufNum0					//环形缓冲区设备号
			
#define INTERPRETER_CMD_MAX_NUM     20								/* 全局命令表最大容量（根据项目需要调整） */
			
#define SNAKE_MAX_LEN     			48								/* 贪吃蛇最大缓存长度 */
//#define SNAKE0     					emSnakeNum0					/* 贪吃蛇设备号 */
#define SNAKE_MOVE_SPEED     		150								/* 贪吃蛇移动速度（ms） */


// ===================== 页面枚举 =====================
typedef enum
{
    emMenu = 0,                										// 主菜单
    emMenu_Calculator,         										// 计算器页面
    emMenu_LED,                										// LED控制页面
    emMenu_SnakeGame,          										// 贪吃蛇页面
    emMenu_SerialTest,         										// 串口测试页面
} emAllMenuTdf;
/**
 * @brief 		贪吃蛇游戏状态枚举
 * @note 		根据当前状态实现对应按键功能，以及显示文本、按钮
 */
typedef enum
{
    emSnakeGameStu_Idle    	 		= 0,							//游戏开始前
    emSnakeGameStu_Running    	 	= 1,							//游戏开始后
	emSnakeGameStu_Win    	 		= 2,							//游戏胜利
	emSnakeGameStu_Fail    		 	= 3,							//游戏失败
	emBackMenu	    	 			= 4,							//回到主菜单状态
}
emSnakeGameStuTdf;

#endif
