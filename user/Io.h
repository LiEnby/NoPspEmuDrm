#ifndef IO_H
#define IO_H 1
#include <stddef.h>
#define MAX_PATH (1028)

int WriteFile(const char *file, void *buf, int size);
int ReadFile(const char *file, void *buf, int size);
int GetFileSize(const char *file);
void GetExtension(const char* filename, char* ext, size_t ext_len);

#endif