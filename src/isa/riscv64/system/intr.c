#include <isa.h>

word_t isa_raise_intr(word_t NO, vaddr_t epc)
{
    // save pc to mepc
    cpu.csr.mepc->val = epc;

    // set exception number
    cpu.csr.mcause->val = NO;

    cpu.csr.mstatus->MPP = cpu.priv;
    cpu.priv = PRIV_M;

    cpu.csr.mstatus->MPIE = cpu.csr.mstatus->MIE;
    cpu.csr.mstatus->MIE = 0;

    IFDEF(CONFIG_ETRACE, log_write("[etrace] %s code %lu is raised at epc = " FMT_WORD_LH "\n",
                                   cpu.csr.mcause->intr ? "Interrupt" : "Exception", cpu.csr.mcause->code, epc));

    int offset = cpu.csr.mtvec->mode == 1 && cpu.csr.mcause->intr ? cpu.csr.mcause->code * 4 : 0; // Vectored MODE
    return (cpu.csr.mtvec->base << 2) + offset;
}

word_t isa_query_intr()
{
    return INTR_EMPTY;
}
