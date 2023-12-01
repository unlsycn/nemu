#ifndef __ISA_H__
#define __ISA_H__

// Located at src/isa/$(GUEST_ISA)/include/isa-def.h
#include <isa-def.h>

// The macro `__GUEST_ISA__` is defined in $(CFLAGS).
// It will be expanded as "x86" or "mips32" ...
typedef CONCAT(__GUEST_ISA__, _CPU_state) CPU_state;
typedef CONCAT(__GUEST_ISA__, _ISADecodeInfo) ISADecodeInfo;

// monitor
extern unsigned char isa_logo[];
void init_isa();

// reg
extern CPU_state cpu;
void csr_display(CPU_state *);
void isa_reg_display(CPU_state *);
word_t isa_reg_str2val(const char *name, bool *success);

// exec
struct Decode;
int isa_exec_once(struct Decode *s);

// memory
enum
{
    MMU_DIRECT,
    MMU_TRANSLATE,
    MMU_FAIL
};
enum
{
    MEM_TYPE_IFETCH,
    MEM_TYPE_READ,
    MEM_TYPE_WRITE
};
enum
{
    MEM_RET_OK,
    MEM_RET_FAIL,
    MEM_RET_CROSS_PAGE
};
int isa_mmu_check(vaddr_t vaddr, int len, int type);
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type);

// interrupt/exception
vaddr_t isa_raise_intr(word_t NO, vaddr_t epc);
#define INTR_EMPTY ((word_t)-1)
word_t isa_query_intr();

// difftest
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();

void isa_difftest_regcpy(CPU_state *dut, bool direction);

#endif
