/** @file mibdefs.h
 *
 * The enums defined in this file are used to request data from the mib
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
 *   The enumeration below is used to access the mib_accessors array
 *   The MIB accessors array is implemented in mibdefs.c and is a list of
 *   BER encoded OIDs
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mibdefs.h#3 $
 *
 ****************************************************************************/
#ifndef MIBDEFS_H
#define MIBDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @{
 * @ingroup mib_access
 */
#include "abstractions/osa.h"

/* Constants */
#define MAX_ACCESSOR_LEN 30

/* struct to store MIB accessors */
typedef struct {
    const CsrUint8* oidheader;
    CsrUint8 length;
} oid_header;

/* Order is important for efficient packing of the struct */
typedef struct {
    const CsrUint8* oid;
    const oid_header* oidheader;
    CsrUint8 oidlength;
    CsrUint8 indices;
} mib_accessor;


extern const mib_accessor mib_accessors[];

typedef enum {
    gb15629dot11wapiOptionImplemented,
    gb15629dot11wapiEnabled,
    dot11MultiDomainCapabilityImplemented,
    dot11MultiDomainCapabilityEnabled,
    dot11CountryString,
    dot11HighThroughputOptionImplemented,
    dot11WEPDefaultKeyValue,
    dot11PrivacyInvoked,
    dot11WEPDefaultKeyID,
    dot11ExcludeUnencrypted,
    dot11WEPICVErrorCount,
    dot11WEPExcludedCount,
    dot11RSNAEnabled,
    dot11RSNAConfigGroupCipher,
    dot11RSNATKIPCounterMeasuresInvoked,
    dot11RSNA4WayHandshakeFailures,
    dot11RSNAStatsTKIPICVErrors,
    dot11RSNAStatsTKIPLocalMICFailures,
    dot11RSNAStatsCCMPReplays,
    dot11RSNAStatsCCMPDecryptErrors,
    dot11RSNAStatsTKIPReplays,
    dot11MACAddress,
    dot11RTSThreshold,
    dot11ShortRetryLimit,
    dot11LongRetryLimit,
    dot11FragmentationThreshold,
    dot11HCCWmin,
    dot11TransmittedFragmentCount,
    dot11MulticastTransmittedFrameCount,
    dot11FailedCount,
    dot11RetryCount,
    dot11MultipleRetryCount,
    dot11FrameDuplicateCount,
    dot11RTSSuccessCount,
    dot11RTSFailureCount,
    dot11ACKFailureCount,
    dot11ReceivedFragmentCount,
    dot11MulticastReceivedFrameCount,
    dot11FCSErrorCount,
    dot11TransmittedFrameCount,
    dot11WEPUndecryptableCount,
    dot11Address,
    dot11GroupAddressesStatus,
    dot11manufacturerOUI,
    dot11manufacturerName,
    dot11manufacturerProductName,
    dot11CurrentRegDomain,
    dot11CurrentTxPowerLevel,
    dot11ShortSlotTimeOptionImplemented,
    dot11ShortSlotTimeOptionEnabled,
    unifiMLMEConnectionTimeOut,
    unifiMLMEAutonomousScanHighRSSIThreshold,
    unifiMLMEAutonomousScanLowRSSIThreshold,
    unifiMLMEAutonomousScanDeltaRSSIThreshold,
    unifiMLMEAutonomousScanHighSNRThreshold,
    unifiMLMEAutonomousScanLowSNRThreshold,
    unifiMLMEAutonomousScanDeltaSNRThreshold,
    unifiMLMEAutonomousScanMaximumAge,
    unifiMLMETriggeredGetHighRSSIThreshold,
    unifiMLMETriggeredGetLowRSSIThreshold,
    unifiMLMETriggeredGetHighSNRThreshold,
    unifiMLMETriggeredGetLowSNRThreshold,
    unifiFirmwarePatchBuildID,
    unifiRSSI,
    unifiSNR,
    unifiTxDataRate,
    unifiTSFTime,
    unifiTxPowerAdjustment,
    unifiCoexScheme,
    unifiCoexPeriodicPIOInputDuration,
    unifiCoexPeriodicPIOInputPeriod,
    unifiWMMEnabled,
    unifiFixTxDataRate,
    unifiFixMaxTxDataRate,
    unifiRadioCalibrationMode,
    unifiScanAverageType,
    unifiExcludeUnencryptedExceptEAPOL,
    unifiFirmwareDriverInterface,
    unifiRadioTrimCache,
    unifiCSROnlyHighThroughputOptionEnabled,
    unifiKeepAliveTime,
    unifiScheduledInterruptPeriod,
#ifdef CCX_VARIANT
    unifiCCXVersionImplemented,
    unifiCCXCiscoRadioMeasurementCapabilityEnabled,
    unifiCCXTSMetricsTable,
#endif /* CCX_VARIANT */
    endoflist} mib_ids; /* end of enum mib_ids */

/** @}
 */

#ifdef __cplusplus
}
#endif

#endif
