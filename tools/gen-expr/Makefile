NAME = gen-expr
SRCS = gen-expr.c
CC = clang
LOOP := 10
DEPTH := 1000

CFLAGS += -glldb -DLOOP=$(LOOP) -DDEPTH=$(DEPTH)
include $(NEMU_HOME)/scripts/build.mk


test: $(BINARY)
	@$(BINARY)
	@$(NEMU_HOME)/build/riscv64-nemu-interpreter < /tmp/.input > /tmp/.nemu.result
	@sed -i '1,14d' /tmp/.nemu.result
	@sed -i '/nemu/d' /tmp/.nemu.result
	diff /tmp/.result /tmp/.nemu.result

.PHONY: test