/***************************************************************************//**
 * @file     fw_metadata.c
 * @brief    韌體 Metadata 寫入與驗證模組
 *
 * 提供 Metadata 寫入 Flash、CRC 計算與驗證功能，適用於雙韌體與 OTA 更新。
 ******************************************************************************/

#include "fw_metadata.h"
#include "crc_util.h"
#include "fmc.h"
#include "fmc_user.h"

/**
 * @brief 將 Metadata 寫入 Flash，並更新 meta_crc。
 *
 * 計算 CRC 後寫入 meta_crc，執行 Flash 擦除與寫入。
 *
 * @param meta      Metadata 指標（會更新 meta_crc）
 * @param meta_base Flash 寫入起始位址
 * @return 0 成功，-1 失敗
 */
int WriteMetadata(FWMetadata *meta, uint32_t meta_base) {
    FMC_Open();
    meta->meta_crc = CRC32_Calc((const uint8_t *)meta, sizeof(FWMetadata) - sizeof(uint32_t));
    FMC_ENABLE_ISP();
    if (FMC_Proc(FMC_ISPCMD_PAGE_ERASE, meta_base, meta_base + FMC_FLASH_PAGE_SIZE, 0) != 0) goto fail;
    if (FMC_Proc(FMC_ISPCMD_PROGRAM, meta_base, meta_base + sizeof(FWMetadata), (uint32_t *)meta) != 0) goto fail;
    FMC_DISABLE_ISP(); FMC_Close();
    return 0;
fail:
    FMC_DISABLE_ISP(); FMC_Close();
    return -1;
}

/**
 * @brief 驗證韌體區塊的 CRC 是否與 Metadata 相符。
 *
 * 根據 fw_start_addr 與 fw_size 計算 CRC，與 fw_crc32 比對。
 *
 * @param meta Metadata 指標
 * @return true 通過，false 失敗
 */
bool VerifyFirmware(FWMetadata *meta) {
    if ((meta->fw_start_addr == 0xFFFFFFFF) ||
        (meta->fw_size == 0xFFFFFFFF) ||
        (meta->fw_crc32 == 0xFFFFFFFF)) return false;
    uint32_t crc = CRC32_Calc((const uint8_t *)meta->fw_start_addr, meta->fw_size);
    return (crc == meta->fw_crc32);
}

/**
 * @brief 驗證 Metadata 自身的 CRC。
 *
 * 計算結構內容（不含 meta_crc）並比對。
 *
 * @param meta Metadata 指標
 * @return true 通過，false 失敗
 */
bool VerifyMetadataCRC(FWMetadata *meta) {
    uint32_t calc_crc = CRC32_Calc((const uint8_t *)meta, sizeof(FWMetadata) - sizeof(uint32_t));
    return (calc_crc == meta->meta_crc);
}
