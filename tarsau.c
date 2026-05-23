/**
 * @file tarsau.c
 * @brief tarsau - .sau formatinda metin dosyasi arsivleyici (tek dosya surumu)
 *
 * Derleme:
 *   gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -o tarsau tarsau.c
 *
 * @author Zekiye Nur Kirat - Sistem Programlama
 */

/* Linux: POSIX; Windows (MinGW): _WIN32 tanimli */
#if !defined(_WIN32) && !defined(__MINGW32__)
#define _POSIX_C_SOURCE 200809L
#endif

/* Sistem cagrilari - implicit declaration hatalarini onler */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(_WIN32) || defined(__MINGW32__)
#include <direct.h>   /* _mkdir */
#ifndef R_OK
#define R_OK 4
#endif
#endif

/* Standart kutuphaneler */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Sabitler ve veri yapilari
 * ============================================================================ */

#define TARSAU_MAX_FILES        32
#define TARSAU_MAX_TOTAL_SIZE   (200UL * 1024UL * 1024UL)
#define TARSAU_MAX_FILENAME     256
#define TARSAU_PREVIEW_BYTES    10
#define TARSAU_MAGIC_SIZE       10
#define TARSAU_DEFAULT_ARCHIVE  "a.sau"
#define TARSAU_ARCHIVE_EXT      ".sau"
#define TARSAU_MAGIC_HEADER     "SAUARCH01\n"
#define TARSAU_META_END_MARKER  "META_END"
#define TARSAU_CONTENT_MARKER   "CONTENT_START"
#define TARSAU_META_FIELD_SEP   '|'
#define TARSAU_EXIT_OK          0
#define TARSAU_EXIT_ERROR       1
#define TARSAU_MAX_ARCHIVE_FILES 32

typedef enum {
    TARSAU_MODE_NONE = 0,
    TARSAU_MODE_BUILD,
    TARSAU_MODE_EXTRACT
} tarsau_mode_t;

typedef struct {
    char  name[TARSAU_MAX_FILENAME];
    mode_t mode;
    unsigned long size;
    unsigned char preview[TARSAU_PREVIEW_BYTES];
} tarsau_file_meta_t;

typedef struct {
    tarsau_mode_t mode;
    char         *output_archive;
    char         *input_files[TARSAU_MAX_FILES];
    int           input_count;
    char         *archive_path;
    char         *extract_directory;
} tarsau_args_t;

/* ============================================================================
 * Yardimci fonksiyonlar
 * ============================================================================ */

static void tarsau_die(const char *message)
{
    if (message != NULL) {
        fprintf(stderr, "tarsau hata: %s\n", message);
    }
    exit(TARSAU_EXIT_ERROR);
}

static void tarsau_warn(const char *message)
{
    if (message != NULL) {
        fprintf(stderr, "tarsau uyari: %s\n", message);
    }
}

static void *tarsau_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        tarsau_die("bellek ayirma basarisiz (malloc).");
    }
    return ptr;
}

static void tarsau_free(void *ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}

static int tarsau_has_sau_extension(const char *path)
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

/* Windows (\) ve Unix (/) yol ayiricisi destegi */
static const char *tarsau_basename(const char *path)
{
    const char *sep;

    if (path == NULL) {
        return "";
    }
    sep = strrchr(path, '/');
    if (sep == NULL) {
        sep = strrchr(path, '\\');
    }
    if (sep != NULL) {
        return sep + 1;
    }
    return path;
}

static int tarsau_mkdir_one(const char *path)
{
#if defined(_WIN32) || defined(__MINGW32__)
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

/* Platforma gore guvenli ayristirici (MinGW 6.x icin strtok yedegi) */
static char *tarsau_strtok(char *str, const char *delim, char **saveptr)
{
#if defined(_MSC_VER)
    return strtok_s(str, delim, saveptr);
#elif defined(__MINGW32__)
    (void)saveptr;
    return strtok(str, delim);
#else
    return strtok_r(str, delim, saveptr);
#endif
}

/* Arsiv olusturulduktan sonra dosyanin diskte var oldugunu dogrular */
static int tarsau_verify_archive_file(const char *archive_path)
{
    struct stat st;

    if (stat(archive_path, &st) != 0) {
        fprintf(stderr,
                "tarsau hata: Arsiv dosyasi diskte bulunamadi: %s (%s)\n",
                archive_path, strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "tarsau hata: Cikti duzenli bir dosya degil: %s\n",
                archive_path);
        return -1;
    }
    if (st.st_size <= 0) {
        fprintf(stderr, "tarsau hata: Arsiv dosyasi bos: %s\n", archive_path);
        return -1;
    }
    return 0;
}

static int tarsau_mkdir_p(const char *path)
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

    while (len > 0U && (buf[len - 1U] == '/' || buf[len - 1U] == '\\')) {
        buf[len - 1U] = '\0';
        len--;
    }

    if (len == 0U) {
        tarsau_free(buf);
        return 0;
    }

    if (stat(buf, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            tarsau_free(buf);
            return 0;
        }
        tarsau_free(buf);
        return -1;
    }

    p = buf;
    if (buf[0] == '/' || buf[0] == '\\') {
        p++;
    }
#if defined(_WIN32) || defined(__MINGW32__)
    if (((buf[0] >= 'A' && buf[0] <= 'Z') ||
         (buf[0] >= 'a' && buf[0] <= 'z')) &&
        buf[1] == ':' && buf[2] != '\0') {
        p = buf + 2;
        if (*p == '/' || *p == '\\') {
            p++;
        }
    }
#endif

    for (; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            char sep = *p;
            *p = '\0';
            if (buf[0] != '\0' && stat(buf, &st) != 0) {
                if (tarsau_mkdir_one(buf) != 0 && errno != EEXIST) {
                    tarsau_free(buf);
                    return -1;
                }
            }
            *p = sep;
        }
    }

    if (tarsau_mkdir_one(buf) != 0 && errno != EEXIST) {
        tarsau_free(buf);
        return -1;
    }

    tarsau_free(buf);
    return 0;
}

static void tarsau_preview_to_hex(const unsigned char *preview, char *hex_out)
{
    int i;

    if (preview == NULL || hex_out == NULL) {
        return;
    }
    for (i = 0; i < TARSAU_PREVIEW_BYTES; i++) {
        sprintf(hex_out + (i * 2), "%02x", preview[i]);
    }
    hex_out[TARSAU_PREVIEW_BYTES * 2] = '\0';
}

static int tarsau_hex_to_preview(const char *hex, unsigned char *preview)
{
    int i;

    if (hex == NULL || preview == NULL) {
        return -1;
    }
    if ((int)strlen(hex) != TARSAU_PREVIEW_BYTES * 2) {
        return -1;
    }

    for (i = 0; i < TARSAU_PREVIEW_BYTES; i++) {
        char pair[3];
        unsigned int value;

        pair[0] = hex[i * 2];
        pair[1] = hex[i * 2 + 1];
        pair[2] = '\0';

        if (!isxdigit((unsigned char)pair[0]) ||
            !isxdigit((unsigned char)pair[1])) {
            return -1;
        }
        if (sscanf(pair, "%02x", &value) != 1) {
            return -1;
        }
        preview[i] = (unsigned char)value;
    }
    return 0;
}

/* ============================================================================
 * Komut satiri
 * ============================================================================ */

static char *tarsau_strdup_local(const char *s)
{
    size_t len;
    char *copy;

    if (s == NULL) {
        return NULL;
    }
    len = strlen(s);
    copy = (char *)tarsau_malloc(len + 1U);
    memcpy(copy, s, len + 1U);
    return copy;
}

static void tarsau_print_usage(const char *program_name)
{
    const char *name = (program_name != NULL) ? program_name : "tarsau";

    fprintf(stderr,
            "Kullanim:\n"
            "  Arsiv olusturma:\n"
            "    %s -b dosya1 [dosya2 ...] [-o cikti.sau]\n"
            "  Arsiv acma:\n"
            "    %s -a arsiv.sau hedef_dizin\n"
            "\n"
            "Notlar:\n"
            "  - En fazla %d metin dosyasi arsivlenebilir.\n"
            "  - Toplam boyut %lu bayti (200 MB) gecemez.\n"
            "  - -o belirtilmezse varsayilan: %s\n",
            name, name,
            TARSAU_MAX_FILES,
            (unsigned long)TARSAU_MAX_TOTAL_SIZE,
            TARSAU_DEFAULT_ARCHIVE);
}

static void tarsau_args_free(tarsau_args_t *args)
{
    int i;

    if (args == NULL) {
        return;
    }
    tarsau_free(args->output_archive);
    args->output_archive = NULL;
    tarsau_free(args->archive_path);
    args->archive_path = NULL;
    tarsau_free(args->extract_directory);
    args->extract_directory = NULL;

    for (i = 0; i < args->input_count; i++) {
        tarsau_free(args->input_files[i]);
        args->input_files[i] = NULL;
    }
    args->input_count = 0;
}

static int tarsau_parse_args(int argc, char *argv[], tarsau_args_t *out)
{
    int i;
    int saw_mode = 0;
    int saw_dash_o = 0;

    if (out == NULL || argc < 2) {
        if (argv != NULL && argv[0] != NULL) {
            tarsau_print_usage(argv[0]);
        }
        return -1;
    }

    memset(out, 0, sizeof(*out));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) {
            if (saw_mode) {
                fprintf(stderr, "tarsau hata: Birden fazla mod (-b/-a) verilemez.\n");
                return -1;
            }
            out->mode = TARSAU_MODE_BUILD;
            saw_mode = 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            if (saw_mode) {
                fprintf(stderr, "tarsau hata: Birden fazla mod (-b/-a) verilemez.\n");
                return -1;
            }
            out->mode = TARSAU_MODE_EXTRACT;
            saw_mode = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (out->mode != TARSAU_MODE_BUILD) {
                fprintf(stderr, "tarsau hata: -o yalnizca -b modunda kullanilabilir.\n");
                return -1;
            }
            if (saw_dash_o) {
                fprintf(stderr, "tarsau hata: -o parametresi tekrarlanamaz.\n");
                return -1;
            }
            if (i + 1 >= argc) {
                fprintf(stderr, "tarsau hata: -o icin dosya adi eksik.\n");
                return -1;
            }
            i++;
            out->output_archive = tarsau_strdup_local(argv[i]);
            saw_dash_o = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "tarsau hata: Bilinmeyen parametre: %s\n", argv[i]);
            return -1;
        } else if (out->mode == TARSAU_MODE_BUILD) {
            if (out->input_count >= TARSAU_MAX_FILES) {
                fprintf(stderr, "tarsau hata: En fazla %d dosya arsivlenebilir.\n",
                        TARSAU_MAX_FILES);
                return -1;
            }
            out->input_files[out->input_count++] = tarsau_strdup_local(argv[i]);
        } else if (out->mode == TARSAU_MODE_EXTRACT) {
            if (out->archive_path == NULL) {
                out->archive_path = tarsau_strdup_local(argv[i]);
            } else if (out->extract_directory == NULL) {
                out->extract_directory = tarsau_strdup_local(argv[i]);
            } else {
                fprintf(stderr, "tarsau hata: -a modunda fazla arguman: %s\n", argv[i]);
                return -1;
            }
        } else {
            fprintf(stderr, "tarsau hata: Once -b veya -a modu belirtilmelidir.\n");
            return -1;
        }
    }

    if (out->mode == TARSAU_MODE_NONE) {
        tarsau_print_usage(argv[0]);
        return -1;
    }

    if (out->mode == TARSAU_MODE_BUILD) {
        if (out->input_count == 0) {
            fprintf(stderr, "tarsau hata: -b icin en az bir dosya gerekli.\n");
            return -1;
        }
        if (out->output_archive == NULL) {
            out->output_archive = tarsau_strdup_local(TARSAU_DEFAULT_ARCHIVE);
        }
        if (!tarsau_has_sau_extension(out->output_archive)) {
            fprintf(stderr, "tarsau hata: Cikti dosyasi .sau uzantili olmalidir: %s\n",
                    out->output_archive);
            return -1;
        }
    } else if (out->archive_path == NULL || out->extract_directory == NULL) {
        fprintf(stderr, "tarsau hata: -a modu: ./tarsau -a arsiv.sau hedef_dizin\n");
        return -1;
    }

    return 0;
}

/* ============================================================================
 * Dogrulama
 * ============================================================================ */

 static int tarsau_is_text_file(const char *path)
 {
     FILE *fp;
     unsigned char buffer[4096];
     size_t n, i;
     int has_text = 0;
 
     fp = fopen(path, "rb");
     if (!fp) return -1;
 
     while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
     {
         for (i = 0; i < n; i++)
         {
             unsigned char c = buffer[i];
 
             /* NULL byte varsa kesin binary */
             if (c == 0x00)
             {
                 fclose(fp);
                 return -1;
             }
 
             /* sadece aşırı kontrol karakterlerini engelle */
             if (c < 0x09 || (c > 0x0D && c < 0x20))
             {
                 fclose(fp);
                 return -1;
             }
 
             has_text = 1;
         }
     }
 
     fclose(fp);
     return has_text ? 0 : -1;
 }

static int tarsau_validate_input_files(const char *const *files, int count,
                                       unsigned long *total_out)
{
    int i;
    unsigned long total = 0UL;
    struct stat st;

    if (files == NULL || count <= 0) {
        fprintf(stderr, "tarsau hata: Arsivlenecek dosya listesi bos.\n");
        return -1;
    }

    for (i = 0; i < count; i++) {
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
        total += (unsigned long)st.st_size;
        if (total > TARSAU_MAX_TOTAL_SIZE) {
            fprintf(stderr, "tarsau hata: Toplam boyut 200 MB sinirini asti.\n");
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

static int tarsau_validate_archive_path(const char *archive_path)
{
    struct stat st;

    if (!tarsau_has_sau_extension(archive_path)) {
        fprintf(stderr, "tarsau hata: Arsiv dosyasi .sau uzantili olmalidir: %s\n",
                archive_path);
        return -1;
    }
    if (stat(archive_path, &st) != 0) {
        fprintf(stderr, "tarsau hata: Arsiv bulunamadi: %s (%s)\n",
                archive_path, strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "tarsau hata: Arsiv duzenli bir dosya olmali.\n");
        return -1;
    }
    if (access(archive_path, R_OK) != 0) {
        fprintf(stderr, "tarsau hata: Arsiv okunamiyor: %s\n", archive_path);
        return -1;
    }
    return 0;
}

/* ============================================================================
 * Arsiv olusturma (-b)
 * ============================================================================ */

static int tarsau_fill_meta(const char *path, tarsau_file_meta_t *meta)
{
    struct stat st;
    FILE *fp;
    size_t read_count;

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
        return -1;
    }

    read_count = fread(meta->preview, 1, TARSAU_PREVIEW_BYTES, fp);
    if (read_count < TARSAU_PREVIEW_BYTES) {
        memset(meta->preview + read_count, 0, TARSAU_PREVIEW_BYTES - read_count);
    }
    fclose(fp);
    return 0;
}

static int tarsau_write_metadata(FILE *out, const tarsau_file_meta_t *meta_list,
                                 int count)
{
    int i;
    char hex_preview[TARSAU_PREVIEW_BYTES * 2 + 1];

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
    if (fprintf(out, "%s\n%s\n", TARSAU_META_END_MARKER, TARSAU_CONTENT_MARKER) < 0) {
        return -1;
    }
    return 0;
}

static int tarsau_append_file_content(FILE *out, const char *path)
{
    FILE *in;
    unsigned char *buffer;
    size_t n;
    const size_t chunk = 65536U;

    in = fopen(path, "rb");
    if (in == NULL) {
        return -1;
    }

    buffer = (unsigned char *)tarsau_malloc(chunk);
    while ((n = fread(buffer, 1, chunk, in)) > 0U) {
        if (fwrite(buffer, 1, n, out) != n) {
            tarsau_free(buffer);
            fclose(in);
            return -1;
        }
    }
    {
        int read_err = ferror(in);
        tarsau_free(buffer);
        fclose(in);
        return read_err ? -1 : 0;
    }
}

static int tarsau_build_archive(const char *const *files, int count,
                                const char *archive_path)
{
    FILE *out;
    tarsau_file_meta_t *meta_list;
    int i;

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

    if (fwrite(TARSAU_MAGIC_HEADER, 1, TARSAU_MAGIC_SIZE, out) !=
        (size_t)TARSAU_MAGIC_SIZE) {
        fprintf(stderr, "tarsau hata: Arsiv basligi yazilamadi.\n");
        fclose(out);
        remove(archive_path);
        tarsau_free(meta_list);
        return -1;
    }

    if (tarsau_write_metadata(out, meta_list, count) != 0) {
        fprintf(stderr, "tarsau hata: Metadata yazilamadi.\n");
        fclose(out);
        remove(archive_path);
        tarsau_free(meta_list);
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (tarsau_append_file_content(out, files[i]) != 0) {
            fprintf(stderr, "tarsau hata: Dosya icerigi yazilamadi: %s\n",
                    files[i]);
            fclose(out);
            remove(archive_path);
            tarsau_free(meta_list);
            return -1;
        }
    }

    if (fflush(out) != 0) {
        fprintf(stderr, "tarsau hata: Arsiv tamponu boslatilamadi.\n");
        fclose(out);
        remove(archive_path);
        tarsau_free(meta_list);
        return -1;
    }

    if (fclose(out) != 0) {
        fprintf(stderr, "tarsau hata: Arsiv kapatilamadi: %s\n", archive_path);
        remove(archive_path);
        tarsau_free(meta_list);
        return -1;
    }

    if (tarsau_verify_archive_file(archive_path) != 0) {
        tarsau_free(meta_list);
        return -1;
    }

    tarsau_free(meta_list);
    printf("tarsau: %d dosya basariyla arsivlendi -> %s\n", count, archive_path);
    fflush(stdout);
    return 0;
}

/* ============================================================================
 * Arsiv acma (-a)
 * ============================================================================ */

static int tarsau_parse_meta_line(const char *line, tarsau_file_meta_t *meta)
{
    char *work;
    char *token;
    char *saveptr = NULL;
    char hex_preview[TARSAU_PREVIEW_BYTES * 2 + 1];
    unsigned int mode_val;
    unsigned long size_val;
    int field;

    if (strncmp(line, TARSAU_META_END_MARKER, strlen(TARSAU_META_END_MARKER)) == 0) {
        return 1;
    }
    if (strncmp(line, TARSAU_CONTENT_MARKER, strlen(TARSAU_CONTENT_MARKER)) == 0) {
        return 2;
    }

    work = (char *)tarsau_malloc(strlen(line) + 1U);
    strcpy(work, line);
    memset(meta, 0, sizeof(*meta));
    field = 0;

    token = tarsau_strtok(work, "|", &saveptr);
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
    return (field < 4 || meta->name[0] == '\0') ? -1 : 0;
}

static int tarsau_verify_header(FILE *fp)
{
    char header[TARSAU_MAGIC_SIZE];

    if (fread(header, 1, TARSAU_MAGIC_SIZE, fp) != (size_t)TARSAU_MAGIC_SIZE) {
        return -1;
    }
    return (memcmp(header, TARSAU_MAGIC_HEADER, TARSAU_MAGIC_SIZE) == 0) ? 0 : -1;
}

static int tarsau_read_metadata(FILE *fp, tarsau_file_meta_t *list, int *count)
{
    char line[1024];
    int n = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t len = strlen(line);

        if (len > 0U && line[len - 1U] == '\n') {
            line[--len] = '\0';
        }
        if (len == 0U) {
            continue;
        }
        if (strncmp(line, TARSAU_CONTENT_MARKER, strlen(TARSAU_CONTENT_MARKER)) == 0) {
            *count = n;
            return 0;
        }
        if (strncmp(line, TARSAU_META_END_MARKER, strlen(TARSAU_META_END_MARKER)) == 0) {
            continue;
        }
        if (n >= TARSAU_MAX_ARCHIVE_FILES) {
            return -1;
        }
        if (tarsau_parse_meta_line(line, &list[n]) != 0) {
            return -1;
        }
        n++;
    }

    *count = n;
    return (n == 0) ? -1 : 0;
}

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

    if (chmod(out_path, meta->mode) != 0) {
        tarsau_warn("Dosya izinleri tam geri yuklenemedi; chmod basarisiz.");
    }
    return 0;
}

static int tarsau_extract_archive(const char *archive_path, const char *output_dir)
{
    FILE *fp;
    tarsau_file_meta_t *meta_list;
    int count = 0;
    int i;

    if (tarsau_validate_archive_path(archive_path) != 0) {
        return -1;
    }
    if (tarsau_mkdir_p(output_dir) != 0) {
        fprintf(stderr, "tarsau hata: Hedef dizin olusturulamadi: %s\n", output_dir);
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
        fprintf(stderr, "tarsau hata: Metadata okunamadi.\n");
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (tarsau_extract_one(fp, &meta_list[i], output_dir) != 0) {
            fclose(fp);
            tarsau_free(meta_list);
            return -1;
        }
    }

    if (fgetc(fp) != EOF) {
        fclose(fp);
        tarsau_free(meta_list);
        fprintf(stderr, "tarsau hata: Arsiv icerigi metadata ile uyumsuz.\n");
        return -1;
    }

    fclose(fp);
    tarsau_free(meta_list);
    printf("tarsau: %d dosya basariyla cikarildi -> %s\n", count, output_dir);
    return 0;
}

/* ============================================================================
 * main
 * ============================================================================ */

int main(int argc, char *argv[])
{
    tarsau_args_t args;
    int status = TARSAU_EXIT_OK;

    /* Hata mesajlari aninda gorunsun (Windows PowerShell) */
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    if (tarsau_parse_args(argc, argv, &args) != 0) {
        return TARSAU_EXIT_ERROR;
    }

    if (args.mode == TARSAU_MODE_BUILD) {
        if (tarsau_build_archive((const char *const *)args.input_files,
                               args.input_count,
                               args.output_archive) != 0) {
            status = TARSAU_EXIT_ERROR;
        }
    } else if (tarsau_extract_archive(args.archive_path, args.extract_directory) != 0) {
        status = TARSAU_EXIT_ERROR;
    }

    tarsau_args_free(&args);
    return status;
}
