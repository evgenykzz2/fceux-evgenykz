/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022 EvgenyKz
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
 * AA6023A and AA6023B ASICs
 * ppu
 * 0---0000-03ff
 * a---0400-07ff
 * 1---0800-0bff
 * b---0c00-0fff
 * 2---1000-13ff
 * 3---1400-17ff
 * 4---1800-1bff
 * 5---1c00-1fff
 * cpu
 * 6---8000-9fff
 * 7---a000-bfff
 * 8---c000-dfff
 * 9---e000-ffff
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
static uint8 Mirroring4x;
static uint8 ExtBank;
//static uint8 EXPREGS[8];

static SFORMAT MMC3_StateRegs[] =
{
	//{ DRegBuf, 8, "REGS" },
	{ &MMC3_cmd, 1, "CMD" },
	{ &MMC3_mirroring, 1, "A000" },
	//{ &A001B, 1, "A001" },
	{ &IRQReload, 1, "IRQR" },
	{ &IRQCount, 1, "IRQC" },
	{ &IRQLatch, 1, "IRQL" },
	{ &IRQa, 1, "IRQA" },
	{ &Mirroring4x, 1, "Mirroring4x" },
	{ &ExtBank, 1, "ExtBank" },
	{ 0 }
};

static DECLFW(TomasC4_Write)
{
	switch (A & 0xE001)
	{
	case 0x8000:
		MMC3_cmd = V;
		break;

	case 0x8001:

		switch (MMC3_cmd)
		{
		case 0x0:
			if (ExtBank)
				setchr1(0x000, V);
			else
				setchr2(0x000, V >> 1);
			break;
		case 0xA:
			if (ExtBank)
				setchr1(0x400, V);
			break;
		case 0x1:
			if (ExtBank)
				setchr1(0x800, V);
			else
				setchr2(0x800, V >> 1);
			break;
		case 0xB:
			if (ExtBank)
				setchr1(0xC00, V);
			break;
		case 0x2:
			setchr1(0x1000, V);
			break;
		case 0x3:
			setchr1(0x1400, V);
			break;
		case 0x4:
			setchr1(0x1800, V);
			break;
		case 0x5:
			setchr1(0x1C00, V);
			break;
		case 0x6:
			setprg8(0x8000, V);
			break;
		case 0x7:
			setprg8(0xA000, V);
			break;
		case 0x8:
			if (ExtBank)
				setprg8(0xC000, V);
			break;
		case 0x9:
			if (ExtBank)
				setprg8(0xE000, V);
			break;
		default:
			break;
		}
		break;

	case 0xA000:
		MMC3_mirroring = V;
		if (Mirroring4x == 0)
		{
			setmirror((V & 1) ^ 1);
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
		break;
	default:
		break;
	}
}

static DECLFW(TomasC4_IRQWrite)
{
	switch (A & 0xE001)
	{
	case 0xC000: IRQLatch = V; break;
	case 0xC001: IRQReload = 1; break;
	case 0xE000: X6502_IRQEnd(FCEU_IQEXT); IRQa = 0; break;
	case 0xE001: IRQa = 1; break;
	}
}

static void TomasC4_Power(void)
{
	MMC3_cmd = 0;
	MMC3_mirroring = 0;
	IRQCount = 0;
	IRQReload = 0;
	IRQLatch = 0;
	IRQa = 0;
	Mirroring4x = 0;
	ExtBank = 1;
	SetWriteHandler(0x8000, 0xBFFF, TomasC4_Write);
	SetWriteHandler(0xC000, 0xFFFF, TomasC4_IRQWrite);
	SetReadHandler(0x8000, 0xFFFF, CartBR);

	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	setprg8r(0x10, 0x6000, 0);

	setprg8(0xC000, 0xFE);
	setprg8(0xE000, 0xFF);
}

static void TomasC4_Close(void)
{}

static void TomasC4_hb(void)
{
	int count = IRQCount;
	if (!count || IRQReload) {
		IRQCount = IRQLatch;
		IRQReload = 0;
	}
	else
		IRQCount--;
	if (count && !IRQCount) {
		if (IRQa) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

void TomasC4_Init(CartInfo* info)
{
	info->Power = TomasC4_Power;
	info->Close = TomasC4_Close;
	info->Reset = TomasC4_Power;

	WRAM = (uint8*)FCEU_gmalloc(8*1024);
	SetupCartPRGMapping(0x10, WRAM, 8 * 1024, 1);
	AddExState(WRAM, 8 * 1024, 0, "WRAM");

	AddExState(MMC3_StateRegs, ~0, 0, 0);
	GameHBIRQHook = TomasC4_hb;
}

static void Mmc3x4Mirroring_Power(void)
{
	MMC3_cmd = 0;
	MMC3_mirroring = 0;
	IRQCount = 0;
	IRQReload = 0;
	IRQLatch = 0;
	IRQa = 0;
	Mirroring4x = 1;
	ExtBank = 0;
	SetWriteHandler(0x8000, 0xBFFF, TomasC4_Write);
	SetWriteHandler(0xC000, 0xFFFF, TomasC4_IRQWrite);
	SetReadHandler(0x8000, 0xFFFF, CartBR);

	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	setprg8r(0x10, 0x6000, 0);

	setprg8(0xC000, 0xFE);
	setprg8(0xE000, 0xFF);
}


void Mmc3x4Mirroring_Init(CartInfo* info)
{
	info->Power = Mmc3x4Mirroring_Power;
	info->Close = TomasC4_Close;
	info->Reset = Mmc3x4Mirroring_Power;

	WRAM = (uint8*)FCEU_gmalloc(8 * 1024);
	SetupCartPRGMapping(0x10, WRAM, 8 * 1024, 1);
	AddExState(WRAM, 8 * 1024, 0, "WRAM");

	AddExState(MMC3_StateRegs, ~0, 0, 0);
	GameHBIRQHook = TomasC4_hb;
}
