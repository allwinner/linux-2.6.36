/*
**********************************************************************************************************************
*                                                  CCMU BSP for sun
*                                 CCMU hardware registers definition and BSP interfaces
*
*                             Copyright(C), 2006-2009, uLIVE
*											       All Rights Reserved
*
* File Name : ccmu.h
*
* Author : Jerry
*
* Version : 1.1.0
*
* Date : 2009-8-20 11:10:01
*
* Description : This file provides some definition of CCMU's hardware registers and BSP interfaces.
*             This file is very similar to file "ccmu.inc"; the two files should be modified at the
*             same time to keep coherence of information.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Jerry      2008.05.23       1.1.0        build the file
*
* Jerry      2009-8-20        2.1.0        updata for new vision
*
**********************************************************************************************************************
*/
#ifndef _VIC_REGS_
#define _VIC_REGS_

#define PA_VIC_BASE		0x01c20400
#define VA_VIC_BASE		IO_ADDRESS(PA_VIC_BASE)

#define INTC_REG_VCTR            (VA_VIC_BASE + 0x00)
#define INTC_REG_VTBLBADDR       (VA_VIC_BASE + 0x04)                                           
#define INTC_REG_PENDCLR0        (VA_VIC_BASE + 0x08)                                           
#define INTC_REG_PENDCLR1        (VA_VIC_BASE + 0x0C)                                           
#define INTC_REG_CTRL            (VA_VIC_BASE + 0x10)                                           
#define INTC_REG_ENABLE0         (VA_VIC_BASE + 0x14)                                           
#define INTC_REG_ENABLE1         (VA_VIC_BASE + 0x18)                                           
#define INTC_REG_MASK0           (VA_VIC_BASE + 0x1C)                                           
#define INTC_REG_MASK1           (VA_VIC_BASE + 0x20)                                           
#define INTC_REG_RESPONSE        (VA_VIC_BASE + 0x18)                                           
#define INTC_REG_FASTFORCE       (VA_VIC_BASE + 0x1c)                                           
#define INTC_REG_PRIO0           (VA_VIC_BASE + 0x20)                                           
#define INTC_REG_PRIO1           (VA_VIC_BASE + 0x24) 

#endif    // #ifndef _VIC_REGS_