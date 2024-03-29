#include <difftest-def.h>

#include "../../include/common.h"
#include "mmu.h"
#include "processor.h"
#include "sim.h"

#define csr_t nemu_csr_t
#include "../../src/isa/riscv64/include/csr.h"
#undef csr_t

#define NR_GPR MUXDEF(CONFIG_RVE, 16, 32)

static std::vector<std::pair<reg_t, abstract_device_t *>> difftest_plugin_devices;
static std::vector<std::string> difftest_htif_args;
static std::vector<std::pair<reg_t, mem_t *>> difftest_mem(1, std::make_pair(reg_t(DRAM_BASE), new mem_t(CONFIG_MSIZE)));
static debug_module_config_t difftest_dm_config = {.progbufsize = 2,
                                                   .max_sba_data_width = 0,
                                                   .require_authentication = false,
                                                   .abstract_rti = 0,
                                                   .support_hasel = true,
                                                   .support_abstract_csr_access = true,
                                                   .support_abstract_fpr_access = true,
                                                   .support_haltgroups = true,
                                                   .support_impebreak = true};

struct diff_context_t
{
    word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
    word_t pc;
    CSRs csr;
    uint32_t priv;
};

static sim_t *s = NULL;
static processor_t *p = NULL;
static state_t *state = NULL;

void sim_t::diff_init(int port)
{
    p = get_core("0");
    state = p->get_state();
}

void sim_t::diff_step(uint64_t n)
{
    step(n);
}

#define DIFF_CSRS(f) f(mepc) f(mtvec) f(mstatus) f(mcause) f(satp) f(sstatus) f(sepc) f(stvec) f(scause) f(medeleg)

#define PLACE_CSR(name, ignore) uint64_t CONCAT(csr_, name) = 0;
MAP(CSRS, PLACE_CSR);
#define SET_NONDIFF_CSR(name, ignore) ctx->csr.name = NULL;
#define GET_CSR(name)                         \
    CONCAT(csr_, name) = state->name->read(); \
    ctx->csr.name = (CONCAT(name, _t *)) & CONCAT(csr_, name);

void sim_t::diff_get_regs(void *diff_context)
{
    struct diff_context_t *ctx = (struct diff_context_t *)diff_context;
    for (int i = 0; i < NR_GPR; i++)
    {
        ctx->gpr[i] = state->XPR[i];
    }
    ctx->pc = state->pc;
    MAP(CSRS, SET_NONDIFF_CSR)
    MAP(DIFF_CSRS, GET_CSR)

    ctx->priv = state->prv;
}

#define SET_CSR(name) state->name->write(ctx->csr.name->val);
void sim_t::diff_set_regs(void *diff_context)
{
    struct diff_context_t *ctx = (struct diff_context_t *)diff_context;
    for (int i = 0; i < NR_GPR; i++)
    {
        state->XPR.write(i, (sword_t)ctx->gpr[i]);
    }
    state->pc = ctx->pc;
    MAP(DIFF_CSRS, SET_CSR)

    // no need to change priv mannually
}

void sim_t::diff_memcpy(reg_t dest, void *src, size_t n)
{
    mmu_t *mmu = p->get_mmu();
    for (size_t i = 0; i < n; i++)
    {
        mmu->store<uint8_t>(dest + i, *((uint8_t *)src + i));
    }
}

extern "C"
{

    __EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction)
    {
        if (direction == DIFFTEST_TO_REF)
        {
            s->diff_memcpy(addr, buf, n);
        }
        else
        {
            assert(0);
        }
    }

    __EXPORT void difftest_regcpy(void *dut, bool direction)
    {
        if (direction == DIFFTEST_TO_REF)
        {
            s->diff_set_regs(dut);
        }
        else
        {
            s->diff_get_regs(dut);
        }
    }

    __EXPORT void difftest_exec(uint64_t n)
    {
        s->diff_step(n);
    }

    __EXPORT void difftest_init(int port)
    {
        difftest_htif_args.push_back("");
        const char *isa = "RV" MUXDEF(CONFIG_RV64, "64", "32") MUXDEF(CONFIG_RVE, "E", "I") "MAFDC";
        cfg_t cfg(/*default_initrd_bounds=*/std::make_pair((reg_t)0, (reg_t)0),
                  /*default_bootargs=*/nullptr,
                  /*default_isa=*/isa,
                  /*default_priv=*/DEFAULT_PRIV,
                  /*default_varch=*/DEFAULT_VARCH,
                  /*default_misaligned=*/false,
                  /*default_endianness*/ endianness_little,
                  /*default_pmpregions=*/16,
                  /*default_mem_layout=*/std::vector<mem_cfg_t>(),
                  /*default_hartids=*/std::vector<size_t>(1),
                  /*default_real_time_clint=*/false,
                  /*default_trigger_count=*/4);
        s = new sim_t(&cfg, false, difftest_mem, difftest_plugin_devices, difftest_htif_args, difftest_dm_config, nullptr,
                      false, NULL, false, NULL, true);
        s->diff_init(port);
    }

    __EXPORT void difftest_raise_intr(uint64_t NO)
    {
        trap_t t(NO);
        p->take_trap_public(t, state->pc);
    }
}
