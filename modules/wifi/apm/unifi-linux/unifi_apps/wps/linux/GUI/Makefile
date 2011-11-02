##############################################################################
# FILE
#   Makefile
#
# CONFIDENTIAL
#   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
#
# DESCRIPTION
#   This is the Makefile that builds the Wi-Fi Protected Setup GUI
#   application for Linux. 
#
##############################################################################
#
# REVISION
#   $Id: //depot/dot11/v7.0p/host/unifi_apps/wps/linux/GUI/Makefile#1 $
#
##############################################################################

gtk2 :
	gcc -DGTK_ENABLE_BROKEN -Wall -g unifi_wps_setup.c -o unifi_wps_setup `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0` -lgthread-2.0
	
gtk1 :
	gcc -Wall -g unifi_wps_setup.c -o unifi_wps_setup `gtk-config --cflags` `gtk-config --libs` `gtk-config --cflags --libs gthread`
