/* Tag: noCheckHeader */

/*    CONFIDENTIAL */
/*    Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved. */


#include "sme_trace/sme_trace_utils.h"

#ifdef SME_TRACE_TYPES_ENABLE
#ifdef SME_TRACE_ENABLE

const char *trace_unifi_MACAddress(unifi_MACAddress addr, char* macaddressBuffer)
{
    char     tmp = 0;
    CsrUint8    L1 = 0;

    for (L1 = 0; L1 < 17; L1++)
    {
        switch (L1 % 3)
        {
        case 0:
            tmp = ((addr.data[L1/3] & 0xF0) >> 4) + '0';
            break;
        case 1:
            tmp = (addr.data[L1/3] & 0x0F) + '0';
            break;
        case 2:
            macaddressBuffer[L1] = ':';
            continue;
        default:
            break;
        }
        macaddressBuffer[L1]=(tmp > '9')?(((tmp - '0') - 10) + 'A'):tmp;
    }
    macaddressBuffer[L1] = 0;
    return macaddressBuffer;
}

const char *trace_unifi_SSID(const unifi_SSID* pSsid, char* ssid)
{
    /* max SSID size +1 fo rthe null terminator */
    static const char* cloakedStr = "<CLOAKED>"; /*lint !e1776*/
    CsrUint8 c;
    CsrUint8 ssidLen = sizeof(pSsid->ssid);
    CsrBool cloaked = TRUE;

    /* The length should never be bigger than the actual SSID array */
    if (ssidLen > pSsid->length)
    {
        ssidLen = pSsid->length;
    }

    for(c = 0; c < ssidLen; c++)
    {
        if(pSsid->ssid[c] != 0)
        {
            cloaked = FALSE;
        }
    }

    if(cloaked || (ssidLen == 0))
    {
        return cloakedStr;
    }

    for(c = 0; c < ssidLen; c++)
    {
        if ( (pSsid->ssid[c] > 0)
           &&(pSsid->ssid[c] <= 0x1F))
        {
            ssid[c] = (char)'.';
        }
        else
        {
            ssid[c] = (char)pSsid->ssid[c];
        }
    }

    /* alway append the null terminator */
    ssid[ssidLen] = (char)'\0';

    return ssid;
}

#endif /* SME_TRACE_ENABLE */
#endif /* SME_TRACE_TYPES_ENABLE */
