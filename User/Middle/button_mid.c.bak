#include "stm32f1xx_hal.h"
#include "string.h"
#include "button_mid.h"

void vBtnEventUpdata(emButDevNumTdf emDevNum)
{
    uint32_t now = HAL_GetTick();
    emBtnStatusTdf cur  = emBtnGetCurStatus(emDevNum);
    emBtnStatusTdf last = emBtnGetLastStatus(emDevNum);

    // 1. 已经有事件了，不再处理，防止重复触发
    if(emBtnGetCurEvent(emDevNum) != emBtnEvent_None)
    {
        vBtnSetLastStatus(emDevNum, cur);
        return;
    }

    // 2. 刚按下：释放 → 按下
    if(cur == emBtnStatus_Pressed && last == emBtnStatus_Released)
    {
        vBtnSetPressTick(emDevNum, now);

        // ==============================================
        // 【核心修复：双击判断 放在 按下瞬间，不是释放时】
        // ==============================================
        if(ucBtnGetucDoubleWait(emDevNum) == 1)
        {
            // 两次按下间隔 < 双击时间 → 直接判定双击！
            if(now - ulBtnGetClickTick(emDevNum) <= ulBtnGetDoubleClickCntTh(emDevNum))
            {
                vBtnSetCurEvent(emDevNum, emBtnEvent_DoubleClick);  // 双击
                vBtnSetucDoubleWait(emDevNum, 0);
                vBtnSetClickTick(emDevNum, 0);
                vBtnSetLastStatus(emDevNum, cur);
                return;
            }
            else
            {
                vBtnSetucDoubleWait(emDevNum, 0);
            }
        }
    }

    // 3. 刚松开：按下 → 释放
    if(cur == emBtnStatus_Released && last == emBtnStatus_Pressed)
    {
        uint32_t press_time = now - ulBtnGetPressTick(emDevNum);

        // 长按判断
        if(press_time >= ulBtnGetLongPressCntTh(emDevNum))
        {
            vBtnSetCurEvent(emDevNum, emBtnEvent_LongPress);
        }
        // 短按 → 等待双击
        else if(press_time >= ulBtnGetDeCntTh(emDevNum))
        {
            vBtnSetClickTick(emDevNum, now);
            vBtnSetucDoubleWait(emDevNum, 1);
        }

        vBtnSetPressTick(emDevNum, 0);
    }

    // 4. 双击等待超时 → 触发单击
    if(ucBtnGetucDoubleWait(emDevNum) == 1)
    {
        if(now - ulBtnGetClickTick(emDevNum) > ulBtnGetDoubleClickCntTh(emDevNum))
        {
            vBtnSetCurEvent(emDevNum, emBtnEvent_Click);
            vBtnSetucDoubleWait(emDevNum, 0);
            vBtnSetClickTick(emDevNum, 0);
        }
    }

    vBtnSetLastStatus(emDevNum, cur);
}


// ================================
// 清空事件（绝对干净）
// ================================
void vBtnEventClear(emButDevNumTdf emDevNum)
{
    vBtnSetCurEvent(emDevNum, emBtnEvent_None);
    vBtnSetucDoubleWait(emDevNum, 0);
    vBtnSetClickTick(emDevNum, 0);
    vBtnSetPressTick(emDevNum, 0);
}

// ================================
// 主执行函数
// ================================
void vBtnExecute(void)
{
    uint8_t i;
    for (i = 0; i < BUTTON_NUM; i++)
    {
        vBtnStatusUpdata((emButDevNumTdf)i);
        vBtnEventUpdata((emButDevNumTdf)i);
    }
}