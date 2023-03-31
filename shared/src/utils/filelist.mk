ifdef CONFIG_ITRACE
CXXSRC = shared/src/utils/disasm.cc
CXXFLAGS += $(shell llvm-config --cxxflags) -fPIE
LIBS += $(shell llvm-config --libs)
endif
