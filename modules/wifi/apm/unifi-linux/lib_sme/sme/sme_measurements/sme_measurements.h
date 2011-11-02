/** @file sme_measurements.h
 *
 * Utilities to read and write information elements
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2007. All rights reserved.
 *
 * @section DESCRIPTION
 *   Utilities to read and write ccx information elements
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/sme_measurements/sme_measurements.h#2 $
 *
 ****************************************************************************/
#ifndef SME_MEASUREMENTS_H
#define SME_MEASUREMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

/** \@{
 * @ingroup sme_measurements
 */

/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "abstractions/osa.h"
#include "sme_top_level_fsm/sme_top_level_fsm.h"
#include "csr_cstl/csr_wifi_list.h"

/* MACROS *******************************************************************/

/* TYPES DEFINITIONS ********************************************************/
/**
 * @brief defines the ie result codes
 *
 * @par Description
 *   the ie result codes
 */
typedef enum MeasurementType
{
    MeasurementType_Basic   = 0x0000,
    MeasurementType_CCA     = 0x0001,
    MeasurementType_RPI     = 0x0002
} MeasurementType;

/**
 * @brief defines access point global data structure
 *
 * @par Description
 *   defines access point ranking global data structure
 */
typedef struct smh_measurement_data
{
    /** Dialogue Token */
    DialogToken dialogToken;
    /** Catergory */
    MeasurementCategory measurementCategory;
    /** Measurement Req originator */
    unifi_MACAddress originatorMacAddress;

}smh_measurement_data;

/**
 * @brief defines access point global data structure
 *
 * @par Description
 *   defines access point ranking global data structure
 */
typedef struct smh_measurement_data_block
{
    /** The primary scan results list */
    csr_list measurementList;


}smh_measurement_data_block;



/* GLOBAL VARIABLE DECLARATIONS *********************************************/

/* PUBLIC FUNCTION PROTOTYPES ***********************************************/

extern void smh_initialise_measurement_handler(
                    smh_measurement_data_block *pDataBlock);

extern void smh_reset_measurement_handler(
                    smh_measurement_data_block *pDataBlock);

extern void smh_add_measurement_data(
                    smh_measurement_data_block *pDataBlock,
                    smh_measurement_data  *pMeasurementData);

extern smh_measurement_data* smh_get_measurement_data(
                    smh_measurement_data_block *pDataBlock,
                    DialogToken dialogToken);

extern void smh_delete_measurement_data(
                    smh_measurement_data_block *pDataBlock,
                    DialogToken dialogToken);

extern CsrBool smh_replace_current_measurement(
        smh_measurement_data  *pCurrentMeasurementData,
        unifi_MACAddress destinationAddress);

extern smh_measurement_data* smh_get_current_measurement_data(
        smh_measurement_data_block *pDataBlock);


/** \@}
 */

#ifdef __cplusplus
}
#endif

#endif /* SME_MEASUREMENTS_H */
