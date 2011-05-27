//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//------------------------------------------------------------------------------
//
//  Header: tcc78x_clk.h
//
//  Defines the clock and power register layout and definitions.
//
#ifndef __S3C2410X_CLKPWR_H
#define __S3C2410X_CLKPWR_H

#if __cplusplus
    extern "C" 
    {
#endif


//------------------------------------------------------------------------------
//  Type: S3C2410X_CLKPWR_REG
//
//  Clock and Power Management registers.
//

typedef struct 
{
    UINT32   LOCKTIME;               // PLL lock time count register

} S3C2410X_CLKPWR_REG, *PS3C2410X_CLKPWR_REG;


#if __cplusplus
    }
#endif

#endif 
