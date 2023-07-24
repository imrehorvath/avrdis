/*****************************************************************************
 * 
 * Description:
 *     Ihex parser module for the avrdis project, parses the record structure
 *     found in the ihex file and produces the data structure with the parsed
 *     data for further processing.
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
#include <ctype.h>
#include <stdint.h>

#include "avrdis.h"

#define IHEX_DATA_RECORD            0x00
#define IHEX_EOF_RECORD             0x01
#define IHEX_EXT_SEG_ADDR_RECORD    0x02

static int parsehexbyte(FILE *fp, uint8_t *b)
{
    int i, c;
    char buf[3];

    memset(buf, '\0', sizeof(buf));

    for (i = 0; i < 2; i++) {
        if (!isxdigit(c = fgetc(fp))) {
            ungetc(c, fp);
            return 0;
        }
        buf[i] = c;
    }

    if (b)
        *b = strtoul(buf, NULL, 16);

    return 1;
}

struct wordlist *parseihexfile(const char *filename)
{
    int c, lineno = 1, eofr = 0, recparsed = 0;
    uint8_t bytecount, recordtype, chksum, wordcount, wdh, wdl, wordsum, addrh, addrl;
    uint8_t extsah, extsal;
    uint16_t extsegaddr = 0;
    uint32_t wordaddr;
    struct wordlist *newword, *wlst, *firstrecwd, *lastrecwd;
    struct wordlist *firstword = NULL, *lastword = NULL;
    FILE *fp = fopen(filename, "r");

    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return NULL;
    }

    /* Main parser loop */
    for (;;) {

        /* Position after the record start ':' */
        while ((c = fgetc(fp)) != EOF && c != ':')
            ;
        if (c == EOF)
            break;

        /* When yet another record after the "End Of File" record was found, signal an error */
        if (eofr) {
            fprintf(stderr, 
                    "Record after \"End Of File\" record found at line %d in file %s.\n", 
                    lineno, filename);
            freewordlist(firstword);
            fclose(fp);
            return NULL;
        }

        /* Parse record fields */

        /* Byte count */
        if (!parsehexbyte(fp, &bytecount)) {
            fprintf(stderr, 
                    "Error parsing \"byte count\" in record at line %d in file %s.\n", 
                    lineno, filename);
            freewordlist(firstword);
            fclose(fp);
            return NULL;
        }

        /* Address high byte */
        if (!parsehexbyte(fp, &addrh)) {
            fprintf(stderr, 
                    "Error parsing \"address\" high byte in record at line %d in file %s.\n", 
                    lineno, filename);
            freewordlist(firstword);
            fclose(fp);
            return NULL;
        }

        /* Address low byte */
        if (!parsehexbyte(fp, &addrl)) {
            fprintf(stderr, 
                    "Error parsing \"address\" low byte in record at line %d in file %s.\n", 
                    lineno, filename);
            freewordlist(firstword);
            fclose(fp);
            return NULL;
        }

        /* Record type */
        if (!parsehexbyte(fp, &recordtype)) {
            fprintf(stderr, 
                    "Error parsing \"record type\" in record at line %d in file %s.\n", 
                    lineno, filename);
            freewordlist(firstword);
            fclose(fp);
            return NULL;
        }

        /* Process records based on their type */
        switch (recordtype) {

            case IHEX_DATA_RECORD:

                wordaddr = ((extsegaddr << 4) + ((addrh << 8) | addrl)) >> 1;
                firstrecwd = lastrecwd = NULL;

                /* Data word parser loop */
                for (wordcount = bytecount >> 1; wordcount; wordcount--, wordaddr++) {

                    /* Parse data word low byte */
                    if (!parsehexbyte(fp, &wdl)) {
                        fprintf(stderr, 
                                "Error parsing \"word\" low byte in record at line %d in file %s.\n", 
                                lineno, filename);
                        freewordlist(firstrecwd);
                        freewordlist(firstword);
                        fclose(fp);
                        return NULL;
                    }

                    /* Parse data word high byte */
                    if (!parsehexbyte(fp, &wdh)) {
                        fprintf(stderr, 
                                "Error parsing \"word\" high byte in record at line %d in file %s.\n", 
                                lineno, filename);
                        freewordlist(firstrecwd);
                        freewordlist(firstword);
                        fclose(fp);
                        return NULL;
                    }

                    /* Allocate word structure */
                    newword = (struct wordlist *) malloc(sizeof(struct wordlist));
                    if (!newword) {
                        fprintf(stderr, "Error allocating memory.\n");
                        freewordlist(firstrecwd);
                        freewordlist(firstword);
                        fclose(fp);
                        return NULL;
                    }

                    /* Fill word structure */
                    newword->next = NULL;
                    newword->address = wordaddr;
                    newword->word = (wdh << 8) | wdl;
 
                    /* Link the data record word into a list */
                    if (!firstrecwd)
                        firstrecwd = newword;
                    else
                        lastrecwd->next = newword;
                    lastrecwd = newword;

                } /* Data word parser loop */

                /* Parse checksum */
                if (!parsehexbyte(fp, &chksum)) {
                    fprintf(stderr, 
                            "Error parsing \"checksum\" in record at line %d in file %s.\n", 
                            lineno, filename);
                    freewordlist(firstrecwd);
                    freewordlist(firstword);
                    return NULL;
                }

                /* Calculate data word sum for checksum check */
                for (wordsum = 0, wlst = firstrecwd; wlst; wlst = wlst->next)
                    wordsum += (wlst->word >> 8) + (wlst->word & 0xff);

                /* Check checksum */
                if ((uint8_t) (bytecount + addrh + addrl + recordtype + wordsum + chksum) != 0) {
                    fprintf(stderr, "Checksum error at line %d in file %s.\n", lineno, filename);
                    freewordlist(firstrecwd);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }

                /* Add accumulated list of data words to the word list */
                if (!firstword)
                    firstword = firstrecwd;
                else
                    lastword->next = firstrecwd;
                lastword = lastrecwd;

                recparsed = 1;  /* Flag record has been parsed */
                break;

            case IHEX_EOF_RECORD:

                if (!parsehexbyte(fp, &chksum)) {
                    fprintf(stderr, 
                            "Error parsing \"checksum\" in record at line %d in file %s.\n", 
                            lineno, filename);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }

                /* Check checksum */
                if ((uint8_t) (bytecount + addrh + addrl + recordtype + chksum) != 0) {
                    fprintf(stderr, "Checksum error at line %d in file %s.\n", lineno, filename);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }

                eofr = 1;       /* Flag "End Of File" record */
                recparsed = 1;  /* Flag record has been parsed */
                break;
    
            case IHEX_EXT_SEG_ADDR_RECORD:

                /* Check if the "Extended Segment Address" record is in the first position */
                if (recparsed) {
                    fprintf(stderr, 
                            "\"Extended Segment Address\" record at line %d in file %s\n", 
                            lineno, filename);
                    freewordlist(firstword);
                    return NULL;
                }

                /* Parse the "Extended Segment Address" address */
                if (!parsehexbyte(fp, &extsah)) {
                    fprintf(stderr, 
                            "Error parsing \"segment base address\" high byte in record at line %d in file %s.\n", 
                            lineno, filename);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }
                if (!parsehexbyte(fp, &extsal)) {
                    fprintf(stderr, 
                            "Error parsing \"segment base address\" low byte in record at line %d in file %s.\n", 
                            lineno, filename);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }
    
                /* Parse checksum */
                if (!parsehexbyte(fp, &chksum)) {
                    fprintf(stderr, "Error parsing \"checksum\" in record at line %d in file %s.\n", 
                    lineno, filename);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }
 
                /* Check checksum */
                if ((uint8_t) (bytecount + addrh + addrl + recordtype + extsah + extsal + chksum) != 0) {
                    fprintf(stderr, "Checksum error at line %d in file %s.\n", lineno, filename);
                    freewordlist(firstword);
                    fclose(fp);
                    return NULL;
                }

                extsegaddr = (extsah << 8) | extsal;

                recparsed = 1;  /* Flag record has been parsed */
                break;

            default:

                /* Unsupported record types gets ignored */
                while (isxdigit(c = fgetc(fp)))
                    ;
                ungetc(c, fp);

                recparsed = 1;  /* Flag record has been parsed */
                break;

        } /* Switch case on record type */

        /* Position after the end of line */
        while ((c = fgetc(fp)) != EOF && c != '\n' && c != '\r')
            ;
        if (c == EOF)
            break;
 
        /* Increment line number when end of line found */
        if (c == '\n') {
            if ((c = fgetc(fp)) != '\r')
                ungetc(c, fp);
            lineno++;
        } else if (c == '\r') {
            lineno++;
        }

    } /* Main parser loop */

    /* Close file after parsing */
    fclose(fp);

    /* No "End Of File" record was found */
    if (!eofr) {
        fprintf(stderr, 
                "No \"End Of File\" record was found when reaching the end of file %s\n", 
                filename);
        freewordlist(firstword);
        return NULL;
    }

    /* File was successfully parsed */
    return firstword;
}
