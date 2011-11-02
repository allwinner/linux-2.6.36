# Build the unifi-helper, userspace NON-GPL code
LOCAL_MODULE_TAGS := eng #optional
LOCAL_PATH := $(call my-dir)

LIB_SME_ROOT      = $(LOCAL_PATH)/../lib_sme
LIB_NME_ROOT      = $(LOCAL_PATH)/../lib_nme
LIB_SECURITY_ROOT = $(LOCAL_PATH)/../lib_security
LIB_CRYPTO_ROOT   = $(LOCAL_PATH)/../lib_crypto
LIB_IE_ROOT       = $(LOCAL_PATH)/../lib_info_elements

# --------------------------------------------------------
# Include paths
# --------------------------------------------------------
#SME_INCLUDES += -I$(LIB_SME_ROOT)
SME_INCLUDES += $(LOCAL_PATH)/common
SME_INCLUDES += $(LOCAL_PATH)/../lib_info_elements
SME_INCLUDES += $(LIB_SME_ROOT)/sme
SME_INCLUDES += $(LIB_SME_ROOT)/saps
SME_INCLUDES += $(LIB_SME_ROOT)/common
SME_INCLUDES += $(LOCAL_PATH)/../lib_synergy_framework/linux
SME_INCLUDES += $(LIB_SME_ROOT)/common/abstractions/linux
SME_INCLUDES += $(LIB_SME_ROOT)/ccx

SME_INCLUDES += $(LIB_NME_ROOT)/nme
SME_INCLUDES += $(LIB_NME_ROOT)/saps
SME_INCLUDES += $(LIB_NME_ROOT)/common
SME_INCLUDES += $(LIB_SECURITY_ROOT)
SME_INCLUDES += $(LIB_CRYPTO_ROOT)
SME_INCLUDES += $(LIB_IE_ROOT)

# --------------------------------------------------------
#  SME DEFINES
# --------------------------------------------------------
#SME_CFLAGS += $(CFLAGSEXTRA)
SME_CFLAGS += -DFSM_MUTEX_ENABLE
SME_CFLAGS += -DFSM_TRANSITION_LOCK
SME_CFLAGS += -DSME_SYNC_ACCESS

SME_CFLAGS += -DIPC_CHARDEVICE
SME_CFLAGS += -DREMOTE_SYS_SAP -DSYS_SAP_IPC_CHARDEVICE
SME_CFLAGS += -DREMOTE_BT_SAP -DBT_SAP_IPC_IP -DIPC_IP

# ... Optional debug and trace defines below ... 
#SME_CFLAGS += -DSME_TRACE_ENABLE
#SME_CFLAGS += -DSME_TRACE_TYPES_ENABLE
#SME_CFLAGS += -DFSM_DEBUG
#SME_CFLAGS += -DFSM_DEBUG_DUMP

#SME_CFLAGS += -DSME_TRACE_NO_ENTRY
#SME_CFLAGS += -DSME_TRACE_NO_DEBUG
#SME_CFLAGS += -DSME_TRACE_NO_INFO
#SME_CFLAGS += -DSME_TRACE_NO_WARN
#SME_CFLAGS += -DSME_TRACE_NO_ERROR
#SME_CFLAGS += -DSME_TRACE_NO_HEX

#SME_CFLAGS += -DSME_PBC_NO_ASSERTS
#SME_CFLAGS += -DSME_PBC_NO_REQUIRE
#SME_CFLAGS += -DSME_PBC_NO_VERIFY
#SME_CFLAGS += -DSME_PBC_NO_ENSURE
#SME_CFLAGS += -DSME_PBC_NO_INVARIANT
SME_CFLAGS += -DSME_PBC_NO_ASSERTS

# --------------------------------------------------------
# SME Library
# --------------------------------------------------------
SME_SRC += $(wildcard $(LIB_SME_ROOT)/sme/*/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/bt_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/dbg_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/sys_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/saps/mgt_sap/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/common/*/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/../lib_synergy_framework/linux/*.c)
SME_SRC += $(wildcard $(LIB_SME_ROOT)/../lib_info_elements/*/*.c)

SME_SRC_NOPATH := $(patsubst $(LOCAL_PATH)/%,%,$(SME_SRC))

# --------------------------------------------------------
# Linux SME Library
# --------------------------------------------------------
#SME_LINUX_SRC += common/main.c
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/sme_trace/stdout/*.c)
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/ipc/*.c)
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/ipc/linux/*.c)
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/saps_impl/dbg_sap/stub/*.c)
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/saps_impl/mgt_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/saps_impl/sys_sap/ipc/*.c)
SME_LINUX_SRC += $(wildcard $(LOCAL_PATH)/common/saps_impl/bt_sap/ipc/*.c)

SME_LINUX_SRC_NOPATH := $(patsubst $(LOCAL_PATH)/%,%,$(SME_LINUX_SRC))

SME_LINUX_CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_OFF_IND
SME_LINUX_CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_ON_CFM
SME_LINUX_CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_OFF_CFM
SME_LINUX_CFLAGS += -DCUSTOM_UNIFI_MGT_WIFI_FLIGHTMODE_CFM
SME_LINUX_CFLAGS += -DREMOTE_MGT_SAP -DMGT_SAP_IPC_CHARDEVICE

# --------------------------------------------------------
# unifi_helper exe
# --------------------------------------------------------
# unifi_helper application
include $(CLEAR_VARS)
LOCAL_CFLAGS += $(SME_CFLAGS) $(SME_LINUX_CFLAGS)
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_SRC_FILES := common/main.c
LOCAL_SRC_FILES += $(SME_SRC_NOPATH)
LOCAL_SRC_FILES += $(SME_LINUX_SRC_NOPATH)
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(SME_INCLUDES)
LOCAL_MODULE := unifi_helper
LOCAL_MODULE_TAGS := eng
include $(BUILD_EXECUTABLE)
