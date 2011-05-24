/*
*********************************************************************************************************
*                                                    eBase
*                                           the Abstract of Hardware
*
*                                   (c) Copyright 2006-2010, holigun China
*                                                All Rights Reserved
*
* File        :  sdxc_spec.h
* Date        :  2012-11-25 14:24:12
* Author      :  Aaron.Maoye
* Version     :  v1.00
* Description:
*                            Register-based Operation for SD3.0 Controller module, aw1620
* History     :
*      <author>         <time>                  <version >      <desc>
*       Maoye           2012-11-25 14:25:25     1.0             create this file
*
*********************************************************************************************************
*/

#ifndef _SUN4I_SDC_SYSCALL_H_
#define _SUN4I_SDC_SYSCALL_H_

#include <mach/platform.h>
#include <linux/io.h>
#include "host_op.h"

extern unsigned int smc_debug;

#define GPIO_BASE            		SW_PA_PORTC_IO_BASE

#define PA_CFG0_REG        			(gpio_base+0x000)
#define PA_CFG1_REG           	    (gpio_base+0x004)
#define PA_CFG2_REG         		(gpio_base+0x008)
#define PA_CFG3_REG         		(gpio_base+0x00C)
#define PA_DAT_REG         			(gpio_base+0x010)
#define PA_DRV0_REG        			(gpio_base+0x014)
#define PA_DRV1_REG        			(gpio_base+0x018)
#define PA_PULL0_REG     			(gpio_base+0x01C)
#define PA_PULL1_REG     			(gpio_base+0x020)
#define PB_CFG0_REG        			(gpio_base+0x024)
#define PB_CFG1_REG           	    (gpio_base+0x028)
#define PB_CFG2_REG         		(gpio_base+0x02C)
#define PB_CFG3_REG         		(gpio_base+0x030)
#define PB_DAT_REG         			(gpio_base+0x034)
#define PB_DRV0_REG        			(gpio_base+0x038)
#define PB_DRV1_REG        			(gpio_base+0x03C)
#define PB_PULL0_REG     			(gpio_base+0x040)
#define PB_PULL1_REG     			(gpio_base+0x044)
#define PC_CFG0_REG        			(gpio_base+0x048)
#define PC_CFG1_REG           	    (gpio_base+0x04C)
#define PC_CFG2_REG         		(gpio_base+0x050)
#define PC_CFG3_REG         		(gpio_base+0x054)
#define PC_DAT_REG         			(gpio_base+0x058)
#define PC_DRV0_REG        			(gpio_base+0x05C)
#define PC_DRV1_REG        			(gpio_base+0x060)
#define PC_PULL0_REG     			(gpio_base+0x064)
#define PC_PULL1_REG     			(gpio_base+0x068)
#define PD_CFG0_REG        			(gpio_base+0x06C)
#define PD_CFG1_REG           	    (gpio_base+0x070)
#define PD_CFG2_REG         		(gpio_base+0x074)
#define PD_CFG3_REG         		(gpio_base+0x078)
#define PD_DAT_REG         			(gpio_base+0x07C)
#define PD_DRV0_REG        			(gpio_base+0x080)
#define PD_DRV1_REG        			(gpio_base+0x084)
#define PD_PULL0_REG     			(gpio_base+0x088)
#define PD_PULL1_REG     			(gpio_base+0x08C)
#define PE_CFG0_REG        			(gpio_base+0x090)
#define PE_CFG1_REG           	    (gpio_base+0x094)
#define PE_CFG2_REG         		(gpio_base+0x098)
#define PE_CFG3_REG         		(gpio_base+0x09C)
#define PE_DAT_REG         			(gpio_base+0x0A0)
#define PE_DRV0_REG        			(gpio_base+0x0A4)
#define PE_DRV1_REG        			(gpio_base+0x0A8)
#define PE_PULL0_REG     			(gpio_base+0x0AC)
#define PE_PULL1_REG     			(gpio_base+0x0B0)
#define PF_CFG0_REG        			(gpio_base+0x0B4)
#define PF_CFG1_REG           	    (gpio_base+0x0B8)
#define PF_CFG2_REG         		(gpio_base+0x0BC)
#define PF_CFG3_REG         		(gpio_base+0x0C0)
#define PF_DAT_REG         			(gpio_base+0x0C4)
#define PF_DRV0_REG        			(gpio_base+0x0C8)
#define PF_DRV1_REG        			(gpio_base+0x0CC)
#define PF_PULL0_REG     			(gpio_base+0x0D0)
#define PF_PULL1_REG     			(gpio_base+0x0D4)
#define PG_CFG0_REG        			(gpio_base+0x0D8)
#define PG_CFG1_REG           	    (gpio_base+0x0DC)
#define PG_CFG2_REG         		(gpio_base+0x0E0)
#define PG_CFG3_REG         		(gpio_base+0x0E4)
#define PG_DAT_REG         			(gpio_base+0x0E8)
#define PG_DRV0_REG        			(gpio_base+0x0EC)
#define PG_DRV1_REG        			(gpio_base+0x0F0)
#define PG_PULL0_REG     			(gpio_base+0x0F4)
#define PG_PULL1_REG     			(gpio_base+0x0F8)
#define PH_CFG0_REG         		(gpio_base+0x0FC)
#define PH_CFG1_REG           	    (gpio_base+0x100)
#define PH_CFG2_REG         		(gpio_base+0x104)
#define PH_CFG3_REG         		(gpio_base+0x108)
#define PH_DAT_REG         		    (gpio_base+0x10C)
#define PH_DRV0_REG        		    (gpio_base+0x110)
#define PH_DRV1_REG        		    (gpio_base+0x114)
#define PH_PULL0_REG     			(gpio_base+0x118)
#define PH_PULL1_REG     			(gpio_base+0x11C)
#define PI_CFG0_REG         		(gpio_base+0x120)
#define PI_CFG1_REG           	    (gpio_base+0x124)
#define PI_CFG2_REG         		(gpio_base+0x128)
#define PI_CFG3_REG         		(gpio_base+0x12C)
#define PI_DAT_REG         		    (gpio_base+0x130)
#define PI_DRV0_REG        		    (gpio_base+0x134)
#define PI_DRV1_REG        		    (gpio_base+0x138)
#define PI_PULL0_REG     			(gpio_base+0x13C)
#define PI_PULL1_REG     			(gpio_base+0x140) 

//SRAMC register
#define SRAMC_BASE              0x01c00000
#define SRAMC_CFG_REG           (SRAMC_BASE+0x00)
#define SRAMC_ITCM_AWC_REG      (SRAMC_BASE+0xb4)

static void __iomem* gpio_base = NULL;

static inline void aw_gpio_set_trigger_pio(void)
{
	u32 rval = sdc_read(PI_CFG0_REG);
	
	rval &= ~ 0x7;
	rval &= ~ (0x7 << 4);
	rval |= 0x1;
	rval |= 0x1 << 4;
	sdc_write(PI_CFG0_REG, rval);
	
	rval = sdc_read(PI_DAT_REG);
	rval &= ~3;
	rval |= 3;
	sdc_write(PI_DAT_REG, rval);
}

static inline void aw_gpio_trigger_single(void)
{
	u32 rval = sdc_read(PI_DAT_REG);
	
	rval &= ~1;
	sdc_write(PI_DAT_REG, rval);
	rval |= 1;
	sdc_write(PI_DAT_REG, rval);
}

static inline void aw_gpio_trigger_single1(void)
{
	u32 rval = sdc_read(PI_DAT_REG);
	
	rval &= ~2;
	sdc_write(PI_DAT_REG, rval);
	rval |= 2;
	sdc_write(PI_DAT_REG, rval);
}

static __inline void smc_syscall_ioremap(void)
{
    gpio_base = (void __iomem*)SW_VA_PORTC_IO_BASE;
    #ifdef AW1623_FPGA
    sdc_write(PC_CFG0_REG, 0x33333333);
    sdc_write(PC_CFG1_REG, 0x33333333);
    sdc_write(PC_CFG2_REG, 0x33333333);
    sdc_write(PC_CFG3_REG, 0x33333333);
    #endif
//    aw_gpio_set_trigger_pio();
}

static __inline void smc_syscall_iounremap(void)
{
    return;
}

#endif
