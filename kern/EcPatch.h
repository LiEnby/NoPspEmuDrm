#ifndef __EC_PATCH_H__
#define __EC_PATCH_H__
#include <taihen.h>
#include <vitasdkkern.h>

void init_ec_patch();
void term_ec_patch();
int is_ecdsa_all_ff(unsigned char* ecdsaSig);

#endif