
TARGETNAME=lib_hip

PDRV_DIR=..\..
SME_DIR=$(PDRV_DIR)\os_windows\sme
CORE_DIR=..


UNIFI_C_DEFINES=\
	$(UNIFI_C_DEFINES) \
	-D__KERNEL__


UNIFI_INCLUDES=\
	$(UNIFI_INCLUDES) \
	$(CORE_DIR);			\
	$(PDRV_DIR)/common; \
	$(PDRV_DIR)/os_windows/include;	\
	$(SME_DIR)/lib_sme/common; \


UNIFI_SOURCES=\
	$(UNIFI_SOURCES) \
	$(CORE_DIR)\card_sdio.c  \
	$(CORE_DIR)\card_sdio_intr.c  \
	$(CORE_DIR)\card_sdio_mem.c  \
	$(CORE_DIR)\chiphelper.c \
	$(CORE_DIR)\chiphelper.c \
	$(CORE_DIR)\download.c  \
	$(CORE_DIR)\packing.c \
	$(CORE_DIR)\send.c  \
	$(CORE_DIR)\dump.c  \
	$(CORE_DIR)\signals.c \
	$(CORE_DIR)\ta_sampling.c  \
	$(CORE_DIR)\udi.c  \
	$(CORE_DIR)\unifi_signal_names.c  \
	$(CORE_DIR)\xbv.c \