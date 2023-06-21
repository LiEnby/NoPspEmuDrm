#ifndef PSP_SHELL_H
#define PSP_SHELL_H 1

#include <taihen.h>

int sceshell_module_start(tai_module_info_t tai_info);
int sceshell_module_stop();

typedef struct shell_launch_param{
	char startFolder[0x80];
	uint64_t startDate;
	uint64_t endDate;
	
	uint32_t reserved;
	uint32_t reserved2;
	uint32_t error;
	uint32_t reserved3;
	uint32_t flags;
	
} __attribute__((packed)) shell_launch_param;

#endif