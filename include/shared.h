#ifndef __SHARED_H__
#define __SHARED_H__

#include <common.h>
#include <cpu/cpu.h>
#include <cpu/difftest.h>
#include <isa.h>
#include <memory/vaddr.h>

inline void reg_display()
{
    isa_reg_display(&cpu);
}
#define reg_str2val isa_reg_str2val

extern void quit();

#endif