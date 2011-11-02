/*
 * ---------------------------------------------------------------------------
 * FILE: xbv.h
 *
 * PURPOSE:
 *      Definitions and declarations for code to read XBV files - the UniFi
 *      firmware download file format.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#ifndef __XBV_H__
#define __XBV_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSR_WIFI_XBV_TEST
/* Driver includes */
#include "driver/unifi.h"
#endif


struct VMEQ {
    CsrUint32 addr;
    CsrUint16 mask;
    CsrUint16 value;
};

struct VAND {
    CsrUint32 first;
    CsrUint32 count;
};

struct VERS {
    CsrUint32 num_vand;
};

struct FWDL {
    CsrUint32 dl_addr;
    CsrUint32 dl_size;
    CsrUint32 dl_offset;
};

struct FWOV {
    CsrUint32 dl_size;
    CsrUint32 dl_offset;
};

struct PTDL {
    CsrUint32 dl_size;
    CsrUint32 dl_offset;
};

#define MAX_VMEQ 64
#define MAX_VAND 64
#define MAX_FWDL 256
#define MAX_PTDL 256

/* An XBV1 file can either contain firmware or patches (at the
 * moment).  The 'mode' member of the xbv1_t structure tells us which
 * one is the case. */
typedef enum
{
    xbv_unknown,
    xbv_firmware,
    xbv_patch
} xbv_mode;

typedef struct {

    xbv_mode mode;

    /* Parts of a Firmware XBV1 */

    struct VMEQ vmeq[MAX_VMEQ];
    CsrUint32 num_vmeq;
    struct VAND vand[MAX_VAND];
    struct VERS vers;

    CsrUint32 slut_addr;

    /* F/W download image, possibly more than one part */
    struct FWDL fwdl[MAX_FWDL];
    CsrInt16 num_fwdl;

    /* F/W overlay image, add r not used */
    struct FWOV fwov;

    /* Parts of a Patch XBV1 */

    CsrUint32 build_id;

    struct PTDL ptdl[MAX_PTDL];
    CsrInt16 num_ptdl;

}  xbv1_t;


typedef CsrInt32 (*fwreadfn_t)(void *ospriv, void *dlpriv, CsrUint32 offset, void *buf, CsrUint32 len);

CsrInt32 xbv1_parse(card_t *card, fwreadfn_t readfn, void *dlpriv, xbv1_t *fwinfo);
CsrInt32 xbv1_read_slut(card_t *card, fwreadfn_t readfn, void *dlpriv, xbv1_t *fwinfo,
                        symbol_t *slut, CsrUint32 slut_len);
void *xbv_to_patch(card_t *card, fwreadfn_t readfn, const void *fw_buf, const xbv1_t *fwinfo,
                   CsrUint32 *size);

#ifdef __cplusplus
}
#endif

#endif /* __XBV_H__ */
