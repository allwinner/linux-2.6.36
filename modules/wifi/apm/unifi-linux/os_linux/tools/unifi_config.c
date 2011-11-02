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
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

#include "unifiio.h"

#include "unifi_config_lib.h"

#define INSTSIZE 256  /* must be >= IFNAMSIZ */

static const char*
trace_traffic_type(unifi_TrafficType traffic)
{
    switch (traffic)
    {
        case unifi_TrafficOccasional:
            return "Occasional";
        case unifi_TrafficBursty:
            return "Bursty";
        case unifi_TrafficPeriodic:
            return "Periodic";
        case unifi_TrafficContinuous:
            return "Continuous";
        default:
            return "ERROR: unrecognised Traffic Type";
    }
 }


static const char*
trace_power_save_type(unifi_PowerSaveLevel power_save)
{
    switch (power_save)
    {
        case unifi_PowerSaveLow:
            return "Active";
        case unifi_PowerSaveHigh:
            return "Full";
        case unifi_PowerSaveMed:
            return "Fast";
        case unifi_PowerSaveAuto:
            return "Auto";
        default:
            return "ERROR: unrecognised Power Save Mode";
    }
 }


static const char*
trace_coex_scheme_type(unifi_CoexScheme coex_scheme)
{
    switch (coex_scheme)
    {
        case unifi_CoexSchemeDisabled:
            return "None";
        case unifi_CoexSchemeCSR:
            return "CSR";
        case unifi_CoexSchemeCSRChannel:
            return "CSR Channel";
        case unifi_CoexSchemePTA:
            return "PTA";
        default:
            return "ERROR: unrecognised Coex Scheme";
    }
 }


static const char*
trace_power_supply_type(unifi_HostPowerMode power_supply)
{
    switch (power_supply)
    {
        case unifi_HostActive:
            return "Running on mains";
        case unifi_HostPowersave:
            return "Running on batteries";
        case unifi_HostFullPowersave:
            return "Unspecified power supply";
        default:
            return "ERROR: unrecognised power supply";
    }
 }


/*
 * ---------------------------------------------------------------------------
 *  read_byte
 *  is_delim
 *
 *      Support fns.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static int
read_byte(char *p, unsigned char *b)
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
 *  read_ie
 *
 *
 *  Arguments:
 *      arg             The command-line argument string
 *      periodic_ie     Pointer to the array to receive IE.
 *
 *  Returns:
 *      number of bytes on success, -1 if the string does not conform to expected format.
 * ---------------------------------------------------------------------------
 */
static int
read_ie(char *arg, unsigned char *periodic_ie)
{
    char *p = arg;
    int i, n, offset;

    i = 0;
    n = 0;
    offset = 0;
    do {
        n = read_byte(p, &periodic_ie[offset]);
        p += n;

        if (!is_delim(*p)) break;

        p++;
        offset ++;
    } while ((n > 0) && (n < UNIFI_CFG_MAX_ADDTS_IE_LENGTH));

    return offset+1;
} /* read_ie() */


int
main(int argc, char **argv)
{
    char device[32];
    int start;
    int index;
    int r = 0;
    int fd;
    unifi_cfg_power_t wifi_power = UNIFI_CFG_POWER_UNSPECIFIED;
    unifi_cfg_powersave_t wifi_powersave = UNIFI_CFG_POWERSAVE_UNSPECIFIED;
    unifi_cfg_powersupply_t wifi_powersupply = UNIFI_CFG_POWERSUPPLY_UNSPECIFIED;
    unsigned char wifi_filter_set;
    unsigned int wifi_filter;
    unsigned char uapsd_mask_set;
    CsrUint8 uapsd_mask;
    unsigned char maxsp_set;
    CsrUint8 maxsp_length;
    CsrUint8 wmm_qos_info;
    unsigned char wmm_addts_tid_set;
    CsrUint32 wmm_addts_tid;
    unsigned char wmm_delts_tid_set;
    CsrUint32 wmm_delts_tid;
    unsigned char wmm_addts_ie_set;
    unsigned char addts_ie_length;
    CsrUint8 addts_ie[UNIFI_CFG_MAX_ADDTS_IE_LENGTH];
    unsigned char show_set;
    int option_index = 0, optc;
    char *value;
    unsigned char uchar_value;

    static const struct option long_options[] = {
               {"dev", 1, 0, 0},
               {"show", 0, 0, 0},
               {"wifion", 0, 0, 0},
               {"wifioff", 0, 0, 0},
               {"powersave", 1, 0, 0},
               {"batteries", 1, 0, 0},
               {"filter", 1, 0, 0},
               {"uapsd", 1, 0, 0},
               {"maxsp", 1, 0, 0},
               {"addts_ie", 1, 0, 0},
               {"addts_tid", 1, 0, 0},
               {"delts_tid", 1, 0, 0},
               {"strict_draftn", 1, 0, 0},
               {"enable_okc", 1, 0, 0},
               {0, 0, 0, 0} };

    static const char *filter_options[] = {"dhcp", "arp", "nbns", "nbds",
                                           "cups", "none", "all", NULL };

    static const char *uapsd_options[] = {"vo", "vi", "bk", "be", "none",
                                          "all", "default", NULL };

    static const char *maxsp_options[] = {"2", "4", "6", "all", NULL };

    if (argc > 1) {
        if (!strcasecmp(argv[1], "--help")) {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, " unifi_config --dev /dev/unifiudiN \n");
            fprintf(stderr, "           [ --show ] \n");
            fprintf(stderr, "           [ --wifioff ] \n");
            fprintf(stderr, "           [ --wifion ] \n");
            fprintf(stderr, "           [ --batteries {yes | no} ] \n");
            fprintf(stderr, "           [ --powersave {none | fast | full | auto} ] \n");
            fprintf(stderr, "           [ --filter {dhcp,arp,nbns,nbds,cups | none | all} ] \n");
            fprintf(stderr, "           [ --uapsd {be,bk,vi,vo | none | all} ] \n");
            fprintf(stderr, "           [ --maxsp {2,4,6 | all} ] \n");
            fprintf(stderr, "           [ --addts_ie {NN:NN:...:NN (in hex)} ] \n");
            fprintf(stderr, "           [ --addts_tid {NN (in hex)} ] \n");
            fprintf(stderr, "           [ --delts_tid {NN (in hex)} ] \n");
            fprintf(stderr, "           [ --strict_draftn {yes | no} ] \n");
            fprintf(stderr, "           [ --enable_okc {yes | no} ] \n");
            fprintf(stderr, "\n");
            fprintf(stderr, "Note that a valid ADDTS command must specify both --addts_ie and --addts_tid\n");
            fprintf(stderr, "\n");
            exit(1);
        }
    }

    start = 0;
    show_set = 0;
    wifi_filter_set = 0;
    wifi_filter = UNIFI_CFG_FILTER_NONE;
    uapsd_mask_set = uapsd_mask = 0;
    maxsp_set = maxsp_length = wmm_qos_info = 0;
    wmm_addts_tid_set = addts_ie_length = wmm_addts_ie_set = wmm_addts_tid = 0;
    wmm_delts_tid_set = wmm_delts_tid = 0;
    CsrBool strict_draftn_set = FALSE;
    CsrBool strict_draftn     = FALSE;
    CsrBool enable_okc_set    = FALSE;
    CsrBool enable_okc        = TRUE;

    while ((optc = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        start = 1;
        if (optc == 0) {

            if (!strcasecmp(long_options[option_index].name, "dev")) {
                r = strlen(optarg);
                if (r >= (sizeof(device) - 1)) {
                    printf("dev arg %s is too long\n", optarg);
                    exit(1);
                }
                memcpy(device, optarg, r);
                device[r] = 0;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "wifion")) {
                wifi_power = UNIFI_CFG_POWER_ON;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "wifioff")) {
                wifi_power = UNIFI_CFG_POWER_OFF;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "show")) {
                show_set = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "batteries")) {

                if (optarg == NULL) {
                    printf("option batteries requires an argument\n");
                    exit(1);
                }

                if (!strcasecmp(optarg, "yes")) {
                    wifi_powersupply = UNIFI_CFG_POWERSUPPLY_BATTERIES;
                } else if (!strcasecmp(optarg, "no")) {
                    wifi_powersupply = UNIFI_CFG_POWERSUPPLY_MAINS;
                } else {
                    printf("batteries invalid option: %s\n", optarg);
                    start = 0;
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "powersave")) {

                if (optarg == NULL) {
                    printf("option powersave requires an argument\n");
                    exit(1);
                }

                if (!strcasecmp(optarg, "none")) {
                    wifi_powersave = UNIFI_CFG_POWERSAVE_NONE;
                } else if (!strcasecmp(optarg, "fast")) {
                    wifi_powersave = UNIFI_CFG_POWERSAVE_FAST;
                } else if (!strcasecmp(optarg, "full")) {
                    wifi_powersave = UNIFI_CFG_POWERSAVE_FULL;
                } else if (!strcasecmp(optarg, "auto")) {
                    wifi_powersave = UNIFI_CFG_POWERSAVE_AUTO;
                } else {
                    printf("powersave invalid option: %s\n", optarg);
                    start = 0;
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "filter")) {

                while ((index = getsubopt(&optarg, filter_options, &value)) != -1) {
                    switch(index) {
                      case 0:
                        wifi_filter |= UNIFI_CFG_FILTER_DHCP;
                        wifi_filter_set = 1;
                        break;
                      case 1:
                        wifi_filter |= UNIFI_CFG_FILTER_ARP;
                        wifi_filter_set = 1;
                        break;
                      case 2:
                        wifi_filter |= UNIFI_CFG_FILTER_NBNS;
                        wifi_filter_set = 1;
                        break;
                      case 3:
                        wifi_filter |= UNIFI_CFG_FILTER_NBDS;
                        wifi_filter_set = 1;
                        break;
                      case 4:
                        wifi_filter |= UNIFI_CFG_FILTER_CUPS;
                        wifi_filter_set = 1;
                        break;
                      case 5:
                        wifi_filter = UNIFI_CFG_FILTER_NONE;
                        wifi_filter_set = 1;
                        break;
                      case 6:
                        wifi_filter = UNIFI_CFG_FILTER_ALL;
                        wifi_filter_set = 1;
                        break;
                      default:
                        printf("unrecognised filter arg: %s\n", filter_options[index]);
                        break;
                    }
                }

                if (!wifi_filter_set)
                {
                    printf("option --filter requires an argument\n");
                    exit(1);
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "uapsd")) {

                while ((index = getsubopt(&optarg, uapsd_options, &value)) != -1) {
                    switch(index) {
                      case 0:       /* VO */
                      case 1:       /* VI */
                      case 2:       /* BK */
                      case 3:       /* BE */
                        uapsd_mask |= (1 << index);
                        uapsd_mask_set = 1;
                        break;
                      case 4:
                        uapsd_mask = 0x00;
                        uapsd_mask_set = 1;
                        break;
                      case 5:
                        uapsd_mask = 0x0F;      /* All 4 bits set to 1 */
                        uapsd_mask_set = 1;
                        break;
                      case 6:
                        uapsd_mask = 0xFF;      /* SME will handle this */
                        uapsd_mask_set = 1;
                        break;
                      default:
                        printf("unrecognised filter arg: %s\n", uapsd_options[index]);
                        break;
                    }
                }

                if (!uapsd_mask_set)
                {
                    printf("option --uapsd requires an argument\n");
                    exit(1);
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "maxsp")) {

                while ((index = getsubopt(&optarg, maxsp_options, &value)) != -1) {
                    switch(index) {
                      case 0:
                        maxsp_length = 0x01;
                        maxsp_set = 1;
                        break;
                      case 1:
                        maxsp_length = 0x02;
                        maxsp_set = 1;
                        break;
                      case 2:
                        maxsp_length = 0x03;
                        maxsp_set = 1;
                        break;
                      case 3:
                        maxsp_length = 0x00;
                        maxsp_set = 1;
                        break;
                      default:
                        printf("unrecognised filter arg: %s\n", maxsp_options[index]);
                        break;
                    }
                }

                if (!maxsp_set)
                {
                    printf("option --maxsp requires an argument\n");
                    exit(1);
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "addts_ie")) {
                addts_ie_length = read_ie(optarg, addts_ie);
                if (addts_ie_length != UNIFI_CFG_MAX_ADDTS_IE_LENGTH) {
                    printf("option --addts_ie requires a valid %d bytes hex array\n",
                           UNIFI_CFG_MAX_ADDTS_IE_LENGTH);
                    exit(1);
                }
                wmm_addts_ie_set = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "addts_tid")) {
                if (!read_byte(optarg, &uchar_value)) {
                    printf("option --addts_tid requires a valid hex number\n");
                    exit(1);
                }
                wmm_addts_tid = uchar_value;
                wmm_addts_tid_set = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "delts_tid")) {
                if (!read_byte(optarg, &uchar_value)) {
                    printf("option --addts_tid requires a valid hex number\n");
                    exit(1);
                }
                wmm_delts_tid = uchar_value;
                wmm_delts_tid_set = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "strict_draftn")) {
                if (optarg == NULL) {
                    printf("option strict_draftn requires as argument\n");
                    exit(1);
                }

                if (!strcasecmp(optarg, "yes")) {
                    strict_draftn = TRUE;
                    strict_draftn_set = TRUE;
                } else if (!strcasecmp(optarg, "no")) {
                    strict_draftn = FALSE;
                    strict_draftn_set = TRUE;
                } else {
                    printf("strict_draftn invalid option: %s\n", optarg);
                    start = 0;
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "enable_okc")) {
                if (optarg == NULL) {
                    printf("option enable_okc requires as argument\n");
                    exit(1);
                }

                if (!strcasecmp(optarg, "yes")) {
                    enable_okc = TRUE;
                    enable_okc_set = TRUE;
                } else if (!strcasecmp(optarg, "no")) {
                    enable_okc = FALSE;
                    enable_okc_set = TRUE;
                } else {
                    printf("enable_okc invalid option: %s\n", optarg);
                    start = 0;
                }
                continue;
            }

            printf("Unrecognised option: %s\n", long_options[option_index].name);
            start = 0;
            break;

        } else {
            start = 0;
        }
    }

    if (optind < argc)
    {
        printf("Unrecognised option(s) ignored: ");
        while (optind < argc) {
            printf ("%s ", argv[optind++]);
        }
        printf ("\n");
    }

    if (start == 0) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, " unifi_config --dev /dev/unifiudiN \n");
        fprintf(stderr, "           [ --show ] \n");
        fprintf(stderr, "           [ --wifioff ] \n");
        fprintf(stderr, "           [ --wifion ] \n");
        fprintf(stderr, "           [ --batteries {yes | no} ] \n");
        fprintf(stderr, "           [ --powersave {none | fast | full | auto} ] \n");
        fprintf(stderr, "           [ --filter {dhcp,arp,nbns,nbds,cups | none | all} ] \n");
        fprintf(stderr, "           [ --uapsd {be,bk,vi,vo | none | all} ] \n");
        fprintf(stderr, "           [ --maxsp {2,4,6 | all} ] \n");
        fprintf(stderr, "           [ --addts_ie {NN:NN:...:NN (in hex)} ] \n");
        fprintf(stderr, "           [ --addts_tid {NN (in hex)} ] \n");
        fprintf(stderr, "           [ --delts_tid {NN (in hex)} ] \n");
        fprintf(stderr, "           [ --strict_draftn {yes | no} ] \n");
        fprintf(stderr, "           [ --enable_okc {yes | no} ] \n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Note that a valid ADDTS command must specify both --addts_ie and --addts_tid\n");
        fprintf(stderr, "\n");
        exit(1);
    }

#if 0
    fprintf(stderr, " Using:\n");
    fprintf(stderr, " DEVICE: %s\n", device);
    fprintf(stderr, " Show: %s\n", (show_set) ? "YES" : "NO");
    fprintf(stderr, " WiFi ON/OFF: %d\n", wifi_power);
    fprintf(stderr, " Power: %d\n", wifi_powersave);
    fprintf(stderr, " Filter: 0x%x\n", wifi_filter);
    fprintf(stderr, " U-APSD Mask: 0x%x\n", uapsd_mask);
    fprintf(stderr, "\n");
#endif

    fd = unifi_cfg_init(device);
    if (fd < 0) {
        perror("Failed to open device");
        exit(errno);
    }

    if (strict_draftn_set)
    {
        r = unifi_cfg_set_strict_draft_n(fd, strict_draftn);
        if (r < 0)
        {
            printf("UNIFI_CFG_STRICT_DRAFT_N: failed\n");
            goto EXIT;
        }
    }
    if (enable_okc_set) 
    {
        r = unifi_cfg_set_enable_okc(fd, enable_okc);
        if (r < 0)
        {
            printf("UNIFI_CFG_ENABLE_OKC: failed\n");
            goto EXIT;
        }
    }

    if (wifi_power != UNIFI_CFG_POWER_UNSPECIFIED) {
        r = unifi_cfg_set_power(fd, wifi_power);
        if (r < 0) {
            printf("UNIFI_CFG_POWER: failed\n");
            goto EXIT;
        }
    }

    if (wifi_powersave != UNIFI_CFG_POWERSAVE_UNSPECIFIED) {
        r = unifi_cfg_set_power_save(fd, wifi_powersave);
        if (r < 0) {
            printf("UNIFI_CFG_POWERSAVE: failed\n");
            goto EXIT;
        }
    }

    if (wifi_powersupply != UNIFI_CFG_POWERSUPPLY_UNSPECIFIED) {
        r = unifi_cfg_set_power_supply(fd, wifi_powersupply);
        if (r < 0) {
            printf("UNIFI_CFG_POWERSUPPLY: failed\n");
            goto EXIT;
        }
    }

    if (wifi_filter_set) {
        r = unifi_cfg_set_filters(fd, wifi_filter);
        if (r < 0) {
            printf("UNIFI_CFG_FILTER: failed\n");
            goto EXIT;
        }
    }

    if (maxsp_set || uapsd_mask_set) {
        /* Max SP Length can only be used with one or more U-APSD ACs */
        if (!uapsd_mask && maxsp_set) {
            printf("UNIFI_CFG_WMM_QOSINFO: Max SP Length can only be used with one or more U-APSD ACs\n");
            goto EXIT;
        }

        wmm_qos_info = (maxsp_length << 5) | uapsd_mask;
        r = unifi_cfg_set_wmm_qos_info(fd, wmm_qos_info);
        if (r < 0) {
            printf("UNIFI_CFG_WMM_QOSINFO: failed\n");
            goto EXIT;
        }
    }

    if (wmm_delts_tid_set) {
        r = unifi_cfg_wmm_delts(fd, wmm_delts_tid);
        if (r < 0) {
            printf("UNIFI_CFG_WMM_DELTS: failed\n");
            goto EXIT;
        }
    }

    if (wmm_addts_tid_set && wmm_addts_ie_set) {
        r = unifi_cfg_wmm_addts(fd, wmm_addts_tid, addts_ie, addts_ie_length);
        if (r < 0) {
            printf("UNIFI_CFG_WMM_ADDTS: failed\n");
            goto EXIT;
        }
    }

    if (show_set) {
        unsigned char *result;
        unifi_CoexInfo coex_info;
        unifi_PowerSaveLevel power_save_info;
        unifi_HostPowerMode host_power_mode;
        char drv_inst[INSTSIZE];

        drv_inst[0] = '\0';
        result = (unsigned char*) (&drv_inst[0]);
        r = unifi_cfg_get_info(fd, UNIFI_CFG_GET_INSTANCE, result);
        if (r < 0) {
            printf("UNIFI_CFG_GET_INSTANCE: failed\n");
        }
        printf("Current Driver Info:\n");
        printf("    Interface Name = %s\n", drv_inst);

        result = (unsigned char*) (&coex_info);
        r = unifi_cfg_get_info(fd, UNIFI_CFG_GET_COEX, result);
        if (r < 0) {
            printf("UNIFI_CFG_GET_COEX: failed\n");
            goto EXIT;
        }

        printf("Current Coex Info:\n");
        printf("    Traffic Type = %s\n",
               trace_traffic_type(coex_info.currentTrafficType));
        if (coex_info.currentTrafficType == unifi_TrafficPeriodic) {
            printf("    Traffic Period = %d\n", coex_info.currentPeriodMs);
        } else if (coex_info.currentCoexLatencyMs) {
            printf("    Traffic Latency = %d\n", coex_info.currentCoexLatencyMs);
        }
        printf("    Power Save Mode = %s\n",
               trace_power_save_type(coex_info.currentPowerSave));
        printf("    Has Traffic Data = %s\n",
               coex_info.hasTrafficData ? "Yes" : "No");
        printf("    Has Bluetooth device = %s\n",
               coex_info.hasBtDevice ? "Yes" : "No");
        printf("    Coex Scheme = %s\n",
               trace_coex_scheme_type(coex_info.currentCoexScheme));

        result = (unsigned char*) (&power_save_info);
        r = unifi_cfg_get_info(fd, UNIFI_CFG_GET_POWER_MODE, result);
        if (r < 0) {
            printf("UNIFI_CFG_GET_POWER_MODE: failed\n");
            goto EXIT;
        }

        printf("Configured Power Save Mode = %s\n",
               trace_power_save_type(power_save_info));

        result = (unsigned char*) (&host_power_mode);
        r = unifi_cfg_get_info(fd, UNIFI_CFG_GET_POWER_SUPPLY, result);
        if (r < 0) {
            printf("UNIFI_CFG_GET_POWER_SUPPLY: failed\n");
            goto EXIT;
        }

        printf("%s\n",
               trace_power_supply_type(host_power_mode));

    }


EXIT:
    /* If the command failed report the error. Generally this will be the
     * driver error code returned via the ioctl.
     */
    if (r < 0) {
        printf("status %d, errno %d\n", r, errno);
    }

    /* Kill communication with driver. */
    unifi_cfg_deinit(fd);

    return r;
} /* main() */

