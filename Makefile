XBE_TITLE = xbox_xmb_dash
SRCS = $(wildcard src/*.c)
NXDK_DIR ?= $(CURDIR)/nxdk
export NXDK_DIR

# We want to use SDL2, which is provided by nxdk
NXDK_SDL = y
CFLAGS += -I$(CURDIR)/include

include $(NXDK_DIR)/Makefile
