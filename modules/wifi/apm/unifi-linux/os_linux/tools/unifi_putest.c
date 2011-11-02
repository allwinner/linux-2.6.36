/*
 * ***************************************************************************
 *
 *  FILE: unifi_config.c
 *
 *      Get/Set configuration to the UniFi driver.
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

#include "unifi_putest_lib.h"

#define PUTEST_FW_FILE  "putest_sta.xbv"

static int verbose = 0;

int read16(int fd, unsigned int address, CsrUint16 *value)
{
    struct unifi_putest_cmd52 cmd52_params;
    unsigned char lo = 0xAD, hi = 0xDE;
    int r;

    cmd52_params.funcnum = 1;
    cmd52_params.addr = address;
    cmd52_params.data = lo;
    r = unifi_putest_cmd52_read(fd, &cmd52_params);
    if (r < 0) {
        printf("UNIFI_PUTEST_CMD52_READ: failed\n");
        return r;
    }

    lo = cmd52_params.data;

    cmd52_params.funcnum = 1;
    cmd52_params.addr = address + 1;
    cmd52_params.data = hi;
    r = unifi_putest_cmd52_read(fd, &cmd52_params);
    if (r < 0) {
        printf("UNIFI_PUTEST_CMD52_READ: failed\n");
        return r;
    }

    hi = cmd52_params.data;
    *value = (((CsrUint16)hi) << 8) | lo;

    return 0;
}


int write16(int fd, unsigned int address, CsrUint16 value)
{
    struct unifi_putest_cmd52 cmd52_params;
    int r;

    cmd52_params.funcnum = 1;
    cmd52_params.addr = address + 1;
    cmd52_params.data = (unsigned char)(value >> 8);
    r = unifi_putest_cmd52_write(fd, &cmd52_params);
    if (r < 0) {
        printf("UNIFI_PUTEST_CMD52_WRITE: failed\n");
        return r;
    }

    cmd52_params.funcnum = 1;
    cmd52_params.addr = address;
    cmd52_params.data = (unsigned char)value;
    r = unifi_putest_cmd52_write(fd, &cmd52_params);
    if (r < 0) {
        printf("UNIFI_PUTEST_CMD52_WRITE: failed\n");
        return r;
    }

    return 0;
}


static int Cmd52Test(int fd)
{
    CsrUint16 reply;
    int n, r;

    /* select MAC processor DBG_HOST_PROC_SELECT */
    r = write16(fd, 0x1fd20, 0 );
    if (r) {
        printf("CMD52 Test Failed\n");
        return -1;
    }

    /* set up memory window 0x233 MMU_HOST_GW2_CONFIG (-> shared data) */
    r = write16(fd, 0x1f15a, 0x233); 
    if (r) {
        printf("CMD52 Test Failed\n");
        return -1;
    }

    for ( n = 0; n < 10; n++ )
    {
        CsrUint16 data = (0x06AD << n) + n;
        r = write16(fd, 0x4000 + 0x0fb0, data); /* in GW2 */
        if (r) {
            printf("CMD52 Test Failed\n");
            return -1;
        }

        r = read16(fd, 0x4000 + 0x0fb0, &reply);
        if (r) {
            printf("CMD52 Test Failed\n");
            return -1;
        }

        if (data != reply)
        {
            printf("Fail at %d, expected: %04x observed: %04x\n", n, data, reply);
            return -1;
        }
    }

    printf("CMD52 Test Succeed\n");
    return 0;
}


int
main(int argc, char **argv)
{
    char device[] = "/dev/unifiudi0";
    int r;
    int fd;
    int clock_khz;
    int i;
    int iters = 1;
    void *fw_buff_ptr = NULL;
    unsigned int fw_buff_len = 0;
    
    if (argc > 1) {
        verbose = 1; /* Print progress, and do more thorough tests */
        iters = 3;   /* Test that we can do multiple STARTs in succession */
    }
    
    if (verbose) {
        printf("unifi_putest started.\n");
    }

    /* Load the f/w buffer in advance for testing the dl_fw_buff ioctl */
    if (verbose) {
        FILE *fd = fopen(PUTEST_FW_FILE, "r");
        if (!fd) {
            printf("Can't open f/w file %s in pwd\n", PUTEST_FW_FILE);
        } else {
            /* Determine required buffer size */
            fseek(fd, 0, SEEK_END);
            fw_buff_len = (unsigned int)ftell(fd);
            fseek(fd, 0, SEEK_SET);
            printf("Length of f/w file is %u bytes\n", fw_buff_len);

            fw_buff_ptr = malloc(fw_buff_len);
            if (!fw_buff_ptr) {
                printf("Failed to allocate f/w buffer\n");
            } else {
                if (!fread(fw_buff_ptr, fw_buff_len, 1, fd)) {
                    printf("Failed to read f/w buffer\n");
                    fw_buff_len = 0;
                } else {
                    printf("Read f/w buffer\n");
                }
            }
            fclose(fd);
        }
    }
    
    if (verbose) {
        printf("unifi_putest_init...\n");
    }    
    fd = unifi_putest_init(device);
    if (fd < 0) {
        perror("Failed to open device");
        exit(errno);
    }

    for (i = 0; i < iters; i++) {    
       if (verbose) {
          printf("unifi_putest_start %d...\n", i);
       }
       r = unifi_putest_start(fd);
       if (r < 0) {
          printf("UNIFI_PUTEST_START: failed\n");
          goto EXIT;
       }
       sleep(1);
    }

    if (verbose) {
        printf("unifi_putest_set_sdio_clock...\n");
    }
    clock_khz = 25000;
    r = unifi_putest_set_sdio_clock(fd, clock_khz);
    if (r < 0) {
        printf("UNIFI_PUTEST_SDIO_CLOCK: failed\n");
        goto EXIT;
    }
    sleep(1);
    
    if (verbose) {
        printf("Cmd52Test...\n");
    }
    r = Cmd52Test(fd);
    if (r < 0) {
        goto EXIT;
    }

    if (verbose) {
        printf("unifi_putest_dl_fw %s...\n", PUTEST_FW_FILE);
    }
    r = unifi_putest_dl_fw(fd, PUTEST_FW_FILE, strlen(PUTEST_FW_FILE));
    if (r < 0) {
        printf("UNIFI_PUTEST_DL_FW: failed\n");
        goto EXIT;
    }

    /* At this point ptest f/w should be running */
    
    if (verbose) {
        /* Wait for a keypress to provide a chance to connect xIDE, etc */
        printf("\nHit <return>....\n");
        getchar();
        
        /* The ptest host s/w expects to be able to do multiple (START, DL)s */
        printf("unifi_putest_start (2)...\n");
        r = unifi_putest_start(fd);
        if (r < 0) {
            printf("UNIFI_PUTEST_START: failed\n");
            goto EXIT;
        }

        /* Test the download-as-buffer ioctl */
        if (fw_buff_len) {
            printf("unifi_putest_dl_fw_buff %s...\n", PUTEST_FW_FILE);
            r = unifi_putest_dl_fw_buff(fd, fw_buff_ptr, fw_buff_len);
            if (r < 0) {
               printf("UNIFI_PUTEST_DL_FW_BUFF: failed\n");
               goto EXIT;
            }
        }
        
        printf("unifi_putest_dl_fw (2)...\n");
        r = unifi_putest_dl_fw(fd, PUTEST_FW_FILE, strlen(PUTEST_FW_FILE));
        if (r < 0) {
            printf("UNIFI_PUTEST_DL_FW: failed\n");
            goto EXIT;
        }

        /* The ptest host s/w expects to be able to do multiple DLs without
         * STARTs - it may download its own EEPROM programming f/w in addition
         * to the ptest.xbv
         */
        printf("unifi_putest_dl_fw (3)...\n");
        r = unifi_putest_dl_fw(fd, PUTEST_FW_FILE, strlen(PUTEST_FW_FILE));
        if (r < 0) {
            printf("UNIFI_PUTEST_DL_FW: failed\n");
            goto EXIT;
        }

    }
    if (verbose) {
        printf("Cmd52Test...\n");
    }
    r = Cmd52Test(fd);
    if (r < 0) {
        goto EXIT;
    }

EXIT:
    if (verbose) {
        printf("unifi_putest_stop...\n");
    }
    r = unifi_putest_stop(fd);
    if (r < 0) {
        printf("UNIFI_PUTEST_STOP: failed\n");
    }

    /* Kill communication with driver. */
    if (verbose) {
        printf("unifi_putest_deinit...\n");
    }
    unifi_putest_deinit(fd);

    if (fw_buff_ptr) {
        free(fw_buff_ptr);
    }
    if (verbose) {
        printf("exit.\n");
    }
    return r;
} /* main() */

