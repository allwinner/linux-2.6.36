##############################################################################
# FILE
#   Makefile
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
#
# DESCRIPTION
#   This is the Makefile that builds the Wi-Fi Protected Setup command
#   line application for Linux. 
#
##############################################################################
#
# REVISION
#   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/Makefile#3 $
#
##############################################################################

# --------------------------------------------------------------------
# Setup the Top level Paths
# --------------------------------------------------------------------
SOURCE_TREE_ROOT = $(shell pwd)/../../..

# --------------------------------------------------------------------
# SOURCE ROOT PATHS
# --------------------------------------------------------------------
CRYPTO_ROOT = $(SOURCE_TREE_ROOT)/lib_crypto

WPS_SRCS += wps_main.c
WPS_SRCS += wps_encode.c
WPS_SRCS += wps_decode.c
WPS_SRCS += wps_crypto.c
WPS_SRCS += wps_common.c
WPS_SRCS += wps_supplicant.c
WPS_SRCS += wps_debug.c
WPS_SRCS += wps_scan.c
WPS_SRCS += wps_event.c
WPS_SRCS += wps_eap.c
WPS_SRCS += wps_unifi.c
WPS_SRCS += wps_random.c

WPS_INCS += wps_types.h
WPS_INCS += wps_encode.h
WPS_INCS += wps_decode.h
WPS_INCS += wps_crypto.h
WPS_INCS += wps_common.h
WPS_INCS += wps_supplicant.h
WPS_INCS += wps_debug.h
WPS_INCS += wps_scan.h
WPS_INCS += wps_event.h
WPS_INCS += wps_eap.h
WPS_INCS += wps_unifi.h

SHA256_CRYPTO_FLAGS += -DCSR_CRYPTO_SHA256_ENABLE
SHA256_CRYPTO_FLAGS += -DCSR_CRYPTO_HMAC_ENABLE
SHA256_CRYPTO_FLAGS += -I$(CRYPTO_ROOT)
SHA256_CRYPTO_FLAGS += -I$(SOURCE_TREE_ROOT)/lib_synergy_framework/linux/
SHA256_CRYPTO_FLAGS += -I$(SOURCE_TREE_ROOT)/lib_sme/common

SHA256_CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_crypto_blocking.c
SHA256_CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_hmac/csr_hmac.c
SHA256_CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_sha256/csr_sha256.c

CRYPTO_FLAGS += $(SHA256_CRYPTO_FLAGS)
CRYPTO_FLAGS += -DCSR_CRYPTO_AES_ENABLE
CRYPTO_FLAGS += -DCSR_CRYPTO_BN_ENABLE

CRYPTO_SRCS += $(SHA256_CRYPTO_SRCS)
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_aes/csr_aes128_wrap.c
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_aes/csr_aes128_cbc.c
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_aes/csr_aes128.c
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_bn/csr_bn_mod_exp.c
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_bn/csr_bn_mont.c
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_bn/csr_bn_mod.c
CRYPTO_SRCS += $(CRYPTO_ROOT)/csr_bn/csr_bn_base.c

default : standalone

standalone : $(WPS_SRCS) $(WPS_INCS) $(CRYPTO_SRCS)
	gcc -g -o unifi_wps $(WPS_SRCS) $(CRYPTO_FLAGS) $(CRYPTO_SRCS) -DWPS_64BIT -DWPS_KEYLEN=128

openssl : $(WPS_SRCS) $(WPS_INCS)
	gcc -g -o unifi_wps $(WPS_SRCS) -DWPS_OPENSSL -DWPS_OPENSSL_SHA256 -lcrypto

openssl_nosha256 : $(WPS_SRCS) $(WPS_INCS) $(SHA256_CRYPTO_SRCS)
	gcc -g -o unifi_wps $(WPS_SRCS) $(SHA256_CRYPTO_FLAGS) $(SHA256_CRYPTO_SRCS) -DWPS_OPENSSL -lcrypto
			