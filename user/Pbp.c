#include "Io.h"
#include "Pbp.h"
#include "PspNpDrm.h"
#include "Log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <vitasdk.h>



int read_edat_key(char* file, char* contentId, char* key){
	NpPspEdat pspEdat;
	int ret = 0;

	// check the edat is atleast the size of an edat header
	size_t edatSz = GetFileSize(file);
	if(edatSz < sizeof(NpPspEdat)) return 0;	
		
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
	int ret = 0;
	
	// check pbp sz is atleast the size of a pbp header
	size_t pbpSz = GetFileSize(file);
	if(pbpSz < sizeof(PbpHdr)) return 0;

	// check file was opened successfully.	
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);
	if(fd < 0) return 0;
	
	// check total data read is > PbpHdr size.
	size_t readSz = sceIoRead(fd, &pbpHdr, sizeof(PbpHdr));
	if(readSz < sizeof(PbpHdr)) return 0;
		
	// check magic is PBP magic
	if(memcmp(pbpHdr.magic, "\0PBP", sizeof(pbpHdr.magic)) == 0) {
		// calculate data.psar size based on pbp size - data psar size
		size_t dataPsarSz = pbpSz - pbpHdr.data_psar;
		// if the data.psar size is greater than 0 -- there IS a data.psar in this eboot
		if(dataPsarSz > 0) {
			// read the first 8 bytes of data.psar to determine eboot type
			char head[0x10];
			memset(head, 0x00, sizeof(head));
			
			sceIoLseek(fd, pbpHdr.data_psar, SCE_SEEK_SET);
			readSz = sceIoRead(fd, head, 0x8);
			if(readSz == 0x8) {
				// check if its a NPUMDIMG -- a psp game
				if(strcmp("NPUMDIMG", head) == 0) {
					sceIoLseek(fd, pbpHdr.data_psar, SCE_SEEK_SET);
					
					NpUmdHdr npUmdHdr;
					readSz = sceIoRead(fd, &npUmdHdr, sizeof(NpUmdHdr));
					
					// check read size is npumdhdr size
					if(readSz == sizeof(NpUmdHdr)) {
						// check the content id matches the one were searching for.
						if(strcmp(contentId, npUmdHdr.content_id) == 0)
							ret = sceNpDrmCalcNpUmdKey(&npUmdHdr, key); // extract key
					}
				}
				// check if its a PSISOIMG or PSTITLEIMG -- a PS1 game
				else if((strcmp("PSISOIMG", head) == 0 || strcmp("PSTITLEI", head) == 0)) {
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
							if(memcmp(npPgd.magic, "\0PGD", 0x4) == 0) 
								ret = sceNpDrmCalcPgdKey(&npPgd, key); // extract key
						}
					}
				}
				else {
					log("[NOPSPEMUDRM_USER] unknown eboot type! psar header was invalid. (file: %s)\n", file);
				}
			}
			else {
				log("[NOPSPEMUDRM_USER] read size check failed got: %x  - (file: %s)\n", readSz, file);
			}
		}
		else {
			log("[NOPSPEMUDRM_USER] data.psar size is %x which is < 0  - (file: %s)\n", dataPsarSz, file);
		}
	}
	else {
		log("[NOPSPEMUDRM_USER] PBP Header was not \"\\0PBP\" - (file: %s)\n", file);
	}
	
	sceIoClose(fd);
	return ret;
}

int check_file(char* file, char* contentId, char* key) {
	char extension[0x10];
	GetExtension(file, extension, sizeof(extension));

	if(strcmp(extension, ".PBP") == 0){
		return read_pbp_key(file, contentId, key);
	}
	else if(strcmp(extension, ".EDAT") == 0){
		return read_edat_key(file, contentId, key);
	}

	return 0;
	
}

int search_games(char* dir_path, char* contentId, char* key){
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
