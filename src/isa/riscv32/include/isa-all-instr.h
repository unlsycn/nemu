#include "../local-include/rtl.h"
#include <cpu/decode.h>

#define INSTR_LIST(f) f(lui) f(lw) f(sw) f(inv) f(nemu_trap)

def_all_EXEC_ID();
