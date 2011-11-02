# --------------------------------------------------------------------
# FILE
#   Makefile - Top level makefile
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2006-2009. All rights reserved.
#
# DESCRIPTION
#   This is the main Makefile that orchestrates the building of the IEEE
#   802.11 SME software. 
#
#   To kick off the top level build type 'make' from the $(SME_BASE)
#   directory.  That will display the help listing the public target
#   options.
#
# --------------------------------------------------------------------
#
# REVISION
#   $Id: //depot/dot11/v7.0p/host/lib_synergy_framework/Makefile#1 $
#
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# Setup the Top level Paths
# --------------------------------------------------------------------
SOURCE_TREE_ROOT = $(shell pwd)
SME_BUILD = $(SOURCE_TREE_ROOT)/../lib_sme/build

# --------------------------------------------------------------------
# Load the configuration files
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.buildcfg.inc
BUILD_TREE_ROOT = $(SOURCE_TREE_ROOT)/../builds/lib_synergy_framework/$(SME_TARGET_NAME)

# --------------------------------------------------------------------
# Help Target
# --------------------------------------------------------------------
.PHONY: help
help:
	@echo " "
	@echo "================================================"
	@echo "Simple Framework Library"
	@echo "================================================"
	@echo " Build Config"
	@echo "  CONFIG=<> options: See builds/buildcfg/Makefile.base.*.cfg"
	@echo "  TYPE=<> options: See builds/buildcfg/Makefile.type.*.cfg"
	@echo "  PLATFORM=<> options: See builds/buildcfg/Makefile.platform.*.inc"
	@echo " "
	@echo "Targets: "
	@echo "    clean     - clean up build output"
	@echo "    build     - Build the library"
	@echo " "

# --------------------------------------------------------------------
# Include Paths
# --------------------------------------------------------------------
CFG_INCLUDES = -I$(SOURCE_TREE_ROOT)/linux

# --------------------------------------------------------------------
# Source Files
# --------------------------------------------------------------------
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/linux/*.c)

# --------------------------------------------------------------------
# Include compile and link rules
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.rules.inc

# --------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------
.PHONY: all
all:
	+make clean
	+make build


.PHONY: clean
clean:
	-rm -rf $(BUILD_TREE_ROOT)

.PHONY: build
build: $(BUILD_TREE_ROOT)/libsynergyframework.a
