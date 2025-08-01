#ifndef CRC_UTIL_H
#define CRC_UTIL_H
#include <stdint.h>

// 計算 CRC32
uint32_t CRC32_Calc(const uint8_t *data, uint32_t len);
#endif