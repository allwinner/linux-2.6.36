/* Tag: noCheckHeader */

/*    CONFIDENTIAL */
/*    Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved. */

#ifndef SME_TYPE_TRACE_UTILS_H
#define SME_TYPE_TRACE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "abstractions/osa.h"

#ifdef SME_TRACE_TYPES_ENABLE
#ifdef SME_TRACE_ENABLE

#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"

/* Custom Trace Functions */
const char *trace_unifi_MACAddress(unifi_MACAddress addr, char* macaddressBuffer);
const char *trace_unifi_SSID(const unifi_SSID* pSsid, char* ssid);

#endif /* SME_TRACE_ENABLE */
#endif /* SME_TRACE_TYPES_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* SME_TYPE_TRACE_UTILS_H */

