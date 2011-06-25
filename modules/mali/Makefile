
all:
	@echo "chip_id: $(CONFIG_CHIP_ID)"
	export LANG=en_US.UTF-8
	make -C sun4i/DX910-SW-99002-r2p1-05rel1/src/devicedrv/mali USING_MMU=1 USING_UMP=0 USING_PMM=0 BUILD=release \
		CONFIG=ca8-virtex820-m400-1 CROSS_COMPILE=$(HOST) KDIR=$(KERNEL_DIR)

install:
	@echo "installing mali driver"
	cp -v sun4i/DX910-SW-99002-r2p1-05rel1/src/devicedrv/mali/mali.ko $(LICHEE_MOD_DIR)/mali.ko

