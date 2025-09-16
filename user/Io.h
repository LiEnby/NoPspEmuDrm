#ifndef IO_H
#define IO_H 1
#include <stddef.h>
#define MAX_PATH (1028)

int write_file(const char *file, void *buf, int size);
int read_file(const char *file, void *buf, int size);
int get_file_size(const char *file);
void get_extension(const char* filename, char* ext, size_t ext_len);

#endif