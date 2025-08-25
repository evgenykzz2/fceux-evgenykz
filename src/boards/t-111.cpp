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

//EK-303 3-in-1 Battletoads, Contra, DuckTales
// Address Latch:
//   XXXX LXMH  : Lock, Mode(0=unrom; 1=aorom); H - high address


#include "mapinc.h"

static uint8 ADDR_LATCH;
static uint8 DATA_LATCH;


static SFORMAT T111_StateRegs[] =
{
	{ &ADDR_LATCH, 1, "ADDR_LATCH" },
	{ &DATA_LATCH, 1, "DATA_LATCH" },
	{ 0 }
};

static void T111_Sync()
{
	if ((uint8_t)(ADDR_LATCH & 0x02) != 0)
	{
		//AOROM
		setprg32(0x8000, (DATA_LATCH & 0x07) | 0x08);	//Battletoads at hight part
		setmirror(DATA_LATCH & 0x10 ? MI_1 : MI_0);
	}
	else
	{
		//UNROM
		setprg16(0x8000, ((ADDR_LATCH & 1) << 3) | (DATA_LATCH & 0x07));
		setprg16(0xC000, ((ADDR_LATCH & 1) << 3) | 7);
		setmirror(MI_V);
	}
}

static DECLFW(T111_Write)
{
	if ((uint8_t)(ADDR_LATCH & 0x08) == 0)
		ADDR_LATCH = A & 0x0F;
	DATA_LATCH = V;
	T111_Sync();
}

static void T111_Power(void)
{
	ADDR_LATCH = 0;
	DATA_LATCH = 0;
	SetWriteHandler(0x8000, 0xFFFF, T111_Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	T111_Sync();
	setchr8(0);
}

static void T111_Close(void)
{}

void T111_Init(CartInfo* info)
{
	info->Power = T111_Power;
	info->Close = T111_Close;
	info->Reset = T111_Power;

	AddExState(T111_StateRegs, ~0, 0, 0);
}
