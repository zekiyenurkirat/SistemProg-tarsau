#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TARSAU_PREVIEW_BYTES 10

static int tarsau_hex_to_preview(const char *hex, unsigned char *preview)
{
    int i;
    if ((int)strlen(hex) != TARSAU_PREVIEW_BYTES * 2) {
        printf("len=%zu\n", strlen(hex));
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
            printf("bad pair %s\n", pair);
            return -1;
        }
        if (sscanf(pair, "%02x", &value) != 1) {
            return -1;
        }
        preview[i] = (unsigned char)value;
    }
    return 0;
}

int main(void)
{
    unsigned char p[10];
    const char *h = "4d657268616261204475";
    printf("ret=%d\n", tarsau_hex_to_preview(h, p));
    return 0;
}
