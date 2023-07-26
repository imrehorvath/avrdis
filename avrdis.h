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

struct wordlist {
    struct wordlist *next;
    uint32_t address;
    uint16_t word;
};

void freewordlist(struct wordlist *wl);

int ihexfile(const char *filename);
struct wordlist *parseihexfile(const char *filename);

void emitavrasm(struct wordlist *wl, int listing);

int strcmpnocase(const char *lhs, const char *rhs);

#endif /* _AVRDIS_H_ */
