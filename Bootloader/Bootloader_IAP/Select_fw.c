/******************************************************************************
 * @file     Select_fw.c
 * @version  V1.00
 * @brief    開機韌體選擇與跳轉邏輯
 * 1. 根據 Metadata 判斷啟動哪個韌體或進入 ISP
 * 2. 驗證韌體與 Metadata CRC
 * 3. 跳轉指定韌體
 * 4. WDT 初始化與餵狗
 * 5. LED 狀態顯示
 ******************************************************************************/

#include "Select_fw.h"
#include "wdt.h" 

// 檢查 Metadata 是否有效且啟用
bool is_valid_active(FWMetadata *meta) {
    if (meta->flags == 0xFFFFFFFF ||
                meta->fw_start_addr == 0xFFFFFFFF ||
                meta->meta_crc == 0xFFFFFFFF)
        return false;
    return (meta->flags & (FW_FLAG_VALID | FW_FLAG_ACTIVE)) == (FW_FLAG_VALID | FW_FLAG_ACTIVE);
}

// 開機決定啟動哪個韌體或進入 ISP
int32_t Boot_Decision(void)
{
    FWstatus Nowstatus = {0};
    FMC_Open();
    ReadData(FW_Status_Base, FW_Status_Base + sizeof(FWstatus), (uint32_t *)&Nowstatus);

    // OTA 狀態進入 ISP
    if (Nowstatus.status == OTA_UPDATE_FLAG) {
        FMC_Erase(FW_Status_Base);
        return -4;
    }

    // 讀取兩份 Metadata
    FWMetadata meta1 = {0}, meta2 = {0};
    ReadData(METADATA_FW1_BASE, METADATA_FW1_BASE + sizeof(FWMetadata), (uint32_t *)&meta1);
    ReadData(METADATA_FW2_BASE, METADATA_FW2_BASE + sizeof(FWMetadata), (uint32_t *)&meta2);

    // 選擇合法且啟用的韌體
    FWMetadata* selectedFW = NULL;
    if (is_valid_active(&meta1)) {
        selectedFW = &meta1;
        Nowstatus.FW_Addr = FW1_BASE;
        Nowstatus.FW_meta_Addr = METADATA_FW1_BASE;
    } else if (is_valid_active(&meta2)) {
        selectedFW = &meta2;
        Nowstatus.FW_Addr = FW2_BASE;
        Nowstatus.FW_meta_Addr = METADATA_FW2_BASE;
    }
    Nowstatus.status = 0;

    // 驗證起始位址
    if (!selectedFW || !IsValidAddress(selectedFW->fw_start_addr)) {
        return -2;
    }
    WriteStatus(&Nowstatus);
    JumpToFirmware(selectedFW->fw_start_addr);

    return 0;
}

// WDT 初始化
void InitBootloaderWDT(void)
{
    WDT_Open(WDT_TIMEOUT_2POW16, WDT_RESET_DELAY_18CLK, TRUE, FALSE);
    WDT->CTL |= WDT_CTL_WDTEN_Msk;
    ClearWDTResetFlags();
}

// 餵狗
void FeedWDT(void)
{
    WDT->RSTCNT |= WDT_RESET_COUNTER_KEYWORD;
}

// 清除 WDT Reset 旗標
void ClearWDTResetFlags(void)
{
    WDT_CLEAR_RESET_FLAG();
}

// 跳轉至指定韌體
void JumpToFirmware(uint32_t fwAddr)
{
    BlinkStatusLED(PD, 7, PF, 2, 5, 2000);
    WDT_Open(WDT_TIMEOUT_2POW16, WDT_RESET_DELAY_18CLK, TRUE, TRUE);
    WDT->RSTCNT |= WDT_RESET_COUNTER_KEYWORD;
    WDT_CLEAR_RESET_FLAG();
    __disable_irq();
    SYS_UnlockReg();
    FMC_Open();
    FMC_SetVectorPageAddr(fwAddr);
    NVIC_SystemReset();
}

// 驗證韌體 CRC
bool VerifyFirmware(FWMetadata *meta)
{
    if ((meta->fw_start_addr == 0xFFFFFFFF) ||
        (meta->fw_size       == 0xFFFFFFFF) ||
        (meta->fw_crc32      == 0xFFFFFFFF))
        return false;
    uint32_t crc = Calculate_CRC32(meta->fw_start_addr, meta->fw_size);
    return (crc == meta->fw_crc32);
}

// 驗證韌體起始位址
static bool IsValidAddress(uint32_t addr)
{
    return ((addr == 0x2000 || addr == 0x10000) &&
            (addr % 4 == 0) &&
            (addr >= FW1_BASE && addr < APROM_SIZE));
}

// 驗證 Metadata CRC
bool VerifyMetadataCRC(FWMetadata *meta)
{
    uint32_t calc_crc = Calculate_CRC32((uint32_t)meta, sizeof(FWMetadata) - sizeof(uint32_t));
    return (calc_crc == meta->meta_crc);
}

// 兩顆 LED 同步閃爍
void BlinkStatusLED(GPIO_T *port, uint32_t pin, GPIO_T *port2, uint32_t pin2, uint8_t times, uint32_t delay_ms)
{
    for (uint8_t i = 0; i < times; i++) {
        port->DOUT ^= (1 << pin);
        port2->DOUT ^= (1 << pin2);
        CLK_SysTickDelay(delay_ms * 1000);
        port->DOUT ^= (1 << pin);
        port2->DOUT ^= (1 << pin2);
        CLK_SysTickDelay(delay_ms * 1000);
    }
}