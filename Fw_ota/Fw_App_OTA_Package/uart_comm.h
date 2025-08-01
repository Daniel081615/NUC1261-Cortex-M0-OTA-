#ifndef UART_COMM_H
#define UART_COMM_H
#include <stdint.h>
#include "fw_metadata.h"
#include "fw_status.h"

void Mcu_respond(uint8_t cmd, FWstatus *status, FWMetadata *meta, FWMetadata *nameta);

#endif