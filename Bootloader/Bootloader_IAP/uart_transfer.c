/**************************************************************************//**
 * @file     uart_transfer.c
 * @brief    UART ISP Slave 傳輸處理
 ******************************************************************************/

#include <string.h>
#include "targetdev.h"
#include "uart_transfer.h"

__attribute__((aligned(4))) uint8_t uart_rcvbuf[MAX_PKT_SIZE] = {0};  // 接收緩衝區
uint8_t volatile bUartDataReady = 0;  // 資料接收完成旗標
uint8_t volatile bufhead = 0;         // 緩衝區索引

/**
 * @brief UART 中斷處理函式
 */
void UART_T_IRQHandler(void)
{
    uint32_t u32IntSrc = UART_T->INTSTS;

    // 接收中斷或逾時中斷
    if (u32IntSrc & (UART_INTSTS_RXTOIF_Msk | UART_INTSTS_RDAIF_Msk)) {
        // 讀取 FIFO 中資料直到空或達最大長度
        while (((UART_T->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0) && (bufhead < MAX_PKT_SIZE)) {
            uart_rcvbuf[bufhead++] = UART_T->DAT;
        }
    }

    // 接收完成
    if (bufhead == MAX_PKT_SIZE) {
        bUartDataReady = TRUE;
        bufhead = 0;
    }
    // 若逾時但未滿一包，清除緩衝
    else if (u32IntSrc & UART_INTSTS_RXTOIF_Msk) {
        bufhead = 0;
    }
}

extern __attribute__((aligned(4))) uint8_t response_buff[100];

/**
 * @brief 傳送資料給主機
 */
void PutString(void)
{
    for (uint32_t i = 0; i < MAX_PKT_SIZE; i++) {
        while (UART_T->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);  // 等待 TX buffer 可寫
        UART_T->DAT = response_buff[i];                     // 傳送資料
    }
}

/**
 * @brief 初始化 UART
 */
void UART_Init()
{
    UART_T->FUNCSEL = UART_FUNCSEL_UART;  																				// 設定為 UART 模式
    UART_T->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;  				// 8N1 格式
    UART_T->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;  			  // FIFO 觸發等級
    UART_T->BAUD = (UART_BAUD_MODE0 | UART_BAUD_MODE0_DIVIDER(__HIRC, 115200));  	// 設定波特率
    UART_T->TOUT = (UART_T->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);  							 	// 設定逾時時間
    NVIC_SetPriority(UART_T_IRQn, 2);  																						// 設定中斷優先權
    NVIC_EnableIRQ(UART_T_IRQn);      																						// 啟用中斷
    UART_T->INTEN = UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk | UART_INTEN_RDAIEN_Msk;  // 啟用接收與逾時中斷
}
