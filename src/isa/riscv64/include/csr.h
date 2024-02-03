#ifndef __CSR_H__
#define __CSR_H__

#include <common.h>

#define MXLEN 64
#define SXLEN 64
#define CSRS(f)                                                                                                          \
    f(mepc, 0x341) f(mtvec, 0x305) f(mstatus, 0x300) f(mcause, 0x342) f(mcycle, 0xc00) f(mscratch, 0x340) f(satp, 0x180) \
        f(sepc, 0x141) f(stvec, 0x105) f(sstatus, 0x100) f(scause, 0x142) f(sscratch, 0x140) f(medeleg, 0x302)           \
            f(mideleg, 0x303)

#define _WORD_T(val) word_t val;
#define CSR_TYPE(name, ...)               \
    typedef struct                        \
    {                                     \
        union {                           \
            struct                        \
            {                             \
                MAP(_WORD_T, __VA_ARGS__) \
            };                            \
            word_t val;                   \
        };                                \
        bool valid;                       \
        uint16_t addr;                    \
    } CONCAT(name, _t);

// CSR type template
CSR_TYPE(csr, csr : 64);

CSR_TYPE(mepc, mepc : MXLEN);
CSR_TYPE(mtvec, mode : 2, base : MXLEN - 2);
CSR_TYPE(mstatus, WPRI1 : 1, SIE : 1, WPRI2 : 1, MIE : 1, WPRI3 : 1, SPIE : 1, UBE : 1, MPIE : 1, SPP : 1, VS : 2, MPP : 2,
         FS : 2, XS : 2, MPRV : 1, SUM : 1, MXR : 1, TVM : 1, TW : 1, TSR : 1, WPRI4 : 9, UXL : 2, SXL : 2, SBE : 1, MBE : 1,
         WPRI5 : 25, SD : 1);
CSR_TYPE(mcause, code : MXLEN - 1, intr : 1);
CSR_TYPE(mcycle, cycle : MXLEN);
CSR_TYPE(mscratch, mscratch : MXLEN);
CSR_TYPE(medeleg, no : SXLEN);
CSR_TYPE(mideleg, no : SXLEN);

CSR_TYPE(satp, ppn : 44, asid : 16, mode : 4);
CSR_TYPE(sepc, sepc : SXLEN);
CSR_TYPE(stvec, mode : 2, base : SXLEN - 2);
CSR_TYPE(sstatus, WPRI1 : 1, SIE : 1, WPRI2 : 3, SPIE : 1, UBE : 1, WPRI3 : 1, SPP : 1, VS : 2, WPRI4 : 2, FS : 2, XS : 2,
         WPRI5 : 1, SUM : 1, MXR : 1, WPRI6 : 12, UXL : 2, WPRI7 : 29, SD : 1);
CSR_TYPE(scause, code : SXLEN - 1, intr : 1);
CSR_TYPE(sscratch, sscratch : SXLEN);
#undef _WORD_T

typedef struct
{
#define CSR_PTR(name, addr) CONCAT(name, _t) * name;
    MAP(CSRS, CSR_PTR)
} CSRs;

void init_csr();
word_t csr_read(uint16_t addr);
void csr_write(uint16_t addr, word_t data);

#endif