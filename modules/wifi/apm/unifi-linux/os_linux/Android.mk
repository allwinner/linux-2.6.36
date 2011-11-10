# Copy accross the driver, tools and firmware
LOCAL_MODULE_TAGS := eng
LOCAL_PATH := $(call my-dir)

## Driver module - built beforehand - strip it first
#ALL_PREBUILT += $(TARGET_OUT)/lib/modules/unifi_sdio.ko
#$(TARGET_OUT)/lib/modules/unifi_sdio.ko : $(LOCAL_PATH)/driver/unifi_sdio_stripped.ko | $(ACP)
#	$(transform-prebuilt-to-target)
#
#$(LOCAL_PATH)/driver/unifi_sdio_stripped.ko : $(LOCAL_PATH)/driver/unifi_sdio.ko
#	$(CROSS_COMPILE)strip --strip-unneeded $(dir $@)/unifi_sdio.ko -o $@
#
#$(LOCAL_PATH)/driver/unifi_sdio.ko :
#	$(dir $@)/build android-arm

# Tools, including udev-helper script
ALL_PREBUILT += $(TARGET_OUT)/bin/hotplug
$(TARGET_OUT)/bin/hotplug : $(LOCAL_PATH)/tools/android/hotplug | $(ACP)
	$(transform-prebuilt-to-target)

ALL_PREBUILT += $(TARGET_OUT)/bin/unififw
$(TARGET_OUT)/bin/unififw : $(LOCAL_PATH)/tools/android/unififw | $(ACP)
	$(transform-prebuilt-to-target)

ifneq ($(UNIFI_BUILD_HELPER),true)
# unifi_helper - prebuilt
ALL_PREBUILT += $(TARGET_OUT)/bin/unifi_helper
$(TARGET_OUT)/bin/unifi_helper : $(LOCAL_PATH)/tools/android/unifi_helper | $(ACP)
	$(transform-prebuilt-to-target)
endif


# unifi_config application
include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_SRC_FILES := tools/unifi_config_lib.c \
                   tools/unifi_config.c \
                   tools/android/getsubopt.c
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
                    $(LOCAL_PATH)/../common \
                    $(LOCAL_PATH)/driver \
                    $(LOCAL_PATH)/../lib_sme/common \
                    $(LOCAL_PATH)/../lib_synergy_framework/linux
LOCAL_MODULE := unifi_config
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

# unifi_manager application
#include $(CLEAR_VARS)
#LOCAL_SHARED_LIBRARIES := libc libcutils
#LOCAL_SRC_FILES := tools/unifi_manager.c \
                   tools/mibquery.c \
                   tools/sme_drv.c
#LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
                    $(LOCAL_PATH)/../common \
                    $(LOCAL_PATH)/driver \
                    $(LOCAL_PATH)/../lib_sme/common \
                    $(LOCAL_PATH)/../lib_synergy_framework/linux
#LOCAL_MODULE := unifi_manager
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_EXECUTABLE)

# Firmware - originals go in /etc/firmware/unifi-sdio-0
UNIFI_FW_PATH := unifi-sdio-0
#ALL_PREBUILT += $(TARGET_OUT)/etc/firmware/$(UNIFI_FW_PATH)/loader.xbv
#$(TARGET_OUT)/etc/firmware/$(UNIFI_FW_PATH)/loader.xbv : $(LOCAL_PATH)/../firmware/loader.xbv | $(ACP)
#	$(transform-prebuilt-to-target)

ALL_PREBUILT += $(TARGET_OUT)/etc/firmware/$(UNIFI_FW_PATH)/sta.xbv
$(TARGET_OUT)/etc/firmware/$(UNIFI_FW_PATH)/sta.xbv : $(LOCAL_PATH)/../firmware/sta.xbv | $(ACP)
	$(transform-prebuilt-to-target)

# Create a symlink for unifi plugged into second sdio slot
#/etc/firmware/unifi-sdio-1 -> /etc/firmware/unifi-sdio-0
SYMLINKS := $(addprefix $(TARGET_OUT),/etc/firmware/unifi-sdio-1)
$(SYMLINKS): $(TARGET_OUT)/etc/firmware/$(UNIFI_FW_PATH)/sta.xbv
	@echo "Symlink: $@ -> $(UNIFI_FW_PATH)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(UNIFI_FW_PATH) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)
