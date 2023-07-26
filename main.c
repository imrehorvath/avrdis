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

#include "avrdis.h"

#define VERSION "1.0.0"

enum filetype {
    ERROR = -1,
    UNKNOWN,
    IHEX
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

int deterfiletype(char *filename)
{
    int check;
    char *ext = fileextension(filename);

    /* First try to determine by the extension */
    if (ext && !strcmpnocase(ext, "hex"))
        return IHEX;

    /* Second try to determine by contents */
    if ((check = ihexfile(filename)) == -1)
        return ERROR;
    if (check)
        return IHEX;

    return UNKNOWN;
}

void printusage(void)
{
    fprintf(stderr, "AVR Disassembler for the 8-bit AVRs. v%s (c) Imre Horvath, 2023\n", VERSION);
    fprintf(stderr, "Usage: %s [options] inputfile\n", command);
    fprintf(stderr, "Currently supported inputfile types are: IHEX\n");
    fprintf(stderr, "  IHEX: Intel hex, file should have an extension .hex\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h : Show this printusage info and exit.\n");
    fprintf(stderr, "  -l : Listing mode. Instead of just assembly source, include word addresses and instruction words too.\n");
}

int main(int argc, char **argv)
{
    int i, listing = 0;
    char *filename = NULL;
    struct wordlist *wl;

    command = cmdname(argv[0]);

    if (argc < 2 || argc > 3)
        printusage(), exit(1);

    for (i = 1; i < argc; i++)
        if (*argv[i] == '-')
            if (!strcmp(argv[i], "-l"))
                listing = 1;
            else if (!strcmp(argv[i], "-h"))
                printusage(), exit(0);
            else
                fprintf(stderr, "Invalid option %s\n", argv[i]), printusage(), exit(1);
        else
            if (filename)
                fprintf(stderr, "%s expects a single filename\n", command), printusage(), exit(1);
            else
                filename = argv[i];

    if (!filename)
        fprintf(stderr, "No filename specified\n"), printusage(), exit(1);

    switch (deterfiletype(filename)) {
        case IHEX:
            wl = parseihexfile(filename);
            break;
        case UNKNOWN:
            fprintf(stderr, "Unknown file type for file %s\n", filename), printusage();
            return 1;
        case ERROR:
            fprintf(stderr, "Error occured during determining type of file %s\n", filename);
            return 1;
    }

    if (!wl)
        fprintf(stderr, "No contents from parsing the file %s\n", filename), exit(1);

    emitavrasm(wl, listing);
    freewordlist(wl);

    return 0;
}
