#include "csr.h"

#include <isa.h>

#include "common.h"

#define CSR cpu.csr
csr_t csr_space[4096];

#define _CSR_NAME(name, addr) STR(name),
#define _CSR_ADDR(name, addr) addr,
const char *csr_name[] = {MAP(CSRS, _CSR_NAME)};
const uint16_t csr_addr[] = {MAP(CSRS, _CSR_ADDR)};
const size_t csr_count = sizeof(csr_name) / sizeof(char *);

#define SSTATUS_MASK 0x80000003000De762
void sync_sstatus()
{
    CSR.sstatus->val = CSR.mstatus->val & SSTATUS_MASK;
}

void sync_mstatus()
{
    CSR.mstatus->val = (CSR.mstatus->val & (~SSTATUS_MASK)) | CSR.sstatus->val;
}

void init_csr()
{
#define CSR_INIT(name, _addr)                           \
    CSR.name = (CONCAT(name, _t *)) & csr_space[_addr]; \
    CSR.name->valid = true;                             \
    CSR.name->addr = _addr;

    MAP(CSRS, CSR_INIT);
    CSR.mstatus->val = 0xa00001800;
    CSR.sstatus->val = 0x200000000;
}

word_t csr_read(uint16_t addr)
{
    csr_t *csr = &csr_space[addr];
    if (csr->valid)
        return csr->val;
    else
        panic("TODO: CSR at %x invalid to read. Should raise a instruction exception.", addr);
}

void csr_write(uint16_t addr, word_t data)
{
    csr_t *csr = &csr_space[addr];
    if (csr->valid)
        csr->val = data;
    else
        panic("TODO: CSR at %x invalid to write. Should raise a instruction exception.", addr);

    // sync mstatus and sstatus
    if (csr->addr == CSR.mstatus->addr)
        sync_sstatus();
    else if (csr->addr == CSR.sstatus->addr)
    {
        csr->val &= SSTATUS_MASK; // ignore illegal value
        sync_mstatus();
    }
}

void csr_display(CPU_state *state)
{
    word_t **csr = (word_t **)&state->csr;
    for (int i = 0; i < sizeof(CSRs) / sizeof(csr_t *); i++)
    {
        if (*csr)
            printf(ANSI_FMT("%-8s", ANSI_FG_MAGENTA) FMT_WORD_LH "  " FMT_WORD_LD "\n", csr_name[i], **csr, **csr);
        csr++;
    }
}