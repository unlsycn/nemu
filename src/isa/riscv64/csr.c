#include "csr.h"
#include "common.h"
#include <isa.h>

word_t csr_space[4096];
bool csr_valid[4096];

#define _CSR_NAME(name, addr) STR(name),
const char *csr_name[] = {MAP(CSRS, _CSR_NAME)};

void init_csr()
{
#define CSR_INIT(name, addr)                               \
    cpu.csr.name = (CONCAT(name, _t *)) & csr_space[addr]; \
    csr_valid[addr] = true;

    MAP(CSRS, CSR_INIT);
    cpu.csr.mstatus->val = 0xa00001800;
}

inline bool check_csr_valid(uint16_t addr)
{
    return csr_valid[addr];
}

word_t csr_read(uint16_t addr)
{
    if (check_csr_valid(addr))
        return csr_space[addr];
    else
        panic("TODO: CSR at %x invalid to read. Should raise a instruction exception.", addr);
}

void csr_write(uint16_t addr, word_t data)
{
    if (check_csr_valid(addr))
        csr_space[addr] = data;
    else
        panic("TODO: CSR at %x invalid to write. Should raise a instruction exception.", addr);
}

void csr_display()
{
    for (int i = 0; i < sizeof(CSRs) / sizeof(mepc_t *); i++)
    {
        word_t **csr = (word_t **)&cpu.csr;
        printf("%-8s" FMT_WORD_LH "  " FMT_WORD_LD "\n", csr_name[i], **csr, **csr);
        (*csr)++;
    }
}