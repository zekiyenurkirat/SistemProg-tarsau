/**
 * @file archive.h
 * @brief .sau arsiv olusturma (-b modu) arayuzu.
 */

#ifndef TARSAU_ARCHIVE_H
#define TARSAU_ARCHIVE_H

#include "args.h"

/**
 * @brief Verilen metin dosyalarini .sau formatinda arsivler.
 *
 * Arsiv yapisi:
 *   1) Ilk 10 bayt: SAU magic/header (icerik alani)
 *   2) Metadata blogu: her dosya icin ad|izin|boyut|ilk10bayt_hex satiri
 *   3) META_END ayiricisi
 *   4) CONTENT_START ayiricisi
 *   5) Dosya icerikleri sirayla (ham binary degil, metin baytlari)
 *
 * @param files   Girdi dosya yollari.
 * @param count   Dosya sayisi.
 * @param archive_path Cikti .sau dosyasi.
 * @return 0 basari, -1 hata.
 */
int tarsau_build_archive(const char *const *files, int count,
                         const char *archive_path);

#endif /* TARSAU_ARCHIVE_H */
