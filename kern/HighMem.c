// NoPspEmuDrm 
// Created by Li, based on NoNpDrm kernel plugin

// highmem routines imported from Adrenaline

// below is the copyright notice for the original NoNpDrm :

/*
  NoNpDrm Plugin
  Copyright (C) 2017-2018, TheFloW

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

// below is the copyright notice for the original Adrenaline :

/*
  Adrenaline
  Copyright (C) 2016-2018, TheFloW

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

#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2kern/kernel/sysmem.h>

#include "Log.h"

static SceUID extra_1_blockid = -1;
static SceUID extra_2_blockid = -1;
static SceUID mem_hooks[4];

static tai_hook_ref_t ksceKernelAllocMemBlockRef;
static tai_hook_ref_t ksceKernelFreeMemBlockRef;
static tai_hook_ref_t ksceKernelUnmapMemBlockRef;
static tai_hook_ref_t SceGrabForDriver_E9C25A28_ref;

static SceUID ksceKernelAllocMemBlockPatched(const char *name, SceKernelMemBlockType type, int size, SceKernelAllocMemBlockKernelOpt *optp) {
	SceUID blockid = TAI_CONTINUE(SceUID, ksceKernelAllocMemBlockRef, name, type, size, optp);

	uint32_t addr;
	ksceKernelGetMemBlockBase(blockid, (void *)&addr);

	if (addr == 0x23000000) {
		extra_1_blockid = blockid;
	} else if (addr == 0x24000000) {
		extra_2_blockid = blockid;
	}

	return blockid;
}

static int ksceKernelFreeMemBlockPatched(SceUID uid) {
	if (uid == extra_1_blockid){
		log("%s: blocked releasing of extra_1\n", __func__);
		return 0;
	}

	int res = TAI_CONTINUE(int, ksceKernelFreeMemBlockRef, uid);

	if (uid == extra_2_blockid) {
		ksceKernelFreeMemBlock(extra_1_blockid);
		extra_1_blockid = -1;
		extra_2_blockid = -1;
		log("%s: released both extra_1 and extra_2\n", __func__);
	}

	return res;
}

static int ksceKernelUnmapMemBlockPatched(SceUID uid) {
	return 0;
}

static int SceGrabForDriver_E9C25A28_patched(int unk, uint32_t paddr) {
	log("%s: %d 0x%x\n", __func__, unk, paddr);

	if (unk == 2 && paddr == 0x21000001){
		log("%s: overriding address 0x%x to 0x%x\n", __func__, 0x21000001, 0x22000001);
		paddr = 0x22000001;
	}

	return TAI_CONTINUE(int, SceGrabForDriver_E9C25A28_ref, unk, paddr);
}

void init_highmem(){
	mem_hooks[0] = taiHookFunctionImportForKernel(KERNEL_PID, &ksceKernelAllocMemBlockRef, "SceCompat", 0x6F25E18A, 0xC94850C9, ksceKernelAllocMemBlockPatched);
	mem_hooks[1] = taiHookFunctionImportForKernel(KERNEL_PID, &ksceKernelFreeMemBlockRef, "SceCompat", 0x6F25E18A, 0x009E1C61, ksceKernelFreeMemBlockPatched);
	mem_hooks[2] = taiHookFunctionImportForKernel(KERNEL_PID, &ksceKernelUnmapMemBlockRef, "SceCompat", 0x6F25E18A, 0xFFCD9B60, ksceKernelUnmapMemBlockPatched);
	mem_hooks[3] = taiHookFunctionImportForKernel(KERNEL_PID, &SceGrabForDriver_E9C25A28_ref, "SceCompat", 0x81C54BED, 0xE9C25A28, SceGrabForDriver_E9C25A28_patched);
}

void term_highmem(){
	taiHookReleaseForKernel(mem_hooks[0], ksceKernelAllocMemBlockRef);
	taiHookReleaseForKernel(mem_hooks[1], ksceKernelFreeMemBlockRef);
	taiHookReleaseForKernel(mem_hooks[2], ksceKernelUnmapMemBlockRef);
	taiHookReleaseForKernel(mem_hooks[3], SceGrabForDriver_E9C25A28_ref);
}
