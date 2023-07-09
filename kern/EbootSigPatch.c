#include "EbootSigPatch.h"
#include "Log.h"

// hook
static SceUID ksceNpDrmEbootSigVerifyHook;
static SceUID ksceNpDrmPspEbootVerifyHook;
static SceUID ksceNpDrmEbootSigGenMultiDiscHook;
static SceUID ksceNpDrmEbootSigGenPs1Hook;
static SceUID ksceNpDrmEbootSigGenPspHook;
static SceUID ksceNpDrmPspEbootSigGenHook;

// ref
static tai_hook_ref_t ksceNpDrmEbootSigVerifyRef;
static tai_hook_ref_t ksceNpDrmPspEbootVerifyRef;
static tai_hook_ref_t ksceNpDrmEbootSigGenMultiDiscRef;
static tai_hook_ref_t ksceNpDrmEbootSigGenPs1Ref;
static tai_hook_ref_t ksceNpDrmEbootSigGenPspRef;
static tai_hook_ref_t ksceNpDrmPspEbootSigGenRef;

static int return_0() {
	log("[NOPSPEMUDRM_KERN] returning 0\n");
	return 0;
}

static int ksceNpDrmPspEbootSigGenPatched(const char *eboot_pbp_path, const void *eboot_sha256, void *eboot_signature) {
	int res = TAI_CONTINUE(int, ksceNpDrmPspEbootSigGenRef, eboot_pbp_path, eboot_sha256, eboot_signature);
	
	if(res < 0) {
		log("[NOPSPEMUDRM_KERN] ksceNpDrmPspEbootSigGen failed (0x%x), faking it!\n", res);
		memset(eboot_signature, 0xFF, 0x100);
		return 0x100;
	}
	
	return res;
}

static int ksceNpDrmEbootSigGenPspPatched(const char *eboot_pbp_path, const void *eboot_sha256, void *eboot_signature, int sw_version) {
	int res = TAI_CONTINUE(int, ksceNpDrmEbootSigGenPspRef, eboot_pbp_path, eboot_sha256, eboot_signature, sw_version);
	
	if(res < 0) {
		log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigGenPsp failed (0x%x), faking it!\n", res);
		memset(eboot_signature, 0xFF, 0x200);
		return 0x200;
	}
	
	return res;
}


static int ksceNpDrmEbootSigGenPs1Patched(const char *eboot_pbp_path, const void *eboot_sha256, void *eboot_signature, int sw_version) {
	int res = TAI_CONTINUE(int, ksceNpDrmEbootSigGenPs1Ref, eboot_pbp_path, eboot_sha256, eboot_signature, sw_version);
	
	if(res < 0) {
		log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigGenPs1 failed (0x%x), faking it!\n", res);
		memset(eboot_signature, 0xFF, 0x200);
		return 0x200;
	}
	
	return res;
}


static int ksceNpDrmEbootSigGenMultiDiscPatched(const char *eboot_pbp_path, const void *sce_discinfo, void *eboot_signature, int sw_version) {
	int res = TAI_CONTINUE(int, ksceNpDrmEbootSigGenMultiDiscRef, eboot_pbp_path, sce_discinfo, eboot_signature, sw_version);
	
	if(res < 0) {
		log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigGenMultiDisc failed (0x%x), faking it!\n", res);
		memset(eboot_signature, 0xFF, 0x200);
		return 0x200;
	}
	
	return res;
}

void init_eboot_sig_patch() {
	ksceNpDrmEbootSigVerifyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmEbootSigVerifyRef, "SceNpDrm", 0xD84DC44A, 0x7A319692, return_0); // //ksceNpDrmEbootSigVerif
	ksceNpDrmPspEbootVerifyHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmPspEbootVerifyRef, "SceNpDrm", 0xD84DC44A, 0x7A319692, return_0); // //ksceNpDrmPspEbootVerify

	ksceNpDrmEbootSigGenMultiDiscHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmEbootSigGenMultiDiscRef, "SceNpDrm", 0xD84DC44A, 0x39A7A666, ksceNpDrmEbootSigGenMultiDiscPatched); // //ksceNpDrmEbootSigGenMultiDisc
	ksceNpDrmEbootSigGenPs1Hook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmEbootSigGenPs1Ref, "SceNpDrm", 0xD84DC44A, 0x6D9223E1, ksceNpDrmEbootSigGenPs1Patched); // //ksceNpDrmEbootSigGenPs1
	ksceNpDrmEbootSigGenPspHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmEbootSigGenPspRef, "SceNpDrm", 0xD84DC44A, 0x90B1A6D3, ksceNpDrmEbootSigGenPspPatched); // //ksceNpDrmEbootSigGenPsp
	ksceNpDrmPspEbootSigGenHook = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmPspEbootSigGenRef, "SceNpDrm", 0xD84DC44A, 0xEF387FC4, ksceNpDrmPspEbootSigGenPatched); // //ksceNpDrmPspEbootSigGen

	log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigVerifyHook: %x ksceNpDrmEbootSigVerifyRef: %x\n", ksceNpDrmEbootSigVerifyHook, ksceNpDrmEbootSigVerifyRef);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmPspEbootVerifyHook: %x ksceNpDrmPspEbootVerifyRef: %x\n", ksceNpDrmPspEbootVerifyHook, ksceNpDrmPspEbootVerifyRef);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigGenMultiDiscHook: %x ksceNpDrmEbootSigGenMultiDiscRef: %x\n", ksceNpDrmEbootSigGenMultiDiscHook, ksceNpDrmEbootSigGenMultiDiscRef);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigGenPs1Hook: %x ksceNpDrmEbootSigGenPs1Ref: %x\n", ksceNpDrmEbootSigGenPs1Hook, ksceNpDrmEbootSigGenPs1Ref);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmEbootSigGenPspHook: %x ksceNpDrmEbootSigGenPspRef: %x\n", ksceNpDrmEbootSigGenPspHook, ksceNpDrmEbootSigGenPspRef);
	log("[NOPSPEMUDRM_KERN] ksceNpDrmPspEbootSigGenHook: %x ksceNpDrmPspEbootSigGenRef: %x\n", ksceNpDrmPspEbootSigGenHook, ksceNpDrmPspEbootSigGenRef);
	
}

void term_eboot_sig_patch() {
	if (ksceNpDrmEbootSigVerifyHook >= 0) taiHookReleaseForKernel(ksceNpDrmEbootSigVerifyHook, ksceNpDrmEbootSigVerifyHook);
	if (ksceNpDrmPspEbootVerifyHook >= 0) taiHookReleaseForKernel(ksceNpDrmPspEbootVerifyHook, ksceNpDrmPspEbootVerifyRef);
	if (ksceNpDrmEbootSigGenMultiDiscHook >= 0) taiHookReleaseForKernel(ksceNpDrmEbootSigGenMultiDiscHook, ksceNpDrmEbootSigGenMultiDiscRef);
	if (ksceNpDrmEbootSigGenPs1Hook >= 0) taiHookReleaseForKernel(ksceNpDrmEbootSigGenPs1Hook, ksceNpDrmEbootSigGenPs1Ref);
	if (ksceNpDrmEbootSigGenPspHook >= 0) taiHookReleaseForKernel(ksceNpDrmEbootSigGenPspHook, ksceNpDrmEbootSigGenPspRef);
	if (ksceNpDrmPspEbootSigGenHook >= 0) taiHookReleaseForKernel(ksceNpDrmPspEbootSigGenHook, ksceNpDrmPspEbootSigGenRef);

}