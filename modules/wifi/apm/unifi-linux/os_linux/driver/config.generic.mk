KDIR ?= /lib/modules/$(shell uname -r)/build
SDIO_DRIVER ?= emb
SME ?= csr_wext

ifeq ($(SDIO_DRIVER),emb)
ifeq ($(SDIODIR),)
$(error Set SDIODIR to the sdioemb driver path e.g. ../../sdioemb )
endif
endif

modules:
	$(MAKE) -C $(KDIR) M=$(BUILDDIR)/os_linux/driver \
		O=$(O) V=$(V)

install_modules:
	$(MAKE) -C $(KDIR) M=$(BUILDDIR)/os_linux/driver modules_install INSTALL_MOD_PATH=$(INSTALL_DIR) \
		O=$(O) V=$(V)

# Kbuild's clean target doesn't play nicely with our Kbuild file:
# ../lib_hip/ isn't cleaned because it's above M; and .config isn't
# included so SDIO_PLATFORM isn't defined.
clean_modules:
	$(MAKE) -C $(KDIR) M=$(BUILDDIR)/os_linux/driver clean \
		O=$(O) V=$(V) SDIO_PLATFORM=unused
	rm -f $(BUILDDIR)/lib_hip/*.o $(BUILDDIR)/lib_hip/.*.o.cmd
	rm -f $(BUILDDIR)/os_linux/driver/sme_csr/event_pack_unpack/event_pack_unpack.c
	rm -f $(BUILDDIR)/os_linux/driver/sme_csr/event_pack_unpack/event_pack_unpack.h
	rm -f $(BUILDDIR)/os_linux/driver/Module.markers
	rm -f $(BUILDDIR)/os_linux/driver/modules.order

ifneq ($(CROSS_COMPILE),)
export CROSS_COMPILE
endif

ifneq ($(ARCH),)
export ARCH
endif

export SDIO_DRIVER
export SDIODIR
export SME
