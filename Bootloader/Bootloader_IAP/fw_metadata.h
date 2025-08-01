/***************************************************************************//**
 * @file     fw_metadata.h
 * @version  V1.00 
 * @brief    metadata source file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __FW_METADATA_H__
#define __FW_METADATA_H__

#include "fmc.h"
#include "fmc_user.h"
#include "stdbool.h"

#define FW_FLAG_INVALID         (1 << 0)
#define FW_FLAG_VALID           (1 << 1)
#define FW_FLAG_PENDING					(1 << 2)
#define FW_FLAG_ACTIVE          (1 << 3)

#define FW_Status_Base					0x0001E800
#define METADATA_FW1_BASE     	0x0001F000  
#define METADATA_FW2_BASE				0x0001F800

#define FW1_BASE								0x00002000
#define FW2_BASE								0x00010000

//	FWStatus
typedef struct {
    uint32_t FW_Addr;
		uint32_t FW_meta_Addr;
    uint32_t status;
} FWstatus;

//	FWMetadata
typedef struct {
    uint32_t flags;
    uint32_t fw_crc32;
    uint32_t fw_version;
    uint32_t fw_start_addr;
    uint32_t fw_size;
    uint32_t trial_counter;  // 新增的試運行次數計數欄位
    uint32_t reserve;       // 預留欄位，之後可擴充其他用途
    uint32_t meta_crc;       // meta_crc 不包含此欄位本身的內容
} FWMetadata;


bool isUpdatable(FWMetadata *meta, int32_t status);
int WriteMetadata(FWMetadata *meta, uint32_t meta_base);
int WriteStatus(FWstatus *meta);

#endif