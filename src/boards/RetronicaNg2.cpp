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
static uint16 LATCH;

/*
LATCH [MC CCCC CPPP] Pgm Chr Mirroring
board: 128KB PGM + 512KB CHR + Mirroring (H/V)
       logic = 7432 + 74273 + 74161 + 157
*/

static SFORMAT RetronicaNg2_StateRegs[] =
{
	{ &LATCH, 2, "LATCH" },
	{ 0 }
};

static void RetronicaNg2_Sync()
{
	setprg16(0x8000, LATCH & 7);
	setprg16(0xC000, 7);
	setchr8( (LATCH >> 3) & 0x3F);
	setmirror( ((LATCH >> 9) & 1) ^ 1);
}

static DECLFW(RetronicaNg2_Write)
{
	LATCH = A;
	RetronicaNg2_Sync();
}

static void StateRestore(int version) {
	RetronicaNg2_Sync();
}

static void RetronicaNg2_Power(void)
{
	LATCH = 0;
	SetWriteHandler(0x8000, 0xFFFF, RetronicaNg2_Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	RetronicaNg2_Sync();
}

static void RetronicaNg2_Close(void)
{}

void RetronicaNg2_Init(CartInfo* info)
{
	info->Power = RetronicaNg2_Power;
	info->Close = RetronicaNg2_Close;
	info->Reset = RetronicaNg2_Power;
	GameStateRestore = StateRestore;

	AddExState(RetronicaNg2_StateRegs, ~0, 0, 0);
}
