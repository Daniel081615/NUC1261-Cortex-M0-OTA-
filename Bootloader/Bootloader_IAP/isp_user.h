/**************************************************************************//**
 * @file     isp_user.h
 * @brief    ISP Command header file
 * @version  0x32
 *
 * @note
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef ISP_USER_H
#define ISP_USER_H

#define FW_VERSION 0x34

#include "fmc_user.h"
#include "fw_metadata.h"
#include "crc_user.h"
#include "stdbool.h"
#include <string.h>

#define CMD_UPDATE_APROM      0xA0
#define CMD_UPDATE_CONFIG     0xA1
#define CMD_READ_CONFIG       0xA2
#define CMD_ERASE_ALL         0xA3
#define CMD_SYNC_PACKNO       0xA4
#define	CMD_UPDATE_METADATA		0xA5
#define CMD_GET_FWVER         0xA6
#define CMD_SEL_FW						0xA7	
#define CMD_RUN_APROM         0xAB
#define CMD_RUN_LDROM         0xAC
#define CMD_RESET             0xAD
#define CMD_CONNECT           0xAE
#define CMD_GET_DEVICEID      0xB1
#define CMD_RESEND_PACKET     0xFF

#define V6M_AIRCR_VECTKEY_DATA    0x05FA0000UL
#define V6M_AIRCR_SYSRESETREQ     0x00000004UL

// targetdev.c
extern void GetDataFlashInfo(uint32_t *addr, uint32_t *size);
extern uint32_t GetApromSize(void);

// isp_user.c
extern int ParseCmd(unsigned char *buffer, uint8_t len);
extern uint32_t g_apromSize, g_dataFlashAddr, g_dataFlashSize;
extern __attribute__((aligned(4))) uint8_t response_buff[100];
extern volatile uint8_t bISPDataReady;

extern __attribute__((aligned(4))) uint8_t usb_rcvbuf[];

#endif	// #ifndef ISP_USER_H

/*** (C) COPYRIGHT 2019 Nuvoton Technology Corp. ***/
