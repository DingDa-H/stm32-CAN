#ifndef __TIM_DEVICE_H
#define __TIM_DEVICE_H

#include "stm32f1xx_hal.h"
#include "project.h"


uint8_t ucTimDeviceGetFlag(void);
void vTimDeviceClearFlag(void);

#endif
