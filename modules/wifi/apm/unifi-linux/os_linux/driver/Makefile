# 
# Makefile for Linux driver
#
# Copyright (C) 2005-2008 by Cambridge Silicon Radio Ltd.
#
# Refer to LICENSE.txt included with this source code for details on
# the license terms.
#

ifeq ($(CONFIG),)
$(error CONFIG not set)
endif

#all: driver tools
all: driver

include config.$(CONFIG).mk

DRIVERTOP := $(shell pwd)/..
OSTOP := $(DRIVERTOP)/os_linux/driver

# If the ../packaging/ is missing this is a build from released
# sources, so build in-place.
ifeq ($(shell test -d $(DRIVERTOP)/packaging && echo y),y)
BUILDDIR := $(DRIVERTOP)/../builds/driver/$(CONFIG)
else
BUILDDIR := $(DRIVERTOP)/..
endif


driver: sme_common_files buildtree modules install_modules

tools: buildtree
	$(MAKE) -C $(BUILDDIR)/os_linux/tools

install: install_driver install_tools post_install_hook

install_driver: driver install_modules

install_tools: tools
	$(MAKE) -C $(BUILDDIR)/os_linux/tools install

clean: clean_modules clean_tools

clean_tools:
	$(MAKE) -C $(BUILDDIR)/os_linux/tools clean

sme_common_files: sme_csr/event_pack_unpack/event_pack_unpack.h sme_csr/event_pack_unpack/event_pack_unpack.c

sme_csr/event_pack_unpack/event_pack_unpack.h: $(DRIVERTOP)/../lib_sme/common/event_pack_unpack/event_pack_unpack.h
	mkdir -p sme_csr/event_pack_unpack
	cp -f $< $@

sme_csr/event_pack_unpack/event_pack_unpack.c: $(DRIVERTOP)/../lib_sme/common/event_pack_unpack/event_pack_unpack.c
	mkdir -p sme_csr/event_pack_unpack
	cp -f $< $@

buildtree: $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	find $(BUILDDIR) -type l -exec rm '{}' ';'
	set -e ; for d in common lib_hip os_linux/driver os_linux/tools lib_sme lib_info_elements lib_synergy_framework; do \
		mkdir -p $(BUILDDIR)/$$d ; \
		( cd $(BUILDDIR)/$$d && $(DRIVERTOP)/scripts/lndir $(DRIVERTOP)/../$$d ) ; \
	done



.PHONY: post_install_hook
