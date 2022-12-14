#include "local-include/reg.h"
#include <isa.h>

const char *regs[] = {"zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
                      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
                      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

void isa_reg_display()
{
    for (int i = 0; i < 32; i++)
    {
        printf("%4s %20lu\n", regs[i], gpr(i));
    }
}

word_t isa_reg_str2val(const char *s, bool *success)
{
    for (int i = 0; i < 32; i++)
    {
        if (strcmp(s, regs[i]))
        {
            *success = true;
            return gpr(i);
        }
    }
    *success = false;
    return 0;
}
