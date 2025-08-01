/***************************************************************************//**
 * @file     fw_status.c
 * @brief    韌體狀態管理模組實作
 *
 * 本檔案負責 Firmware 狀態結構（FWstatus）的寫入與旗標設定。
 * 主要功能：
 * - 將 FWstatus 結構寫入固定 Flash 區域
 * - 提供狀態旗標的安全寫入（含原子操作）
 *
 * 適用於 OTA、雙韌體等需追蹤韌體狀態的應用場景。
 ******************************************************************************/

#include "fw_status.h"

/**
 * @brief 將 FWstatus 結構寫入固定 Flash 區域。
 *
 * 會先開啟 FMC ISP，擦除目標區塊，再將 FWstatus 結構寫入指定區域。
 * 若任一步驟失敗，會自動關閉 ISP 並回傳錯誤碼。
 *
 * @param meta FWstatus 結構指標
 * @return 0 表示成功，負值表示失敗
 */
int WriteFWstatus(FWstatus *meta) {
    FMC_Open();
    FMC_ENABLE_ISP();
    // 先擦除目標 Flash Page
    if (FMC_Proc(FMC_ISPCMD_PAGE_ERASE, FW_Status_Base, FW_Status_Base + FMC_FLASH_PAGE_SIZE, 0) != 0) goto fail;
    // 再寫入 FWstatus 結構
    if (FMC_Proc(FMC_ISPCMD_PROGRAM, FW_Status_Base, FW_Status_Base + sizeof(FWstatus), (uint32_t *)meta) != 0) goto fail;
    FMC_DISABLE_ISP(); FMC_Close();
    return 0;
fail:
    FMC_DISABLE_ISP(); FMC_Close();
    return -1;
}

/**
 * @brief 寫入 Firmware 狀態旗標。
 *
 * 會先讀取原始 FWstatus 結構，修改狀態欄位後寫回 Flash，
 * 並於寫入後關閉中斷，確保狀態切換過程不被打斷。
 *
 * @param flag 要寫入的狀態旗標值
 */
void WRITE_FW_STATUS_FLAG(uint32_t flag) {
		FMC_Open();
    FWstatus OTA_Status;
    ReadData(FW_Status_Base, FW_Status_Base + sizeof(FWstatus), (uint32_t *)&OTA_Status);
    OTA_Status.status = flag;
    WriteFWstatus(&OTA_Status);
    __disable_irq();
}