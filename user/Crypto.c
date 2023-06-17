#include "crypto/aes.h"
#include "crypto/kirk_engine.h"
#include "Crypto.h"

#include <stdint.h>

void aes_encrypt_out(char* data_out, char* data, const char* key) {
	AES_ctx aesCtx;
	AES_set_key(&aesCtx, (uint8_t*)key, 128);
	AES_encrypt(&aesCtx, (uint8_t*)data, (uint8_t*)data_out);
}

void aes_decrypt_out(char* data_out, char* data, const char* key) {
	AES_ctx aesCtx;
	AES_set_key(&aesCtx, (uint8_t*)key, 128);
	AES_decrypt(&aesCtx, (uint8_t*)data, (uint8_t*)data_out);
}

void aes_encrypt(char* data, const char* key) {
	AES_ctx aesCtx;
	AES_set_key(&aesCtx, (uint8_t*)key, 128);
	AES_encrypt(&aesCtx, (uint8_t*)data, (uint8_t*)data);
}

void aes_decrypt(char* data, const char* key) {
	AES_ctx aesCtx;
	AES_set_key(&aesCtx, (uint8_t*)key, 128);
	AES_decrypt(&aesCtx, (uint8_t*)data, (uint8_t*)data);
}

int32_t random_int() {
	int32_t r;
	sceUtilsBufferCopyWithRange((uint8_t*)&r, sizeof(int32_t), NULL, 0, KIRK_CMD_PRNG);
	return r;
}

uint32_t random_uint() {
	uint32_t r;
	sceUtilsBufferCopyWithRange((uint8_t*)&r, sizeof(uint32_t), NULL, 0, KIRK_CMD_PRNG);
	return r;
}

