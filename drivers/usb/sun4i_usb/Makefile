#
# Makefile for SUN3I USB2.0 Dual Role Controller Driver.
#

sw_usbc-objs += usbc/usbc.o \
                usbc/usbc_dev.o \
                usbc/usbc_host.o \
                usbc/usbc_phy.o \
                misc/sw_usb_debug.o

sw_hcd_core-objs := hcd/core/sw_hcd_core.o \
                    hcd/core/sw_hcd_host.o \
                    hcd/core/sw_hcd_virt_hub.o \
                    hcd/core/sw_hcd_debug.o \
                    hcd/core/sw_hcd_dma.o

sw_usb_hcd0-objs := hcd/hcd0/sw_hcd0.o sw_hcd_core.o

sw_usb_udc-objs  := udc/sw_udc.o \
                    udc/sw_udc_board.o \
                    udc/sw_udc_debug.o \
                    udc/sw_udc_dma.o



obj-$(CONFIG_USB_SW_SUN4I_USB_MANAGER) += sw_usbc.o manager/usb_manager.o
obj-$(CONFIG_USB_SW_SUN4I_HCD0) += sw_usb_hcd0.o
obj-$(CONFIG_USB_SW_SUN4I_UDC0) += sw_usb_udc.o

