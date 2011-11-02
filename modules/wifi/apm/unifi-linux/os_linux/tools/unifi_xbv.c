/*
 * ---------------------------------------------------------------------------
 * FILE: unifi_xbv.c
 *
 * PURPOSE:
 *      Test entry point around XBV parser in lib_hip, for testing the
 *      XBV parsing and conversion routines.
 *      
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

/* xbv.c must be built with -DCSR_WIFI_XBV_TEST and linked with this file */

#include "unifi_xbv.h"
#include "../../lib_hip/xbv.h"

/* Struct to represent a buffer for reading firmware file */
typedef struct {
    void *dlpriv;
    int ioffset;
    fwreadfn_t iread;
} ct_t;

struct img {
    unsigned char *imgptr;
    int imglen;
};

/*
 * Callback function to read firmware from file.
 */
static CsrInt32
rfn(void* ospriv, void *dlpriv, CsrUint32 offset, void *buf, CsrUint32 len)
{
    struct img *img = dlpriv;


    if (offset == img->imglen) {
        /* at end of file */
        return 0;
    }

    if ((offset+len) > img->imglen) {
        /* attempt to read past end of file */
        return -1;
    }

    memcpy(buf, img->imgptr+offset, len);

    return len;
}

/*
 * Stubs for functions normally provided by the driver
 */
void
*unifi_malloc(void *priv, size_t size)
{
    return malloc(size);
}

void
unifi_free(void *priv, void *buf)
{
    free(buf);
}

void
unifi_error(void* ospriv, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void
unifi_trace(void* ospriv, int lvl, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/*
 * main()
 *      unifi_xbv entry point.
 * 
 *      Usage: argv[1] specifies XBV file name for processing - 
 *             FWDL format files will convert to PTCH format, and
 *             be written as patchconv.xbv 
 */
int
main(int argc, char *argv[])
{
    char *filename = "test.xbv";
    FILE *fp;
    unsigned char *buf;
    int i, j, n;
    struct img img;
    xbv1_t fwinfo;
#define MAX_SLUT_ENTRIES 32
    symbol_t slut[MAX_SLUT_ENTRIES];
    int num_slut_entries;
    struct card card;

    if (argc > 1) {
        filename = argv[1];
    }

    buf = malloc(256*1024);

    fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open test.xbv");
        exit(errno);
    }
    n = fread(buf, 1, 256*1024, fp);
    if (n < 0) {
        perror("Failed to read test.xbv");
        exit(errno);
    }

    memset(&fwinfo, 0, sizeof(fwinfo));

    card.build_id = 611;    /* ID of "on chip" firmware */
        
    img.imgptr = buf;
    img.imglen = n;

    n = xbv1_parse(&card, rfn, &img, &fwinfo);
    if (n) {
        printf("Parse failed\n");
        exit(1);
    }

    switch (fwinfo.mode)
    {
    case xbv_firmware:
        printf("XBV File contains a firmware image.\n");
        num_slut_entries = xbv1_read_slut(&card, rfn, &img, &fwinfo, slut, MAX_SLUT_ENTRIES);

        if (fwinfo.vers.num_vand != 0) {
            printf("    VERS: for %lu\n", fwinfo.vers.num_vand);

            for (i = 0; i < fwinfo.vers.num_vand; i++) {
                printf("      VAND: %lu for %lu\n",
                       fwinfo.vand[i].first, fwinfo.vand[i].count);

                for (j = 0; j < fwinfo.vand[i].count; j++) {
                    int k = fwinfo.vand[i].first + j;
                    printf("        VMEQ: *0x%08lx & 0x%04X == 0x%04X\n",
                           fwinfo.vmeq[k].addr, fwinfo.vmeq[k].mask, fwinfo.vmeq[k].value);
                }
            }
        }

        printf("SLUT addr 0x%lX\n", fwinfo.slut_addr);
        printf("%d SLUT entr%s:\n", num_slut_entries, (num_slut_entries == 1) ? "y" : "ies");
        for (i = 0; i < num_slut_entries; i++) {
            printf("    SLUT: 0x%04X, 0x%08lX\n",
                   slut[i].id, slut[i].obj);
        }

        printf("%d F/W segment%s\n", fwinfo.num_fwdl, (fwinfo.num_fwdl == 1) ? "" : "s");
        for (i = 0; i < fwinfo.num_fwdl; i++) {
            printf("    FWDL: dest 0x%08lx, len %6ld, file offset 0x%lX\n",
                   fwinfo.fwdl[i].dl_addr, fwinfo.fwdl[i].dl_size,
                   fwinfo.fwdl[i].dl_offset);
        }

        if (fwinfo.fwov.dl_size > 0) {
            printf("Overlay segment\n");
            printf("    FWOV: len %6ld, file offset 0x%lX\n",
                   fwinfo.fwov.dl_size, fwinfo.fwov.dl_offset);
        } else {
            printf("No overlay segment\n");
        }
        
        /* Test conversion-to-patch support */
        printf("\nTesting conversion of XBV:FWDL to XBV:PTCH...\n");
        {
            FILE *fdo;
            CsrUint32 dl_len;
            void *dl_data = xbv_to_patch(&card, (fwreadfn_t)rfn, &img, &fwinfo, &dl_len);            
            if (dl_data) {
                printf("Apparent success converting to patch format\n");
                fdo = fopen("patchconv.xbv", "wb");
                if (fdo) {
                    int n = fwrite(dl_data, 1, dl_len, fdo);
                    printf("Wrote %d bytes to patchconv.xbv\n\n", n);
                    fclose(fdo);
                }
                img.imgptr = dl_data;
                img.imglen = dl_len;
                n = xbv1_parse(&card, rfn, &img, &fwinfo);
                if (n) {
                    printf("Parse of patch conversion failed\n");
                }
                
                free(dl_data);
            } else {
                printf("Conversion to patch format failed\n");
            }
        }
        break;

    case xbv_patch:
        printf("XBV File contains a patch set.\n");

        printf("FWID build id %lu\n", fwinfo.build_id);

        printf("%d Patch segment%s\n", fwinfo.num_ptdl, (fwinfo.num_ptdl == 1) ? "" : "s");
        for (i = 0; i < fwinfo.num_ptdl; i++) {
            CsrUint8 csum[2];
            csum[0] = buf[fwinfo.ptdl[i].dl_offset + fwinfo.ptdl[i].dl_size-1];
            csum[1] = buf[fwinfo.ptdl[i].dl_offset + fwinfo.ptdl[i].dl_size-2];
            printf("    PTDL: len %5ld, file offset 0x%lX, csum 0x%02x%02x\n",
                   fwinfo.ptdl[i].dl_size, fwinfo.ptdl[i].dl_offset, csum[1], csum[0]);
        }
        break;

    default:
        printf("XBV File seems to be corrupt.\n");
        break;
    }
    return 0;
}
