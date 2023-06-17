#ifndef IO_H
#define IO_H 1

#define MAX_PATH (1028)

int WriteFile(char *file, void *buf, int size);
int ReadFile(char *file, void *buf, int size);
int GetFileSize(const char *file);


#endif