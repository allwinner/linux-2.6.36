##############################################################################
# FILE
#   Makefile - Simple linux example makefile 
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2007. All rights reserved.
#
# DESCRIPTION
#   Simple example that builds an SME executable under linux
#
##############################################################################

SBINDIR = /usr/sbin

CROSS_COMPILE:=arm-none-linux-gnueabi-

LIB_SME_ROOT = ../../lib_sme
DRV_ROOT   = ../../os_linux

# --------------------------------------------------------
# SME Include paths
# --------------------------------------------------------
SME_INCLUDES += -I../common
SME_INCLUDES += -I../common/saps_impl
SME_INCLUDES += -I$(LIB_SME_ROOT)/sme
SME_INCLUDES += -I$(LIB_SME_ROOT)/saps
SME_INCLUDES += -I$(LIB_SME_ROOT)/common
SME_INCLUDES += -I../../lib_synergy_framework/linux
SME_INCLUDES += -I$(LIB_SME_ROOT)/../lib_info_elements
SME_INCLUDES += -I$(DRV_ROOT)

# --------------------------------------------------------
#  SME DEFINES
# --------------------------------------------------------
CFLAGS += $(CFLAGSEXTRA)
CFLAGS += -DFSM_MUTEX_ENABLE
CFLAGS += -DFSM_TRANSITION_LOCK
CFLAGS += -DSME_SYNC_ACCESS

CFLAGS += -DIPC_CHARDEVICE
CFLAGS += -DREMOTE_SYS_SAP -DSYS_SAP_IPC_CHARDEVICE
CFLAGS += -DIPC_IP
CFLAGS += -DREMOTE_BT_SAP -DBT_SAP_IPC_IP

# ... Optional debug and trace defines below ... 
#CFLAGS += -DSME_TRACE_ENABLE
#CFLAGS += -DSME_TRACE_TYPES_ENABLE
#CFLAGS += -DFSM_DEBUG
#CFLAGS += -DFSM_DEBUG_DUMP

#CFLAGS += -DSME_TRACE_NO_ENTRY
#CFLAGS += -DSME_TRACE_NO_DEBUG
#CFLAGS += -DSME_TRACE_NO_INFO
#CFLAGS += -DSME_TRACE_NO_WARN
#CFLAGS += -DSME_TRACE_NO_ERROR
#CFLAGS += -DSME_TRACE_NO_HEX

#CFLAGS += -DSME_PBC_NO_ASSERTS
#CFLAGS += -DSME_PBC_NO_REQUIRE
#CFLAGS += -DSME_PBC_NO_VERIFY
#CFLAGS += -DSME_PBC_NO_ENSURE
#CFLAGS += -DSME_PBC_NO_INVARIANT
CFLAGS += -DSME_PBC_NO_ASSERTS

# --------------------------------------------------------
# SME Source Files
# --------------------------------------------------------
SME_SRC += $(wildcard $(LIB_SME_ROOT)/sme/*/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/bt_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/dbg_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/mgt_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/sys_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/*/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/../lib_synergy_framework/linux/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/../lib_info_elements/*/*.c)

# Release compiler options Overridden when debug target is used
CFLAGSEXTRA=-O2 -fno-strict-aliasing

# --------------------------------------------------------
# Targets
# --------------------------------------------------------
all: lib bin

debug: CFLAGSEXTRA= -O0 -g -DSME_TRACE_ENABLE -DSME_TRACE_TYPES_ENABLE -DFSM_DEBUG -DFSM_DEBUG_DUMP
debug: all

bluez_coex: CFLAGSEXTRA= -DBT_SAP_FD_SUPPORT
bluez_coex: all

bluez_coex_debug: CFLAGSEXTRA= -O0 -g -DSME_TRACE_ENABLE -DSME_TRACE_TYPES_ENABLE -DFSM_DEBUG -DFSM_DEBUG_DUMP -DBT_SAP_FD_SUPPORT
bluez_coex_debug: all

lib: libsme.a liblinuxsme.a

bin: unifi_helper

install: unifi_helper
	install -D unifi_helper $(DESTDIR)$(SBINDIR)/unifi_helper


clean:
	rm -f unifi_helper
	rm -f libsme.a
	rm -f $(LIBOBJECTS)
	rm -f liblinuxsme.a
	rm -f $(LIBLINUXOBJECTS)

build_report :
	@echo Compiling UniFi helper app

# --------------------------------------------------------
# Compiler options
# --------------------------------------------------------
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -Werror
CFLAGS += $(SME_INCLUDES)

AR      = $(CROSS_COMPILE)ar
ARFLAGS = crs

# --------------------------------------------------------
# SME Library
# --------------------------------------------------------
LIBOBJECTS := $(patsubst %.c,%.o,$(SME_SRC))
libsme.a : build_report $(LIBOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBOBJECTS)

# --------------------------------------------------------
# Linux SME Library
# --------------------------------------------------------
SME_LINUX_SRC += $(wildcard ../common/sme_trace/stdout/*.c)
SME_LINUX_SRC += $(wildcard ../common/ipc/*.c)
SME_LINUX_SRC += $(wildcard ../common/ipc/linux/*.c)
SME_LINUX_SRC += $(wildcard ../common/saps_impl/bt_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard ../common/saps_impl/dbg_sap/stub/*.c)
SME_LINUX_SRC += $(wildcard ../common/saps_impl/mgt_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard ../common/saps_impl/sys_sap/ipc/*.c)
# FIXME: Add sources for AMP when ready on mainline.
#SME_LINUX_SRC += $(wildcard ../common/saps_impl/pal_acl_sap/*/*.c)
#SME_LINUX_SRC += $(wildcard ../common/saps_impl/pal_hci_sap/*/*.c)
#SME_LINUX_SRC += $(wildcard ../common/saps_impl/pal_udt_sap/*/*.c)
#SME_LINUX_SRC += $(wildcard ../common/saps_impl/pal_ctrl_sap/*/*.c)
LIBLINUXOBJECTS := $(patsubst %.c,%.o,$(SME_LINUX_SRC))
liblinuxsme.a : build_report $(LIBLINUXOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBLINUXOBJECTS)

# --------------------------------------------------------
# Linux Exe's
# --------------------------------------------------------
EXELIBS += libsme.a 
EXELIBS += liblinuxsme.a 
EXELIBS += -lbsd-compat
EXELIBS += -lpthread
EXELIBS += -lrt

# SME Linux exe
unifi_helper : libsme.a liblinuxsme.a
	@echo linking $@
	$(CC) -o $@ $(CFLAGS) main.c bt_sap.c dbg_sap.c mgt_sap.c sys_sap.c -Xlinker --start-group $(EXELIBS) -Xlinker --end-group
