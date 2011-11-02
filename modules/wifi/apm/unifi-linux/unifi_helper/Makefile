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
# REVISION
#   $Id: //depot/dot11/v7.0p/host/os_linux/packaging/helper_Makefile#1 $
#
##############################################################################

SBINDIR = /usr/sbin
#CROSS_COMPILE := ~/prebuilt/linux-x86/toolchain/arm-eabi-4.3.1/bin/arm-eabi-
CROSS_COMPILE:=arm-none-linux-gnueabi-
LIB_SME_ROOT      = ../lib_sme
LIB_NME_ROOT      = ../lib_nme
LIB_SECURITY_ROOT = ../lib_security
LIB_CRYPTO_ROOT   = ../lib_crypto
LIB_IE_ROOT       = ../lib_info_elements
LIB_PALDATA_ROOT  = ../lib_paldata


# --------------------------------------------------------
# Include paths
# --------------------------------------------------------
#INCLUDES += -I$(LIB_SME_ROOT)
INCLUDES += -Icommon
INCLUDES += -I../lib_info_elements
INCLUDES += -I$(LIB_SME_ROOT)/sme
INCLUDES += -I$(LIB_SME_ROOT)/saps
INCLUDES += -I$(LIB_SME_ROOT)/common
INCLUDES += -I../lib_synergy_framework/linux
INCLUDES += -I$(LIB_SME_ROOT)/ccx

INCLUDES += -I$(LIB_NME_ROOT)/nme
INCLUDES += -I$(LIB_NME_ROOT)/saps
INCLUDES += -I$(LIB_NME_ROOT)/common
INCLUDES += -I$(LIB_SECURITY_ROOT)
INCLUDES += -I$(LIB_CRYPTO_ROOT)
INCLUDES += -I$(LIB_IE_ROOT)


ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
# --------------------------------------------------------
# PAL Include paths
# --------------------------------------------------------
INCLUDES += -I$(LIB_SME_ROOT)/pal

# --------------------------------------------------------
# PAL Data Include paths
# --------------------------------------------------------
INCLUDES += -I$(LIB_PALDATA_ROOT)/paldata
INCLUDES += -I$(LIB_PALDATA_ROOT)/saps
INCLUDES += -I$(LIB_PALDATA_ROOT)/common
endif


# --------------------------------------------------------
#  SME DEFINES
# --------------------------------------------------------
CFLAGS += $(CFLAGSEXTRA)
CFLAGS += -DFSM_MUTEX_ENABLE
CFLAGS += -DFSM_TRANSITION_LOCK
CFLAGS += -DSME_SYNC_ACCESS

CFLAGS += -DIPC_CHARDEVICE
CFLAGS += -DREMOTE_SYS_SAP -DSYS_SAP_IPC_CHARDEVICE
CFLAGS += -DREMOTE_DBG_SAP -DDBG_SAP_IPC_CHARDEVICE
CFLAGS += -DREMOTE_BT_SAP -DBT_SAP_IPC_IP -DIPC_IP
CFLAGS += --static

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


# Security Defines
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
CFLAGS += -DCSR_WIFI_SECURITY_WPA_PERSONAL_SUPPLICANT_ENABLE
CFLAGS += -DCSR_WIFI_SECURITY_WPA_PERSONAL_AUTHENTICATOR_ENABLE
CFLAGS += -DCSR_CRYPTO_PASSPHRASE_HASHING_ENABLE
CFLAGS += -DCSR_WIFI_SECURITY_TLS_ENABLE
CFLAGS += -DCSR_WIFI_SECURITY_LEAP_ENABLE
CFLAGS += -DCSR_WIFI_SECURITY_FAST_ENABLE
CFLAGS += -DCSR_WIFI_SECURITY_WPS_ENABLE

CFLAGS += -DCSR_CRYPTO_HMAC_ENABLE
CFLAGS += -DCSR_CRYPTO_SHA1_ENABLE
CFLAGS += -DCSR_CRYPTO_PASSPHRASE_HASHING_ENABLE

CFLAGS += -DCSR_WIFI_SECURITY_EAP_ENABLE
CFLAGS += -DCSR_CRYPTO_AES_ENABLE
CFLAGS += -DCSR_CRYPTO_ASN1_ENABLE
CFLAGS += -DCSR_CRYPTO_BN_ENABLE
CFLAGS += -DCSR_CRYPTO_DES_ENABLE
CFLAGS += -DCSR_CRYPTO_DH_ENABLE
CFLAGS += -DCSR_CRYPTO_HMAC_ENABLE
CFLAGS += -DCSR_CRYPTO_MD4_ENABLE
CFLAGS += -DCSR_CRYPTO_MD5_ENABLE
CFLAGS += -DCSR_CRYPTO_MSCHAP_ENABLE
CFLAGS += -DCSR_CRYPTO_RC4_ENABLE
CFLAGS += -DCSR_CRYPTO_SHA1_ENABLE
CFLAGS += -DCSR_CRYPTO_SHA256_ENABLE
CFLAGS += -DCSR_CRYPTO_TLS_ENABLE

CFLAGS += -DREMOTE_PAL_HCI_SAP
CFLAGS += -DREMOTE_PALDATA_ACL_SAP
CFLAGS += -DREMOTE_PALDATA_SYS_SAP
CFLAGS += -DCUSTOM_PALDATA_SYS_MA_UNITDATA_REQ
CFLAGS += -DCUSTOM_PALDATA_SYS_HIP_REQ
CFLAGS += -DCUSTOM_PAL_ACL_DATA_IND
CFLAGS += -DPAL_RAW_ACL_DATA_ENABLED -DCUSTOM_UNIFI_SYS_MA_UNITDATA_REQ
CFLAGS += -DCSR_AMP_ENABLE

ifeq (exists, $(shell [ -e $(LIB_SECURITY_ROOT)/csr_wapi ] && echo exists))
CFLAGS += -DCSR_CRYPTO_EC_ENABLE
CFLAGS += -DCSR_CRYPTO_KDHMACSHA256_ENABLE
CFLAGS += -DCSR_CRYPTO_SMS4_ENABLE
CFLAGS += -DCSR_WIFI_SECURITY_WAPI_ENABLE
endif
endif

# Release compiler options Overridden when debug target is used
CFLAGSEXTRA=-O2 -fno-strict-aliasing

# --------------------------------------------------------
# Targets
# --------------------------------------------------------

# SME (release and debug) with wext support
wext: unifi_helper

installdebugwext: CFLAGSEXTRA= -O0 -g -DSME_TRACE_ENABLE -DSME_TRACE_TYPES_ENABLE -DFSM_DEBUG -DFSM_DEBUG_DUMP
installdebugwext: installwext

# NME (release and debug) with wext support
nme: unifi_helper_nme

installdebugnme: CFLAGSEXTRA= -O0 -g -DSME_TRACE_ENABLE -DSME_TRACE_TYPES_ENABLE -DFSM_DEBUG -DFSM_DEBUG_DUMP
installdebugnme: installnme

installwext: unifi_helper
	install -D unifi_helper $(DESTDIR)$(SBINDIR)/unifi_helper

installnme: unifi_helper_nme
	install -D unifi_helper_nme $(DESTDIR)$(SBINDIR)/unifi_helper

clean:
	rm -f unifi_helper
	rm -f lib*.a
	rm -f $(LIBOBJECTS)
	rm -f $(LIBNMEOBJECTS)
	rm -f $(LIBSECURITYOBJECTS)
	rm -f $(LIBCRYPTOOBJECTS)
	rm -f $(LIBNMELINUXOBJECTS)
	rm -f $(LIBSMELINUXOBJECTS)
	rm -f $(LIBPALDATAOBJECTS)

# --------------------------------------------------------
# Compiler options
# --------------------------------------------------------
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -Werror
CFLAGS += $(INCLUDES)

AR      = $(CROSS_COMPILE)ar
ARFLAGS = crs

# --------------------------------------------------------
# SME Library
# --------------------------------------------------------
SME_SRC += $(wildcard $(LIB_SME_ROOT)/sme/*/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/bt_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/dbg_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/sys_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/mgt_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/ap_utils/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/csr_cstl/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/event_pack_unpack/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/fsm/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/ie_access/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/ie_message_handling/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/payload_manager/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/smeio/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/sme_trace/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/version/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/../lib_synergy_framework/linux/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/../lib_info_elements/*/*.c)
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
# --------------------------------------------------------
# PAL Control Source Files
# --------------------------------------------------------
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/palio/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/pal/*/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/pal_ctrl_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/pal_hci_sap/*.c)
endif

LIBOBJECTS := $(patsubst %.c,%.o,$(SME_SRC))
libsme.a : $(LIBOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBOBJECTS)

# --------------------------------------------------------
# NME Library
# --------------------------------------------------------
NME_SRC += $(wildcard $(LIB_NME_ROOT)/nme/*/*.c)
NME_SRC += $(wildcard $(LIB_NME_ROOT)/saps/*/*.c)
NME_SRC += $(wildcard $(LIB_NME_ROOT)/common/*/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/ap_utils/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/csr_cstl/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/event_pack_unpack/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/fsm/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/ie_access/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/ie_message_handling/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/payload_manager/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/smeio/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/sme_trace/*.c)
NME_SRC += $(wildcard $(LIB_SME_ROOT)/common/version/*.c)
LIBNMEOBJECTS := $(patsubst %.c,%.o,$(NME_SRC))
libnme.a : CFLAGS += -DCSR_WIFI_NME_ENABLE
libnme.a : $(LIBNMEOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBNMEOBJECTS)

# --------------------------------------------------------
# Security Library
# --------------------------------------------------------
SECURITY_SRC += $(wildcard $(LIB_SECURITY_ROOT)/*.c)
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
SECURITY_SRC += $(wildcard $(LIB_SECURITY_ROOT)/*/*.c)
SECURITY_SRC += $(wildcard $(LIB_SECURITY_ROOT)/plugins/*/*.c)
endif
LIBSECURITYOBJECTS := $(patsubst %.c,%.o,$(SECURITY_SRC))
libsecurity.a : $(LIBSECURITYOBJECTS)
	@echo Archiving $@ $(LIBSECURITYOBJECTS)
	$(AR) $(ARFLAGS) $@ $(LIBSECURITYOBJECTS)

# --------------------------------------------------------
# Crypto Library
# --------------------------------------------------------
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
CRYPTO_SRC += $(LIB_CRYPTO_ROOT)/csr_crypto_blocking.c
CRYPTO_SRC += $(wildcard $(LIB_CRYPTO_ROOT)/*/*.c)
LIBCRYPTOOBJECTS := $(patsubst %.c,%.o,$(CRYPTO_SRC))
libcrypto.a : $(LIBCRYPTOOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBCRYPTOOBJECTS)
else
libcrypto.a:
endif

# --------------------------------------------------------
# PALDATA Library
# --------------------------------------------------------
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
PALDATA_SRC += $(wildcard $(LIB_PALDATA_ROOT)/paldata/*/*.c)
PALDATA_SRC += $(wildcard $(LIB_PALDATA_ROOT)/saps/*/*.c)
PALDATA_SRC += $(wildcard $(LIB_PALDATA_ROOT)/common/*/*.c)
LIBPALDATAOBJECTS := $(patsubst %.c,%.o,$(PALDATA_SRC))
libpaldata.a : $(LIBPALDATAOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBPALDATAOBJECTS)
else
libpaldata.a:
endif

# --------------------------------------------------------
# Linux SME Library
# --------------------------------------------------------
SME_LINUX_SRC += common/main.c
SME_LINUX_SRC += $(wildcard common/sme_trace/stdout/*.c)
SME_LINUX_SRC += $(wildcard common/ipc/*.c)
SME_LINUX_SRC += $(wildcard common/ipc/linux/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/dbg_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/mgt_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/sys_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/bt_sap/ipc/*.c)
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
# --------------------------------------------------------
# PAL Source Files
# --------------------------------------------------------
SME_LINUX_SRC += $(wildcard common/saps_impl/paldata_acl_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/paldata_sys_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/pal_ctrl_sap/linuxfunctioncall/*.c)
SME_LINUX_SRC += $(wildcard common/saps_impl/pal_hci_sap/ipc/*.c)
endif
LIBSMELINUXOBJECTS := $(patsubst %.c,%.o,$(SME_LINUX_SRC))
liblinuxsme.a : CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_OFF_IND
liblinuxsme.a : CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_ON_CFM
liblinuxsme.a : CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_OFF_CFM
liblinuxsme.a : CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_FLIGHTMODE_CFM
liblinuxsme.a : CFLAGS += -DREMOTE_MGT_SAP -DMGT_SAP_IPC_CHARDEVICE
liblinuxsme.a : $(LIBSMELINUXOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBSMELINUXOBJECTS)

# --------------------------------------------------------
# Linux NME Library
# --------------------------------------------------------
NME_LINUX_SRC += common/main.c
NME_LINUX_SRC += $(wildcard common/sme_trace/stdout/*.c)
NME_LINUX_SRC += $(wildcard common/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/ipc/linux/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/csr_wifi_nme_sap/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/csr_wifi_nme_sys_sap/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/dbg_sap/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/mgt_sap/nme_linux_function_call/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/sys_sap/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/bt_sap/ipc/*.c)
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
# --------------------------------------------------------
# PAL Source Files
# --------------------------------------------------------
NME_LINUX_SRC += $(wildcard common/saps_impl/paldata_acl_sap/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/paldata_sys_sap/ipc/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/pal_ctrl_sap/linuxfunctioncall/*.c)
NME_LINUX_SRC += $(wildcard common/saps_impl/pal_hci_sap/ipc/*.c)
endif
LIBNMELINUXOBJECTS := $(patsubst %.c,%.o,$(NME_LINUX_SRC))
liblinuxnme.a : CFLAGS += -DCSR_WIFI_NME_ENABLE
liblinuxnme.a : CFLAGS += -DREMOTE_CSR_WIFI_NME_SAP
liblinuxnme.a : CFLAGS += -DREMOTE_CSR_WIFI_NME_SYS_SAP
liblinuxnme.a : $(LIBNMELINUXOBJECTS)
	@echo Archiving $@
	$(AR) $(ARFLAGS) $@ $(LIBNMELINUXOBJECTS)

# --------------------------------------------------------
# unifi_helper exe
# --------------------------------------------------------
UNIFI_HELPER_EXELIBS += libsme.a 
UNIFI_HELPER_EXELIBS += liblinuxsme.a 
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
UNIFI_HELPER_EXELIBS += libsecurity.a
UNIFI_HELPER_EXELIBS += libcrypto.a
UNIFI_HELPER_EXELIBS += libpaldata.a
endif 
UNIFI_HELPER_EXELIBS += -lbsd-compat
UNIFI_HELPER_EXELIBS += -lpthread
UNIFI_HELPER_EXELIBS += -lrt

# SME Linux exe
unifi_helper : libsme.a liblinuxsme.a libsecurity.a libcrypto.a libpaldata.a
	@echo linking $@
	$(CC) -o $@ $(CFLAGS) -Xlinker --start-group $(UNIFI_HELPER_EXELIBS) -Xlinker --end-group

# --------------------------------------------------------
# unifi_helper_nme
# --------------------------------------------------------
UNIFI_HELPER_NME_EXELIBS += liblinuxnme.a 
UNIFI_HELPER_NME_EXELIBS += libsme.a 
UNIFI_HELPER_NME_EXELIBS += libnme.a 
UNIFI_HELPER_NME_EXELIBS += libsecurity.a 
ifeq (exists, $(shell [ -e $(LIB_CRYPTO_ROOT) ] && echo exists))
UNIFI_HELPER_NME_EXELIBS += libcrypto.a
UNIFI_HELPER_NME_EXELIBS += libpaldata.a
endif 
ifeq (exists, $(shell [ -e $(LIB_SECURITY_ROOT)/csr_wapi ] && echo exists))
UNIFI_HELPER_NME_EXELIBS += -lopenssl
endif 
UNIFI_HELPER_NME_EXELIBS += -lbsd-compat
UNIFI_HELPER_NME_EXELIBS += -lpthread
UNIFI_HELPER_NME_EXELIBS += -lrt

unifi_helper_nme : libsme.a libnme.a libsecurity.a libcrypto.a liblinuxnme.a libpaldata.a
	@echo linking $@
	@echo $(CC) -o $@ $(CFLAGS) -Xlinker --start-group $(UNIFI_HELPER_NME_EXELIBS) -Xlinker --end-group
	$(CC) -o $@ $(CFLAGS) -Xlinker --start-group $(UNIFI_HELPER_NME_EXELIBS) -Xlinker --end-group
