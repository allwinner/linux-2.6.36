/*
 * ***************************************************************************
 *
 *  FILE: unifi_coredump.c
 *
 *      Display mini-coredumps automatically captured by the driver when it
 *      reset a crashed UniFi.
 *
 * Copyright (C) 2009 by Cambridge Silicon Radio Ltd.
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

#define MAX_DUMP            100         /* Max coredump captures to allow */
#define MAX_REGS_TO_DUMP    0x1000      /* Max NUMBER OF locations to dump */
#define HIGH_DATA_ADDR_MAC  0x3c00      /* 1st *high* 16-bit word to dump from MAC data RAM */
#define HIGH_DATA_ADDR_PHY  0x1c00      /* 1st *high* 16-bit word to dump from PHY data RAM */
#define MAX_COLUMN          10          /* Max values to print per line */
#define BRIEF_LIMIT         60          /* Regs per region to dump in brief mode */

static void print_usage(void);
static const char *space_to_str(unifiio_coredump_space_t space);
static int dump_range(int fd, int dump_index, unifiio_coredump_space_t space,
                      unsigned int start_reg, int num_regs);
static int force_coredump(int fd, int force_reset);
static int display_dump(int fd, int dump_index);
static int display_list(int fd);
static int set_driver_debug(int fd, int level);

static int verbose = 0;
static int brief = 0;


/*
 * ---------------------------------------------------------------------------
 *  printusage
 *      Print program usage hints to stderr
 *
 *  Arguments:
 *      None
 *
 *  Returns:
 *      None
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static void
print_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, " unifi_coredump [--dev /dev/unifiudiN] \n");
    fprintf(stderr, "       [ --force     Force a new coredump capture now ]\n");
    fprintf(stderr, "       [ --reset     Reset UniFi f/w, triggering a future coredump ]\n");
    fprintf(stderr, "       [ --list      Summarise all occurrences ]\n");
    fprintf(stderr, "       [ --index n   Display one (n=0: newest, n=-1: oldest) ]\n");
    fprintf(stderr, "       [ --debug n   Set driver debug level (n=0: lowest) ]\n");
    fprintf(stderr, "       [ --verbose   Increase command verbosity ]\n");
    fprintf(stderr, "       [ --brief     Display short format ]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Dumps UniFi chip status captured at previous h/w resets\n");
    fprintf(stderr, "\n");
}

/*
 * ---------------------------------------------------------------------------
 *  space_to_str
 *      Describe the appropriate memory space enum value
 *
 *  Arguments:
 *      space - space enum to describe
 *
 *  Returns:
 *      Pointer to static string describing space
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static const char *space_to_str(unifiio_coredump_space_t space)
{
    switch (space) {
        case UNIFIIO_COREDUMP_MAC_REG:  return "MAC Registers";
        case UNIFIIO_COREDUMP_PHY_REG:  return "PHY Registers";
        case UNIFIIO_COREDUMP_SH_DMEM:  return "Shared Data RAM";
        case UNIFIIO_COREDUMP_MAC_DMEM: return "MAC Data RAM";
        case UNIFIIO_COREDUMP_PHY_DMEM: return "PHY Data RAM";
        default:                        return "Unknown";
    }
}

/*
 * ---------------------------------------------------------------------------
 *  dump_range
 *      Dumps a buffered memory/register range to stdout
 *
 *  Arguments:
 *      fd          - handle for open "/dev/unifiudiX" device
 *      dump_index  - index of the coredump buffer to display:
 *                      0 = newest, -1 = oldest, 1 = next newest, etc.
 *      space       - CPU memory space of the range to display
 *      num_regs    - number of (16-bit) locations to dump (-1 = all)
 *
 *  Returns:
 *      0 on success, nonzero on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static int
dump_range(int fd,
           int dump_index,
           unifiio_coredump_space_t space,
           unsigned int start_reg,
           int num_regs)
{
    int i, col, r;
    unsigned int offset;
    unifiio_coredump_req_t req;

    i = col = r = 0;

    /* Max values to dump. The ioctl will return an error before this */
    if (num_regs == -1) {
        num_regs = MAX_REGS_TO_DUMP;
    }

    for (i = 0, offset = start_reg; i < num_regs; i++, offset++) {
        memset((void *)&req, 0, sizeof(req));
        /* Retrieve a single register from the dump buffer */
        req.index = dump_index;
        req.space = space;
        req.offset = offset;

        r = ioctl(fd, UNIFI_COREDUMP_GET_REG, (void *)&req);
        if (r < 0) {
            if (errno == ENOENT) {
                fprintf(stderr, "Dump index %d not found\n", dump_index);
            } else if (errno == ERANGE) {
                r = 0;      /* Extent of captured data. No error, just stop */
            } else {
                fprintf(stderr, "%d - ", errno);
                perror("ioctl failed");
            }
            break;
        } else {
            /* Got something */
            if (i == 0) {
                /* Heading banner */
                if (space == UNIFIIO_COREDUMP_MAC_REG) {
                    /* Assumes we dump MAC CPU registers first! */
                    printf("  Chip 0x%04x, F/W %d, Driver %d, Time %d, SeqNo %d, %s:\n",
                           req.chip_ver, req.fw_ver, req.drv_build, req.timestamp, req.serial,
                           (req.requestor == 0) ? "Auto" : "Manual");
                }
                printf("    Space %2d (%s) :", space, space_to_str(space));
            }
            if (verbose > 1) {
                /* One per line */
                printf("\n      %04x : %04x  [%s]",
                       offset, (CsrUint16)req.value, space_to_str(space));
            } else {
                /* Many per line */
                if (col == 0) {
                    printf("\n      %04x : %04x", offset, (CsrUint16)req.value);
                } else {
                    printf("  %04x", (CsrUint16)req.value);
                }
                col++;
                col %= MAX_COLUMN;
            }
        }
    }

    printf("\n");
    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  display_dump
 *      Displays a previously captured coredump to stdout
 *
 *  Arguments:
 *      fd          - handle for open "/dev/unifiudiX" device
 *      dump_index  - index of the coredump buffer to display:
 *                      0 = newest, -1 = oldest, 1 = next newest, etc.
 *  Returns:
 *      0 on success, nonzero on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static int
display_dump(int fd, int dump_index)
{
    int r = 0;
    int num_regs = -1;  /* Dump all by default */

    if (dump_index > MAX_DUMP) {
        fprintf(stderr, "Out of range (%d)\n", MAX_DUMP);
        return ERANGE;
    }
    if (brief != 0) {
        num_regs = BRIEF_LIMIT;
    }
    if (verbose) {
        printf("Dump buffer %d:\n", dump_index);
    }

    r = dump_range(fd, dump_index, UNIFIIO_COREDUMP_MAC_REG, 0, num_regs);
    if (r >= 0) {
        r = dump_range(fd, dump_index, UNIFIIO_COREDUMP_PHY_REG, 0, num_regs);
    }
    if (r >= 0) {
        r = dump_range(fd, dump_index, UNIFIIO_COREDUMP_SH_DMEM, 0, num_regs);
    }
    if (r >= 0) {
        r = dump_range(fd, dump_index, UNIFIIO_COREDUMP_MAC_DMEM, 0, num_regs);
    }
    if (r >= 0) {
        r = dump_range(fd, dump_index,
                       UNIFIIO_COREDUMP_MAC_DMEM, HIGH_DATA_ADDR_MAC, num_regs);
    }
    if (r >= 0) {
        r = dump_range(fd, dump_index, UNIFIIO_COREDUMP_PHY_DMEM, 0, num_regs);
    }
    if (r >= 0) {
        r = dump_range(fd, dump_index,
                       UNIFIIO_COREDUMP_PHY_DMEM, HIGH_DATA_ADDR_PHY, num_regs);
    }
    if (verbose) {
        printf("Done.\n");
    }

    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  display_list
 *      Displays a summary list of all coredumps currently being stored by
 *      the driver
 *
 *  Arguments:
 *      fd - handle for open "/dev/unifiudiX" device
 *
 *  Returns:
 *      0 on success, nonzero on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static int
display_list(int fd)
{
    int i, r = 0;
    unifiio_coredump_req_t req;

    if (verbose) {
        printf("Listing...\n");
    }

    for (i = 0; i < MAX_DUMP; i++) {
        memset((void *)&req, 0, sizeof(req));

        /* Grab a single register from the dump buffer, to see if anything's there */
        req.index = i;
        req.space = UNIFIIO_COREDUMP_MAC_REG;
        req.offset = 0;
        r = ioctl(fd, UNIFI_COREDUMP_GET_REG, (void *)&req);

        if (r < 0) {
            if (errno == ENOENT) {
                break;  /* Not found: extent of available dumps */
            }
            fprintf(stderr, "%d - ", errno);
            perror("ioctl UNIFI_COREDUMP_GET_REG failed");
            return r;
        } else {
            printf("  %3d : Chip 0x%04x, F/W %d, Driver %d, Time %d, SeqNo %d, %s\n",
                   i, req.chip_ver, req.fw_ver, req.drv_build,
                   req.timestamp, req.serial,
                   (req.requestor == 0) ? "Auto" : "Manual");
            if (verbose) {
                printf("        0x%x:%04x = 0x%04x\n", req.space, req.offset, req.value);
            }
        }
    }
    printf("%d coredumps available\n", i);
    if (verbose) {
        printf("Done.\n");
    }

    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  force_coredump
 *      Makes the device driver stop the UniFi and capture a coredump of its
 *      current register values into the driver's next available buffer
 *
 *  Arguments:
 *      fd          - handle for open "/dev/unifiudiX" device
 *      force_reset - force a UniFi reset to simulate crash
 *
 *  Returns:
 *      0 on success, nonzero on error
 *
 *  Notes:
 *      The device driver buffers coredumps in a circular buffer format, thus
 *      a new coredump will overwrite the oldest stored one.
 * ---------------------------------------------------------------------------
 */
static int
force_coredump(int fd, int force_reset)
{
    int r = 0;
    unifiio_coredump_req_t req;

    if (verbose) {
        printf("Forcing coredump...\n");
    }

    memset((void *)&req, 0, sizeof(req));
    req.space = UNIFIIO_COREDUMP_TRIGGER_MAGIC;
    if (force_reset) {
        /* This will force a firmware reset, to simulate watchdog reset */
        req.index = UNIFIIO_COREDUMP_TRIGGER_MAGIC;
    }

    r = ioctl(fd, UNIFI_COREDUMP_GET_REG, (void *)&req);
    if (r < 0) {
        fprintf(stderr, "%d - ", errno);
        perror("ioctl UNIFI_COREDUMP_GET_REG failed");
    }
    if (verbose) {
        printf("Done.\n");
    }
    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  set_driver_debug
 *      Sets the debug level in the UniFi driver
 *
 *  Arguments:
 *      fd    - handle for open "/dev/unifiudiX" device
 *      level - new debug level to set
 *
 *  Returns:
 *      0 on success, nonzero on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static int
set_driver_debug(int fd, int level)
{
    int r = 0;

    if (verbose) {
        printf("Setting unifi_debug level to %d\n", level);
    }

    r = ioctl(fd, UNIFI_SET_DEBUG, level);
    if (r < 0) {
        fprintf(stderr, "%d - ", errno);
        perror("ioctl UNIFI_SET_DEBUG failed");
    }

    if (verbose) {
        printf("Done.\n");
    }
    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  main
 *      Entry point for the unifi_coredump program
 *
 *  Arguments:
 *      argc, argv
 *
 *  Returns:
 *      0 on success, non-zero on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    char device[32];
    int start = 0;
    int index = -2;
    int list = 0;
    int force = 0;
    int force_reset = 0;
    int debug_level = -1;
    int r = 0;
    int fd = -1;
    int option_index = 0;
    int optc;

    static const struct option long_options[] = {
               {"dev",      1, 0, 0},
               {"force",    0, 0, 0},
               {"reset",    0, 0, 0},
               {"verbose",  0, 0, 0},
               {"brief",    0, 0, 0},
               {"list",     0, 0, 0},
               {"index",    1, 0, 0},
               {"debug",    1, 0, 0},
               {0,          0, 0, 0} };

    strcpy(&device[0], "/dev/unifiudi0");       /* Default device */

    if (argc > 1) {
        if (!strcasecmp(argv[1], "--help")) {
            print_usage();
            r = 1;
            goto EXIT;
        }
    }

    while ((optc = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        start = 1;
        if (optc == 0) {

            if (!strcasecmp(long_options[option_index].name, "dev")) {
                r = strlen(optarg);
                if (r >= (sizeof(device) - 1)) {
                    fprintf(stderr, "dev arg %s is too long\n", optarg);
                    r = 1;
                    goto EXIT;
                }
                memcpy(device, optarg, r);
                device[r] = 0;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "verbose")) {
                verbose++;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "force")) {
                if (verbose) {
                    printf("Triggering coredump on UniFi\n");
                }
                force = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "reset")) {
                if (verbose) {
                    printf("Triggering simulated reset on UniFi\n");
                }
                force = 1;
                force_reset = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "list")) {
                list = 1;
                if (verbose) {
                    printf("list specified\n");
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "brief")) {
                if (verbose) {
                    printf("Using shortened format\n");
                }
                brief = 1;
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "index")) {
                if (optarg == NULL) {
                    fprintf(stderr, "option 'index' requires an argument\n");
                    r = 1;
                    goto EXIT;
                }

                index = atoi(optarg);
                if (verbose) {
                    fprintf(stderr, "index %d specified\n", index);
                }
                continue;
            }

            if (!strcasecmp(long_options[option_index].name, "debug")) {
                if (optarg == NULL) {
                    fprintf(stderr, "option 'debug' requires an argument\n");
                    r = 1;
                    goto EXIT;
                }

                debug_level = atoi(optarg);
                if (verbose) {
                    fprintf(stderr, "driver debug level %d specified\n", debug_level);
                }
                continue;
            }

            fprintf(stderr, "Unrecognised option: %s\n", long_options[option_index].name);
            start = 0;
            break;

        } else {
            start = 0;
        }
    }

    if (optind < argc) {
        printf("Unrecognised option(s) ignored: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf (stderr, "\n");
    }

    if (start == 0) {
        print_usage();
        r = 1;
        goto EXIT;
    }

    if (verbose) {
        fprintf(stderr, "Using:\n");
        fprintf(stderr, " DEVICE    : %s\n", device);
        fprintf(stderr, " Verbosity : %d\n", verbose);
        fprintf(stderr, "\n");
    }

    /*
     * Act on commands
     */
    fd = open(device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "%d - ", errno);
        perror("Failed to open device");
        r = errno;
        goto EXIT;
    } else if (verbose) {
        fprintf(stderr, "Opened %s sucessfully\n", device);
    }

    if (force) {
        r = force_coredump(fd, force_reset);
        if (r < 0) {
            goto EXIT;
        }
    }

    if (list) {
        r = display_list(fd);
        if (r < 0) {
            goto EXIT;
        }
    }

    if (index >= -1) {
        r = display_dump(fd, index);
        if (r < 0) {
            goto EXIT;
        }
    }

    if (debug_level >= 0) {
        r = set_driver_debug(fd, debug_level);
        if (r < 0) {
            goto EXIT;
        }
    }

EXIT:
    if (fd >= 0) {
        close(fd);
    }

    if (verbose) {
        fprintf(stderr, "Exit (%d)\n", r);
    }

    return r;
} /* main() */

