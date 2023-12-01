#include <cpu/difftest.h>
#include <isa.h>

#include "../local-include/reg.h"

extern const char *regs[];

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
    bool result = true;
    ref_difftest_regcpy(ref_r, DIFFTEST_TO_DUT);
    result &= difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    for (int i = 0; i < 32; i++)
        result &= difftest_check_reg(regs[i], pc, ref_r->gpr[i], gpr(i));

    // check priv
    result &= difftest_check_reg("priv", pc, ref_r->priv, cpu.priv);

    // skip comparing mcycle
    ref_r->csr.mcycle->val = cpu.csr.mcycle->val;

    word_t **dut_csr_p = (word_t **)&cpu.csr;
    word_t **ref_csr_p = (word_t **)&ref_r->csr;
    for (int i = 0; i < sizeof(CSRs) / sizeof(mepc_t *); i++)
    {
        extern char *csr_name[];
        result &= difftest_check_reg(csr_name[i], pc, **ref_csr_p, **dut_csr_p);
        dut_csr_p++;
        ref_csr_p++;
    }
    return result;
}

void isa_difftest_attach()
{
}
