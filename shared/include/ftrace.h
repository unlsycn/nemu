#ifndef __FTRACE_H__
#define __FTRACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <shared.h>

    void parse_elf(const char *elf_file);

    void check_call(vaddr_t addr);

    void check_ret(uint32_t inst);

#ifdef __cplusplus
}
#endif

#endif