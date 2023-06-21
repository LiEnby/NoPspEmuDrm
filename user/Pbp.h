#ifndef PBP_H
#define PBP_H 1

#include <stdint.h>

typedef struct PbpHdr
{
    char magic[0x4];
	uint32_t version;
	uint32_t param_sfo;	
	uint32_t icon0_png;	
	uint32_t icon1_pmf;
	uint32_t pic0_png;
	uint32_t pic1_png;	
	uint32_t snd0_at3;	
	uint32_t data_psp;	
	uint32_t data_psar;

} __attribute__((packed)) PbpHdr;

int search_games(char* dir_path, char* contentId, char* key);

#endif