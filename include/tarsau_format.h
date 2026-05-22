/**
 * @file tarsau_format.h
 * @brief .sau arsiv formati icin metadata yapisi ve sabitler.
 */

#ifndef TARSAU_FORMAT_H
#define TARSAU_FORMAT_H

#include "common.h"

#include <sys/types.h>

/**
 * @brief Arsivdeki tek bir dosyanin metadata kaydi.
 *
 * Metadata satir formati (ayirici '|'):
 *   dosyaAdi|izin_octal|boyut|ilk10bayt_hex(20 karakter)
 *
 * Ilk 10 bayt onizleme alani, dosyanin basindaki icerigi temsil eder;
 * kisa dosyalarda sifir ile doldurulur.
 */
typedef struct {
    char  name[TARSAU_MAX_FILENAME];
    mode_t mode;                          /* st_mode & 07777 */
    unsigned long size;
    unsigned char preview[TARSAU_PREVIEW_BYTES];
} tarsau_file_meta_t;

/**
 * @brief Onizleme baytlarini 20 karakterlik hex dizisine cevirir.
 * @param preview  10 baytlik onizleme.
 * @param hex_out  En az 21 bayt buffer (20 hex + null).
 */
void tarsau_preview_to_hex(const unsigned char *preview, char *hex_out);

/**
 * @brief Hex dizisini 10 baytlik onizlemeye geri cevirir.
 * @param hex      20 karakter hex.
 * @param preview  10 bayt cikis.
 * @return 0 basari, -1 gecersiz hex.
 */
int tarsau_hex_to_preview(const char *hex, unsigned char *preview);

#endif /* TARSAU_FORMAT_H */
