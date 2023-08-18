/*****************************************************************************
 * 
 * Description:
 *     Common module for the avrdis project.
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

#include "avrdis.h"

void freewordlist(struct wordlist *wl)
{
    struct wordlist *temp;

    while (wl) {
        temp = wl;
        wl = wl->next;
        free(temp);
    }
}

struct regionstruct *allocregions(void)
{
    struct regionstruct *rs = malloc(sizeof(struct regionstruct));

    if (rs)
        memset(rs, 0, sizeof(struct regionstruct));
    return rs;
}

void freeregions(struct regionstruct *rs)
{
    struct region *r, *temp;

    for (r = rs->first; r;) {
        temp = r;
        r = r->next;
        free(temp);
    }
    free(rs);
}

int addregion(struct regionstruct *rs, uint32_t begin, uint32_t end)
{
    struct region *r = malloc(sizeof(struct region));

    if (!r)
        return 0;
    memset(r, 0, sizeof(struct region));
    r->begin = begin;
    r->end = end;

    if (!rs->first)
        rs->first = r;
    else
        rs->last->next = r;
    rs->last = r;
    return 1;
}

struct region *inregionswithprev(struct regionstruct *rs, uint32_t wordaddress, struct region **prev)
{
    struct region *r, *pr = NULL;

    for (r = rs->first; r; r = r->next) {
        if (r->begin <= wordaddress && wordaddress <= r->end) {
            if (prev)
                *prev = pr;
            return r;
        }
        pr = r;
    }
    return NULL;
}

struct region *inregions(struct regionstruct *rs, uint32_t wordaddress)
{
    return inregionswithprev(rs, wordaddress, NULL);
}

void printregions(FILE *fp, struct regionstruct *rs)
{
    struct region *r;

    for (r = rs->first; r; r = r->next)
        fprintf(fp, "0x%04x:0x%04x\n", r->begin, r->end);
}

int strcmpnocase(const char *lhs, const char *rhs)
{
    while (*lhs && *rhs) {
        if (tolower(*lhs) < tolower(*rhs))
            return -1;
        if (tolower(*lhs) > tolower(*rhs))
            return 1;
        lhs++;
        rhs++;
    }
    if (!*lhs && !*rhs)
        return 0;
    if (*lhs)
        return 1;
    return -1;
}
