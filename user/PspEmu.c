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

// Wait you mean its based on adrenaline code?
// well uh, yes, but its also based on chovy-sign code ;)

#include <vitasdk.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <taihen.h>

#include "crypto/kirk_engine.h"
#include "PspEmu.h"
#include "PspNpDrm.h"

#define IS_RIF_PATH(x) (strncmp(x, "ms0:PSP/LICENSE/", 16) == 0 || strncmp(x, "ms0:/PSP/LICENSE/", 17) == 0)

int (* ScePspemuConvertAddress)(uint32_t addr, int mode, uint32_t cache_size);
int (* ScePspemuWritebackCache)(void *addr, int size);

static SceUID sceIoOpenHook;
static SceUID sceIoGetstatHook;

static tai_hook_ref_t sceIoOpenRef;
static tai_hook_ref_t sceIoGetstatRef;

static int patched_npdrm_prx = 0;

uint32_t text_addr, text_size, data_addr, data_size;


void get_functions() {
	ScePspemuConvertAddress = (void *)text_addr + 0x6364 + 0x1;
	ScePspemuWritebackCache = (void *)text_addr + 0x6490 + 0x1;
}

void nop_func(uintptr_t addr){
	
	//sceClibPrintf("[NPDRM_PATCH] Nopping : %p\n", addr);
	char *opcode = (char *)ScePspemuConvertAddress(addr, SCE_PSPEMU_CACHE_INVALIDATE, 0x8);
	
	//sceClibPrintf("[NPDRM_PATCH] prev instruction: %llx\n", *(uint32_t*)opcode);
	
	*(uint64_t*) opcode = 0x3e0000824020000ull; // li $v0 0, jr $ra
	
	//sceClibPrintf("[NPDRM_PATCH] new instruction: %llx\n", *(uint32_t*)opcode);
	
	ScePspemuWritebackCache(opcode, 0x8);
	
}

void nop(uintptr_t addr){
	
	//sceClibPrintf("[NPDRM_PATCH] Nopping : %p\n", addr);
	char *opcode = (char *)ScePspemuConvertAddress(addr, SCE_PSPEMU_CACHE_INVALIDATE, 0x4);
	
	//sceClibPrintf("[NPDRM_PATCH] prev instruction: %x\n", *(uint32_t*)opcode);
	
	memset(opcode, 0x00, 0x4);

	//sceClibPrintf("[NPDRM_PATCH] new instruction: %x\n", *(uint32_t*)opcode);
	
	ScePspemuWritebackCache(opcode, 0x4);
}

uintptr_t find_npdrm_key(){
	
	
	uintptr_t addr = 0x88000000;
	size_t sz = (32 * 1024 * 1024);
	
	char *m = (char *)ScePspemuConvertAddress(addr, SCE_PSPEMU_CACHE_NONE, sz);
	
	//sceClibPrintf("[NPDRM_PATCH] Searching for npdrm rif ecdsa key in memory ...\n");
	int found = 0;
	
	char* end = m + sz;
	char* ptr = m;
	while(!found && ptr < end){
		if(memcmp(ptr++, PSP_RIF_ECDSA, sizeof(PSP_RIF_ECDSA)) == 0)
			found = 1;
	}
	
	int offset = (0x88000000 + (ptr - m));
	
	//sceClibPrintf("[NPDRM_PATCH] Found npdrm key at %p\n", offset);
	return offset;
	
}

void patch_npdrm_prx(){
	if(!patched_npdrm_prx){
		uintptr_t addr = find_npdrm_key(); // 0x880ef7c1
		nop_func(addr - 0x33D9); // 0x880ec3e8
		nop_func(addr - 0x3341); // 0x880ec480
		patched_npdrm_prx = 1;
	}
}

void handle_rif(const char** file){
	//sceClibPrintf("[RIFCHECK] Handling: %s\n", *file);
	const char* nodrm_rif = "ux0:/temp/pspemu/nodrm.rif";
	
	char contentId[0x100];
	memset(contentId, 0x00, sizeof(contentId));
	
	int cnt = strlen(*file);
	strncpy(contentId, *file + (cnt - 40), 36);
	
	//sceClibPrintf("[RIFCHECK] Searching for drm file of %s\n", contentId);
	
	patch_npdrm_prx();	
	
	PspRifState state = sceNpDrmCheckRifState(contentId, *file);
	
	if(state == VALID_RIF){ // there is already a rif for this game and its valid, so just use that
		//sceClibPrintf("[RIFCHECK] Valid rif already exists!\n");
		return;
	}
	
	if(state == OFFICAL_INVALID) { // this is an offical rif, but its not for this game or account
								   // generate a new rif in a temporary folder, and use that
								   // as do not want to overwrite any legitimate rif files
		//sceClibPrintf("[RIFCHECK] Valid rif exist but its not for our account.. fake rif at temporary location!\n");
		sceNpDrmGenerateRif(contentId, nodrm_rif);
		*file = nodrm_rif;
	}
	
	if(state == NOPSPEMUDRM_INVALID) { // this is a nopspemudrm rif but for another account
									   // so must regenerate this rif. and its fine to overwrite the original
									   // because it is not an offical rif.
		//sceClibPrintf("[RIFCHECK] Valid rif not exist (or is for another account) generating fake rif!\n");
		sceIoMkdir("ms0:/PSP/LICENSE", 0777);
		sceNpDrmGenerateRif(contentId, *file);		
	}
}

// popscore stats license, its very rude
static SceUID sceIoGetstatPatched(const char* file, SceIoStat* stat) {
	int ret = TAI_CONTINUE(SceUID, sceIoGetstatRef, file, stat);	
	
	if( file != NULL && ret < 0 && IS_RIF_PATH(file)) {
		// fake it existing
		//sceClibPrintf("[STAT] faking sceIoGetstat success to: %s\n", file);
		memset(stat, 0x00, sizeof(SceIoStat));
		stat->st_mode = 0x2186;
		stat->st_attr = 0x0;
		stat->st_size = (SceOff)sizeof(PspRif);
		return 0;
	}
	
	return ret;
}

static SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode) {
	
	if(file != NULL && IS_RIF_PATH(file)){
		handle_rif(&file);
	}
	
	return TAI_CONTINUE(SceUID, sceIoOpenRef, file, flags, mode);
}

int pspemu_module_start(tai_module_info_t tai_info) {
	SceKernelModuleInfo mod_info;
	mod_info.size = sizeof(SceKernelModuleInfo);
	int ret = sceKernelGetModuleInfo(tai_info.modid, &mod_info);
	if (ret < 0){
		return SCE_KERNEL_START_NO_RESIDENT;
	}
	
	// Addresses
	text_addr = (uint32_t)mod_info.segments[0].vaddr;
	text_size = (uint32_t)mod_info.segments[0].memsz;

	data_addr = (uint32_t)mod_info.segments[1].vaddr;
	data_size = (uint32_t)mod_info.segments[1].memsz;

	// Get PSPEMU functions
	get_functions();

	kirk_init();
		
	sceIoOpenHook = taiHookFunctionImport(&sceIoOpenRef, "ScePspemu", 0xCAE9ACE6, 0x6C60AC61, sceIoOpenPatched);
	sceIoGetstatHook = taiHookFunctionImport(&sceIoGetstatRef, "ScePspemu", 0xCAE9ACE6, 0xBCA5B623, sceIoGetstatPatched);
	
	return SCE_KERNEL_START_SUCCESS;
}

int  pspemu_module_stop() {
	if(sceIoOpenHook >= 0) taiHookRelease(sceIoOpenHook, sceIoOpenRef);
	if(sceIoGetstatHook >= 0) taiHookRelease(sceIoGetstatHook, sceIoGetstatRef);
	
	return SCE_KERNEL_STOP_SUCCESS;
}