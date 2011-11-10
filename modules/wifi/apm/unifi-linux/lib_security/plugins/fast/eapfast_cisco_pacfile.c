/** @file eapfast_cisco_pacfile.c
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
 *   A function to load an out-band Cisco PAC file.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/eapfast_cisco_pacfile.c#2 $
 *
 ****************************************************************************/

#include "eapfast_cisco_pacfile.h"
#include "csr_types.h"
#include "sme_trace/sme_trace.h"
#include "csr_crypto.h"

/* Definitions for the PAC file format from Cisco CCXv3 specification */
#define EAPFAST_PAC_FILE_MAGIC_LEN 4
#define EAPFAST_PAC_FILE_VERSION       1
#define EAPFAST_PAC_CYPHER_IV_LENGTH   16
#define EAPFAST_CHECK_SUM_LEN          16
#define MAX_PAC_DATA 500/* arbitary size*/

const CsrUint8 pacfile_defPasswd[] =
{
    0xCC,0xB8,0xE0,0x49,0x05,0x4A,0xA3,0xBB,0xE7,0xEB,0x9C,0xD2,0x98,0x13,0xA7,0x8B,0x00
};
const CsrUint8 eapfastPacFileMagicBigEndian[EAPFAST_PAC_FILE_MAGIC_LEN] = {0x00, 0x05, 0x04, 0x29};
const CsrUint8 eapfastPacFileMagicLittleEndian[EAPFAST_PAC_FILE_MAGIC_LEN] = {0x29, 0x04, 0x05, 0x00};

typedef struct
{
    CsrUint8 version;
    CsrUint8 defpasswd;     /* boolean 1=default password */
    CsrUint8 magicNum[EAPFAST_PAC_FILE_MAGIC_LEN];
    CsrUint8 cypher[2];     /* 0x0001 = AES-CBC-128 bit */
    CsrUint8 iv[EAPFAST_PAC_CYPHER_IV_LENGTH];
    CsrUint8 integritymac[EAPFAST_CHECK_SUM_LEN];
    CsrUint8 cyphermac[EAPFAST_CHECK_SUM_LEN];
    CsrUint8 blob[1];
} eapfast_pac_file_format;

/**
 * @brief Parse a PAC TLV record and put its contents into eapfast_context.
 *
 * @param[in]   pac_tlv : data buffer with the PAC TLV in it.
 * @param[in]   size : size of the PAC TLV buffer
 *
 * @return TRUE=success, FALSE=failure
 */
static CsrBool parse_PAC_TLV(FastContext *fastContext, CsrUint8 *pac_tlv, CsrInt32 size)
{
    CsrInt32 data_len;
    CsrUint8 *scan = pac_tlv;

    /* Do some checking of known values */
    if ((scan[0] != 0x80) ||
        (scan[1] != 0x0b) ||
        (size < 4) )
    {
        sme_trace_error((TR_SECURITY_LIB, "parse_PAC_TLV() :: PAC is invalid."));
        return FALSE;
    }

    scan += 2;

    data_len = (scan[0]<<8) + scan[1];
    scan += 2;

    /* Process the PAC TLV field by field */
    while(scan < pac_tlv + data_len - 4/*min size worth processing*/)
    {
        CsrUint16 field_type = (scan[0]<<8) + scan[1];
        CsrUint16 field_length = (scan[2]<<8) + scan[3];
        switch(field_type)
        {
        case 1/*PAC-Key*/:
            if (field_length != 32)
            {
                sme_trace_error((TR_SECURITY_LIB, "parse_PAC_TLV() :: PAC-Key is the wrong length."));
                return FALSE;
            }
            scan += 4;
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_PAC_TLV() :: PAC_key", scan, 32));
            CsrMemCpy(fastContext->tls.PAC_key, scan, 32);
            break;
        case 2/*PAC-Opaque*/:
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_PAC_TLV() :: PAC_opaque", scan, field_length+4));
            CsrMemCpy(fastContext->PAC_opaque,
                   scan,
                   field_length+4/*field type and length fields considered to be part of the PAC-Opaque*/);
            fastContext->PAC_opaque_length = field_length+4;
            scan += 4;
            break;
        default:
            /* not interested in the others */
            break;
        }
        scan += field_length;
    }

    return TRUE;
}

/*
 * Description:
 * See description in eapfast_cisco_pacfile.h
 */
/*---------------------------------------------------------------------------*/
CsrBool decode_pac_file(FastContext *fastContext, char *password)
{
    CsrUint8 *decyphered_pac_data;
    CsrUint16 pac_length = 0;
    eapfast_pac_file_format *pac;
    CsrUint8 old_mac[EAPFAST_CHECK_SUM_LEN];
    CsrUint8 new_mac[CSR_SHA1_DIGEST_LENGTH];
    CsrUint16 blob_length;
    CsrUint8 key[CSR_SHA1_DIGEST_LENGTH];
    CsrBool ret_val;

    sme_trace_info((TR_SECURITY_LIB, "decode_pac_file() :: fast_pac_length = %d", fastContext->context->setupData.fast_pac_length));

    if(fastContext->context->setupData.fast_pac_length == 0)
    {
        return FALSE;
    }

    pac = (eapfast_pac_file_format *)(fastContext->context->setupData.fast_pac);

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "decode_pac_file() :: PAC", fastContext->context->setupData.fast_pac, fastContext->context->setupData.fast_pac_length));

    if ((decyphered_pac_data = CsrPmalloc(MAX_PAC_DATA)) == NULL)
    {
        sme_trace_error((TR_SECURITY_LIB, "decode_pac_file() :: memory allocation error."));
        return FALSE;
    }

    pac_length = (CsrUint16)(fastContext->context->setupData.fast_pac_length);

    /* Sanity check the header. */
    if ((pac->version != EAPFAST_PAC_FILE_VERSION)
        || (pac->cypher[0] != 0)
        || (pac->cypher[1] != 1)
        /* accept both endian magic numbers, although Cisco APs seem to put out little endian */
        || ( (CsrMemCmp(pac->magicNum,
                     eapfastPacFileMagicLittleEndian,
                     EAPFAST_PAC_FILE_MAGIC_LEN) != 0) &&
             (CsrMemCmp(pac->magicNum,
                     eapfastPacFileMagicBigEndian,
                     EAPFAST_PAC_FILE_MAGIC_LEN) != 0) )
        || (pac_length < sizeof(eapfast_pac_file_format) + 8/* Minimum PAC TLV size */)
       )
    {
        sme_trace_error((TR_SECURITY_LIB, "decode_pac_file() :: Bad PAC file header."));
        CsrPfree(decyphered_pac_data);
        return FALSE;
    }

    /* Check the file integrity MAC */
    CsrMemCpy(old_mac, pac->integritymac, 16);
    CsrMemSet(pac->integritymac, 0, 16);
    CsrCryptoCallSha1(fastContext->context->cryptoContext, NULL,
                      fastContext->context->setupData.fast_pac, pac_length, new_mac);
    if (CsrMemCmp(old_mac, new_mac, 16) != 0)
    {
        sme_trace_error((TR_SECURITY_LIB, "decode_pac_file() :: File Integrity MAC failure."));
        CsrPfree(decyphered_pac_data);
        return FALSE;
    }

    /* Calculate the encryption key - HMAC-SHA1(password, "ENC") */
    if ((pac->defpasswd) || (password==NULL))
    {
        CsrCryptoCallHmacSha1(fastContext->context->cryptoContext, NULL,
                pacfile_defPasswd, 17, (CsrUint8 *)"ENC", 3, key);
    }
    else
    {
        CsrCryptoCallHmacSha1(fastContext->context->cryptoContext, NULL,
                (CsrUint8 *)password, CsrStrLen(password), (CsrUint8 *)"ENC", 3, key);
    }

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "decode_pac_file() :: AES key", key, CSR_SHA1_DIGEST_LENGTH));
    {
        CsrUint32 outLength;

        blob_length = pac_length - (sizeof(eapfast_pac_file_format) - 1);
        CsrCryptoAes128CbcDecrypt(key, pac->iv,
                pac->blob, blob_length, decyphered_pac_data, &outLength, PAD_MODE_TLS);
    }

    /* Drop the padding bytes from the blob_length as they are not part of the encryption MAC computation*/
    blob_length -= (CsrUint16)(decyphered_pac_data[blob_length - 1]);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Decrypted blob", decyphered_pac_data, blob_length));

    /* Check the encryption MAC */
    CsrMemSet(new_mac, 0, CSR_SHA1_DIGEST_LENGTH);
    CsrCryptoCallSha1(fastContext->context->cryptoContext, NULL,
                      decyphered_pac_data, blob_length, new_mac);

#if (EAPFAST_CHECK_SUM_LEN > CSR_SHA1_DIGEST_LENGTH)
#error Something gone wrong with the EAP-FAST constants!
#endif

    if (CsrMemCmp(pac->cyphermac, new_mac, EAPFAST_CHECK_SUM_LEN) != 0)
    {
        sme_trace_error((TR_SECURITY_LIB, "decode_pac_file() :: Encryption MAC failure."));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Supplied MAC", pac->cyphermac, EAPFAST_CHECK_SUM_LEN));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Computed MAC", new_mac, EAPFAST_CHECK_SUM_LEN));
        CsrPfree(decyphered_pac_data);
        return FALSE;
    }
    /* decyphered_pac_data should now contain a complete PAC TLV */
    ret_val = parse_PAC_TLV(fastContext, decyphered_pac_data, blob_length);

    CsrPfree(decyphered_pac_data);

    return ret_val;
}

CsrBool encode_pac_file(FastContext *fastContext)
{
    eapfast_pac_file_format *pac;
    CsrUint8 *decyphered_pac_data;
    CsrUint8 digest[CSR_SHA1_DIGEST_LENGTH];
    CsrUint8 *scan;
    CsrUint8 *data_len;
    CsrUint16 length;
    CsrInt32 rounded_length;
    CsrUint8 padding;
    CsrUint8 key[CSR_SHA1_DIGEST_LENGTH];
    CsrUint8 iv[EAPFAST_PAC_CYPHER_IV_LENGTH];
    CsrInt32 i;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: PAC_key", fastContext->tls.PAC_key, 32));
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: PAC_opaque", fastContext->PAC_opaque, fastContext->PAC_opaque_length));

    pac = (eapfast_pac_file_format *)CsrPmalloc(CSR_WIFI_SECURITY_MAX_PAC_LENGTH);
    fastContext->context->setupData.fast_pac = (CsrUint8 *)pac;

    pac->version = EAPFAST_PAC_FILE_VERSION;
    pac->defpasswd = 1; /* Using default password */
    CsrMemCpy(pac->magicNum, eapfastPacFileMagicLittleEndian, EAPFAST_PAC_FILE_MAGIC_LEN);
    pac->cypher[0] = 0; /* AES-CBC-128 */
    pac->cypher[1] = 1;
    if ((decyphered_pac_data = CsrPmalloc(MAX_PAC_DATA)) == NULL)
    {
        sme_trace_error((TR_SECURITY_LIB, "encode_pac_file() :: memory allocation error."));
        return FALSE;
    }

    scan = decyphered_pac_data;
    *scan++ = 0x80; /* Set known values */
    *scan++ = 0x0b;

    data_len = scan; /* Keep pointer to data_len to fill in later */
    scan += 2;

    *scan++ = 0x00;
    *scan++ = 0x01; /* PAC Key type */
    *scan++ = 0x00;
    *scan++ = 0x20; /* PAC Key length = 32 */
    CsrMemCpy(scan, fastContext->tls.PAC_key, 32);
    scan += 32;

    *scan++ = 0x00;
    *scan++ = 0x02; /* PAC Opaque type */
    *scan++ = (fastContext->PAC_opaque_length >> 8) & 0xff;
    *scan++ = fastContext->PAC_opaque_length & 0xff;
    CsrMemCpy(scan, fastContext->PAC_opaque, fastContext->PAC_opaque_length);
    scan += fastContext->PAC_opaque_length;

    length = (CsrUint16)(scan - decyphered_pac_data);
    *data_len++ = (length >> 8) & 0xff;
    *data_len++ = length & 0xff;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: decyphered_pac_data", decyphered_pac_data, length));

    /* Compute the encryption MAC */
    CsrCryptoCallSha1(fastContext->context->cryptoContext, NULL,
                      decyphered_pac_data, length, digest);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: Encryption MAC", digest, EAPFAST_CHECK_SUM_LEN));
    CsrMemCpy(pac->cyphermac, digest, EAPFAST_CHECK_SUM_LEN);

    sme_trace_info((TR_SECURITY_LIB, "encode_pac_file() :: Decyphered length = %d", length));

    /* Need to adjust the length to be a multiple of 128 bits before encoding */
    rounded_length = ((length + 16) / 16) * 16;
    sme_trace_info((TR_SECURITY_LIB, "encode_pac_file() :: Rounded up length = %d", rounded_length));

    /* Work out how many padding bytes are required */
    padding = (CsrUint8)(rounded_length - length);
    if (padding == 0) padding = 16;

    /* Append the padding bytes */
    scan = decyphered_pac_data + length;
    for (i = 0; i < padding; i++)
    {
        *scan++ = padding;
    }
    length += padding;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: Padded decyphered_pac_data", decyphered_pac_data, length));

    CsrCryptoCallHmacSha1(fastContext->context->cryptoContext, NULL,
                          pacfile_defPasswd, 17, (CsrUint8 *)"ENC", 3, key);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: AES key", key, CSR_SHA1_DIGEST_LENGTH));
    /* AES CBC encode the decrypted PAC data */

    {
        CsrUint32 outLength;

        /* FIXME - the IV should be initialized to a random value */
        CsrMemSet(pac->iv, 0, EAPFAST_PAC_CYPHER_IV_LENGTH);
        /* Make a copy of the IV as the AES encrypt function will not preserve it */
        CsrMemCpy(iv, pac->iv, EAPFAST_PAC_CYPHER_IV_LENGTH);

        CsrCryptoAes128CbcEncrypt(key, iv,
                decyphered_pac_data, length, pac->blob, &outLength, PAD_MODE_TLS);
    }

    /* Adjust to the total length for the integritymac calculation */
    length += (sizeof(eapfast_pac_file_format) - 1);

    /* Zero the MAC before computing */
    CsrMemSet(pac->integritymac, 0, EAPFAST_CHECK_SUM_LEN);
    CsrCryptoCallSha1(fastContext->context->cryptoContext, NULL,
                      fastContext->context->setupData.fast_pac, length, digest);
    CsrMemCpy(pac->integritymac, digest, EAPFAST_CHECK_SUM_LEN);

    fastContext->context->setupData.fast_pac_length = length;

    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "encode_pac_file() :: PAC", fastContext->context->setupData.fast_pac, length));

    return TRUE;
}
