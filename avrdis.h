/*****************************************************************************
 * 
 * Description:
 *     Header for the avrdis project, contains type definitions and prototypes
 *     used across the modules.
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
    uint32_t wordaddress;
    uint16_t word;
};

struct region {
    struct region *next;
    uint32_t begin;
    uint32_t end;
};

struct regionstruct {
    struct region *first;
    struct region *last;
};

void freewordlist(struct wordlist *wl);

struct regionstruct *allocregions(void);
void freeregions(struct regionstruct *rs);
int addregion(struct regionstruct *rs, uint32_t begin, uint32_t end);
struct region *inregionswithprev(struct regionstruct *rs, uint32_t wordaddress, struct region **prev);
struct region *inregions(struct regionstruct *rs, uint32_t wordaddress);
void printregions(FILE *fp, struct regionstruct *rs);

int strcmpnocase(const char *lhs, const char *rhs);

int ihexfile(const char *filename);
struct wordlist *parseihexfile(const char *filename);

void emitavrasm(struct wordlist *wl, struct regionstruct *enaregs, int listing);

#endif /* _AVRDIS_H_ */
