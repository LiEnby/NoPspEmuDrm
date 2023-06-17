#ifndef PSP_CRYPTO_H
#define PSP_CRYPTO_H 1

#include <stddef.h>
#include <stdint.h>

void aes_encrypt_out(char* data_out, char* data, const char* key);
void aes_decrypt_out(char* data_out, char* data, const char* key);
void aes_encrypt(char* data, const char* key);
void aes_decrypt(char* data, const char* key);
int32_t random_int();
uint32_t random_uint();

#endif