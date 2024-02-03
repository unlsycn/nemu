#include "local-include/reg.h"

#include <isa.h>

#include "csr.h"
#include "utils.h"

const char *regs[] = {"zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
                      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
                      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

void isa_reg_display(CPU_state *state)
{
    printf(ANSI_FMT("[pc]    " FMT_WORD "\n", ANSI_FG_MAGENTA), state->pc);

    const int column = 2;
    for (int i = 0; i < column; i++)
        printf(ANSI_FMT("Reg     %-18s  %-20s", ANSI_FG_GREEN), "Hex", "Dec");
    printf("\n");
    for (int i = 0; i < 32; i++)
    {
        printf(ANSI_FMT("%-8s", ANSI_FG_MAGENTA) FMT_WORD_LH "  " FMT_WORD_LD, regs[i], gpr(i), gpr(i));
        if (i % column == column - 1 || i == 31)
            printf("\n");
    }

    csr_display(state);
}

extern size_t csr_count;
extern char *csr_name[];
extern uint16_t csr_addr[];
word_t isa_reg_str2val(const char *s, bool *success)
{
    if (strcmp(s, "pc") == 0)
    {
        *success = true;
        return cpu.pc;
    }
    for (int i = 0; i < 32; i++)
    {
        if (strcmp(s, regs[i]) == 0)
        {
            *success = true;
            return gpr(i);
        }
    }
    for (int i = 0; i < csr_count; i++)
    {
        if (strcmp(s, csr_name[i]) == 0)
        {
            *success = true;
            return csr_read(csr_addr[i]);
        }
    }
    *success = false;
    return 0;
}
