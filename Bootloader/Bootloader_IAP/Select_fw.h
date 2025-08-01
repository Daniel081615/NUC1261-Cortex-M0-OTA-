#ifndef __SELECT_FW_H__
#define __SELECT_FW_H__

#include "stdbool.h"
#include "fw_metadata.h"
#include "fmc.h"
#include "crc_user.h"
#include "wdt.h"

/**
 * @def FWstatus
 * @brief 開機後 fw 操作的標記，寫在 Data Flash 中
 */
#define OTA_UPDATE_FLAG  	0xDDCCBBAA
#define OTA_PENDING_FLAG	0x0AA0A00A
#define OTA_FAILED_FLAG  	0xDEADDEAD
#define SWITCH_FW_FLAG  	0xA5A5BEEF

/**
 * @def APROM_SIZE
 * @brief APROM 的總大小（單位：位元組）。
 */
#define APROM_SIZE       0x00020000

/**
 * @def IS_VALID
 * @brief 檢查 Metadata 是否標記為有效。
 * 
 * @param meta 韌體 Metadata 結構。
 * @return 若有效則回傳 true。
 */
#define IS_VALID(meta)   (((meta).flags & FW_FLAG_VALID) != 0)

// --------------------------------------------------------------------
// WDT 模組化函式宣告
// --------------------------------------------------------------------

/**
 * @brief 初始化 Bootloader 使用的 Watchdog Timer（WDT）。
 */
void InitBootloaderWDT(void);

/**
 * @brief 重設 Watchdog Timer 計數器（餵狗）。
 */
void FeedWDT(void);

/**
 * @brief 清除 Watchdog Timer 的重設旗標。
 */
void ClearWDTResetFlags(void);

// --------------------------------------------------------------------
// Boot decision 輔助函式宣告
// --------------------------------------------------------------------

/**
 * @brief 根據韌體完整性更新 Metadata 的 flags。
 * 
 * @param meta 指向韌體 Metadata 結構的指標。
 * @param integrity 韌體是否通過完整性驗證。
 */
static void UpdateMetadataFlags(FWMetadata *meta, bool integrity);

/**
 * @brief 根據 Metadata 與安裝狀態選擇要啟動的韌體。
 * 
 * @param meta1 韌體 1 的 Metadata。
 * @param meta2 韌體 2 的 Metadata。
 * @param fw1Installed 韌體 1 是否已安裝。
 * @param fw2Installed 韌體 2 是否已安裝。
 * @return 指向選定韌體 Metadata 的指標。
 */
static FWMetadata* SelectFirmware(FWMetadata *meta1, FWMetadata *meta2, bool fw1Installed, bool fw2Installed);

/**
 * @brief 閃爍兩個 LED 指示燈以顯示狀態。
 * 
 * @param port 第一個 LED 所在的 GPIO 埠。
 * @param pin 第一個 LED 的腳位。
 * @param port2 第二個 LED 所在的 GPIO 埠。
 * @param pin2 第二個 LED 的腳位。
 * @param times 閃爍次數。
 * @param delay_ms 每次閃爍的延遲時間（毫秒）。
 */
void BlinkStatusLED(GPIO_T *port, uint32_t pin, GPIO_T *port2, uint32_t pin2, uint8_t times, uint32_t delay_ms);

// --------------------------------------------------------------------
// JumpToFirmware 模組化（可放入獨立文件中）
// --------------------------------------------------------------------

/**
 * @brief 跳轉至指定的韌體起始位址。
 * 
 * @param fwAddr 韌體的起始記憶體位址。
 */
void JumpToFirmware(uint32_t fwAddr);

/**
 * @brief 開機時的韌體選擇邏輯。
 * 
 * @retval -4 若需進入 ISP 模式（OTA 更新中）。
 * @retval -2 若無合法韌體可啟動。
 * @retval 0  成功跳轉至韌體（實際上不會回傳）。
 */
int32_t Boot_Decision(void);

/**
 * @brief 驗證指定韌體區塊的 CRC 是否正確。
 * 
 * @param meta 指向韌體 Metadata 結構的指標。
 * @return 若 CRC 驗證成功則回傳 true。
 */
bool VerifyFirmware(FWMetadata *meta);

/**
 * @brief 驗證韌體起始位址是否合法。
 * 
 * @param addr 韌體起始位址。
 * @return 若合法則回傳 true。
 */
static bool IsValidAddress(uint32_t addr);

#endif // __SELECT_FW_H__
