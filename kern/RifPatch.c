#include "RifPatch.h"
#include "EcPatch.h"
#include "../user/PspNpDrm.h"
#include "Log.h"

// hook
static SceUID ksceNpDrmGetRifInfoHook;
static SceUID ksceNpDrmGetRifVitaKeyHook;
static SceUID ksceNpDrmGetRifPspKeyHook;
static SceUID sceCompatCheckPocketStationHook;

// ref
static tai_hook_ref_t ksceNpDrmGetRifInfoRef;
static tai_hook_ref_t ksceNpDrmGetRifVitaKeyRef;
static tai_hook_ref_t ksceNpDrmGetRifPspKeyRef;
static tai_hook_ref_t sceCompatCheckPocketStationRef;


int check_pksuite_installed() {
	SceIoStat stat;
	int pocketstation_installed = ksceIoGetstat("ux0:/ps1emu/PCSC80018/texture.enc", &stat);
	log("[NOPSPEMUDRM_KERN] check_pksuite_installed stat = %x\n", pocketstation_installed);
	if(pocketstation_installed >= 0) {
		return 1;
	}
	return 0;
}

int is_offical_rif(PspRif* rif){
	// if its pkstation license && we have the pocketstation "app" -- then we want to patch the rif checks-
	// don't care if its an offical rif or not in that case, 
	if(strcmp(rif->contentId, "JA0003-PCSC80018_00-POCKETSTATION001") == 0 && check_pksuite_installed()) {
		log("[NOPSPEMUDRM_KERN] is_offical_rif -> false (is pksuite)\n");
		return 0;
	}
	
	// otherwise, just check it has all 0xFF ECDSA signature -- created by nopspemudrm.
	if(!is_ecdsa_all_ff(rif->ecdsaSig)) {
		log("[NOPSPEMUDRM_KERN] is_offical_rif -> true (ecdsaSig != 0xff)\n");
		return 1;
	}
	log("[NOPSPEMUDRM_KERN] is_offical_rif -> false (ecdsaSig == 0xff)\n");
	return 0;
}


static int fake_rif_data(PspRif *psprif, uint8_t *klicensee, uint32_t *flags, uint64_t *start_time, uint64_t *expiration_time)
{
	
	// check is nopspemudrm rif
	if (psprif != NULL &&            // check if is null
		psprif->version != -1 &&     // check if is psp rif
		!is_offical_rif(psprif)) {   /// check is not offical rif
		
		log("[NOPSPEMUDRM_KERN] fake_rif_data reached, for contentid : %s\n", psprif->contentId);
		
		// Bypass expiration time for PS Plus games
		if (start_time)
			*start_time = 0LL;
		
		if (expiration_time)
			*expiration_time = 0x7FFFFFFFFFFFFFFFLL;
	
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

static int ksceNpDrmGetRifInfoPatched(PspRif *psprif, int license_size, int mode, char *content_id, uint64_t *aid, uint16_t *license_version, uint8_t *license_flags, uint32_t *flags, uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time, uint64_t *flags2) {

	int ret = TAI_CONTINUE(int, ksceNpDrmGetRifInfoRef, psprif, license_size, mode, content_id, aid, license_version, license_flags, flags, sku_flag, start_time, expiration_time, flags2);

	if (ret < 0 &&
		psprif != NULL &&       // check if is null
		psprif->version != -1 && // check is  not vita rif
		!is_offical_rif(psprif)) { // check is not offical rif
		
		log("[NOPSPEMUDRM_KERN] ksceNpDrmGetRifInfoPatched reached, for contentid : %s\n", psprif->contentId);
		
		// Bypass expiration time for PS Plus games
		if (start_time)
			*start_time = 0LL;
		
		if (expiration_time)
			*expiration_time = 0x7FFFFFFFFFFFFFFFLL;
		
		// bypass account check
		if (aid)
		  *aid = 0LL;
		
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


static int ksceNpDrmGetRifVitaKeyPatched(PspRif *psprif, uint8_t *klicensee, uint32_t *flags, uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time) {
	int ret = TAI_CONTINUE(int, ksceNpDrmGetRifVitaKeyRef, psprif, klicensee, flags, sku_flag, start_time, expiration_time);
	if(ret < 0) {
		log("[NOPSPEMUDRM_KERN] ksceNpDrmGetRifVitaKeyPatched reached, for contentid : %s\n", psprif->contentId);
		int res = fake_rif_data(psprif, klicensee, flags, start_time, expiration_time);
		if(res) {
			return 0;
		}
	}
	return ret;
}

static int ksceNpDrmGetRifPspKeyPatched(PspRif *psprif, uint8_t *klicensee, uint32_t *flags, uint64_t *start_time, uint64_t *expiration_time) {
	int ret = TAI_CONTINUE(int, ksceNpDrmGetRifPspKeyRef, psprif, klicensee, flags, start_time, expiration_time);
	if(ret < 0) {
		log("[NOPSPEMUDRM_KERN] ksceNpDrmGetRifPspKeyPatched reached, for contentid : %s\n", psprif->contentId);
		int res = fake_rif_data(psprif, klicensee, flags, start_time, expiration_time);
		if(res) {
			return 0;
		}
	}
	return ret;
}

static int sceCompatCheckPocketStationPatched() {
	int res = TAI_CONTINUE(int, sceCompatCheckPocketStationRef);
	if(res < 0 && check_pksuite_installed()) {
		return 0;
	}
	return res;
}

void init_rif_patch() {

	ksceNpDrmGetRifInfoHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifInfoRef, "SceNpDrm", 0xD84DC44A, 0xDB406EAE, ksceNpDrmGetRifInfoPatched); // ksceNpDrmGetRifInfo
	ksceNpDrmGetRifVitaKeyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifVitaKeyRef, "SceNpDrm", 0xD84DC44A, 0x723322B5, ksceNpDrmGetRifVitaKeyPatched); // ksceNpDrmGetRifVitaKey
	ksceNpDrmGetRifPspKeyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifPspKeyRef, "SceNpDrm", 0xD84DC44A, 0xDACB71F4, ksceNpDrmGetRifPspKeyPatched); // ksceNpDrmGetRifPspKey
	sceCompatCheckPocketStationHook = taiHookFunctionExportForKernel(KERNEL_PID, &sceCompatCheckPocketStationRef, "SceCompat", 0x0F35909D, 0x96FC2A87, sceCompatCheckPocketStationPatched); // sceCompatCheckPocketStation
	log("[NOPSPEMUDRM_KERN] ksceNpDrmGetRifInfoHook: %x ksceNpDrmGetRifInfoRef: %x\n", ksceNpDrmGetRifInfoHook, ksceNpDrmGetRifInfoRef);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmGetRifVitaKeyHook: %x ksceNpDrmGetRifVitaKeyRef: %x\n", ksceNpDrmGetRifVitaKeyHook, ksceNpDrmGetRifVitaKeyRef);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmGetRifPspKeyHook: %x ksceNpDrmGetRifPspKeyRef: %x\n", ksceNpDrmGetRifPspKeyHook, ksceNpDrmGetRifPspKeyRef);
	log("[NOPSPEMUDRM_KERN] sceCompatCheckPocketStationHook: %x sceCompatCheckPocketStationRef: %x\n", ksceNpDrmGetRifPspKeyHook, ksceNpDrmGetRifPspKeyRef);


}

void term_rif_patch() {
	if (ksceNpDrmGetRifInfoHook >= 0) taiHookReleaseForKernel(ksceNpDrmGetRifInfoHook, ksceNpDrmGetRifInfoRef);
	if (ksceNpDrmGetRifVitaKeyHook >= 0) taiHookReleaseForKernel(ksceNpDrmGetRifVitaKeyHook, ksceNpDrmGetRifVitaKeyRef);
	if (ksceNpDrmGetRifPspKeyHook >= 0) taiHookReleaseForKernel(ksceNpDrmGetRifPspKeyHook, ksceNpDrmGetRifPspKeyRef);
	if (sceCompatCheckPocketStationHook >= 0) taiHookReleaseForKernel(sceCompatCheckPocketStationHook, sceCompatCheckPocketStationRef);
}