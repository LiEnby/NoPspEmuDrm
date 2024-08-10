#include <vitasdk.h>
#include <string.h>

// define toupper
int toupper(int ch);

int WriteFile(const char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT , 0777);
	if (fd < 0)
		return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}

int ReadFile(const char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file,SCE_O_RDONLY, 0777);
	if (fd < 0)
		return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}
void GetExtension(const char* filename, char* ext, size_t ext_len) {
	memset(ext, 0x00, ext_len);
	int cpy = 0;
	
	for(int i = 0; ; i++){
		if(filename[i] == '.') cpy = i;
		if(filename[i] == '\0') break;
	}
	
	if(cpy == 0) return;
	
	strncpy(ext, filename+cpy, ext_len-1);
	
	for(int i = 0; i < 0x10; i++)
		ext[i] = toupper(ext[i]);
}

int GetFileSize(const char *file) {
	SceIoStat stat;
	
	int ret = sceIoGetstat(file, &stat);
	if(ret < 0)
		return ret;
	
	return (int32_t)stat.st_size;
}
