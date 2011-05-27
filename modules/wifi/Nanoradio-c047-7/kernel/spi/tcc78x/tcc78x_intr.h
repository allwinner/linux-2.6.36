//
// Copyright (c) Telechips Corporation.  All rights reserved.
//
//
//
//------------------------------------------------------------------------------
//
//  Header:  tcc78x_intr.h
//
//  Defines the interrupt controller register layout and associated interrupt
//  sources and bit masks.
//
#ifndef __TCC78X_INTR_H
#define __TCC78X_INTR_H

//------------------------------------------------------------------------------
//
//  Type: TCC78X_INTR_REG    
//
//  Interrupt control registers. This register bank is located by the constant 
//  TCC78X_BASE_REG_XX_INTR in the configuration file tcc78x_base_reg_cfg.h.
//

typedef struct {
volatile uint32_t IEN;                     // Interrupt Enable Register, R/W
volatile uint32_t CLR;                     // Interrupt Clear Register, R/W
volatile uint32_t STS;                     // Interrupt Status Register, R
volatile uint32_t SEL;                   // IRQ or FIQ Selection Register, 1 for IRQ, R/W
volatile uint32_t SRC;                     // Source Interrupt Status Register, R
volatile uint32_t MSTS;                  // Masked Status Register, R
volatile uint32_t TIG;                  // Test Interrupt Generation Register, R/W
volatile uint32_t POL;                 // Interrupt Polarity Register, R/W
volatile uint32_t IRQ;                  // IRQ Raw Status Register, R
volatile uint32_t FIQ;                   // FIQ Status Register, R
volatile uint32_t MIRQ;               // Masked IRQ Status Register, R
volatile uint32_t MFIQ;               // Masked FIQ Status Register, R
volatile uint32_t MODE;              // Trigger Mode Register -- level or edge, R/W
volatile uint32_t SYNC;              // Synchronization Enable Register, R/W
volatile uint32_t WKEN;             // Wakup Event Enable Register, R/W
volatile uint32_t MODEA;           // Both Edge or Single Edge Register, R/W
volatile uint32_t INTMSK;          // Interrupt Output Masking Register, R/W
volatile uint32_t ALLMSK;          // All Mask Region, R/W
} TCC78X_INTR_REG, *PTCC78X_INTR_REG;


//------------------------------------------------------------------------------
//
//  Define: IRQ_XXX
//
//  Interrupt sources numbers
//


#define IRQ_EINT0           0          
#define IRQ_EINT1           1
#define IRQ_EINT2           2
#define IRQ_EINT3           3

#define IRQ_RTC				4
#define IRQ_GPSB			5
#define IRQ_TIMER0			6
#define IRQ_TIMER1			7
#define IRQ_SCORE           8
#define IRQ_SPDTX           9

#define IRQ_VIDEO			10
#define IRQ_GSIO			11
#define IRQ_SC				12
#define IRQ_I2C				13
#define IRQ_DAIRX			14
#define IRQ_DAITX           15

#define IRQ_CDRX            16
#define IRQ_HPI				17
#define IRQ_UT0				18
#define IRQ_UT1				19
#define IRQ_UDMA            20       // USB 2.0 DMA Interrupt
#define IRQ_UD				21           // USB 2.0 Device Interrupt

#define IRQ_UH				22            // USB 1.1 Host Interrupt
#define IRQ_DMA				23          // DMA Controller Interrupt
#define IRQ_HDD				24              // HDD controller Interrupt
#define IRQ_MS				25           // Memory Stick Interrupt
#define IRQ_NFC				26          // Nand Flash Controller Interrupt
#define IRQ_SD				27          // SD/MMC Interrupt

#define IRQ_CAM				28         // Camera Interrupt
#define IRQ_LCD				29         // LCD Controller Interrupt
#define IRQ_ADC             30       // ADC Interrupt
#define IRQ_ECC             31        // ECC Interrupt


#endif 
