# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core
BLOCKSDSEXT	?= /opt/blocksds/external

export LIBTWL           ?= $(shell pwd)/libs/libtwl

# User config
# ===========

NAME		:= 	DsRunner

# Version
# -------
# The file VERSION holds the version number (X.Y.Z, without "v"), see docs/RELEASE.md.
# Release builds (make DSRUNNER_RELEASE=1, done by the release workflow) use it as it is,
# every other build is marked as development build with the commit if git knows it,
# e.g. 0.2.0-dev+36184e8. The launcher shows it in the "Anzeige" menu, the banner in its
# title line.

DSRUNNER_BASE_VERSION	:= $(strip $(shell cat VERSION))
ifeq ($(DSRUNNER_RELEASE),1)
    DSRUNNER_VERSION	?= $(DSRUNNER_BASE_VERSION)
else
    DSRUNNER_COMMIT	:= $(strip $(shell git rev-parse --short HEAD 2>/dev/null))
    DSRUNNER_VERSION	?= $(DSRUNNER_BASE_VERSION)-dev$(if $(DSRUNNER_COMMIT),+$(DSRUNNER_COMMIT))
endif
export DSRUNNER_VERSION

GAME_TITLE	:= DsRunner $(DSRUNNER_VERSION)
GAME_SUBTITLE	:= based on Pico Launcher
GAME_AUTHOR	:= HolziDape
GAME_ICON	:= icon.png

# DLDI and internal SD slot of DSi
# --------------------------------

# Root folder of the SD image
SDROOT		:= sdroot
# Name of the generated image it "DSi-1.sd" for no$gba in DSi mode
SDIMAGE		:= image.bin

# Source code paths
# -----------------

# A single directory that is the root of NitroFS:
NITROFSDIR	:=

# Tools
# -----

MAKE		:= make
RM		:= rm -rf

# Verbose flag
# ------------

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

# Directories
# -----------

ARM9DIR		:= arm9
ARM7DIR		:= arm7

# Build artfacts
# --------------

ROM		:= $(NAME).nds

# Targets
# -------

.PHONY: all clean arm9 arm7 dldipatch sdimage checklibtwl

all: $(ROM)

clean:
	@echo "  CLEAN"
	$(V)$(MAKE) -f Makefile.arm9 clean --no-print-directory
	$(V)$(MAKE) -f Makefile.arm7 clean --no-print-directory
	$(V)$(RM) $(ROM) build $(SDIMAGE)

arm9: checklibtwl
	$(V)+$(MAKE) -f Makefile.arm9 --no-print-directory

arm7: checklibtwl
	$(V)+$(MAKE) -f Makefile.arm7 --no-print-directory

checklibtwl:
	$(MAKE) -C $(LIBTWL)

ifneq ($(strip $(NITROFSDIR)),)
# Additional arguments for ndstool
NDSTOOL_ARGS	:= -d $(NITROFSDIR)

# Make the NDS ROM depend on the filesystem only if it is needed
$(ROM): $(NITROFSDIR)
endif

# Combine the title strings
ifeq ($(strip $(GAME_SUBTITLE)),)
    GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_AUTHOR)
else
    GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_SUBTITLE);$(GAME_AUTHOR)
endif

$(ROM): arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(BLOCKSDS)/tools/ndstool/ndstool -c $@ \
		-7 build/arm7.elf -9 build/arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)" \
		$(NDSTOOL_ARGS)

sdimage:
	@echo "  MKFATIMG $(SDIMAGE) $(SDROOT)"
	$(V)$(BLOCKSDS)/tools/mkfatimg/mkfatimg -t $(SDROOT) $(SDIMAGE)

dldipatch: $(ROM)
	@echo "  DLDIPATCH $(ROM)"
	$(V)$(BLOCKSDS)/tools/dldipatch/dldipatch patch \
		$(BLOCKSDS)/sys/dldi_r4/r4tf.dldi $(ROM)
