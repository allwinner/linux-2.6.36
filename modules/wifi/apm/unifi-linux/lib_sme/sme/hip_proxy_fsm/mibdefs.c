/** @file mibdefs.c
 *
 * Provides definitions to be used in MIB (Management Information Block)
 * Access
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
 *  The mib_acessor array below is indexed using the mib_ids enumeration
 * from mibdefs.h
 *
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/sme/hip_proxy_fsm/mibdefs.c#3 $
 *
 ****************************************************************************/


/* STANDARD INCLUDES ********************************************************/

/* PROJECT INCLUDES *********************************************************/
#include "mibdefs.h"

/* MACROS *******************************************************************/

/* GLOBAL VARIABLE DEFINITIONS **********************************************/
static const CsrUint8 dot11MibHeaderData[] = {0x30,0xFF,0x06,0xFF,0x2a,0x86,0x48,0xce,0x34};
static const CsrUint8 unifiMibHeaderData[] = {0x30,0xFF,0x06,0xFF,0x2b,0x06,0x01,0x04,0x01,0x81,0xad,0x14,0x01};
static const CsrUint8 gb15629MibHeaderData[] = {0x30,0xFF,0x06,0xFF,0x2a,0x81,0x1c,0xd7,0x63};

static const oid_header dot11MibHeader = {dot11MibHeaderData, sizeof(dot11MibHeaderData)};
static const oid_header unifiMibHeader = {unifiMibHeaderData, sizeof(unifiMibHeaderData)};
static const oid_header gb15629MibHeader = {gb15629MibHeaderData,  sizeof(gb15629MibHeaderData)};


/** BER Encoded MIB OIDS and their value lengths */
static const CsrUint8 gb15629dot11wapiOptionImplementedData[]          = {0xfa,0x0d,0x0b,0x01,0x01,0x01,0x01,0x01,0x04,0x01};
static const CsrUint8 gb15629dot11wapiEnabledData[]                    = {0xfa,0x0d,0x0b,0x01,0x01,0x01,0x01,0x01,0x06,0x01};
static const CsrUint8 dot11MultiDomainCapabilityImplementedData[]      = {0x01,0x01,0x01,0x15,0x01};
static const CsrUint8 dot11MultiDomainCapabilityEnabledData[]          = {0x01,0x01,0x01,0x16,0x01};
static const CsrUint8 dot11CountryStringData[]                         = {0x01,0x01,0x01,0x17,0x01};
static const CsrUint8 dot11HighThroughputOptionImplementedData[]       = {0x01,0x01,0x01,0x5d,0x01};
static const CsrUint8 dot11WEPDefaultKeyValueData[]                    = {0x01,0x03,0x01,0x02,0x01,0x01};
static const CsrUint8 dot11PrivacyInvokedData[]                        = {0x01,0x05,0x01,0x01,0x01};
static const CsrUint8 dot11WEPDefaultKeyIDData[]                       = {0x01,0x05,0x01,0x02,0x01};
static const CsrUint8 dot11ExcludeUnencryptedData[]                    = {0x01,0x05,0x01,0x04,0x01};
static const CsrUint8 dot11WEPICVErrorCountData[]                      = {0x01,0x05,0x01,0x05,0x01};
static const CsrUint8 dot11WEPExcludedCountData[]                      = {0x01,0x05,0x01,0x06,0x01};
static const CsrUint8 dot11RSNAEnabledData[]                           = {0x01,0x05,0x01,0x07,0x01};
static const CsrUint8 dot11RSNAConfigGroupCipherData[]                 = {0x01,0x09,0x01,0x04,0x01};
static const CsrUint8 dot11RSNATKIPCounterMeasuresInvokedData[]        = {0x01,0x09,0x01,0x1b,0x01};
static const CsrUint8 dot11RSNA4WayHandshakeFailuresData[]             = {0x01,0x09,0x01,0x1c,0x01};
static const CsrUint8 dot11RSNAStatsTKIPICVErrorsData[]                = {0x01,0x0c,0x01,0x05,0x01,0x01};
static const CsrUint8 dot11RSNAStatsTKIPLocalMICFailuresData[]         = {0x01,0x0c,0x01,0x06,0x01,0x01};
static const CsrUint8 dot11RSNAStatsCCMPReplaysData[]                  = {0x01,0x0c,0x01,0x08,0x01,0x01};
static const CsrUint8 dot11RSNAStatsCCMPDecryptErrorsData[]            = {0x01,0x0c,0x01,0x09,0x01,0x01};
static const CsrUint8 dot11RSNAStatsTKIPReplaysData[]                  = {0x01,0x0c,0x01,0x0a,0x01,0x01};
static const CsrUint8 dot11MACAddressData[]                            = {0x02,0x01,0x01,0x01,0x01};
static const CsrUint8 dot11RTSThresholdData[]                          = {0x02,0x01,0x01,0x02,0x01};
static const CsrUint8 dot11ShortRetryLimitData[]                       = {0x02,0x01,0x01,0x03,0x01};
static const CsrUint8 dot11LongRetryLimitData[]                        = {0x02,0x01,0x01,0x04,0x01};
static const CsrUint8 dot11FragmentationThresholdData[]                = {0x02,0x01,0x01,0x05,0x01};
static const CsrUint8 dot11HCCWminData[]                               = {0x02,0x01,0x01,0x0b,0x01};
static const CsrUint8 dot11TransmittedFragmentCountData[]              = {0x02,0x02,0x01,0x01,0x01};
static const CsrUint8 dot11MulticastTransmittedFrameCountData[]        = {0x02,0x02,0x01,0x02,0x01};
static const CsrUint8 dot11FailedCountData[]                           = {0x02,0x02,0x01,0x03,0x01};
static const CsrUint8 dot11RetryCountData[]                            = {0x02,0x02,0x01,0x04,0x01};
static const CsrUint8 dot11MultipleRetryCountData[]                    = {0x02,0x02,0x01,0x05,0x01};
static const CsrUint8 dot11FrameDuplicateCountData[]                   = {0x02,0x02,0x01,0x06,0x01};
static const CsrUint8 dot11RTSSuccessCountData[]                       = {0x02,0x02,0x01,0x07,0x01};
static const CsrUint8 dot11RTSFailureCountData[]                       = {0x02,0x02,0x01,0x08,0x01};
static const CsrUint8 dot11ACKFailureCountData[]                       = {0x02,0x02,0x01,0x09,0x01};
static const CsrUint8 dot11ReceivedFragmentCountData[]                 = {0x02,0x02,0x01,0x0a,0x01};
static const CsrUint8 dot11MulticastReceivedFrameCountData[]           = {0x02,0x02,0x01,0x0b,0x01};
static const CsrUint8 dot11FCSErrorCountData[]                         = {0x02,0x02,0x01,0x0c,0x01};
static const CsrUint8 dot11TransmittedFrameCountData[]                 = {0x02,0x02,0x01,0x0d,0x01};
static const CsrUint8 dot11WEPUndecryptableCountData[]                 = {0x02,0x02,0x01,0x0e,0x01};
static const CsrUint8 dot11AddressData[]                               = {0x02,0x03,0x01,0x02,0x01,0x01};
static const CsrUint8 dot11GroupAddressesStatusData[]                  = {0x02,0x03,0x01,0x03,0x01,0x01};
static const CsrUint8 dot11manufacturerOUIData[]                       = {0x03,0x01,0x02,0x01,0x01,0x01};
static const CsrUint8 dot11manufacturerNameData[]                      = {0x03,0x01,0x02,0x01,0x02,0x01};
static const CsrUint8 dot11manufacturerProductNameData[]               = {0x03,0x01,0x02,0x01,0x03,0x01};
static const CsrUint8 dot11CurrentRegDomainData[]                      = {0x04,0x01,0x01,0x02,0x01};
static const CsrUint8 dot11CurrentTxPowerLevelData[]                   = {0x04,0x03,0x01,0x0a,0x01};
static const CsrUint8 dot11ShortSlotTimeOptionImplementedData[]        = {0x04,0x0e,0x01,0x05,0x01};
static const CsrUint8 dot11ShortSlotTimeOptionEnabledData[]            = {0x04,0x0e,0x01,0x06,0x01};
static const CsrUint8 unifiMLMEConnectionTimeOutData[]                 = {0x01,0x01,0x01};
static const CsrUint8 unifiMLMEAutonomousScanHighRSSIThresholdData[]   = {0x01,0x01,0x09};
static const CsrUint8 unifiMLMEAutonomousScanLowRSSIThresholdData[]    = {0x01,0x01,0x0a};
static const CsrUint8 unifiMLMEAutonomousScanDeltaRSSIThresholdData[]  = {0x01,0x01,0x0b};
static const CsrUint8 unifiMLMEAutonomousScanHighSNRThresholdData[]    = {0x01,0x01,0x0c};
static const CsrUint8 unifiMLMEAutonomousScanLowSNRThresholdData[]     = {0x01,0x01,0x0d};
static const CsrUint8 unifiMLMEAutonomousScanDeltaSNRThresholdData[]   = {0x01,0x01,0x0e};
static const CsrUint8 unifiMLMEAutonomousScanMaximumAgeData[]          = {0x01,0x01,0x0f};
static const CsrUint8 unifiMLMETriggeredGetHighRSSIThresholdData[]     = {0x01,0x01,0x13};
static const CsrUint8 unifiMLMETriggeredGetLowRSSIThresholdData[]      = {0x01,0x01,0x14};
static const CsrUint8 unifiMLMETriggeredGetHighSNRThresholdData[]      = {0x01,0x01,0x15};
static const CsrUint8 unifiMLMETriggeredGetLowSNRThresholdData[]       = {0x01,0x01,0x16};
static const CsrUint8 unifiFirmwarePatchBuildIDData[]                  = {0x01,0x02,0x04};
static const CsrUint8 unifiRSSIData[]                                  = {0x01,0x03,0x06,0x01};
static const CsrUint8 unifiSNRData[]                                   = {0x01,0x03,0x06,0x03};
static const CsrUint8 unifiTxDataRateData[]                            = {0x01,0x03,0x06,0x05};
static const CsrUint8 unifiTSFTimeData[]                               = {0x01,0x03,0x06,0x07};
static const CsrUint8 unifiTxPowerAdjustmentData[]                     = {0x01,0x03,0x07,0x09,0x01,0x02,0x01,0x01};
static const CsrUint8 unifiCoexSchemeData[]                            = {0x01,0x03,0x08,0x01};
static const CsrUint8 unifiCoexPeriodicPIOInputDurationData[]          = {0x01,0x03,0x09,0x04};
static const CsrUint8 unifiCoexPeriodicPIOInputPeriodData[]            = {0x01,0x03,0x09,0x05};
static const CsrUint8 unifiWMMEnabledData[]                            = {0x01,0x04,0x01,0x06};
static const CsrUint8 unifiFixTxDataRateData[]                         = {0x01,0x04,0x01,0x0a};
static const CsrUint8 unifiFixMaxTxDataRateData[]                      = {0x01,0x04,0x01,0x0b};
static const CsrUint8 unifiRadioCalibrationModeData[]                  = {0x01,0x04,0x01,0x16};
static const CsrUint8 unifiScanAverageTypeData[]                       = {0x01,0x04,0x01,0x1a};
static const CsrUint8 unifiExcludeUnencryptedExceptEAPOLData[]         = {0x01,0x04,0x01,0x1c};
static const CsrUint8 unifiFirmwareDriverInterfaceData[]               = {0x01,0x04,0x01,0x21};
static const CsrUint8 unifiRadioTrimCacheData[]                        = {0x01,0x04,0x02,0x01,0x01,0x02,0x01};
static const CsrUint8 unifiCSROnlyHighThroughputOptionEnabledData[]    = {0x02,0x03,0x01,0x06};
static const CsrUint8 unifiKeepAliveTimeData[]                         = {0x01,0x04,0x01,0x15};
static const CsrUint8 unifiScheduledInterruptPeriodData[]              = {0x02,0x03,0x02,0x01,0x01,0x03,0x02};
#ifdef CCX_VARIANT
static const CsrUint8 unifiCCXVersionImplementedData[]                 = {0x04,0x01};
static const CsrUint8 unifiCCXCiscoRadioMeasurementCapabilityEnabledData[] = {0x04,0x03};
static const CsrUint8 unifiCCXTSMetricsTableData[]                     = {0x04,0x07,0x01,0x01};
#endif /* CCX_VARIANT */

const mib_accessor mib_accessors[] = {
    {gb15629dot11wapiOptionImplementedData, &gb15629MibHeader, sizeof(gb15629dot11wapiOptionImplementedData), 1},
    {gb15629dot11wapiEnabledData, &gb15629MibHeader, sizeof(gb15629dot11wapiEnabledData), 1},
    {dot11MultiDomainCapabilityImplementedData, &dot11MibHeader, sizeof(dot11MultiDomainCapabilityImplementedData), 1},
    {dot11MultiDomainCapabilityEnabledData, &dot11MibHeader, sizeof(dot11MultiDomainCapabilityEnabledData), 1},
    {dot11CountryStringData, &dot11MibHeader, sizeof(dot11CountryStringData), 1},
    {dot11HighThroughputOptionImplementedData, &dot11MibHeader, sizeof(dot11HighThroughputOptionImplementedData), 1},
    {dot11WEPDefaultKeyValueData, &dot11MibHeader, sizeof(dot11WEPDefaultKeyValueData), 2},
    {dot11PrivacyInvokedData, &dot11MibHeader, sizeof(dot11PrivacyInvokedData), 1},
    {dot11WEPDefaultKeyIDData, &dot11MibHeader, sizeof(dot11WEPDefaultKeyIDData), 1},
    {dot11ExcludeUnencryptedData, &dot11MibHeader, sizeof(dot11ExcludeUnencryptedData), 1},
    {dot11WEPICVErrorCountData, &dot11MibHeader, sizeof(dot11WEPICVErrorCountData), 1},
    {dot11WEPExcludedCountData, &dot11MibHeader, sizeof(dot11WEPExcludedCountData), 1},
    {dot11RSNAEnabledData, &dot11MibHeader, sizeof(dot11RSNAEnabledData), 1},
    {dot11RSNAConfigGroupCipherData, &dot11MibHeader, sizeof(dot11RSNAConfigGroupCipherData), 1},
    {dot11RSNATKIPCounterMeasuresInvokedData, &dot11MibHeader, sizeof(dot11RSNATKIPCounterMeasuresInvokedData), 1},
    {dot11RSNA4WayHandshakeFailuresData, &dot11MibHeader, sizeof(dot11RSNA4WayHandshakeFailuresData), 1},
    {dot11RSNAStatsTKIPICVErrorsData, &dot11MibHeader, sizeof(dot11RSNAStatsTKIPICVErrorsData), 2},
    {dot11RSNAStatsTKIPLocalMICFailuresData, &dot11MibHeader, sizeof(dot11RSNAStatsTKIPLocalMICFailuresData), 2},
    {dot11RSNAStatsCCMPReplaysData, &dot11MibHeader, sizeof(dot11RSNAStatsCCMPReplaysData), 2},
    {dot11RSNAStatsCCMPDecryptErrorsData, &dot11MibHeader, sizeof(dot11RSNAStatsCCMPDecryptErrorsData), 2},
    {dot11RSNAStatsTKIPReplaysData, &dot11MibHeader, sizeof(dot11RSNAStatsTKIPReplaysData), 2},
    {dot11MACAddressData, &dot11MibHeader, sizeof(dot11MACAddressData), 1},
    {dot11RTSThresholdData, &dot11MibHeader, sizeof(dot11RTSThresholdData), 1},
    {dot11ShortRetryLimitData, &dot11MibHeader, sizeof(dot11ShortRetryLimitData), 1},
    {dot11LongRetryLimitData, &dot11MibHeader, sizeof(dot11LongRetryLimitData), 1},
    {dot11FragmentationThresholdData, &dot11MibHeader, sizeof(dot11FragmentationThresholdData), 1},
    {dot11HCCWminData, &dot11MibHeader, sizeof(dot11HCCWminData), 1},
    {dot11TransmittedFragmentCountData, &dot11MibHeader, sizeof(dot11TransmittedFragmentCountData), 1},
    {dot11MulticastTransmittedFrameCountData, &dot11MibHeader, sizeof(dot11MulticastTransmittedFrameCountData), 1},
    {dot11FailedCountData, &dot11MibHeader, sizeof(dot11FailedCountData), 1},
    {dot11RetryCountData, &dot11MibHeader, sizeof(dot11RetryCountData), 1},
    {dot11MultipleRetryCountData, &dot11MibHeader, sizeof(dot11MultipleRetryCountData), 1},
    {dot11FrameDuplicateCountData, &dot11MibHeader, sizeof(dot11FrameDuplicateCountData), 1},
    {dot11RTSSuccessCountData, &dot11MibHeader, sizeof(dot11RTSSuccessCountData), 1},
    {dot11RTSFailureCountData, &dot11MibHeader, sizeof(dot11RTSFailureCountData), 1},
    {dot11ACKFailureCountData, &dot11MibHeader, sizeof(dot11ACKFailureCountData), 1},
    {dot11ReceivedFragmentCountData, &dot11MibHeader, sizeof(dot11ReceivedFragmentCountData), 1},
    {dot11MulticastReceivedFrameCountData, &dot11MibHeader, sizeof(dot11MulticastReceivedFrameCountData), 1},
    {dot11FCSErrorCountData, &dot11MibHeader, sizeof(dot11FCSErrorCountData), 1},
    {dot11TransmittedFrameCountData, &dot11MibHeader, sizeof(dot11TransmittedFrameCountData), 1},
    {dot11WEPUndecryptableCountData, &dot11MibHeader, sizeof(dot11WEPUndecryptableCountData), 1},
    {dot11AddressData, &dot11MibHeader, sizeof(dot11AddressData), 2},
    {dot11GroupAddressesStatusData, &dot11MibHeader, sizeof(dot11GroupAddressesStatusData), 2},
    {dot11manufacturerOUIData, &dot11MibHeader, sizeof(dot11manufacturerOUIData), 1},
    {dot11manufacturerNameData, &dot11MibHeader, sizeof(dot11manufacturerNameData), 1},
    {dot11manufacturerProductNameData, &dot11MibHeader, sizeof(dot11manufacturerProductNameData), 1},
    {dot11CurrentRegDomainData, &dot11MibHeader, sizeof(dot11CurrentRegDomainData), 1},
    {dot11CurrentTxPowerLevelData, &dot11MibHeader, sizeof(dot11CurrentTxPowerLevelData), 1},
    {dot11ShortSlotTimeOptionImplementedData, &dot11MibHeader, sizeof(dot11ShortSlotTimeOptionImplementedData), 1},
    {dot11ShortSlotTimeOptionEnabledData, &dot11MibHeader, sizeof(dot11ShortSlotTimeOptionEnabledData), 1},
    {unifiMLMEConnectionTimeOutData, &unifiMibHeader, sizeof(unifiMLMEConnectionTimeOutData), 0},
    {unifiMLMEAutonomousScanHighRSSIThresholdData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanHighRSSIThresholdData), 0},
    {unifiMLMEAutonomousScanLowRSSIThresholdData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanLowRSSIThresholdData), 0},
    {unifiMLMEAutonomousScanDeltaRSSIThresholdData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanDeltaRSSIThresholdData), 0},
    {unifiMLMEAutonomousScanHighSNRThresholdData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanHighSNRThresholdData), 0},
    {unifiMLMEAutonomousScanLowSNRThresholdData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanLowSNRThresholdData), 0},
    {unifiMLMEAutonomousScanDeltaSNRThresholdData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanDeltaSNRThresholdData), 0},
    {unifiMLMEAutonomousScanMaximumAgeData, &unifiMibHeader, sizeof(unifiMLMEAutonomousScanMaximumAgeData), 0},
    {unifiMLMETriggeredGetHighRSSIThresholdData, &unifiMibHeader, sizeof(unifiMLMETriggeredGetHighRSSIThresholdData), 0},
    {unifiMLMETriggeredGetLowRSSIThresholdData, &unifiMibHeader, sizeof(unifiMLMETriggeredGetLowRSSIThresholdData), 0},
    {unifiMLMETriggeredGetHighSNRThresholdData, &unifiMibHeader, sizeof(unifiMLMETriggeredGetHighSNRThresholdData), 0},
    {unifiMLMETriggeredGetLowSNRThresholdData, &unifiMibHeader, sizeof(unifiMLMETriggeredGetLowSNRThresholdData), 0},
    {unifiFirmwarePatchBuildIDData, &unifiMibHeader, sizeof(unifiFirmwarePatchBuildIDData), 0},
    {unifiRSSIData, &unifiMibHeader, sizeof(unifiRSSIData), 0},
    {unifiSNRData, &unifiMibHeader, sizeof(unifiSNRData), 0},
    {unifiTxDataRateData, &unifiMibHeader, sizeof(unifiTxDataRateData), 0},
    {unifiTSFTimeData, &unifiMibHeader, sizeof(unifiTSFTimeData), 0},
    {unifiTxPowerAdjustmentData, &unifiMibHeader, sizeof(unifiTxPowerAdjustmentData), 2},
    {unifiCoexSchemeData, &unifiMibHeader, sizeof(unifiCoexSchemeData), 0},
    {unifiCoexPeriodicPIOInputDurationData, &unifiMibHeader, sizeof(unifiCoexPeriodicPIOInputDurationData), 0},
    {unifiCoexPeriodicPIOInputPeriodData, &unifiMibHeader, sizeof(unifiCoexPeriodicPIOInputPeriodData), 0},
    {unifiWMMEnabledData, &unifiMibHeader, sizeof(unifiWMMEnabledData), 0},
    {unifiFixTxDataRateData, &unifiMibHeader, sizeof(unifiFixTxDataRateData), 0},
    {unifiFixMaxTxDataRateData, &unifiMibHeader, sizeof(unifiFixMaxTxDataRateData), 0},
    {unifiRadioCalibrationModeData, &unifiMibHeader, sizeof(unifiRadioCalibrationModeData), 0},
    {unifiScanAverageTypeData, &unifiMibHeader, sizeof(unifiScanAverageTypeData), 0},
    {unifiExcludeUnencryptedExceptEAPOLData, &unifiMibHeader, sizeof(unifiExcludeUnencryptedExceptEAPOLData), 0},
    {unifiFirmwareDriverInterfaceData, &unifiMibHeader, sizeof(unifiFirmwareDriverInterfaceData), 0},
    {unifiRadioTrimCacheData, &unifiMibHeader, sizeof(unifiRadioTrimCacheData), 1},
    {unifiCSROnlyHighThroughputOptionEnabledData, &unifiMibHeader, sizeof(unifiCSROnlyHighThroughputOptionEnabledData), 0},
    {unifiKeepAliveTimeData, &unifiMibHeader, sizeof(unifiKeepAliveTimeData), 0},
    {unifiScheduledInterruptPeriodData, &unifiMibHeader, sizeof(unifiScheduledInterruptPeriodData), 0},
#ifdef CCX_VARIANT
    {unifiCCXVersionImplementedData, &unifiMibHeader, sizeof(unifiCCXVersionImplementedData), 0},
    {unifiCCXCiscoRadioMeasurementCapabilityEnabledData, &unifiMibHeader, sizeof(unifiCCXCiscoRadioMeasurementCapabilityEnabledData), 0},
    {unifiCCXTSMetricsTableData, &unifiMibHeader, sizeof(unifiCCXTSMetricsTableData), 2},
#endif /* CCX_VARIANT */
}; /* end of mib_accesors array */
