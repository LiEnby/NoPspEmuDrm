#include "Io.h"
#include "Pbp.h"
#include "PspNpDrm.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <vitasdk.h>

#define DEBUG 1 

#ifndef DEBUG
#define //sceClibPrintf(...) {};
#endif

void get_extension(char* filename, char* ext){
	memset(ext, 0x00, 0x10);
	int cpy = 0;
	
	for(int i = 0; ; i++){
		if(filename[i] == '.') cpy = i;
		if(filename[i] == '\0') break;
	}
	
	if(cpy == 0) return;
	
	strncpy(ext, filename+cpy, 0x9);
	
	for(int i = 0; i < 0x10; i++)
		ext[i] = toupper(ext[i]);
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
	if(fd < 0) return 0;
	
	int read_sz = sceIoRead(fd, &pspEdat, sizeof(NpPspEdat));
	if(read_sz >= sizeof(NpPspEdat)) {
		// check magic is PSPEDAT magic
		if(memcmp(pspEdat.magic, "\0PSPEDAT", 0x8) == 0){
			// check the content id is the one were searching for.
			if(strcmp(pspEdat.content_id, contentId) == 0){
				NpPgd edatPgd;

				// read PGD from edat
				sceIoLseek(fd, pspEdat.data_offset, SCE_SEEK_SET);
				read_sz = sceIoRead(fd, &edatPgd, sizeof(NpPgd));
				
				if(read_sz >= sizeof(NpPgd) && memcmp(edatPgd.magic, "\0PGD", 0x4) == 0) {
					//sceClibPrintf("[VKEY] FOUND (PSP) EDAT: %s\n", file);
					
					// calculate the edat key
					ret = sceNpDrmCalcEdatKey(&pspEdat, &edatPgd, key);
				}				
			}
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
							//sceClibPrintf("[VKEY] FOUND (PSP) EBOOT: %s\n", file);
							// extract key
							ret = sceNpDrmCalcNpUmdKey(&npUmdHdr, key);
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
							
							// locate disc info PGD 
							
							// in PSISOIMG its 0x400 bytes from the start
							if(strcmp("PSISOIMG", head) == 0)
								sceIoLseek(fd, pbpHdr.data_psar+0x400, SCE_SEEK_SET);
							
							// in PSTITLEIMG its 0x200 bytes from the start
							else if(strcmp("PSTITLEI", head) == 0)
								sceIoLseek(fd, pbpHdr.data_psar+0x200, SCE_SEEK_SET);
							
							// read the PGD
							sceIoRead(fd, &npPgd, sizeof(NpPgd));
							
							// check magic is "PGD"
							if(memcmp(npPgd.magic, "\0PGD", 0x4) == 0) {
								//sceClibPrintf("[VKEY] FOUND (PS1) EBOOT: %s\n", file);
								
								// extract the key
								ret = sceNpDrmCalcPgdKey(&npPgd, key);
							}
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

int search_games(char* dir_path, char* contentId, char* key){
	//const char* pspGameFolder = "ms0:/PSP/GAME";
	SceUID dfd = sceIoDopen(dir_path);	
	char* subEnt = malloc(MAX_PATH);
	memset(subEnt, 0x00, MAX_PATH);
	
	int dir_read_ret = 0;
	SceIoDirent* dir = malloc(sizeof(SceIoDirent));
	int ret = 0;
	do{
		memset(dir, 0x00, sizeof(SceIoDirent));
		
		dir_read_ret = sceIoDread(dfd, dir);
		
		snprintf(subEnt, MAX_PATH, "%s/%s", dir_path, dir->d_name);
		
		
		if(SCE_S_ISDIR(dir->d_stat.st_mode)) {
			if(search_games(subEnt, contentId, key)) {
				ret = 1; break;
			}
		}
		else{
			if(check_file(subEnt, contentId, key)) {
				ret = 1; break;
			}
		}
		
		
	} while(dir_read_ret > 0);
	
	
	free(dir);
	sceIoDclose(dfd);
	
	free(subEnt);
	return ret;
}
