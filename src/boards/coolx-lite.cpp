/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2016 Evgenykz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "mapinc.h"
#include "mmc3.h"

#include <sstream>
#include <iomanip>
#include <Windows.h>

static uint8* WRAM;
static uint8 MMC3_cmd;
static uint8 MMC3_mirroring;
static uint8 IRQCount;
static uint8 IRQReload;
static uint8 IRQLatch;
static uint8 IRQa;
static uint8 MMC3_mirroring_ext;
static uint8 MMC3_reg[8];

static uint8 CoolX_reg5;
static uint8 CoolX_reg1;
static uint8 CoolX_reg2;
static uint8 CoolX_reg3;
static uint8 CoolX_reg4;

//uint8_t coolx_mode = (CoolX_reg3 >> 2) & 3;
//uint8_t coolx_lock_vram = CoolX_reg3 & 1;
//uint8_t coolx_lock = (CoolX_reg3 >> 1) & 1;
//uint8_t coolx_sram_page = (CoolX_reg3 >> 4) & 0x0F;

//uint8_t coolx_vram_mask = (CoolX_reg4) & 0x1f;
//uint8_t coolx_write = (CoolX_reg4 >> 5) & 1;
//uint8_t coolx_mirroring = (CoolX_reg4 >> 6) & 1;
//uint8_t coolx_pgm32 = (CoolX_reg4 >> 7) & 1;

//uint8_t coolx_pgm_addr_mask_18_14 = CoolX_reg5 & 0x1F;
//uint8_t coolx_pgm_addr_or_18_14 = CoolX_reg1 & 0x1F;
//uint8_t coolx_pgm_addr_26_19 = CoolX_reg2;

static SFORMAT CoolXLite_StateRegs[] =
{
    { &MMC3_cmd, 1, "CMD" },
    { &MMC3_mirroring, 1, "A000" },
    { &IRQCount, 1, "IRQC" },
    { &IRQReload, 1, "IRQR" },
    { &IRQLatch, 1, "IRQL" },
    { &IRQa, 1, "IRQA" },
    { &MMC3_mirroring_ext, 1, "MIREX" },
    { MMC3_reg, 8, "mmc3" },

    { &CoolX_reg5, 1, "CXR5" },
    { &CoolX_reg1, 1, "CXR1" },
    { &CoolX_reg2, 1, "CXR2" },
    { &CoolX_reg3, 1, "CXR3" },
    { &CoolX_reg4, 1, "CXR4" },
    { 0 }
};

static void CoolXLite_PgmUpdate(void)
{
    uint8_t coolx_mode = (CoolX_reg3 >> 2) & 3;
    uint8_t coolx_pgm_addr_mask_18_14 = CoolX_reg5 & 0x1F;
    uint8_t coolx_pgm_addr_or_18_14 = CoolX_reg1 & 0x1F;
    uint8_t coolx_pgm_addr_26_19 = CoolX_reg2;

    uint32_t bank_and = ((coolx_pgm_addr_mask_18_14 << 1) | 0x01) & 0x3F;
    uint32_t bank_or = (coolx_pgm_addr_or_18_14 << 1) | (coolx_pgm_addr_26_19 << 6);

    if (coolx_mode == 0)
    {
        uint8_t coolx_pgm32 = (CoolX_reg4 >> 7) & 1;
        if (coolx_pgm32)
        {
            //same as aorom
            setprg8(0x8000, (((MMC3_reg[6] << 2) | 0) & bank_and) | bank_or);
            setprg8(0xA000, (((MMC3_reg[6] << 2) | 1) & bank_and) | bank_or);
            setprg8(0xC000, (((MMC3_reg[6] << 2) | 2) & bank_and) | bank_or);
            setprg8(0xE000, (((MMC3_reg[6] << 2) | 3) & bank_and) | bank_or);
        }
        else
        {
            uint8_t pgm_mode = (MMC3_cmd >> 7) & 1;
            if (pgm_mode == 0)
            {
                setprg8(0x8000, (MMC3_reg[6] & bank_and) | bank_or);
                setprg8(0xA000, (MMC3_reg[7] & bank_and) | bank_or);
                setprg8(0xC000, (bank_and | bank_or) & (~1));
                setprg8(0xE000, (bank_and | bank_or));
            }
        }
    }
    else if (coolx_mode == 0x01) //aorom
    {
        setprg8(0x8000, (((MMC3_reg[6] << 2) | 0) & bank_and) | bank_or);
        setprg8(0xA000, (((MMC3_reg[6] << 2) | 1) & bank_and) | bank_or);
        setprg8(0xC000, (((MMC3_reg[6] << 2) | 2) & bank_and) | bank_or);
        setprg8(0xE000, (((MMC3_reg[6] << 2) | 3) & bank_and) | bank_or);
    }
    else  //unrom
    {
        setprg8(0x8000, (((MMC3_reg[6] << 1) | 0) & bank_and) | bank_or);
        setprg8(0xA000, (((MMC3_reg[6] << 1) | 1) & bank_and) | bank_or);
        setprg8(0xC000, (bank_and | bank_or) & (~1));
        setprg8(0xE000, (bank_and | bank_or));
    }
    //uint8_t coolx_mode = (CoolX_reg3 >> 2) & 3;
    //uint8_t coolx_lock_vram = CoolX_reg3 & 1;
    //uint8_t coolx_lock = (CoolX_reg3 >> 1) & 1;
    //uint8_t coolx_sram_page = (CoolX_reg3 >> 4) & 0x0F;

    //uint8_t coolx_vram_mask = (CoolX_reg4) & 0x1f;
    //uint8_t coolx_write = (CoolX_reg4 >> 5) & 1;
    //uint8_t coolx_mirroring = (CoolX_reg4 >> 6) & 1;
    //uint8_t coolx_pgm32 = (CoolX_reg4 >> 7) & 1;

    //uint8_t coolx_pgm_addr_mask_18_14 = CoolX_reg5 & 0x1F;
    //uint8_t coolx_pgm_addr_or_18_14 = CoolX_reg1 & 0x1F;
    //uint8_t coolx_pgm_addr_26_19 = CoolX_reg2;
}

static void CoolXLite_ChrUpdate(void)
{
    uint8_t coolx_vram_mask = (CoolX_reg4) & 0x1f;
    uint8_t chr_mode = (MMC3_cmd >> 6) & 1;
    if (chr_mode == 0)
    {
        uint32_t bank_and = (coolx_vram_mask << 3) | 0x07;
        setchr2(0x0000, (MMC3_reg[0] & bank_and) >> 1);
        setchr2(0x0800, (MMC3_reg[1] & bank_and) >> 1);
        setchr1(0x1000, MMC3_reg[2] & bank_and);
        setchr1(0x1400, MMC3_reg[3] & bank_and);
        setchr1(0x1800, MMC3_reg[4] & bank_and);
        setchr1(0x1C00, MMC3_reg[5] & bank_and);
    }
    //uint8_t coolx_mode = (CoolX_reg3 >> 2) & 3;
    //uint8_t coolx_lock_vram = CoolX_reg3 & 1;
    //uint8_t coolx_lock = (CoolX_reg3 >> 1) & 1;
    //uint8_t coolx_sram_page = (CoolX_reg3 >> 4) & 0x0F;

    //
    //uint8_t coolx_write = (CoolX_reg4 >> 5) & 1;
    //uint8_t coolx_mirroring = (CoolX_reg4 >> 6) & 1;
    //uint8_t coolx_pgm32 = (CoolX_reg4 >> 7) & 1;

    //uint8_t coolx_pgm_addr_mask_18_14 = CoolX_reg5 & 0x1F;
    //uint8_t coolx_pgm_addr_or_18_14 = CoolX_reg1 & 0x1F;
    //uint8_t coolx_pgm_addr_26_19 = CoolX_reg2;
}

static void CoolXLite_MirroringUpdate(void)
{
    uint8_t coolx_mirroring = (CoolX_reg4 >> 6) & 1;
    if (coolx_mirroring == 0)
    {
        if (MMC3_mirroring == 0)
            setmirror(MI_V);
        else
            setmirror(MI_H);
    }
    else
    {
        if (MMC3_mirroring_ext == 0)
        {
            if (MMC3_mirroring == 0)
                setmirror(MI_0);
            else
                setmirror(MI_1);
        }
        else
        {
            if (MMC3_mirroring == 0)
                setmirror(MI_V);
            else
                setmirror(MI_H);
        }
    }
}

static DECLFW(CoolXLite_Write_8000_FFFF)
{
    uint8_t coolx_mode = (CoolX_reg3 >> 2) & 3;

    if (coolx_mode == 0x00) //mmc3
    {
        switch (A & 0xE001)
        {
        case 0x8000:
            MMC3_cmd = V;
            break;

        case 0x8001:
            MMC3_reg[MMC3_cmd & 0x07] = V;
            CoolXLite_PgmUpdate();
            CoolXLite_ChrUpdate();
            break;

        case 0xA000:
            MMC3_mirroring = V & 1;
            MMC3_mirroring_ext = (V >> 1) & 1;
            CoolXLite_MirroringUpdate();
            break;

        case 0xC000:
            IRQLatch = V;
            break;

        case 0xC001:
            IRQReload = 1;
            break;

        case 0xE000:
            X6502_IRQEnd(FCEU_IQEXT);
            IRQa = 0;
            break;

        case 0xE001:
            IRQa = 1;
            break;

        default:
            break;
        }
    }
    else if (coolx_mode == 0x01) //aorom
    {
        MMC3_reg[6] = V & 0x0F;
        MMC3_mirroring = (V >> 4) & 1;
        CoolXLite_PgmUpdate();
        CoolXLite_MirroringUpdate();
    }
    else //unrom
    {
        MMC3_reg[6] = V & 0x1F;
        CoolXLite_PgmUpdate();
    }
}

static DECLFW(CoolXLite_Write_4FF0)
{
    if (A == 0x4ff5)
        CoolX_reg5 = V;
    else if (A == 0x4ff1)
        CoolX_reg1 = V;
    else if (A == 0x4ff2)
        CoolX_reg2 = V;
    else if (A == 0x4ff3)
        CoolX_reg3 = V;
    else if (A == 0x4ff4)
        CoolX_reg4 = V;

    CoolXLite_PgmUpdate();
    CoolXLite_ChrUpdate();
    CoolXLite_MirroringUpdate();
}

static void COOLX_Lite_Power(void)
{
    MMC3_cmd = 0;
    MMC3_mirroring = 0;
    MMC3_mirroring_ext = 0;
    IRQCount = 0;
    IRQReload = 0;
    IRQLatch = 0;
    IRQa = 0;

    CoolX_reg5 = 3;
    CoolX_reg1 = 0;
    CoolX_reg2 = 0;
    CoolX_reg3 = 0;
    CoolX_reg4 = 0;

    SetWriteHandler(0x4ff0, 0x4fff, CoolXLite_Write_4FF0);
    SetWriteHandler(0x8000, 0xFFFF, CoolXLite_Write_8000_FFFF);
    SetReadHandler(0x8000, 0xFFFF, CartBR);

    SetWriteHandler(0x6000, 0x7FFF, CartBW);
    SetReadHandler(0x6000, 0x7FFF, CartBR);
    setprg8r(0x10, 0x6000, 0);

    CoolXLite_PgmUpdate();
    CoolXLite_ChrUpdate();
    CoolXLite_MirroringUpdate();
}

static void COOLX_Lite_Close(void)
{}

static void CoolXLite_hb(void)
{
    int count = IRQCount;
    if (!count || IRQReload)
    {
        IRQCount = IRQLatch;
        IRQReload = 0;
    } else
        IRQCount--;
    if (count && !IRQCount)
    {
        if (IRQa)
        {
            X6502_IRQBegin(FCEU_IQEXT);
        }
    }
}


void COOLX_Lite_Init(CartInfo *info)
{
    info->Power = COOLX_Lite_Power;
    info->Close = COOLX_Lite_Close;
    info->Reset = COOLX_Lite_Power;

    int chr_kb = 512;
    CHRmask1[0] &= (chr_kb >> 10) - 1;
    CHRmask2[0] &= (chr_kb >> 11) - 1;

    WRAM = (uint8*)FCEU_gmalloc(8 * 1024);
    SetupCartPRGMapping(0x10, WRAM, 8 * 1024, 1);
    AddExState(WRAM, 8 * 1024, 0, "WRAM");

    AddExState(CoolXLite_StateRegs, ~0, 0, 0);
    GameHBIRQHook = CoolXLite_hb;
}
