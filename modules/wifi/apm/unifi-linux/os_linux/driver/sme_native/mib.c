/*
 * ---------------------------------------------------------------------------
 * FILE:     mib.c
 *
 * PURPOSE:
 *      This file provides functions to send MLME requests to the UniFi.
 *      It is OS-independent.
 *
 * Copyright (C) 2005-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "unifi_priv.h"

#define MIB_TAG_BOOLEAN     0x01
#define MIB_TAG_INTEGER     0x02
#define MIB_TAG_OCTETSTRING 0x04
#define MIB_TAG_OID         0x06
#define MIB_TAG_SEQUENCE    0x30


#ifndef isdigit
#define isdigit(_c) (((_c) >= '0') && ((_c) <= '9'))
#endif


/* How much buffer space to use for varbinds */
#define VARBIND_LENGTH 128

static int MibEncodeOID(const char *oid_str, unsigned char *aData);
static int MibEncodeVarBindInt(int aInt, unsigned char *aData);
static int MibEncodeInteger(int aVal, unsigned char *aData);
static int MibEncodeOctetString(unsigned char *aStr, int aStrLength, unsigned char *aData);
#if 0
static int MibEncodeBoolean(unsigned char aVal, unsigned char *aData);
static int MibDecodeBoolean(unsigned char *aData, uint32 *aVal);
static int MibDecodeInteger(unsigned char *aData, uint32 *aVal);
static int MibDecodeOctetString(unsigned char *aData, int aDataLength, unsigned char *aStr);
#endif




#define dot11MACAddress_oid "1.2.840.10036.2.1.1.1.1"



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
 * ******************************************************************************
 * MibEncodeOID -
 *
 * PARAMETERS
 * aBindIndex - Index into gVarBinds.
 * aIndices - Pointer to array of indices for table entry.
 * aNumIndices - Number of indices in the aIndices array. Must match the
 *               mNumIndices member in gVarBinds entry given by aBindIndex.
 * buf - Pointer to buffer to encode varbind into.
 *
 * RETURNS
 * Number of bytes written into buffer pointed to by aData.
 *
 * REMARKS
 * This function makes the following assumptions:
 * 1) The contents field will be no longer than 127 octets when encoded.
 *
 */
static int
MibEncodeOID(const char *oid_str, unsigned char *buf)
{
    int length = 0;
    unsigned char *data = buf;
    const char *p;
    int id, tmp;
    int i;

    buf[0] = MIB_TAG_OID;
    data = buf + 2; /* Assuming length will always be 1 octet */

    p = oid_str;
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
 * MibEncodeVarBindInt
 */
static int
MibEncodeVarBindInt(int aInt, unsigned char *aData)
{
    int bytesWritten;
    unsigned char value[4];
    int i;

    for(bytesWritten = 0; bytesWritten < 4; aInt /= 128)
    {
        value[bytesWritten] = (unsigned char)(aInt % 128);
        ++bytesWritten;

        if(aInt <= 127)
        {
            break;
        }
    }

    for(i = bytesWritten - 1; i >= 0; --i, ++aData)
    {
        *aData = value[i];
        if(i > 0)
        {
            *aData += 0x80;
        }
    }

    return bytesWritten;
} /* MibEncodeVarBindInt() */


/*
 * MibEncodeNull
 */
static int
MibEncodeNull(unsigned char *aData)
{
    aData[0] = 5;
    aData[1] = 0;

    return 2;
} /* MibEncodeNull() */



/*
 * ******************************************************************************
 * MibEncodeInteger -
 *
 * RETURNS
 * Number of bytes written into buffer pointed to by aData.
 *
 */
static int
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
} /* MibEncodeInteger() */



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
static int
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
} /* MibEncodeOctetString() */


/*
 * ---------------------------------------------------------------------------
 *  MibDecodeInteger
 *
 *      Decode a varbind from UniFi MLME-GET result into an integer value.
 *
 *  Arguments:
 *      aData           Pointer to varbind.
 *      aVal            Pointer to integer for return value.
 *
 *  Returns:
 *      Size of integer read in bytes (1,2,3,4) or 0 on error.
 *
 *  Notes:
 *      This function will only decode up to 32-bit integers.
 * ---------------------------------------------------------------------------
 */
static int
MibDecodeInteger(unsigned char *aData, unsigned int *aVal)
{
    unsigned char *data = aData + 2;
    int nbytes;
    int i = 0;
    unsigned int u;

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

    u = (data[0] & 0x80) ? 0xFFFFFFFF : 0;

    for (i = 0; i < nbytes; i++)
    {
        u = (u << 8) | data[i];
    }
    *aVal = u;

    return nbytes;
} /* MibDecodeInteger() */



/*
 * ---------------------------------------------------------------------------
 *  lookup_mib_status
 *
 *      desc
 * 
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static const char *
lookup_mib_status(int status)
{
    const char *str;

    switch (status) {
      case CSR_MIB_SUCCESSFUL:
        str = "Success";
        break;

      case CSR_MIB_INVALID_PARAMETERS:
        str = "Invalid Parameters";
        break;

      case CSR_MIB_WRITE_ONLY:
        str = "Write only";
        break;

      case CSR_MIB_READ_ONLY:
        str = "Read only";
        break;

      default:
        str = "<unknown>";
    }

    return str;
} /* lookup_mib_status() */



/*
 * ----------------------------------------------------------------
 *  unifi_get_mib
 *  unifi_set_mib
 *
 *      This function is called from the driver ioctl method to
 *      handle UNIFI_GET_MIB and UNIFI_SET_MIB.
 *
 * Arguments:
 *	card            Pointer to card context struct
 *      varbind         Pointer to MIB varbind passed by the user.
 *      maxlen          The number of bytes available in varbind
 *                      for the return value.
 *
 * Returns:
 *      0 on success,
 *      -ve number: transport error code
 *      +ve number: MIB Status code returned by UniFi:
 *              1       Invalid Parameters
 *              2       Write only
 *              3       Read only
 * ----------------------------------------------------------------
 */
int
unifi_set_mib(unifi_priv_t *priv, ul_client_t *pcli, unsigned char *varbind)
{
    CSR_SIGNAL signal;
    bulk_data_param_t data_ptrs;
    int r, status;
    int timeout = 1000;
    unsigned char *request_buf = NULL;
    unsigned int request_len = 0;

    /* Check varbind begins with a SEQUENCE tag */
    if (varbind[0] != MIB_TAG_SEQUENCE) {
        return -EINVAL;
    }

    /* Extract total length */
    request_len = varbind[1] + 2;

    r = unifi_net_data_malloc(priv, &data_ptrs.d[0], request_len);
    if (r != 0) {
        unifi_error(priv, "unifi_set_mib: failed to allocate request_buf.\n");
        return -EIO;
    }
    request_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

    memcpy(request_buf, varbind, request_len);

#if 0
    printk("ioctl MIB SET varbind (%d):\n", vb_len);
    dump(varbind, vb_len);
#endif

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_SET_REQUEST_ID;

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = request_buf;
    data_ptrs.d[0].data_length = request_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send SET request, error %d\n", r);
        return r;
    }

    status = pcli->reply_signal->u.MlmeSetConfirm.Status;
    if (status) {
        unifi_trace(priv, UDBG1, "MIB-SET request was rejected with status 0x%X (%s)\n",
              status, lookup_mib_status(status));
    }

    return status;
} /* unifi_set_mib() */

int
unifi_get_mib(unifi_priv_t *priv, ul_client_t *pcli, unsigned char *varbind, int maxlen)
{
    CSR_SIGNAL signal;
    bulk_data_param_t data_ptrs;
    int datalen;
    int r, status;
    int timeout = 1000;
    unsigned char *request_buf = NULL;
    unsigned int request_len = 0;

    /* Check varbind begins with a SEQUENCE tag */
    if (varbind[0] != MIB_TAG_SEQUENCE) {
        return -EINVAL;
    }

    /* Extract total length */
    request_len = varbind[1] + 2;

    r = unifi_net_data_malloc(priv, &data_ptrs.d[0], request_len);
    if (r != 0) {
        unifi_error(priv, "unifi_get_mib: failed to allocate request_buf.\n");
        return -EIO;
    }
    request_buf = (unsigned char*)data_ptrs.d[0].os_data_ptr;

    memcpy(request_buf, varbind, request_len);

#if 0
    printk("ioctl MIB GET varbind:\n");
    dump(varbind, vb_len);
#endif

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_GET_REQUEST_ID;

    /* Do not mess with data_ptrs.d[0].os_net_buf_ptr. */
    data_ptrs.d[0].os_data_ptr = request_buf;
    data_ptrs.d[0].data_length = request_len;
    data_ptrs.d[1].os_data_ptr = data_ptrs.d[1].os_net_buf_ptr = NULL;
    data_ptrs.d[1].data_length = 0;

    r = unifi_mlme_blocking_request(priv, pcli, &signal, &data_ptrs, timeout);
    if (r < 0) {
        unifi_error(priv, "failed to send GET request, error %d\n", r);
        return r;
    }

    status = pcli->reply_signal->u.MlmeGetConfirm.Status;
    if (status) {
        unifi_trace(priv, UDBG1, "MIB-GET request was rejected with status 0x%X (%s)\n",
              status, lookup_mib_status(status));
        return status;
    }

#if 0
    printk("ioctl MIB GET result:\n");
    dump(varbind, varbind[1] + 2);
#endif

    /* Now retrieve the bulk data */
    datalen = pcli->reply_bulkdata[0]->length;
    if (varbind && datalen) {
        if (datalen > maxlen) {
            datalen = maxlen;
        }

        memcpy(varbind, pcli->reply_bulkdata[0]->ptr, datalen);
    }
    return 0;

} /* unifi_get_mib() */




/*
 * ---------------------------------------------------------------------------
 *  unifi_set_mib_string
 *  unifi_set_mib_int
 *
 *      Convenience functions for setting MIB values of OCTETSTRING and
 *      INTEGER types.
 *
 *  Arguments:
 *	card            Pointer to card context struct
 *      oidstr          String describing OID, e.g. "1.2.840.10036.2.1.1.1.1"
 *      value           MIB value
 *      val_len         Length of string value
 *
 *  Returns:
 *      0 on success,
 *      -ve number: transport error code
 *      +ve number: MIB Status code returned by UniFi:
 *              1       Invalid Parameters
 *              2       Write only
 *              3       Read only
 * ---------------------------------------------------------------------------
 */
int
unifi_set_mib_string(unifi_priv_t *priv, ul_client_t *pcli, char *oidstr, unsigned char *value, int val_len)
{
    unsigned char varbind[MAX_VARBIND_LENGTH];
    unsigned char *ptr;
    int len;

    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    len = MibEncodeOID(oidstr, ptr);

    len += MibEncodeOctetString(value, val_len, ptr+len);

    varbind[1] = len;

    return unifi_set_mib(priv, pcli, varbind);
} /* unifi_set_mib_string() */

int
unifi_set_mib_int(unifi_priv_t *priv, ul_client_t *pcli, char *oidstr, int value)
{
    unsigned char varbind[MAX_VARBIND_LENGTH];
    unsigned char *ptr;
    int len;

    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    len = MibEncodeOID(oidstr, ptr);

    len += MibEncodeInteger(value, ptr+len);

    varbind[1] = len;

    return unifi_set_mib(priv, pcli, varbind);
} /* unifi_set_mib_int() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_get_mib_int
 *
 *      Retrieve and integer MIB value.
 *
 *  Arguments:
 *	card            Pointer to card context struct
 *      oidstr          String describing OID, e.g. "1.2.840.10036.2.1.1.1.1"
 *      valuep          Pointer to which to write the returned MIB value
 *
 *  Returns:
 *      0 on success,
 *      -ve number: transport error code
 *      +ve number: MIB Status code returned by UniFi:
 *              1       Invalid Parameters
 *              2       Write only
 *              3       Read only
 * ---------------------------------------------------------------------------
 */
int
unifi_get_mib_int(unifi_priv_t *priv, ul_client_t *pcli, char *oidstr, int *valuep)
{
    unsigned char varbind[MAX_VARBIND_LENGTH];
    unsigned char *ptr;
    int vblen;
    int r;

    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    vblen = MibEncodeOID(oidstr, ptr);

    vblen += MibEncodeNull(ptr+vblen);

    varbind[1] = vblen;

    /* Use the ioctl helper fn to execute a MIB GET operation */
    r = unifi_get_mib(priv, pcli, varbind, MAX_VARBIND_LENGTH);
    if (r) {
        return r;
    }

    /* Retrieve the returned value */

    /* Skip over the varbind SEQUENCE tag and OID */
    ptr = varbind + 2;
    ptr += ptr[1] + 2;

    if (ptr[0] != MIB_TAG_INTEGER) {
        /* unexpected type of return value */
        unifi_error(priv, "Unexpected type of return value for %s: %d\n",
                    oidstr, ptr[0]);
        return -EIO;
    }

    if (MibDecodeInteger(ptr, (unsigned int *)valuep) == 0) {
        unifi_error(priv, "Malformed return value for %s\n", oidstr);
        return -EIO;
    }

    func_exit();
    return 0;
} /* unifi_get_mib_int() */



