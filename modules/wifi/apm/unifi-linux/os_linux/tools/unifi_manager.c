/*
 * ***************************************************************************
 *
 *  FILE: unifi_manager.c
 *
 *      Download the loader and firmware to the UniFi.
 *      Set the MIB data files to the UniFi.
 *      Initialise the UniFi firmware (set MAC address, send MLME-RESET.req).
 *      Handle UniFi core driver errors, if SME is not present.
 *
 * Copyright (C) 2005-2008 by Cambridge Silicon Radio Ltd.
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "csr_types.h"
#include "sme_drv.h"
#include "driver/sigs.h"
#include "unifiio.h"

#ifndef isdigit
#define isdigit(_c) (((_c) >= '0') && ((_c) <= '9'))
#endif

#define UNIFI_INIT_COMPLETED    0x02

#define MAX_INIT_RETRIES    3


/*
 * ---------------------------------------------------------------------------
 *  read_byte
 *  is_delim
 *
 *      Support fns for read_mac() below.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
read_byte(const char *p, unsigned char *b)
{
    int n = 0;
    int i, c, v;

    /* Use a loop so we accept single digit fields */
    v = 0;
    for (i = 0; i < 2; i++) {

        c = *p++;

        if ((c >= '0') && (c <= '9')) {
            c -= '0';
        } else if ((c >= 'A') && (c <= 'F')) {
            c = c - 'A' + 10;
        } else if ((c >= 'a') && (c <= 'f')) {
            c = c - 'a' + 10;
        } else {
            break;
        }

        v = (v * 16) + c;
        n++;
    }

    if (n) {
        *b = v;
    }

    return n;
} /* read_byte() */


static int
is_delim(int ch)
{
    return ((ch == ':') || (ch == '.') || (ch == ','));
} /* is_delim() */


/*
 * ---------------------------------------------------------------------------
 *  read_mac
 *
 *      Extract the MAC address from command line argument.
 *      The expected format is something like 00:00:5B:00:23:45, although
 *      '-' and ',' are also accepted as delimiters.
 *
 *  Arguments:
 *      arg             The command-line argument string
 *      macaddr         Pointer to 6-byte array to receive decoded MAC address
 *
 *  Returns:
 *      0 on success, -1 if the string does not conform to expected format.
 * ---------------------------------------------------------------------------
 */
static int
read_mac(const char *arg, unsigned char *macaddr)
{
    const char *p = arg;
    int i, n, j;

    i = 0;

    for (j=0;; j++) {
        n = read_byte(p, &macaddr[j]);
        if (n == 0)
            return -1;
        p += n;

        if (j >= 5) {
            return (*p != '\0') ? -1 : 0;
        }

        if (!is_delim(*p))
            return -1;
        p++;
    }
} /* read_mac() */


int
main(int argc, char *argv[])
{
    char *device = "/dev/unifi0";
    char *initprog = NULL;
    int start = 0;
    int c, err;
    int i, r;
    int sme_is_present;

    char *full_init = "1";

    MANAGER_PRIV_DATA unifi_manager;

    err = 0;
    while ((c=getopt(argc, argv, "d:qs:b:x:")) != -1)
    {
        switch (c) {
          case 's':
            start = 1;
            if (read_mac(optarg, unifi_manager.macaddress)) {
                fprintf(stderr, "Badly formatted MAC address\n");
                err++;
            }
            break;
          case 'd':
            device = optarg;
            break;
          case 'b':
            full_init = optarg;
            break;
          case 'x':
            initprog = optarg;
            break;
          default:
            fprintf(stderr, "Bad option: %c\n", c);
            err++;
        }
    }
    if ((start == 0) && (optind >= argc)) {
        fprintf(stderr, "  No MIB file or MAC address given\n");
        err++;
    }
    if (err) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    unifi_manager [-d dev] [-i chip] [-b init] -s mac mibfile mibfile...\n");
        fprintf(stderr, "        Set the MIB values in the given binary file (usually supplied\n");
        fprintf(stderr, "        with the chip).\n");
        fprintf(stderr, "        Specify '-' to read from stdin.\n");
        fprintf(stderr, "    Options:\n");
        fprintf(stderr, "        -d dev      Use <dev> as the device (default is /dev/unifi0).\n");
        fprintf(stderr, "        -b init     Fully initialise UniFi (0=No, 1=Yes).\n");
        fprintf(stderr, "        -s macaddr  Set the card MAC address and start the network\n");
        fprintf(stderr, "                    interface (after loading any MIBs).\n");
        fprintf(stderr, "        -x initprog If the supplied pathname exists and is a regular file,\n");
        fprintf(stderr, "                    then execute it just after MIB download, with a\n");
        fprintf(stderr, "                    parameter of '-d <dev>', where <dev> is as supplied\n");
        fprintf(stderr, "                    by -d to this program\n");
        exit(1);
    }

    if ((*full_init) == '2') {
        return 0;
    }

    /* Store the mib files in our private structure. */
    unifi_manager.mib_files_num = 0;
    for ( ; optind < argc; optind++) {
        sprintf(unifi_manager.mib_files[unifi_manager.mib_files_num], argv[optind]);
        unifi_manager.mib_files_num ++;
    }

    /* Establish communication with char driver. */
    unifi_manager.Fd = open(device, O_RDWR);
    if (unifi_manager.Fd < 0) {
        perror("Failed to open device");
        exit(errno);
    }

    /* Try to initialise UniFi. */
    sme_is_present = 0;
    r = DD_IND_ERROR;
    while (1) {
        /* If we need to initialize UniFi.. */
        if ((r == DD_IND_ERROR) && (sme_is_present == 0)) {
            /* Try a few times */
            for (i = 0; i < MAX_INIT_RETRIES; i++) {

                /* Step1: Init H/W */
                r = ioctl(unifi_manager.Fd, UNIFI_INIT_HW, NULL);
                if (r < 0) {
                    perror("Step1: returned error.\n");
                    goto error;
                }

                /* If the initialisation is left to the SME, exit now. */
                if ((*full_init) == '0') {
                    goto exit1;
                }

                /* Step2: Download initial MIBs */
                /* Download all files on command line */
                for (i = 0; i < unifi_manager.mib_files_num; i++) {
                    r = drv_download_mib(&unifi_manager, unifi_manager.mib_files[i]);
                    if (r != 0) {
                        perror("Step2: returned error.\n");
                        goto error;
                    }
                }

                /* Step2.5: Run optional command */
                /* It is anticipated that this command will set extra mib values by some
                   means (e.g. omnicli) which need to be set before the first mlme_reset
                   but which also need to be changed regularly. In other words, this is for
                   testing the firmware. */
                if (initprog) {
                    struct stat sbuf;

                    /* Don't complain if the lstat fails; assume this means the file does
                       not exist and skip the rest of it */
                    if (!lstat(initprog,&sbuf)) {
                        char *command;

                        /* However, if it exists, we should complain if things go wrong... */
                        if (!S_ISREG(sbuf.st_mode)) {
                            perror("Step 2.5: initprog not a regular file.\n");
                            goto error;
                        }
                        if (!(command=calloc(1,strlen(initprog)+strlen(device)+10))) {
                            perror("Step 2.5: calloc error\n");
                            goto error;
                        }
                        sprintf(command,"%s -d %s",initprog,device);
                        /* Don't bother with the error from the system command; let it do its own complaining */
                        system(command);
                        free(command);
                    }
                }

                /* Step3: Set MAC address */
                r = drv_set_mac_address(&unifi_manager);
                if (r != 0) {
                    perror("Step3: returned error.\n");
                    goto error;
                }

                /* Step4: Reset unifi */
                r = drv_set_reset_state(&unifi_manager);
                if (r != 0) {
                    perror("Step4: returned error.\n");
                    goto error;
                }

                /* Step5: Wait for the first MLME-CONNECT.ind */
                r = wait_for_event(&unifi_manager, CSR_MLME_CONNECTED_INDICATION_ID);
                if (r != 0) {
                    fprintf(stderr, "Step5: returned error.\n");
                    goto error;
                }

                /* Step6: Register the network device. */
                r = ioctl(unifi_manager.Fd, UNIFI_INIT_NETDEV, unifi_manager.macaddress);
                if (r < 0) {
                    fprintf(stderr, "Step10: returned error.\n");
                    r = ERROR_IO_ERROR;
                    goto error;
                }

error:
                if (r == DD_IND_ERROR) {
                    continue;
                }

                break;
            } /* for() */
        }
        /* UniFi has gone. */
        if ((r == DD_IND_EXIT) || (r == ERROR_IO_ERROR)) {
            break;
        }
        if (r == DD_SME_PRESENT) {
            fprintf(stderr, "SME detected, manager will be idle.\n");
            sme_is_present = 1;
        }
        if (r == DD_SME_NOT_PRESENT) {
            fprintf(stderr, "Manager will handle events again.\n");
            sme_is_present = 0;
        }
        /* Wait for an event from the driver. */
        r = wait_for_event(&unifi_manager, INVALID_SIGNAL);
    } /* while () */

exit1:
    printf("EXITING...\n");
    /* Kill communication with driver. */
    close(unifi_manager.Fd);

    return 0;
} /* main() */

