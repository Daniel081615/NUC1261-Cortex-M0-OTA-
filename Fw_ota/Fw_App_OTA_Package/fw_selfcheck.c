/***************************************************************************//**
 * @file     fw_selfcheck.c
 * @brief    MCU 開機自我檢查與流程控制模組
 *
 * 本檔案專責 MCU 開機自檢、OTA 流程控制、LED 狀態顯示、UART 指令分派。
 * - 韌體與 Metadata 驗證
 * - LED 狀態顯示
 * - UART 指令 switch-case 流程
 * - 不直接操作底層 Metadata 寫入
 ******************************************************************************/
 
#include "fw_selfcheck.h"
#include "fw_metadata.h"
#include "uart_comm.h"

// 全域變數，用來儲存目前的 Firmware 狀態資訊
static FWstatus g_fw_ctx;

/**
 * @brief 驗證目前 ACTIVE 的 Firmware 是否完整（Code + Metadata）。
 * 
 * 若驗證成功且符合 ACTIVE 或 PENDING 狀態，則回傳 true，否則回傳 false。
 * 並透過 LED 閃爍顯示驗證結果。
 * 
 * @retval true 驗證成功且狀態合法
 * @retval false 驗證失敗或狀態不合法
 */
bool FirmwareSelfCheck(void) {
    FWMetadata meta, other;
    ScanMetadataPair(&g_fw_ctx, &meta, &other);
    bool ok = VerifyFirmware(&meta);
    bool valid = (meta.flags == FW_FLAG_PENDING) ||
                 ((meta.flags & (FW_FLAG_ACTIVE | FW_FLAG_VALID)) == (FW_FLAG_ACTIVE | FW_FLAG_VALID));
		//	驗證成功，快速閃綠燈 : 紅燈
    BlinkStatusLED(valid ? PD : PF, valid ? 7 : 2, 4, 100);
    return ok && valid;
}

/**
 * @brief 控制 LED 閃爍，用於除錯或顯示狀態。
 */
void BlinkStatusLED(GPIO_T *port, uint32_t pin, uint8_t times, uint32_t delay_ms) {
    for (uint8_t i = 0; i < times; i++) {
        port->DOUT ^= (1 << pin);
        CLK_SysTickDelay(delay_ms * 1000);
    }
}

/**
 * @brief 處理 UART 封包命令。
 * 
 * 根據接收到的命令執行對應動作，如 OTA 更新、狀態回報、切換 Firmware 等。
 */
void ProcessUartPacket(void) {
	
		if((uart_rcvbuf[0] != UART_HEADER_BYTE) && (uart_rcvbuf[99] != UART_TAIL_BYTE))
				return;
    FWMetadata meta, other;
    FWstatus fw_ctx;
    ScanMetadataPair(&fw_ctx, &meta, &other);
    CMD_SWITCHCASE(uart_rcvbuf[2], &fw_ctx, &meta, &other);
    bufhead = 0;
    bUartDataReady = FALSE;
}

/**
 * @brief 處理 UART 命令的 switch-case 對應邏輯。
 * 
 * @param lcmd 命令代碼
 * @param fw_ctx 韌體狀態指標
 * @param meta 目前 Metadata
 * @param other 備援 Metadata
 */
void CMD_SWITCHCASE(uint8_t lcmd, FWstatus* fw_ctx, FWMetadata* meta, FWMetadata* other) {
    switch (lcmd) {
        case CMD_OTA_UPDATE:
            WRITE_FW_STATUS_FLAG(OTA_UPDATE_FLAG);
            Mcu_respond(CMD_OTA_UPDATE, fw_ctx, 0, 0);
            JumpToFirmware(APROM_BASE);
            break;
        case CMD_REPORT_STATUS:
            Mcu_respond(CMD_REPORT_STATUS, fw_ctx, meta, other);
            break;
        case CMD_TO_BOOTLOADER:
            Mcu_respond(CMD_TO_BOOTLOADER, fw_ctx, meta, 0);
            JumpToFirmware(APROM_BASE);
            break;
        case CMD_SWITCH_FW:
            WRITE_FW_STATUS_FLAG(SWITCH_FW_FLAG);
            MarkFirmwareAsActive(FALSE);
            Mcu_respond(CMD_SWITCH_FW, fw_ctx, meta, other);
						JumpToFirmware(APROM_BASE);
            break;
        default:
            break;
    }
}


void JumpToFirmware(uint32_t fwAddr)
{
    WDT_Open(WDT_TIMEOUT_2POW18, WDT_RESET_DELAY_18CLK, TRUE, TRUE);
    WDT->CTL |= WDT_CTL_WDTEN_Msk;
		WDT->RSTCNT |= WDT_RESET_COUNTER_KEYWORD;
    WDT_CLEAR_RESET_FLAG();
	
    __disable_irq();
    SYS_UnlockReg();
    FMC_Open();
    FMC_SetVectorPageAddr(fwAddr);
    NVIC_SystemReset();
}




