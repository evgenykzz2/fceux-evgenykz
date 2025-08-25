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
static uint8 LATCH;

/*
LATCH [CCCC CPPP] Pgm Chr
board: 128KB PGM + 256KB CHR
       logic = 7432 + 74273

*/

static SFORMAT RetronicaRobo3_StateRegs[] =
{
	{ &LATCH, 1, "LATCH" },
	{ 0 }
};

static void RetronicaRobo3_Sync()
{
	setprg16(0x8000, LATCH & 7);
	setprg16(0xC000, 7);
	setchr8(LATCH >> 3);
}

static DECLFW(RetronicaRobo3_Write)
{
	LATCH = V;
	RetronicaRobo3_Sync();
}

static void StateRestore(int version) {
	RetronicaRobo3_Sync();
}

static void RetronicaRobo3_Power(void)
{
	LATCH = 0;
	SetWriteHandler(0x8000, 0xFFFF, RetronicaRobo3_Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	RetronicaRobo3_Sync();
}

static void RetronicaRobo3_Close(void)
{}

void RetronicaRobo3_Init(CartInfo* info)
{
	info->Power = RetronicaRobo3_Power;
	info->Close = RetronicaRobo3_Close;
	info->Reset = RetronicaRobo3_Power;
	GameStateRestore = StateRestore;

	//WRAM = (uint8*)FCEU_gmalloc(8 * 1024);
	//SetupCartPRGMapping(0x10, WRAM, 8 * 1024, 1);
	//AddExState(WRAM, 8 * 1024, 0, "WRAM");

	AddExState(RetronicaRobo3_StateRegs, ~0, 0, 0);
}
