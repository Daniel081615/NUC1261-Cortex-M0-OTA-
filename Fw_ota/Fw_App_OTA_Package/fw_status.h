#ifndef FW_STATUS_H
#define FW_STATUS_H
#include <stdint.h>
#include "fmc_user.h"
#include "fmc.h"

// OTA update flags
#define OTA_UPDATE_FLAG          0xDDCCBBAA
#define SWITCH_FW_FLAG	         0xA5A5BEEF
#define	OTA_FAILED_FLAG					 0xDEADDEAD

#define FW_Status_Base           0x0001E800

// Firmware status structure
typedef struct {
    uint32_t FW_Addr;
    uint32_t FW_meta_Addr;
    uint32_t status;
} FWstatus;

int WriteFWstatus(FWstatus *meta);
void WRITE_FW_STATUS_FLAG(uint32_t flag);
#endif