/** @file sme_measurements.c
 *
 * Measurement support functions
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007-2008. All rights reserved.
 *
 * @section DESCRIPTION
 * Measurement support functions
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_measurements/sme_measurements.c#3 $
 *
 ****************************************************************************/

/** @{
 * @ingroup sme_measurements
 */

#include "sme_measurements/sme_measurements.h"

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/

#include "abstractions/osa.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"

#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_pbc.h"
#include "fsm/csr_wifi_fsm.h"
#include "fsm/fsm_debug.h"

#include "csr_cstl/csr_wifi_list.h"


/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/

/* PRIVATE TYPES DEFINITIONS ************************************************/

/**
 * @brief defines access point global data structure
 *
 * @par Description
 *   defines access point ranking global data structure
 */
typedef struct measurement_measurement_node
{
    /** list structure */
    csr_list_node         node;

    smh_measurement_data  measurementData;
}measurement_node;

/* PRIVATE VARIABLE DEFINITIONS *********************************************/

/* PRIVATE FUNCTION PROTOTYPES **********************************************/

/* PRIVATE FUNCTION DEFINITIONS *********************************************/

/**
 * @brief Process measurements commands
 *
 * @par Description:
 * Process measurements commands
 *
 * @ingroup ccx
 *
 * @param[in] context : Context of calling FSM
 * @param[in] command : Command pointer
 *
 * @return
 *      void
 */
/*---------------------------------------------------------------------------*/
static measurement_node* get_meas_by_token(
        smh_measurement_data_block *pDataBlock,
        DialogToken dialogToken)
{
    measurement_node* pNode;

    for (pNode = csr_list_gethead_t(measurement_node *, &pDataBlock->measurementList); pNode != NULL;
         pNode = csr_list_getnext_t(measurement_node *, &pDataBlock->measurementList, &pNode->node))
    {
        if(pNode->measurementData.dialogToken == dialogToken)
        {
            return pNode;
        }
    }
    return NULL;

}


/* PUBLIC FUNCTION DEFINITIONS **********************************************/
/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
void smh_initialise_measurement_handler(
        smh_measurement_data_block *pDataBlock)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "smh_initialise_measurement_handler"));

    csr_list_init(&pDataBlock->measurementList);
}

/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
void smh_reset_measurement_handler(
        smh_measurement_data_block *pDataBlock)
{
    measurement_node *pMeasurementNode = NULL;
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "smh_reset_measurement_handler()"));

    pMeasurementNode = csr_list_gethead_t(measurement_node*, &pDataBlock->measurementList);

    while (NULL != pMeasurementNode)
    {
        (void)csr_list_remove(&pDataBlock->measurementList, &pMeasurementNode->node);
        pMeasurementNode = csr_list_gethead_t(measurement_node *, &pDataBlock->measurementList);
    }

    csr_list_init(&pDataBlock->measurementList);
}

/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
void smh_add_measurement_data(
        smh_measurement_data_block *pDataBlock,
        smh_measurement_data  *pMeasurementData)
{
    measurement_node *pMeasurementNode;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "smh_add_measurement_data"));
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "dialog %d size %d", pMeasurementData->dialogToken, sizeof(measurement_node)));

    pMeasurementNode = CsrPmalloc(sizeof(measurement_node));
    /* a copy to be returned */
    pMeasurementNode->measurementData = *pMeasurementData;

    csr_list_insert_tail(&pDataBlock->measurementList,
                         list_owns_value,
                         &pMeasurementNode->node,
                         pMeasurementNode );
}

/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
smh_measurement_data* smh_get_measurement_data(
        smh_measurement_data_block *pDataBlock,
        DialogToken dialogToken)
{
    measurement_node     *pMeasurementNode = get_meas_by_token(pDataBlock, dialogToken);
    smh_measurement_data *pMeasurementData = NULL;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "smh_get_measurement_data:"));

    if (pMeasurementNode != NULL)
    {
        pMeasurementData = &pMeasurementNode->measurementData;
    }

    return pMeasurementData;
}

/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
smh_measurement_data* smh_get_current_measurement_data(
        smh_measurement_data_block *pDataBlock)
{
    measurement_node     *pMeasurementNode = csr_list_gethead_t(measurement_node *, &pDataBlock->measurementList);
    smh_measurement_data *pMeasurementData = NULL;

    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "smh_get_current_measurement_data:"));

    if (pMeasurementNode != NULL)
    {
        pMeasurementData = &pMeasurementNode->measurementData;
    }

    return pMeasurementData;
}

/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
void smh_delete_measurement_data(
        smh_measurement_data_block *pDataBlock,
        DialogToken dialogToken)
{
    measurement_node *pMeasurementNode = get_meas_by_token(pDataBlock, dialogToken);

    if (pMeasurementNode != NULL)
    {
        /* now remove the wireless profile */
        (void)csr_list_remove(&pDataBlock->measurementList, &pMeasurementNode->node);
    }
}

/*
 * Description:
 * See description in sme_measurements/sme_measurements.h
 */
/*---------------------------------------------------------------------------*/
CsrBool smh_replace_current_measurement(
        smh_measurement_data  *pCurrentMeasurementData,
        unifi_MACAddress destinationAddress)
{
    sme_trace_entry((TR_SME_MEASUREMENTS_FSM, "smh_replace_current_measurement"));

    return TRUE;
}



