/**
 * @file args.c
 * @brief Komut satiri parametre analizi.
 *
 * Desteklenen kullanim:
 *   ./tarsau -b dosya1 dosya2 ... [-o cikti.sau]
 *   ./tarsau -a arsiv.sau hedef_dizin
 */

#include "args.h"

#include <stdlib.h>
#include <string.h>

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

void tarsau_print_usage(const char *program_name)
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

void tarsau_args_free(tarsau_args_t *args)
{
    int i;

    if (args == NULL) {
        return;
    }

    if (args->output_archive != NULL) {
        tarsau_free(args->output_archive);
        args->output_archive = NULL;
    }
    if (args->archive_path != NULL) {
        tarsau_free(args->archive_path);
        args->archive_path = NULL;
    }
    if (args->extract_directory != NULL) {
        tarsau_free(args->extract_directory);
        args->extract_directory = NULL;
    }

    for (i = 0; i < args->input_count; i++) {
        if (args->input_files[i] != NULL) {
            tarsau_free(args->input_files[i]);
            args->input_files[i] = NULL;
        }
    }
    args->input_count = 0;
}

int tarsau_parse_args(int argc, char *argv[], tarsau_args_t *out)
{
    int i;
    int saw_mode;
    int saw_dash_o;

    if (out == NULL || argc < 2) {
        if (argv != NULL && argv[0] != NULL) {
            tarsau_print_usage(argv[0]);
        }
        return -1;
    }

    memset(out, 0, sizeof(*out));
    out->mode = TARSAU_MODE_NONE;
    saw_mode = 0;
    saw_dash_o = 0;

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
        } else {
            /* Konumsal arguman */
            if (out->mode == TARSAU_MODE_BUILD) {
                if (out->input_count >= TARSAU_MAX_FILES) {
                    fprintf(stderr,
                            "tarsau hata: En fazla %d dosya arsivlenebilir.\n",
                            TARSAU_MAX_FILES);
                    return -1;
                }
                out->input_files[out->input_count++] =
                    tarsau_strdup_local(argv[i]);
            } else if (out->mode == TARSAU_MODE_EXTRACT) {
                if (out->archive_path == NULL) {
                    out->archive_path = tarsau_strdup_local(argv[i]);
                } else if (out->extract_directory == NULL) {
                    out->extract_directory = tarsau_strdup_local(argv[i]);
                } else {
                    fprintf(stderr,
                            "tarsau hata: -a modunda fazla arguman: %s\n",
                            argv[i]);
                    return -1;
                }
            } else {
                fprintf(stderr,
                        "tarsau hata: Once -b veya -a modu belirtilmelidir.\n");
                return -1;
            }
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
            fprintf(stderr,
                    "tarsau hata: Cikti dosyasi .sau uzantili olmalidir: %s\n",
                    out->output_archive);
            return -1;
        }
    } else if (out->mode == TARSAU_MODE_EXTRACT) {
        if (out->archive_path == NULL || out->extract_directory == NULL) {
            fprintf(stderr,
                    "tarsau hata: -a modu: ./tarsau -a arsiv.sau hedef_dizin\n");
            return -1;
        }
    }

    return 0;
}
