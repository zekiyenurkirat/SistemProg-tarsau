/**
 * @file validate.h
 * @brief Giris dosyalarinin dogrulanmasi (metin, sayi, boyut).
 */

#ifndef TARSAU_VALIDATE_H
#define TARSAU_VALIDATE_H

#include "args.h"

/**
 * @brief Tek bir dosyanin metin (ASCII) dosya olup olmadigini kontrol eder.
 *
 * Tum baytlar yazdirilabilir ASCII (32-126) veya satir sonu/tab
 * karakterleri (9, 10, 13) olmalidir. Binar dosyalar reddedilir.
 *
 * @param path Dosya yolu.
 * @return 0 metin dosyasi, -1 degilse veya okuma hatasi.
 */
int tarsau_is_text_file(const char *path);

/**
 * @brief Arsivlenecek dosya listesini ders kurallarina gore dogrular.
 *
 * - En fazla 32 dosya
 * - Toplam boyut 200 MB'i asmamali
 * - Her dosya metin dosyasi olmali
 * - Dosya mevcut ve okunabilir olmali
 *
 * @param files   Dosya yolu dizisi.
 * @param count   Dosya sayisi.
 * @param total_out Toplam boyut (bayt) cikis parametresi; NULL olabilir.
 * @return 0 tum kontroller gecti, -1 aksi halde.
 */
int tarsau_validate_input_files(const char *const *files, int count,
                                unsigned long *total_out);

/**
 * @brief Arsiv dosyasinin .sau uzantili ve erisilebilir oldugunu kontrol eder.
 * @param archive_path Arsiv yolu.
 * @return 0 gecerli, -1 degilse.
 */
int tarsau_validate_archive_path(const char *archive_path);

/**
 * @brief Hedef cikarma dizini yolunu dogrular (relative/absolute).
 * @param dir Dizin yolu.
 * @return 0 gecerli, -1 degilse.
 */
int tarsau_validate_extract_directory(const char *dir);

#endif /* TARSAU_VALIDATE_H */
