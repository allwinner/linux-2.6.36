KDIR ?= /lib/modules/$(shell uname -r)/build

modules:
	$(MAKE) -C $(KDIR) M=$(BUILDDIR) \
		O=$(O) V=$(V)

install_modules:
	$(MAKE) -C $(KDIR) M=$(BUILDDIR) modules_install INSTALL_MOD_PATH=$(DESTDIR) \
		O=$(O) V=$(V)

# Kbuild's clean target doesn't play nicely with our Kbuild file:
# ../core/ isn't cleaned because it's above M; and .config isn't
# included so SDIO_PLATFORM isn't defined.
clean_modules:
	$(MAKE) -C $(KDIR) M=$(BUILDDIR) clean \
		O=$(O) V=$(V) SDIO_PLATFORM=unused

ifneq ($(CROSS_COMPILE),)
export CROSS_COMPILE
endif

ifneq ($(ARCH),)
export ARCH
endif

