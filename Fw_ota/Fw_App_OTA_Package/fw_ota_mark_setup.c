/***************************************************************************//**
 * @file     fw_ota_mark_setup.c
 * @brief    OTA 標記與 Metadata 操作模組
 *
 * 本檔案專責 OTA 相關旗標、Metadata 狀態區的底層操作。
 * - Metadata 寫入、切換、標記 ACTIVE/PENDING
 * - 狀態旗標寫入
 * - 不處理流程控制與自我檢查
 ******************************************************************************/

#include "fw_ota_mark_setup.h"


// 外部 UART 傳送緩衝區
extern uint8_t response_buff[100];
 
 /**
 * @brief 將指定的 Firmware 標記為 ACTIVE 並設為 VALID。
 * @param mark 若為 true，表示目前 metadata 為主；false 則切換至備援 metadata。
 */
void MarkFirmwareAsActive(bool mark) {
    FWMetadata meta, other;
    FWstatus fw_ctx;
    ScanMetadataPair(&fw_ctx, &meta, &other);
		
    uint32_t other_addr = (fw_ctx.FW_meta_Addr == METADATA_FW1_BASE) ? METADATA_FW2_BASE : METADATA_FW1_BASE;
    if (mark) {
        meta.flags &= ~FW_FLAG_PENDING;
        meta.flags |= (FW_FLAG_VALID | FW_FLAG_ACTIVE);
				meta.trial_counter += 1;
        other.flags &= ~FW_FLAG_ACTIVE;
    } else if (fw_ctx.status == SWITCH_FW_FLAG) {
        other.flags &= ~FW_FLAG_PENDING;
        other.flags |= (FW_FLAG_VALID | FW_FLAG_ACTIVE);
        meta.flags &= ~FW_FLAG_ACTIVE;
    }
    WriteMetadata(&meta, fw_ctx.FW_meta_Addr);
    WriteMetadata(&other, other_addr);
}

/**
 * @brief 掃描目前與備援的 Metadata 區塊。
 * @param ctx 儲存目前狀態的 FWstatus 結構指標
 * @param active 指向目前使用的 Metadata 結構
 * @param backup 指向備援的 Metadata 結構
 */
void ScanMetadataPair(FWstatus *ctx, FWMetadata *active, FWMetadata *backup) {
		FMC_Open();
    ReadData(FW_Status_Base, FW_Status_Base + sizeof(FWstatus), (uint32_t *)ctx);
    uint32_t addr[2] = { METADATA_FW1_BASE, METADATA_FW2_BASE };
    uint32_t idx = (ctx->FW_meta_Addr == METADATA_FW1_BASE) ? 0 : 1;
    ReadData(addr[idx], addr[idx] + sizeof(FWMetadata), (uint32_t *)active);
    ReadData(addr[1 - idx], addr[1 - idx] + sizeof(FWMetadata), (uint32_t *)backup);
}