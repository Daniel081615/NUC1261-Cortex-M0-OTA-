#ifndef __FW_OTA_MARK_SETUP_H__
#define __FW_OTA_MARK_SETUP_H__

#include "stdint.h"
#include "string.h"
#include "crc.h"
#include "stdbool.h"
#include "uart_user.h"
#include "NUC1261.h"
#include "wdt.h"
#include "fw_metadata.h"
#include "fw_status.h"

// UART commands
#define CMD_OTA_UPDATE           0xA7
#define CMD_TO_BOOTLOADER        0xAE
#define CMD_SWITCH_FW            0xAD
#define CMD_REPORT_STATUS        0xAF

// Packet size
#define MAX_PKT_SIZE             100



// Function declarations
void MarkFirmwareAsActive(bool reboot);
void ScanMetadataPair(FWstatus *g_fw_ctx, FWMetadata *active, FWMetadata *non_active);
#endif // __FW_OTA_MARK_SETUP_H__
