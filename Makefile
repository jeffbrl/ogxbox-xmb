GEN_XISO = $(XBE_TITLE).iso
SRCS = $(wildcard src/*.c)
NXDK_DIR ?= $(CURDIR)/nxdk
export NXDK_DIR
export PATH := $(NXDK_DIR)/bin:$(PATH)

# We want to use SDL2, which is provided by nxdk
NXDK_SDL = y
NXDK_NET = y
CFLAGS += -I$(CURDIR)/include

include $(NXDK_DIR)/Makefile
