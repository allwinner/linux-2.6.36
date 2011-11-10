/*
 * ***************************************************************************
 *
 *  FILE: unifi_mib.c
 * 
 *      Get or Set MIB data to the UniFi driver.
 *      Read data from command line and send it to the unifi driver.
 *
 * Copyright (C) 2008-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stddef.h>
#include <sys/types.h>

#include "csr_types.h"
#include "sme_drv.h"
#define UNION union
#define STRUCT struct
#define PACKING
#include "unifiio.h"
#include "driver/sigs.h"
#include "driver/conversions.h"


#define UNIFI_DO_GET_MIB       0
#define UNIFI_DO_SET_MIB       1

#define UNIFIIO_MIB_MAX_OID_LENGTH      32
#define UNIFIIO_MIB_MAX_DATA_LENGTH     256
#define UNIFI_MIB_TYPE_BOOL             0
#define UNIFI_MIB_TYPE_INTEGER          1
#define UNIFI_MIB_TYPE_OCTET_STRING     2
#define MIB_TAG_SEQUENCE        0x30
#define MIB_TAG_BOOLEAN         0x01
#define MIB_TAG_INTEGER         0x02
#define MIB_TAG_OCTETSTRING     0x04
#define MIB_TAG_NULL            0x05
#define MIB_TAG_COUNTER32       0x41


/*
 * ---------------------------------------------------------------------------
 *  print_octet_string
 *
 *      Fucntion to format OCTET_STRING data for printing.
 * 
 *  Arguments:
 *      data, data_len
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static char *
print_octet_string(unsigned char *data, int data_len)
{
    /* Allow 3 chars per byte for xx-xx-xx format */
    static char buf[UNIFIIO_MIB_MAX_DATA_LENGTH * 3];
    char *p;
    int n;

    if (data_len == 0) {
        return "";
    }

    p = buf;
    n = sprintf(p, "%02X", *data++);
    p += n;
    data_len--;

    while (data_len--) {
        n = sprintf(p, "-%02X", *data++);
        p += n;
    }

    return buf;
} /* print_octet_string() */


void print_mib_value(unsigned char *value, int value_len, int mibtype) 
{
    if (mibtype == UNIFI_MIB_TYPE_BOOL) {
        printf("B       %d\n", (int)(*value) ? 1 : 0);
    } else if (mibtype == UNIFI_MIB_TYPE_INTEGER) {
        printf("I       %d\n", *((int*)value));
    } else if (mibtype == UNIFI_MIB_TYPE_OCTET_STRING) {
        printf("S       %s\n", print_octet_string(value, value_len));
    } else {
        printf("*\n");
    }
}

/*
 * ---------------------------------------------------------------------------
 *  query_mib
 *
 *      desc
 * 
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
int
query_mib(int fd, char *oid_str, int mibtype)
{
    unsigned char varbind[MAX_VARBIND_LENGTH];
    unsigned char *ptr;
    int len;
    int rc;

    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    /* Build the MLME-GET.req */
    len = MibEncodeOID(oid_str, ptr);
    len += MibEncodeNull(ptr+len);
    varbind[1] = len;

    if (len > 0) {
        rc = ioctl(fd, UNIFI_GET_MIB, varbind);
        if (rc < 0) {
            perror("ioctl UNIFI_GET_MIB failed");
            return rc;
        }
    } else {
        perror("ioctl UNIFI_GET_MIB failed. Empty request.");
        return -1;
    }

    /* Retrieve the response from UniFi */
    {
        int value_int;
        CsrUint32 value_boolean;
        unsigned char value_string[256];
        int value_string_len;
        unsigned char *ptr;

        /* Find the appended bulk data */
        ptr = varbind;

        /* Skip over the varbind SEQUENCE tag */
        ptr += 2;
        /* Skip over the OID */
        ptr += ptr[1] + 2;

        value_string_len = ptr[1];

        if ((ptr[0] == MIB_TAG_INTEGER) ||
            (ptr[0] == MIB_TAG_COUNTER32))
        {
            /* 
             * Treat counters as integer. 
             * Override the type field so MibDecodeInteger does the right thing.
             */
            ptr[0] = MIB_TAG_INTEGER;
            if (MibDecodeInteger(ptr, &value_int) != 0) {
                print_mib_value((unsigned char*)&value_int, 0, UNIFI_MIB_TYPE_INTEGER);
            }
        } else if (ptr[0] == MIB_TAG_BOOLEAN) {
            if (MibDecodeBoolean(ptr, &value_boolean) != 0) {
                print_mib_value((unsigned char*)&value_boolean, 0, UNIFI_MIB_TYPE_BOOL);
            }
        } else if (ptr[0] == MIB_TAG_OCTETSTRING) {
            if (MibDecodeOctetString(ptr, value_string_len, value_string) != 0) {
                print_mib_value(value_string, value_string_len, UNIFI_MIB_TYPE_OCTET_STRING);
            }
        } else {
            /* unexpected type of return value */
            printf("unrecognised MIB type: %d\n", ptr[0]);
        }
    }


    return 0;
} /* query_mib() */


int
set_mib_int(int fd, char *oid_str, int set_int, int mib_type)
{
    unsigned char varbind[MAX_VARBIND_LENGTH];
    unsigned char *ptr;
    int len;
    int rc;
    
    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    /* Build the MLME-GET.req */
    len = MibEncodeOID(oid_str, ptr);
    if (mib_type == UNIFI_MIB_TYPE_INTEGER) {
        len += MibEncodeInteger(set_int, (ptr+len));
    } else if (mib_type == UNIFI_MIB_TYPE_BOOL) {
        len += MibEncodeBoolean(set_int, (ptr+len));
    } else {
        return -1;
    }
    varbind[1] = len;

    if (len > 0) {
        rc = ioctl(fd, UNIFI_SET_MIB, varbind);
        if (rc < 0) {
            perror("ioctl UNIFI_SET_MIB failed");
            return rc;
        }
    } else {
        perror("ioctl UNIFI_SET_MIB failed. Empty request.");
        return -1;
    }

    return 0;
} /* set_mib_int() */


int
set_mib_string(int fd, char *oid_str, char* set_str, int set_str_len)
{
    unsigned char varbind[MAX_VARBIND_LENGTH + 256];
    unsigned char *ptr;
    int len;
    int rc;

    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    /* Build the MLME-GET.req */
    len = MibEncodeOID(oid_str, ptr);
    len += MibEncodeOctetString((unsigned char*)set_str, set_str_len, (ptr+len));
    varbind[1] = len;

    if (len > 0) {
        rc = ioctl(fd, UNIFI_SET_MIB, varbind);
        if (rc < 0) {
            perror("ioctl UNIFI_SET_MIB failed");
            return rc;
        }
    } else {
        perror("ioctl UNIFI_SET_MIB failed. Empty request.");
        return -1;
    }

    return 0;
} /* set_mib_string() */

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
main(int argc, char **argv)
{
    int fd;
    char *device = "/dev/unifiudi0";
    char *query_str = NULL;
    int set_int = 0;
    char *set_str=NULL, *c, *x;
    int set_str_len = 0;
    int mib_type = UNIFI_MIB_TYPE_INTEGER;
    int request_type = UNIFI_DO_GET_MIB;
    int err = 0, i, j, r;

    /* The other args on the line specify options to be set... */
    for(i = 1; i < argc; i ++)
    {
        if (!strcasecmp(argv[i], "dev")) {
            device = argv[i+1];
            i += 2;
        }
        
        if (!strcasecmp(argv[i], "mode")) {
            if (!strcasecmp(argv[i+1], "get")) {
                request_type = UNIFI_DO_GET_MIB;
                query_str = argv[i+2];
                break;
            } else if (!strcasecmp(argv[i+1], "set")){
                request_type = UNIFI_DO_SET_MIB;
                query_str = argv[i+2];
                i += 2;
            }
        }
    
        if (!strcasecmp(argv[i], "type")) {
            if (!strcasecmp(argv[i+1], "int")) {
                mib_type = UNIFI_MIB_TYPE_INTEGER;
                if(sscanf(argv[i+2], "%d", &set_int) != 1) {
                    perror("Read integer value failed\n");
                }
            } else if (!strcasecmp(argv[i+1], "bool")){
                mib_type = UNIFI_MIB_TYPE_BOOL;
                if(sscanf(argv[i+2], "%d", &set_int) != 1) {
                    perror("Read boolean value failed\n");
                }
            } else if (!strcasecmp(argv[i+1], "string")) {
                mib_type = UNIFI_MIB_TYPE_OCTET_STRING;

                if (argv[i+2] != NULL) {
                    x = argv[i+2];
                    set_str_len = strlen(argv[i+2])/2;
                    c = set_str = (char*)malloc(set_str_len);
                    for ( j = 0; j < (set_str_len*2); j+=2, x+=2, c++) {
                        read_byte(x, (unsigned char*)c);
                    }
                } else {
                    fprintf(stderr, "Empty set string.\n");
                    err++;
                }
            }
            i += 2;
        }
    }

    if ((query_str == NULL)) {
        fprintf(stderr, "Empty request oid\n");
        err++;
    }
    if (err) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    unifi_mib [dev dev] mode get|set oid type int|bool|string N|{ N N .. }\n");
        fprintf(stderr, "        Query or set the value of the given Object ID (e.g. 1.2.840.10036.2.1.1.1.1)\n");
        fprintf(stderr, "        mode gives the operation type (get or set)\n");
        fprintf(stderr, "        type gives the data type: bool, int, string\n");
        fprintf(stderr, "        dev dev    Use <dev> as the device (default is /dev/unifi0).\n");
        fprintf(stderr, "For example: \n");
        fprintf(stderr, "  unifi_mib mode get 1.2.840.10036.2.2.1.4.1 type int\n");
        exit(1);
    }

    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device (Hint: make sure you are root).");
        exit(1);
    }

    if (request_type == UNIFI_DO_GET_MIB) 
    {
        if (query_str) {
            /* Query mode */
            query_mib(fd, query_str, mib_type);
        }
    } else {
        if (mib_type == UNIFI_MIB_TYPE_INTEGER) {
            r = set_mib_int(fd, query_str, set_int, mib_type);
            if (r < 0) {
                printf("set_mib_int: failed\n");
            }
        } else if (mib_type == UNIFI_MIB_TYPE_BOOL) {
            r = set_mib_int(fd, query_str, set_int, mib_type);
            if (r < 0) {
                printf("set_mib_int: failed\n");
            }
        } else if (mib_type == UNIFI_MIB_TYPE_OCTET_STRING) {
            if (set_str != NULL) {
                r = set_mib_string(fd, query_str, set_str, set_str_len);
                if (r < 0) {
                    printf("set_mib_string: failed\n");
                }
            }
        }
    }
    
    if (set_str != NULL) {
        free(set_str);
    }
    close(fd);
    
    return 0;
}

