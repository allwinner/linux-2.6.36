/** @file csr_3des_ecb.c
 *
 * Implementation of the 3DES ECB (Electronic Code Book) functions
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   This provides an implementation of 3DES ECB (Electronic Code Book) functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_des/csr_3des_ecb.c#1 $
 *
 ****************************************************************************/
#include "csr_des.h"
#include "csr_types.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/
/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCrypto3DesEcbEncrypt(CsrUint8 *input, CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3, CsrUint8 *output)
{
    /* 3DES EDE (Encrypt-Decrypt-Encrypt) using the three key schedules */
    CsrCryptoDesEcbEncrypt(input,  ks1, output);
    CsrCryptoDesEcbDecrypt(output, ks2, output);
    CsrCryptoDesEcbEncrypt(output, ks3, output);
}

/*
 * Description:
 * See description in csr_des/csr_des.h
 */
/*---------------------------------------------------------------------------*/
void CsrCrypto3DesEcbDecrypt(CsrUint8 *input, CsrKeySchedule *ks1, CsrKeySchedule *ks2, CsrKeySchedule *ks3, CsrUint8 *output)
{
    /* 3DES EDE (Encrypt-Decrypt-Encrypt) using the three key schedules */
    /* For decryption, the sequence is Decrypt-Encrypt-Decrypt */
    CsrCryptoDesEcbDecrypt(input,  ks3, output);
    CsrCryptoDesEcbEncrypt(output, ks2, output);
    CsrCryptoDesEcbDecrypt(output, ks1, output);
}
