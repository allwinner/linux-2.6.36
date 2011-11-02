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
#   $Id: //depot/dot11/v7.0p/host/lib_nme/Makefile#2 $
#
# --------------------------------------------------------------------


# --------------------------------------------------------------------
# Setup the Top level Paths
# --------------------------------------------------------------------
SOURCE_TREE_ROOT = $(shell pwd)
SME_ROOT = $(SOURCE_TREE_ROOT)/../lib_sme
SME_BUILD = $(SME_ROOT)/build
SME_BUILD_TREE_ROOT = $(SME_ROOT)/../builds/sme/$(SME_TARGET_NAME)
INFO_ELEMENT_ROOT = $(SOURCE_TREE_ROOT)/../lib_info_elements
CRYPTO_ROOT = $(SOURCE_TREE_ROOT)/../lib_crypto
SYNERGY_FW_ROOT   = $(SOURCE_TREE_ROOT)/../lib_synergy_framework/$(OSA_VARIANT)

# --------------------------------------------------------------------
# Load the configuration files
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.buildcfg.inc
BUILD_TREE_ROOT = $(SOURCE_TREE_ROOT)/../builds/lib_nme/$(SME_TARGET_NAME)

include $(CRYPTO_ROOT)/Makefile.security.inc

# --------------------------------------------------------------------
# Help Target
# --------------------------------------------------------------------
.PHONY: help
help:
	@echo " "
	@echo "================================================"
	@echo "Network Management Entity Library"
	@echo "================================================"
	@echo " Build Config"
	@echo "  CONFIG=<> options: See builds/buildcfg/Makefile.base.*.cfg"
	@echo "  TYPE=<> options: See builds/buildcfg/Makefile.type.*.cfg"
	@echo "  PLATFORM=<> options: See builds/buildcfg/Makefile.platform.*.inc"
	@echo " "
	@echo "Targets: "
	@echo "    clean     - clean up all build output"
	@echo "    build     - NME Library"
	@echo "    all       - performs clean, autogen and build targets"
	@echo "    lint      - lint all files"
	@echo " "

# --------------------------------------------------------------------
# Include Paths
# --------------------------------------------------------------------
CFG_INCLUDES = -I$(SOURCE_TREE_ROOT)/nme \
               -I$(SOURCE_TREE_ROOT)/saps \
               -I$(SOURCE_TREE_ROOT)/common \
               -I$(SYNERGY_FW_ROOT) \
               -I$(SME_ROOT)/saps \
               -I$(SME_ROOT)/sme \
               -I$(SME_ROOT)/common \
   		       -I$(SME_ROOT)/ccx \
               -I$(SOURCE_TREE_ROOT)/../lib_security \
               -I$(SOURCE_TREE_ROOT)/../lib_crypto \
               -I$(INFO_ELEMENT_ROOT)

# --------------------------------------------------------------------
# Source Files
# --------------------------------------------------------------------
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/saps/*/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/common/*/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_connection_manager_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_core_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_profile_manager_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_security_manager_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_top_level_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_wps_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_network_selector_fsm/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_signal_routing_fsm/*.c)
ifneq ($(CCX_VARIANT), )
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/nme/nme_ccx/*.c)
endif

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

.PHONY: cleanall
cleanall: clean

.PHONY: clean
clean:
	-rm -rf $(BUILD_TREE_ROOT)

.PHONY: build
build: $(BUILD_TREE_ROOT)/libnme.a
