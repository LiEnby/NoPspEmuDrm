#include "crypto/kirk_engine.h"
#include "crypto/amctrl.h"
#include "crypto/aes.h"

#include "PspNpDrm.h"
#include "Io.h"
#include "Pbp.h"
#include "Crypto.h"
#include "Log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <vitasdk.h>

/*
* workaround console id spoofers; read IDPS from idstorage
*/
int get_real_console_id(char* console_id_out) {
	
	char idstorage_leaf[0x200];
	int ret = vshIdStorageReadLeaf(0x01, idstorage_leaf);
	if(ret < 0) return ret;
	memcpy(console_id_out, idstorage_leaf + 0x60, 0x10);

	return 0;
}

uint64_t get_account_id() {
	uint64_t account_id = -1;
	sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &account_id, sizeof(account_id));
	return account_id;
}

int is_offical_rif(PspRif* rif){
	for(int i = 0; i < 0x28; i++) {
		if(rif->ecdsa_signature[i] != 0xff) {
			return 1;
		}
	}
	return 0;
}

int is_npdrm_activated() {
	int act_type = 0;
	int version_flag = 0;
	uint64_t account_id = -1;
	uint64_t expire_date[2];
	
	memset(expire_date, 0x00, sizeof(expire_date));
	
	int ret = _sceNpDrmCheckActData(&act_type, &version_flag, &account_id, expire_date);

	if(ret >= 0) {
		if(get_account_id() != account_id) return 0;
		return 1;
	}
	else {
		return 0;		
	}
}

int reverse_gen_versionkey(char* version_key, int key_index)
{
	key_index &= 0xffffff;
	int ret = 0;
	
	if(key_index != 0){
		ret = 0x80550901;
		if (key_index < 4)
        {
			aes_decrypt(version_key, VERSIONKEY_INDEX_DERIVATION_KEYS[key_index]);
			ret = 0;
		}		
	}
	
	return ret;
}

int gen_versionkey(char* version_key, int key_index)
{
	key_index &= 0xffffff;
	int ret = 0;
	
	if(key_index != 0){
		ret = 0x80550901;
		if (key_index < 4)
        {
			aes_encrypt(version_key, VERSIONKEY_INDEX_DERIVATION_KEYS[key_index]);
			ret = 0;
		}		
	}
	
	return ret;
}

void get_act_key(char* key_out, char* encrypted_act_key, int count){
	char decrypt_key[0x10];
	char idps[0x10];
	
	get_real_console_id(idps);
	aes_encrypt_out(decrypt_key, PSP_ACT_AES, idps);
	
	for (int i = 0; i < count; i++){
		aes_decrypt_out(key_out, encrypted_act_key, decrypt_key);
	}
	
}

int get_activation_data(PspAct* act) {
	// read act.dat
	
	if(is_npdrm_activated()) {
		read_file("tm0:/npdrm/act.dat", act, sizeof(PspAct));
	}
	else {
		memcpy(act, fake_activation_buffer, sizeof(PspAct));
	}
	
	return 0;
}

void generate_encrypted_key_id(char* encrypted_key_id, int key_id){
	// zero-out the keyid buffer.
	memset(encrypted_key_id, 0x00, AES_BUFFER_SIZE);
	
	// generate random bytes into buffer
	sceUtilsBufferCopyWithRange(encrypted_key_id, AES_BUFFER_SIZE, NULL, 0, KIRK_CMD_PRNG);

	// set the keyid inside the buffer.
	*(uint32_t*)(encrypted_key_id+0xC) = __builtin_bswap32(key_id);
	LOG("[NOPSPEMUDRM_USER] Using activation key id: %x\n", key_id);

	// encrypt using the psp rif aes key,
	aes_encrypt(encrypted_key_id, PSP_RIF_AES);
}

void generate_encrypted_version_key(char* encrypted_version_key, const char* version_key, int key_id){
	PspAct act;
	memset(&act, 0x00, sizeof(PspAct));
	
	// read activation data
	get_activation_data(&act);

	// get encryption key from the activation table
	char act_key[0x10];
	get_act_key(act_key, act.primary_key_table[key_id], 1);

	aes_encrypt_out(encrypted_version_key, version_key, act_key);

}


int get_rif_state(PspRif* rif, const char* expected_content_id){
	// is this an offical rif?
	int offical_rif = is_offical_rif(rif);
	
	// is the console activated?
	int is_activated = is_npdrm_activated();
	
	// if its an offical rif, set OFFICAL_INVALID, so we dont overwrite it when generating fake rif.
	int invalid_state = offical_rif ? OFFICAL_INVALID : NOPSPEMUDRM_INVALID;

	// get the current account
	uint64_t account_id = get_account_id();
	
	// get current secure tick
	SceRtcTick rtc_tick;
	memset(&rtc_tick, 0x00, sizeof(SceRtcTick));
	sceCompatGetCurrentSecureTick(&rtc_tick);
	
	LOG("[NOPSPEMUDRM_USER] == console information ==\n");
	LOG("[NOPSPEMUDRM_USER] secure tick %llx\n", rtc_tick.tick);
	LOG("[NOPSPEMUDRM_USER] account id: %llx\n", account_id);
	LOG("[NOPSPEMUDRM_USER] invalid state: %x\n", invalid_state);
	LOG("[NOPSPEMUDRM_USER] is activated: %x\n", is_activated);
	LOG("[NOPSPEMUDRM_USER] offical rif: %x\n", offical_rif);
	LOG("[NOPSPEMUDRM_USER] expected content id: %s\n", expected_content_id);
	
	LOG("[NOPSPEMUDRM_USER] == rif information ==\n");
	LOG("[NOPSPEMUDRM_USER] rif->version %x\n", rif->version);
	LOG("[NOPSPEMUDRM_USER] rif->version_flag %x\n", rif->version_flag);
	LOG("[NOPSPEMUDRM_USER] rif->license_type %x\n", rif->license_type);
	LOG("[NOPSPEMUDRM_USER] rif->drm_type %x\n", rif->drm_type);
	LOG("[NOPSPEMUDRM_USER] rif->account_id %llx\n", rif->account_id);
	LOG("[NOPSPEMUDRM_USER] rif->content_id %s\n", rif->content_id);
	LOG("[NOPSPEMUDRM_USER] rif->start_time %llx\n", __builtin_bswap64(rif->start_time));
	LOG("[NOPSPEMUDRM_USER] rif->end_time %llx\n", __builtin_bswap64(rif->end_time));
	
	// check the console is activated
	if(offical_rif && !is_activated) {
		LOG("[NOPSPEMUDRM_USER] (offical_rif && !is_activated) check failed.\n");
		return invalid_state;
	}
	
	// check the rif is for this account
	if(rif->account_id != account_id) { 
		LOG("[NOPSPEMUDRM_USER] (rif->account_id != account_id) check failed.\n");
		return invalid_state;
	}

	// check the rif contentid matches
	if(strcmp(expected_content_id, rif->content_id) != 0) {
		LOG("[NOPSPEMUDRM_USER] (strcmp(expected_content_id, rif->content_id) != 0) check failed.\n");
		return invalid_state;
	}
	
	// check version_flag matches npdrm activation status
	if(!offical_rif && rif->version_flag != __builtin_bswap16(is_activated)) {
		LOG("[NOPSPEMUDRM_USER] (!offical_rif && rif->version_flag != __builtin_bswap16(is_activated)) check failed.\n");
		return invalid_state;
	}
	
	// check rif start time
	if(rif->start_time != 0 && rtc_tick.tick < __builtin_bswap64(rif->start_time)) {
		LOG("[NOPSPEMUDRM_USER] (rif->end_time != 0 && rtc_tick.tick > __builtin_bswap64(rif->end_time)) check failed.\n");
		return invalid_state;
	}

	// check rif end time
	if(rif->end_time != 0 && rtc_tick.tick > __builtin_bswap64(rif->end_time)) {
		LOG("[NOPSPEMUDRM_USER] (rif->end_time != 0 && rtc_tick.tick > __builtin_bswap64(rif->end_time)) check failed.\n");
		return invalid_state;
	}

	// check encrypted_version_key is set
	for(int i = 0; i < 0x10; i++) {
		if(rif->encrypted_version_key[i] != 0x00) return VALID_RIF;
	}
	
	LOG("[NOPSPEMUDRM_USER] encrypted_version_key check failed.\n");
	return invalid_state;
}

// library functions

int sceNpDrmCalcPgdKey(NpPgd* pgd, char* version_key) {
	MAC_KEY mkey;
	
	int flag = 0x2;
	int mac_type = 0;
	int cipher_type = 0;
	
	// determine cipher types
	if (pgd->drm_type == 1) {
		mac_type = 1;
		flag |= 4;

		if(pgd->key_index > 1) {
			mac_type = 3;
			flag |= 8;
		}
	}
	else {
		mac_type = 2;
	}

	// calculate the key
	int ret = 0;
	
	if(sceDrmBBMacInit(&mkey, mac_type) < SCE_OK) return 0;
	if(sceDrmBBMacUpdate(&mkey, (uint8_t*)pgd, offsetof(NpPgd, pgd_ekey))  < SCE_OK) return 0;
	if(bbmac_getkey(&mkey, pgd->pgd_ekey, version_key) < SCE_OK) return 0;
	
	// turn key to key_index 0
	sceNpDrmTransformVersionKey(version_key, pgd->key_index, 0);
	return 1;
}

int sceNpDrmCalcEdatKey(NpPspEdat* edat, NpPgd* pgd, char* version_key) {
	MAC_KEY mkey;

	char mac_key[0x10];
	char pgd_key[0x10];
	
	memset(mac_key, 0x00, 0x10); 
	memset(pgd_key, 0x00, 0x10);
	
	if ((edat->version & 1) == 1) {
		if(sceDrmBBMacInit(&mkey, 3) < SCE_OK) return 0;
		if(sceDrmBBMacUpdate(&mkey, (uint8_t*)edat, offsetof(NpPspEdat, header_hash)) < SCE_OK) return 0;
		if(bbmac_getkey(&mkey, edat->header_hash, mac_key) < SCE_OK) return 0;
	}
	else if((edat->version & 2) == 2) { // version 2 just has key outright in the header 
		memcpy(mac_key, edat->key, 0x10);
	}
	
	// get key from pgd header
	sceNpDrmCalcPgdKey(pgd, pgd_key);
	
	// get pgd key index
	gen_versionkey(pgd_key, pgd->key_index);
	
	// reverse the aes decrypt step done within npdrm.prx
	aes_encrypt(pgd_key, PSP_EDAT_AES);
		
	// reverse the xor step done within npdrm.prx
	for(int i = 0; i < 0x10; i++)
		version_key[i] = pgd_key[i] ^ mac_key[i];
	
	// turn key to key_index 0
	sceNpDrmTransformVersionKey(version_key, edat->key_index, 0);
	
	return 1;
}

int sceNpDrmCalcNpUmdKey(NpUmdHdr* hdr, char* version_key) {
	MAC_KEY mkey;
	
	// calcluate the key
	if(sceDrmBBMacInit(&mkey, 3) < SCE_OK) return 0;
	if(sceDrmBBMacUpdate(&mkey, (uint8_t*)hdr, offsetof(NpUmdHdr, header_hash)) < SCE_OK) return 0;
	if(bbmac_getkey(&mkey, hdr->header_hash, version_key) < SCE_OK) return 0;

	// turn key to key_index 0
	sceNpDrmTransformVersionKey(version_key, hdr->key_index, 0);
	return 1;
}

PspRifState sceNpDrmCheckRifState(char* content_id, const char* path) {
	PspRif* rif = malloc(sizeof(PspRif));
	memset(rif, 0x00, sizeof(PspRif));
	
	int ret = NOPSPEMUDRM_INVALID;	
	int rd = read_file(path, rif, sizeof(PspRif));
	
	if(rd > 0 && rd >= sizeof(PspRif)) {
		ret = get_rif_state(rif, content_id);
	}
	
	free(rif);	
	return ret;
}


void sceNpDrmGenerateRif(char* content_id, const char* path, char* last_opened_drm_file) {
	PspRif* rif = malloc(sizeof(PspRif));
	memset(rif, 0x00, sizeof(PspRif));
	
	uint8_t version_key[0x10];
	memset(version_key, 0xFF, 0x10);
	

	// try find version_key
	if(check_pbp_file(last_opened_drm_file, content_id, version_key)) { LOG("[NOPSPEMUDRM_USER] Read version_key from last drm file (%s)\n", last_opened_drm_file); } // speedup in most cases - check last opened PBP/EDAT file
	else if(!search_psp_games_folder("ms0:/PSP/GAME", content_id, version_key)) {     // if that fails, scan the entire /PSP/GAME for this content id
		LOG("[NOPSPEMUDRM_USER] Failed to find version_key for %s\n", content_id);
		return;
	}
	LOG("[NOPSPEMUDRM_USER] version_key: ");
	logBuf(version_key, sizeof(version_key));
	
	// determine a random key from act.dat to use.
	int key_id = (int)(random_uint() % 0x80);
	LOG("[NOPSPEMUDRM_USER] key_id: %x\n", key_id);
	
	/*
	*	GENERATE THE RIF
	*/

	// set rif version
	rif->version = __builtin_bswap16(1);
	
	// set version flag
	// im using this to store if the rif was generated with fake activation or real activation
	// this way if a user without activation, suddenly activates we can detect that and regenerate all their rifs
	// 
	// the psp firmware never does anything with this besides checking its not 0x30000 or something like that anyway-

	rif->version_flag = __builtin_bswap16(is_npdrm_activated());
	
	// set license flag
	rif->license_type = 0;
	
	// set drm type
	rif->drm_type = __builtin_bswap16(2);

	// set account id
	rif->account_id = get_account_id();

	// set content id
	strncpy(rif->content_id, content_id, 0x24);
	
	// set start time
	rif->start_time = 0;

	// set end time
	rif->end_time = 0;

	// set encrypted key id
	generate_encrypted_key_id(rif->encrypted_key_id, key_id);

	// set encrypted verison key
	generate_encrypted_version_key(rif->encrypted_version_key, version_key, key_id);
	
	// set signature to all 0xFF
	memset(rif->ecdsa_signature, 0xFF, 0x28);

	// print rif info	
	write_file(path, rif, sizeof(PspRif));
		
	free(rif);	
}


int sceNpDrmTransformVersionKey(char* version_key, int src_key_type, int dst_key_type)
{
	int ret = reverse_gen_versionkey(version_key, src_key_type);
	if (ret >= 0) ret = gen_versionkey(version_key, dst_key_type); // generate a new version key of this type
	return ret;
}
