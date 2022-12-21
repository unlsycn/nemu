SRCS-y += src/nemu-main.c
DIRS-y += src/cpu src/monitor src/utils shared/src
DIRS-$(CONFIG_MODE_SYSTEM) += src/memory
DIRS-BLACKLIST-$(CONFIG_TARGET_AM) += shared/src/sdb
INC_PATH += shared/include

SHARE = $(if $(CONFIG_TARGET_SHARE),1,0)
LIBS += $(if $(CONFIG_TARGET_NATIVE_ELF),-lreadline -ldl -pie,)

ifdef mainargs
ASFLAGS += -DBIN_PATH=\"$(mainargs)\"
endif
SRCS-$(CONFIG_TARGET_AM) += src/am-bin.S
.PHONY: src/am-bin.S
