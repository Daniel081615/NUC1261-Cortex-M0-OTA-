/***************************************************************************//**
 * @file     targetdev.h
 * @brief    ISP 支援功能的標頭檔
 * @version  0x32
 *
 * @note
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __TARGETDEV_H__
#define __TARGETDEV_H__

#include "NUC1261.h"
#include "ISP_USER.h"

/**
 * @brief UART 設定（用於 ISP 傳輸）
 */
#define UART_T              UART0
#define UART_T_IRQHandler   UART02_IRQHandler
#define UART_T_IRQn         UART02_IRQn

/**
 * @brief Config 區域大小（單位：位元組）
 */
#define CONFIG_SIZE         8

#endif // __TARGETDEV_H__

/*** (C) COPYRIGHT 2019 Nuvoton Technology Corp. ***/
