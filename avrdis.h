/*****************************************************************************
 * 
 * Description:
 *     Header for the avrdis project, containing type definitions and proto-
 *     types used across the modules.
 * 
 * Author:
 *     Imre Horvath <imi [dot] horvath [at] gmail [dot] com> (c) 2023
 * 
 * License:
 *     GNU GPLv3
 * 
 *****************************************************************************/

#ifndef _AVRDIS_H_
#define _AVRDIS_H_

#include <stdint.h>

/* ihexparser */

struct wordlist {
    struct wordlist *next;
    uint32_t address;
    uint16_t word;
};

struct ihex {
    uint16_t extsegaddr;
    struct wordlist *words;
};

struct ihex *parseihexfile(const char *filename);
void freeihexdata(struct ihex *ih);

/* avrasmgen */

void emitavrasm(struct ihex *ih, int listing);

#endif /* _AVRDIS_H_ */
