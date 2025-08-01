/******************************************************************************
 * @file     uart_user.h
 * @brief    General UART ISP slave header file
 * @version  1.0.0
 *
 * @note
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
 
#ifndef __UART_USER_H__
#define __UART_USER_H__

#include "NUC1261.h"

/*-------------------------------------------------------------*/
/* Define maximum packet size */
#define MAX_PKT_SIZE        	100
// Define uart header byte
#define UART_HEADER_BYTE      0x55
// Define uart tail byte
#define UART_TAIL_BYTE        0x0A

//#define UART_T							UART0
#define UART_T_IRQHandler			UART02_IRQHandler
#define UART_T_IRQn						UART02_IRQn

#define CONFIG_SIZE 8 // in bytes

/*-------------------------------------------------------------*/

extern uint8_t  uart_rcvbuf[];
extern uint8_t volatile bUartDataReady;
extern uint8_t volatile bufhead;

void UART02_IRQHandler(void);
void UART_Init();
void PutString(void);

#endif