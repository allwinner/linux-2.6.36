# --------------------------------------------------------------------
# FILE
#   Makefile - Top level makefile
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2006-2007. All rights reserved.
#
# DESCRIPTION
#   This is the main Makefile that orchestrates the building of the Crypto Library. 
#
# REVISION
#   $Id: //depot/dot11/v7.0p/host/lib_crypto/Makefile#1 $
#
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# Default Build CFG
# --------------------------------------------------------------------
BUILDCFG=LibSecurity

# --------------------------------------------------------------------
# Setup the Top level Paths
# --------------------------------------------------------------------
SOURCE_TREE_ROOT	= $(shell pwd)
SME_ROOT			= $(SOURCE_TREE_ROOT)/../lib_sme
SME_BUILD			= $(SME_ROOT)/build
SECURITY_ROOT       = $(SOURCE_TREE_ROOT)/../lib_security
SYNERGY_FW_ROOT     = $(SOURCE_TREE_ROOT)/../lib_synergy_framework/$(OSA_VARIANT)

# --------------------------------------------------------------------
# Load the configuration files
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.buildcfg.inc
BUILD_TREE_ROOT = $(SOURCE_TREE_ROOT)/../builds/lib_crypto/$(SME_TARGET_NAME)

# --------------------------------------------------------------------
# Set the #defines based on the security protocols required
# --------------------------------------------------------------------
include Makefile.security.inc

# --------------------------------------------------------------------
# Help Target
# --------------------------------------------------------------------
.PHONY: help
help:
	@echo " "
	@echo "================================================"
	@echo "Crypto Library"
	@echo "================================================"
	@echo " Build Config"
	@echo "  CONFIG=<> options: See builds/buildcfg/Makefile.base.*.cfg"
	@echo "  TYPE=<> options: See builds/buildcfg/Makefile.type.*.cfg"
	@echo "  PLATFORM=<> options: See builds/buildcfg/Makefile.platform.*.inc"
	@echo " "
	@echo "Targets: "
	@echo "    clean		- clean up all build output"
	@echo "    build		- Crypto Library"
	@echo "    all			- clean autogen build options"
	@echo "    lint			- lint all files"
	@echo " "

# --------------------------------------------------------------------
# Include Paths
# --------------------------------------------------------------------
CFG_INCLUDES =  -I$(SOURCE_TREE_ROOT) \
				-I$(SECURITY_ROOT) \
				-I$(SME_ROOT)/common \
				-I$(SME_ROOT)/common/abstractions/$(OSA_VARIANT) \
				-I$(SYNERGY_FW_ROOT)

# --------------------------------------------------------------------
# Source Files
# --------------------------------------------------------------------
#SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/*/*.c)

ifneq (,$(findstring -DCSR_CRYPTO_AES_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_aes/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_DES_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_des/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_MD5_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_md5/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_RC4_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_rc4/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_SHA1_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_sha1/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_HMAC_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_hmac/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_SHA256_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_sha256/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_KDHMACSHA256_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_kd_hmac_sha256/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_SMS4_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_sms4/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_ASN1_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_asn1/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_BN_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_bn/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_MD4_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_md4/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_MSCHAP_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_mschap/*.c)
endif
ifneq (,$(findstring -DCSR_CRYPTO_DH_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_dh/*.c)
endif

# Compile EC only when required as it needs a new version of openssl
ifneq (,$(findstring -DCSR_CRYPTO_EC_ENABLE,$(CFG_DEFINES)))
ifneq (,$(findstring -DCSR_CRYPTO_USE_OPENSSL,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_ec/openssl/*.c)
else
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_ec/*.c)
endif
endif

ifneq (,$(findstring -DCSR_CRYPTO_PASSPHRASE_HASHING_ENABLE,$(CFG_DEFINES)))
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_passphrase_hashing/*.c)
endif


# --------------------------------------------------------------------
# Open SSL == BAD Needs replacing
# --------------------------------------------------------------------
ifneq (,$(findstring -DCSR_CRYPTO_USE_OPENSSL,$(CFG_DEFINES)))
CFG_INCLUDES+= -I /usr/include/openssl
endif

ifeq ($(CSR_WIFI_SECURITY_NONBLOCKING_ENABLE), )
SOURCES += $(wildcard $(SOURCE_TREE_ROOT)/csr_crypto_blocking.c)
endif

# --------------------------------------------------------------------
# Include compile and link rules
# --------------------------------------------------------------------
include $(SME_BUILD)/Makefile.rules.inc

# --------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------
.PHONY: all
all:
	+make clean
	+make build

.PHONY: clean
clean:
	-rm -rf $(BUILD_TREE_ROOT)

.PHONY: build

build: $(BUILD_TREE_ROOT)/libcrypto.a
