# --------------------------------------------------------------------
# FILE
#   Makefile - Top level makefile
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2006-2007. All rights reserved.
#
# DESCRIPTION
#   This is the main Makefile that orchestrates the building of the Crypto Library. 
#
# REVISION
#   $Id: //depot/dot11/v7.0p/host/lib_info_elements/Makefile#1 $
#
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# Default Build CFG
# --------------------------------------------------------------------
BUILDCFG=LibInfoElements

# --------------------------------------------------------------------
# Setup the Top level Paths
# --------------------------------------------------------------------
SOURCE_TREE_ROOT	= $(shell pwd)
SME_ROOT			= $(SOURCE_TREE_ROOT)/../lib_sme
SME_BUILD			= $(SME_ROOT)/build
LIB_IE_BUILD		= $(SOURCE_TREE_ROOT)/build
SYNERGY_FW_ROOT      = $(SOURCE_TREE_ROOT)/../lib_synergy_framework/$(OSA_VARIANT)

# --------------------------------------------------------------------
# Load the configuration files
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.buildcfg.inc
BUILD_TREE_ROOT = $(SOURCE_TREE_ROOT)/../builds/lib_info_elements/$(SME_TARGET_NAME)

# --------------------------------------------------------------------
# Help Target
# --------------------------------------------------------------------
.PHONY: help
help:
	@echo " "
	@echo "================================================"
	@echo "Security Library"
	@echo "================================================"
	@echo " Build Config"
	@echo "  CONFIG=<> options: See builds/buildcfg/Makefile.base.*.cfg"
	@echo "  TYPE=<> options: See builds/buildcfg/Makefile.type.*.cfg"
	@echo "  PLATFORM=<> options: See builds/buildcfg/Makefile.platform.*.inc"
	@echo " "
	@echo "Targets: "
	@echo "    clean        - clean up all build output"
	@echo "    build        - Crypto Library"
	@echo "    all          - clean build options"
	@echo "    lint         - lint all files"
	@echo " "

# --------------------------------------------------------------------
# Include Paths
# --------------------------------------------------------------------
CFG_INCLUDES =  -I$(SOURCE_TREE_ROOT) \
				-I$(SOURCE_TREE_ROOT)/.. \
				-I$(SME_ROOT)/common \
				-I$(SYNERGY_FW_ROOT) \

# --------------------------------------------------------------------
# Source Files
# --------------------------------------------------------------------
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/ie_access/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/ie_access_support/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/ie_utils/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/ie_access_trace/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/ie_test/*.c)

# --------------------------------------------------------------------
# Include compile and link rules
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.rules.inc

# --------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------
.PHONY: all
all:
	$(MAKE) cleanall
	$(MAKE) autogen
	$(MAKE) build

.PHONY: autogen
autogen:
	$(PERL)  $(SME_ROOT)/../common/autogen/generate_ie.pl

.PHONY: cleanall
cleanall: clean
	$(PERL)  $(SME_ROOT)/../common/autogen/generate_ie.pl --clean

.PHONY: clean
clean:
	-rm -rf $(BUILD_TREE_ROOT)

.PHONY: build

build: $(BUILD_TREE_ROOT)/libinfoelements.a
