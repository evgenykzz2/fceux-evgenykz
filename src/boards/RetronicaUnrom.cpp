/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023 EvgenyKz
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
static uint8 REG_5000;
static uint8 UNROM_LATCH;

static SFORMAT RetronicaUnrom_StateRegs[] =
{
	{ &REG_5000, 1, "REG_5000" },
	{ &UNROM_LATCH, 1, "UNROM_LATCH" },
	{ 0 }
};

static void RetronicaUnrom_Sync()
{
	setprg16(0x8000, ((REG_5000 & 3) << 3) | (UNROM_LATCH & 7));
	setprg16(0xC000, ((REG_5000 & 3) << 3) | (              7));
	setmirror(((REG_5000 >> 2) & 1) ^ 1);
}

static DECLFW(RetronicaUnrom_Write)
{
	if ((uint8_t)(REG_5000 & 0x08) != 0)
		UNROM_LATCH = V;
	RetronicaUnrom_Sync();
}

static DECLFW(RetronicaUnrom_Write5000)
{
	if ( (uint8_t)(REG_5000 & 0x08) == 0 )
		REG_5000 = V;
	RetronicaUnrom_Sync();
}

static DECLFW(RetronicaUnrom_Write6000)
{
	if ((uint8_t)(REG_5000 & 0x08) == 0)
		UNROM_LATCH = V;
	RetronicaUnrom_Sync();
}

static void RetronicaUnrom_Power(void)
{
	REG_5000 = 0;
	UNROM_LATCH = 0;
	SetWriteHandler(0x5000, 0x5FFF, RetronicaUnrom_Write5000);
	SetWriteHandler(0x6000, 0x6FFF, RetronicaUnrom_Write6000);
	SetWriteHandler(0x8000, 0xFFFF, RetronicaUnrom_Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	RetronicaUnrom_Sync();
	setchr8(0);
}

static void RetronicaUnrom_Close(void)
{}

void RetronicaUnrom_Init(CartInfo* info)
{
	info->Power = RetronicaUnrom_Power;
	info->Close = RetronicaUnrom_Close;
	info->Reset = RetronicaUnrom_Power;

	//WRAM = (uint8*)FCEU_gmalloc(8 * 1024);
	//SetupCartPRGMapping(0x10, WRAM, 8 * 1024, 1);
	//AddExState(WRAM, 8 * 1024, 0, "WRAM");

	AddExState(RetronicaUnrom_StateRegs, ~0, 0, 0);
}
