#include "Io.h"
#include "Pbp.h"
#include "PspNpDrm.h"

#include "crypto/kirk_engine.h"
#include "crypto/amctrl.h"
#include "crypto/aes.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <vitasdk.h>

#define DEBUG 1 

#ifndef DEBUG
#define sceClibPrintf(...) {};
#endif

void get_extension(char* filename, char* ext){
	memset(ext, 0x00, 0x10);
	int cpy = 0;
	for(cpy = 0; ; cpy++){
		if(filename[cpy] == '.') break;
		if(filename[cpy] == '\0') break;
	}
	
	strncpy(ext, filename+cpy, 0x9);
	
	for(int i = 0; i < 0x10; i++)
		ext[i] = toupper(ext[i]);
}

int calc_pgd_key(NpPgd* pgd, char* versionkey){
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
	
	
	// get dnas key
	const char* fkey = NULL;	
	if((flag & 0x2) == 0x2)
		fkey = DNAS_KEY1A90;
	
	if((flag & 0x1) == 0x1)
		fkey = DNAS_KEY1AA0;
	
	if(fkey == NULL)
		return 0;

	// calculate the key
	int ret = 0;
	
	if(sceDrmBBMacInit(&mkey, mac_type) < SCE_OK) return 0;
	if(sceDrmBBMacUpdate(&mkey, (uint8_t*)pgd, offsetof(NpPgd, pgd_ekey))  < SCE_OK) return 0;
	if(bbmac_getkey(&mkey, pgd->pgd_ekey, versionkey) < SCE_OK) return 0;
	
	// turn key to keyindex 0
	sceNpDrmTransformVersionKey(versionkey, pgd->key_index, 0);
	
	return 1;
}


int calc_npumd_key(NpUmdHdr* hdr, char* versionkey){
	MAC_KEY mkey;
	
	// calcluate the key
	if(sceDrmBBMacInit(&mkey, 3) < SCE_OK) return 0;
	if(sceDrmBBMacUpdate(&mkey, (uint8_t*)hdr, offsetof(NpUmdHdr, header_hash)) < SCE_OK) return 0;
	if(bbmac_getkey(&mkey, hdr->header_hash, versionkey) < SCE_OK) return 0;

	// turn key to keyindex 0
	sceNpDrmTransformVersionKey(versionkey, hdr->key_index, 0);
	return 1;
}

int read_edat_key(char* file, char* contentId, char* key){
	NpPspEdat pspEdat;
	
	// check the edat is atleast the size of an edat header
	size_t edatSz = GetFileSize(file);
	if(edatSz < sizeof(NpPspEdat))
		return 0;	
	
	int ret = 0;
	
	// read the edat header
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);
	sceIoRead(fd, &pspEdat, sizeof(pspEdat));
	
	// check magic is PSPEDAT magic
	if(memcmp(pspEdat.magic, "\0PSPEDAT", sizeof(pspEdat.magic)) == 0){
		// check the content id is the one were searching for.
		if(strcmp(pspEdat.content_id, contentId) == 0){
			sceClibPrintf("[EDAT] using edat: %s\n", file);
			sceClibPrintf("[EDAT] content_id %s\n", pspEdat.content_id);

			// calculate the pgd key
			ret = calc_pgd_key(&pspEdat.pgd, key);
			
			sceClibPrintf("[EDAT] ret = 0x%x\n", ret);
		}		
	}
	
	sceIoClose(fd);
	return ret;
}

int read_pbp_key(char* file, char* contentId, char* key){	
	PbpHdr pbpHdr;
	
	// check pbp sz is atleast the size of a pbp header
	size_t pbpSz = GetFileSize(file);
	if(pbpSz < sizeof(PbpHdr)) {
		return 0;
	}
	
	int ret = 0;
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);	
	sceIoRead(fd, &pbpHdr, sizeof(PbpHdr));
	
	// check magic is PBP magic
	if(memcmp(pbpHdr.magic, "\0PBP", sizeof(pbpHdr.magic)) == 0){
		// calculate data.psar size based on pbp size - data psar size
		int dataPsarSz = pbpSz - pbpHdr.data_psar;
		
		// if the data.psar size is greater than 0 -- there IS a data.psar in this eboot
		if(dataPsarSz > 0){
			// read the first 8 bytes of data.psar to determine eboot type
			char head[0x10];
			memset(head, 0x00, sizeof(head));
			
			sceIoLseek(fd, pbpHdr.data_psar, SCE_SEEK_SET);
			int readSz = sceIoRead(fd, head, 0x8);
			if(readSz == 0x8) {
				// check if its a NPUMDIMG -- a psp game
				if(strcmp("NPUMDIMG", head) == 0){
					sceIoLseek(fd, pbpHdr.data_psar, SCE_SEEK_SET);
					
					NpUmdHdr npUmdHdr;
					readSz = sceIoRead(fd, &npUmdHdr, sizeof(NpUmdHdr));
					
					// check read size is npumdhdr size
					if(readSz == sizeof(NpUmdHdr)) {
						// check the content id matches the one were searching for.
						if(strcmp(contentId, npUmdHdr.content_id) == 0){
							// extract key
							sceClibPrintf("[PBP] using pbp: %s\n", file);
							sceClibPrintf("[PBP] content_id %s\n", npUmdHdr.content_id);
							ret = calc_npumd_key(&npUmdHdr, key);
						}
					}
				}
				// check if its a PSISOIMG ro PSTITLEIMG -- a PS1 game
				else if((strcmp("PSISOIMG", head) == 0 || strcmp("PSTITLEI", head) == 0)){
					NpDataPsp npDataPsp;
					
					sceIoLseek(fd, pbpHdr.data_psp, SCE_SEEK_SET);
					readSz = sceIoRead(fd, &npDataPsp, sizeof(NpDataPsp));
					
					// check read size is npdatapsp size
					if(readSz == sizeof(NpDataPsp)) { 
						// check the content id matches the one were searching for.
						if(strcmp(npDataPsp.content_id, contentId) == 0){
							NpPgd npPgd;
							
							// locate iso header PGD location 
							// in PSISOIMG its 0x400 bytes from the start
							if(strcmp("PSISOIMG", head) == 0)
								sceIoLseek(fd, pbpHdr.data_psar+0x400, SCE_SEEK_SET);
							
							// in PSTITLEIMG its 0x200 bytes from the start
							else if(strcmp("PSTITLEI", head) == 0)
								sceIoLseek(fd, pbpHdr.data_psar+0x200, SCE_SEEK_SET);
							
							// read the PGD
							sceIoRead(fd, &npPgd, sizeof(NpPgd));
							
							// extract the key
							ret = calc_pgd_key(&npPgd, key);
						}
					}
				}
			}
		}
	}
	
	sceIoClose(fd);
	return ret;
}

int check_file(char* file, char* contentId, char* key){
	char extension[0x10];
	get_extension(file, extension);
	
	if(strcmp(extension, ".PBP") == 0){
		return read_pbp_key(file, contentId, key);
	}
	else if(strcmp(extension, ".EDAT") == 0){
		return read_edat_key(file, contentId, key);
	}
	return 0;
	
}

int search_files(char* path, char* contentId, char* key){
	SceUID dfd = sceIoDopen(path);	
	char* pbpFile = malloc(MAX_PATH);
	memset(pbpFile, 0x00, MAX_PATH);
	
	int ret = 0;

	SceIoDirent* dir = malloc(sizeof(SceIoDirent));
	
	do{
		memset(dir, 0x00, sizeof(SceIoDirent));		
		ret = sceIoDread(dfd, dir);
		
		if(!SCE_S_ISDIR(dir->d_stat.st_mode)) {
			snprintf(pbpFile, MAX_PATH, "%s/%s", path, dir->d_name);
			if(check_file(pbpFile, contentId, key))
				return 1;
		}
		
		
	} while(ret > 0);
	
	free(dir);
	sceIoDclose(dfd);		
	
	free(pbpFile);
	
	return 0;
}

int search_games(char* contentId, char* key){
	const char* pspGameFolder = "ms0:/PSP/GAME";
	SceUID dfd = sceIoDopen(pspGameFolder);	
	char* gameFolder = malloc(MAX_PATH);
	memset(gameFolder, 0x00, MAX_PATH);
	
	int ret = 0;
	SceIoDirent* dir = malloc(sizeof(SceIoDirent));

	do{
		memset(dir, 0x00, sizeof(SceIoDirent));
		
		ret = sceIoDread(dfd, dir);
		
		if(SCE_S_ISDIR(dir->d_stat.st_mode)) {
			snprintf(gameFolder, MAX_PATH, "%s/%s", pspGameFolder, dir->d_name);
			if(search_files(gameFolder, contentId, key))
				return 1;
		}
		
		
	} while(ret > 0);
	
	free(dir);
	sceIoDclose(dfd);		
	
	free(gameFolder);
	return 0;
}
