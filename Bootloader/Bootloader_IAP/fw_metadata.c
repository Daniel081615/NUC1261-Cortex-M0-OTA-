/***************************************************************************//**
 * @file     fw_metadata.c
 * @brief    韌體 Metadata 寫入與狀態管理
 ******************************************************************************/

#include "fw_metadata.h"
#include <stdint.h>
#include "fmc.h"

#define MAX_FLASH_SIZE      0x001FFFFF          // 最大 Flash 容量
#define FLASH_PAGE_SIZE     0x800               // Flash Page 大小（2KB）
#define METADATA_SIZE       sizeof(FWMetadata)  // Metadata 結構大小

extern int32_t g_FMC_i32ErrCode;

/**
 * @brief  將 Metadata 寫入指定 Flash 位址
 * @param  meta      Metadata 指標
 * @param  meta_base 寫入起始位址
 * @return 0 成功，負值失敗
 */
int WriteMetadata(FWMetadata *meta, uint32_t meta_base)
{
    meta->meta_crc = CRC32_Calc((uint32_t *)meta, sizeof(FWMetadata) - sizeof(uint32_t));

    FMC_Open();
    FMC_ENABLE_ISP();

    if (FMC_Proc(FMC_ISPCMD_PAGE_ERASE, meta_base, meta_base + FLASH_PAGE_SIZE, 0) != 0) {
        g_FMC_i32ErrCode = -1;
        goto exit_proc;
    }

    if (FMC_Proc(FMC_ISPCMD_PROGRAM, meta_base, meta_base + sizeof(FWMetadata), (uint32_t *)meta) != 0) {
        g_FMC_i32ErrCode = -2;
        goto exit_proc;
    }

exit_proc:
    FMC_DISABLE_ISP();
    FMC_Close();
    return g_FMC_i32ErrCode;
}

/**
 * @brief  寫入韌體狀態資訊
 * @param  meta 狀態結構指標
 * @return 0 成功，負值失敗
 */
int WriteStatus(FWstatus *meta)
{
    FMC_Open();
    FMC_ENABLE_ISP();

    if (FMC_Proc(FMC_ISPCMD_PAGE_ERASE, FW_Status_Base, FW_Status_Base + FLASH_PAGE_SIZE, 0) != 0) {
        g_FMC_i32ErrCode = -1;
        goto exit_proc;
    }

    if (FMC_Proc(FMC_ISPCMD_PROGRAM, FW_Status_Base, FW_Status_Base + sizeof(FWstatus), (uint32_t *)meta) != 0) {
        g_FMC_i32ErrCode = -2;
        goto exit_proc;
    }

exit_proc:
    FMC_DISABLE_ISP();
    FMC_Close();
    return g_FMC_i32ErrCode;
}

/**
 * @brief  判斷 Metadata 是否可更新
 * @param  meta    Metadata 指標
 * @param  status  CRC 驗證結果
 * @return true 可更新，false 不可更新
 */
bool isUpdatable(FWMetadata *meta, int32_t status)
{
    if (meta->flags == 0xFFFF || status < 0) return true;  // 未初始化或 CRC 錯誤
    if (meta->flags & FW_FLAG_ACTIVE) return false;        // 執行中版本不可更新
    return true;  // 其他狀態允許更新
}
