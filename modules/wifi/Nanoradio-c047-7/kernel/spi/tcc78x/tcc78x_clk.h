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
#ifndef __TCC78X_CLK_H
#define __TCC78X_CLK_H


#define     FIN_CLK_FREQ        120000  // unit : 100Hz

//------------------------------------------------------------------------------
//  Type: TCC78X_CLK_REG 
//
//  Clock control registers.
//

typedef struct 
{
volatile uint32_t  CLKCTRL;        // CPU & BUS Clock 
volatile uint32_t  PLL0CFG;        // PLL0 Configuration
volatile uint32_t  PLL1CFG;        // PLL1 Configuration
volatile uint32_t  CLKDIVC;        // Divided Clock Configuration
volatile uint32_t  CLKDIVC1;       // PLL0 Divider 1 Configuration
volatile uint32_t  MODECTR;        // Operating Mode Control
volatile uint32_t  BCLKCTR;        // Bus Clock Mask Control
volatile uint32_t  SWRESET;        // Software Reset Control
volatile uint32_t  PCK_USB11H;     // USB11H Clock Control
volatile uint32_t  PCK_SDMMC;
volatile uint32_t  PCK_MSTICK;
volatile uint32_t  PCK_I2C;
volatile uint32_t  PCK_LCD;
volatile uint32_t  PCK_CAM;
volatile uint32_t  PCK_UART0;
volatile uint32_t  PCK_UART1;
volatile uint32_t  PCK_GSIO;
volatile uint32_t  PCK_TCT;
volatile uint32_t  PCK_TCX;
volatile uint32_t  PCK_TCZ;
volatile uint32_t  PCK_ADC;
volatile uint32_t  PCK_DAI;
volatile uint32_t  PCK_SPDTX;
volatile uint32_t  PCK_RFREQ;
volatile uint32_t  PCK_SPIS;
volatile uint32_t  PCK_SCALER;
volatile uint32_t  dummy1[6];
volatile uint32_t  BCKVCFG;        // Bus Clock(in Video Part) Control
volatile uint32_t  reserved;       // Reserved
volatile uint32_t  MCLKCTRL;       // Main Processor Clock Control
volatile uint32_t  SCLKCTRL;       // Sub Processor Clock Control
volatile uint32_t  BCLKMSKE;       // Bus Clock Mask Enable
    //uint32_t  OPTION0;
} TCC78X_CLK_REG, *PTCC78X_CLK_REG;

// CLKCTRL[0:2] ==> CKSEL
#define CLKCTRL_CKSEL_PLL0      0x00
#define CLKCTRL_CKSEL_PLL1      0x01
#define CLKCTRL_CKSEL_DIVPLL0   0x02
#define CLKCTRL_CKSEL_DIVPLL1   0x03
#define CLKCTRL_CKSEL_XIN       0x04
#define CLKCTRL_CKSEL_DIVXIN    0x05
#define CLKCTRL_CKSEL_XTIN      0x06
#define CLKCTRL_CKSEL_DIVXTIN   0x07
#define CLKCTRL_CKSEL_MASK      0x07

// PLLCFG
#define PLLCFG_S_SHIFT         16
#define PLLCFG_S_MASK          0x03 << PLLCFG_S_SHIFT
#define PLLCFG_M_SHIFT         8
#define PLLCFG_M_MASK          0xFF << PLLCFG_M_SHIFT
#define PLLCFG_P_MASK          0x3F

// PCK_XXX 
#define PCK_XXX_EN              1<<28
#define PCK_XXX_SEL_PLL0        0
#define PCK_XXX_SEL_PLL1        1<<24
#define PCK_XXX_SEL_XIN         4<<24

#endif 
