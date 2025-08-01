/***************************************************************************//**
 * @file     crc_user.c
 * @brief    CRC32 計算函式
 *
 * 提供通用 CRC32 計算功能，支援記憶體區段與資料陣列。
 * 使用硬體 CRC 模組，自動處理未對齊資料。
 ******************************************************************************/

#include "crc_user.h"

/**
 * @brief 通用 CRC32 計算（支援非 4-byte 對齊）
 * @param pData 資料指標
 * @param len   資料長度（位元組）
 * @return CRC32 結果
 */
uint32_t Common_CRC32_Calc(const uint8_t *pData, uint32_t len)
{
    uint32_t i, crc_result;

    CRC_Open(CRC_32, CRC_CHECKSUM_COM | CRC_CHECKSUM_RVS | CRC_WDATA_RVS,
             0xFFFFFFFF, CRC_CPU_WDATA_32);

    // 處理完整 4-byte 區塊
    for (i = 0; i + 4 <= len; i += 4) {
        uint32_t chunk;
        memcpy(&chunk, pData + i, 4);
        CRC->DAT = chunk;
    }

    // 處理剩餘資料（補 0xFF）
    if (i < len) {
        uint32_t lastChunk = 0xFFFFFFFF;
        memcpy(&lastChunk, pData + i, len - i);
        CRC->DAT = lastChunk;
    }

    crc_result = CRC_GetChecksum();
    return crc_result;
}

/**
 * @brief 計算記憶體區段的 CRC32
 * @param startAddr 起始位址
 * @param dataSize  資料長度
 * @return CRC32 結果
 */
uint32_t Calculate_CRC32(uint32_t startAddr, uint32_t dataSize)
{
    return Common_CRC32_Calc((const uint8_t *)startAddr, dataSize);
}

/**
 * @brief 計算資料陣列的 CRC32
 * @param data 資料指標
 * @param len  資料長度
 * @return CRC32 結果
 */
uint32_t CRC32_Calc(uint32_t *data, uint32_t len)
{
    return Common_CRC32_Calc((const uint8_t *)data, len);
}
