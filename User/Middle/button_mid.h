#ifndef __BUTTON_MID_H
#define __BUTTON_MID_H

#include "botton.h"
#include "project.h"
#include STM32_IC_HAL


void vBtnEventClear(emButDevNumTdf emDevNum);
void vBtnExecute(void);

void vBtnEventUpdata(emButDevNumTdf emDevNum);

#endif
