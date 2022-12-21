#include <cpu/cpu.h>
#include <monitor/sdb.h>

void engine_start()
{
#ifdef CONFIG_TARGET_AM
    cpu_exec(-1);
#else
    /* Receive commands from user. */
    sdb_mainloop();
#endif
}
