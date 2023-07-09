#include "EcPatch.h"
#include "../user/PspNpDrm.h"
#include "Log.h"

// hook
static SceUID npdrm_rif_verify_ecdsa_and_rsa_hook; 
static SceUID npdrm_act_verify_ecdsa_and_rsa_hook; 

// ref
static tai_hook_ref_t npdrm_rif_verify_ecdsa_and_rsa_ref;
static tai_hook_ref_t npdrm_act_verify_ecdsa_and_rsa_ref;

int is_ecdsa_all_ff(unsigned char* ecdsaSig){
	for(int i = 0; i < 0x28; i++) {
		if(ecdsaSig[i] != 0xff) {
			return 0;
		}
	}
	return 1;
}

static int act_verify_ecdsa_and_rsa_patched(PspAct* pspact) {
	int res = TAI_CONTINUE(int, npdrm_act_verify_ecdsa_and_rsa_ref, pspact);
	
	if(res < 0 && is_ecdsa_all_ff(pspact->ecdsaSig)) {
		log("[NOPSPEMUDRM_KERN] act_verify_ecdsa_and_rsa returned %x .. faking signature validation success.\n", res);
		return 0;
	}
	
	return res;
}

static int rif_verify_ecdsa_and_rsa_patched(PspRif* psprif, int flag) {
	int res = TAI_CONTINUE(int, npdrm_rif_verify_ecdsa_and_rsa_ref, psprif, flag);
	
	if(res < 0 && is_ecdsa_all_ff(psprif->ecdsaSig)) {
		log("[NOPSPEMUDRM_KERN] rif_verify_ecdsa_and_rsa returned %x .. faking signature validation success.\n", res);
		return 0;
	}
	
	return res;
}

void init_ec_patch() {
	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_info);
	int ret = taiGetModuleInfoForKernel(KERNEL_PID, "SceNpDrm", &tai_info);
	if(ret >= 0) {
		npdrm_rif_verify_ecdsa_and_rsa_hook = taiHookFunctionOffsetForKernel(KERNEL_PID, &npdrm_rif_verify_ecdsa_and_rsa_ref, tai_info.modid, 0, 0xA8D8, 1, rif_verify_ecdsa_and_rsa_patched); // rif_verify_ecdsa_and_rsa
		npdrm_act_verify_ecdsa_and_rsa_hook = taiHookFunctionOffsetForKernel(KERNEL_PID, &npdrm_act_verify_ecdsa_and_rsa_ref, tai_info.modid, 0, 0xA840, 1, act_verify_ecdsa_and_rsa_patched);  // act_verify_ecdsa_and_rsa
		
		log("[NOPSPEMUDRM_KERN] npdrm_rif_verify_ecdsa_and_rsa_hook: %x npdrm_rif_verify_ecdsa_and_rsa_ref: %x\n", npdrm_rif_verify_ecdsa_and_rsa_hook, npdrm_rif_verify_ecdsa_and_rsa_ref);
		log("[NOPSPEMUDRM_KERN] npdrm_act_verify_ecdsa_and_rsa_hook: %x npdrm_act_verify_ecdsa_and_rsa_ref: %x\n", npdrm_act_verify_ecdsa_and_rsa_hook, npdrm_act_verify_ecdsa_and_rsa_ref);	
	}
}

void term_ec_patch() {
	if (npdrm_rif_verify_ecdsa_and_rsa_hook >= 0) taiHookReleaseForKernel(npdrm_rif_verify_ecdsa_and_rsa_hook, npdrm_rif_verify_ecdsa_and_rsa_ref);
	if (npdrm_act_verify_ecdsa_and_rsa_hook >= 0) taiHookReleaseForKernel(npdrm_act_verify_ecdsa_and_rsa_hook, npdrm_act_verify_ecdsa_and_rsa_ref);
}