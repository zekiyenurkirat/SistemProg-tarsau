/**
 * @file common.h
 * @brief tarsau projesi icin ortak sabitler, makrolar ve yardimci bildirimler.
 *
 * Bu baslik dosyasi tum moduller tarafindan paylasilan sinir degerleri,
 * hata kodlari ve genel yardimci fonksiyon prototiplerini icerir.
 */

#ifndef TARSAU_COMMON_H
#define TARSAU_COMMON_H

#include <stddef.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * Proje sinirlari (ders gereksinimleri)
 * ------------------------------------------------------------------------- */
#define TARSAU_MAX_FILES        32
#define TARSAU_MAX_TOTAL_SIZE   (200UL * 1024UL * 1024UL)  /* 200 MB */
#define TARSAU_MAX_FILENAME     256
#define TARSAU_PREVIEW_BYTES    10
#define TARSAU_MAGIC_SIZE       10

/* Varsayilan cikti arsiv adi (-o verilmezse) */
#define TARSAU_DEFAULT_ARCHIVE  "a.sau"

/* .sau dosya uzantisi */
#define TARSAU_ARCHIVE_EXT      ".sau"

/* Arsiv basligi: tam 10 bayt (icerik alani / magic) */
#define TARSAU_MAGIC_HEADER     "SAUARCH01\n"

/* Metadata ve icerik bolumu ayiricilari */
#define TARSAU_META_END_MARKER  "META_END"
#define TARSAU_CONTENT_MARKER   "CONTENT_START"

/* Metadata satir ayiricisi: dosyaAdi|izin|boyut|onizleme_hex */
#define TARSAU_META_FIELD_SEP   '|'

/* Program cikis kodlari */
#define TARSAU_EXIT_OK          0
#define TARSAU_EXIT_ERROR       1

/**
 * @brief Standart hata akisina mesaj yazar ve programi guvenli sonlandirir.
 * @param message Kullaniciya gosterilecek hata metni.
 */
void tarsau_die(const char *message);

/**
 * @brief Uyarilari stderr uzerinden yazdirir (program devam eder).
 * @param message Uyari metni.
 */
void tarsau_warn(const char *message);

/**
 * @brief Guvenli bellek ayirma; basarisizlikta die() cagirir.
 * @param size Istenecek bayt sayisi.
 * @return Ayrilan bellek blogu (asla NULL donmez).
 */
void *tarsau_malloc(size_t size);

/**
 * @brief NULL kontrolu ile bellek serbest birakma.
 * @param ptr Serbest birakilacak isaretci.
 */
void tarsau_free(void *ptr);

/**
 * @brief Dosya yolunun .sau uzantili olup olmadigini kontrol eder.
 * @param path Kontrol edilecek yol.
 * @return 1 ise .sau uzantisi var, 0 degilse.
 */
int tarsau_has_sau_extension(const char *path);

/**
 * @brief Mutlak veya goreceli hedef dizini olusturur (mkdir -p benzeri).
 * @param path Olusturulacak dizin yolu.
 * @return 0 basari, -1 hata.
 */
int tarsau_mkdir_p(const char *path);

/**
 * @brief Yolun son bilesenini (dosya adi) dondurur.
 * @param path Tam yol.
 * @return Dosya adi baslangici (path icinde pointer).
 */
const char *tarsau_basename(const char *path);

#endif /* TARSAU_COMMON_H */
