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

#include <stdlib.h>
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
