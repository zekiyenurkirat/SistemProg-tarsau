/**
 * @file validate.c
 * @brief Dosya dogrulama: metin kontrolu, boyut ve sayi limitleri.
 */

#include "validate.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int tarsau_is_text_file(const char *path)
{
    FILE *fp;
    unsigned char buffer[4096];
    size_t n;
    size_t i;

    if (path == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "tarsau hata: Dosya acilamadi: %s (%s)\n",
                path, strerror(errno));
        return -1;
    }

    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0U) {
        for (i = 0; i < n; i++) {
            unsigned char c = buffer[i];

            /* Izin verilen kontrol karakterleri: TAB, LF, CR */
            if (c == 0x09 || c == 0x0A || c == 0x0D) {
                continue;
            }
            /* Yazdirilabilir ASCII */
            if (c >= 0x20 && c <= 0x7E) {
                continue;
            }

            fclose(fp);
            fprintf(stderr,
                    "tarsau hata: Uyumsuz dosya formati (yalnizca metin): %s\n",
                    path);
            return -1;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        fprintf(stderr, "tarsau hata: Dosya okunamadi: %s\n", path);
        return -1;
    }

    fclose(fp);
    return 0;
}

int tarsau_validate_input_files(const char *const *files, int count,
                                unsigned long *total_out)
{
    int i;
    unsigned long total = 0UL;
    struct stat st;

    if (files == NULL || count <= 0) {
        fprintf(stderr, "tarsau hata: Arsivlenecek dosya listesi bos.\n");
        return -1;
    }

    if (count > TARSAU_MAX_FILES) {
        fprintf(stderr,
                "tarsau hata: En fazla %d dosya arsivlenebilir.\n",
                TARSAU_MAX_FILES);
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (files[i] == NULL || files[i][0] == '\0') {
            fprintf(stderr, "tarsau hata: Gecersiz dosya yolu.\n");
            return -1;
        }

        if (stat(files[i], &st) != 0) {
            fprintf(stderr, "tarsau hata: Dosya bulunamadi: %s (%s)\n",
                    files[i], strerror(errno));
            return -1;
        }

        if (!S_ISREG(st.st_mode)) {
            fprintf(stderr,
                    "tarsau hata: Yalnizca duzenli dosyalar arsivlenebilir: %s\n",
                    files[i]);
            return -1;
        }

        if (access(files[i], R_OK) != 0) {
            fprintf(stderr, "tarsau hata: Dosya okunamiyor: %s\n", files[i]);
            return -1;
        }

        if (st.st_size < 0) {
            fprintf(stderr, "tarsau hata: Gecersiz dosya boyutu: %s\n", files[i]);
            return -1;
        }

        if ((unsigned long)st.st_size > TARSAU_MAX_TOTAL_SIZE) {
            fprintf(stderr,
                    "tarsau hata: Tek dosya 200 MB sinirini asiyor: %s\n",
                    files[i]);
            return -1;
        }

        total += (unsigned long)st.st_size;
        if (total > TARSAU_MAX_TOTAL_SIZE) {
            fprintf(stderr,
                    "tarsau hata: Toplam boyut 200 MB sinirini asti.\n");
            return -1;
        }

        if (tarsau_is_text_file(files[i]) != 0) {
            return -1;
        }
    }

    if (total_out != NULL) {
        *total_out = total;
    }

    return 0;
}

int tarsau_validate_archive_path(const char *archive_path)
{
    struct stat st;

    if (archive_path == NULL || archive_path[0] == '\0') {
        fprintf(stderr, "tarsau hata: Arsiv yolu bos.\n");
        return -1;
    }

    if (!tarsau_has_sau_extension(archive_path)) {
        fprintf(stderr,
                "tarsau hata: Arsiv dosyasi .sau uzantili olmalidir: %s\n",
                archive_path);
        return -1;
    }

    if (stat(archive_path, &st) != 0) {
        fprintf(stderr, "tarsau hata: Arsiv bulunamadi: %s (%s)\n",
                archive_path, strerror(errno));
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "tarsau hata: Arsiv duzenli bir dosya olmali: %s\n",
                archive_path);
        return -1;
    }

    if (access(archive_path, R_OK) != 0) {
        fprintf(stderr, "tarsau hata: Arsiv okunamiyor: %s\n", archive_path);
        return -1;
    }

    return 0;
}

int tarsau_validate_extract_directory(const char *dir)
{
    if (dir == NULL || dir[0] == '\0') {
        fprintf(stderr, "tarsau hata: Hedef dizin yolu bos.\n");
        return -1;
    }

    /* Relative ve absolute path kabul edilir; olusturma extract asamasinda */
    return 0;
}
