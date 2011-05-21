#
# Makefile for SUN3I USB2.0 Dual Role Controller Driver.
#

obj-y += usbc/usbc.o \
         usbc/usbc_dev.o \
         usbc/usbc_host.o \
         usbc/usbc_phy.o \
         misc/sw_usb_debug.o

sw_hcd_core-objs := hcd/core/sw_hcd_core.o \
                    hcd/core/sw_hcd_host.o \
                    hcd/core/sw_hcd_virt_hub.o \
                    hcd/core/sw_hcd_debug.o \
                    hcd/core/sw_hcd_dma.o

sw_usb_udc-objs  := udc/sw_udc.o \
                    udc/sw_udc_board.o \
                    udc/sw_udc_debug.o \
                    udc/sw_udc_dma.o

obj-$(CONFIG_USB_SW_USB_MANAGER) += manager/usb_manager.o

obj-$(CONFIG_USB_SW_SUN3I_HCD0) += hcd/hcd0/sw_hcd0.o sw_hcd_core.o
obj-$(CONFIG_USB_SW_SUN3I_HCD1) += hcd/hcd1/sw_hcd1.o sw_hcd_core.o
obj-$(CONFIG_USB_SW_SUN3I_HCD2) += hcd/hcd2/sw_hcd2.o sw_hcd_core.o

obj-$(CONFIG_USB_SW_SUN3I_UDC0) += sw_usb_udc.o

