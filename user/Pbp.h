#ifndef PBP_H
#define PBP_H 1

#include <stdint.h>

typedef struct NpPgd{
	char magic[0x4];
	int key_index;
	int drm_type;
	int unk0;
	char encrypted_body[0x60];
	char pgd_ekey[0x10];
	char pgd_hash[0x10];
} __attribute__((packed)) NpPgd;

typedef struct NpDataPsp{
	char magic[0x4];
	char unk[0x7C];
	char unk2[0x32];
	char unk3[0xE];
	char hash[0x14];
	char reserved[0x58];
	char unk4[0x434];
	char content_id[0x30];
} __attribute__((packed)) NpDataPsp;

typedef struct NpUmdBdy{
	uint16_t sector_size; 	// 0x0800
	uint16_t unk_2;			// 0xE000
	uint32_t unk_4;
	uint32_t unk_8;
	uint32_t unk_12;
	uint32_t unk_16;
	uint32_t lba_start;
	uint32_t unk_24;
	uint32_t nsectors;
	uint32_t unk_32;
	uint32_t lba_end;
	uint32_t unk_40;
	uint32_t block_entry_offset;
	char disc_id[0x10];
	uint32_t header_start_offset;
	uint32_t unk_68;
	uint8_t unk_72;
	uint8_t bbmac_param;
	uint8_t unk_74;
	uint8_t unk_75;
	uint32_t unk_76;
	uint32_t unk_80;
	uint32_t unk_84;
	uint32_t unk_88;
	uint32_t unk_92;
} __attribute__((packed)) NpUmdBdy;

typedef struct NpUmdHdr{
	uint8_t magic[0x08];  // NPUMDIMG
	uint32_t key_index;
	uint32_t block_basis;
	uint8_t content_id[0x30];
	NpUmdBdy body;	
	uint8_t header_key[0x10];
	uint8_t data_key[0x10];
	uint8_t header_hash[0x10];
	uint8_t padding[0x8];
	uint8_t ecdsa_sig[0x28];
} __attribute__((packed)) NpUmdHdr;


typedef struct NpPspEdat{
	char magic[0x8];
	int unk0;
	int unk1;
	char content_id[0x30];
	char hash[0x10];
	int unk2;
	int unk3;
	char signature[0x38];
	NpPgd pgd;
} __attribute__((packed)) NpPspEdat;

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

int search_games(char* contentId, char* key);

#endif