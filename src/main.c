/**
 * @file main.c
 * @brief tarsau - Ozel .sau formatinda metin dosyasi arsivleyici.
 *
 * Sistem Programlama dersi akademik projesi.
 * Linux/Unix uyumlu, moduler C uygulamasi.
 *
 * @author Zekiye Nur Kirat
 */

#include "archive.h"
#include "args.h"
#include "common.h"
#include "extract.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Program giris noktasi: parametreleri isler ve moda gore calisir.
 */
int main(int argc, char *argv[])
{
    tarsau_args_t args;
    int status = TARSAU_EXIT_OK;

    if (tarsau_parse_args(argc, argv, &args) != 0) {
        return TARSAU_EXIT_ERROR;
    }

    switch (args.mode) {
    case TARSAU_MODE_BUILD:
        if (tarsau_build_archive(
                (const char *const *)args.input_files,
                args.input_count,
                args.output_archive) != 0) {
            status = TARSAU_EXIT_ERROR;
        }
        break;

    case TARSAU_MODE_EXTRACT:
        if (tarsau_extract_archive(args.archive_path,
                                   args.extract_directory) != 0) {
            status = TARSAU_EXIT_ERROR;
        }
        break;

    default:
        tarsau_print_usage(argv[0]);
        status = TARSAU_EXIT_ERROR;
        break;
    }

    tarsau_args_free(&args);
    return status;
}
