#include "crypto/kirk_engine.h"
#include "crypto/amctrl.h"
#include "crypto/aes.h"

#include "PspNpDrm.h"
#include "Io.h"
#include "Pbp.h"
#include "Crypto.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <vitasdk.h>

static void print_buf(unsigned char* buffer, int sz){
	for(int i = 0; i < sz; i++){
		sceClibPrintf("%02X ", buffer[i]);
	}
	sceClibPrintf("\n");
}

int sceNpDrmCalcPgdKey(NpPgd* pgd, char* versionkey) {
	MAC_KEY mkey;
	
	int flag = 0x2;
	int mac_type = 0;
	int cipher_type = 0;
	
	
	// determine cipher types
	if (pgd->drm_type == 1)
	{
		mac_type = 1;
		flag |= 4;

		if(pgd->key_index > 1)
		{
			mac_type = 3;
			flag |= 8;
		}
	}
	else
	{
		mac_type = 2;
	}

	// calculate the key
	int ret = 0;
	
	if(sceDrmBBMacInit(&mkey, mac_type) < SCE_OK) return 0;
	if(sceDrmBBMacUpdate(&mkey, pgd, offsetof(NpPgd, pgd_ekey))  < SCE_OK) return 0;
	if(bbmac_getkey(&mkey, pgd->pgd_ekey, versionkey) < SCE_OK) return 0;
	
	// turn key to keyindex 0
	sceNpDrmTransformVersionKey(versionkey, pgd->key_index, 0);
	return 1;
}

int sceNpDrmCalcEdatKey(NpPspEdat* edat, NpPgd* pgd, char* versionkey) {
	MAC_KEY mkey;

	char osx_key[0x10]; // ha? caz its a MAC.. :D
	char pgd_key[0x10];
	
	memset(osx_key, 0x00, 0x10); 
	memset(pgd_key, 0x00, 0x10);
	
	if ((edat->version & 1) == 1) {
		if(sceDrmBBMacInit(&mkey, 3) < SCE_OK) return 0;
		if(sceDrmBBMacUpdate(&mkey, edat, offsetof(NpPspEdat, header_hash)) < SCE_OK) return 0;
		if(bbmac_getkey(&mkey, edat->header_hash, osx_key) < SCE_OK) return 0;
	}
	else if((edat->version & 2) == 2) { // version 2 just has key outright in the header 
		memcpy(osx_key, edat->key, 0x10);
	}
	
	// get key from pgd header
	sceNpDrmCalcPgdKey(pgd, pgd_key);
	
	// get pgd key index
	gen_versionkey(pgd_key, pgd->key_index);
	
	// reverse the aes decrypt step done within npdrm.prx
	aes_encrypt(pgd_key, PSP_EDAT_AES);
		
	// reverse the xor step done within npdrm.prx
	for(int i = 0; i < 0x10; i++)
		versionkey[i] = pgd_key[i] ^ osx_key[i];
	
	// turn key to keyindex 0
	reverse_gen_versionkey(versionkey, edat->key_index, 0);
	
	return 1;
}

int sceNpDrmCalcNpUmdKey(NpUmdHdr* hdr, char* versionkey) {
	MAC_KEY mkey;
	
	// calcluate the key
	if(sceDrmBBMacInit(&mkey, 3) < SCE_OK) return 0;
	if(sceDrmBBMacUpdate(&mkey, (uint8_t*)hdr, offsetof(NpUmdHdr, header_hash)) < SCE_OK) return 0;
	if(bbmac_getkey(&mkey, hdr->header_hash, versionkey) < SCE_OK) return 0;

	// turn key to keyindex 0
	sceNpDrmTransformVersionKey(versionkey, hdr->key_index, 0);
	return 1;
}

int reverse_gen_versionkey(char* versionkey, int keyindex)
{
	keyindex &= 0xffffff;
	int ret = 0;
	if(keyindex != 0){
		ret = 0x80550901;
		if (keyindex < 4)
        {
			aes_decrypt(versionkey, VERSIONKEY_INDEX_DERIVATION_KEYS[keyindex]);
			ret = 0;
		}		
	}
	return ret;
}

int gen_versionkey(char* versionkey, int keyindex)
{
	keyindex &= 0xffffff;
	int ret = 0;
	if(keyindex != 0){
		ret = 0x80550901;
		if (keyindex < 4)
        {
			aes_encrypt(versionkey, VERSIONKEY_INDEX_DERIVATION_KEYS[keyindex]);
			ret = 0;
		}		
	}
	return ret;
}

void get_act_key(char* key_out, char* key_table, int count){
	char decKey[0x10];
	char idps[32];
	
	// get idps
	_vshSblAimgrGetConsoleId(idps);
	
	aes_encrypt_out(decKey, PSP_ACT_AES, idps);
	
	
	for (int i = 0; i < count; i++){
		aes_decrypt_out(key_out, &key_table[i * 0x10], decKey);
	}
	
}



void gen_enc_key1(char* encKey1_out, int keyId){
	char encKey1[0x10];
	sceUtilsBufferCopyWithRange(encKey1, 0x10, NULL, 0, KIRK_CMD_PRNG);
	*(uint32_t*)(encKey1+0xC) = __builtin_bswap32(keyId);
	
	
	aes_encrypt(encKey1, PSP_RIF_AES);

	memcpy(encKey1_out, encKey1, sizeof(encKey1));

}

void gen_enc_key2(char* encKey2_out, char* versionKey, int keyId){

	PspAct* act = malloc(sizeof(PspAct));
	memset(act, 0x00, sizeof(PspAct));

	// read act.dat
	ReadFile("tm0:/npdrm/act.dat", act, sizeof(PspAct));
	
	char actKey[0x10];
	get_act_key(actKey, &act->primKeyTable[keyId*0x10], 1);
	
	char encKey2[0x10];
	aes_encrypt_out(encKey2, versionKey, actKey);
	
	memcpy(encKey2_out, encKey2, sizeof(encKey2));
	
	free(act);

}

int is_offical_rif(PspRif* rif){
	for(int i = 0; i < 0x28; i++) {
		if(rif->ecdsaSig[i] != 0xff) {
			return 1;
		}
	}
	return 0;
}

int get_rif_state(PspRif* rif, char* expectedContentId){
	
	int officalRif = is_offical_rif(rif);
	int invalidState = officalRif ? OFFICAL_INVALID : NOPSPEMUDRM_INVALID;
	
	sceClibPrintf("[RIFCHECK] is offical rif? %x\n",  officalRif);
	
	// get the current account
	uint64_t accountId = -1;
	sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &accountId, sizeof(uint64_t));
	
	// check the rif is for this account
	if(rif->accountId != accountId) { 
		sceClibPrintf("[RIFCHECK] account Id do not match\n");
		return invalidState;
	}

	sceClibPrintf("[RIFCHECK] account Id match\n");

	// check the rif contentid matches
	if(strcmp(expectedContentId, rif->contentId) != 0) {
		sceClibPrintf("[RIFCHECK] content Id do not match\n");
		return invalidState;
	}
	
	// check encKey2 is set
	for(int i = 0; i < 0x10; i++)
		if(rif->encKey2[i] != 0x00) return VALID_RIF;

	sceClibPrintf("encKey2 is all 0\n");
	return invalidState;
}

PspRifState sceNpDrmCheckRifState(char* contentId, const char* path) {
	PspRif* rif = malloc(sizeof(PspRif));
	memset(rif, 0x00, sizeof(PspRif));
	
	int ret = NOPSPEMUDRM_INVALID;	
	int rd = ReadFile(path, rif, sizeof(PspRif));
	
	if(rd > 0 && rd >= sizeof(PspRif)) {
		ret = get_rif_state(rif, contentId);
	}
	
	free(rif);	
	return ret;
}

void sceNpDrmGenerateRif(char* contentId, const char* path) {
	PspRif* rif = malloc(sizeof(PspRif));
	memset(rif, 0x00, sizeof(PspRif));
	
	uint8_t versionkey[0x10];
	memset(versionkey, 0xFF, 0x10);
	
	// get version key
	if(!search_games("ms0:/PSP/GAME", contentId, versionkey)) {
		sceClibPrintf("[VKEY] Unable to find versionkey!!!\n");
		return;
	}
	
	sceClibPrintf("[VKEY] versionkey: ");
	print_buf(versionkey, 0x10);
	
	// determine a random key from act.dat to use.
	int keyId = (int)(random_uint() % 0x80);
	
	/*
	*	GENERATE THE RIF
	*/

	// set rif version
	rif->version = __builtin_bswap16(1);
	
	// set version flag
	rif->versionFlag = __builtin_bswap16(1);;	
	
	// set license flag
	rif->licenseType = 0;
	
	// set drm type
	rif->drmType = __builtin_bswap16(2);

	// set account id
	sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &rif->accountId, sizeof(uint64_t));

	// set content id
	strncpy(rif->contentId, contentId, 0x24);
	
	// set start time
	rif->startTime = 0;

	// set end time
	rif->endTime = 0;

	// set enckey	
	gen_enc_key1(rif->encKey1, keyId);

	// set enckey2
	gen_enc_key2(rif->encKey2, versionkey, keyId);
	
	// set signature to all 0xFF
	memset(rif->ecdsaSig, 0xFF, 0x28);

	// print rif info	
	WriteFile(path, rif, sizeof(PspRif));
		
	free(rif);	
}


int sceNpDrmTransformVersionKey(char* versionKey, int srcKeyType, int dstKeyType)
{
	int ret = reverse_gen_versionkey(versionKey, srcKeyType);
	if (ret >= 0)
		ret = gen_versionkey(versionKey, dstKeyType); // generate a new version key of this type
	return ret;
}
