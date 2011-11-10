/** @file sync_access.c
 *
 *  Allow thread safe syncronous access to the scan results
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
 *   Allow thread safe syncronous access to the scan results
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sync_access/sync_access.c#1 $
 *
 ****************************************************************************/

#ifdef SME_SYNC_ACCESS

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sync_access/sync_access.h"
#include "fsm/fsm_internal.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"

#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "scan_manager_fsm/scan_results_storage.h"
#include "sme_configuration/sme_configuration_fsm.h"

#include "smeio/smeio_trace_types.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE CONSTANT DEFINITIONS *********************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/* PUBLIC FUNCTION DEFINITIONS *********************************************/

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
void unifi_mgt_claim_sync_access(FsmContext* context)
{
    SmeContext* smeContext;
    require(TR_SME_CONFIGURATION_FSM, context != NULL);

    if (context == NULL) return;

#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexLock(context->transitionLock);
#endif
#endif
    smeContext = getSmeContext(context);
    smeContext->syncAccessLocked = TRUE;
    smeContext->syncAccessScanResults.numElements = 0;
    smeContext->syncAccessScanResults.results = NULL;
    smeContext->syncAccessPmkid.numElements = 0;
    smeContext->syncAccessPmkid.pmkids = NULL;

}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
void unifi_mgt_release_sync_access(FsmContext* context)
{
    SmeContext* smeContext;

    require(TR_SME_CONFIGURATION_FSM, context != NULL);
    if (context == NULL) return;

    smeContext = getSmeContext(context);

    require(TR_SME_CONFIGURATION_FSM, smeContext->syncAccessLocked);

    smeContext->syncAccessLocked = FALSE;
    CsrPfree(smeContext->syncAccessScanResults.results);
    CsrPfree(smeContext->syncAccessPmkid.pmkids);

#ifdef FSM_MUTEX_ENABLE
#ifdef FSM_TRANSITION_LOCK
    (void)CsrMutexUnlock(context->transitionLock);
#endif
#endif
}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status unifi_mgt_scan_results_get(FsmContext* context, unifi_ScanResultList* resultsBuffer)
{
    SmeContext* smeContext;
    CsrUint16 bufferSize;
    CsrBool fullCopy = TRUE;

    sme_trace_entry((TR_SCAN, "scan_results_get_req()"));

    if (context == NULL) return unifi_InvalidParameter;
    if (resultsBuffer == NULL) return unifi_InvalidParameter;

    require(TR_SME_CONFIGURATION_FSM, getSmeContext(context)->syncAccessLocked);

    bufferSize = resultsBuffer->numElements;
    smeContext = getSmeContext(context);

    resultsBuffer->numElements = 0;

    /* If no Buffer passed in then allocate 1 of the correct size */
    if (bufferSize == 0)
    {
        if (smeContext->syncAccessScanResults.results)
        {
            *resultsBuffer = smeContext->syncAccessScanResults;
            return unifi_Success;
        }

        bufferSize = srs_copy_scanresults_bytes_required(context);
        resultsBuffer->results =  (unifi_ScanResult*)CsrPmalloc(bufferSize);
        sme_trace_debug((TR_SCAN, "unifi_mgt_scan_results_get() Allocating %d bytes", bufferSize));
        smeContext->syncAccessScanResults = *resultsBuffer;
        fullCopy = FALSE;
    }

    return srs_copy_scanresults(context, &resultsBuffer->numElements, resultsBuffer->results, bufferSize, fullCopy);
}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status unifi_mgt_scan_results_flush(FsmContext* context)
{
    if (context == NULL) return unifi_InvalidParameter;
    require(TR_SME_CONFIGURATION_FSM, getSmeContext(context)->syncAccessLocked);

    srs_delete_unconnected_scan_results(context);
    return unifi_Success;
}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status unifi_mgt_get_value(FsmContext* context, unifi_AppValue* appValue)
{
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "unifi_mgt_get_value(%s)", trace_unifi_AppValueId(appValue->id)));

    if (context == NULL) return unifi_InvalidParameter;
    if (appValue == NULL) return unifi_InvalidParameter;

    require(TR_SME_CONFIGURATION_FSM, getSmeContext(context)->syncAccessLocked);
    return get_sme_config_value(context, appValue);
}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status unifi_mgt_set_value(FsmContext* context, const unifi_AppValue* appValue)
{
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "unifi_mgt_set_value(%s)", trace_unifi_AppValueId(appValue->id)));

    if (context == NULL) return unifi_InvalidParameter;
    if (appValue == NULL) return unifi_InvalidParameter;

    require(TR_SME_CONFIGURATION_FSM, getSmeContext(context)->syncAccessLocked);
    return set_sme_config_value(context, appValue);
}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status unifi_mgt_pmkid(FsmContext* context, unifi_ListAction action, unifi_PmkidList *list)
{
    unifi_Status status;
    SmeContext* smeContext;
    unifi_PmkidList pmkids =  {0, NULL};

    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "unifi_mgt_pmkid(%s)", trace_unifi_ListAction(action)));

    if (context == NULL) return unifi_InvalidParameter;
    if (action == unifi_ListActionGet && list == NULL) return unifi_InvalidParameter;

    smeContext = getSmeContext(context);

    if (list != NULL)
    {
        pmkids = *list;
    }

    require(TR_SME_CONFIGURATION_FSM, getSmeContext(context)->syncAccessLocked);

    if (action == unifi_ListActionGet && smeContext->syncAccessPmkid.pmkids)
    {
        *list = smeContext->syncAccessPmkid;
        return unifi_Success;
    }

    status = set_sme_config_pmkids(context, action, &pmkids);

    if (action == unifi_ListActionGet)
    {
        getSmeContext(context)->syncAccessPmkid = pmkids;
        *list = pmkids;
    }

    return status;
}

/*
 * Description:
 * See description in sync_access/sync_access.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status unifi_mgt_blacklist(FsmContext* context, unifi_ListAction action, unifi_AddressList *list)
{
    sme_trace_entry((TR_SME_CONFIGURATION_FSM, "unifi_mgt_blacklist(%s)", trace_unifi_ListAction(action)));

    if (context == NULL) return unifi_InvalidParameter;
    if (action == unifi_ListActionGet && list == NULL) return unifi_InvalidParameter;

    require(TR_SME_CONFIGURATION_FSM, getSmeContext(context)->syncAccessLocked);

    return set_sme_config_blacklist(context, action, list);
}
#endif /* SME_SYNC_ACCESS */
