ifeq ($(CONFIG),)
$(error CONFIG not set)
endif

all: driver

include config.$(CONFIG).mk

DRIVERTOP := $(shell pwd)
OSTOP := $(DRIVERTOP)

BUILDDIR := $(DRIVERTOP)

driver: buildtree modules

tools: buildtree
	$(MAKE) -C $(BUILDDIR)

install: install_driver post_install_hook

install_driver: driver install_modules

clean: clean_modules

buildtree: $(BUILDDIR)

.PHONY: post_install_hook
