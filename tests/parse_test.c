#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>

int main(void)
{
    char work[256];
    char *tok;
    char *save = NULL;
    int field = 0;

    strcpy(work, "sample1.txt|0777|81|4d657268616261204475");
    tok = strtok_r(work, "|", &save);
    while (tok != NULL) {
        printf("field %d: [%s]\n", field, tok);
        field++;
        tok = strtok_r(NULL, "|", &save);
    }
    printf("total fields=%d\n", field);
    return 0;
}
