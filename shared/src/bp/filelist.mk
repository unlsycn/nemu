ifneq ($(CONFIG_BRANCH_PREDICTION),)
CXXSRC += shared/src/bp/bp.cc
LDFLAGS += -lfmt
INC_PATH += /home/unlsycn/Workspaces/BranchPrediction
endif