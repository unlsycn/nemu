#ifndef __MMU_H__
#define __MMU_H__

#include "common.h"

#define PG_OFFSET 12
typedef union PageTableEntry {
    struct
    {
        uint8_t v : 1;
        uint8_t r : 1;
        uint8_t w : 1;
        uint8_t x : 1;
        uint8_t u : 1;
        uint8_t g : 1;
        uint8_t a : 1;
        uint8_t d : 1;
        word_t rsw : 2;
        word_t ppn : 44;
        word_t reserved : 10;
    };
    uintptr_t val;
} PageTableEntry;

inline paddr_t vpn(vaddr_t va, int lv)
{
    return BITS((uintptr_t)va, PG_OFFSET + 9 * (4 - lv) - 1, PG_OFFSET + 9 * (3 - lv)) * sizeof(PageTableEntry);
}

#endif
