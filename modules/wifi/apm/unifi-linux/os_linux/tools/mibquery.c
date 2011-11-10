/*
 * ***************************************************************************
 *
 *  FILE: mibquery.c
 * 
 *      Code for querying a MIB value from UniFi.
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
#include <errno.h>
#include <sys/ioctl.h>
#include "csr_types.h"
#include "sme_drv.h"
#include "unifiio.h"


#define MIB_TAG_BOOLEAN     0x01
#define MIB_TAG_INTEGER     0x02
#define MIB_TAG_OCTETSTRING 0x04
#define MIB_TAG_NULL        0x05
#define MIB_TAG_OID         0x06
#define MIB_TAG_SEQUENCE    0x30



/* 
 * MibEncodeVarBindInt
 */
static int
MibEncodeVarBindInt(int aInt, unsigned char *aData)
{
    int bytesWritten;
    unsigned char value[4];
    int i;

    for (bytesWritten = 0; bytesWritten < 4; aInt /= 128)
    {
        value[bytesWritten] = (unsigned char)(aInt % 128);
        ++bytesWritten;

        if (aInt <= 127)
        {
            break;
        }
    }

    for (i = bytesWritten - 1; i >= 0; --i, ++aData)
    {
        *aData = value[i];
        if (i > 0)
        {
            *aData += 0x80;
        }
    }

    return bytesWritten;
} /* MibEncodeVarBindInt() */


/*
 * ---------------------------------------------------------------------------
 *  read_decimal
 *
 *      Scan a string for digits. Convert the digits (up to a non-digit
 *      delimiter) into a decimal number.
 * 
 *  Arguments:
 *      s               String to scan
 *      val             Pointer to which to write the result
 *
 *  Returns:
 *      NUmber of characters used.
 * ---------------------------------------------------------------------------
 */
#define isdigit(_c) (((_c) >= '0') && ((_c) <= '9'))
static int
read_decimal(const char *s, int *val)
{
    const char *p = s;
    int v = 0;

    /* Convert the number */
    while (*p && isdigit(*p)) {
        v = (v * 10) + *p - '0';
        p++;
    }

    *val = v;

    return p - s;
} /* read_int() */


/*
 * ---------------------------------------------------------------------------
 *  MibEncodeOID
 *
 *      Encode an OID (given as a string of dotted numbers, e.g. 1.2.840.1)
 *      into ASN.1 BER.
 * 
 *  Arguments:
 *      oid_str         OID string to convert
 *      buf             Pointer to buffer to write encoding into
 *
 *  Returns:
 *      number of bytes written to buf.
 *
 *  Notes:
 *      This function makes the following assumptions:
 *      1) The contents field will be no longer than 127 octets when encoded.
 * ---------------------------------------------------------------------------
 */
int
MibEncodeOID(const char *oid_str, unsigned char *buf)
{
    int length = 0;
    unsigned char *data = buf;
    const char *p;
    int id, tmp;
    int i;

    buf[0] = MIB_TAG_OID;

    p = oid_str;
    data = buf + 2; /* Assuming length will always be 1 octet */
    i = 0;
    while (*p)
    {
        /* Read integer from string */
        p += read_decimal(p, &id);
        if (*p) p++;    /* step over separator */

        switch (i)
        {
          case 0:
            *data = 40 * id;
            length++;
            break;
          case 1:
            *data += id;
            data++;
            break;
          default:
            tmp = MibEncodeVarBindInt(id, data);
            length += tmp;
            data += tmp;
            break;
        }
        i++;
    }

    buf[1] = (unsigned char)length;

    return 2 + length;
} /* MibEncodeOID() */


/* 
 * MibEncodeNull
 */
int
MibEncodeNull(unsigned char *aData)
{
    aData[0] = MIB_TAG_NULL;
    aData[1] = 0;

    return 2;
} /* MibEncodeNull() */


/*
 * ******************************************************************************
 * MibEncodeBoolean - 
 *
 * PARAMETERS
 * aVal - Boolean value to encode (0 = FALSE, 1 = TRUE).
 * aData - Pointer to buffer to decode.
 *
 * RETURNS
 * Number of bytes written into buffer pointed to by aData.
 *
 */
int
MibEncodeBoolean(int aVal, unsigned char *aData)
{
    aData[0] = MIB_TAG_BOOLEAN;
    aData[1] = 1;
    aData[2] = aVal;

    return 3;
}


/*
 * ******************************************************************************
 * MibEncodeInteger - 
 *
 * RETURNS
 * Number of bytes written into buffer pointed to by aData.
 *
 */
int
MibEncodeInteger(int aVal, unsigned char *aData)
{
    int length = 0;
    unsigned char valBytes[5] = { 0, 0, 0, 0, 0 };
    unsigned char *data = aData;
    int val = aVal;
    int i;

    *data = MIB_TAG_INTEGER;
    data += 2; /* Assuming length will always be 1 octet */

    do
    {
        if(val == 0)
        {
            *data = 0;
            length = 1;
            break;
        }

        while((val != 0) && (length < 4))
        {
            valBytes[length] = (val & 0xff);
            ++length;
            if((val < -128) || (val > 127))
            {
                val >>= 8;
            }
            else
            {
                /* Ensures we only use the minimum number of contents octets */
                val = 0;
            }
        }

        if((aVal > 0) && (valBytes[length - 1] & 0x80))
        {
            valBytes[length]  = 0;
            ++length;
        }

        for(i = length - 1; i >= 0; --i)
        {
            *data = valBytes[i]; ++data;
        }
    }
    while(0);

    aData[1] = (unsigned char)length;

    return 2 + length;
}



/*
 * ******************************************************************************
 * MibEncodeOctetString - 
 *
 * PARAMETERS
 * aStrLength - Number of bytes pointed to by aStr (<= 255).
 * aStr - Pointer to an array of bytes to include in the octet string.
 * aData - Pointer to buffer to encode octet string into.
 *
 * RETURNS
 * Number of bytes written into buffer pointed to by aData.
 *
 * REMARKS
 * This encoding is always primitive.
 *
 */
int
MibEncodeOctetString(unsigned char *aStr, int aStrLength, unsigned char *aData)
{
    unsigned char *data = aData;
    unsigned char *inData = aStr;
    int i;

    *data = MIB_TAG_OCTETSTRING;
    data += 2;                  /* Assuming length will always be 1 octet */
    
    for(i = 0; i < aStrLength; ++i, ++data, ++inData)
    {
        *data = *inData;
    }

    aData[1] = (unsigned char)aStrLength;

    return 2 + aStrLength;
}




/*
 * ******************************************************************************
 * MibDecodeInteger - 
 *
 * PARAMETERS
 * aData - 
 * aVal - Pointer to integer to contain decoded value.
 *
 * RETURNS
 * Number of bytes read from aData.
 *
 * REMARKS
 * This function will only decode up to 32-bit integers.
 *
 */
int
MibDecodeInteger(unsigned char *aData, int *aVal)
{
    unsigned char *data = aData + 2;
    int nbytes;
    int i = 0;
    int u;

    if(aData[0] != MIB_TAG_INTEGER)
    {
        return 0;
    }

    nbytes = aData[1];
    if (nbytes > 4)
    {
        return 0;
    }

    if (nbytes == 0)
    {
        *aVal = 0;
    }

    u = (data[0] & 0x80) ? -1 : 0;

    for (i = 0; i < nbytes; i++)
    {
        u = (u << 8) | data[i];
    }
    *aVal = u;

    return nbytes;
} /* MibDecodeInteger() */

/*
 * ******************************************************************************
 * MibDecodeOctetString - 
 *
 * PARAMETERS
 * aData - Octet string to decode.
 * aStrLength - Number of bytes in buffer pointed to by aData.
 * aStr - Pointer to buffer to decode octet string to.
 *
 * RETURNS
 * Number of bytes read from aOctStr.
 *
 */
int
MibDecodeOctetString(unsigned char *aData, int aStrLength, unsigned char *aStr)
{
    int length;
    unsigned char *inData = aData + 2;
    unsigned char *data = aStr;
    int maxlen;

    if (aData[0] != MIB_TAG_OCTETSTRING)
    {
        return 0;
    }

    /* maxlen = min(aStrLength, aData[1]); */
    maxlen = aStrLength;
    if (maxlen > aData[1]) maxlen = aData[1];

    for (length = 0; length < maxlen; ++length, ++data, ++inData)
    {
        *data = *inData;
    }

    return length;
}

/*
 * ******************************************************************************
 * MibDecodeBoolean - 
 *
 * PARAMETERS
 * aData - Pointer to buffer to decode.
 * aVal - Pointer to boolean value to decode into.
 *
 * RETURNS
 * Returns 1 if successfully decoded, otherwise 0.
 *
 */
int
MibDecodeBoolean(unsigned char *aData, CsrUint32 *aVal)
{
    if(aData[0] != MIB_TAG_BOOLEAN)
    {
        return 0;
    }

    if(aData[1] != 1)
    {
        return 0;
    }

    *aVal = aData[2];

    return 1;
}
