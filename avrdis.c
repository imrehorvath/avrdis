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
