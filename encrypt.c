#include "encrypt.h"

struct encryptor *create_encryptor(enum encrypt_method method,
					 const uint8_t *key, size_t key_len)
{
	struct encryptor *encryptor;

	switch (method) {
	case RC4_METHOD:
		encryptor = calloc(1, sizeof(typeof(*encryptor)) + key_len);
		encryptor->enc_method = method;
		encryptor->rc4_enc.key_len = key_len;
		memcpy(encryptor->rc4_enc.key, key, key_len);
		rc4_init(&encryptor->rc4_enc.en_state, key, key_len);
		rc4_init(&encryptor->rc4_enc.de_state, key, key_len);
		return encryptor;
	default:
		fprintf(stderr, "not support %d", method);
	}
	return NULL;
}

void release_encryptor(struct encryptor *encryptor)
{
	free(encryptor);
}

uint8_t *encrypt(struct encryptor *encryptor, uint8_t *dest,
		    uint8_t *src, size_t src_len)
{
	switch (encryptor->enc_method) {
	case RC4_METHOD:
		rc4_crypt(&encryptor->rc4_enc.en_state, src, dest, src_len);
		return dest;
	default:
		fprintf(stderr, "not support %d", encryptor->enc_method);
	}
	return NULL;
}

uint8_t *decrypt(struct encryptor *decryptor, uint8_t *dest,
		    uint8_t *src, size_t src_len)
{
	switch (decryptor->enc_method) {
	case RC4_METHOD:
		rc4_crypt(&decryptor->rc4_enc.de_state, src, dest, src_len);
		return dest;
	default:
		fprintf(stderr, "not support %d", decryptor->enc_method);
	}
	return NULL;
}
