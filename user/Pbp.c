#include "Io.h"
#include "Pbp.h"
#include "PspNpDrm.h"
#include "Log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <vitasdk.h>

int read_edat_key(const char* file, const char* content_id, char* key){
	NpPspEdat psp_edata;
	int ret = 0;

	// check the edat is atleast the size of an edat header
	size_t psp_edata_size = get_file_size(file);
	if(psp_edata_size < sizeof(NpPspEdat)) return 0;	
		
	// read the edat header
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);
	if(fd < 0) return 0;
	
	int read_size = sceIoRead(fd, &psp_edata, sizeof(NpPspEdat));
	if(read_size >= sizeof(NpPspEdat)) {
		// check magic is PSPEDAT magic
		if(memcmp(psp_edata.magic, "\0PSPEDAT", 0x8) == 0){
			// check the content id is the one were searching for.
			if(strcmp(psp_edata.content_id, content_id) == 0){
				NpPgd edatPgd;

				// read PGD from edat
				sceIoLseek(fd, psp_edata.data_offset, SCE_SEEK_SET);
				read_size = sceIoRead(fd, &edatPgd, sizeof(NpPgd));
				
				if(read_size >= sizeof(NpPgd) && memcmp(edatPgd.magic, "\0PGD", 0x4) == 0) {
					
					// calculate the edat key
					ret = sceNpDrmCalcEdatKey(&psp_edata, &edatPgd, key);
				}				
			}
		}
	}

	sceIoClose(fd);
	return ret;
}

int read_pbp_key(const char* file, const char* content_id, char* key){	
	PbpHdr pbp_header;
	int ret = 0;
	
	// check pbp sz is atleast the size of a pbp header
	size_t pbp_size = get_file_size(file);
	if(pbp_size < sizeof(PbpHdr)) return 0;

	// check file was opened successfully.	
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);
	if(fd < 0) return 0;
	
	// check total data read is > PbpHdr size.
	size_t read_size = sceIoRead(fd, &pbp_header, sizeof(PbpHdr));
	if(read_size < sizeof(PbpHdr)) return 0;
		
	// check magic is PBP magic
	if(memcmp(pbp_header.magic, "\0PBP", sizeof(pbp_header.magic)) == 0) {
		// calculate data.psar size based on pbp size - data psar size
		size_t dataPsarSz = pbp_size - pbp_header.data_psar;
		// if the data.psar size is greater than 0 -- there IS a data.psar in this eboot
		if(dataPsarSz > 0) {
			// read the first 8 bytes of data.psar to determine eboot type
			char head[0x10];
			memset(head, 0x00, sizeof(head));
			
			sceIoLseek(fd, pbp_header.data_psar, SCE_SEEK_SET);
			read_size = sceIoRead(fd, head, 0x8);
			if(read_size == 0x8) {
				// check if its a NPUMDIMG -- a psp game
				if(strcmp("NPUMDIMG", head) == 0) {
					sceIoLseek(fd, pbp_header.data_psar, SCE_SEEK_SET);
					
					NpUmdHdr np_umd_img_header;
					read_size = sceIoRead(fd, &np_umd_img_header, sizeof(NpUmdHdr));
					
					// check read size is npumdhdr size
					if(read_size == sizeof(NpUmdHdr)) {
						// check the content id matches the one were searching for.
						if(strcmp(content_id, np_umd_img_header.content_id) == 0)
							ret = sceNpDrmCalcNpUmdKey(&np_umd_img_header, key); // extract key
					}
				}
				// check if its a PSISOIMG or PSTITLEIMG -- a PS1 game
				else if((strcmp("PSISOIMG", head) == 0 || strcmp("PSTITLEI", head) == 0)) {
					NpDataPsp np_data_psp;
					
					sceIoLseek(fd, pbp_header.data_psp, SCE_SEEK_SET);
					read_size = sceIoRead(fd, &np_data_psp, sizeof(NpDataPsp));
					
					// check read size is npdatapsp size
					if(read_size == sizeof(NpDataPsp)) { 
						// check the content id matches the one were searching for.
						if(strcmp(np_data_psp.content_id, content_id) == 0){							
							NpPgd np_pgd;
							
							// locate disc info PGD 
							
							// in PSISOIMG its 0x400 bytes from the start
							if(strcmp("PSISOIMG", head) == 0)
								sceIoLseek(fd, pbp_header.data_psar+0x400, SCE_SEEK_SET);
							
							// in PSTITLEIMG its 0x200 bytes from the start
							else if(strcmp("PSTITLEI", head) == 0)
								sceIoLseek(fd, pbp_header.data_psar+0x200, SCE_SEEK_SET);
							
							// read the PGD
							sceIoRead(fd, &np_pgd, sizeof(NpPgd));
							
							// check magic is "PGD"
							if(memcmp(np_pgd.magic, "\0PGD", 0x4) == 0) 
								ret = sceNpDrmCalcPgdKey(&np_pgd, key); // extract key
						}
					}
				}
				else {
					LOG("[NOPSPEMUDRM_USER] unknown eboot type! psar header was invalid. (file: %s)\n", file);
				}
			}
			else {
				LOG("[NOPSPEMUDRM_USER] read size check failed got: %x  - (file: %s)\n", read_size, file);
			}
		}
		else {
			LOG("[NOPSPEMUDRM_USER] data.psar size is %x which is < 0  - (file: %s)\n", dataPsarSz, file);
		}
	}
	else {
		LOG("[NOPSPEMUDRM_USER] PBP Header was not \"\\0PBP\" - (file: %s)\n", file);
	}
	
	sceIoClose(fd);
	return ret;
}

int check_pbp_file(const char* file, const char* content_id, char* key) {
	char extension[0x10];
	get_extension(file, extension, sizeof(extension));

	if(strcmp(extension, ".PBP") == 0){
		return read_pbp_key(file, content_id, key);
	}
	else if(strcmp(extension, ".EDAT") == 0){
		return read_edat_key(file, content_id, key);
	}

	return 0;
	
}

int search_psp_games_folder(const char* dir_path, const char* content_id, char* key) {
	SceUID dfd = sceIoDopen(dir_path);	
	
	// As mentioned in another comment on PspNpDrm.c; 
	// the stack of PspEmu seems to be very small
	// as such, we are allocating SceIoDirent on the heap here,
	// because otherwise ScePspEmu can StackOverflow with too many games.
	char* sub_entry = malloc(MAX_PATH);
	memset(sub_entry, 0x00, MAX_PATH);
	
	int dir_read_ret = 0;
	SceIoDirent* dir = malloc(sizeof(SceIoDirent));
	int ret = 0;
	do{
		memset(dir, 0x00, sizeof(SceIoDirent));
		
		dir_read_ret = sceIoDread(dfd, dir);
		
		snprintf(sub_entry, MAX_PATH, "%s/%s", dir_path, dir->d_name);
		
		
		if(SCE_S_ISDIR(dir->d_stat.st_mode)) {
			if(search_psp_games_folder(sub_entry, content_id, key)) {
				ret = 1; 
				break;
			}
		}
		else{
			if(check_pbp_file(sub_entry, content_id, key)) {
				ret = 1; 
				break;
			}
		}
		
		
	} while(dir_read_ret > 0);
	
	
	if(dir != NULL)
		free(dir);
	
	sceIoDclose(dfd);
	
	if(sub_entry != NULL)
		free(sub_entry);
	return ret;
}
