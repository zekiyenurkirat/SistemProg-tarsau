/**
 * @file extract.h
 * @brief .sau arsiv acma (-a modu) arayuzu.
 */

#ifndef TARSAU_EXTRACT_H
#define TARSAU_EXTRACT_H

/**
 * @brief .sau arsivini belirtilen dizine cikarir; izinleri geri yukler.
 * @param archive_path Kaynak .sau dosyasi.
 * @param output_dir   Hedef dizin (olusturulur).
 * @return 0 basari, -1 hata.
 */
int tarsau_extract_archive(const char *archive_path, const char *output_dir);

#endif /* TARSAU_EXTRACT_H */
