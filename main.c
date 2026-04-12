/*****************************************************************************
 * 
 * Description:
 *     Main module for the avrdis project, handles command line arguments and
 *     calls the input parser and the AVR asm generator to produce the output.
 * 
 * Author:
 *     Imre Horvath <imi [dot] horvath [at] gmail [dot] com> (c) 2023
 * 
 * License:
 *     GNU GPLv3
 * 
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "avrdis.h"

#define VERSION "1.0.0"

enum filetype {
    FILETYPE_ERROR = -1,
    FILETYPE_UNKNOWN,
    FILETYPE_IHEX
};

const char *command;

char *cmdname(char *path)
{
    char *p = path;

    while (*p) p++;
    while (path < p && *p != '/' && *p != '\\') p--;
    if (path < p) p++;

    return p;
}

char *fileextension(char *filename)
{
    char *p = filename;

    while (*p) p++;
    while (filename < p && *p != '.' && *p != '/' && *p != '\\') p--;
    if (filename < p && *p == '.' && *(p+1) && *(p+1) != '/' && *(p+1) != '\\')
        return p+1;

    return NULL;
}

enum filetype deterfiletype(char *filename)
{
    int checks;
    char *ext = fileextension(filename);

    /* When file has an extension, determine file type by the extension */
    if (ext) {
        if (!strcmpnocase(ext, "hex"))
            return FILETYPE_IHEX;
        /* TODO: More extension types goes here below... */

    }

    /* Otherwise, try to infer type by its contents */
    if ((checks = ihexfile(filename)) == -1)
        return FILETYPE_ERROR;
    if (checks)
        return FILETYPE_IHEX;
    /* TODO: More file type-checks goes here below... */

    return FILETYPE_UNKNOWN;
}

void printusage(void)
{
    fprintf(stderr, "AVR Disassembler for the 8-bit AVRs. v%s (c) Imre Horvath, 2023\n", VERSION);
    fprintf(stderr, "Usage: %s [options] inputfile\n", command);

#define USAGE_DESCRIPTION "Currently supported inputfile types are: IHEX\n" \
"  IHEX: Intel hex format, file should have an extension .hex\n" \
"Options:\n" \
"  -h : Show this usage info and exit.\n" \
"  -l : List disabled regions, word addresses and raw instructions together with the disassembled code.\n" \
"  -e nnnn:nnnn : Enable disassembly of otherwise disabled region. Multiple options are possible.\n" \
"                 Use hex numbers. For reference, see listing of disabled regions in listing mode.\n"

    fprintf(stderr, USAGE_DESCRIPTION);
}

int main(int argc, char **argv)
{
    int res = 1;    /* Error */
    int i, listing = 0;
    char *filename = NULL;
    struct wordlist *wl = NULL;
    struct regionstruct *enaregs;
    uint32_t begin, end;

    command = cmdname(argv[0]);

    if (argc < 2) {
        printusage();
        goto err;
    }

    enaregs = allocregions();
    if (!enaregs) {
        fprintf(stderr, "Error allocating memory\n");
        goto err;
    }

    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            /* Process options */
            if (!strcmp(argv[i], "-l"))
                listing = 1;
            else if (!strcmp(argv[i], "-h")) {
                printusage();
                goto out;
            } else if (!strcmp(argv[i], "-e")) {
                if (i+1 >= argc) {
                    fprintf(stderr, "Address after option -e missing.\n");
                    goto err_reg;
                }
                i++;
                if (sscanf(argv[i], "%x:%x", &begin, &end) != 2) {
                    fprintf(stderr, "Option -e : Failed to parse a hex memory address range.\n");
                    goto err_reg;
                }
                if (begin > end) {
                    fprintf(stderr, "Option -e : Starting address must be smaller or equal than end address.\n");
                    goto err_reg;
                }
                if (!addregion(enaregs, begin, end)) {
                    fprintf(stderr, "Error allocating memory\n");
                    goto err_reg;
                }
            } else {
                fprintf(stderr, "Invalid option %s\n", argv[i]);
                goto err_reg;
            }
        } else {
            /* Process arguments */
            if (!filename)
                filename = argv[i];
            else {
                fprintf(stderr, "%s expects a single filename\n", command);
                goto err_reg;
            }
        }
    }

    if (!filename) {
        fprintf(stderr, "No filename specified\n");
        goto err_reg;
    }

    switch (deterfiletype(filename)) {
        case FILETYPE_ERROR:
            fprintf(stderr, "Error occured during determining file type %s\n", filename);
            goto err_reg;
        case FILETYPE_UNKNOWN:
            fprintf(stderr, "Unknown file type %s\n", filename);
            goto err_reg;
        case FILETYPE_IHEX:
            wl = parseihexfile(filename);
            break;
        /* TODO: Other file types goes here... */

    }

    emitavrasm(wl, enaregs, listing);
    freewordlist(wl);

out:
    res = 0;    /* Success */

err_reg:
    freeregions(enaregs);
err:
    return res;
}
