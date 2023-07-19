/*****************************************************************************
 * 
 * Description:
 *     Main module for the avrdis project, handles command line arguments and
 *     calls the ihex parser and the AVR asm generator to produce the output.
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
#include <libgen.h>

#include "avrdis.h"

#define VERSION "1.0.0"

const char *command;

void usage(int status)
{
    fprintf(stderr, "AVR Disassembler for the 8-bit AVRs. v%s (c) Imre Horvath, 2023\n", VERSION);
    fprintf(stderr, "Usage: %s [options] ihexfile\n", command);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h : Show this usage info and exit.\n");
    fprintf(stderr, "  -l : Listing mode. Instead of just assembly source, include word addresses and instruction words too.\n");

    exit(status);
}

int main(int argc, char **argv)
{
    int i, listing = 0;
    const char *filename = NULL;
    struct ihex *ih;

    command = basename(argv[0]);

    if (argc < 2 || argc > 3)
        usage(1);

    for (i = 1; i < argc; i++)
        if (*argv[i] == '-')
            if (!strcmp(argv[i], "-l"))
                listing = 1;
            else if (!strcmp(argv[i], "-h"))
                usage(0);
            else
                fprintf(stderr, "Invalid option %s\n", argv[i]), usage(1);
        else
            if (filename)
                fprintf(stderr, "%s expects a single filename\n", command), usage(1);
            else
                filename = argv[i];

    if (!filename)
        fprintf(stderr, "No filename specified\n"), usage(1);

    ih = parseihexfile(filename);
    if (!ih) {
        fprintf(stderr, "Error parsing file %s\n", filename);
        return 1;
    }

    emitavrasm(ih, listing);
    freeihexdata(ih);

    return 0;
}
