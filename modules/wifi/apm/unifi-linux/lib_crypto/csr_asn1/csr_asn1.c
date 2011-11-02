/** @file csr_asn1.c
 *
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
 *   see below
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_asn1/csr_asn1.c#1 $
 *
 ****************************************************************************/


#include "csr_asn1/csr_asn1.h"
#include "csr_types.h"
#include "sme_trace/sme_pbc.h"

/* PRIVATE CONSTANTS ********************************************************/

/* PRIVATE MACROS ***********************************************************/
#define ASN1_CLASS_UNIVERSAL 0x00
#define ASN1_CLASS_CONTEXT_SPECIFIC 0x80

#define ASN1_CONSTRUCTED 0x20

#define UNIVERSAL_INTEGER 2
#define UNIVERSAL_BIT_STRING 3
#define UNIVERSAL_OCTET_STRING 4
#define UNIVERSAL_NULL 5
#define UNIVERSAL_OBJECT_IDENTIFIER 6
#define UNIVERSAL_SEQUENCE 16

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

static CsrUint32 getAsn1Length(const CsrUint8 **scan)
{
    CsrUint32 length = 0;
    CsrUint32 size;
    CsrUint32 i;

    if(**scan & 0x80)
    {
        /* Multi octet length encoding */
        size = *(*scan)++ & 0x7f;
        /* Only handle up to three octet length encodings */
        if(size > 3) return 0;
        for(i = 0; i < size; i++)
        {
            length <<= 8;
            length |= *(*scan)++;
        }
    }
    else
    {
        length = *(*scan)++ & 0x7f;
    }

    return length;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in csr_asn1/csr_asn1.h
 */
/*---------------------------------------------------------------------------*/

CsrUint32 CsrCryptoAsn1GetRsaPublicModulus(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;
    /* DER encoding of { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 1} for RSA Encryption */
    CsrUint8 rsaEncryption[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01};

    do
    {
        /* Certificate */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* tbsCertificate TBSCertificate */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this TBSCertificate SEQUENCE */

        /* version Version */
        if(*(*scan)++ != (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* serialNumber CertificateSerialNumber */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* signature AlgorithmIdentifier */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* issuer Name */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* validity Validity */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* subject Name */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* subjectPublicKeyInfo SubjectPublicKeyInfo */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this SubjectPublicKeyInfo SEQUENCE */

        /* algorithm AlgorithmIdentifier */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Walk into AlgorithmIdentifier to check algorithm Object Identifier */

        /* algorithm OBJECT IDENTIFIER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_OBJECT_IDENTIFIER)) break;
        length = getAsn1Length(scan);

        /* Check that this Object Identifier corresponds to RSA encryption for the Public Key */
        if(CsrMemCmp(rsaEncryption, *scan, length)) break;
        *scan += length;

        /* parameters NULL */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_NULL)) break;
        length = getAsn1Length(scan);
        /* length is zero for NULL type, so no need to update scan pointer */

        /* subjectPublicKey BIT STRING */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_BIT_STRING)) break;
        length = getAsn1Length(scan);

        /* Skip the BIT STRING unused bits - this should always be zero */
        if(*(*scan)++ != 0x00) break;

        /* RSAPublicKey SEQUENCE */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* modulus INTEGER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);

        /* If this point is reached, there were no parsing errors encountered */
        result = length;
    }
    while(0);

    return result;
}

CsrUint32 CsrCryptoAsn1GetRsaPublicExponent(const CsrUint8 **scan)
{
    CsrUint32 result = 0;

    do
    {
        /* publicExponent INTEGER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        result = getAsn1Length(scan);
    }
    while(0);

    return result;
}

CsrUint32 CsrCryptoAsn1GetRsaPrivateModulus(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;

    do
    {
        /* RSAPrivateKey */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* version Version */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* modulus INTEGER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);

        /* If this point is reached, there were no parsing errors encountered */
        result = length;
    }
    while(0);

    return result;
}

CsrUint32 CsrCryptoAsn1GetRsaPrivateExponent(const unsigned char **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;

    do
    {
        /* publicExponent INTEGER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* privateExponent INTEGER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        result = getAsn1Length(scan);
    }
    while(0);

    return result;
}

/*
 * Description:
 * See description in csr_asn1/csr_asn1.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoAsn1GetEcSubjectName(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;
    CsrUint8 *temp;

    do
    {
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this Certificate SEQUENCE */

        /* version Version */
        if(*(*scan)++ != (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* serialNumber CertificateSerialNumber */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* signature AlgorithmIdentifier */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* issuer Name */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* validity Validity */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* subject Name */
        temp = (CsrUint8*)*scan;
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* If this point is reached, there were no parsing errors encountered */
        result = length + (CsrUint32)(*scan - temp);
        *scan = temp;
    }
    while(0);

    return result;
}

/*
 * Description:
 * See description in csr_asn1/csr_asn1.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoAsn1GetEcIssuer(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;
    CsrUint8 *temp;

    do
    {
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this Certificate SEQUENCE */

        /* version Version */
        if(*(*scan)++ != (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* serialNumber CertificateSerialNumber */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* signature AlgorithmIdentifier */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* issuer Name */
        temp = (CsrUint8*)*scan;
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* If this point is reached, there were no parsing errors encountered */
        result = length + (CsrUint32)(*scan - temp);
        *scan = temp;

    }
    while(0);

    return result;
}

/*
 * Description:
 * See description in csr_asn1/csr_asn1.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoAsn1GetEcSerialNumber(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;
    CsrUint8 *temp;

    do
    {
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this Certificate SEQUENCE */

        /* version Version */
        if(*(*scan)++ != (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* serialNumber CertificateSerialNumber */
        temp = (CsrUint8*) *scan;
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);

        /* If this point is reached, there were no parsing errors encountered */
        result = length + (CsrUint32)(*scan - temp);
        *scan = temp;
    }
    while(0);

    return result;
}

/*
 * Description:
 * See description in csr_asn1/csr_asn1.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoAsn1GetEcPublicKey(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length;
    /* DER encoding of 1.2.840.10045.2.1 Elliptic Curve cryptography */
    CsrUint8 ec[] = {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01};
    /* DER encoding of 1.2.156.11235.1.1.2.1 WAPI Elliptic Curve parameters */
    CsrUint8 ecWapiParams[] = {0x2a, 0x81, 0x1c, 0xd7, 0x63, 0x01, 0x01, 0x02, 0x01};

    do
    {
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this Certificate SEQUENCE */

        /* version Version */
        if(*(*scan)++ != (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* serialNumber CertificateSerialNumber */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* signature AlgorithmIdentifier */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* issuer Name */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* validity Validity */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* subject Name */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);
        *scan += length;

        /* subjectPublicKeyInfo SubjectPublicKeyInfo */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this SubjectPublicKeyInfo SEQUENCE */

        /* algorithm AlgorithmIdentifier */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Walk into AlgorithmIdentifier to check algorithm Object Identifier */

        /* algorithm OBJECT IDENTIFIER */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_OBJECT_IDENTIFIER)) break;
        length = getAsn1Length(scan);
        /* Check that this Object Identifier corresponds to EC for the Public Key */
        if(CsrMemCmp(ec, *scan, length)) break;
        *scan += length;

        /* parameters */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_OBJECT_IDENTIFIER)) break;
        length = getAsn1Length(scan);
        /* Check that this Object Identifier corresponds to WAPI parameters for EC */
        if(CsrMemCmp(ecWapiParams, *scan, length)) break;
        *scan += length;

        /* subjectPublicKey BIT STRING */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_BIT_STRING)) break;
        length = getAsn1Length(scan);
        /* Make sure the number of unused bits is 0 */
        if(*(*scan)++ != 0) break;

        /* If this point is reached, there were no parsing errors encountered */
        result = length - 1; /* - 1 for the byte indicating unused bits */
    }
    while(0);

    return result;
}

/*
 * Description:
 * See description in csr_asn1/csr_asn1.h
 */
/*---------------------------------------------------------------------------*/
CsrUint32 CsrCryptoAsn1GetEcPrivateKey(const CsrUint8 **scan)
{
    CsrUint32 result = 0;
    CsrUint32 length, keyLength;
    CsrUint8 *privateKey;
    /* DER encoding of 1.2.156.11235.1.1.2.1 WAPI Elliptic Curve parameters */
    CsrUint8 ecWapiParams[] = {0x2a, 0x81, 0x1c, 0xd7, 0x63, 0x01, 0x01, 0x02, 0x01};

    do
    {
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | ASN1_CONSTRUCTED | UNIVERSAL_SEQUENCE)) break;
        length = getAsn1Length(scan);

        /* Now start walking the elements of this SEQUENCE */

        /* Private Key version */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_INTEGER)) break;
        length = getAsn1Length(scan);
        *scan += length;

        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_OCTET_STRING)) break;
        length = getAsn1Length(scan);
        keyLength = length;
        privateKey = (CsrUint8*)*scan;
        *scan += length;

        if(*(*scan)++ != (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)) break;
        length = getAsn1Length(scan);

        /* Verify the OBJECT IDENTIFIER corresponds to WAPI EC parameters */
        if(*(*scan)++ != (ASN1_CLASS_UNIVERSAL | UNIVERSAL_OBJECT_IDENTIFIER)) break;
        length = getAsn1Length(scan);
        /* Check that this Object Identifier corresponds to EC for WAPI */
        if(CsrMemCmp(ecWapiParams, *scan, length)) break;

        /* If this point is reached, there were no parsing errors encountered */
        result = keyLength;
        *scan = privateKey;
    }
    while(0);

    return result;
}

