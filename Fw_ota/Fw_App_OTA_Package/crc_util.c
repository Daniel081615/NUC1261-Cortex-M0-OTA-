/***************************************************************************//**
 * @file     crc_util.c
 * @brief    CRC32 計算工具模組實作
 *
 * 本檔案提供 CRC32 計算相關函式，支援任意長度與非對齊資料的 CRC32 校驗。
 * 主要功能：
 * - 初始化並操作 NUC1261 CRC 硬體模組
 * - 以 4 bytes 為單位進行資料 CRC 計算
 * - 支援剩餘不足 4 bytes 的資料補齊運算
 *
 * 適用於韌體完整性驗證、資料封包校驗等應用場景。
 ******************************************************************************/

#include "crc_util.h"
#include <string.h>
#include "NUC1261.h"

/**
 * @brief 計算指定資料陣列的 CRC32
 * @param data 資料陣列指標
 * @param len 資料長度（bytes）
 * @return 計算出的 CRC32 值
 */
uint32_t CRC32_Calc(const uint8_t *pData, uint32_t len) {
    uint32_t i, crc_result;
    // 初始化 CRC 模組，設定為 32 位元、補數、反轉等模式
    CRC_Open(CRC_32, CRC_CHECKSUM_COM | CRC_CHECKSUM_RVS | CRC_WDATA_RVS, 0xFFFFFFFF, CRC_CPU_WDATA_32);

    // 以 4 bytes 為單位送入 CRC 計算
    for (i = 0; i + 4 <= len; i += 4) {
        uint32_t chunk;
        memcpy(&chunk, pData + i, 4);
        CRC->DAT = chunk;
    }

    // 若剩餘不足 4 bytes，補 0xFF 後送入 CRC 計算
    if (i < len) {
        uint32_t lastChunk = 0xFFFFFFFF;
        memcpy(&lastChunk, pData + i, len - i);
        CRC->DAT = lastChunk;
    }

    // 取得 CRC 計算結果
    crc_result = CRC_GetChecksum();
    return crc_result;
}