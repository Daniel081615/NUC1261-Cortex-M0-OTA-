#ifndef __CRC_USER_H__
#define __CRC_USER_H__

#include "NUC1261.h"
#include <string.h>    ///< 使用 memcpy 確保安全讀取
#include "crc.h"

/**
 * @brief 計算指定記憶體區段的 CRC32。
 * 
 * @param startAddr 起始記憶體位址。
 * @param dataSize 資料長度（位元組）。
 * @return 計算出的 CRC32 值。
 */
uint32_t Calculate_CRC32(uint32_t startAddr, uint32_t dataSize);

/**
 * @brief 計算指定資料陣列的 CRC32。
 * 
 * @param data 指向資料陣列的指標。
 * @param len 資料長度（位元組）。
 * @return 計算出的 CRC32 值。
 */
uint32_t CRC32_Calc(uint32_t *data, uint32_t len);

/**
 * @brief 通用 CRC32 計算函式，支援任意位元組資料。
 * 
 * @param pData 指向資料的位元組指標。
 * @param len 資料長度（位元組）。
 * @return 計算出的 CRC32 值。
 */
uint32_t Common_CRC32_Calc(const uint8_t *pData, uint32_t len);

#endif // __CRC_USER_H__
