/**
 * @file args.h
 * @brief Komut satiri parametrelerinin analizi (-b, -a, -o).
 */

#ifndef TARSAU_ARGS_H
#define TARSAU_ARGS_H

#include "common.h"

/** Calisma modlari */
typedef enum {
    TARSAU_MODE_NONE = 0,
    TARSAU_MODE_BUILD,   /* -b : arsiv olustur */
    TARSAU_MODE_EXTRACT  /* -a : arsiv ac     */
} tarsau_mode_t;

/**
 * @brief Komut satirindan okunan tum parametreler.
 */
typedef struct {
    tarsau_mode_t mode;
    char         *output_archive;              /* -o (build) veya NULL -> a.sau */
    char         *input_files[TARSAU_MAX_FILES];
    int           input_count;
    char         *archive_path;                /* -a modunda arsiv dosyasi */
    char         *extract_directory;           /* -a modunda hedef dizin */
} tarsau_args_t;

/**
 * @brief argv/argc degerlerini parse eder ve tarsau_args_t doldurur.
 * @param argc Arguman sayisi.
 * @param argv Arguman dizisi.
 * @param out  Doldurulacak yapi.
 * @return 0 basari, -1 hata (mesaj yazdirilir).
 */
int tarsau_parse_args(int argc, char *argv[], tarsau_args_t *out);

/**
 * @brief Kullanim bilgisini stderr'e yazar.
 * @param program_name argv[0].
 */
void tarsau_print_usage(const char *program_name);

/**
 * @brief args yapisi icindeki malloc edilmis alanlari serbest birakir.
 * @param args Temizlenecek yapi.
 */
void tarsau_args_free(tarsau_args_t *args);

#endif /* TARSAU_ARGS_H */
