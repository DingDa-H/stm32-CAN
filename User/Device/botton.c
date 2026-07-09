#include "stm32f1xx_hal.h"
#include "string.h"
#include "botton.h"


/// @brief      设备实例
///
/// @note
static stBtnDeviceParamTdf s_astBtnDevice[BUTTON_NUM];

/// @brief      按钮静态参数初始化
///
/// @note
static void s_vBtnStaticParamCopy(stBtnStaticParamTdf *pststaticInit,emButDevNumTdf emDevNum)
{
	memcpy(&s_astBtnDevice[emDevNum].stStaticParam,pststaticInit,sizeof(stBtnStaticParamTdf)/sizeof(uint8_t));
}

/// @brief      按钮设备参数初始化
///
/// @note
void vBtnParamInit(stBtnStaticParamTdf *pststaticInit,emButDevNumTdf emDevNum)
{
	
	s_vBtnStaticParamCopy(pststaticInit,emDevNum);
	
	// 先读取当前引脚电平，确定初始状态
    emBtnStatusTdf init_state;
    if(pststaticInit->emButLevel == emBtnActiveLevel_Low && 
       HAL_GPIO_ReadPin(pststaticInit->pstGpioBase, pststaticInit->usGpioPin) == GPIO_PIN_RESET)
        init_state = emBtnStatus_Pressed;
    else if(pststaticInit->emButLevel == emBtnActiveLevel_High && 
            HAL_GPIO_ReadPin(pststaticInit->pstGpioBase, pststaticInit->usGpioPin) == GPIO_PIN_SET)
        init_state = emBtnStatus_Pressed;
    else
        init_state = emBtnStatus_Released;
	
	//动态参数初始化
	s_astBtnDevice[emDevNum].stRunningParam.emCurrentEvent 		= emBtnEvent_None;
	s_astBtnDevice[emDevNum].stRunningParam.emCurrentStatus		= init_state;
	s_astBtnDevice[emDevNum].stRunningParam.emLastStatus		= init_state;
	s_astBtnDevice[emDevNum].stRunningParam.ulDebounceTick		= 0; // 原ulDebounceTick，改为头文件定义的ulDebounceTick
	s_astBtnDevice[emDevNum].stRunningParam.ulPressTick			= 0; // 原ulPressCount，改为头文件定义的ulPressTick
	s_astBtnDevice[emDevNum].stRunningParam.ulClickTick			= 0; // 原ulClickCount，改为头文件定义的ulClickTick
	s_astBtnDevice[emDevNum].stRunningParam.emlast_raw			= init_state; // 取消注释，初始化消抖原始状态
	s_astBtnDevice[emDevNum].stRunningParam.ucDoubleWait		= 0; 
}


/**
 * @brief 		按键状态读取与消抖处理
 * @param 		emDevNum：设备号
 * @data 		
 * @note 		根据指定按键的电平变化，更新按键的状态，这个需要一直扫描
 */

void vBtnStatusUpdata(emButDevNumTdf emDevNum)
{
	GPIO_PinState emPinLevel;

    emBtnStatusTdf real_state;
    // 读取实时电平，转换为按键状态

	emPinLevel = HAL_GPIO_ReadPin(s_astBtnDevice[emDevNum].stStaticParam.pstGpioBase, 
								s_astBtnDevice[emDevNum].stStaticParam.usGpioPin);
	if(emPinLevel == s_astBtnDevice[emDevNum].stStaticParam.emButLevel)
	{
		real_state = emBtnStatus_Pressed;
	}
	else
	{
		real_state = emBtnStatus_Released;
	}

    // 电平变化检测（与上一次记录的状态比较）
    if(real_state != s_astBtnDevice[emDevNum].stRunningParam.emCurrentStatus)
    {
		uint32_t now = HAL_GetTick();
		 if (s_astBtnDevice[emDevNum].stRunningParam.ulDebounceTick == 0)
		{
			// 首次检测到变化，记录起始时间
			s_astBtnDevice[emDevNum].stRunningParam.ulDebounceTick = now;
		}
		else if ((now - s_astBtnDevice[emDevNum].stRunningParam.ulDebounceTick) >=
             s_astBtnDevice[emDevNum].stStaticParam.ulDebounceMs)
		{
			s_astBtnDevice[emDevNum].stRunningParam.emLastStatus = 
                   s_astBtnDevice[emDevNum].stRunningParam.emCurrentStatus;
			// 持续时间超过消抖阈值，确认状态变化
			s_astBtnDevice[emDevNum].stRunningParam.emCurrentStatus = real_state;
			s_astBtnDevice[emDevNum].stRunningParam.ulDebounceTick = 0;
		}
    }
	else
	{
		// 状态恢复一致，取消消抖计时
		s_astBtnDevice[emDevNum].stRunningParam.ulDebounceTick = 0;
	}

}

//void vBtnDetectEvent(emButDevNumTdf emDevNum)
//{
//    emBtnStatusTdf emCurrentStatus = s_astBtnDevice[emDevNum].stRunningParam.emCurrentStatus;
//    emBtnStatusTdf emLastStatus = s_astBtnDevice[emDevNum].stRunningParam.emLastStatus;
//    
//    // 状态变化处理
//    if(emCurrentStatus != emLastStatus)
//    {
//        // 按下状态处理
//        if(emCurrentStatus == emBtnStatus_Pressed)
//        {
//            s_astBtnDevice[emDevNum].stRunningParam.ulPressCount = 0;
//        }
//        // 释放状态处理
//        else
//        {
//            // 判断是否为长按
//            if(s_astBtnDevice[emDevNum].stRunningParam.ulPressCount >= 
//               s_astBtnDevice[emDevNum].stStaticParam.ulLongPressMs)
//            {
//                s_astBtnDevice[emDevNum].stRunningParam.emCurrentEvent = emBtnEvent_LongPress;
//                s_astBtnDevice[emDevNum].stRunningParam.ulClickCount = 0;
//            }
//            // 短按处理
//            else
//            {
//                s_astBtnDevice[emDevNum].stRunningParam.ulClickCount++;
//                s_astBtnDevice[emDevNum].stRunningParam.ulDoubleClickCount = 0;
//            }
//        }
//    }
//    // 状态未变化处理
//    else
//    {
//        // 按下状态持续计数
//        if(emCurrentStatus == emBtnStatus_Pressed)
//        {
//            s_astBtnDevice[emDevNum].stRunningParam.ulPressCount++;
//        }
//        // 释放状态处理双击判断
//        else
//        {
//            if(s_astBtnDevice[emDevNum].stRunningParam.ulClickCount == 1)
//            {
//                s_astBtnDevice[emDevNum].stRunningParam.ulDoubleClickCount++;
//                if(s_astBtnDevice[emDevNum].stRunningParam.ulDoubleClickCount >= 
//                   s_astBtnDevice[emDevNum].stStaticParam.ulDebounceMs)
//                {
//                    s_astBtnDevice[emDevNum].stRunningParam.emCurrentEvent = emBtnEvent_Click;
//                    s_astBtnDevice[emDevNum].stRunningParam.ulClickCount = 0;
//                }
//            }
//            else if(s_astBtnDevice[emDevNum].stRunningParam.ulClickCount == 2)
//            {
//                s_astBtnDevice[emDevNum].stRunningParam.emCurrentEvent = emBtnEvent_DoubleClick;
//                s_astBtnDevice[emDevNum].stRunningParam.ulClickCount = 0;
//            }
//        }
//    }
//    
//    // 更新上一次状态
//    s_astBtnDevice[emDevNum].stRunningParam.emLastStatus = emCurrentStatus;
//}

// ===================== 驱动层对外接口 =====================
// 1. 获取当前状态
emBtnStatusTdf emBtnGetCurStatus(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stRunningParam.emCurrentStatus;
}

// 2. 获取上一次状态
emBtnStatusTdf emBtnGetLastStatus(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stRunningParam.emLastStatus;
}

// 3. 获取长按下计时
uint32_t ulBtnGetPressTick(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stRunningParam.ulPressTick;
}
// 4. 设置长按下计时
void vBtnSetPressTick(emButDevNumTdf emDevNum, uint32_t tick)
{
	s_astBtnDevice[emDevNum].stRunningParam.ulPressTick = tick;
}
// 5. 获取当前事件
emButEventTdf emBtnGetCurEvent(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stRunningParam.emCurrentEvent;
}
// 6. 设置当前事件
void vBtnSetCurEvent(emButDevNumTdf emDevNum, emButEventTdf event)
{
	s_astBtnDevice[emDevNum].stRunningParam.emCurrentEvent = event;
}

// 7. 获取双击间隔计时
uint32_t ulBtnGetClickTick(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stRunningParam.ulClickTick;
}
// 8. 设置双击间隔计时
void vBtnSetClickTick(emButDevNumTdf emDevNum, uint32_t tick)
{
	s_astBtnDevice[emDevNum].stRunningParam.ulClickTick = tick;
}
// 9. 获取消抖计数阈值
uint32_t ulBtnGetDeCntTh(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stStaticParam.ulDebounceMs;
}
// 10. 获取长按计数阈值
uint32_t ulBtnGetLongPressCntTh(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stStaticParam.ulLongPressMs;
}
// 11. 获取双击间隔计数阈值
uint32_t ulBtnGetDoubleClickCntTh(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stStaticParam.ulDoubleClickMs;
}
// 12. 获取等待双击的标志
uint8_t ucBtnGetucDoubleWait(emButDevNumTdf emDevNum)
{
	return s_astBtnDevice[emDevNum].stRunningParam.ucDoubleWait;
}

// 13. 设置等待双击的标志
void vBtnSetucDoubleWait(emButDevNumTdf emDevNum,uint8_t s)
{
	s_astBtnDevice[emDevNum].stRunningParam.ucDoubleWait = s;
}

void vBtnSetLastStatus(emButDevNumTdf emDevNum, emBtnStatusTdf status)
{
    s_astBtnDevice[emDevNum].stRunningParam.emLastStatus = status;
}
/**
 * @brief 		返回结构体指针函数
 * @param 		emDevNum：设备号
 * @data 		
 * @note 		只读的,给应用层用的
 */
const stBtnRunningParamTdf *c_pstGetBtnRunningParam(emButDevNumTdf emDevNum)
{
	return &s_astBtnDevice[emDevNum].stRunningParam;
}
