#ifndef __CSR_H__
#define __CSR_H__

#include <common.h>

#define MXLEN 64
#define CSRS(f) f(mepc, 0x341) f(mtvec, 0x305) f(mstatus, 0x300) f(mcause, 0x342) f(cycle, 0xc00) f(satp, 0x180)

#define _WORD_T(val) word_t val;
#define CSR_TYPE(name, ...)           \
    typedef union {                   \
        struct                        \
        {                             \
            MAP(_WORD_T, __VA_ARGS__) \
        };                            \
        word_t val;                   \
    } CONCAT(name, _t);

CSR_TYPE(mepc, mepc : MXLEN)
CSR_TYPE(mtvec, mode : 2, base : MXLEN - 2)
CSR_TYPE(mstatus, UIE : 1, SIE : 1, WPRI1 : 1, MIE : 1, UPIE : 1, SPIE : 1, WPRI2 : 1, MPIE : 1, SPP : 2, WPRI3 : 2, MPP : 2,
         FS : 2, XS : 2, MPRV : 1, SUM : 1, MXR : 1, TVM : 1, TW : 1, TSR : 1, WPRI4 : 9, UXL : 2, SXL : 2, WPRI5
         : MXLEN - 37, SD : 1)
CSR_TYPE(mcause, code : MXLEN - 1, intr : 1)
CSR_TYPE(cycle, cycle : MXLEN)
CSR_TYPE(satp, ppn : 44, asid : 16, mode : 4)
#undef _WORD_T

typedef struct
{
#define CSR_PTR(name, addr) CONCAT(name, _t) * name;
    MAP(CSRS, CSR_PTR)
} CSRs;

void init_csr();
word_t csr_read(uint16_t addr);
void csr_write(uint16_t addr, word_t data);
void csr_display();

#endif