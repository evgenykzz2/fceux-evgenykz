/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025 EvgenyKz
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

static uint8* WRAM;
static uint8 MMC3_cmd;
static uint8 MMC3_mirroring;
static uint8 IRQCount;
static uint8 IRQReload;
static uint8 IRQLatch;
static uint8 IRQa;
static uint16 IRQprescaler;
static uint8 MMC3_reg[16];

static SFORMAT RetronicaNg2_StateRegs[] =
{
	{ &MMC3_cmd, 1, "MMC3_cmd" },
	{ &MMC3_mirroring, 1, "MMC3_mirroring" },
	{ &IRQCount, 1, "IRQCount" },
	{ &IRQReload, 1, "IRQReload" },
	{ &IRQLatch, 1, "IRQLatch" },
	{ &IRQa, 1, "IRQa" },
	{ &IRQprescaler, 2, "IRQprescaler" },
	{ MMC3_reg, 16, "MMC3_reg" },
	{ 0 }
};

static void T4Vrc_SyncPgm()
{
    uint8_t pgm_mode = (MMC3_cmd >> 6) & 1;
    if (pgm_mode == 0)
    {
        setprg8(0x8000, MMC3_reg[6]);
        setprg8(0xA000, MMC3_reg[7]);
        setprg8(0xC000, MMC3_reg[8]);
        setprg8(0xE000, MMC3_reg[9]);
    }
    else
    {
		setprg8(0x8000, MMC3_reg[8]);
        setprg8(0xA000, MMC3_reg[7]);
        setprg8(0xC000, MMC3_reg[6]);
        setprg8(0xE000, MMC3_reg[9]);
    }
}

static void T4Vrc_SyncChr()
{
	uint8_t chr_mode = (MMC3_cmd >> 7) & 1;
    if (chr_mode == 0)
    {
        setchr1(0x0000, MMC3_reg[0x0]);
        setchr1(0x0400, MMC3_reg[0xA]);
        setchr1(0x0800, MMC3_reg[0x1]);
        setchr1(0x0C00, MMC3_reg[0xB]);
        setchr1(0x1000, MMC3_reg[0x2]);
        setchr1(0x1400, MMC3_reg[0x3]);
        setchr1(0x1800, MMC3_reg[0x4]);
        setchr1(0x1C00, MMC3_reg[0x5]);
    }
    else
    {
        setchr1(0x0000, MMC3_reg[0x2]);
        setchr1(0x0400, MMC3_reg[0x3]);
        setchr1(0x0800, MMC3_reg[0x4]);
        setchr1(0x0C00, MMC3_reg[0x5]);
        setchr1(0x1000, MMC3_reg[0x0]);
        setchr1(0x1400, MMC3_reg[0xA]);
        setchr1(0x1800, MMC3_reg[0x1]);
        setchr1(0x1C00, MMC3_reg[0xB]);
    }
}

static void T4Vrc_SyncMirroring()
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

static DECLFW(T4Vrc_Write)
{
    switch (A & 0xE001)
    {
    case 0x8000:
        MMC3_cmd = V;
		T4Vrc_SyncPgm();
		T4Vrc_SyncChr();
        break;

    case 0x8001:
        MMC3_reg[MMC3_cmd & 0x0F] = V;
		T4Vrc_SyncPgm();
		T4Vrc_SyncChr();
        break;

    case 0xA000:
        MMC3_mirroring = V & 3;
        T4Vrc_SyncMirroring();
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

static void StateRestore(int version)
{
	T4Vrc_SyncPgm();
	T4Vrc_SyncChr();
	T4Vrc_SyncMirroring();
}

static void T4Vrc_Power(void)
{
	MMC3_cmd = 0;
	MMC3_mirroring = 0;
	IRQCount = 0;
	IRQReload = 0;
	IRQLatch = 0;
	IRQa = 0;
	IRQprescaler = 0;

    memset(MMC3_reg, 0, sizeof(MMC3_reg));
    MMC3_reg[8] = 0xFE;
    MMC3_reg[9] = 0xFF;

	SetWriteHandler(0x8000, 0xFFFF, T4Vrc_Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
    T4Vrc_SyncPgm();
    T4Vrc_SyncChr();
    T4Vrc_SyncMirroring();
}

static void T4Vrc_Close(void)
{}

void T4VcrHook(int a)
{
#define LCYCS 341
	IRQprescaler += a * 3;
	if (IRQprescaler >= LCYCS)
	{
		while (IRQprescaler >= LCYCS)
		{
			IRQprescaler -= LCYCS;
			IRQCount--;
			if (IRQCount == 0 || IRQReload)
			{
				IRQCount = IRQLatch;
				IRQReload = 0;
			}
			if (IRQCount == 0 && IRQa)
			{
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

void T4Vrc_Init(CartInfo* info)
{
	info->Power = T4Vrc_Power;
	info->Close = T4Vrc_Close;
	info->Reset = T4Vrc_Power;
    MapIRQHook = T4VcrHook;
	GameStateRestore = StateRestore;

	AddExState(RetronicaNg2_StateRegs, ~0, 0, 0);
}
