/*
 * ***************************************************************************
 *
 *  FILE: unifi_putest_lib.c
 *
 *      Sets putest ioctls to the UniFi driver.
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <stddef.h>

#include "csr_types.h"
#include "unifiio.h"


int unifi_putest_init(char *device)
{
    int fd;

    /* Start communication with driver. */
    fd = open(device, O_RDWR);
    if (fd < 0) {
        printf("Failed to open device\n");
    }
    return fd;
}

int unifi_putest_deinit(int fd)
{
    /* Kill communication with driver. */
    close(fd);

    return 0;
}

int unifi_putest_start(int fd)
{
    unsigned char buffer[8];
    int r;

    *((unifi_putest_command_t*)buffer) = UNIFI_PUTEST_START;

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    if (r < 0) {
        printf("UNIFI_PUTEST: Start failed\n");
        return r;
    }

    return 0;
}

int unifi_putest_stop(int fd)
{
    unsigned char buffer[8];
    int r;

    *((unifi_putest_command_t*)buffer) = UNIFI_PUTEST_STOP;

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    if (r < 0) {
        printf("UNIFI_PUTEST: Stop failed\n");
        return r;
    }

    return 0;
}

int unifi_putest_set_sdio_clock(int fd, int clock_khz)
{
    unsigned char buffer[32];
    CsrUint8 *buf_pos;
    int r;

    buf_pos = buffer;
    *((unifi_putest_command_t*)buf_pos) = UNIFI_PUTEST_SET_SDIO_CLOCK;

    buf_pos += sizeof(unifi_putest_command_t);
    *((int*)buf_pos) = clock_khz;

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    if (r < 0) {
        printf("UNIFI_PUTEST: Set Clock failed\n");
        return r;
    }

    return 0;
}

int unifi_putest_cmd52_read(int fd, struct unifi_putest_cmd52 *cmd52_params)
{
    unsigned char buffer[32];
    CsrUint8 *buf_pos;
    int r;

    buf_pos = buffer;
    *((unifi_putest_command_t*)buf_pos) = UNIFI_PUTEST_CMD52_READ;

    buf_pos += sizeof(unifi_putest_command_t);
    *((unsigned int*)buf_pos) = sizeof(struct unifi_putest_cmd52);

    buf_pos += sizeof(unsigned int);
    memcpy(buf_pos, cmd52_params, sizeof(struct unifi_putest_cmd52));

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    if (r < 0) {
        printf("UNIFI_PUTEST: CMD52 Read failed\n");
        return r;
    }

    /* Copy the result back to the buffer */
    memcpy(cmd52_params, buf_pos, sizeof(struct unifi_putest_cmd52));

    return 0;
}

int unifi_putest_cmd52_write(int fd, struct unifi_putest_cmd52 *cmd52_params)
{
    unsigned char buffer[32];
    CsrUint8 *buf_pos;
    int r;

    buf_pos = buffer;
    *((unifi_putest_command_t*)buf_pos) = UNIFI_PUTEST_CMD52_WRITE;

    buf_pos += sizeof(unifi_putest_command_t);
    *((unsigned int*)buf_pos) = sizeof(struct unifi_putest_cmd52);

    buf_pos += sizeof(unsigned int);
    memcpy(buf_pos, cmd52_params, sizeof(struct unifi_putest_cmd52));

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    if (r < 0) {
        printf("UNIFI_PUTEST: CMD52 Write failed\n");
        return r;
    }

    return 0;
}


int unifi_putest_dl_fw(int fd, const char *fw_file_name,
                       const unsigned int fw_file_name_len)
{
    unsigned char buffer[32];
    CsrUint8 *buf_pos;
    int r;
#define UF_PUTEST_MAX_FW_FILE_NAME      16

    if (fw_file_name_len > UF_PUTEST_MAX_FW_FILE_NAME) {
        printf("unifi_putest_dl_fw: f/w file name exceeds UF_PUTEST_MAX_FW_FILE_NAME\n");
        return -EINVAL;
    }

    buf_pos = buffer;
    *((unifi_putest_command_t*)buf_pos) = UNIFI_PUTEST_DL_FW;

    buf_pos += sizeof(unifi_putest_command_t);
    *((unsigned int*)buf_pos) = fw_file_name_len;

    buf_pos += sizeof(unsigned int);
    memcpy(buf_pos, fw_file_name, fw_file_name_len);

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    if (r < 0) {
        printf("UNIFI_PUTEST: UNIFI_PUTEST_DL_FW failed\n");
        return r;
    }

    return 0;
}

int unifi_putest_dl_fw_buff(int fd, const void *fw_buf, const unsigned int buf_len)
{
    void *buffer;
    CsrUint8 *buf_pos;
    int r;

    /* Sanity check for a corrupt buffer length or NULL pointer*/
    if (buf_len == 0 || buf_len > 0xfffffff || !fw_buf) {
        printf("unifi_putest_dl_fw_buff: bad buffer length %u, ptr %p\n", buf_len, fw_buf);
        return -EINVAL;
    }

    /* Allocate f/w buffer copy, plus space for command struct */
    buffer = malloc(buf_len + 32);
    if (!buffer) {
        printf("couldn't allocate buffer\n");        
        return -ENOMEM;
    }
    
    buf_pos = buffer;
    *((unifi_putest_command_t*)buf_pos) = UNIFI_PUTEST_DL_FW_BUFF;

    buf_pos += sizeof(unifi_putest_command_t);
    *((unsigned int*)buf_pos) = buf_len;

    buf_pos += sizeof(unsigned int);
    memcpy(buf_pos, fw_buf, buf_len);

    r = ioctl(fd, UNIFI_PUTEST, buffer);
    free(buffer);
    
    if (r < 0) {
        printf("UNIFI_PUTEST: UNIFI_PUTEST_DL_FW_BUFF failed\n");
        return r;
    }

    return 0;
}

