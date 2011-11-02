/** @file csr_rc4.c
 *
 * CSR Crypto support function for Rivest Cipher 4 calculation.
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
 *   Rivest Cipher 4 implementation.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_rc4/csr_rc4.c#1 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "csr_types.h"
#include "csr_rc4.h"

/* MACROS *******************************************************************/
/**
 * @brief
 *
 * @par Description
 */

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/**
 * @brief defines access point global data structure
 *
 * @par Description
 *   defines access point ranking global data structure
 */

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_rc4/csr_rc4.h
 */
/*---------------------------------------------------------------------------*/
void CsrRc4SetKey(CsrRc4Key *key,
         const CsrUint16 len,
         const CsrUint8 *data)
{
    CsrUint16 i;
    CsrUint8 j = 0, swap = 0;

    /* Initialize the state table */
    for(i = 0; i < CSR_RC4_STATE_SIZE; i++)
    {
        key->state[i] = (CsrUint8)i;
    }

    /* Iterate through the complete table */
    for(i = 0; i < CSR_RC4_STATE_SIZE; i++)
    {
        j = (j + key->state[i] + (data[i % len] )) % CSR_RC4_STATE_SIZE;

        /* swap variable */
        swap = key->state[i];
        key->state[i] = key->state[j];
        key->state[j] = swap;
    }

    /* Initialize the table indexes in the key structure */
    key->i = 0;
    key->j = 0;
}

/*
 * Description:
 * See description in csr_rc4/csr_rc4.h
 */
/*---------------------------------------------------------------------------*/
void CsrRc4Discard256 (const CsrUint16 keyLen,
                         const CsrUint8 *key,
                         const CsrUint16 len,
                         const CsrUint8 *inBuf,
                         CsrUint8 *outBuf)
{
    CsrRc4Key keyCtx, *pKeyCtx = &keyCtx;

    CsrRc4SetKey(pKeyCtx, keyLen, key);
    /* Discard the first 256 bytes of keystream */
    CsrRc4(pKeyCtx, 256, NULL, NULL);
    CsrRc4(pKeyCtx, len, inBuf, outBuf);
}


/*
 * Description:
 * See description in csr_rc4/csr_rc4.h
 */
/*---------------------------------------------------------------------------*/
void CsrRc4(CsrRc4Key *key,
         const CsrUint16 len,
         const CsrUint8 *inBuf,
         CsrUint8 *outBuf)
{
    CsrUint16 offset;
    CsrUint8 swap;

/*
     The variable i is incremented by one
     The contents of the ith element of 'State' is then added to j
     The ith and jth elements of 'State' are swapped and their contents
      are added together to form a new value n.
     The nth element of 'State' is then combined with the message byte,
     using a bit by bit exclusive-or operation (XOR), to form the output byte.
*/

    for(offset = 0; offset < len; offset++)
    {
        key->i = (key->i + 1) % CSR_RC4_STATE_SIZE;
        key->j = (key->j + key->state[key->i]) % CSR_RC4_STATE_SIZE;

        /* swap values */
        swap = key->state[key->i];
        key->state[key->i] =  key->state[key->j];
        key->state[key->j] =  swap;

        if(outBuf && inBuf)
        {
            outBuf[offset] = inBuf[offset] ^ key->state[(key->state[key->i] + key->state[key->j] ) % CSR_RC4_STATE_SIZE];
        }
    }
}
