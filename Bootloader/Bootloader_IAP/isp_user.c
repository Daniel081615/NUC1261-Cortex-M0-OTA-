/******************************************************************************
 * @file      isp_user.c
 * @brief     ISP 韌體更新範例程式
 * @version   0x31
 ******************************************************************************/

#include <stdio.h>
#include "isp_user.h"
#include "fmc_user.h"
#include "uart_transfer.h"
#include "Select_fw.h"
#include "crc_user.h"

/*----------------------------- 全域變數區 -----------------------------*/

volatile uint8_t bISPDataReady;              // ISP 資料接收完成旗標
__attribute__((aligned(4))) uint8_t response_buff[100]; // 回應封包緩衝區
__attribute__((aligned(4))) static uint8_t aprom_buf[FMC_FLASH_PAGE_SIZE]; // APROM 暫存緩衝
uint32_t bUpdateApromCmd;                   // APROM 更新旗標
uint32_t g_apromSize;                       // APROM 大小
uint32_t g_dataFlashAddr, g_dataFlashSize;  // Data Flash 起始位址與大小

/**
 * @brief 計算 8-bit 校驗和
 */
__STATIC_INLINE uint8_t Checksum(unsigned char *buf, int len)
{
    int i;
    uint8_t c = 0;
    for (i = 0; i < len; i++) c += buf[i];
    return c;
}

/**
 * @brief 解析 ISP 指令並執行對應操作
 * @param buffer 接收封包
 * @param len    封包長度
 * @return 處理成功回傳 1，失敗回傳 0
 */
int ParseCmd(unsigned char *buffer, uint8_t len)
{
    static uint32_t StartAddress, TotalLen, LastDataLen;
    static uint8_t gcmd, g_packno;
    static bool g_fw_bank_locked;

    uint8_t *response = response_buff;
    uint8_t lcmd, lcksum;
    uint32_t srclen, i, regcnf0, security;
    unsigned char *pSrc = buffer;
    srclen = len;

    // 驗證封包格式
    if (len != 100 || buffer[0] != UART_HEADER_BYTE || buffer[99] != UART_TAIL_BYTE) return 0;

    FWMetadata meta;

    // 檢查 Checksum
    uint8_t calc_cksum = Checksum(&buffer[0], 98);
    if (calc_cksum != buffer[98]) {
        response[0] = UART_HEADER_BYTE;
        response[3] = buffer[3];
        response[98] = 0xFF;
        response[99] = UART_TAIL_BYTE;
        return 0;
    }

    // 解析命令與封包編號
    lcmd     = buffer[2];
    g_packno = buffer[3];
    pSrc     = &buffer[4];
    srclen   = 92;

    // 讀取 Config 設定
    ReadData(Config0, Config0 + 8, (uint32_t *)(response + 4));
    regcnf0 = *(uint32_t *)(response + 4);
    security = regcnf0 & 0x2;

    // 處理各種命令
    if (lcmd == CMD_SYNC_PACKNO) g_packno = buffer[3];
    if ((lcmd != 0) && (lcmd != CMD_RESEND_PACKET)) gcmd = lcmd;

    if (lcmd == CMD_GET_FWVER) {
        response[8] = FW_VERSION;
    }
    else if (lcmd == CMD_SEL_FW) {
        uint32_t fw_offset = *(uint32_t *)(pSrc);
        if ((fw_offset != FW1_BASE) && (fw_offset != FW2_BASE)) {
            *(uint32_t *)(response + 8) = 0xDEADBEEF;
            goto out;
        }
        FMC_Open();
        FMC_SetVectorPageAddr(fw_offset);
        NVIC_SystemReset();
        while (1);
    }
    else if (lcmd == CMD_GET_DEVICEID) {
        *(uint32_t *)(response + 8) = SYS->PDID;
        goto out;
    }
    else if (lcmd == CMD_RUN_APROM || lcmd == CMD_RUN_LDROM || lcmd == CMD_RESET) {
        SYS->RSTSTS = 3;
        if (lcmd == CMD_RUN_APROM) i = (FMC->ISPCTL & 0xFFFFFFFC);
        else if (lcmd == CMD_RUN_LDROM) i = (FMC->ISPCTL & 0xFFFFFFFC) | 0x00000002;
        else i = (FMC->ISPCTL & 0xFFFFFFFE);
        FMC->ISPCTL = i;
        SCB->AIRCR = (V6M_AIRCR_VECTKEY_DATA | V6M_AIRCR_SYSRESETREQ);
        while (1);
    }
    else if (lcmd == CMD_CONNECT) {
        g_packno = 0;
        goto out;
    }
    // 韌體更新/全擦除
    else if ((lcmd == CMD_UPDATE_APROM) || (lcmd == CMD_ERASE_ALL)) {
        if (lcmd == CMD_UPDATE_APROM) {
            gcmd = CMD_UPDATE_APROM;
            StartAddress = meta.fw_start_addr;
            TotalLen = meta.fw_size;
            if (StartAddress < FMC_APROM_BASE || (StartAddress + TotalLen) > g_dataFlashAddr) goto out;
            FMC_Open();
            EraseAP(StartAddress, TotalLen);
        }
        else if (lcmd == CMD_ERASE_ALL) {
            EraseAP(FMC_APROM_BASE, (g_apromSize < g_dataFlashAddr) ? g_apromSize : g_dataFlashAddr);
            EraseAP(g_dataFlashAddr, g_dataFlashSize);
            *(uint32_t *)(response + 4) = regcnf0 | 0x02;
            UpdateConfig((uint32_t *)(response + 4), NULL);
        }
        bUpdateApromCmd = TRUE;
    }
    // Metadata 更新
    else if (lcmd == CMD_UPDATE_METADATA) {
        FWMetadata newMeta;
        newMeta.fw_version = *(uint32_t *)pSrc;
        newMeta.fw_crc32 = *(uint32_t *)(pSrc + 4);
        newMeta.fw_size = *(uint32_t *)(pSrc + 8);
        newMeta.trial_counter = 0;
        newMeta.reserve = 0;
        newMeta.flags = 0;

        FWMetadata meta1 = {0}, meta2 = {0};
        int32_t status1 = ReadData(METADATA_FW1_BASE, METADATA_FW1_BASE + sizeof(FWMetadata), (uint32_t *)&meta1);
        int32_t status2 = ReadData(METADATA_FW2_BASE, METADATA_FW2_BASE + sizeof(FWMetadata), (uint32_t *)&meta2);

        bool candidate1 = isUpdatable(&meta1, status1);
        bool candidate2 = isUpdatable(&meta2, status2);

        uint32_t updateAddr;

        if (!g_fw_bank_locked) {
            if (candidate1 && !candidate2) {
                updateAddr = METADATA_FW1_BASE;
                newMeta.fw_start_addr = FW1_BASE;
            }
            else if (!candidate1 && candidate2) {
                updateAddr = METADATA_FW2_BASE;
                newMeta.fw_start_addr = FW2_BASE;
            }
            else {
                updateAddr = METADATA_FW2_BASE;
                newMeta.fw_start_addr = FW2_BASE;
            }
            g_fw_bank_locked = true;
        } else {
            if (updateAddr == METADATA_FW1_BASE)
                newMeta.fw_start_addr = FW1_BASE;
            else
                newMeta.fw_start_addr = FW2_BASE;
        }

        if (WriteMetadata(&newMeta, updateAddr) == 0) {
            *(uint32_t *)(response + 8) = newMeta.fw_start_addr;
            meta = newMeta;
        } else {
            *(uint32_t *)(response + 8) = 0xFFFFFFFF;
        }
        goto out;
    }
    // 重送封包
    else if (lcmd == CMD_RESEND_PACKET) {
        uint32_t PageAddress;
        StartAddress -= LastDataLen;
        TotalLen += LastDataLen;
        PageAddress = StartAddress & ~(FMC_FLASH_PAGE_SIZE - 1);
        if (PageAddress >= Config0) goto out;
        ReadData(PageAddress, StartAddress, (uint32_t *)aprom_buf);
        FMC_Erase_User(PageAddress);
        WriteData(PageAddress, StartAddress, (uint32_t *)aprom_buf);
        if ((StartAddress % FMC_FLASH_PAGE_SIZE) >= (FMC_FLASH_PAGE_SIZE - LastDataLen)) {
            FMC_Erase_User(PageAddress + FMC_FLASH_PAGE_SIZE);
        }
        goto out;
    }
    // 韌體寫入流程
    if ((gcmd == CMD_UPDATE_APROM)) {
        if (TotalLen < srclen) srclen = TotalLen;
        TotalLen -= srclen;
        FMC_Open();
        WriteData(StartAddress, StartAddress + srclen, (uint32_t *)pSrc);
        memset(pSrc, 0, srclen);
        ReadData(StartAddress, StartAddress + srclen, (uint32_t *)pSrc);
        WDT->RSTCNT = 0x00005AA5;
        StartAddress += srclen;
        LastDataLen = srclen;
        // 寫入完成驗證
        if ((gcmd == CMD_UPDATE_APROM) && (TotalLen == 0)) {
            PD7 = 0;
            FWstatus NowStatus;
            if (meta.fw_start_addr == FW1_BASE) {
                NowStatus.FW_Addr = FW1_BASE;
                NowStatus.FW_meta_Addr = METADATA_FW1_BASE;
                NowStatus.status = OTA_PENDING_FLAG;
            } else if (meta.fw_start_addr == FW2_BASE) {
                NowStatus.FW_Addr = FW2_BASE;
                NowStatus.FW_meta_Addr = METADATA_FW2_BASE;
                NowStatus.status = OTA_PENDING_FLAG;
            } else {
                goto out;
            }
            if (VerifyFirmware(&meta)) {
                meta.flags |= FW_FLAG_PENDING;
                WriteMetadata(&meta, NowStatus.FW_meta_Addr);
                WriteStatus(&NowStatus);
                JumpToFirmware(NowStatus.FW_Addr);
            } else {
                meta.flags = FW_FLAG_INVALID;
                WriteMetadata(&meta, NowStatus.FW_meta_Addr);
                JumpToFirmware(0x0000);
            }
        }
    }

out:
    // 組裝回應封包
    response[0] = UART_HEADER_BYTE;
    response[1] = 0xFF;
    response[2] = lcmd;
    response[3] = g_packno;
    lcksum = Checksum(&response[0], 98);
    response[98] = lcksum;
    response[99] = UART_TAIL_BYTE;
    ++g_packno;

    return 0;
}