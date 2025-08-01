/***************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    韌體啟動驗證與 OTA 等待主程式
 *
 * 本程式為韌體啟動後的主流程，包含以下功能：
 * - 系統時鐘與 UART 初始化
 * - 啟動 WDT（看門狗）
 * - 韌體自我驗證（CRC 檢查）
 * - 驗證成功後標記為 ACTIVE
 * - 若驗證失敗則寫入 OTA 失敗旗標並強制重啟
 * - 主迴圈中持續餵狗並等待 UART OTA 指令
 * - 閃爍 LED 顯示狀態
 *
 * 適用於支援 OTA 更新與雙韌體切換的系統架構。
 ******************************************************************************/

#include "stdio.h"
#include "NUC1261.h"
#include "string.h"
#include "uart_user.h"
#include "fmc.h"
#include "fw_ota_mark_setup.h"
#include "fw_selfcheck.h"
#include "wdt_user.h"

#define PLLCTL_SETTING      CLK_PLLCTL_72MHz_HIRC
#define PLL_CLOCK           72000000

volatile bool g_marked_active = false;

void BlinkLED(GPIO_T *port, uint32_t pin);

/**
 * @brief 硬體錯誤處理（保留空函式，可擴充）
 */
void ProcessHardFault(void)
{
}

// ...existing code...

void SH_Return(void)
{
}

// ...existing code...

/**
 * @brief 系統初始化：設定時鐘、UART、GPIO 等
 */
void SYS_Init(void)
{
    // 啟用 HIRC 並等待穩定
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;
    while(!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

    // HCLK 使用 HIRC，除頻為 1
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(1);

    // 設定 PLL 並等待穩定
    CLK->PLLCTL = PLLCTL_SETTING;
    while(!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk));

    // 切換 HCLK 為 PLL
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_PLL;

    // 更新系統時鐘參數
    PllClock        = PLL_CLOCK;
    SystemCoreClock = PLL_CLOCK;
    CyclesPerUs     = PLL_CLOCK / 1000000;

    // 啟用 UART0 與 CRC 模組時鐘 、WDT
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;
    CLK->AHBCLK  |= CLK_AHBCLK_CRCCKEN_Msk;

    // UART0 使用 HIRC，除頻為 1
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UARTSEL_Msk)) | CLK_CLKSEL1_UARTSEL_HIRC;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_UARTDIV_Msk)) | CLK_CLKDIV0_UART(1);


    // 設定 UART0 腳位（PD0: RXD, PD1: TXD）
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD0MFP_Msk | SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD0MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD);

    // 啟用 GPIOF 模組時鐘（LED 用）
    CLK->AHBCLK |= CLK_AHBCLK_GPIOFCKEN_Msk;
}

/**
 * @brief 主程式入口點
 */
int32_t main(void)
{
    SYS_UnlockReg();   // 解鎖系統保護
    SYS_Init();        // 初始化系統時鐘與 UART
    UART_Init();       // 初始化 UART
    InitWDT();         // 初始化 WDT

    // 韌體自我驗證（CRC 檢查）
    bool selfCheckPassed = FirmwareSelfCheck();
    if (!selfCheckPassed)
    {
        BlinkStatusLED(PF, 2, 4, 50);  // 顯示錯誤狀態
        WRITE_FW_STATUS_FLAG(OTA_FAILED_FLAG);  // 標記 OTA 失敗
				MarkFirmwareAsActive(FALSE);
        ForceResetByWDT();  // 強制重啟
    }

    // 驗證成功後標記為 ACTIVE（只做一次）
    if (!g_marked_active) {
        MarkFirmwareAsActive(TRUE);
        g_marked_active = true;
    }

    // 主迴圈：餵狗 + 處理 UART OTA 指令 + 閃爍 LED
    while (1)
    {
				FeedWDT();
        if (bUartDataReady == TRUE)
        {
            ProcessUartPacket();
        }
				/*		紅燈 : PF2 ,
							綠燈 : PD7		*/
        BlinkLED(PD, 7);  // 閃爍 LED 顯示狀態
    }
}

/**
 * @brief 閃爍指定 LED（非阻塞）
 * @param port GPIO 埠
 * @param pin  腳位
 */
void BlinkLED(GPIO_T *port, uint32_t pin)
{
    static uint32_t tick = 0;
    if (tick++ > 400) {
        port->DOUT ^= (1 << pin);  // 反轉 LED 狀態
        tick = 0;
    }
    CLK_SysTickDelay(400);  // 延遲 400us
}


