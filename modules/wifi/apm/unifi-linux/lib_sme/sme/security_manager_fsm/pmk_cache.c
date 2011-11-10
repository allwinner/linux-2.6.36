/** @file pmk_cache.c
 *
 * PMK Cache store
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
 *   Handles storing of PMK's
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/security_manager_fsm/pmk_cache.c#2 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "security_manager_fsm/pmk_cache.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "csr_cstl/csr_wifi_list.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

struct PmkCacheContext
{
    csr_list pmkList;
    CsrUint8 pmkListCount;
};

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief Gets a node from the list
 *
 * @par Description:
 * This function uses finds a cached PMK from the list
 *
 * @ingroup profile_manager
 *
 * @param[in] bssid : AP associated with the key
 *
 * @return
 *      csr_list_pmk* node in the list or NULL
 */
static csr_list_pmk* get_pmk_node(csr_list* pmkList, const unifi_MACAddress* bssid)
{
    csr_list_pmk* node;
    require(TR_SECURITY, bssid != NULL);

    sme_trace_entry((TR_SECURITY, "pmkList(%p) count %d",pmkList, csr_list_size(pmkList) ));

    for (node = csr_list_gethead_t(csr_list_pmk *, pmkList); node != NULL;
         node = csr_list_getnext_t(csr_list_pmk *, pmkList, &node->node))
    {
        if(CsrMemCmp(bssid, &node->pmkid.bssid, sizeof(bssid->data)) == 0)
        {
            return node;
        }
    }
    return NULL;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
PmkCacheContext* sec_initialise_pmk_cache(void)
{
    PmkCacheContext* context = (PmkCacheContext*)CsrPmalloc(sizeof(PmkCacheContext));

    sme_trace_entry((TR_SECURITY, "sec_initialise_pmk_cache(%p)", context));

    context->pmkListCount=0;
    csr_list_init(&context->pmkList);
    return context;
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
void sec_clear_pmk_cache(PmkCacheContext* context)
{
    csr_list_pmk* node = csr_list_gethead_t(csr_list_pmk *, &context->pmkList);
    context->pmkListCount=0;

    sme_trace_entry((TR_SECURITY, "sec_clear_pmk_cache(%p)", context));
    while(node != NULL)
    {
        (void)csr_list_remove(&context->pmkList, &node->node);
        node = csr_list_gethead_t(csr_list_pmk *, &context->pmkList);
    }
    csr_list_clear(&context->pmkList);
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
void sec_shutdown_pmk_cache(PmkCacheContext* context)
{
    sme_trace_entry((TR_SECURITY, "sec_shutdown_pmk_cache(%p)", context));
    sec_clear_pmk_cache(context);
    CsrPfree(context);
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
unifi_Status sec_get_pmk_cached_data(PmkCacheContext* context, const unifi_MACAddress* bssid, unifi_Pmkid** pmkid)
{
    csr_list_pmk* node = get_pmk_node(&context->pmkList, bssid);
    sme_trace_entry((TR_SECURITY, "sec_get_pmk_cached_data(%p)", node));
    require(TR_SECURITY, bssid != NULL);
    require(TR_SECURITY, pmkid != NULL);


    sme_trace_entry((TR_SECURITY, "sec_set_pmk_cached_data pmkCacheContext* context(%p)", context));
    *pmkid = NULL;

    if (node != NULL)
    {
        *pmkid = &node->pmkid;
        return unifi_Success;
    }
    return unifi_NotFound;
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
void sec_set_pmk_cached_data(PmkCacheContext* context, const unifi_SSID* ssid, const unifi_Pmkid* pmkid)
{
    csr_list_pmk* node = get_pmk_node(&context->pmkList, &pmkid->bssid);
    sme_trace_entry((TR_SECURITY, "sec_set_pmk_cached_data(%p)", node));
    require(TR_SECURITY, pmkid != NULL);

    sme_trace_debug((TR_SECURITY, "sec_set_pmk_cached_data: PmkCacheContext* context %p", context));

    /* remove any old values */
    sec_delete_pmk_cached_data(context, &pmkid->bssid);

    if (context->pmkListCount == UNIFI_PMKID_LIST_MAX)
    {
        /* Remove Head of list as that will be the oldest unused key */
        node = csr_list_gethead_t(csr_list_pmk *, &context->pmkList);
        (void)csr_list_remove(&context->pmkList, &node->node);
        context->pmkListCount--;
    }

    /* Create new Entry */
    context->pmkListCount++;
    node = CsrPmalloc(sizeof(csr_list_pmk));
    CsrMemCpy(&node->pmkid, pmkid, sizeof(unifi_Pmkid));
    node->ssid = *ssid;

    csr_list_insert_tail(&context->pmkList, list_owns_value, &node->node, node);
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
void sec_delete_pmk_cached_data(PmkCacheContext* context, const unifi_MACAddress* bssid)
{
    csr_list_pmk* node = get_pmk_node(&context->pmkList, bssid);
    sme_trace_entry((TR_SECURITY, "sec_delete_pmk_cached_data(%p)", node));
    require(TR_SECURITY, bssid != NULL);
    if (node != NULL)
    {
        context->pmkListCount--;
        (void)csr_list_remove(&context->pmkList, &node->node);
    }
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
csr_list* sec_get_pmk_list_handle(PmkCacheContext* context)
{
    sme_trace_debug((TR_SECURITY, "PmkCacheContext* context %p", context));
    return &context->pmkList;
}

/*
 * Description:
 * See description in sme_configuration/pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
CsrUint8 sec_get_pmk_list_count(PmkCacheContext* context)
{
    return context->pmkListCount;
}
