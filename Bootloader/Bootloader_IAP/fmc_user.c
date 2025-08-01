/***************************************************************************//**
 * @file     fmc_user.c
 * @brief    Flash 操作函式
 ******************************************************************************/

#include <stdio.h>
#include "fmc_user.h"

// 全域錯誤碼
int32_t g_FMC_i32ErrCode = 0;

/**
 * @brief 執行 FMC 指令（讀寫擦除）
 * @param u32Cmd     指令類型
 * @param addr_start 起始位址
 * @param addr_end   結束位址
 * @param data       資料指標（可為 NULL）
 * @return 0 成功，-1 失敗
 */
int FMC_Proc(unsigned int u32Cmd, unsigned int addr_start, unsigned int addr_end, unsigned int *data)
{
    unsigned int u32Addr, Reg;
    uint32_t u32TimeOutCnt;

    for (u32Addr = addr_start; u32Addr < addr_end; data++) {
        FMC->ISPCMD = u32Cmd;
        FMC->ISPADDR = u32Addr;

        if (u32Cmd == FMC_ISPCMD_PROGRAM)
            FMC->ISPDAT = *data;

        FMC->ISPTRG = 0x1;
        __ISB();

        u32TimeOutCnt = FMC_TIMEOUT_WRITE;
        while (FMC->ISPTRG & 0x1)
            if (--u32TimeOutCnt == 0) return -1;

        Reg = FMC->ISPCTL;
        if (Reg & FMC_ISPCTL_ISPFF_Msk) {
            FMC->ISPCTL = Reg;
            return -1;
        }

        if (u32Cmd == FMC_ISPCMD_READ)
            *data = FMC->ISPDAT;

        u32Addr += (u32Cmd == FMC_ISPCMD_PAGE_ERASE) ? FMC_FLASH_PAGE_SIZE : 4;
    }

    return 0;
}

/**
 * @brief 更新 Config 區設定
 * @param data 要寫入的資料
 * @param res  若不為 NULL，讀回結果
 */
void UpdateConfig(unsigned int *data, unsigned int *res)
{
    unsigned int u32Size = CONFIG_SIZE;

    FMC_ENABLE_CFG_UPDATE();
    FMC_Proc(FMC_ISPCMD_PAGE_ERASE, Config0, Config0 + 8, 0);
    FMC_Proc(FMC_ISPCMD_PROGRAM, Config0, Config0 + u32Size, data);

    if (res)
        FMC_Proc(FMC_ISPCMD_READ, Config0, Config0 + u32Size, res);

    FMC_DISABLE_CFG_UPDATE();
}