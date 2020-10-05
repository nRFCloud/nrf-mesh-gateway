#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define STR_LEN 33

static bool util_valid(const char* str)
{
    uint8_t i;

    /* Check for valid characters 0-9, a-f, A-F */
    for (i = 0; i < STR_LEN - 1; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            continue;
        } else if (str[i] >= 'A' && str[i] <= 'F') {
            continue;
        } else if (str[i] >= 'a' && str[i] <= 'f') {
            continue;
        } else {
            return false;
        }
    }

    /* Check the length of the string is correct */
    if (str[i] != '\0') {
        return false;
    }

    return true;
}

void util_uuid_cpy(uint8_t dest[UUID_LEN], uint8_t src[UUID_LEN])
{
    memcpy(dest, src, UUID_LEN);
}

int util_uuid_cmp(const uint8_t uuid_a[UUID_LEN], const uint8_t uuid_b[UUID_LEN])
{
    return memcmp(uuid_a, uuid_b, UUID_LEN);
}

static void util_str2(const char str[STR_LEN], uint8_t arr[LEN])
{
    uint8_t i;
    char byte_str[2];

    for (i = 0; i < LEN; i++) {
        byte_str[0] = str[i*2];
        byte_str[1] = str[(i*2) + 1];
        arr[i] = (uint8_t)strtoul(byte_str, NULL, LEN);
    }
}

static void util_2str(const uint8_t arr[LEN], char str[STR_LEN])
{
    uint8_t i;
    uint8_t nyble;

    for (i = 0; i < LEN; i++) {
        /* Most significant nyble of byte */
        nyble = (arr[i] & 0xF0) >> 4;
        if (nyble < 10) {
            str[i*2] = (char)(nyble + 48);  /* For characters 0-9 */
        } else {
            str[i*2] = (char)(nyble + 87);  /* For characters a-f */
        }

        /* Least significant nyble of byte */
        nyble = arr[i] & 0x0F;
        if (nyble < 10) {
            str[(i*2) + 1] = (char)(nyble + 48);
        } else {
            str[(i*2) + 1] = (char)(nyble + 87);
        }
    }
    str[32] = '\0';
}

bool util_uuid_valid(const char *uuid_str)
{
    return util_valid(uuid_str);
}

void util_str2uuid(const char str[UUID_STR_LEN], uint8_t uuid[UUID_LEN])
{
    util_str2(str, uuid);
}

void util_uuid2str(const uint8_t uuid[UUID_LEN], char str[UUID_STR_LEN])
{
    util_2str(uuid, str);
}

bool util_key_valid(const char *key_str)
{
    return util_valid(key_str);
}

void util_str2key(const char str[KEY_STR_LEN], uint8_t key[KEY_LEN])
{
    util_str2(str, key);
}

void util_key2str(const uint8_t key[KEY_LEN], char str[KEY_STR_LEN])
{
    util_2str(key, str);
}
