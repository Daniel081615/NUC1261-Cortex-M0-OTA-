#ifndef FW_METADATA_H
#define FW_METADATA_H
#include <stdint.h>
#include <stdbool.h>

#include "fmc.h"

// Firmware flags
#define FW_FLAG_INVALID          (1 << 0)
#define FW_FLAG_VALID            (1 << 1)
#define FW_FLAG_PENDING          (1 << 2)
#define FW_FLAG_ACTIVE           (1 << 3)

// Memory addresses
#define APROM_BASE               0x00000000
#define FW1_BASE                 0x00002000
#define FW2_BASE                 0x00010000
#define METADATA_FW1_BASE        0x0001F000
#define METADATA_FW2_BASE        0x0001F800

// Firmware metadata structure
typedef struct __attribute__((packed)) {
    uint32_t flags;
    uint32_t fw_crc32;
    uint32_t fw_version;
    uint32_t fw_start_addr;
    uint32_t fw_size;
    uint32_t trial_counter;
    uint32_t magic;
    uint32_t meta_crc;
} FWMetadata;

// Metadata 相關操作
int WriteMetadata(FWMetadata *meta, uint32_t meta_base);
bool VerifyMetadataCRC(FWMetadata *meta);
bool VerifyFirmware(FWMetadata *meta);
#endif