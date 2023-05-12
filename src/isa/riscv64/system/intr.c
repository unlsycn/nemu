#include <isa.h>

word_t isa_raise_intr(word_t NO, vaddr_t epc)
{
    // save pc to mepc
    cpu.csr.mepc->val = epc;

    // set exception number
    cpu.csr.mcause->val = NO;

    int offset = cpu.csr.mtvec->mode == 1 && cpu.csr.mcause->intr ? cpu.csr.mcause->code * 4 : 0; // Vectored MODE
    return cpu.csr.mtvec->base + offset;
}

word_t isa_query_intr()
{
    return INTR_EMPTY;
}
