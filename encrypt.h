#ifndef _ENCRYPT_H
#define _ENCRYPT_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rc4.h"

enum encrypt_method {
	NO_ENCRYPT = 0,
	RC4_METHOD,
};

struct rc4_encryptor {
	struct rc4_state en_state;
	struct rc4_state de_state;
	size_t key_len;
	uint8_t key[0];
};

struct encryptor {
	enum encrypt_method enc_method;
	union {
		struct rc4_encryptor rc4_enc;
	};
};

struct encryptor *create_encryptor(enum encrypt_method method,
		const uint8_t *key, size_t key_len);
void release_encryptor(struct encryptor *encryptor);
uint8_t *encrypt(struct encryptor *encryptor, uint8_t *dest,
		uint8_t *src, size_t src_len);
uint8_t *decrypt(struct encryptor *decryptor, uint8_t *dest,
		uint8_t *src, size_t src_len);
#endif
