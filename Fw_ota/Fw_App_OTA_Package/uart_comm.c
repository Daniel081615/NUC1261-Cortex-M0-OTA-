#include "uart_comm.h"
#include <string.h>
#include "uart_user.h" // PutString, uart_rcvbuf 等
#include "wdt_user.h"

extern uint8_t response_buff[100];

/**
 * @brief MCU 響應封包製作與傳送（含 Metadata 狀態）
 * @param cmd     響應指令碼
 * @param status  韌體狀態結構（可為 NULL）
 * @param meta    啟用中的 Metadata（可為 NULL）
 * @param nameta  非啟用的 Metadata（可為 NULL）
 */
void Mcu_respond(uint8_t cmd, FWstatus *status, FWMetadata *meta, FWMetadata *nameta)
{
    static uint8_t packet_seq = 0;

    FMC_Open();


    // Step 1：初始化 payload（總長度固定 94 bytes，預設填 0xFF）
    memset(&response_buff[4], 0xFF, 94);

    if (status)
        memcpy(&response_buff[4], status, sizeof(FWstatus));
    if (meta)
        memcpy(&response_buff[20], meta, sizeof(FWMetadata));
    if (nameta)
        memcpy(&response_buff[52], nameta, sizeof(FWMetadata));

    // Step 2：填入封包頭尾與控制欄位
    response_buff[0] = UART_HEADER_BYTE;
    response_buff[1] = uart_rcvbuf[1];  // Center ID
    response_buff[2] = cmd;
    response_buff[3] = packet_seq++;

    // Step 3：計算 checksum（XOR 0~97）
    uint8_t checksum = 0;
    for (int i = 0; i < 98; i++) {
        checksum ^= response_buff[i];
    }
    response_buff[98] = checksum;
    response_buff[99] = UART_TAIL_BYTE;

    // Step 4：透過 UART 傳送封包
    PutString();  // 確保傳送 100 bytes
		while (!(UART0->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk));
}

