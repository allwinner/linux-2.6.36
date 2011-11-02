/** @file csr_dh.c
 *
 * Support function for Diffie-Hellman calculation.
 * Diffie-Hellman caculations are used for public/private key generation
 * and pre-master key computation.
 * Reference:
 *      New Directions in Cryptography, W. Diffie and M. E. Hellman,
 *      IEEE Transactions on Information Theory, vol. IT-22, Nov. 1976, pp: 644-654.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   Diffie-Hellman computation implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_dh/csr_dh.c#2 $
 *
 ****************************************************************************/

#include "csr_dh.h"
#include "csr_bn/csr_bn.h"
#include "sme_trace/sme_trace.h"

CsrBool CsrCryptoDhGenerateKeys
(CsrUint32 g_base, CsrUint8 *prime, CsrUint32 size, CsrUint8 *private_key, CsrUint8 *public_key, CsrBool do_private)
{
    CsrBignumCtx *ctx = CsrCryptoBnCtxNew();
    CsrBignum *g;
    CsrBignum *p;
    CsrBignum *priv;
    CsrBignum *pub;
    CsrUint32 i;
    CsrBool result = TRUE;

    if (do_private)
    {
        /* Use this for the private key for now. TODO: fill this with random data */
        for (i=0; i<size; i++)
            private_key[i] = (CsrUint8)i;
    }

    g = CsrCryptoBnNew();
    p = CsrCryptoBnNew();
    priv = CsrCryptoBnNew();
    pub  = CsrCryptoBnNew();

    if (CsrCryptoBnBinToBn(prime, size, p) == NULL)
        result =  FALSE;
    if (CsrCryptoBnBinToBn(private_key, size, priv) == NULL)
        result =  FALSE;
    if (CsrCryptoBnSetWord(g, g_base) == 0)
        result =  FALSE;

    if (result)
    {
/* Standard DH key pair calculation: pub = g^priv mod p */
        if (!(CsrCryptoBnModExp(pub, g, priv, p, ctx)))
        {
            sme_trace_error((TR_CRYPTO_LIB, "Big number error."));
            result = FALSE;
        }
        else
        {
            sme_trace_info((TR_CRYPTO_LIB, "%d, %d.", CsrCryptoBnNumBytes(pub), CsrCryptoBnNumBytes(priv)));

            if (CsrCryptoBnNumBytes(pub) != size)
            {
                sme_trace_error((TR_CRYPTO_LIB, "Big number size error."));
                result = FALSE;
            }
            else
            {
                (void)CsrCryptoBnBnToBin(pub, public_key);
            }
        }
    }

    CsrCryptoBnFree(g);
    CsrCryptoBnFree(p);
    CsrCryptoBnFree(priv);
    CsrCryptoBnFree(pub);
    CsrCryptoBnCtxFree(ctx);
    return result;
}

void CsrCryptoDhCalculateSharedSecret
(CsrUint8 *result, CsrUint8 *others_pub, CsrUint8 *out_priv, CsrUint8 *prime, CsrUint32 size)
{
    CsrBignumCtx *ctx = CsrCryptoBnCtxNew();
    CsrBignum *r;
    CsrBignum *a;
    CsrBignum *p;
    CsrBignum *m;
    r = CsrCryptoBnNew();
    a = CsrCryptoBnNew();
    p = CsrCryptoBnNew();
    m = CsrCryptoBnNew();

    (void)CsrCryptoBnBinToBn(others_pub, size, a);
    (void)CsrCryptoBnBinToBn(out_priv, size, p);
    (void)CsrCryptoBnBinToBn(prime, size, m);

/* r = a^p mod m */
    if (!(CsrCryptoBnModExp(r, a, p, m, ctx)))
        sme_trace_error((TR_CRYPTO_LIB, "Big number error."));
    else
    {
        sme_trace_info((TR_CRYPTO_LIB, "%d.", CsrCryptoBnNumBytes(r)));

        if (CsrCryptoBnNumBytes(r) != size)
            sme_trace_error((TR_CRYPTO_LIB, "Big number size error."));
        else
            (void)CsrCryptoBnBnToBin(r, result);
    }

    CsrCryptoBnFree(r);
    CsrCryptoBnFree(a);
    CsrCryptoBnFree(p);
    CsrCryptoBnFree(m);

    CsrCryptoBnCtxFree(ctx);
}


