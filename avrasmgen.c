/*****************************************************************************
 * 
 * Description:
 *     AVR asm generator module for the avrdis project, generates either the
 *     assembly source, or the listing with word addresses and instruction
 *     words plus the source, from the data structure coming from the parser.
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
#include <stdint.h>

#include "avrdis.h"

#define DEFAULT_LABELS_SIZE 64

struct labelstruct {
    uint32_t address;
    char *label;
};

static struct labelstruct *labels = NULL;
static size_t labelscount = 0;
static size_t labelssize = 0;

static void freelabels(void)
{
    size_t i;
    char *label;

    if (labels) {
        for (i = 0; i < labelscount; i++)
            if ((label = labels[i].label))
                free(label);
        free(labels);
    }

    labels = NULL;
    labelscount = labelssize = 0;
}

static int addlabeladdr(uint32_t addr)
{
    struct labelstruct *newlabels;
    size_t i;

    /* Dynamic size management */
    /* Initial allocation */
    if (!labelssize) {
        labels = calloc(DEFAULT_LABELS_SIZE, sizeof(struct labelstruct));
        if (!labels) {
            fprintf(stderr, "Error allocating memory.\n");
            return 0;
        }
        labelscount = 0;
        labelssize = DEFAULT_LABELS_SIZE;

    /* Reallocate when limit reached, increase capacity by DEFAULT_LABELS_SIZE */
    } else if (labelscount >= labelssize) {
        newlabels = realloc(labels, (labelssize + DEFAULT_LABELS_SIZE) * sizeof(struct labelstruct));
        if (!newlabels) {
            fprintf(stderr, "Error allocating memory.\n");
            return 0;
        }
        labels = newlabels;
        labelssize += DEFAULT_LABELS_SIZE;
    }

    /* Skip already added addresses */
    /* Not yet sorted at this point, so do linear search */
    for (i = 0; i < labelscount; i++)
        if (labels[i].address == addr)
            return 1;

    /* Add address in the order its found */
    labels[labelscount++].address = addr;

    return 1;
}

static int comp(const void *lhs, const void *rhs)
{
    const struct labelstruct *l = lhs;
    const struct labelstruct *r = rhs;

    if (l->address < r->address)
        return -1;

    if (l->address > r->address)
        return 1;

    return 0;    
}

static const char *lookuplabel(uint32_t addr)
{
    struct labelstruct key = { .address = addr };
    struct labelstruct *elem;

    /* Binary search on sorted array */
    elem = bsearch(&key, labels, labelscount, sizeof(struct labelstruct), comp);
    if (elem)
        return elem->label;

    return NULL;
}

static int condrelbranch(uint16_t word, uint32_t address, const char **mnemonic, uint32_t *targetaddr)
{
    const char *s;
    int8_t offset;

    switch (word & 0xfc07) {
        case 0xf400:
            s = "brcc";
            break;
        case 0xf000:
            s = "brcs";
            break;
        case 0xf001:
            s = "breq";
            break;
        case 0xf404:
            s = "brge";
            break;
        case 0xf405:
            s = "brhc";
            break;
        case 0xf005:
            s = "brhs";
            break;
        case 0xf407:
            s = "brid";
            break;
        case 0xf007:
            s = "brie";
            break;
        case 0xf004:
            s = "brlt";
            break;
        case 0xf002:
            s = "brmi";
            break;
        case 0xf401:
            s = "brne";
            break;
        case 0xf402:
            s = "brpl";
            break;
        case 0xf406:
            s = "brtc";
            break;
        case 0xf006:
            s = "brts";
            break;
        case 0xf403:
            s = "brvc";
            break;
        case 0xf003:
            s = "brvs";
            break;
        default:
            return 0;
    }

    if (mnemonic)
        *mnemonic = s;

    if (targetaddr) {

        offset = (word & 0x03f8) >> 3;
        /* In case of negative offset, complete the 8 bit signed representation */
        if (word & 0x0200)
            offset |= 0x80; /* Two's complement */

        *targetaddr = address + offset + 1;
    }

    return 1;
}

static int relcalljmp(uint16_t word, uint32_t address, const char **mnemonic, uint32_t *targetaddr)
{
    const char *s;
    int16_t offset;

    switch (word & 0xf000) {
        case 0xd000:
            s = "rcall";
            break;
        case 0xc000:
            s = "rjmp";
            break;
        default:
            return 0;
    }

    if (mnemonic)
        *mnemonic = s;

    if (targetaddr) {

        offset = word & 0x0fff;
        /* In case of negative offset, complete the 16 bit signed representation */
        if (word & 0x0800)
            offset |= 0xf000;   /* Two's complement */
       
        *targetaddr = address + offset + 1;
    }

    return 1;
}

static int longcalljmp(struct wordlist *wl, const char **mnemonic, uint32_t *targetaddr)
{
    const char *s;

    switch (wl->word & 0xfe0e) {
        case 0x940e:
            s = "call";
            break;
        case 0x940c:
            s = "jmp";
            break;
        default:
            return 0;
    }

    if (wl->next == NULL) {
        fprintf(stderr, "2nd word of 32-bit opcode after %05x missing\n", wl->address);
        return 0;
    }

    if (mnemonic)
        *mnemonic = s;

    if (targetaddr)
        *targetaddr = ((((wl->word & 0x01f0) >> 3) | (wl->word & 0x0001)) << 16) | wl->next->word;

    return 1;
}

static int adc(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x1c00)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int add(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x0c00)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int adiw(uint16_t word, int *d, int *K)
{
    if ((word & 0xff00) != 0x9600)
        return 0;

    if (d)
        *d = (word & 0x0030) >> 4;    

    if (K)
        *K = ((word & 0x00c0) >> 2) | (word & 0x000f);

    return 1;
}

static int and(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x2000)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int andi(uint16_t word, int *d, int *K)
{
    if ((word & 0xf000) != 0x7000)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (K)
        *K = ((word & 0x0f00) >> 4) | (word & 0x000f);

    return 1;
}

static int asr(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9405)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int bld(uint16_t word, int *d, int *b)
{
    if ((word & 0xfe08) != 0xf800)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int bst(uint16_t word, int *r, int *b)
{
    if ((word & 0xfe08) != 0xfa00)
        return 0;

    if (r)
        *r = (word & 0x01f0) >> 4;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int cbi(uint16_t word, int *A, int *b)
{
    if ((word & 0xff00) != 0x9800)
        return 0;

    if (A)
        *A = (word & 0x00f8) >> 3;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int com(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9400)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int cp(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x1400)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int cpc(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x0400)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int cpi(uint16_t word, int *d, int *K)
{
    if ((word & 0xf000) != 0x3000)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (K)
        *K = ((word & 0x0f00) >> 4) | (word & 0x000f);

    return 1;
}

static int cpse(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x1000)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int dec(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x940a)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int des(uint16_t word, int *K)
{
    if ((word & 0xff0f) != 0x940b)
        return 0;

    if (K)
        *K = (word & 0x00f0) >> 4;

    return 1;
}

static int elpm(uint16_t word, int *d, const char **operand)
{
    const char *t;

    if (word == 0x95d8) {

        if (operand)
            *operand = "";

        return 1;
    }

    switch (word & 0xfe0f) {
        case 0x9006:
            t = "Z";
            break;
        case 0x9007:
            t = "Z+";
            break;
        default:
            return 0;
    }

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (operand)
        *operand = t;

    return 1;
}

static int eor(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x2400)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int fmul(uint16_t word, int *d, int *r)
{
    if ((word & 0xff88) != 0x0308)
        return 0;

    if (d)
        *d = (word & 0x0070) >> 4;

    if (r)
        *r = word & 0x0007;

    return 1;
}

static int fmuls(uint16_t word, int *d, int *r)
{
    if ((word & 0xff88) != 0x0380)
        return 0;

    if (d)
        *d = (word & 0x0070) >> 4;

    if (r)
        *r = word & 0x0007;

    return 1;
}

static int fmulsu(uint16_t word, int *d, int *r)
{
    if ((word & 0xff88) != 0x0388)
        return 0;

    if (d)
        *d = (word & 0x0070) >> 4;

    if (r)
        *r = word & 0x0007;

    return 1;
}

static int in(uint16_t word, int *d, int *A)
{
    if ((word & 0xf800) != 0xb000)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (A)
        *A = ((word & 0x0600) >> 5) | (word & 0x000f);

    return 1;
}

static int inc(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9403)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int lac(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9206)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int las(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9205)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int lat(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9207)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int ld(uint16_t word, int *d, const char **operand, int *q)
{
    const char *t;
    uint8_t offset = 0;

    switch (word & 0xfe0f) {
        case 0x900c:
            t = "X";
            break;
        case 0x900d:
            t = "X+";
            break;
        case 0x900e:
            t = "-X";
            break;
        case 0x9009:
            t = "Y+";
            break;
        case 0x900a:
            t = "-Y";
            break;
        case 0x9001:
            t = "Z+";
            break;
        case 0x9002:
            t = "-Z";
            break;
        default:
            switch (word & 0xd208) {
                case 0x8008:
                    offset = ((word & 0x2000) >> 8) | ((word & 0x0c00) >> 7) | (word & 0x0007);
                    t = "Y";
                    break;
                case 0x8000:
                    offset = ((word & 0x2000) >> 8) | ((word & 0x0c00) >> 7) | (word & 0x0007);
                    t = "Z";
                    break;
                default:
                    return 0;
            }
            break;
    }

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (operand)
        *operand = t;

    if (q)
        *q = offset;

    return 1;
}

static int ldi(uint16_t word, int *d, int *K)
{
    if ((word & 0xf000) != 0xe000)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (K)
        *K = ((word & 0x0f00) >> 4) | (word & 0x000f);

    return 1;
}

static int lds(struct wordlist *wl, int *skip, int *d, int *k)
{
    if ((wl->word & 0xfe0f) == 0x9000) {

        if (wl->next == NULL) {
            fprintf(stderr, "2nd word of 32-bit opcode after %05x missing\n", wl->address);
            return 0;
        }

        if (skip)
            *skip = 1; /* 32-bit opcode, need to skip next word */

        if (d)
            *d = (wl->word & 0x01f0) >> 4;

        if (k)
            *k = wl->next->word;

        return 1;
    }

    if ((wl->word & 0xf800) == 0xa000) {

        if (skip)
            *skip = 0; /* 16-bit opcode, no need to skip next word */

        if (d)
            *d = (wl->word & 0x00f0) >> 4;

        if (k)
            *k = ((wl->word & 0x0700) >> 4) | (wl->word & 0x000f);

        return 1;
    }

    return 0;
}

static int lpm(uint16_t word, int *d, const char **operand)
{
    const char *t;

    if (word == 0x95c8) {

        if (operand)
            *operand = "";

        return 1;
    }

    switch (word & 0xfe0f) {
        case 0x9004:
            t = "Z";
            break;
        case 0x9005:
            t = "Z+";
            break;
        default:
            return 0;
    }

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (operand)
        *operand = t;

    return 1;
}

static int lsr(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9406)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int mov(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x2c00)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int movw(uint16_t word, int *d, int *r)
{
    if ((word & 0xff00) != 0x0100)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (r)
        *r = word & 0x000f;

    return 1;
}

static int mul(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x9c00)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int muls(uint16_t word, int *d, int *r)
{
    if ((word & 0xff00) != 0x0200)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (r)
        *r = word & 0x000f;

    return 1;
}

static int mulsu(uint16_t word, int *d, int *r)
{
    if ((word & 0xff88) != 0x0300)
        return 0;

    if (d)
        *d = (word & 0x0070) >> 4;

    if (r)
        *r = word & 0x0007;

    return 1;
}

static int neg(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9401)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int or(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x2800)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int ori(uint16_t word, int *d, int *K)
{
    if ((word & 0xf000) != 0x6000)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (K)
        *K = ((word & 0x0f00) >> 4) | (word & 0x000f);

    return 1;
}

static int out(uint16_t word, int *A, int *r)
{
    if ((word & 0xf800) != 0xb800)
        return 0;

    if (A)
        *A = ((word & 0x0600) >> 5) | (word & 0x000f);

    if (r)
        *r = (word & 0x01f0) >> 4;

    return 1;
}

static int pop(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x900f)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int push(uint16_t word, int *r)
{
    if ((word & 0xfe0f) != 0x920f)
        return 0;

    if (r)
        *r = (word & 0x01f0) >> 4;

    return 1;
}

static int ror(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9407)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int sbc(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x0800)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int sbci(uint16_t word, int *d, int *K)
{
    if ((word & 0xf000) != 0x4000)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (K)
        *K = ((word & 0x0f00) >> 4) | (word & 0x000f);

    return 1;
}

static int sbi(uint16_t word, int *A, int *b)
{
    if ((word & 0xff00) != 0x9a00)
        return 0;

    if (A)
        *A = (word & 0x00f8) >> 3;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int sbic(uint16_t word, int *A, int *b)
{
    if ((word & 0xff00) != 0x9900)
        return 0;

    if (A)
        *A = (word & 0x00f8) >> 3;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int sbis(uint16_t word, int *A, int *b)
{
    if ((word & 0xff00) != 0x9b00)
        return 0;

    if (A)
        *A = (word & 0x00f8) >> 3;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int sbiw(uint16_t word, int *d, int *K)
{
    if ((word & 0xff00) != 0x9700)
        return 0;

    if (d)
        *d = (word & 0x0030) >> 4;    

    if (K)
        *K = ((word & 0x00c0) >> 2) | (word & 0x000f);

    return 1;
}

static int sbrc(uint16_t word, int *r, int *b)
{
    if ((word & 0xfe08) != 0xfc00)
        return 0;

    if (r)
        *r = (word & 0x01f0) >> 4;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int sbrs(uint16_t word, int *r, int *b)
{
    if ((word & 0xfe08) != 0xfe00)
        return 0;

    if (r)
        *r = (word & 0x01f0) >> 4;

    if (b)
        *b = word & 0x0007;

    return 1;
}

static int st(uint16_t word, const char **operand, int *q, int *r)
{
    const char *t;
    uint8_t offset = 0;

    switch (word & 0xfe0f) {
        case 0x920c:
            t = "X";
            break;
        case 0x920d:
            t = "X+";
            break;
        case 0x920e:
            t = "-X";
            break;
        case 0x9209:
            t = "Y+";
            break;
        case 0x920a:
            t = "-Y";
            break;
        case 0x9201:
            t = "Z+";
            break;
        case 0x9202:
            t = "-Z";
            break;
        default:
            switch (word & 0xd208) {
                case 0x8208:
                    offset = ((word & 0x2000) >> 8) | ((word & 0x0c00) >> 7) | (word & 0x0007);
                    t = "Y";
                    break;
                case 0x8200:
                    offset = ((word & 0x2000) >> 8) | ((word & 0x0c00) >> 7) | (word & 0x0007);
                    t = "Z";
                    break;
                default:
                    return 0;
            }
            break;
    }

    if (operand)
        *operand = t;

    if (q)
        *q = offset;

    if (r)
        *r = (word & 0x01f0) >> 4;

    return 1;
}

static int sts(struct wordlist *wl, int *skip, int *k, int *r)
{
    if ((wl->word & 0xfe0f) == 0x9200) {

        if (wl->next == NULL) {
            fprintf(stderr, "2nd word of 32-bit opcode after %05x missing\n", wl->address);
            return 0;
        }

        if (skip)
            *skip = 1; /* 32-bit opcode, need to skip next word */

        if (k)
            *k = wl->next->word;

        if (r)
            *r = (wl->word & 0x01f0) >> 4;

        return 1;
    }

    if ((wl->word & 0xf800) == 0xa800) {

        if (skip)
            *skip = 0; /* 16-bit opcode, no need to skip next word */

        if (k)
            *k = ((wl->word & 0x0700) >> 4) | (wl->word & 0x000f);

        if (r)
            *r = (wl->word & 0x00f0) >> 4;

        return 1;
    }

    return 0;
}

static int sub(uint16_t word, int *d, int *r)
{
    if ((word & 0xfc00) != 0x1800)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    if (r)
        *r = ((word & 0x0200) >> 5) | (word & 0x000f);

    return 1;
}

static int subi(uint16_t word, int *d, int *K)
{
    if ((word & 0xf000) != 0x5000)
        return 0;

    if (d)
        *d = (word & 0x00f0) >> 4;

    if (K)
        *K = ((word & 0x0f00) >> 4) | (word & 0x000f);

    return 1;
}

static int swap(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9402)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int xch(uint16_t word, int *d)
{
    if ((word & 0xfe0f) != 0x9204)
        return 0;

    if (d)
        *d = (word & 0x01f0) >> 4;

    return 1;
}

static int collectlabeladdrs(struct wordlist *wl)
{
    uint32_t targetaddr;

    for (; wl; wl = wl->next) {

        /* BRCC, BRCS, BREQ, BRGE, BRHC, BRHS, BRID, BRIE, BRLT, BRMI, BRNE, BRPL, BRTC, BRTS, BRVC and BRVS */
        if (condrelbranch(wl->word, wl->address, NULL, &targetaddr)) {
            if (!addlabeladdr(targetaddr))
                return 0;
        }
        /* RCALL and RJMP */
        else if (relcalljmp(wl->word, wl->address, NULL, &targetaddr)) {
            if (!addlabeladdr(targetaddr))
                return 0;
        }
        /* CALL and JMP */
        else if (longcalljmp(wl, NULL, &targetaddr)) {
            if (!addlabeladdr(targetaddr))
                return 0;
            wl = wl->next; /* Skip 2nd word of the 32-bit opcode */
        }
    }

    /* Sort addresses in ascending order */
    qsort(labels, labelscount, sizeof(struct labelstruct), comp);

    return 1;
}

static int genlabels(void)
{
    size_t i;
    int sz;
    char *buf, *fmt = "L%zu";

    for (i = 0; i < labelscount; i++) {
        sz = snprintf(NULL, 0, fmt, i);
        buf = malloc(sz+1);
        if (!buf) {
            fprintf(stderr, "Error allocating memory.\n");
            return 0;
        }
        snprintf(buf, sz+1, fmt, i);

        labels[i].label = buf;
    }

    return 1;
}

void emitavrasm(struct ihex *ih, int listing)
{
    struct wordlist *wl;
    const char *label, *mnemonic, *operand;
    int d, r, b, k, K, A, q;
    int skip;
    uint32_t targetaddr;
    uint32_t lastaddr = 0;

    if (!ih)
        return;

    if (collectlabeladdrs(ih->words) && genlabels())
        for (wl = ih->words; wl; wl = wl->next) {        

            /* When there is a discontinuity in the address, emit an .org directive */
            if (!listing && lastaddr+1 != wl->address)  /* Except for when listing mode is enabled */
                printf(".org 0x%04x\n", wl->address);

            /* Include word address and instruction word when in listing mode */
            if (listing)
                printf("%05x: %04x ", wl->address, wl->word);

            /* When there is a label for this address, emit the label */
            if ((label = lookuplabel(wl->address)))
                printf("%s: ", label);

            /* Generate symbolic instructions from opcode */
            if (adc(wl->word, &d, &r))
                if (d != r)
                    printf("adc r%d, r%d\n", d, r);
                else
                    printf("rol r%d\n", d);
            else if (add(wl->word, &d, &r))
                if (d != r)
                    printf("add r%d, r%d\n", d, r);
                else
                    printf("lsl r%d\n", d);
            else if (adiw(wl->word, &d, &K))
                printf("adiw r%d:r%d, %d\n", 2*d+24+1, 2*d+24, K);
            else if (and(wl->word, &d, &r))
                if (d != r)
                    printf("and r%d, r%d\n", d, r);
                else
                    printf("tst r%d\n", d);
            else if (andi(wl->word, &d, &K))
                printf("andi r%d, %d\n", d+16, K);
            else if (asr(wl->word, &d))
                printf("asr r%d\n", d);
            else if (bld(wl->word, &d, &b))
                printf("bld r%d, %d\n", d, b);
            else if (bst(wl->word, &r, &b))
                printf("bst r%d, %d\n", r, b);
            else if (condrelbranch(wl->word, wl->address, &mnemonic, &targetaddr))
                printf("%s %s\n", mnemonic, lookuplabel(targetaddr));
            else if (relcalljmp(wl->word, wl->address, &mnemonic, &targetaddr))
                printf("%s %s\n", mnemonic, lookuplabel(targetaddr));
            else if (longcalljmp(wl, &mnemonic, &targetaddr)) {
                printf("%s %s\n", mnemonic, lookuplabel(targetaddr));
                wl = wl->next; /* Skip 2nd word of the 32-bit opcode */
                if (listing)
                    printf("%05x: %04x\n", wl->address, wl->word);
            }
            else if (wl->word == 0x9598)
                printf("break\n");
            else if (cbi(wl->word, &A, &b))
                printf("cbi 0x%02x, %d\n", A, b);
            else if (wl->word == 0x9488)
                printf("clc\n");
            else if (wl->word == 0x94d8)
                printf("clh\n");
            else if (wl->word == 0x94f8)
                printf("cli\n");
            else if (wl->word == 0x94a8)
                printf("cln\n");
            else if (wl->word == 0x94c8)
                printf("cls\n");
            else if (wl->word == 0x94e8)
                printf("clt\n");
            else if (wl->word == 0x94b8)
                printf("clv\n");
            else if (wl->word == 0x9498)
                printf("clz\n");
            else if (com(wl->word, &d))
                printf("com r%d\n", d);
            else if (cp(wl->word, &d, &r))
                printf("cp r%d, r%d\n", d, r);
            else if (cpc(wl->word, &d, &r))
                printf("cpc r%d, r%d\n", d, r);
            else if (cpi(wl->word, &d, &K))
                printf("cpi r%d, %d\n", d+16, K);
            else if (cpse(wl->word, &d, &r))
                printf("cpse r%d, r%d\n", d, r);
            else if (dec(wl->word, &d))
                printf("dec r%d\n", d);
            else if (des(wl->word, &K))
                printf("des 0x%02x\n", K);
            else if (wl->word == 0x9519)
                printf("eicall\n");
            else if (wl->word == 0x9419)
                printf("eijmp\n");
            else if (elpm(wl->word, &d, &operand))
                if (*operand)
                    printf("elpm r%d, %s\n", d, operand);
                else
                    printf("elpm\n");
            else if (eor(wl->word, &d, &r))
                if (d != r)
                    printf("eor r%d, r%d\n", d, r);
                else
                    printf("clr r%d\n", d);
            else if (fmul(wl->word, &d, &r))
                printf("fmul r%d, r%d\n", d+16, r);
            else if (fmuls(wl->word, &d, &r))
                printf("fmuls r%d, r%d\n", d+16, r);
            else if (fmulsu(wl->word, &d, &r))
                printf("fmulsu r%d, r%d\n", d+16, r);
            else if (wl->word == 0x9509)
                printf("icall\n");
            else if (wl->word == 0x9409)
                printf("ijmp\n");
            else if (in(wl->word, &d, &A))
                printf("in r%d, 0x%02x\n", d, A);
            else if (inc(wl->word, &d))
                printf("inc r%d\n", d);
            else if (lac(wl->word, &d))
                printf("lac Z, r%d\n", d);
            else if (las(wl->word, &d))
                printf("las Z, r%d\n", d);
            else if (lat(wl->word, &d))
                printf("lat Z, r%d\n", d);
            else if (ld(wl->word, &d, &operand, &q))
                if (q > 0) 
                    printf("ldd r%d, %s+%d\n", d, operand, q);
                else
                    printf("ld r%d, %s\n", d, operand);
            else if (ldi(wl->word, &d, &K))
                printf("ldi r%d, %d\n", d+16, K);
            else if (lds(wl, &skip, &d, &k)) {
                printf("lds r%d, 0x%02x\n", d, k);
                if (skip) {
                    wl = wl->next; /* Skip 2nd word of the 32-bit opcode */
                    if (listing)
                        printf("%05x: %04x\n", wl->address, wl->word);
                }
            }
            else if (lpm(wl->word, &d, &operand))
                if (*operand)
                    printf("lpm r%d, %s\n", d, operand);
                else
                    printf("lpm\n");
            else if (lsr(wl->word, &d))
                printf("lsr r%d\n", d);
            else if (mov(wl->word, &d, &r))
                printf("mov r%d, r%d\n", d, r);
            else if (movw(wl->word, &d, &r))
                printf("movw r%d:r%d, r%d:r%d\n", 2*d+1, 2*d, 2*r+1, 2*r);
            else if (mul(wl->word, &d, &r))
                printf("mul r%d, r%d\n", d, r);
            else if (muls(wl->word, &d, &r))
                printf("muls r%d, r%d\n", d+16, r+16);
            else if (mulsu(wl->word, &d, &r))
                printf("mulsu r%d, r%d\n", d+16, r+16);
            else if (neg(wl->word, &d))
                printf("neg r%d\n", d);
            else if (wl->word == 0x0000)
                printf("nop\n");
            else if (or(wl->word, &d, &r))
                printf("or r%d, r%d\n", d, r);
            else if (ori(wl->word, &d, &K))
                printf("ori r%d, %d\n", d+16, K);
            else if (out(wl->word, &A, &r))
                printf("out 0x%02x, r%d\n", A, r);
            else if (pop(wl->word, &d))
                printf("pop r%d\n", d);
            else if (push(wl->word, &r))
                printf("push r%d\n", r);
            else if (wl->word == 0x9508)
                printf("ret\n");
            else if (wl->word == 0x9518)
                printf("reti\n");
            else if (ror(wl->word, &d))
                printf("ror r%d\n", d);
            else if (sbc(wl->word, &d, &r))
                printf("sbc r%d, r%d\n", d, r);
            else if (sbci(wl->word, &d, &K))
                printf("sbci r%d, %d\n", d+16, K);
            else if (sbi(wl->word, &A, &b))
                printf("sbi 0x%02x, %d\n", A, b);
            else if (sbic(wl->word, &A, &b))
                printf("sbic 0x%02x, %d\n", A, b);
            else if (sbis(wl->word, &A, &b))
                printf("sbis 0x%02x, %d\n", A, b);
            else if (sbiw(wl->word, &d, &K))
                printf("sbiw r%d:r%d, %d\n", 2*d+24+1, 2*d+24, K);
            else if (sbrc(wl->word, &r, &b))
                printf("sbrc r%d, %d\n", r, b);
            else if (sbrs(wl->word, &r, &b))
                printf("sbrs r%d, %d\n", r, b);
            else if (wl->word == 0x9408)
                printf("sec\n");
            else if (wl->word == 0x9458)
                printf("seh\n");
            else if (wl->word == 0x9478)
                printf("sei\n");
            else if (wl->word == 0x9428)
                printf("sen\n");
            else if (wl->word == 0x9448)
                printf("ses\n");
            else if (wl->word == 0x9468)
                printf("set\n");
            else if (wl->word == 0x9438)
                printf("sev\n");
            else if (wl->word == 0x9418)
                printf("sez\n");
            else if (wl->word == 0x9588)
                printf("sleep\n");
            else if (wl->word == 0x95e8)
                printf("spm\n");
            else if (st(wl->word, &operand, &q, &r))
                if (q > 0) 
                    printf("std %s+%d, r%d\n", operand, q, r);
                else
                    printf("st %s, r%d\n", operand, r);
            else if (sts(wl, &skip, &k, &r)) {
                printf("sts 0x%02x, r%d\n", k, r);
                if (skip) {
                    wl = wl->next; /* Skip 2nd word of the 32-bit opcode */
                    if (listing)
                        printf("%05x: %04x\n", wl->address, wl->word);
                }
            }
            else if (sub(wl->word, &d, &r))
                printf("sub r%d, r%d\n", d, r);
            else if (subi(wl->word, &d, &K))
                printf("subi r%d, %d\n", d+16, K);
            else if (swap(wl->word, &d))
                printf("swap r%d\n", d);
            else if (wl->word == 0x95a8)
                printf("wdr\n");
            else if (xch(wl->word, &d))
                printf("xch Z, r%d\n", d);
            else 
                printf(".dw 0x%04x\n", wl->word);

            /* Save last address for discontinuity check */
            lastaddr = wl->address;
        }

    /* When tere is nothing more to do, free the memory */
    freelabels();
}
