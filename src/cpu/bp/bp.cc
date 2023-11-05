#include "bp.hh"

#include <array>
#include <cstdint>
#include <fmt/core.h>
#include <memory>
#include <ratio>
#include <vector>

#include "bimodal.hh"
#include "lbp.hh"
#include "macro.h"
#include "ras.hh"
#include "tage.hh"
#include "tage_config.hh"

using DirectionPredPtr = std::unique_ptr<IDirectionPredictor>;
using CallReturnPredPtr = std::unique_ptr<ICallReturnPredictor>;
using AddressPredPtr = std::unique_ptr<IAddressPredictor>;

std::vector<DirectionPredPtr> dir_predictors;
std::vector<CallReturnPredPtr> rass;
std::vector<AddressPredPtr> btbs;

extern "C"
{
    void init_pred()
    {
#define BIMODAL(width) dir_predictors.push_back(DirectionPredPtr(new Bimodal<2, width>()));
#define GSELECT(his, pc) dir_predictors.push_back(DirectionPredPtr(new LocalBranchPredictor<2, 0, his, pc, his + pc>));
#define GSHARE(his, pht) \
    dir_predictors.push_back(DirectionPredPtr(new LocalBranchPredictor<2, 0, his, 16, pht, IndexAlgo::XOR>()));
#define LOCAL5(his, pc)                                                                            \
    dir_predictors.push_back(DirectionPredPtr(new LocalBranchPredictor<2, 5, his, pc, his + pc>)); \
    dir_predictors.push_back(DirectionPredPtr(new LocalBranchPredictor<2, 7, his, pc, his + pc>));
#define RAS(depth, width) rass.push_back(CallReturnPredPtr(new Ras<depth, width>()));

        MAP(BIMODAL, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14);
        MAP_TWO_ARGS(LOCAL5, 10, 6, 10, 4, 8, 6, 12, 2, 8, 2);
        MAP_TWO_ARGS(GSELECT, 5, 6, 5, 7, 6, 6, 6, 8);
        MAP_TWO_ARGS(GSHARE, 5, 10, 5, 11, 5, 12, 6, 10, 6, 11, 6, 12, 6, 13);
        MAP_TWO_ARGS(RAS, 10, 0, 5, 0, 3, 2, 3, 0, 2, 1, 2, 0, 1, 0);

        // dir_predictors.push_back(
        //     DirectionPredPtr(new Tage<12, 3, 2, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false,
        //     false,
        //                               1, AllocCond::FINAL_MISPRED, false, true, false, success_reset_strategy>));
    }

    void branch_check_pred(uint64_t ip, bool taken)
    {
        for (auto &pred : dir_predictors)
            pred.get()->checkPred(ip, taken);
    }

    void call_update_pred(uint64_t npc)
    {
        for (auto &ras : rass)
            ras.get()->push(npc);
    }

    void ret_check_pred(uint64_t addr)
    {
        for (auto &ras : rass)
            ras.get()->checkPred(addr);
    }

    void jmp_check_pred(uint64_t ip, uint64_t addr)
    {
        for (auto &btb : btbs)
            btb->checkPred(ip, addr);
    }

    void bp_statistic()
    {
        for (auto &bimodal : dir_predictors)
            bimodal.get()->statistic();
        for (auto &ras : rass)
            ras.get()->statistic();
    }
}