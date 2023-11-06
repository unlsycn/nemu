ifneq ($(CONFIG_BRANCH_PREDICTION),)
CXXSRC += src/cpu/bp/bp.cc
LDFLAGS += -lfmt
INC_PATH += /home/unlsycn/Workspaces/BranchPrediction
endif