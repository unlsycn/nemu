#include "common.h"
#include "macro.h"
#include <isa.h>
#include <memory/cache.h>
#include <memory/paddr.h>

word_t vaddr_ifetch(vaddr_t addr, int len)
{
    return MUXDEF(CONFIG_CACHE_SIM, icache_fetch(addr, len), paddr_read(addr, len));
}

word_t vaddr_read(vaddr_t addr, int len)
{
    word_t data = MUXDEF(CONFIG_CACHE_SIM, dcache_read(addr, len), paddr_read(addr, len));
#ifdef CONFIG_MTRACE
    if (addr >= CONFIG_MTRACE_LEFT && addr <= CONFIG_MTRACE_RIGHT)
        log_write("[mtrace] address = " FMT_WORD " is READ %d bytes at pc = " FMT_WORD ", data = " FMT_WORD "\n", addr, len,
                  cpu.pc, data);
#endif
    return data;
}

void vaddr_write(vaddr_t addr, int len, word_t data)
{
#ifdef CONFIG_MTRACE
    if (addr >= CONFIG_MTRACE_LEFT && addr <= CONFIG_MTRACE_RIGHT)
        log_write("[mtrace] address = " FMT_WORD " is WRITTEN %d bytes at pc = " FMT_WORD ", data = " FMT_WORD "\n", addr, len,
                  cpu.pc, data);
#endif
    MUXDEF(CONFIG_CACHE_SIM, dcache_write(addr, len, data), paddr_write(addr, len, data));
}
