/** @file eapfast_cisco_pacfile.h
 *
 * Definitions for a function to load an out-band Cisco PAC file.
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
 *   Definitions for a function to load an out-band Cisco PAC file. The
 *   out-band PAC file is obtained manually from a Cisco Access Point, e.g.
 *   by asking the Access Point to TFTP it to a server. It can then be loaded
 *   by a function that understands the format; like this one!
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/fast/eapfast_cisco_pacfile.h#1 $
 *
 ****************************************************************************/
#ifndef SECURITY_METHOD_EAPFAST_CISCOPAC_H
#define SECURITY_METHOD_EAPFAST_CISCOPAC_H

#include "security_method_eapfast.h"

/** \@{
 * @ingroup ie_access
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode EAP-FAST PAC file data supplied out-band.
 *
 * @par Description
 *   Cisco Access Points are capable of supplying PAC data using either the
 *   in-band provisioning protocol or an out-band method. The out-band method
 *   involves the AP dumping the PAC data into an encrypted PAC file and
 *   the client loading it in.
 *   This function parses the PAC file data and fills out the fastContext with
 *   the extracted PAC data.
 *
 * @param[in]   fastContext : pointer to FastContext.
 * @param[in]   password : the password of the encrypted file (can be NULL for Cisco default password)
 *
 * @return TRUE=success, FALSE=failure (invalid PAC file data)
 */
CsrBool decode_pac_file(FastContext *fastContext, char *password);

/**
 * @brief Encode EAP-FAST PAC file data.
 *
 * @par Description
 *   This function encodes the PAC data in the fastContext to create the
 *   data for a PAC file.
 *
 * @param[in]   fastContext : pointer to FastContext.
 * @param[in]   password : the password of the encrypted file (can be NULL for Cisco default password)
 *
 * @return TRUE=success, FALSE=failure (invalid PAC data)
 */
CsrBool encode_pac_file(FastContext *fastContext);

#ifdef __cplusplus
}
#endif

/** \@}
 */

#endif /* SECURITY_METHOD_EAPFAST_CISCOPAC_H */
