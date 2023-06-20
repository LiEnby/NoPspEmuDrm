// NoPspEmuDrm 
// Created by Li, based on NoNpDrm kernel plugin

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

#include <vitasdkkern.h>
#include <taihen.h>

// pull in my rif struct from user plugin 
#include "../user/PspNpDrm.h"

/*
*	Hook References
*/

// SceNpDrm Patches
static tai_hook_ref_t ksceNpDrmGetRifInfoRef;
static tai_hook_ref_t ksceNpDrmGetRifVitaKeyRef;
static tai_hook_ref_t ksceNpDrmGetRifPspKeyRef;

// __sce_ebootpbp patch
static tai_hook_ref_t ksceNpDrmEbootSigVerifyRef;
static tai_hook_ref_t ksceNpDrmPspEbootVerifyRef;

// SceCompat
static tai_hook_ref_t ksceNpDrmReadActDataRef;
static tai_hook_ref_t ksceSblAimgrIsDEXRef;
static tai_hook_ref_t sceCompatCheckPocketStationRef;

// internal functions
static tai_hook_ref_t npdrm_rif_verify_ecdsa_and_rsa_ref;
static tai_hook_ref_t npdrm_act_verify_ecdsa_and_rsa_ref;

/*
*	Hook variables
*/

// SceNpDrm patches
static SceUID ksceNpDrmGetRifInfoHook;
static SceUID ksceNpDrmGetRifVitaKeyHook;
static SceUID ksceNpDrmGetRifPspKeyHook;

// __sce_ebootpbp patch
static SceUID ksceNpDrmEbootSigVerifyHook;
static SceUID ksceNpDrmPspEbootVerifyHook;

// SceCompat
static SceUID ksceNpDrmReadActDataHook;
static SceUID ksceSblAimgrIsDEXHook;
static SceUID sceCompatCheckPocketStationHook;

// internal functions
static SceUID npdrm_rif_verify_ecdsa_and_rsa_hook; 
static SceUID npdrm_act_verify_ecdsa_and_rsa_hook; 


int is_offical_rif(PspRif* rif){
	for(int i = 0; i < 0x28; i++) {
		if(rif->ecdsaSig[i] != 0xff) {
			return 1;
		}
	}
	return 0;
}

static int return_0() {
	return 0;
}

static int return_1() {
	return 1;
}


static int ksceNpDrmGetRifInfoPatched(PspRif *psprif, int license_size, int mode, char *content_id, uint64_t *aid, uint16_t *license_version, uint8_t *license_flags, uint32_t *flags, uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time, uint64_t *flags2) {

	int ret = TAI_CONTINUE(int, ksceNpDrmGetRifInfoRef, psprif, license_size, mode, content_id, aid, license_version, license_flags, flags, sku_flag, start_time, expiration_time, flags2);

	// Bypass expiration time for PS Plus games
	if (start_time)
		*start_time = 0LL;
	if (expiration_time)
		*expiration_time = 0x7FFFFFFFFFFFFFFFLL;
	
	
	if (ret < 0 &&
		( (psprif != NULL &&       // check if is null
		psprif->version != -1)  // check is a psp rif
		|| strcmp(psprif->contentId, "JA0003-PCSC80018_00-POCKETSTATION001") == 0) )  // check is pocketstation
		{
		// bypass account check
		if (aid)
		  *aid = 0LL;

		// copy content id from rif buffer
		if (content_id) // copy content id from rif 
		  memcpy(content_id, psprif->contentId, 0x30);

		if (flags) {
		  if (__builtin_bswap16(psprif->licenseType) & 0x200)
			(*flags) |= 0x1;
		  if (__builtin_bswap16(psprif->licenseType) & 0x100)
			(*flags) |= 0x10000;
		}

		if (flags2)
		  *flags2 = 0;

		if (license_version)
		  *license_version = __builtin_bswap16(psprif->version);
		if (license_flags)
		  *license_flags = (uint8_t)__builtin_bswap16(psprif->licenseType);

		return 0;
  }

  return ret;
}

static int fakeRifData(PspRif *psprif, uint8_t *klicensee, uint32_t *flags, uint64_t *start_time, uint64_t *expiration_time)
{
	// Bypass expiration time for PS Plus games
	if (start_time)
		*start_time = 0LL;
	
	if (expiration_time)
		*expiration_time = 0x7FFFFFFFFFFFFFFFLL;
	
	// check is nopspemudrm rif
	if (( psprif != NULL &&            // check if is null
		psprif->version != -1 &&       // check if is psp rif
		( !is_offical_rif(psprif) || strcmp(psprif->contentId, "JA0003-PCSC80018_00-POCKETSTATION001") == 0 ))) {   /// check is not offical rif, or pocketstation
				
		// vita side never really cares about psp rif keys, so lets just set it to all be 0xFF
		if (klicensee != NULL){
			memset(klicensee, 0xFF, 0x10);
		}

		if (flags) {
			*flags = 0 ;
		}
		
		return 1;
	}
	else{
		return 0;
	}
}

static int ksceNpDrmGetRifVitaKeyPatched(PspRif *psprif, uint8_t *klicensee, uint32_t *flags, uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time) {
	int ret = TAI_CONTINUE(int, ksceNpDrmGetRifVitaKeyRef, psprif, klicensee, flags, sku_flag, start_time, expiration_time);
	if(ret < 0) {
		int res = fakeRifData(psprif, klicensee, flags, start_time, expiration_time);
		if(res)
			return 0;
	}
	return ret;
}

static int ksceNpDrmGetRifPspKeyPatched(PspRif *psprif, uint8_t *klicensee, uint32_t *flags, uint64_t *start_time, uint64_t *expiration_time) {
	int ret = TAI_CONTINUE(int, ksceNpDrmGetRifPspKeyRef, psprif, klicensee, flags, start_time, expiration_time);
	if(ret < 0) {
		int res = fakeRifData(psprif, klicensee, flags, start_time, expiration_time);
		if(res)
			return 0;
	}
	return ret;
}


static int ksceNpDrmReadActDataPatched(PspAct* activationData) {
	int ret = TAI_CONTINUE(int, ksceNpDrmReadActDataRef, activationData);
	
	if(ret < 0){ // if console not activated -- fake activation
		ksceKernelPrintf("[NoPspEmuDrm_kern] Console does not have NpDrm activation, faking it!\n",ret);
	
		// copy fake act.dat into buffer
		memcpy(activationData, FakeActBuffer, sizeof(PspAct));
		
		// Copy current OpenPSID into it
		SceOpenPsId openPsid;
		ksceSblAimgrGetOpenPsId(&openPsid);
		memcpy(activationData->openPsId, openPsid.open_psid, 0x10);
		
		// Copy current Account ID
		ksceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &activationData->accountId, sizeof(uint64_t));
				
		// Wipe signature
		memset(activationData->ecdsaSig, 0xFF, 0x28);		
	}
	
	return ret;
}

static int sceCompatCheckPocketStationPatched() {
	int res = TAI_CONTINUE(int, sceCompatCheckPocketStationRef);
	if(res < 0) {
		SceIoStat stat;
		int pocketstation_installed = ksceIoGetstat("ux0:/ps1emu/PCSC80018/texture.enc", &stat);
		if(pocketstation_installed >= 0)
			return 0;
	}
	
	return res;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {	
	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_info);
	int ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceNpDrm", &tai_info);
	if(ret < 0) {
		ksceKernelPrintf("[NoPspEmuDrm_kern] SceNpDrm not exist??? %x\n", ret);
	}

	// rif patch
	ksceNpDrmGetRifInfoHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifInfoRef, "SceNpDrm", 0xD84DC44A, 0xDB406EAE, ksceNpDrmGetRifInfoPatched); // ksceNpDrmGetRifInfo
	ksceNpDrmGetRifVitaKeyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifVitaKeyRef, "SceNpDrm", 0xD84DC44A, 0x723322B5, ksceNpDrmGetRifVitaKeyPatched); // ksceNpDrmGetRifVitaKey
	ksceNpDrmGetRifPspKeyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifPspKeyRef, "SceNpDrm", 0xD84DC44A, 0xDACB71F4, ksceNpDrmGetRifPspKeyPatched); // ksceNpDrmGetRifPspKey

	
	// __sce_ebootpbp patches
	ksceNpDrmEbootSigVerifyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmEbootSigVerifyRef, "SceNpDrm", 0xD84DC44A, 0x7A319692, return_0); // //ksceNpDrmEbootSigVerifyHook
	ksceNpDrmPspEbootVerifyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmPspEbootVerifyRef, "SceNpDrm", 0xD84DC44A, 0x7A319692, return_0); // //ksceNpDrmPspEbootVerifyHook
	
	// allow SceCompat to start without a rif or activation
	ksceNpDrmReadActDataHook = taiHookFunctionImportForKernel(KERNEL_PID, &ksceNpDrmReadActDataRef, "SceCompat", 0xD84DC44A, 0xD91C3BCE, ksceNpDrmReadActDataPatched); // ksceNpDrmReadActData
	ksceSblAimgrIsDEXHook = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSblAimgrIsDEXRef, "SceCompat", 0xFD00C69A, 0xF4B98F66, return_1); // ksceSblAimgrIsDEX
	sceCompatCheckPocketStationHook = taiHookFunctionExportForKernel(KERNEL_PID, &sceCompatCheckPocketStationRef, "SceCompat", 0xD84DC44A, 0x96FC2A87, sceCompatCheckPocketStationPatched); // sceCompatCheckPocketStation

	// Patch rif and act.dat signature checks, so the vita thinks our licenses are *LEGIT*
	npdrm_rif_verify_ecdsa_and_rsa_hook = taiHookFunctionOffsetForKernel(KERNEL_PID, &npdrm_rif_verify_ecdsa_and_rsa_ref, tai_info.modid, 0, 0xA8D8, 1, return_0); // rif_verify_ecdsa_and_rsa
	npdrm_act_verify_ecdsa_and_rsa_hook = taiHookFunctionOffsetForKernel(KERNEL_PID, &npdrm_act_verify_ecdsa_and_rsa_ref, tai_info.modid, 0, 0xA840, 1, return_0);  // act_verify_ecdsa_and_rsa
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	if (ksceNpDrmGetRifInfoHook >= 0) taiHookReleaseForKernel(ksceNpDrmGetRifInfoHook, ksceNpDrmGetRifInfoRef);
	if (ksceNpDrmGetRifVitaKeyHook >= 0) taiHookReleaseForKernel(ksceNpDrmGetRifVitaKeyHook, ksceNpDrmGetRifVitaKeyRef);
	if (ksceNpDrmGetRifPspKeyHook >= 0) taiHookReleaseForKernel(ksceNpDrmGetRifPspKeyHook, ksceNpDrmGetRifPspKeyRef);
	if (ksceNpDrmEbootSigVerifyHook >= 0) taiHookReleaseForKernel(ksceNpDrmEbootSigVerifyHook, ksceNpDrmEbootSigVerifyRef);
	if (ksceNpDrmPspEbootVerifyHook >= 0) taiHookReleaseForKernel(ksceNpDrmPspEbootVerifyRef, ksceNpDrmEbootSigVerifyRef);
	
	if (ksceSblAimgrIsDEXHook >= 0) taiHookReleaseForKernel(ksceSblAimgrIsDEXHook, ksceSblAimgrIsDEXRef);
	if (ksceNpDrmReadActDataHook >= 0) taiHookReleaseForKernel(ksceNpDrmReadActDataHook, ksceNpDrmReadActDataRef);
	if (sceCompatCheckPocketStationHook >= 0) taiHookReleaseForKernel(sceCompatCheckPocketStationHook, sceCompatCheckPocketStationRef);
	
	if (npdrm_rif_verify_ecdsa_and_rsa_hook >= 0) taiHookReleaseForKernel(npdrm_rif_verify_ecdsa_and_rsa_hook, npdrm_rif_verify_ecdsa_and_rsa_ref);
	if (npdrm_act_verify_ecdsa_and_rsa_hook >= 0) taiHookReleaseForKernel(npdrm_act_verify_ecdsa_and_rsa_hook, npdrm_act_verify_ecdsa_and_rsa_ref);
	
	return SCE_KERNEL_STOP_SUCCESS;
}
