/*
 * ---------------------------------------------------------------------------
 * FILE:     prmib.c
 * 
 * PURPOSE:
 *      This file provides functions to send MLME requests to the UniFi.
 *      It is OS-independent.
 *      
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


static int process_mib_file(const char *mib_file);



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
    const char *filename;

    if (argc < 2) {
        fprintf(stderr, "No filename given\n");
        exit(1);
    }


    filename = argv[1];

    if (process_mib_file(filename)) {
        exit(1);
    }


    return 0;
} /* main() */



/*
 * ---------------------------------------------------------------------------
 *  print_varbind
 *
 *      Print out the MIB setting.
 * 
 *  Arguments:
 *      varbind         The encoded MIB entry.
 *      vblen           Length of the encoded MIB entry.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
print_varbind(unsigned char *varbind, int vblen)
{
    unsigned char *p = varbind;
    int i;
    int tag, len, val;

//    printf("0x%02X (%d)\n", p[0], p[1]);
    p += 2;
    vblen -= 2;

    tag = p[0];
    len = p[1];
    p += 2;
    vblen -= 2;
//    printf("  0x%02X (%d)\n", tag, len);
//    printf("   ");
    printf("%d.%d", *p / 40, *p % 40);
    val = 0;
    for (i = 1; i < len; i++) {
        val = (val * 128) + (p[i] & 127);
        if ((p[i] & 0x80) == 0) {
            printf(".%d", val);
            val = 0;
        }
    }
    printf("\t");
    p += len;

    tag = p[0];
    len = p[1];
    p += 2;
    vblen -= 2;
//    printf("  0x%02X (%d)\n", tag, len);
//    printf("   ");
    if (tag == 4) {
        printf("0x");
        for (i = 0; i < len; i++) {
            printf("%02X", p[i]);
        }
        printf("\n");
    } else {
        val = 0;
        for (i = 0; i < len; i++) {
            val = (val * 128) + (p[i] & 127);
        }
        printf("%d\n", val);
    }

} /* print_varbind() */



/*
 * ---------------------------------------------------------------------------
 *  decode_mibs
 *
 *      Parse a UniFi MIB data file and download the MIB settings to
 *      the UniFi chip.
 * 
 *  Arguments:
 *      mibdata         Pointer to MIB data file content.
 *      miblen          Number of bytes available at mibdata.
 *
 *  Returns:
 *      0 on success, error code on failure.
 * ---------------------------------------------------------------------------
 */
static int
decode_mibs(unsigned char *mibdata, int miblen)
{
    unsigned char *header;
    int version;
    int filelen;
    unsigned char *varbind;

    /* Check mibdata contains at least the file header (10) and one MIB (2) */
    if (miblen < 12) {
        fprintf(stderr, "Error: Invalid MIB file");
        return -EINVAL;
    }

    /* 
     * Read file header.
     */
    header = mibdata;
    mibdata += 10;

    if ((header[0] != 'U') || (header[1] != 'D') ||
        (header[2] != 'M') || (header[3] != 'I'))
    {
        fprintf(stderr, "Error: Bad header in MIB file");
        return -EINVAL;
    }
    version = header[4] + (header[5] << 8);
    filelen = header[6] + (header[7] << 8) +
        (header[8] << 16) + (header[9] << 24);

    /* Check file length from file against given file size */
    if ((filelen + 10) != miblen) {
        fprintf(stderr, "Warning: Length mismatch for MIB file, expected %d, got %d\n",
                      (filelen + 10), miblen);

        /* Truncate filelen if necessary */
        if ((filelen + 10) > miblen) {
            filelen = miblen - 10;
        }
    }

    /* 
     * Read and set MIBs
     */
    while (filelen > 2)
    {
        int datalen;
        unsigned char *vblen;

        /* Read length field */
        vblen = mibdata;
        mibdata += 2;
        filelen -= 2;

        datalen = (vblen[1] << 8) + vblen[0];
        if (datalen > filelen) {
            fprintf(stderr, "Error: End of file reached reading MIB\n");
            return -EINVAL;
        }

        /* Now read the varbind itself */
        varbind = mibdata;
        mibdata += datalen;
        filelen -= datalen;

        /* Check */
        if ((varbind[1] + 2) != datalen) {
            fprintf(stderr, "Error: Malformed MIB entry, bad length (%d != %d)\n",
                        varbind[1] + 2, datalen);
            return -EINVAL;
        }

        /* Useful for debugging MIB files */
        print_varbind(varbind, datalen);

    }

    return 0;

} /* decode_mibs() */



/*
 * ---------------------------------------------------------------------------
 *  process_mib_file
 *
 *      Read, decode and print out the given MIB data file.
 * 
 *  Arguments:
 *      mib_file        Filename of MIB data file to open.
 *
 *  Returns:
 *      0 on success, -1 on error.
 * ---------------------------------------------------------------------------
 */
static int
process_mib_file(const char *mib_file)
{
    FILE *fp;
    unsigned char *mib_data;
    int data_len;
    struct stat st;

    if (stat(mib_file, &st)) {
        perror("Failed to stat img file");
        return -1;
    }

    data_len = st.st_size;

    fp = fopen(mib_file, "r");
    if (!fp) {
        perror("Failed to open MIB file");
        exit(1) ;
    }

    mib_data = (unsigned char *)malloc(data_len);
    if (mib_data == NULL) {
        perror("Failed to allocate memory for firmware");
        fclose(fp);
        return -1;
    }

    /* NB This assumes fread() will successfully read whole file in one call
     */
    if (fread(mib_data, 1, data_len, fp) != data_len) {
        perror("Failed to read MIB file");
        free(mib_data);
        fclose(fp);
        return -1;
    }

    fclose(fp);


    decode_mibs(mib_data, data_len);

    free(mib_data);

    return 0;
} /* process_mib_file() */

