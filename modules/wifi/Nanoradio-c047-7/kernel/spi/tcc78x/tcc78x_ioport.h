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
//  Header:  TCC78X_ioport.h
//
//  Defines the Input/Output Ports (IOPORT) control registers and associated
//  types and constants.
//
#ifndef __TCC78X_IOPORT_H
#define __TCC78X_IOPORT_H

//------------------------------------------------------------------------------

typedef struct {

volatile uint32_t PORTCFG0;        // PORT Config 0
volatile uint32_t PORTCFG1;        // PORT Config 1
volatile uint32_t PORTCFG2;        // PORT Config 2
volatile uint32_t PORTCFG3;        // PORT Config 3
volatile uint32_t dummy0[4];
volatile uint32_t GPADAT;
volatile uint32_t GPAEN;
volatile uint32_t GPASET;
volatile uint32_t GPACLR;
volatile uint32_t GPAXOR;
volatile uint32_t dummy1[3];
volatile uint32_t GPBDAT;
volatile uint32_t GPBEN;
volatile uint32_t GPBSET;
volatile uint32_t GPBCLR;
volatile uint32_t GPBXOR;
volatile uint32_t dummy2[3];
volatile uint32_t GPCDAT;
volatile uint32_t GPCEN;
volatile uint32_t GPCSET;
volatile uint32_t GPCCLR;
volatile uint32_t GPCXOR;
volatile uint32_t dummy3[3];
volatile uint32_t GPDDAT;
volatile uint32_t GPDEN;
volatile uint32_t GPDSET;
volatile uint32_t GPDCLR;
volatile uint32_t GPDXOR;
volatile uint32_t dummy4[3];
volatile uint32_t GPEDAT;
volatile uint32_t GPEEN;
volatile uint32_t GPESET;
volatile uint32_t GPECLR;
volatile uint32_t GPEXOR;
volatile uint32_t dummy5[3];
volatile uint32_t CPUD0;
volatile uint32_t CPUD1;
volatile uint32_t CPUD2;
volatile uint32_t CPUD3;
volatile uint32_t CPUD4;
volatile uint32_t CPUD5;
volatile uint32_t CPUD6;
volatile uint32_t dummy6;
volatile uint32_t EINTSEL;
    
} TCC78X_IOPORT_REG, *PTCC78X_IOPORT_REG;  

//------------------------------------------------------------------------------

#endif // __TCC78X_IOPORT_H
