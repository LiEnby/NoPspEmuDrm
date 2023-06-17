#include "SceShell.h"
#include <vitasdk.h>

#include <string.h>
#include <stdint.h>

static SceUID check_license_hook = -1;
static tai_hook_ref_t check_license_hook_ref;

static int check_license(shell_launch_param *lparam) {
	int ret = TAI_CONTINUE(int, check_license_hook_ref, lparam);
	
	if(lparam != NULL){
		if(sceClibStrncmp(lparam->startFolder, "ux0:pspemu/PSP/GAME/", 20) == 0){
					
			if(lparam->error != 0) {
				sceClibPrintf("Faking license for: %s\n", lparam->startFolder);

				lparam->endDate = 0x7FFFFFFFFFFFFFFFull;
				lparam->flags = 0x2D0001;			
				lparam->error = 0;
			}
			
			
			if(ret != 0)
				ret = 0;
		}
	}
	return ret;
}
 
int sceshell_module_start(tai_module_info_t tai_info) {
	
	switch (tai_info.module_nid) { 
		case 0x9189EB3B: // cexfortool 3.20 SceShell
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x177E8, 1, check_license);
		case 0x6CB01295: // devkit 3.60 SceShell
		case 0x232D733B: // devkit 3.61 SceShell
		case 0xE541DB9B: // devkit 3.63 SceShell
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x1BC16, 1, check_license);
			break;
		case 0xE6A02F2B: //devkit 3.65 SceShell
		case 0xAB5C2A00: //devkit 3.67 SceShell
		case 0x4FE7C671: //devkit 3.68 SceShell
		case 0xC5B7C871: //devkit 3.71 SceShell
		case 0x4670A0C8: //devkit 3.73 SceShell
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x1BC6E, 1, check_license);
			break;
		case 0xEAB89D5C: // testkit 3.60 SceShell
		case 0x7A5F8457: // testkit 3.61 SceShell
		case 0xE7C5011A: // testkit 3.63 SceShell
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x1BE8A, 1, check_license);
			break;
		case 0x587F9CED: // testkit 3.65 SceShell
		case 0x3C652B1A: // testkit 3.67 SceShell
		case 0x4DF04256: // testkit 3.68 SceShell
		case 0xA6509361: // testkit 3.72 SceShell
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x1BEE2, 1, check_license);
			break;
		case 0x0552F692: // retail 3.60 SceShell
		case 0x532155E5: // retail 3.61 SceShell
		case 0xBB4B0A3E: // retail 3.63 SceShell
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x1BCBA, 1, check_license);
			break;
		case 0x5549BF1F: // retail 3.65 SceShell
		case 0x34B4D82E: // retail 3.67 SceShell
		case 0x12DAC0F3: // retail 3.68 SceShell
		case 0x0703C828: // retail 3.69 SceShell
		case 0x2053B5A5: // retail 3.70 SceShell
		case 0xF476E785: // retail 3.71 SceShell
		case 0x939FFBE9: // retail 3.72 SceShell
		case 0x734D476A: // retail 3.73 SceShell
		case 0x51CB6207: // retail 3.74 SceShell
		default:
			check_license_hook = taiHookFunctionOffset(&check_license_hook_ref, tai_info.modid, 0, 0x1BD12, 1, check_license);			
			break;
	}
	
	return SCE_KERNEL_START_SUCCESS;
}
int sceshell_module_stop(){
	if (check_license_hook >= 0) taiHookRelease(check_license_hook, check_license_hook_ref);
}