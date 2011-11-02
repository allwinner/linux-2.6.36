/*
 * ***************************************************************************
 *
 *  FILE: genmacmib.c
 * 
 *      Program to generate a MIB file containing a single entry to
 *      set the chip MAC address.
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

int read_mac(char *arg, unsigned char *macaddr);


static void output_mib(unsigned char *macaddr, FILE *fp);
static void output_raw(unsigned char *macaddr, FILE *fp);



/*
 * ---------------------------------------------------------------------------
 *  main
 *
 *      C entry point
 * 
 *  Arguments:
 *      argc, argv      command line args
 *
 *  Returns:
 *      exit code
 * ---------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    unsigned char macaddr[6];
    int mib = 0;
    char *outfile = "mac.dat";
    FILE *fp;
    int c, err;


    err = 0;
    while ((c=getopt(argc, argv, "bmo:")) != -1)
    {
        switch (c) {
          case 'b':
            mib = 0;
            break;
          case 'm':
            mib = 1;
            break;
          case 'o':
            outfile = optarg;
            break;
          default:
            err++;
        }
    }


    if (optind >= argc) {
        fprintf(stderr, "Expected MAC address\n");
        err++;
    }
    if (err) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    genmacmib [options] xx:xx:xx:xx:xx:xx\n");
        fprintf(stderr, "        Generate a binary containing the given MAC address\n");
        fprintf(stderr, "        The file can be raw binary or a UniFi MIB\n");
        fprintf(stderr, "    Options:\n");
        fprintf(stderr, "        -b        Generate a raw binary file (default)\n");
        fprintf(stderr, "        -m        Generate a MIB file\n");
        fprintf(stderr, "        -o file   Send output to file\n");
        exit(1);
    }

    if (read_mac(argv[optind], macaddr)) {
        fprintf(stderr, "Badly formatted MAC address\n");
        exit(1);
    }

    if (outfile) {
        fp = fopen(outfile, "w");
        if (!fp) {
            perror("Failed to open output file");
            exit(2);
        }
    } else {
        fprintf(stderr, "Unspecified Output file.\n");
        exit(1);
    }

    if (mib) {
        output_mib(macaddr, fp);
    } else {
        output_raw(macaddr, fp);
    }

    if (outfile) {
        fclose(fp);
    }

    return 0;
} /* main() */



static void
output_mib(unsigned char *macaddr, FILE *fp)
{
    /* Output the header */
    putc('U', fp);
    putc('D', fp);
    putc('M', fp);
    putc('I', fp);

    /* Output the version */
    putc(1, fp);
    putc(0, fp);

    /* total length following - 24 */
    putc(24, fp);
    putc(0, fp);
    putc(0, fp);
    putc(0, fp);

    /* MIB length - 22 */
    putc(0x16, fp);
    putc(0, fp);

    /* Sequence tag and len */
    putc(0x30, fp);
    putc(0x14, fp);

    /* OID tag, len and value */
    putc(0x06, fp);
    putc(0x0a, fp);
    putc(0x2a, fp);
    putc(0x86, fp);
    putc(0x48, fp);
    putc(0xce, fp);
    putc(0x34, fp);
    putc(0x02, fp);
    putc(0x01, fp);
    putc(0x01, fp);
    putc(0x01, fp);
    putc(0x01, fp);

    /* MAC addr: OCTETSTRING tag, len and value */
    putc(0x04, fp);
    putc(0x06, fp);
    putc(macaddr[0], fp);
    putc(macaddr[1], fp);
    putc(macaddr[2], fp);
    putc(macaddr[3], fp);
    putc(macaddr[4], fp);
    putc(macaddr[5], fp);
} /* output_mib() */

static void
output_raw(unsigned char *macaddr, FILE *fp)
{
    putc(macaddr[0], fp);
    putc(macaddr[1], fp);
    putc(macaddr[2], fp);
    putc(macaddr[3], fp);
    putc(macaddr[4], fp);
    putc(macaddr[5], fp);
} /* output_raw() */


int
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


int
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
int
read_mac(char *arg, unsigned char *macaddr)
{
    char *p = arg;
    int i, n;

    i = 0;

    n = read_byte(p, &macaddr[0]);
    if (n == 0) return -1;
    p += n;

    if (!is_delim(*p)) return -1;
    p++;

    n = read_byte(p, &macaddr[1]);
    if (n == 0) return -1;
    p += n;

    if (!is_delim(*p)) return -1;
    p++;

    n = read_byte(p, &macaddr[2]);
    if (n == 0) return -1;
    p += n;

    if (!is_delim(*p)) return -1;
    p++;

    n = read_byte(p, &macaddr[3]);
    if (n == 0) return -1;
    p += n;

    if (!is_delim(*p)) return -1;
    p++;

    n = read_byte(p, &macaddr[4]);
    if (n == 0) return -1;
    p += n;

    if (!is_delim(*p)) return -1;
    p++;

    n = read_byte(p, &macaddr[5]);
    if (n == 0) return -1;
    p += n;

    if (*p != '\0') return -1;

    return 0;
} /* read_mac() */
