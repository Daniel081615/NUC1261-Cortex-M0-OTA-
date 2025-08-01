#include "wdt_user.h"
#include "wdt.h"

/**
 * @brief 初始化獨立看門狗 (WDT)，避免程式死鎖。
 * 
 * 啟用 HIRC 時鐘與 WDT 模組，設定 timeout 約為 1.6 秒，
 * 並啟用 WDT 重啟功能，最後清除上次的 WDT Reset Flag。
 */
void InitWDT(void) {
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;
    while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));
    CLK->APBCLK0 |= CLK_APBCLK0_WDTCKEN_Msk;
    WDT->CTL = WDT_TIMEOUT_2POW18 | WDT_CTL_RSTEN_Msk | WDT_CTL_WDTEN_Msk;
		WDT->RSTCNT |= WDT_RESET_COUNTER_KEYWORD;
    SYS->RSTSTS |= SYS_RSTSTS_WDTRF_Msk;
}


/**
 * @brief 強制透過 Watchdog Timer (WDT) 重啟系統。
 * 
 * 設定 WDT 為短 timeout 並啟用 Reset，進入無限迴圈等待重啟。
 */
void ForceResetByWDT(void)
{
    SYS_UnlockReg();
    WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, FALSE);
    while (1);  // 等待 WDT timeout 自動重啟
}