#include <isa.h>

#define CSR cpu.csr

void sync_mstatus();
void sync_sstatus();

word_t isa_raise_intr(word_t NO, vaddr_t epc)
{
    bool delegate_to_s =
        cpu.priv <= PRIV_S && (BIT(NO, 63) /* cause.intr */ ? BIT(CSR.mideleg->no, NO) : BIT(CSR.medeleg->no, NO));

    if (delegate_to_s)
    {
        CSR.sepc->val = epc;
        CSR.scause->val = NO;

        CSR.sstatus->SPP = cpu.priv;
        cpu.priv = PRIV_S;

        CSR.sstatus->SPIE = CSR.sstatus->SIE;
        CSR.sstatus->SIE = 0;

        sync_mstatus();

        IFDEF(CONFIG_ETRACE, log_write("[etrace] S-Mode %s code %lu is raised at epc = " FMT_WORD_LH " from priv %d\n",
                                       CSR.scause->intr ? "Interrupt" : "Exception", CSR.scause->code, epc, CSR.sstatus->SPP));

        int offset = CSR.stvec->mode == 1 && CSR.scause->intr ? CSR.scause->code * 4 : 0; // Vectored MODE
        return (CSR.stvec->base << 2) + offset;
    }
    else
    {
        // save pc to mepc
        CSR.mepc->val = epc;

        // set exception number
        CSR.mcause->val = NO;

        CSR.mstatus->MPP = cpu.priv;
        cpu.priv = PRIV_M;

        CSR.mstatus->MPIE = CSR.mstatus->MIE;
        CSR.mstatus->MIE = 0;

        sync_sstatus();

        IFDEF(CONFIG_ETRACE, log_write("[etrace] M-Mode %s code %lu is raised at epc = " FMT_WORD_LH " from priv %d\n",
                                       CSR.mcause->intr ? "Interrupt" : "Exception", CSR.mcause->code, epc, CSR.mstatus->MPP));

        int offset = CSR.mtvec->mode == 1 && CSR.mcause->intr ? CSR.mcause->code * 4 : 0; // Vectored MODE
        return (CSR.mtvec->base << 2) + offset;
    }
}

word_t isa_query_intr()
{
    return INTR_EMPTY;
}

word_t mret()
{
    if (CSR.mstatus->MPP < PRIV_M)
        CSR.mstatus->MPRV = 0;
    cpu.priv = CSR.mstatus->MPP;
    CSR.mstatus->MPP = PRIV_U;
    CSR.mstatus->MIE = CSR.mstatus->MPIE;
    CSR.mstatus->MPIE = 1;
    sync_sstatus();
    return CSR.mepc->mepc;
}

word_t sret()
{
    if (CSR.mstatus->MPP < PRIV_M)
        CSR.mstatus->MPRV = 0;
    cpu.priv = CSR.sstatus->SPP;
    CSR.sstatus->SPP = PRIV_U;
    CSR.sstatus->SIE = CSR.sstatus->SPIE;
    CSR.sstatus->SPIE = 1;
    sync_mstatus();
    return CSR.sepc->sepc;
}
