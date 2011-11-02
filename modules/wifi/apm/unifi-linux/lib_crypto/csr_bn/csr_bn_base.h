/** @file csr_bn_base.h
 *
 * CSR Crypto support function for BigNum basic operations.
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
 *   BigNum basic operations.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_bn/csr_bn_base.h#1 $
 *
 ****************************************************************************/
#ifndef CSR_BN_BASE_H
#define CSR_BN_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"
#include "csr_bn_public.h"

extern CsrBignumCtx* CsrCryptoBnCtxNew(void);
extern CsrUint32 CsrCryptoBnBnToBin(const CsrBignum* a, CsrUint8* to);
extern CsrUint32 CsrCryptoBnNumBits(const CsrBignum* a);
extern CsrUint32 CsrCryptoBnNumOctets(const CsrBignum* a);
extern void CsrCryptoBnFree(CsrBignum* a);
extern void CsrCryptoBnCtxFree(CsrBignumCtx* bnContext);
extern CsrBignum* CsrCryptoBnBinToBn(const CsrUint8* s, const CsrUint32 len, CsrBignum* result);
extern CsrBignum* CsrCryptoBnHexToBn(const char* s, CsrBignum* result);
extern CsrBignum* CsrCryptoBnNew(void);
extern CsrBignum* CsrCryptoBnOneNew(void);
extern CsrUint32 CsrCryptoBnSetWord(CsrBignum* a, const CsrUint32 w);
extern CsrInt32 CsrCryptoBnUnsignedCompare(const CsrBignum* a, const CsrBignum* b);
extern void CsrCryptoBnSet(CsrBignum* a, const CsrBignum* b);
extern void CsrCryptoBnShiftLeft1(CsrBignum* a);
extern void CsrCryptoBnShiftRight1(CsrBignum* a);
extern void CsrCryptoBnUnsignedSubtract(CsrBignum* result, const CsrBignum* a, const CsrBignum* b);
extern void CsrCryptoBnUnsignedAddMod(CsrBignum* result, const CsrBignum* a, const CsrBignum* b, const CsrBignum* n);
extern void CsrCryptoBnUnsignedSubMod(CsrBignum* result, const CsrBignum* a, const CsrBignum* b, const CsrBignum* n);
extern CsrInt32 CsrCryptoBnUnsignedCompareWord(const CsrBignum* a, const CsrUint32 w);
extern CsrBool CsrCryptoBnIsEven(const CsrBignum* a);
extern void CsrCryptoBnAdd(CsrBignum* result, const CsrBignum* a, const CsrBignum* b);
extern void CsrCryptoBnSubtract(CsrBignum* result, const CsrBignum* a, const CsrBignum* b);

#ifdef __cplusplus
}
#endif

#endif /*CSR_BN_BASE_H*/

