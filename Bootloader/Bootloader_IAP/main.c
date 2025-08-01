/**************************************************************************//**
 * @file     main.c
 * @brief    透過 UART 與 PC 連線，示範如何更新晶片 Flash 資料。
 ******************************************************************************/

#include "stdio.h"
#include "targetdev.h"
#include "uart_transfer.h"
#include "Select_fw.h"
#include "fw_metadata.h"
#include "crc_user.h"
#include "Select_fw.h"

#define PLLCTL_SETTING      CLK_PLLCTL_72MHz_HIRC
#define PLL_CLOCK           71884800

#define OTA_FLAG_ADDR       0x1E800
#define OTA_FLAG_MAGIC      0xA55A5AA5

int32_t main(void);
void ProcessHardFault(void);
void SH_Return(void);

void ProcessHardFault(void){}  // 硬體錯誤處理（保留）
void SH_Return(void){}         // 軟體中斷返回（保留）

// 系統初始化：時鐘、UART、多功能腳位設定
int32_t SYS_Init(void)
{
    uint32_t u32TimeOutCnt;

    // 啟用 HIRC
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;
    u32TimeOutCnt = SystemCoreClock;
    while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
        if(--u32TimeOutCnt == 0) return -1;

    // 設定 PLL 為系統時鐘
    CLK->PLLCTL = PLLCTL_SETTING;
    u32TimeOutCnt = SystemCoreClock;
    while (!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk))
        if(--u32TimeOutCnt == 0) return -1;
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_PLL;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(1);

    // 更新系統時鐘參數
    PllClock        = PLL_CLOCK;
    SystemCoreClock = PLL_CLOCK;
    CyclesPerUs     = PLL_CLOCK / 1000000;

    // 啟用 UART、WDT、CRC 時鐘
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk | CLK_APBCLK0_WDTCKEN_Msk;
    CLK->AHBCLK  |= CLK_AHBCLK_CRCCKEN_Msk;

    // 設定 UART 時鐘來源
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UARTSEL_Msk)) | CLK_CLKSEL1_UARTSEL_HIRC | CLK_CLKSEL1_WDTSEL_LIRC;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_UARTDIV_Msk)) | CLK_CLKDIV0_UART(1);

    // 設定 LED G/R
    PD->MODE = (PD->MODE & (~GPIO_MODE_MODE7_Msk)) | (GPIO_MODE_OUTPUT << GPIO_MODE_MODE7_Pos);
    PF->MODE = (PF->MODE & (~GPIO_MODE_MODE2_Msk)) | (GPIO_MODE_OUTPUT << GPIO_MODE_MODE2_Pos);

    // 設定 UART0 多功能腳位
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD0MFP_Msk | SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD0MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD);

    return 0;
}

// 主程式進入點
int32_t main(void)
{
    SYS_UnlockReg();  // 解鎖保護暫存器

    if( SYS_Init() < 0 )  // 初始化系統
        goto _APROM;

    // 設定 WDT
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk | CLK_APBCLK0_WDTCKEN_Msk;
    WDT->CTL = WDT_TIMEOUT_2POW18 | WDT_CTL_RSTEN_Msk | WDT_CTL_WDTEN_Msk;
    WDT->RSTCNT |= WDT_RESET_COUNTER_KEYWORD;

    UART_Init();  // 初始化 UART

    FMC->ISPCTL |= FMC_ISPCTL_ISPEN_Msk;  // 啟用 ISP

    // 取得 APROM 與 Data Flash 資訊
    g_apromSize = GetApromSize();
    GetDataFlashInfo(&g_dataFlashAddr, &g_dataFlashSize);

    // 判斷是否進入 ISP 模式
    int32_t bootStatus = Boot_Decision();
    if (bootStatus != 0)
    {
        WDT->CTL &= ~WDT_CTL_WDTEN_Msk;
        goto _ISP_MODE;
    }

_ISP_MODE:
    // 等待 UART 接收 CMD_CONNECT 指令
    while (1)
    {
        if ((uart_rcvbuf[0] == UART_HEADER_BYTE) && (uart_rcvbuf[99] == UART_TAIL_BYTE) && (bUartDataReady == TRUE))
        {
            uint8_t lcmd = uart_rcvbuf[2];
            if (lcmd == CMD_CONNECT)
                break;
            else
            {
                bUartDataReady = FALSE;
                bufhead = 0;
            }
        }
    }

    // ISP 指令處理迴圈
    while (1)
    {
        if (bUartDataReady == TRUE)
        {
            // 重設 WDT
            WDT->CTL &= ~(WDT_CTL_WDTEN_Msk | WDT_CTL_ICEDEBUG_Msk);
            WDT->CTL |= (WDT_TIMEOUT_2POW18 | WDT_CTL_RSTEN_Msk);

            bUartDataReady = FALSE;
            ParseCmd(uart_rcvbuf, 100);  // 解析 UART 指令
            PutString();                 // 傳送回應
        }
    }

_APROM:
    // 重設系統並從 APROM 開機
    SYS->RSTSTS = (SYS_RSTSTS_PORF_Msk | SYS_RSTSTS_PINRF_Msk);
    FMC->ISPCTL = FMC->ISPCTL & 0xFFFFFFFC;
    SCB->AIRCR = (V6M_AIRCR_VECTKEY_DATA | V6M_AIRCR_SYSRESETREQ);
    while (1);
}