/**
 * @file archive.c
 * @brief .sau arsiv olusturma: metadata + sirali icerik yazimi.
 *
 * Arsiv dosya duzeni:
 *   [0..9]   : 10 baytlik magic / icerik alani basligi (SAUARCH01\n)
 *   [meta]   : Her dosya: ad|izin|boyut|ilk10bayt_hex satirlari
 *   [satir]  : META_END
 *   [satir]  : CONTENT_START
 *   [data]   : Tum dosya icerikleri ard arda (boyut sirasiyla)
 */

#include "archive.h"
#include "tarsau_format.h"
#include "validate.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @brief Tek dosyanin metadata bilgisini stat ve onizleme ile doldurur.
 */
static int tarsau_fill_meta(const char *path, tarsau_file_meta_t *meta)
{
    struct stat st;
    FILE *fp;
    size_t read_count;

    if (path == NULL || meta == NULL) {
        return -1;
    }

    if (stat(path, &st) != 0) {
        fprintf(stderr, "tarsau hata: stat basarisiz: %s (%s)\n",
                path, strerror(errno));
        return -1;
    }

    memset(meta, 0, sizeof(*meta));
    strncpy(meta->name, tarsau_basename(path), TARSAU_MAX_FILENAME - 1U);
    meta->mode = (mode_t)(st.st_mode & 07777);
    meta->size = (unsigned long)st.st_size;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "tarsau hata: Dosya acilamadi: %s\n", path);
        return -1;
    }

    read_count = fread(meta->preview, 1, TARSAU_PREVIEW_BYTES, fp);
    if (read_count < TARSAU_PREVIEW_BYTES) {
        /* Kisa dosyalarda kalan onizleme baytlarini sifirla */
        memset(meta->preview + read_count, 0,
               TARSAU_PREVIEW_BYTES - read_count);
    }

    if (ferror(fp)) {
        fclose(fp);
        fprintf(stderr, "tarsau hata: Onizleme okunamadi: %s\n", path);
        return -1;
    }

    fclose(fp);
    return 0;
}

/**
 * @brief Metadata blogunu arsiv dosyasina yazar.
 */
static int tarsau_write_metadata(FILE *out, const tarsau_file_meta_t *meta_list,
                               int count)
{
    int i;
    char hex_preview[TARSAU_PREVIEW_BYTES * 2 + 1];

    if (out == NULL || meta_list == NULL || count <= 0) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        tarsau_preview_to_hex(meta_list[i].preview, hex_preview);

        if (fprintf(out, "%s%c%04o%c%lu%c%s\n",
                    meta_list[i].name,
                    TARSAU_META_FIELD_SEP,
                    (unsigned int)meta_list[i].mode,
                    TARSAU_META_FIELD_SEP,
                    meta_list[i].size,
                    TARSAU_META_FIELD_SEP,
                    hex_preview) < 0) {
            return -1;
        }
    }

    if (fprintf(out, "%s\n", TARSAU_META_END_MARKER) < 0) {
        return -1;
    }
    if (fprintf(out, "%s\n", TARSAU_CONTENT_MARKER) < 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Dosya icerigini arsive fwrite ile ekler (optimize tampon okuma).
 */
static int tarsau_append_file_content(FILE *out, const char *path)
{
    FILE *in;
    unsigned char *buffer;
    size_t n;
    const size_t chunk = 65536U;

    in = fopen(path, "rb");
    if (in == NULL) {
        fprintf(stderr, "tarsau hata: Icerik okunamadi: %s\n", path);
        return -1;
    }

    buffer = (unsigned char *)tarsau_malloc(chunk);
    while ((n = fread(buffer, 1, chunk, in)) > 0U) {
        if (fwrite(buffer, 1, n, out) != n) {
            tarsau_free(buffer);
            fclose(in);
            fprintf(stderr, "tarsau hata: Arsive yazma basarisiz.\n");
            return -1;
        }
    }

    if (ferror(in)) {
        tarsau_free(buffer);
        fclose(in);
        return -1;
    }

    tarsau_free(buffer);
    fclose(in);
    return 0;
}

int tarsau_build_archive(const char *const *files, int count,
                         const char *archive_path)
{
    FILE *out;
    tarsau_file_meta_t *meta_list;
    int i;

    if (files == NULL || count <= 0 || archive_path == NULL) {
        return -1;
    }

    if (tarsau_validate_input_files(files, count, NULL) != 0) {
        return -1;
    }

    meta_list = (tarsau_file_meta_t *)tarsau_malloc(
        (size_t)count * sizeof(tarsau_file_meta_t));

    for (i = 0; i < count; i++) {
        if (tarsau_fill_meta(files[i], &meta_list[i]) != 0) {
            tarsau_free(meta_list);
            return -1;
        }
    }

    out = fopen(archive_path, "wb");
    if (out == NULL) {
        fprintf(stderr, "tarsau hata: Arsiv olusturulamadi: %s (%s)\n",
                archive_path, strerror(errno));
        tarsau_free(meta_list);
        return -1;
    }

    /* Adim 16: Ilk 10 bayt icerik alani (magic header) */
    if (fwrite(TARSAU_MAGIC_HEADER, 1, TARSAU_MAGIC_SIZE, out) !=
        (size_t)TARSAU_MAGIC_SIZE) {
        fclose(out);
        tarsau_free(meta_list);
        fprintf(stderr, "tarsau hata: Arsiv basligi yazilamadi.\n");
        return -1;
    }

    if (tarsau_write_metadata(out, meta_list, count) != 0) {
        fclose(out);
        remove(archive_path);
        tarsau_free(meta_list);
        fprintf(stderr, "tarsau hata: Metadata yazilamadi.\n");
        return -1;
    }

    /* Dosya iceriklerini sirayla yaz */
    for (i = 0; i < count; i++) {
        if (tarsau_append_file_content(out, files[i]) != 0) {
            fclose(out);
            remove(archive_path);
            tarsau_free(meta_list);
            return -1;
        }
    }

    if (fclose(out) != 0) {
        tarsau_free(meta_list);
        fprintf(stderr, "tarsau hata: Arsiv kapatilamadi.\n");
        return -1;
    }

    tarsau_free(meta_list);
    printf("tarsau: %d dosya basariyla arsivlendi -> %s\n", count, archive_path);
    return 0;
}
