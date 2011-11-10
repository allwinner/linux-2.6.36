/** @file version.c
 *
 * SME Source Version identifier accessor
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
 *   Implements access function to the sme source version
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/version/version.c#1 $
 *
 ****************************************************************************/

#include "version.h"

const char* getSmeVersion()
{
    return(SME_SOURCE_VERSION_STRING);
}

const CsrUint32 buildIdNum = SME_SOURCE_VERSION_NUMBER;
const CsrUint32 buildVariant = SME_SOURCE_VARIANT_NUMBER;
