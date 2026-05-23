/**
 * @file extract.c
 * @brief .sau arsivinden dosya cikarma ve izin geri yukleme (chmod).
 */

#include "extract.h"
#include "common.h"
#include "tarsau_format.h"
#include "validate.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define TARSAU_MAX_ARCHIVE_FILES 32

static char *tarsau_strtok(char *str, const char *delim, char **saveptr)
{
    char *start;

    if (str != NULL) {
        *saveptr = str;
    }

    if (*saveptr == NULL) {
        return NULL;
    }

    start = *saveptr;

    while (**saveptr) {
        const char *d = delim;

        while (*d) {
            if (**saveptr == *d) {
                **saveptr = '\0';
                (*saveptr)++;
                return start;
            }
            d++;
        }

        (*saveptr)++;
    }

    *saveptr = NULL;
    return start;
}


/**
 * @brief Metadata satirini parse eder: ad|izin|boyut|hex_onizleme
 */
static int tarsau_parse_meta_line(const char *line, tarsau_file_meta_t *meta)
{
    char *work;
    char *token;
    char *saveptr = NULL;
    char hex_preview[TARSAU_PREVIEW_BYTES * 2 + 1];
    unsigned int mode_val;
    unsigned long size_val;
    int field;

    if (line == NULL || meta == NULL) {
        return -1;
    }

    if (strncmp(line, TARSAU_META_END_MARKER,
                strlen(TARSAU_META_END_MARKER)) == 0) {
        return 1; /* META_END */
    }
    if (strncmp(line, TARSAU_CONTENT_MARKER,
                strlen(TARSAU_CONTENT_MARKER)) == 0) {
        return 2; /* CONTENT_START */
    }

    work = (char *)tarsau_malloc(strlen(line) + 1U);
    strcpy(work, line);

    field = 0;
    token = tarsau_strtok(work, "|", &saveptr);
    memset(meta, 0, sizeof(*meta));

    while (token != NULL) {
        switch (field) {
        case 0:
            strncpy(meta->name, token, TARSAU_MAX_FILENAME - 1U);
            break;
        case 1:
            if (sscanf(token, "%o", &mode_val) != 1) {
                tarsau_free(work);
                return -1;
            }
            meta->mode = (mode_t)mode_val;
            break;
        case 2:
            if (sscanf(token, "%lu", &size_val) != 1) {
                tarsau_free(work);
                return -1;
            }
            meta->size = size_val;
            break;
        case 3:
            strncpy(hex_preview, token, sizeof(hex_preview) - 1U);
            hex_preview[sizeof(hex_preview) - 1U] = '\0';
            if (tarsau_hex_to_preview(hex_preview, meta->preview) != 0) {
                tarsau_free(work);
                return -1;
            }
            break;
        default:
            break;
        }
        field++;
        token = tarsau_strtok(NULL, "|", &saveptr);
    }

    tarsau_free(work);

    if (field < 4 || meta->name[0] == '\0') {
        return -1;
    }

    return 0;
}

/**
 * @brief Arsiv basliginin (ilk 10 bayt) gecerli olup olmadigini kontrol eder.
 */
static int tarsau_verify_header(FILE *fp)
{
    char header[TARSAU_MAGIC_SIZE];

    if (fread(header, 1, TARSAU_MAGIC_SIZE, fp) != (size_t)TARSAU_MAGIC_SIZE) {
        return -1;
    }

    if (memcmp(header, TARSAU_MAGIC_HEADER, TARSAU_MAGIC_SIZE) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Metadata bolumunu okuyup dosya listesine yukler.
 */
static int tarsau_read_metadata(FILE *fp, tarsau_file_meta_t *list, int *count)
{
    char line[1024];
    int n = 0;

    if (fp == NULL || list == NULL || count == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t len = strlen(line);

        if (len > 0U && line[len - 1U] == '\n') {
            line[len - 1U] = '\0';
            len--;
        }
        if (len == 0U) {
            continue;
        }

        if (strncmp(line, TARSAU_CONTENT_MARKER,
                    strlen(TARSAU_CONTENT_MARKER)) == 0) {
            *count = n;
            return 0;
        }

        if (strncmp(line, TARSAU_META_END_MARKER,
                    strlen(TARSAU_META_END_MARKER)) == 0) {
            continue;
        }

        if (n >= TARSAU_MAX_ARCHIVE_FILES) {
            fprintf(stderr, "tarsau hata: Arsivde cok fazla dosya kaydi.\n");
            return -1;
        }

        if (tarsau_parse_meta_line(line, &list[n]) != 0) {
            fprintf(stderr, "tarsau hata: Bozuk metadata satiri.\n");
            return -1;
        }
        n++;
    }

    if (ferror(fp)) {
        return -1;
    }

    *count = n;
    if (n == 0) {
        fprintf(stderr, "tarsau hata: Arsivde dosya metadata bulunamadi.\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Tek dosyayi arsivden okuyup hedef dizine yazar; chmod uygular.
 */
static int tarsau_extract_one(FILE *fp, const tarsau_file_meta_t *meta,
                              const char *output_dir)
{
    char out_path[TARSAU_MAX_FILENAME + 512];
    FILE *out;
    unsigned char *buffer;
    unsigned long remaining;
    size_t chunk = 65536U;
    size_t to_read;
    size_t n;

    snprintf(out_path, sizeof(out_path), "%s/%s", output_dir, meta->name);

    out = fopen(out_path, "wb");
    if (out == NULL) {
        fprintf(stderr, "tarsau hata: Cikti dosyasi olusturulamadi: %s (%s)\n",
                out_path, strerror(errno));
        return -1;
    }

    buffer = (unsigned char *)tarsau_malloc(chunk);
    remaining = meta->size;

    while (remaining > 0UL) {
        to_read = (remaining < chunk) ? (size_t)remaining : chunk;
        n = fread(buffer, 1, to_read, fp);
        if (n == 0U && remaining > 0UL) {
            tarsau_free(buffer);
            fclose(out);
            remove(out_path);
            fprintf(stderr, "tarsau hata: Arsiv icerigi eksik/bozuk.\n");
            return -1;
        }
        if (fwrite(buffer, 1, n, out) != n) {
            tarsau_free(buffer);
            fclose(out);
            remove(out_path);
            return -1;
        }
        remaining -= (unsigned long)n;
    }

    tarsau_free(buffer);
    fclose(out);

    /* Dosya izinlerini stat/chmod ile geri yukle */
    if (chmod(out_path, meta->mode) != 0) {
        tarsau_warn("Dosya izinleri tam geri yuklenemedi; chmod basarisiz.");
    }

    return 0;
}

int tarsau_extract_archive(const char *archive_path, const char *output_dir)
{
    FILE *fp;
    tarsau_file_meta_t *meta_list;
    int count = 0;
    int i;
    unsigned long expected_content = 0UL;

    if (archive_path == NULL || output_dir == NULL) {
        return -1;
    }

    if (tarsau_validate_archive_path(archive_path) != 0) {
        return -1;
    }
    if (tarsau_validate_extract_directory(output_dir) != 0) {
        return -1;
    }

    if (tarsau_mkdir_p(output_dir) != 0) {
        fprintf(stderr, "tarsau hata: Hedef dizin olusturulamadi: %s\n",
                output_dir);
        return -1;
    }

    fp = fopen(archive_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "tarsau hata: Arsiv acilamadi: %s\n", archive_path);
        return -1;
    }

    if (tarsau_verify_header(fp) != 0) {
        fclose(fp);
        fprintf(stderr, "tarsau hata: Bozuk arsiv (gecersiz 10 bayt baslik).\n");
        return -1;
    }

    meta_list = (tarsau_file_meta_t *)tarsau_malloc(
        TARSAU_MAX_ARCHIVE_FILES * sizeof(tarsau_file_meta_t));

    if (tarsau_read_metadata(fp, meta_list, &count) != 0) {
        fclose(fp);
        tarsau_free(meta_list);
        return -1;
    }

    for (i = 0; i < count; i++) {
        expected_content += meta_list[i].size;
    }

    for (i = 0; i < count; i++) {
        if (tarsau_extract_one(fp, &meta_list[i], output_dir) != 0) {
            fclose(fp);
            tarsau_free(meta_list);
            return -1;
        }
    }

    /* Icerik tamamen okundu mu (bozuk arsiv kontrolu) */
    if (fgetc(fp) != EOF) {
        fclose(fp);
        tarsau_free(meta_list);
        fprintf(stderr, "tarsau hata: Arsiv icerigi metadata ile uyumsuz.\n");
        return -1;
    }

    fclose(fp);
    tarsau_free(meta_list);

    printf("tarsau: %d dosya basariyla cikarildi -> %s\n", count, output_dir);
    (void)expected_content;
    return 0;
}
