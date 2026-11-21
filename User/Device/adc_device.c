/**
 * @brief 		实现adc的底层驱动
 * @author		
 * @version 	2026.11.21
 * @data 		
 * @note		当前配置为单次转换，非扫描模式
 */
#include "adc_device.h"
#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1; 		// 引入 CubeMX 生成的 ADC 句柄
extern void Error_Handler(void); 		// <--- 告诉编译器这个函数在别的文件里
/**
 * @brief ADC 设备初始化（主要是校准）
 */
void vAdcDeviceInit(void)
{
    // 1. 执行 ADC 校准（F1 芯片必做）
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        // 校准失败处理（一般不会失败，可以直接死循环或打印日志）
        Error_Handler(); 
    }
}

/**
 * @brief 触发一次 ADC 转换并获取 12 位码值
 * @retval 0~4095 的 ADC 码值
 */
uint16_t usAdcDeviceGetValue(void)
{
    static uint16_t s_usLastValidValue = 0U; 	// 保存上一次有效值
    uint16_t usCurrentValue = s_usLastValidValue;

    // 1. 启动转换：失败直接返回上次有效值，避免后面的轮询白等 10ms
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return s_usLastValidValue;
    }

    // 2. 轮询等待转换完成，超时时间设为 10ms（对 6us 的转换来说极其宽裕）
    if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
    {
        // 3. 读取转换结果
        usCurrentValue = (uint16_t)HAL_ADC_GetValue(&hadc1);
        s_usLastValidValue = usCurrentValue; // 更新有效值
    }
    // 转换失败或超时：usCurrentValue 保持上一次有效值，防止发送突变的假数据

    // 4. 停止转换（单次模式，省电且防干扰）
    HAL_ADC_Stop(&hadc1);

    return usCurrentValue;
}
