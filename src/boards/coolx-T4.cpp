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
static uint8 MMC3_reg[16];

static uint8 CoolT4_reg1;
static uint8 CoolT4_reg2;
static uint8 CoolT4_reg3;
static uint8 CoolT4_reg4;
static uint8 CoolT4_reg5;

static uint8_t CoolT4_flash_cmd = 0;
static uint8_t CoolT4_flash_factory = 0;

#define MAPPER_MMC3     0
#define MAPPER_CNROM    1
#define MAPPER_UNROM    2
#define MAPPER_UNROM512 3
#define MAPPER_MMC1     4
#define MAPPER_AOROM        6
#define MAPPER_COLOR_DREAMS 7

//uint8_t coolt4_mapper = CoolT4_reg1 & 7;
//uint8_t coolt4_ex_mirroring = (CoolT4_reg1 >> 3) & 1;
//uint8_t coolt4_tomas_c4 = (CoolT4_reg1 >> 4) & 1;
//uint8_t coolt4_write = (CoolT4_reg1 >> 5) & 1;
//uint8_t coolt4_lock = (CoolT4_reg1 >> 7) & 1;

//uint8_t coolt4_pgm_mask_19_15 = CoolT4_reg2;
//uint8_t coolt4_pgm_or_19_15 = CoolT4_reg3 & 0x1F;
//uint8_t coolt4_pgm_page_26_20 = CoolT4_reg4 & 0x7F;

//uint8_t coolt4_lock_vram = (CoolT4_reg5 >> 0) & 1;
//uint8_t coolt4_vram_a18 = (CoolT4_reg5 >> 1) & 1;
//uint8_t coolt4_sram_page = (CoolT4_reg5 >> 2) & 3;
//uint8_t coolt4_vram_mask_17_15 = (CoolT4_reg5 >> 4) & 7;


static SFORMAT CoolXLite_StateRegs[] =
{
    { &MMC3_cmd, 1, "CMD" },
    { &MMC3_mirroring, 1, "A000" },
    { &IRQCount, 1, "IRQC" },
    { &IRQReload, 1, "IRQR" },
    { &IRQLatch, 1, "IRQL" },
    { &IRQa, 1, "IRQA" },
    { MMC3_reg, 16, "mmc3" },

    { &CoolT4_reg1, 1, "CT41" },
    { &CoolT4_reg2, 1, "CT42" },
    { &CoolT4_reg3, 1, "CT43" },
    { &CoolT4_reg4, 1, "CT44" },
    { &CoolT4_reg5, 1, "CT45" },
    { 0 }
};

static void CoolT4_PgmUpdate(void)
{
    uint8_t coolt4_pgm_mask_19_15 = CoolT4_reg2;
    uint8_t coolt4_pgm_or_19_15 = CoolT4_reg3 & 0x1F;
    uint8_t coolt4_pgm_page_26_20 = CoolT4_reg4 & 0x7F;

    uint32_t bank_and = ((coolt4_pgm_mask_19_15 << 2) | 0x03) & 0x7F;
    uint32_t bank_or = (coolt4_pgm_or_19_15 << 2) | (coolt4_pgm_page_26_20 << 7);

    uint8_t coolt4_mapper = CoolT4_reg1 & 7;
    uint8_t coolt4_tomas_c4 = (CoolT4_reg1 >> 4) & 1;

    if (coolt4_mapper == MAPPER_MMC3 || coolt4_mapper == MAPPER_CNROM)
    {
        uint8_t pgm_mode = (MMC3_cmd >> 6) & 1;
        if (pgm_mode == 0)
        {
            if (coolt4_tomas_c4 == 0)
            {
                setprg8(0x8000, (MMC3_reg[6] & bank_and) | bank_or);
                setprg8(0xA000, (MMC3_reg[7] & bank_and) | bank_or);
                setprg8(0xC000, (bank_and | bank_or) & (~1));
                setprg8(0xE000, (bank_and | bank_or));
            }
            else
            {
                setprg8(0x8000, (MMC3_reg[6] & bank_and) | bank_or);
                setprg8(0xA000, (MMC3_reg[7] & bank_and) | bank_or);
                setprg8(0xC000, (MMC3_reg[8] & bank_and) | bank_or);
                setprg8(0xE000, (MMC3_reg[9] & bank_and) | bank_or);
            }
        }
        else
        {
            if (coolt4_tomas_c4 == 0)
            {
                setprg8(0x8000, (bank_and | bank_or) & (~1));
                setprg8(0xA000, (MMC3_reg[7] & bank_and) | bank_or);
                setprg8(0xC000, (MMC3_reg[6] & bank_and) | bank_or);
                setprg8(0xE000, (bank_and | bank_or));
            }
            else
            {
                setprg8(0x8000, (MMC3_reg[8] & bank_and) | bank_or);
                setprg8(0xA000, (MMC3_reg[7] & bank_and) | bank_or);
                setprg8(0xC000, (MMC3_reg[6] & bank_and) | bank_or);
                setprg8(0xE000, (MMC3_reg[9] & bank_and) | bank_or);
            }
        }
    }
    else if (coolt4_mapper == MAPPER_AOROM) //aorom
    {
        setprg8(0x8000, (((MMC3_reg[6] << 2) | 0) & bank_and) | bank_or);
        setprg8(0xA000, (((MMC3_reg[6] << 2) | 1) & bank_and) | bank_or);
        setprg8(0xC000, (((MMC3_reg[6] << 2) | 2) & bank_and) | bank_or);
        setprg8(0xE000, (((MMC3_reg[6] << 2) | 3) & bank_and) | bank_or);
    }
    else  if (coolt4_mapper == MAPPER_UNROM || coolt4_mapper == MAPPER_UNROM512) //unrom unrom512
    {
        setprg8(0x8000, (((MMC3_reg[6] << 1) | 0) & bank_and) | bank_or);
        setprg8(0xA000, (((MMC3_reg[6] << 1) | 1) & bank_and) | bank_or);
        setprg8(0xC000, (bank_and | bank_or) & (~1));
        setprg8(0xE000, (bank_and | bank_or));
    }
    else if (coolt4_mapper == MAPPER_COLOR_DREAMS)
    {
        setprg8(0x8000, (((MMC3_reg[6] << 2) | 0) & bank_and) | bank_or);
        setprg8(0xA000, (((MMC3_reg[6] << 2) | 1) & bank_and) | bank_or);
        setprg8(0xC000, (((MMC3_reg[6] << 2) | 2) & bank_and) | bank_or);
        setprg8(0xE000, (((MMC3_reg[6] << 2) | 3) & bank_and) | bank_or);
    }
    else if (coolt4_mapper == MAPPER_MMC1)
    {
        uint8_t pgm_mode = MMC3_reg[3] & 3;
        if (pgm_mode == 0 || pgm_mode == 1)
        {
            //32k bank
            uint32_t bank_base = (MMC3_reg[6] & 0xFE) << 1;
            setprg8(0x8000, ((bank_base | 0) & bank_and) | bank_or);
            setprg8(0xA000, ((bank_base | 1) & bank_and) | bank_or);
            setprg8(0xC000, ((bank_base | 2) & bank_and) | bank_or);
            setprg8(0xE000, ((bank_base | 3) & bank_and) | bank_or);
        }
        else if (pgm_mode == 2)
        {
            setprg8(0x8000, (0 & bank_and) | bank_or);
            setprg8(0xA000, (1 & bank_and) | bank_or);
            setprg8(0xC000, (((MMC3_reg[6] << 1) | 0) & bank_and) | bank_or);
            setprg8(0xE000, (((MMC3_reg[6] << 1) | 1) & bank_and) | bank_or);
        }
        else if (pgm_mode == 3)
        {
            setprg8(0x8000, (((MMC3_reg[6] << 1) | 0) & bank_and) | bank_or);
            setprg8(0xA000, (((MMC3_reg[6] << 1) | 1) & bank_and) | bank_or);
            setprg8(0xC000, (bank_and | bank_or) & (~1));
            setprg8(0xE000, (bank_and | bank_or));
        }
    }
}

static void CoolT4_ChrUpdate(void)
{
    uint8_t coolt4_vram_a18 = (CoolT4_reg5 >> 1) & 1;
    uint8_t coolt4_vram_mask_17_15 = (CoolT4_reg5 >> 4) & 7;

    //uint8_t coolx_vram_mask = (CoolX_reg4) & 0x1f;
    uint8_t chr_mode = (MMC3_cmd >> 7) & 1;
    //uint32_t bank_and = (coolx_vram_mask << 3) | 0x07;
    uint32_t bank_and = (coolt4_vram_mask_17_15 << 5) | 0x1F;
    uint32_t bank_or = coolt4_vram_a18 ? 0x100 : 0x00;
    uint8_t coolt4_tomas_c4 = (CoolT4_reg1 >> 4) & 1;

    uint8_t coolt4_mapper = CoolT4_reg1 & 7;

    if (coolt4_mapper == MAPPER_MMC3)
    {
        if (chr_mode == 0)
        {
            if (coolt4_tomas_c4 == 0)
            {
                setchr2(0x0000, ((MMC3_reg[0] & bank_and) | bank_or) >> 1);
                setchr2(0x0800, ((MMC3_reg[1] & bank_and) | bank_or) >> 1);
                setchr1(0x1000, (MMC3_reg[2] & bank_and) | bank_or);
                setchr1(0x1400, (MMC3_reg[3] & bank_and) | bank_or);
                setchr1(0x1800, (MMC3_reg[4] & bank_and) | bank_or);
                setchr1(0x1C00, (MMC3_reg[5] & bank_and) | bank_or);
            }
            else
            {
                setchr1(0x0000, (MMC3_reg[0x0] & bank_and) | bank_or);
                setchr1(0x0400, (MMC3_reg[0xA] & bank_and) | bank_or);
                setchr1(0x0800, (MMC3_reg[0x1] & bank_and) | bank_or);
                setchr1(0x0C00, (MMC3_reg[0xB] & bank_and) | bank_or);
                setchr1(0x1000, (MMC3_reg[0x2] & bank_and) | bank_or);
                setchr1(0x1400, (MMC3_reg[0x3] & bank_and) | bank_or);
                setchr1(0x1800, (MMC3_reg[0x4] & bank_and) | bank_or);
                setchr1(0x1C00, (MMC3_reg[0x5] & bank_and) | bank_or);
            }
        }
        else
        {
            if (coolt4_tomas_c4 == 0)
            {
                setchr1(0x0000, (MMC3_reg[2] & bank_and) | bank_or);
                setchr1(0x0400, (MMC3_reg[3] & bank_and) | bank_or);
                setchr1(0x0800, (MMC3_reg[4] & bank_and) | bank_or);
                setchr1(0x0C00, (MMC3_reg[5] & bank_and) | bank_or);
                setchr2(0x1000, ((MMC3_reg[0] & bank_and) | bank_or) >> 1);
                setchr2(0x1800, ((MMC3_reg[1] & bank_and) | bank_or) >> 1);
            }
            else
            {
                setchr1(0x0000, (MMC3_reg[0x2] & bank_and) | bank_or);
                setchr1(0x0400, (MMC3_reg[0x3] & bank_and) | bank_or);
                setchr1(0x0800, (MMC3_reg[0x4] & bank_and) | bank_or);
                setchr1(0x0C00, (MMC3_reg[0x5] & bank_and) | bank_or);
                setchr1(0x1000, (MMC3_reg[0x0] & bank_and) | bank_or);
                setchr1(0x1400, (MMC3_reg[0xA] & bank_and) | bank_or);
                setchr1(0x1800, (MMC3_reg[0x1] & bank_and) | bank_or);
                setchr1(0x1C00, (MMC3_reg[0xB] & bank_and) | bank_or);
            }
        }
    }
    else if (coolt4_mapper == MAPPER_UNROM512 || coolt4_mapper == MAPPER_COLOR_DREAMS || coolt4_mapper == MAPPER_CNROM)
    {
        //setchr8(MMC3_reg[0] | bank_or);
        uint8_t bank8k = MMC3_reg[0] << 3;
        setchr1(0x0000, ((bank8k | 0) & bank_and) | bank_or);
        setchr1(0x0400, ((bank8k | 1) & bank_and) | bank_or);
        setchr1(0x0800, ((bank8k | 2) & bank_and) | bank_or);
        setchr1(0x0C00, ((bank8k | 3) & bank_and) | bank_or);
        setchr1(0x1000, ((bank8k | 4) & bank_and) | bank_or);
        setchr1(0x1400, ((bank8k | 5) & bank_and) | bank_or);
        setchr1(0x1800, ((bank8k | 6) & bank_and) | bank_or);
        setchr1(0x1C00, ((bank8k | 7) & bank_and) | bank_or);
    }
    else if (coolt4_mapper == MAPPER_MMC1)
    {
        uint8_t chr_mode = (MMC3_reg[3] >> 2) & 1;
        if (chr_mode == 0)
        {
            //8K mode
            uint8_t bank8k = (MMC3_reg[0] << 2) & 0xF8;
            setchr1(0x0000, ((bank8k | 0) & bank_and) | bank_or);
            setchr1(0x0400, ((bank8k | 1) & bank_and) | bank_or);
            setchr1(0x0800, ((bank8k | 2) & bank_and) | bank_or);
            setchr1(0x0C00, ((bank8k | 3) & bank_and) | bank_or);
            setchr1(0x1000, ((bank8k | 4) & bank_and) | bank_or);
            setchr1(0x1400, ((bank8k | 5) & bank_and) | bank_or);
            setchr1(0x1800, ((bank8k | 6) & bank_and) | bank_or);
            setchr1(0x1C00, ((bank8k | 7) & bank_and) | bank_or);
        }
        else
        {
            //4K
            uint8_t chr0 = MMC3_reg[0] << 2;
            setchr1(0x0000, ((chr0 | 0) & bank_and) | bank_or);
            setchr1(0x0400, ((chr0 | 1) & bank_and) | bank_or);
            setchr1(0x0800, ((chr0 | 2) & bank_and) | bank_or);
            setchr1(0x0C00, ((chr0 | 3) & bank_and) | bank_or);
            uint8_t chr1 = MMC3_reg[1] << 2;
            setchr1(0x1000, ((chr1 | 0) & bank_and) | bank_or);
            setchr1(0x1400, ((chr1 | 1) & bank_and) | bank_or);
            setchr1(0x1800, ((chr1 | 2) & bank_and) | bank_or);
            setchr1(0x1C00, ((chr1 | 3) & bank_and) | bank_or);
        }
    }
}

static void CoolT4_MirroringUpdate(void)
{
    uint8_t coolt4_mapper = CoolT4_reg1 & 7;
    uint8_t coolt4_ex_mirroring = (CoolT4_reg1 >> 3) & 1;
    if (coolt4_mapper == MAPPER_MMC1)
    {
        if((uint8_t)(MMC3_mirroring & 3) == 0)
            setmirror(MI_0);
        if ((uint8_t)(MMC3_mirroring & 3) == 1)
            setmirror(MI_1);
        if ((uint8_t)(MMC3_mirroring & 3) == 2)
            setmirror(MI_V);
        if ((uint8_t)(MMC3_mirroring & 3) == 3)
            setmirror(MI_H);
    }
    else if (coolt4_ex_mirroring == 0)
    {
        if ((uint8_t)(MMC3_mirroring & 1) == 0)
            setmirror(MI_V);
        else
            setmirror(MI_H);
    }
    else
    {
        if ((uint8_t)(MMC3_mirroring & 3) == 0)
            setmirror(MI_V);
        if ((uint8_t)(MMC3_mirroring & 3) == 1)
            setmirror(MI_H);
        if ((uint8_t)(MMC3_mirroring & 3) == 2)
            setmirror(MI_0);
        if ((uint8_t)(MMC3_mirroring & 3) == 3)
            setmirror(MI_1);
    }
}

static DECLFW(CoolT4_Write_8000_FFFF)
{
    uint8_t coolt4_write = CoolT4_reg1 & 0x20;
    if (coolt4_write)
    {
        if (CoolT4_flash_cmd == 0 && (uint32_t)(A & 0xFFF) == 0xAAA && V == 0xAA)
            CoolT4_flash_cmd = 1;
        else if (CoolT4_flash_cmd == 1 && (uint32_t)(A & 0xFFF) == 0x555 && V == 0x55)
            CoolT4_flash_cmd = 2;
        else if (CoolT4_flash_cmd == 2 && (uint32_t)(A & 0xFFF) == 0xAAA && V == 0x90)
        {
            CoolT4_flash_cmd = 0;
            CoolT4_flash_factory = 1;
        } else if (V == 0xF0)
            CoolT4_flash_factory = 0;
    }

    uint8_t coolt4_mapper = CoolT4_reg1 & 7;

    if (coolt4_mapper == MAPPER_MMC3) //mmc3
    {
        switch (A & 0xE001)
        {
        case 0x8000:
            MMC3_cmd = V;
            CoolT4_PgmUpdate();
            CoolT4_ChrUpdate();
            break;

        case 0x8001:
            MMC3_reg[MMC3_cmd & 0x0F] = V;
            CoolT4_PgmUpdate();
            CoolT4_ChrUpdate();
            break;

        case 0xA000:
            MMC3_mirroring = V & 3;
            CoolT4_MirroringUpdate();
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
    else if (coolt4_mapper == MAPPER_MMC1)
    {
        if (V & 0x80)
        {
            MMC3_cmd = 0x20;
            MMC3_reg[0] = 0x03;
        }
        else
        {
            MMC3_cmd = (MMC3_cmd >> 1) | ((V & 1) << 5);
            if ((uint8_t)(MMC3_cmd & 1) != 0)
            {
                switch (A & 0xE000)
                {
                case 0x8000:
                    MMC3_mirroring = (MMC3_cmd >> 1) & 3;
                    MMC3_reg[3] = (MMC3_cmd >> 3) & 7;
                    break;
                case 0xA000:
                    MMC3_reg[0] = MMC3_cmd >> 1;
                    break;
                case 0xC000:
                    MMC3_reg[1] = MMC3_cmd >> 1;
                    break;
                case 0xE000:
                    MMC3_reg[6] = MMC3_cmd >> 1;
                    break;
                }
                MMC3_cmd = 0x20;
            }
        }
        CoolT4_PgmUpdate();
        CoolT4_ChrUpdate();
        CoolT4_MirroringUpdate();
    }
    else if (coolt4_mapper == MAPPER_AOROM) //aorom
    {
        MMC3_reg[6] = V & 0x0F;
        MMC3_mirroring = ((V >> 4) & 1) == 0 ? 2 : 3;
        CoolT4_PgmUpdate();
        CoolT4_MirroringUpdate();
    }
    else if (coolt4_mapper == MAPPER_UNROM) //unrom
    {
        MMC3_reg[6] = V;
        CoolT4_PgmUpdate();
        CoolT4_ChrUpdate();
    }
    else if (coolt4_mapper == MAPPER_UNROM512) //unrom-512
    {
        MMC3_reg[6] = V & 0x1F;
        MMC3_reg[0] = (V >> 5) & 0x3;
        uint8_t coolt4_ex_mirroring = (CoolT4_reg1 >> 3) & 1;
        if (coolt4_ex_mirroring)
            MMC3_mirroring = ((V >> 7) & 1) == 0 ? 2 : 3;
        CoolT4_PgmUpdate();
        CoolT4_ChrUpdate();
        CoolT4_MirroringUpdate();
    }
    else if (coolt4_mapper == MAPPER_COLOR_DREAMS)
    {
        MMC3_reg[6] = V & 0x3;
        MMC3_reg[0] = (V >> 4) & 0xF;
        CoolT4_PgmUpdate();
        CoolT4_ChrUpdate();
    }
    else
        //CNROM
    {
        MMC3_reg[0] = V & 0xF;
        CoolT4_ChrUpdate();
    }
}

static const uint8_t s_factory_info[8] = {0xFA, 0x51, 'F', 'C', 'E', 'U', 'X', 0};

static uint8 CoolT4_Read_8000_FFFF(uint32 A)
{
    if (CoolT4_flash_factory == 1)
        return s_factory_info[A & 7];
    else
        return CartBR(A);
}

static DECLFW(CoolT4_Write_4C00)
{
    if ((uint8_t)(A & 7) == 1)
        CoolT4_reg1 = V;
    else if ((uint8_t)(A & 7) == 2)
        CoolT4_reg2 = V;
    else if ((uint8_t)(A & 7) == 3)
        CoolT4_reg3 = V;
    else if ((uint8_t)(A & 7) == 4)
        CoolT4_reg4 = V;
    else if ((uint8_t)(A & 7) == 5)
        CoolT4_reg5 = V;

    uint8_t coolt4_sram_page = (CoolT4_reg5 >> 2) & 0x03;
    setprg8r(0x10, 0x6000, coolt4_sram_page);

    CoolT4_PgmUpdate();
    CoolT4_ChrUpdate();
    CoolT4_MirroringUpdate();
}

static void COOLT4_Power(void)
{
    MMC3_cmd = 0;
    MMC3_mirroring = 0;
    IRQCount = 0;
    IRQReload = 0;
    IRQLatch = 0;
    IRQa = 0;

    CoolT4_reg1 = 0;
    CoolT4_reg2 = 0x07;
    CoolT4_reg3 = 0;
    CoolT4_reg4 = 0;
    CoolT4_reg5 = 0;

    SetWriteHandler(0x4C00, 0x4fff, CoolT4_Write_4C00);
    SetWriteHandler(0x8000, 0xFFFF, CoolT4_Write_8000_FFFF);
    SetReadHandler(0x8000, 0xFFFF, CoolT4_Read_8000_FFFF);

    SetWriteHandler(0x6000, 0x7FFF, CartBW);
    SetReadHandler(0x6000, 0x7FFF, CartBR);
    setprg8r(0x10, 0x6000, 0);

    CoolT4_flash_cmd = 0;
    CoolT4_flash_factory = 0;

    CoolT4_PgmUpdate();
    CoolT4_ChrUpdate();
    CoolT4_MirroringUpdate();
}

static void COOLT4_Close(void)
{}

static void CoolT4_hb(void)
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


void COOLT4_Init(CartInfo *info)
{
    info->Power = COOLT4_Power;
    info->Close = COOLT4_Close;
    info->Reset = COOLT4_Power;

    size_t wram_size = 32 * 1024;
    WRAM = (uint8*)FCEU_gmalloc(wram_size);
    SetupCartPRGMapping(0x10, WRAM, wram_size, 1);
    AddExState(WRAM, wram_size, 0, "WRAM");

    AddExState(CoolXLite_StateRegs, ~0, 0, 0);
    GameHBIRQHook = CoolT4_hb;

    info->battery = 1;
    info->addSaveGameBuf(WRAM, wram_size);
}
