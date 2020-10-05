#ifndef UTIL_H_
#define UTIL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define LEN 16          /* 128-bit length */
#define STR_LEN 33      /* 32 hexidecimal characters null terminated */
#define UUID_LEN LEN
#define UUID_STR_LEN STR_LEN
#define KEY_LEN LEN
#define KEY_STR_LEN STR_LEN

bool util_uuid_valid(const char *uuid_str);

void util_uuid_cpy(uint8_t dest[UUID_LEN], uint8_t src[UUID_LEN]);

int util_uuid_cmp(const uint8_t uuid_a[UUID_LEN], const uint8_t uuid_b[UUID_LEN]);

void util_str2uuid(const char str[UUID_STR_LEN], uint8_t uuid[UUID_LEN]);

void util_uuid2str(const uint8_t uuid[UUID_LEN], char str[UUID_STR_LEN]);

bool util_key_valid(const char *key_str);

void util_str2key(const char str[KEY_STR_LEN], uint8_t key[KEY_LEN]);

void util_key2str(const uint8_t key[KEY_LEN], char str[KEY_STR_LEN]);


#ifdef __cplusplus
}
#endif


#endif /* UTIL_H_ */
