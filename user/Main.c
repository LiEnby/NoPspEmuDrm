#include <vitasdk.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <taihen.h>

#include "PspEmu.h"
#include "SceShell.h"

static int pspemu = 0;
static int sceshell = 0;

void _start() __attribute__ ((weak, alias ("module_start"))); 
int module_start(SceSize args, void *argp) {
	int ret = 0;

	
	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_module_info_t);
	
	ret = taiGetModuleInfo("ScePspemu", &tai_info);
	if (ret >= 0){
		pspemu = 1;
		return pspemu_module_start(tai_info);
	}
	
	ret = taiGetModuleInfo("SceShell", &tai_info);
	if (ret >= 0){
		sceshell = 1;
		return sceshell_module_start(tai_info);
	}
	
	
	return SCE_KERNEL_START_NO_RESIDENT;
}

int module_stop(SceSize args, void *argp) {
	if(pspemu) return pspemu_module_stop();
	if(sceshell) return sceshell_module_stop();
	
	return SCE_KERNEL_STOP_SUCCESS;
}