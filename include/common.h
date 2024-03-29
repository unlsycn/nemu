#ifndef __COMMON_H__
#define __COMMON_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <generated/autoconf.h>
#include <macro.h>

#ifdef CONFIG_TARGET_AM
#include <klib.h>
#else
#include <assert.h>
#include <stdlib.h>
#endif

#if CONFIG_MBASE + CONFIG_MSIZE > 0x100000000ul
#define PMEM64 1
#endif

typedef MUXDEF(CONFIG_ISA64, uint64_t, uint32_t) word_t;
typedef MUXDEF(CONFIG_ISA64, int64_t, int32_t) sword_t;
#define FMT_WORD MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32)
#define FMT_WORD_LH MUXDEF(CONFIG_ISA64, "0x%-16" PRIx64, "0x%-8" PRIx32)
#define FMT_WORD_LD MUXDEF(CONFIG_ISA64, "%-20" PRIu64, "%-10" PRIu32)
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64

typedef word_t vaddr_t;
#define PADDR_LEN MUXDEF(PMEM64, 64, 32)
typedef MUXDEF(PMEM64, uint64_t, uint32_t) paddr_t;
#define FMT_PADDR MUXDEF(PMEM64, "0x%016" PRIx64, "0x%08" PRIx32)
typedef uint16_t ioaddr_t;

#include <debug.h>

#endif
