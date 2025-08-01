#ifndef __FW_WDT_USER_H__
#define __FW_WDT_USER_H__

#include "NUC1261.h"

/// @brief 強制透過 WDT 重啟系統。
void ForceResetByWDT(void);
/// @brief 初始化 Watchdog Timer（WDT），設定約 1.6 秒的 timeout。
void InitWDT(void);

void BlinkLED(GPIO_T *port, uint32_t pin);
/**
 * @brief 餵狗函式，重置 Watchdog Timer 計數器，避免系統被重啟。
 *
 * 呼叫此函式可確保 MCU 在 WDT timeout 前不會被強制重啟。
 */
static inline void FeedWDT(void)
{
    WDT->RSTCNT |= WDT_RESET_COUNTER_KEYWORD;
}

#endif