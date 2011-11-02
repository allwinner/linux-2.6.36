/** @file sme_proxy.c
 *
 *
 * Copyright (C) Cambridge Silicon Radio Ltd 2006-2007. All rights reserved.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 ****************************************************************************/

#include "unifi_priv.h"
#include "event_pack_unpack/event_pack_unpack.h"

/*
 * If message in buffer is a HIP signal, pass to HIP layer for sending
 * to UniFi. Return 1 to say we handled the message.
 * Otherwise return 0 to say we didn't handle the message.
 */
int
receive_remote_sys_hip_req(FsmContext* context, CsrUint8 *buffer, CsrUint32 length)
{
    CsrUint8* tempbuffer = buffer;
    CsrUint16 id = event_unpack_CsrUint16(&tempbuffer);

    if (id >= 0x0100 && id < 0x1000) {
        unifi_DataBlock mlmeCommand;
        unifi_DataBlock dataref1;
        unifi_DataBlock dataref2;

        mlmeCommand.length = (CsrUint16)length;
        mlmeCommand.data = buffer;

        tempbuffer += 6; /* Skip the 4 bytes for dest, sender and 1st slotnumber */
        dataref1.length = event_unpack_CsrUint16(&tempbuffer);
        tempbuffer += 2; /* Skip the slot number */
        dataref2.length = event_unpack_CsrUint16(&tempbuffer);

        dataref1.data = buffer + ((length - dataref2.length) - dataref1.length);
        dataref2.data = buffer +   length - dataref2.length;

        mlmeCommand.length -= dataref1.length + dataref2.length;

        unifi_sys_hip_req(context, mlmeCommand.length, mlmeCommand.data,
                                   dataref1.length, dataref1.data,
                                   dataref2.length, dataref2.data);

        return 1;
    }

    return 0;
}
