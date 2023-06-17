/*
	Adrenaline
	Copyright (C) 2016-2017, TheFloW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PSP_MAIN_H
#define PSP_MAIN_H 1

#define SCE_PSPEMU_CACHE_NONE 0x1
#define SCE_PSPEMU_CACHE_INVALIDATE 0x2

extern int (* ScePspemuConvertAddress)(uint32_t addr, int mode, uint32_t cache_size);
extern int (* ScePspemuWritebackCache)(void *addr, int size);


int pspemu_module_start(tai_module_info_t tai_info);
int pspemu_module_stop();

#endif