#include "stm32f1xx_hal.h"
#include "string.h"
#include <stdint.h>
#include <stdio.h>


//定时触发标志位
static uint8_t s_ucTimFlag;

/**
 * @brief  TIM2更新中断回调，100ms进一次
 * @param  htim 定时器句柄
 * @retval 无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        s_ucTimFlag = 1;
    }
}


// 获取标志
uint8_t ucTimDeviceGetFlag(void)
{
    return s_ucTimFlag;
}

// 清除标志
void vTimDeviceClearFlag(void)
{
    s_ucTimFlag = 0;
}
