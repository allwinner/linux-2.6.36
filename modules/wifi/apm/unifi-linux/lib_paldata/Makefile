# --------------------------------------------------------------------
# FILE
#   Makefile - Top level makefile
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2006-2007. All rights reserved.
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
#   $Id: //depot/dot11/v7.0p/host/lib_paldata/Makefile#1 $
#
# --------------------------------------------------------------------


# --------------------------------------------------------------------
# Setup the Top level Paths
# --------------------------------------------------------------------
SOURCE_TREE_ROOT	= $(shell pwd)
SME_ROOT			= $(SOURCE_TREE_ROOT)/../lib_sme
SME_BUILD			= $(SME_ROOT)/build
SME_BUILD_TREE_ROOT = $(SME_ROOT)/../builds/sme/$(SME_TARGET_NAME)
SYNERGY_FW_ROOT     = $(SOURCE_TREE_ROOT)/../lib_synergy_framework/$(OSA_VARIANT)

# --------------------------------------------------------------------
# Load the configuration files
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.buildcfg.inc
BUILD_TREE_ROOT = $(SOURCE_TREE_ROOT)/../builds/lib_paldata/$(SME_TARGET_NAME)

# --------------------------------------------------------------------
# Help Target
# --------------------------------------------------------------------
.PHONY: help
help:
	@echo " "
	@echo "================================================"
	@echo "Pal Data Library"
	@echo "================================================"
	@echo " Build Config"
	@echo "  CONFIG=<> options: See builds/buildcfg/Makefile.base.*.cfg"
	@echo "  TYPE=<> options: See builds/buildcfg/Makefile.type.*.cfg"
	@echo "  PLATFORM=<> options: See builds/buildcfg/Makefile.platform.*.inc"
	@echo " "
	@echo "Targets: "
	@echo "    clean		- clean up all build output and autogen files"
	@echo "    build		- Paldata Library"
	@echo "    autogen		- autogen builds"
	@echo "    all			- clean autogen build options"
	@echo "    lint			- lint all files"
	@echo " "

# --------------------------------------------------------------------
# Include Paths
# --------------------------------------------------------------------
CFG_INCLUDES =  -I$(SOURCE_TREE_ROOT)/paldata \
				-I$(SME_ROOT)/sme \
				-I$(SOURCE_TREE_ROOT)/saps \
				-I$(SOURCE_TREE_ROOT)/common \
				-I$(BUILD_TREE_ROOT)/paldata/saps \
				-I$(SME_BUILD_TREE_ROOT)/sme/saps \
				-I$(SME_ROOT)/saps \
				-I$(SME_ROOT)/ccx \
				-I$(SME_ROOT)/common \
				-I$(SME_ROOT)/common/abstractions/$(OSA_VARIANT) \
				-I$(SYNERGY_FW_ROOT)

# --------------------------------------------------------------------
# Source Files
# --------------------------------------------------------------------
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/saps/*/*.c)
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/paldata/*/*.c)

# --------------------------------------------------------------------
# Include compile and link rules
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.rules.inc

PERL_INCLUDES = -I $(SME_ROOT)/../../common/hostio/ -I $(SME_ROOT)/../common/smeio/ -I $(SME_BUILD)/scripts/ 
# --------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------
.PHONY: all
all:
	+make cleanall
	+make autogen
	+make build

.PHONY: autogen
autogen:
	@$(PERL) $(PERL_INCLUDES) $(SOURCE_TREE_ROOT)/build/scripts/autogen.pl \
				-h $(SME_ROOT)/../../common/hostio \
				-l $(SOURCE_TREE_ROOT)/build/scripts/event_base_ids_3.txt \
				-p $(SOURCE_TREE_ROOT)/build/scripts/saps.txt \
				-s $(SOURCE_TREE_ROOT) \
				-a $(SME_ROOT)/../common/smeio \
				-d "$(patsubst -D,,$(CFG_DEFINES))"

.PHONY: cleanall
cleanall: clean
	@$(PERL) $(PERL_INCLUDES) $(SOURCE_TREE_ROOT)/build/scripts/autogen_clean.pl \
				-l $(SOURCE_TREE_ROOT)/build/scripts/event_base_ids_3.txt \
				-p $(SOURCE_TREE_ROOT)/build/scripts/saps.txt \
				-s $(SOURCE_TREE_ROOT)

.PHONY: clean
clean:
	-rm -rf $(BUILD_TREE_ROOT)

.PHONY: build
build: $(BUILD_TREE_ROOT)/libpaldata.a
