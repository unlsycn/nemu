#include <locale.h>

#include "bp.h"
#include "cpu/decode.h"
#include "cpu/difftest.h"
#include "macro.h"
#include "memory/cache.h"
#include "sdb.h"
#include "utils.h"

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

#define INST_NUM_IN_BUF 20
static char iringbuf[INST_NUM_IN_BUF][DECODE_LOGBUF_LEN];
static int iringbuf_index = 0;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc)
{
#ifdef CONFIG_ITRACE_COND
    if (ITRACE_COND)
    {
        log_write("[itrace] %s\n", _this->logbuf);
        snprintf(iringbuf[iringbuf_index], DECODE_LOGBUF_LEN, "%s\n", _this->logbuf);
        iringbuf_index++;
        if (iringbuf_index >= INST_NUM_IN_BUF)
            iringbuf_index = 0;
    }
#endif
    if (g_print_step)
    {
        IFDEF(CONFIG_ITRACE, puts(_this->logbuf));
    }
#ifdef CONFIG_WATCHPOINT
    if (nemu_state.state == NEMU_RUNNING && travel_wp())
    {
        IFDEF(CONFIG_STOP_AT_WP, nemu_state.state = NEMU_STOP);
    }
#endif
    IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
}

static void exec_once(Decode *s, vaddr_t pc)
{
    s->pc = pc;
    s->snpc = pc; // snpc + 4 in inst_fecth
    isa_exec_once(s);
    cpu.pc = s->dnpc; // dnpc is determined in decoding
    cpu.csr.cycle->val++;
#ifdef CONFIG_ITRACE  // output insts and disassembly to Deocde.logbuf
    char *p = s->logbuf;
    p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
    int ilen = s->snpc - s->pc; // measured in bytes
    int i;
    uint8_t *inst = (uint8_t *)&s->isa.inst.val;
    for (i = ilen - 1; i >= 0; i--)
    {
        p += snprintf(p, 4, " %02x", inst[i]);
    }
    int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
    int space_len = ilen_max - ilen;
    if (space_len < 0)
        space_len = 0;
    space_len = space_len * 3 + 1;
    memset(p, ' ', space_len);
    p += space_len;

#ifndef CONFIG_ISA_loongarch32r
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    disassemble(p, s->logbuf + sizeof(s->logbuf) - p, MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val,
                ilen);
#else
    p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

static void execute(uint64_t n)
{
    Decode s;
    for (; n > 0; n--)
    {
        exec_once(&s, cpu.pc);
        g_nr_guest_inst++;
        trace_and_difftest(&s, cpu.pc);
        if (nemu_state.state != NEMU_RUNNING)
            break;
        IFDEF(CONFIG_DEVICE, device_update());
    }
}

static void statistic()
{
    IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
    Log("host time spent = " NUMBERIC_FMT " us", g_timer);
    Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
    if (g_timer > 0)
        Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
    else
        Log("Finish running in less than 1 us and can not calculate the simulation frequency");
    IFDEF(CONFIG_CACHE_SIM, cache_statistic());
    IFDEF(CONFIG_BRANCH_PREDICTION, bp_statistic());
}

static void print_iringbuf()
{
    for (int i = iringbuf_index; i < INST_NUM_IN_BUF; i++)
        printf("%s", iringbuf[i]);
    for (int i = 0; i < iringbuf_index; i++)
        printf("%s", iringbuf[i]);
}

void assert_fail_msg()
{
    isa_reg_display();
    print_iringbuf();
    statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n)
{
    g_print_step = (n < MAX_INST_TO_PRINT);
    switch (nemu_state.state)
    {
    case NEMU_END:
    case NEMU_ABORT:
        printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
        return;
    default:
        nemu_state.state = NEMU_RUNNING;
    }

    uint64_t timer_start = get_time();

    execute(n);

    uint64_t timer_end = get_time();
    g_timer += timer_end - timer_start;

    switch (nemu_state.state)
    {
    case NEMU_RUNNING:
        nemu_state.state = NEMU_STOP;
        break;

    case NEMU_ABORT:
        print_iringbuf();
    case NEMU_END:
        Log("nemu: %s at pc = " FMT_WORD,
            (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED)
                                            : (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN)
                                                                        : ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
            nemu_state.halt_pc);
        // fall through
    case NEMU_QUIT:
        statistic();
    }
}
