/** @file nme_pmk_cache.c
 *
 * Nme PMK Cache store
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2009. All rights reserved.
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
 *   $Id: //depot/dot11/v7.0p/host/lib_nme/nme/nme_security_manager_fsm/nme_pmk_cache.c#1 $
 *
 ****************************************************************************/

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "nme_security_manager_fsm/nme_pmk_cache.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief Gets a node from the list
 *
 * @par Description:
 * This function uses finds a cached PMK from the list
 *
 * @param[in] csr_list* : PMK List
 * @param[in] const unifi_MACAddress*  : BssId of associated AP with the key
 *
 * @return
 *      csr_list_pmk* node in the list or NULL
 */
static csr_list_pmk* get_pmk_node(csr_list* pmkList, const unifi_MACAddress* bssid)
{
    csr_list_pmk* node;

    require(TR_NME_SMGR_FSM, pmkList != NULL);
    require(TR_NME_SMGR_FSM, bssid != NULL);

    sme_trace_entry((TR_NME_SMGR_FSM, "get_pmk_node()"));

    for (node = csr_list_gethead_t(csr_list_pmk *, pmkList); node != NULL;
         node = csr_list_getnext_t(csr_list_pmk *, pmkList, &node->node))
    {
        if(CsrMemCmp(bssid->data, node->bssid.data, sizeof(bssid->data)) == 0)
        {
            sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "PMKID",
                           node->pmkid, CSR_WIFI_SECURITY_PMKID_LENGTH));
            sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "PMK",
                           node->pmk, CSR_WIFI_SECURITY_PMK_LENGTH));
            return node;
        }
    }
    return NULL;
}

/* PUBLIC FUNCTION DEFINITIONS **********************************************/

/*
 * Description:
 * See description in nme_security_manager_fsm/nme_pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
csr_list_pmk* nme_pmk_cache_get_pmk(csr_list* pmkList, const unifi_MACAddress* bssid)
{
    require(TR_NME_SMGR_FSM, pmkList != NULL);
    require(TR_NME_SMGR_FSM, bssid != NULL);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_pmk_cache_get_pmk "));

    return get_pmk_node(pmkList, bssid);
}

/*
 * Description:
 * See description in nme_security_manager_fsm/nme_pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
csr_list_pmk* nme_pmk_cache_get_pmk_from_pmkid(csr_list* pmkList, const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH])
{
    csr_list_pmk* node;

    require(TR_NME_SMGR_FSM, pmkList != NULL);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_pmk_cache_get_pmk_from_pmkid()"));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "PMKID",
                   pmkid, CSR_WIFI_SECURITY_PMKID_LENGTH));

    for (node = csr_list_gethead_t(csr_list_pmk *, pmkList); node != NULL;
         node = csr_list_getnext_t(csr_list_pmk *, pmkList, &node->node))
    {
        if(CsrMemCmp(pmkid, node->pmkid, CSR_WIFI_SECURITY_PMKID_LENGTH) == 0)
        {
            sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "PMK",
                           node->pmk, CSR_WIFI_SECURITY_PMK_LENGTH));
            return node;
        }
    }
    return NULL;
}

/*
 * Description:
 * See description in nme_security_manager_fsm/nme_pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
void nme_pmk_cache_set(csr_list* pmkList,
                       const unifi_MACAddress* bssid,
                       const CsrUint8 pmkid[CSR_WIFI_SECURITY_PMKID_LENGTH],
                       const CsrUint8 pmk[CSR_WIFI_SECURITY_PMK_LENGTH])
{
    csr_list_pmk* node;

    require(TR_NME_SMGR_FSM, pmkList != NULL);
    require(TR_NME_SMGR_FSM, bssid != NULL);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_pmk_cache_set()"));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "PMKID",
                   pmkid, CSR_WIFI_SECURITY_PMKID_LENGTH));
    sme_trace_hex((TR_NME_SMGR_FSM, TR_LVL_DEBUG, "PMK",
                   pmk, CSR_WIFI_SECURITY_PMK_LENGTH));

    /* remove any old values */
    (void)nme_pmk_cache_delete(pmkList, bssid);

    /* Create new Entry */
    node = CsrPmalloc(sizeof(csr_list_pmk));
    CsrMemCpy(node->bssid.data, bssid->data, sizeof(bssid->data));
    CsrMemCpy(node->pmkid, pmkid, CSR_WIFI_SECURITY_PMKID_LENGTH);
    CsrMemCpy(node->pmk, pmk, CSR_WIFI_SECURITY_PMK_LENGTH);

    csr_list_insert_tail(pmkList, list_owns_value, &node->node, node);
}

/*
 * Description:
 * See description in nme_security_manager_fsm/nme_pmk_cache.h
 */
/*---------------------------------------------------------------------------*/
CsrBool nme_pmk_cache_delete(csr_list* pmkList, const unifi_MACAddress* bssid)
{
    csr_list_pmk* node;

    require(TR_NME_SMGR_FSM, pmkList != NULL);
    require(TR_NME_SMGR_FSM, bssid != NULL);

    sme_trace_entry((TR_NME_SMGR_FSM, "nme_pmk_cache_delete()"));

    node = get_pmk_node(pmkList, bssid);
    if (node != NULL)
    {
        (void)csr_list_remove(pmkList, &node->node);
        return(TRUE);
    }
    return(FALSE);
}
