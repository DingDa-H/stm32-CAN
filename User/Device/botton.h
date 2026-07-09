#ifndef __BOTTON_H
#define __BOTTON_H


#include "project.h"
#include STM32_IC_HAL

/**
 * @brief 		按钮设备数量枚举
 * @param 	
 * @data 			
 * @note 		
 */
typedef enum
{
    emBtnDevNum0    	 = 0,
    emBtnDevNum1,
	emBtnDevNum2,
	emBtnDevNum3,
}
emButDevNumTdf;


/**
 * @brief 		按钮按下时电平枚举
 * @param 	
 * @data 			
 * @note 		按键按下时的电平状态
 */
typedef enum
{
    emBtnActiveLevel_Low                               = 0,                // 0  按下时低电平
    emBtnActiveLevel_High                              = 1,                // 1  按下时高电平
}
emButLevelTdf;

/// @brief      按键状态枚举定义
///
/// @note
typedef enum
{
    emBtnStatus_Released                               = 0,                            // 0        释放
    emBtnStatus_Pressed                                = 1,                            // 1        按下
//    emBtnStatus_NotModify                              = 0xFFFF,                       // 0xFFFF   不做更改
}
emBtnStatusTdf;

/// @brief      按键事件枚举
///
/// @note       
typedef enum
{
    emBtnEvent_None                                    = 0,                            // 0        无事件
    emBtnEvent_Click                                   = 1,                            // 1        单击事件
    emBtnEvent_LongPress                               = 2,                            // 2        长按事件
    emBtnEvent_DoubleClick                             = 3,                            // 3        双击事件
}
emButEventTdf;


/// @brief      设备运行参数结构定义
///
/// @note       
typedef struct
{
    emBtnStatusTdf                                     emCurrentStatus;               // 当前状态
    emBtnStatusTdf                                     emLastStatus;                  // 上一次状态
	emButEventTdf									   emCurrentEvent;				  // 当前事件
	uint32_t    									   ulDebounceTick;				  // 消抖计时
	uint32_t    									   ulPressTick;				 	  // 长按计时
	uint32_t       									   ulClickTick;					  // 单击计时（双击用）
	emBtnStatusTdf									   emlast_raw;  				  // 新增，用于消抖的上一原始电平
	uint8_t											   ucDoubleWait;				  // 用于置等待双击的标志
}
stBtnRunningParamTdf;


/// @brief      设备静态参数结构定义
///
/// @note       通常情况下也可以理解为硬件参数
//typedef struct
//{
//    GPIO_TypeDef                *pstGpioBase;                  // 使用的 GPIOx
//    uint16_t                    usGpioPin;                     // 使用的 PINx
//    emButLevelTdf		        emButLevel;           	       // 按钮按下时电平
//	
//    uint32_t                    ulDebounceCountThreshold;      // 消抖计数阈值-20
//    uint32_t                    ulLongPressCountThreshold;     // 长按计数阈值-1000
//    uint32_t                    ulDoubleClickInterval;         // 双击间隔计数阈值-200
//}
//stBtnStaticParamTdf;
/// @brief      设备静态参数结构定义
/// @note       通常情况下也可以理解为硬件参数
typedef struct
{
    GPIO_TypeDef                *pstGpioBase;                  // 使用的 GPIOx
    uint16_t                    usGpioPin;                     // 使用的 PINx
    emButLevelTdf		        emButLevel;           	       // 按钮按下时电平
	
    uint32_t                    ulDebounceMs;      // 原ulDebounceCountThreshold，改为与初始化代码一致的命名
    uint32_t                    ulLongPressMs;     // 原ulLongPressCountThreshold，改为与初始化代码一致的命名
    uint32_t                    ulDoubleClickMs;   // 原ulDoubleClickInterval，改为与初始化代码一致的命名
}
stBtnStaticParamTdf;
/// @brief      按键设备参数结构定义
///
/// @note       含静态参数和动态参数
typedef struct
{
    stBtnStaticParamTdf         stStaticParam;                 // 静态参数
    stBtnRunningParamTdf        stRunningParam;                // 运行参数
} 
stBtnDeviceParamTdf;

void vBtnStatusUpdata(emButDevNumTdf emDevNum);
void vBtnParamInit(stBtnStaticParamTdf *pststaticInit,emButDevNumTdf emDevNum);

emBtnStatusTdf emBtnGetCurStatus(emButDevNumTdf emDevNum);
emBtnStatusTdf emBtnGetLastStatus(emButDevNumTdf emDevNum);
uint32_t ulBtnGetPressTick(emButDevNumTdf emDevNum);
void vBtnSetPressTick(emButDevNumTdf emDevNum, uint32_t tick);
emButEventTdf emBtnGetCurEvent(emButDevNumTdf emDevNum);
void vBtnSetCurEvent(emButDevNumTdf emDevNum, emButEventTdf event);
uint32_t ulBtnGetClickTick(emButDevNumTdf emDevNum);
void vBtnSetClickTick(emButDevNumTdf emDevNum, uint32_t tick);

uint32_t ulBtnGetDeCntTh(emButDevNumTdf emDevNum);
uint32_t ulBtnGetLongPressCntTh(emButDevNumTdf emDevNum);
uint32_t ulBtnGetDoubleClickCntTh(emButDevNumTdf emDevNum);

uint8_t ucBtnGetucDoubleWait(emButDevNumTdf emDevNum);
void vBtnSetucDoubleWait(emButDevNumTdf emDevNum,uint8_t s);



void vBtnSetLastStatus(emButDevNumTdf emDevNum, emBtnStatusTdf status);
#endif
