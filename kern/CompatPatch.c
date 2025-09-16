#include <PspNpDrm.h>

#include "CompatPatch.h"
#include "Log.h"

// hook
static SceUID ksceNpDrmReadActDataHook;
static SceUID ksceSblAimgrIsDEXHook;

// ref
static tai_hook_ref_t ksceNpDrmReadActDataRef;
static tai_hook_ref_t ksceSblAimgrIsDEXRef;

static int return_1() {
	LOG("[NOPSPEMUDRM_KERN] returning 1\n");
	return 1;
}


static int ksceNpDrmReadActDataPatched(PspAct* activationData) {
	int ret = TAI_CONTINUE(int, ksceNpDrmReadActDataRef, activationData);
	
	if(ret < 0){ // if console not activated -- fake activation
		LOG("[NOPSPEMUDRM_KERN] ksceNpDrmReadActDataPatched reached\n");
		
		// copy fake act.dat into buffer
		memcpy(activationData, fake_activation_buffer, sizeof(PspAct));
		
		// Copy current OpenPSID into it
		SceOpenPsId openPsid;
		ksceSblAimgrGetOpenPsId(&openPsid);
		memcpy(activationData->open_psid, openPsid.open_psid, 0x10);
		
		// Copy current Account ID
		ksceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &activationData->account_id, sizeof(uint64_t));
				
		// Wipe signature
		memset(activationData->ecdsa_signature, 0xFF, 0x28);		
	}
	
	return ret;
}

void init_compat_patch() {
	ksceNpDrmReadActDataHook = taiHookFunctionImportForKernel(KERNEL_PID, &ksceNpDrmReadActDataRef, "SceCompat", 0xD84DC44A, 0xD91C3BCE, ksceNpDrmReadActDataPatched); // ksceNpDrmReadActData
	ksceSblAimgrIsDEXHook = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSblAimgrIsDEXRef, "SceCompat", 0xFD00C69A, 0xF4B98F66, return_1); // ksceSblAimgrIsDEX

	LOG("[NOPSPEMUDRM_KERN] ksceNpDrmReadActDataHook: %x ksceNpDrmReadActDataRef: %x\n", ksceNpDrmReadActDataHook, ksceNpDrmReadActDataRef);
	LOG("[NOPSPEMUDRM_KERN] ksceSblAimgrIsDEXHook: %x ksceSblAimgrIsDEXRef: %x\n", ksceSblAimgrIsDEXHook, ksceSblAimgrIsDEXRef);

	
}

void term_compat_patch() {
	if (ksceSblAimgrIsDEXHook >= 0) taiHookReleaseForKernel(ksceSblAimgrIsDEXHook, ksceSblAimgrIsDEXRef);
	if (ksceNpDrmReadActDataHook >= 0) taiHookReleaseForKernel(ksceNpDrmReadActDataHook, ksceNpDrmReadActDataRef);
}