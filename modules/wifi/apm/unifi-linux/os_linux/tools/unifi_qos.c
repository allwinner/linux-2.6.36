/*
 * ***************************************************************************
 *
 *  FILE: unifi_qos.c
 *
 *      Configure WMM-PS and PERIODIC TRAFFIC parameters.
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
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "unifiio.h"


#if 0
#define WMMPS_UNKNOWN   0xFFF0
#define WMMPS_OFF       0x0000
#define WMMPS_AC_BK     0x0001
#define WMMPS_AC_BE     0x0002
#define WMMPS_AC_VI     0x0004
#define WMMPS_AC_VO     0x0008
#endif

#define COEX_CONFIG_VALUE_UNKNOWN         0xFFFF


int
main(int argc, char **argv)
{
    char *device = "/dev/unifiudi0";
    int start = 0;
    int err, i;
    int r = 0;
    int fd = -1;
    unifi_CoexConfig *coex_config;
    unsigned char *ioctl_req_buffer;

//    unsigned int wmmps = WMMPS_UNKNOWN;
    unsigned int coex               = COEX_CONFIG_VALUE_UNKNOWN;
    unsigned int afh_channel        = COEX_CONFIG_VALUE_UNKNOWN;
    unsigned int advanced_coex      = COEX_CONFIG_VALUE_UNKNOWN;
    unsigned int scheme_mgt         = COEX_CONFIG_VALUE_UNKNOWN;
    unsigned int periodic_direction = COEX_CONFIG_VALUE_UNKNOWN;
    unsigned int periodic_wake      = COEX_CONFIG_VALUE_UNKNOWN;
    unsigned int periodic_bursty_latency_ms = 0;
    unsigned int periodic_continuous_latency_ms = 0;

    err = 0;
    if (argc > 1) {
        if (!strcasecmp(argv[1], "--help")) {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, " unifi_qos dev /dev/unifiN \n");
//            fprintf(stderr, "           [ wmmps {off|ac_bk|ac_be|ac_vi|ac_vo} ] \n");
            fprintf(stderr, "           [coex off|on] \n");
            fprintf(stderr, "           [afh_channel off|on] \n");
            fprintf(stderr, "           [advanced off|on] \n");
            fprintf(stderr, "           [scheme_mgt off|on] \n");
            fprintf(stderr, "           [direction {input|output}] \n");
            fprintf(stderr, "           [wake {off|on}] \n");
            fprintf(stderr, "           [bursty N (latency in msec)] \n");
            fprintf(stderr, "           [continuous N (latency in msec)] \n");
            fprintf(stderr, "           [--show] \n");
            fprintf(stderr, "\n");
            exit(1);
        }
    }

    /* The other args on the line specify options to be set... */
    for(i = 1; i < argc; i ++)
    {
        if (!strcasecmp(argv[i], "dev")) {
            device = argv[i+1];
            i += 2;
        }

#if 0
        if (!strcasecmp(argv[i], "wmmps")) {
            start = 1;
            for (j=i+1; j < argc; j++) {
                if (!strcasecmp(argv[j], "off")) {
                    wmmps = WMMPS_OFF;
                    i = j;
                    break;
                } else if (!strcasecmp(argv[j], "ac_bk")) {
                    wmmps |= WMMPS_AC_BK;
                } else if (!strcasecmp(argv[j], "ac_be")) {
                    wmmps |= WMMPS_AC_BE;
                } else if (!strcasecmp(argv[j], "ac_vi")) {
                    wmmps |= WMMPS_AC_VI;
                } else if (!strcasecmp(argv[j], "ac_vo")) {
                    wmmps |= WMMPS_AC_VO;
                } else {
                    i = j - 1;
                    break;
                }
            }
        }
#endif
        if (!strcasecmp(argv[i], "--show")) {
            start = 2;
            break;
        }

        if (!strcasecmp(argv[i], "coex")) {
            if (!strcasecmp(argv[i+1], "off")) {
                start = 1;
                coex = 0;
                i++;
            } else if (!strcasecmp(argv[i+1], "on")) {
                start = 1;
                coex = 1;
                i++;
            } else {
                start = 0;
                break;
            }
        }

        if (!strcasecmp(argv[i], "afh_channel")) {
            if (!strcasecmp(argv[i+1], "off")) {
                start = 1;
                afh_channel = 0;
                i++;
            } else if (!strcasecmp(argv[i+1], "on")) {
                start = 1;
                afh_channel = 1;
                i++;
            } else {
                start = 0;
                break;
            }
        }

        if (!strcasecmp(argv[i], "advanced")) {
            if (!strcasecmp(argv[i+1], "off")) {
                start = 1;
                advanced_coex = 0;
                i++;
            } else if (!strcasecmp(argv[i+1], "on")) {
                start = 1;
                advanced_coex = 1;
                i++;
            } else {
                start = 0;
                break;
            }
        }

        if (!strcasecmp(argv[i], "scheme_mgt")) {
            if (!strcasecmp(argv[i+1], "off")) {
                start = 1;
                scheme_mgt = 0;
                i++;
            } else if (!strcasecmp(argv[i+1], "on")) {
                start = 1;
                scheme_mgt = 1;
                i++;
            } else {
                start = 0;
                break;
            }
        }

        if (!strcasecmp(argv[i], "direction")) {
            if (!strcasecmp(argv[i+1], "input")) {
                start = 1;
                periodic_direction = 0;
                i++;
            } else if (!strcasecmp(argv[i+1], "output")) {
                start = 1;
                periodic_direction = 1;
                i++;
            } else {
                start = 0;
                break;
            }
        }

        if (!strcasecmp(argv[i], "wake")) {
            if (!strcasecmp(argv[i+1], "off")) {
                start = 1;
                periodic_wake = 0;
                i++;
            } else if (!strcasecmp(argv[i+1], "on")) {
                start = 1;
                periodic_wake = 1;
                i++;
            } else {
                start = 0;
                break;
            }
        }

        if (!strcasecmp(argv[i], "bursty")) {
            sscanf(argv[i+1], "%d", &periodic_bursty_latency_ms);
            i++;
        }

        if (!strcasecmp(argv[i], "continuous")) {
            sscanf(argv[i+1], "%d", &periodic_continuous_latency_ms);
            i++;
        }

    }
    if (start == 0) {
        fprintf(stderr, "Type \"unifi_qos --help\" to get options.\n");
        return 0;
    }

    fprintf(stderr, " Using:\n");
    fprintf(stderr, " DEVICE: %s\n", device);
//    fprintf(stderr, " WMMPS: 0x%x\n", wmmps);
    if (start == 1) {
        fprintf(stderr, " Coex Enable : %s\n",
                (coex == 1) ? "On" : ((coex == 0) ? "Off" : "Unspecified"));
        fprintf(stderr, " Afh Channel Enable : %s\n",
                (afh_channel == 1) ? "On" : ((afh_channel == 0) ? "Off" : "Unspecified"));
        fprintf(stderr, " Coex Advanced Enable : %s\n",
                (advanced_coex == 1) ? "On" : ((advanced_coex == 0) ? "Off" : "Unspecified"));
        fprintf(stderr, " Scheme Management Enable : %s\n",
                (scheme_mgt == 1) ? "On" : ((scheme_mgt == 0) ? "Off" : "Unspecified"));
        fprintf(stderr, " Periodic Direction: %s\n",
                (periodic_direction == 1) ? "Output" : ((periodic_direction == 0) ? "Input" : "Unspecified"));
        fprintf(stderr, " Periodic Wake Host: %s\n",
                (periodic_wake == 1) ? "On" : ((periodic_wake == 0) ? "Off" : "Unspecified"));
        fprintf(stderr, " Periodic BURSTY Max Latency %d msec \n",
                periodic_bursty_latency_ms);
        fprintf(stderr, " Periodic CONTINUOUS Max Latency %d msec \n",
                periodic_continuous_latency_ms);
        fprintf(stderr, "\n");
    }

    /* Establish communication with char driver. */
    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        exit(errno);
    }

#if 0
    if (wmmps != WMMPS_UNKNOWN) {
        wmmps &= ~WMMPS_UNKNOWN;
        r = ioctl(fd, UNIFI_CFG_UAPSD_TRAFFIC, &wmmps);
        if (r < 0) {
            printf("UNIFI_CFG_UAPSD_TRAFFIC: failed\n");
            goto EXIT;
        } else {
            printf("UNIFI_CFG_UAPSD_TRAFFIC: New mask 0x%x\n", wmmps);
        }
    }
#endif

    ioctl_req_buffer = (unsigned char*) malloc(sizeof(unsigned char) + sizeof(unifi_CoexConfig));
    if (ioctl_req_buffer == NULL) {
        goto EXIT;
    }

    ioctl_req_buffer[0] = 0;
    r = ioctl(fd, UNIFI_CFG_PERIOD_TRAFFIC, ioctl_req_buffer);
    if (r < 0) {
        printf("UNIFI_CFG_PERIOD_TRAFFIC: get failed\n");
        goto EXIT;
    }

    coex_config = (unifi_CoexConfig*) (ioctl_req_buffer + 1);

    if (start == 2) {
        printf("coexEnable = %d \n", coex_config->coexEnable);
        printf("coexAfhChannelEnable = %d \n", coex_config->coexAfhChannelEnable);
        printf("coexAdvancedEnable = %d \n", coex_config->coexAdvancedEnable);
        printf("coexEnableSchemeManagement = %d \n", coex_config->coexEnableSchemeManagement);
        printf("coexDirection = %d \n", coex_config->coexDirection);
        printf("coexPeriodicWakeHost = %d \n", coex_config->coexPeriodicWakeHost);
        printf("coexTrafficBurstyLatencyMs = %d \n", coex_config->coexTrafficBurstyLatencyMs);
        printf("coexTrafficContinuousLatencyMs = %d \n", coex_config->coexTrafficContinuousLatencyMs);

        goto EXIT;
    }

    if (coex != COEX_CONFIG_VALUE_UNKNOWN) {
        coex_config->coexEnable = coex;
    }
    if (afh_channel != COEX_CONFIG_VALUE_UNKNOWN) {
        coex_config->coexAfhChannelEnable = afh_channel;
    }
    if (advanced_coex != COEX_CONFIG_VALUE_UNKNOWN) {
        coex_config->coexAdvancedEnable = advanced_coex;
    }
    if (scheme_mgt != COEX_CONFIG_VALUE_UNKNOWN) {
        coex_config->coexEnableSchemeManagement = scheme_mgt;
    }
    if (periodic_direction != COEX_CONFIG_VALUE_UNKNOWN) {
        if (periodic_direction == 1) {
            coex_config->coexDirection = unifi_CoexDirectionDot11Output;
        } else if (periodic_direction == 0) {
            coex_config->coexDirection = unifi_CoexDirectionDot11Input;
        }
    }
    if (periodic_wake != COEX_CONFIG_VALUE_UNKNOWN) {
        coex_config->coexPeriodicWakeHost = periodic_wake;
    }
    if (periodic_bursty_latency_ms) {
        coex_config->coexTrafficBurstyLatencyMs = periodic_bursty_latency_ms;
    }
    if (periodic_continuous_latency_ms) {
        coex_config->coexTrafficContinuousLatencyMs = periodic_continuous_latency_ms;
    }

    ioctl_req_buffer[0] = 1;
    r = ioctl(fd, UNIFI_CFG_PERIOD_TRAFFIC, ioctl_req_buffer);
    if (r < 0) {
        printf("UNIFI_CFG_PERIOD_TRAFFIC: Set failed\n");
    }
EXIT:

    printf("EXITING...\n");
    if (ioctl_req_buffer != NULL) {
        free(ioctl_req_buffer);
    }
    /* Kill communication with driver. */
    close(fd);

    return r;
} /* main() */

