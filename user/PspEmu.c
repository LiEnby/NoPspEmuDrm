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
#include "Log.h"
#include "PspEmu.h"
#include "PspNpDrm.h"
#include "Io.h"

#define IS_RIF_PATH(x) (strncmp(x, "ms0:PSP/LICENSE/", 16) == 0 || strncmp(x, "ms0:/PSP/LICENSE/", 17) == 0)

void* (*scePspemuConvertAddress)(uintptr_t addr, int mode, size_t cache_size);
int (*scePspemuWritebackCache)(void *addr, int size);

static SceUID sceIoOpenHook;
static SceUID sceIoGetstatHook;

static tai_hook_ref_t sceIoOpenRef;
static tai_hook_ref_t sceIoGetstatRef;

static uintptr_t npdrm_key_addr = 0;
static char last_opened_drm_file[0x1028];

void get_functions(uintptr_t addr) {
	scePspemuConvertAddress = (void*)addr + 0x6364 + 0x1;
	scePspemuWritebackCache = (void*)addr + 0x6490 + 0x1;
}

void nop_func_as_ret_0_mips(uintptr_t addr){
	/*
	* Writes the equivielent MIPS assembly code for "return 0;"
	* to the specified function address;
	*/	

	uint64_t* func_addr = scePspemuConvertAddress(addr, SCE_PSPEMU_CACHE_INVALIDATE, 0x8);
	*func_addr = 0x3e0000824020000ull; // li $v0 0
									   // jr $ra
									   
	scePspemuWritebackCache(func_addr, 0x8);
}


int check_npdrm_key_addr(int addr) {
	if(addr == 0) {
		LOG("[NOPSPEMUDRM_USER] npdrm_key_addr is nullptr\n", addr);
		return 0;
	}
	
	char *rif_key = (char *)scePspemuConvertAddress(addr, SCE_PSPEMU_CACHE_NONE, sizeof(PSP_RIF_ECDSA));
	
	if(memcmp(rif_key, PSP_RIF_ECDSA, sizeof(PSP_RIF_ECDSA)) == 0) {
		LOG("[NOPSPEMUDRM_USER] npdrm_key_addr %x is still correct.\n", addr);
		return 1;
	}
	
	LOG("[NOPSPEMUDRM_USER] npdrm_key_addr %x is no longer correct.\n", addr);
	return 0;
}

uintptr_t find_npdrm_key(){
	uintptr_t base_addr = 0x88000000;
	size_t sz = (32 * 1024 * 1024);
	
	LOG("[NOPSPEMUDRM_USER] locating npdrm rif ecdsa key ... ");
	
	char *mem = scePspemuConvertAddress(base_addr, SCE_PSPEMU_CACHE_NONE, sz);
	
	char* end = mem + sz;
	char* ptr = mem;
	while(ptr < end) {
		if(memcmp(ptr++, PSP_RIF_ECDSA, sizeof(PSP_RIF_ECDSA)) == 0) {
			break;
		}
	}
	
	uintptr_t npdrm_key_addr = (base_addr + (ptr - mem)) - 1;
	LOG("found at %x\n", npdrm_key_addr);
	
	return npdrm_key_addr;
	
}

void patch_npdrm_prx(){
	
	/*
	*	npdrm.prx loads in different location on each firmware and even depending on pops or psp mode
	*	this code will search for the npdrm ECDSA rif key, as it is at a constant location in the prx
	*	will then calculate the offsets to the verify rif and verify act.dat functions from there
	*
	*	- i used to use a boolean variable to determine if was patched already
	*	  however, it turns out some PSP games (e.g POWER STONE COLLETION) will reboot the PSP
	*	  so instead im only caching the memory search result (which is the "slow" part)
	*	  and patching the npdrm module every time .. its a bit of a hack but it should work.
	*/
	
	if(!check_npdrm_key_addr(npdrm_key_addr)) {
		npdrm_key_addr = find_npdrm_key(); // 3.60: 0x880ef7c1
	}
																
	uintptr_t sceNpDrmVerifyRifAddr = (npdrm_key_addr - 0x33D8); // 3.60: 0x880ec3e8 => sceNpDrmVerifyRif
	uintptr_t sceNpDrmVerifyActAddr = (npdrm_key_addr - 0x3340); // 3.60: 0x880ec480 => sceNpDrmVerifyAct

	LOG("[NOPSPEMUDRM_USER] == Patching NpDrm in PspEmu ==\n");
	
	LOG("[NOPSPEMUDRM_USER] npdrm_key_addr: %p\n", npdrm_key_addr);
	LOG("[NOPSPEMUDRM_USER] sceNpDrmVerifyRif: %p\n", sceNpDrmVerifyRifAddr);
	LOG("[NOPSPEMUDRM_USER] sceNpDrmVerifyAct: %p\n", sceNpDrmVerifyActAddr);

	
	nop_func_as_ret_0_mips(sceNpDrmVerifyRifAddr); 
	nop_func_as_ret_0_mips(sceNpDrmVerifyActAddr); 
}

void handle_rif(const char** file){
	const char* nodrm_rif = "ux0:/temp/pspemu/nodrm.rif";
	
	char content_id[0x30];
	memset(content_id, 0x00, sizeof(content_id));
	
	int cnt = strlen(*file);
	strncpy(content_id, *file + (cnt - 40), 36);
		
	patch_npdrm_prx();	
	
	PspRifState state = sceNpDrmCheckRifState(content_id, *file);
	
	if(state == VALID_RIF){ // there is already a rif for this game and its valid, so just use that
		LOG("[NOPSPEMUDRM_USER] valid rif found.\n");
		return;
	}
	
	if(state == OFFICAL_INVALID) { // this is an offical rif, but its not for this game or account
								   // generate a new rif in a temporary folder, and use that
								   // as do not want to overwrite any legitimate rif files
		LOG("[NOPSPEMUDRM_USER] invalid rif, state is OFFICAL_INVALID\n");
		sceIoMkdir("ux0:/temp/pspemu", 0777);
		sceNpDrmGenerateRif(content_id, nodrm_rif, last_opened_drm_file);
		*file = nodrm_rif;
	}
	
	if(state == NOPSPEMUDRM_INVALID) { // this is a nopspemudrm rif but for another account
									   // so must regenerate this rif. and its fine to overwrite the original
									   // because it is not an offical rif.
		LOG("[NOPSPEMUDRM_USER] invalid rif, state is NOPSPEMUDRM_INVALID\n");
		sceIoMkdir("ms0:/PSP/LICENSE", 0777);
		sceNpDrmGenerateRif(content_id, *file, last_opened_drm_file);		
	}
}

// popscore stats license, its very rude
static SceUID sceIoGetstatPatched(const char* file, SceIoStat* stat) {
	SceUID ret = TAI_CONTINUE(SceUID, sceIoGetstatRef, file, stat);	
	if( file != NULL && (ret < 0 && IS_RIF_PATH(file)) ) {
		LOG("[NOPSPEMUDRM_USER] rif_getstat: %s %x\n", file, stat);
		// fake it existing
		memset(stat, 0x00, sizeof(SceIoStat));
		stat->st_mode = 0x2186;
		stat->st_attr = 0x0;
		stat->st_size = (SceOff)sizeof(PspRif);
		return 0;
	}
	
	return ret;
}

static SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode) {
	char extension[0x10];

	if(file != NULL) {
		get_extension(file, extension, sizeof(extension));
		if( (strcasecmp(extension, ".EDAT") == 0 ) || 
			 strcasecmp(extension, ".PBP") == 0 ) {
			LOG("[NOPSPEMUDRM_USER] edat_or_pbp_open: %s %x %x\n", file, flags, mode);
			strncpy(last_opened_drm_file, file, sizeof(last_opened_drm_file));
		}
		if(file != NULL && IS_RIF_PATH(file)){
			LOG("[NOPSPEMUDRM_USER] rif_open: %s %x %x\n", file, flags, mode);
			handle_rif(&file);
		}	
	}

	
	return TAI_CONTINUE(SceUID, sceIoOpenRef, file, flags, mode);
}

int pspemu_module_start(tai_module_info_t tai_info) {
	SceKernelModuleInfo mod_info;
	mod_info.size = sizeof(SceKernelModuleInfo);
	
	SceUID ret = sceKernelGetModuleInfo(tai_info.modid, &mod_info);
	if (ret < 0){
		return SCE_KERNEL_START_NO_RESIDENT;
	}
	
	// Get PspEmu functions
	get_functions((uint32_t)mod_info.segments[0].vaddr);
	
	memset(last_opened_drm_file, 0x00, sizeof(last_opened_drm_file));

	kirk_init();
		
	sceIoOpenHook = taiHookFunctionImport(&sceIoOpenRef, 
											"ScePspemu", 
											0xCAE9ACE6, // SceIofilemgr
											0x6C60AC61, // sceIoOpen 
											sceIoOpenPatched);
											
	sceIoGetstatHook = taiHookFunctionImport(&sceIoGetstatRef,
											"ScePspemu", 
											0xCAE9ACE6, // SceIofilemgr
											0xBCA5B623, // sceIoGetstat
											sceIoGetstatPatched);
		
	return SCE_KERNEL_START_SUCCESS;
}

int pspemu_module_stop() {
	if(sceIoOpenHook >= 0) taiHookRelease(sceIoOpenHook, sceIoOpenRef);
	if(sceIoGetstatHook >= 0) taiHookRelease(sceIoGetstatHook, sceIoGetstatRef);
	
	return SCE_KERNEL_STOP_SUCCESS;
}