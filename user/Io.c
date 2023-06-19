#include <vitasdk.h>

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

int GetFileSize(const char *file) {
	SceIoStat stat;
	
	int ret = sceIoGetstat(file, &stat);
	if(ret < 0)
		return ret;
	
	return (int32_t)stat.st_size;
}
