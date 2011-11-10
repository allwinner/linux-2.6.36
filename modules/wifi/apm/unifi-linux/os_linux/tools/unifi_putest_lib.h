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

#ifndef UNIFI_PUTEST_LIB_H
#define UNIFI_PUTEST_LIB_H


int unifi_putest_init(char *device);
int unifi_putest_deinit(int fd);
int unifi_putest_start(int fd);
int unifi_putest_stop(int fd);
int unifi_putest_set_sdio_clock(int fd, int clock_khz);
int unifi_putest_cmd52_read(int fd, struct unifi_putest_cmd52 *cmd52_params);
int unifi_putest_cmd52_write(int fd, struct unifi_putest_cmd52 *cmd52_params);
int unifi_putest_dl_fw(int fd, const char *fw_file_name, const unsigned int fw_file_name_len);
int unifi_putest_dl_fw_buff(int fd, const void *fw_buf, const unsigned int fw_buf_len);


#endif /* UNIFI_PUTEST_LIB_H */
