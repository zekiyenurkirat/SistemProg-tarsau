/**
 * @file common.c
 * @brief Ortak yardimci fonksiyonlarin uygulamasi.
 */

#include "common.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void tarsau_die(const char *message)
{
    if (message != NULL) {
        fprintf(stderr, "tarsau hata: %s\n", message);
    }
    exit(TARSAU_EXIT_ERROR);
}

void tarsau_warn(const char *message)
{
    if (message != NULL) {
        fprintf(stderr, "tarsau uyari: %s\n", message);
    }
}

void *tarsau_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        tarsau_die("bellek ayirma basarisiz (malloc).");
    }
    return ptr;
}

void tarsau_free(void *ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}

int tarsau_has_sau_extension(const char *path)
{
    size_t len;
    size_t ext_len;

    if (path == NULL) {
        return 0;
    }

    len = strlen(path);
    ext_len = strlen(TARSAU_ARCHIVE_EXT);
    if (len < ext_len) {
        return 0;
    }

    return (strcmp(path + len - ext_len, TARSAU_ARCHIVE_EXT) == 0) ? 1 : 0;
}

const char *tarsau_basename(const char *path)
{
    const char *slash;

    if (path == NULL) {
        return "";
    }

    slash = strrchr(path, '/');
    if (slash != NULL) {
        return slash + 1;
    }

    return path;
}

int tarsau_mkdir_p(const char *path)
{
    char *buf;
    char *p;
    size_t len;
    struct stat st;

    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    len = strlen(path);
    buf = (char *)tarsau_malloc(len + 1U);
    memcpy(buf, path, len + 1U);

    /* Sonundaki '/' karakterlerini temizle */
    while (len > 0U && buf[len - 1U] == '/') {
        buf[len - 1U] = '\0';
        len--;
    }

    if (len == 0U) {
        tarsau_free(buf);
        return 0;
    }

    /* Kok dizin zaten var */
    if (stat(buf, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            tarsau_free(buf);
            return 0;
        }
        tarsau_free(buf);
        return -1;
    }

    /* Goreceli veya mutlak yolda parca parca dizin olustur */
    p = buf;
    if (buf[0] == '/') {
        p++;
    }

    for (; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if (buf[0] != '\0' && stat(buf, &st) != 0) {
                if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
                    tarsau_free(buf);
                    return -1;
                }
            }
            *p = '/';
        }
    }

    if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
        tarsau_free(buf);
        return -1;
    }

    tarsau_free(buf);
    return 0;
}
