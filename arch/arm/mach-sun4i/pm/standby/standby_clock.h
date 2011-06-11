/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_clock.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 21:05
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_CLOCK_H__
#define __STANDBY_CLOCK_H__

#include "standby_cfg.h"
#include <mach/ccmu_regs.h>


__s32 standby_clk_init(void);
__s32 standby_clk_exit(void);
__s32 standby_clk_core2losc(void);
__s32 standby_clk_core2hosc(void);
__s32 standby_clk_core2pll(void);
__s32 standby_clk_plldisable(void);
__s32 standby_clk_pllenable(void);
__s32 standby_clk_hoscdisable(void);
__s32 standby_clk_hoscenable(void);
__s32 standby_clk_ldodisable(void);
__s32 standby_clk_ldoenable(void);

#endif  /* __STANDBY_CLOCK_H__ */

