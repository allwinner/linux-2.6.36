/*
 * ---------------------------------------------------------------------------
 *
 * FILE: sme_dbg_sap.c
 *
 * PURPOSE:
 * This file provides the SME DBG API implementation.
 *
 * Copyright (C) 2008 Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#include "unifi_priv.h"
#include "sme_top_level_fsm/sme.h"
#include "dbg_sap/dbg_sap_serialise.h"

void unifi_dbg_cmd_req(FsmContext* context, char* command)
{
    unifi_priv_t* priv = (unifi_priv_t*)context;
    CsrUint8* buf;
    CsrUint16 buflen;

    unifi_trace(priv, UDBG2, "unifi_dbg_cmd_req(%s)\n", command);

    buflen = serialise_unifi_dbg_cmd_req(&buf, command);

    /* Send message to user space */
    sme_queue_message(priv, buf, buflen);
}
