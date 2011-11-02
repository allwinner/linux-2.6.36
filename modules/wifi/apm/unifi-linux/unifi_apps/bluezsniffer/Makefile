COMMONROOT=../../unifi_helper/common
SMEROOT=../../lib_sme
FRAMEWORKROOT=../../lib_synergy_framework/linux

INC += -I.
INC += -I$(COMMONROOT)
INC += -I$(FRAMEWORKROOT)
INC += -I$(SMEROOT)/common

DEFINES += -DIPC_IP

SAPFILES += bt_sap/bt_sap_remoteserver_from_sme_interface.c
SAPFILES += bt_sap/bt_sap_remoteserver_to_sme_interface.c
SAPFILES += bt_sap/bt_sap_serialise.c
SAPFILES += $(COMMONROOT)/ipc/ipc.c
SAPFILES += $(COMMONROOT)/ipc/linux/ipc_ip.c
SAPFILES += $(SMEROOT)/common/event_pack_unpack/event_pack_unpack.c

LIBS += -lbluetooth

FATHCIDUMPROOT=bluez-hcidump-1.42
FATHCIDUMPINC += -I$(FATHCIDUMPROOT)

FATHCIDUMPFILES += hcidump_to_sme.c
FATHCIDUMPFILES += $(FATHCIDUMPROOT)/src/hcidump.c
FATHCIDUMPFILES += $(wildcard $(FATHCIDUMPROOT)/parser/*.c)

LITEHCIDUMPROOT=bluez-hcidump-1.42-lite
LITEHCIDUMPINC += -I$(LITEHCIDUMPROOT)

LITEHCIDUMPFILES += hcidump_to_sme.c
LITEHCIDUMPFILES += $(LITEHCIDUMPROOT)/src/hcidump.c
LITEHCIDUMPFILES += $(wildcard $(LITEHCIDUMPROOT)/parser/*.c)

# --------------------------------------------------------
# Compiler options
# --------------------------------------------------------
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -Werror


all: fat lite

fat:
	$(CC) $(CFLAGS) -g $(FATHCIDUMPINC) $(INC) $(DEFINES) $(LIBS) $(FATHCIDUMPFILES) $(SAPFILES) -o csrbluezcoexsnifferfat

lite:
	$(CC) $(CFLAGS) -g $(LITEHCIDUMPINC) $(INC) $(DEFINES) $(LIBS) $(LITEHCIDUMPFILES) $(SAPFILES) -o csrbluezcoexsniffer

