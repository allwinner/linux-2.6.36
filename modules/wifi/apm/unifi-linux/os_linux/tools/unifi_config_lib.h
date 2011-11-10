/*
 * ***************************************************************************
 *
 *  FILE: unifi_config_lib.h
 *
 *      Get/Set configuration to the UniFi driver library.
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */

#ifndef UNIFI_CONFIG_LIB_H
#define UNIFI_CONFIG_LIB_H


int unifi_cfg_init(char *device);
int unifi_cfg_deinit(int fd);
int unifi_cfg_set_filters(int fd, unsigned int filter_type);
int unifi_cfg_set_wmm_qos_info(int fd, CsrUint8 wmm_qos_info);
int unifi_cfg_wmm_addts(int fd, CsrUint32 addts_tid,
                        CsrUint8 *addts_ie, CsrUint8 addts_ie_length);
int unifi_cfg_wmm_delts(int fd, CsrUint32 delts_tid);
int unifi_cfg_set_power_save(int fd, unifi_cfg_powersave_t power_mode);
int unifi_cfg_set_power_supply(int fd, unifi_cfg_powersupply_t power_supply);
int unifi_cfg_set_power(int fd, unifi_cfg_power_t power);
int unifi_cfg_get_info(int fd, unifi_cfg_get_t type,
                       unsigned char *out_buffer);
int unifi_cfg_set_strict_draft_n(int fd, CsrBool strict_draft_n);
int unifi_cfg_set_enable_okc(int fd, CsrBool enable_okc);

#define UNIFI_CFG_MAX_ADDTS_IE_LENGTH     63

#endif /* UNIFI_CONFIG_LIB_H */
