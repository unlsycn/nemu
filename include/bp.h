#ifndef __BP_H__
#define __BP_H__

#include <stdbool.h>
#include <stdint.h>

void init_pred();

void pred_branch_direction(uint64_t ip, bool taken);

void pred_call(uint64_t addr);

void pred_ret(uint64_t addr);

void lookup_btb(uint64_t ip, bool is_ct_inst, uint64_t addr);

void bp_statistic();

#endif
