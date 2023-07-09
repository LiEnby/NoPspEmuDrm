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
#include "EcPatch.h"
#include "RifPatch.h"
#include "CompatPatch.h"
#include "EbootSigPatch.h"

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {	

	// rif patch
	init_rif_patch();
	// __sce_ebootpbp patches
	init_eboot_sig_patch();	
	// allow SceCompat to start without a rif or activation
	init_compat_patch();
	// Patch rif and act.dat signature checks, so the vita thinks our licenses are *LEGIT* -- makes manual work
	init_ec_patch();
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {

	term_rif_patch();
	term_eboot_sig_patch();
	term_compat_patch();
	term_ec_patch();
		
	return SCE_KERNEL_STOP_SUCCESS;
}
