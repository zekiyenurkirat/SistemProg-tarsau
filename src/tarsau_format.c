/**
 * @file tarsau_format.c
 * @brief .sau metadata onizleme (ilk 10 bayt) donusum fonksiyonlari.
 */

#include "tarsau_format.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void tarsau_preview_to_hex(const unsigned char *preview, char *hex_out)
{
    int i;

    if (preview == NULL || hex_out == NULL) {
        return;
    }

    for (i = 0; i < TARSAU_PREVIEW_BYTES; i++) {
        sprintf(hex_out + (i * 2), "%02x", preview[i]);
    }
    hex_out[TARSAU_PREVIEW_BYTES * 2] = '\0';
}

int tarsau_hex_to_preview(const char *hex, unsigned char *preview)
{
    int i;

    if (hex == NULL || preview == NULL) {
        return -1;
    }

    if ((int)strlen(hex) != TARSAU_PREVIEW_BYTES * 2) {
        return -1;
    }

    for (i = 0; i < TARSAU_PREVIEW_BYTES; i++) {
        char pair[3];
        unsigned int value;

        pair[0] = hex[i * 2];
        pair[1] = hex[i * 2 + 1];
        pair[2] = '\0';

        if (!isxdigit((unsigned char)pair[0]) ||
            !isxdigit((unsigned char)pair[1])) {
            return -1;
        }

        if (sscanf(pair, "%02x", &value) != 1) {
            return -1;
        }
        preview[i] = (unsigned char)value;
    }

    return 0;
}
