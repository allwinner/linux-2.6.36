/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_spi_mx27.h
 * @brief This header file contains SPI driver low level register definitions for MX27.
 *
 * @ingroup SPI
 */

#ifndef __MXC_SPI_MX27_H__
#define __MXC_SPI_MX27_H__

#include <asm/hardware.h>
#include <asm/mach-types.h>
 

enum mx27_gpio_reg {
        GPIO_DR = 0x1C,
        GPIO_GDIR = 0x00,
        GPIO_SSR = 0x24,
        GPIO_ICR1 = 0x28,
        GPIO_ICR2 = 0x2C,
        GPIO_IMR = 0x30,
        GPIO_ISR = 0x34,
        GPIO_IUS = 0x20,
};

#define MXC_CSPIRXDATA		0x00
#define MXC_CSPITXDATA		0x04
#define MXC_CSPICTRL		0x08
#define MXC_CSPIINT			0x0C
#define MXC_CSPIDMA			0x18
#define MXC_CSPISTAT		0x0C
#define MXC_CSPIPERIOD		0x14
#define MXC_CSPITEST		0x10
#define MXC_CSPIRESET		0x1C

#define MXC_CSPICTRL_SDHC_SPIEN		(1 << 22)
#define MXC_CSPICTRL_SDHC_SPIDIS	0x0
#define MXC_CSPICTRL_NOBURST		(1 << 23)
#define MXC_CSPICTRL_BURST			0x0
#define MXC_CSPICTRL_SWAP			(1 << 23)
#define MXC_CSPICTRL_NOSWAP			0x0
#define MXC_CSPICTRL_ENABLE			(1 << 10)
#define MXC_CSPICTRL_DISABLE		0x0
#define MXC_CSPICTRL_MASTER			(1 << 11)
#define MXC_CSPICTRL_SLAVE			0x0
#define MXC_CSPICTRL_XCH			(1 << 9)
#define MXC_CSPICTRL_LOWPOL			(1 << 5)
#define MXC_CSPICTRL_HIGHPOL		0x0
#define MXC_CSPICTRL_PHA			(1 << 6)
#define MXC_CSPICTRL_NOPHA			0x0
#define MXC_CSPICTRL_SSCTL			(1 << 7)
#define MXC_CSPICTRL_HIGHSSPOL 		(1 << 8)
#define MXC_CSPICTRL_LOWSSPOL		0x0
#define MXC_CSPICTRL_CSMASK			0x3
#define MXC_CSPICTRL_MAXDATRATE		0x10
#define MXC_CSPICTRL_DATAMASK		0x1F
#define MXC_CSPICTRL_DATASHIFT 		14
/* This adjustment in the shift is valid only for even states only(i.e. divide 
   ratio of 2). SDHC_SPIEN is not set by default. If SDHC_SPIEN bit is set in 
   MXC_CSPICTRL, then divide ratio is 3, this shift adjustment is invalid. */
#define MXC_CSPICTRL_ADJUST_SHIFT(x) ((x) = ((x) - 1) * 2)

#define MXC_CSPICTRL_CSSHIFT_0_7	12
#define MXC_CSPICTRL_BCSHIFT_0_7	20
#define MXC_CSPICTRL_BCMASK_0_7		0xFFF
#define MXC_CSPICTRL_DRCTRLSHIFT_0_7 8

#define MXC_CSPICTRL_CSSHIFT_0_5	12
#define MXC_CSPICTRL_BCSHIFT_0_5	20
#define MXC_CSPICTRL_BCMASK_0_5		0xFFF
#define MXC_CSPICTRL_DRCTRLSHIFT_0_5 8

#define MXC_CSPICTRL_CSSHIFT_0_4	24
#define MXC_CSPICTRL_BCSHIFT_0_4	8
#define MXC_CSPICTRL_BCMASK_0_4		0x1F
#define MXC_CSPICTRL_DRCTRLSHIFT_0_4 20

#define MXC_CSPICTRL_CSSHIFT_0_0	19
#define MXC_CSPICTRL_BCSHIFT_0_0	0
#define MXC_CSPICTRL_BCMASK_0_0		0x1F
#define MXC_CSPICTRL_DRCTRLSHIFT_0_0 12

#define MXC_CSPIINT_IRQSHIFT_0_7	8
#define MXC_CSPIINT_IRQSHIFT_0_5	9
#define MXC_CSPIINT_IRQSHIFT_0_4	9
#define MXC_CSPIINT_IRQSHIFT_0_0	18
#define MXC_CSPIINT_TEEN			(1 << 9)
#define MXC_CSPIINT_THEN			(1 << 10)
#define MXC_CSPIINT_TFEN			(1 << 11)
#define MXC_CSPIINT_RREN			(1 << 13)
#define MXC_CSPIINT_RHEN		  	(1 << 14)
#define MXC_CSPIINT_RFEN			(1 << 15)
#define MXC_CSPIINT_ROEN			(1 << 16)
#define MXC_CSPIINT_TCEN_0_7		(1 << 7)
#define MXC_CSPIINT_TCEN_0_5		(1 << 8)
#define MXC_CSPIINT_TCEN_0_4		(1 << 8)
#define MXC_CSPIINT_TCEN_0_0		(1 << 12) /* Actually TSHFEEN */
#define MXC_CSPIINT_BOEN_0_7		0
#define MXC_CSPIINT_BOEN_0_5		(1 << 7)
#define MXC_CSPIINT_BOEN_0_4		(1 << 7)
#define MXC_CSPIINT_BOEN_0_0		(1 << 17)

#define MXC_CSPIINT_TSHFE			(1 << 3)
#define MXC_CSPIINT_RR				(1 << 4)
#define MXC_CSPIINT_TE				(1 << 0)

/* These bits resides in the interrupt control register of the MX27
	Probably as mistake by freescale when porting from MX31

#define MXC_CSPISTAT_TE				(1 << 0)
#define MXC_CSPISTAT_TH				(1 << 1)
#define MXC_CSPISTAT_TF				(1 << 2)
#define MXC_CSPISTAT_RR				(1 << 3)
#define MXC_CSPISTAT_RH				(1 << 4)
#define MXC_CSPISTAT_RF				(1 << 5)
#define MXC_CSPISTAT_RO				(1 << 6)
#define MXC_CSPISTAT_TC_0_7			(1 << 7)
#define MXC_CSPISTAT_TC_0_5			(1 << 8)
#define MXC_CSPISTAT_TC_0_4			(1 << 8)
#define MXC_CSPISTAT_TC_0_0			(1 << 3)
#define MXC_CSPISTAT_BO_0_7			0
#define MXC_CSPISTAT_BO_0_5			(1 << 7)
#define MXC_CSPISTAT_BO_0_4			(1 << 7)
#define MXC_CSPISTAT_BO_0_0			(1 << 8)
*/




#define MXC_CSPIRXDATA_MASK			0x000000FF
#define MXC_CSPITXDATA_MASK			0x000000FF

#define MXC_CSPITEST_TXCNTMASK		0xF
#define MXC_CSPITEST_RXCNTMASK		0xF0
#define MXC_CSPITEST_LBC			(1 << 14)

#define MXC_CSPIPERIOD_32KHZ		(1 << 15)

#define MXC_CSPITEST_LBC			(1 << 14)

/*!
 * @struct mxc_spi_unique_def
 * @brief This structure contains information that differs with
 * SPI master controller hardware version
 */
struct mxc_spi_unique_def {
	/*!
	 * Width of valid bits in MXC_CSPIINT.
	 */
	unsigned int intr_bit_shift;
	/*!
	 * Chip Select shift.
	 */
	unsigned int cs_shift;
	/*!
	 * Bit count shift.
	 */
	unsigned int bc_shift;
	/*!
	 * Bit count mask.
	 */
	unsigned int bc_mask;
	/*!
	 * Data Control shift.
	 */
	unsigned int drctrl_shift;
	/*!
	 * Transfer Complete shift. 
	 */
	unsigned int xfer_complete;
	/*!
	 * Bit counnter overflow shift.
	 */
	unsigned int bc_overflow;
};



#endif				//__MXC_SPI_MX27_H__
