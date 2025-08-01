#ifndef __FW_SELFCHECK_H__
#define __FW_SELFCHECK_H__

#include <stdbool.h>
#include <stdint.h>
#include "fw_ota_mark_setup.h"


//	functions
bool FirmwareSelfCheck(void);
void BlinkStatusLED(GPIO_T *port, uint32_t pin, uint8_t times, uint32_t delay_ms);
void ProcessUartPacket(void);
void CMD_SWITCHCASE(uint8_t lcmd, FWstatus* fw_ctx, FWMetadata* meta, FWMetadata* other);
void JumpToFirmware(uint32_t fwAddr);


#endif // __FW_SELFCHECK_H__
