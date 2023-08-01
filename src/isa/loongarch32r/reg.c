
#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "tp", "sp", "a0", "a1", "a2", "a3",
  "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3",
  "t4", "t5", "t6", "t7", "t8", "rs", "fp", "s0",
  "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8"
};

void isa_reg_display() {
}

word_t isa_reg_str2val(const char *s, bool *success) {
  return 0;
}
