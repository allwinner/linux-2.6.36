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
#ifndef _TMR_REGS_
#define _TMR_REGS_

#include <mach/hardware.h>

#define PA_TIMERS_BASE		0x01c20c00
#define VA_TIMERS_BASE		IO_ADDRESS(PA_TIMERS_BASE)

#define TIME_REGS_BASE		0xf1c20c00
#define TMRC_REG_CTRL           (TIME_REGS_BASE + 0x00)
#define TMRC_REG_STATUS         (TIME_REGS_BASE + 0x04)
#define TMRC_REG_TM0_CTRL       (TIME_REGS_BASE + 0x08)
#define TMRC_REG_TM0_INTV       (TIME_REGS_BASE + 0x0c)
#define TMRC_REG_LOSCCTRL       (TIME_REGS_BASE + 0x20)
#define TMRC_REG_TM5CTRL        (TIME_REGS_BASE + 0x30)
#define TMRC_REG_TM5IVALUE      (TIME_REGS_BASE + 0x34)
#define TMRC_REG_TM2CTRL        (TIME_REGS_BASE + 0x18)
#define TMRC_REG_TM2MOD         (TIME_REGS_BASE + 0x1c)

#endif    // #ifndef _TMR_REGS_
