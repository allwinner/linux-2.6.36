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
//  Header: tcc78x_gpsb.h
//
//  Defines the Serial Peripheral Interface (SPI) controller CPU register layout and
//  definitions.
//
#ifndef __TCC78x_GPSB_H
#define __TCC78x_GPSB_H


//------------------------------------------------------------------------------
//  Type: TCC78x_GPSB_REG    
//
//  Defines the SPI register layout. This register bank is located by the 
//  constant CPU_REG_BASE_XX_SPI in the configuration file cpu_reg_base_cfg.h.
//

typedef struct  
{
volatile uint32_t  PORT;       // Data Port
volatile uint32_t  STAT;       // Status
volatile uint32_t  INTEN;      // Interrupt
volatile uint32_t  MODE;       // Mode
volatile uint32_t  CTRL;       // Control
volatile uint32_t  reserved0[3];
volatile uint32_t  TXBASE;     
volatile uint32_t  RXBASE;
volatile uint32_t  PACKET;
volatile uint32_t  DMACTR;
volatile uint32_t  DMASTR;
} TCC78X_GPSB_REG, *PTCC78X_GPSB_REG; 

#endif 
