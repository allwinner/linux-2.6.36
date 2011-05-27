/************************************************************************
*	TCC7xx Digital Audio Player
*	------------------------------------------------
*
*	FUNCTION	:
*	MODEL		: TCC7xx DAP
*	CPU NAME	: TCC7xx
*	SOURCE		: IO_TCC7XX.h
*
*	START DATE	: 2004 DEC. 8
*	MODIFY DATE : 2005 JUN. 4
*	DEVISION	: DEPT. BT 3 TEAM
*				: TELECHIPS, INC.
************************************************************************/

#ifndef	__IO_TCC7XX_H
#define	__IO_TCC7XX_H

//#define	USE_IO_DEBUG

#if (defined(SDRAM_SIZE) && (SDRAM_SIZE > 0x200000))
#define	DBG_MAXSTR_TX	0x100000
#else
	#define	DBG_MAXSTR_TX	0x10
#endif
#define	DBG_BUFMASK_TX	(DBG_MAXSTR_TX - 1)
#define	DBG_MAXSTR_RX	0x10

//#define	USE_DOMEASURE
//#define	CHECK_SPEED
#define	LOW_FREQ_PLL_INCLUDE

// Select EHI Slave System among the followings.
	//#define	EHIS_TCC77X
	#define	EHIS_TCC75X

//#include "main.h"
#define TCC83XX
#define TCC8300_BOARD

#if defined(TCC77X)
	#include "TCC77x.h"
#elif defined(TCC87XX)
	#include "TCC87xx.h"
#elif defined(TCC82XX)
	#include "TCC82xx.h"
#elif defined(TCC83XX)
	#include "tcc83x_virt.h"
#endif


#ifdef TCC77X
	#define	HwSDR_FIX			0x02E97000
#elif defined(TCC87XX)
	#define	HwSDR_FIX			0x02E51000
#elif defined(TCC82XX)
	#if defined(TCC8200_BOARD)
		#define	HwSDR_FIX		0x02E97000
	#elif defined(TCC822X_BOARD)
		#define	HwSDR_FIX		0x02E51000
	#endif
#elif defined(TCC83XX)
	#if defined(TCC8300_BOARD)
		#define	HwSDR_FIX		0x02E97000
	#elif defined(TCC832X_BOARD)
		#define	HwSDR_FIX		0x02E51000
	#endif
#endif

// SDRAM BusWidth
#define	HwSDR_X16		Hw30
#define	HwSDR_X32		HwZERO

// SDRAM Size (Col, Row Address bus width)
#define	HwSDR_2MB		(HwSDCFG_CW8 + HwSDCFG_RW11)
#define	HwSDR_4MB		(HwSDCFG_CW8 + HwSDCFG_RW11)
#define	HwSDR_8MB		(HwSDCFG_CW8 + HwSDCFG_RW12)
#define	HwSDR_16MB		(HwSDCFG_CW9 + HwSDCFG_RW12)
#define	HwSDR_32MB		(HwSDCFG_CW9 + HwSDCFG_RW13)
#define	HwSDR_64MB		(HwSDCFG_CW10 + HwSDCFG_RW13)

// SDRAM Refresh cycle period factor
#define	HwSDR_RFR		1

#ifndef FALSE
	#define	FALSE		(0)
#endif
#ifndef TRUE
	#define	TRUE		(!FALSE)
#endif
#ifndef NULL
	#define	NULL		(0)
#endif

#if (defined(TELECHIPS_SV) && !defined(USE_IO_DEBUG))
	//#define	USE_IO_DEBUG
#endif

#ifndef SET
	#define	SET			1
#endif
#ifndef CLR
	#define	CLR			0
#endif

#ifndef U32
	typedef unsigned int U32;
#endif
#ifndef U16
	typedef unsigned short U16;
#endif
#ifndef U8
	typedef unsigned char U8;
#endif

#ifndef WINVER
	#ifndef BOOL
		typedef unsigned int BOOL;
	#endif
#endif

#ifndef S32
	typedef signed int S32;
#endif
#ifndef S16
	typedef signed short S16;
#endif
#ifndef S8
	typedef signed char S8;
#endif

/* =============================
   General Bit Operator
   ============================= */
// Bit manipulation macro that is modifying its argument. (task type)
#ifndef BITSET
#define	BITSET(X, MASK)				( (X) |= (U32)(MASK) )
#endif
#ifndef BITSCLR
#define	BITSCLR(X, SMASK, CMASK)	( (X) = ((((U32)(X)) | ((U32)(SMASK))) & ~((U32)(CMASK))) )
#endif
#ifndef BITCSET
#define	BITCSET(X, CMASK, SMASK)	( (X) = ((((U32)(X)) & ~((U32)(CMASK))) | ((U32)(SMASK))) )
#endif
#ifndef BITCLR
#define	BITCLR(X, MASK)				( (X) &= ~((U32)(MASK)) )
#endif
#ifndef BITXOR
#define	BITXOR(X, MASK)				( (X) ^= (U32)(MASK) )
#endif

// Bit manipulation macro that is not modifying its argument. (function type)
#ifndef fBITSET
#define	fBITSET(X, MASK)				( (X) | (U32)(MASK) )
#endif
#ifndef fBITSCLR
#define	fBITSCLR(X, SMASK, CMASK)	( ((((U32)(X)) | ((U32)(SMASK))) & ~((U32)(CMASK))) )
#endif
#ifndef fBITCSET
#define	fBITCSET(X, CMASK, SMASK)	( ((((U32)(X)) & ~((U32)(CMASK))) | ((U32)(SMASK))) )
#endif
#ifndef fBITCLR
#define	fBITCLR(X, MASK)				( (X) & ~((U32)(MASK)) )
#endif
#ifndef fBITXOR
#define	fBITXOR(X, MASK)				( (X) ^ (U32)(MASK) )
#endif

#ifndef ISSET
#define	ISSET(X, MASK)				( (U32)(X) & ((U32)(MASK)) )
#endif
#ifndef IS
#define	IS(X, MASK)					( (U32)(X) & ((U32)(MASK)) )
#endif
#ifndef ISONE
#define	ISONE(X, MASK)				( (U32)(X) & ((U32)(MASK)) )
#endif

#ifndef ISALLONE
#define	ISALLONE(X, MASK)			( ((U32)(X) & ((U32)(MASK))) == ((U32)(MASK)) )
#endif

#ifndef ISCLR
#define	ISCLR(X, MASK)				(  ! (((U32)(X)) & ((U32)(MASK))) )
#endif
#ifndef ISZERO
#define	ISZERO(X, MASK)				(  ! (((U32)(X)) & ((U32)(MASK))) )
#endif
#ifndef ISNOT
#define	ISNOT(X, MASK)				(  ! (((U32)(X)) & ((U32)(MASK))) )
#endif

#ifndef BYTE_OF
#define	BYTE_OF(X)					( *(volatile unsigned char *)(&(X)) )
#endif
#ifndef SHORT_OF
#define	SHORT_OF(X)					( *(volatile short *)(&(X)) )
#endif
#ifndef HWORD_OF
#define	HWORD_OF(X)				( *(volatile unsigned short *)(&(X)) )
#endif
#ifndef WORD_OF
#define	WORD_OF(X)					( *(volatile unsigned int *)(&(X)) )
#endif

#ifndef byte_of
#define	byte_of(X)					( *(volatile unsigned char *)((X)) )
#endif
#ifndef short_of
#define	short_of(X)					( *(volatile short *)((X)) )
#endif
#ifndef hword_of
#define	hword_of(X)					( *(volatile unsigned short *)((X)) )
#endif
#ifndef word_of
#define	word_of(X)					( *(volatile unsigned int *)((X)) )
#endif

#ifndef CkWaitPLLLOCK
#define	CkWaitPLLLOCK()		{ while (ISCLR(HwPLLMODE, Hw20)); }
#endif

#ifdef TCC77X
	#define	HwCSCFG(CS)	*(volatile U32 *)(0xF0000010 + ((CS)<<2))
	#define	McSetLCD(CS, BASE, BW, URDY, SETUP, PW, HOLD)	\
		HwCSCFG(CS)	= (										\
			((3) << 26) |							\
			(((BW)^((HwMCFG&0x1800)>>11)) << 28) |			\
			((BASE)<<22) |									\
			((URDY)<<21) |									\
			((1<<20)) |										\
			(((SETUP)&7)<<11) |								\
			(((PW)&0xFF)<<3) |									\
			((HOLD)&7)										\
		)
#endif

/************************************************************************
*	Clock Controller
************************************************************************/
typedef struct {
	unsigned			uFpll;
	unsigned char		P, M, S, dummy;
} sPLL;


enum {
	XIN_FREQ_120000,
	XIN_FREQ_480000
};

enum {
	IO_CKC_Fuhost	= 0,
	IO_CKC_Fmsc,
	IO_CKC_Fi2c,
	IO_CKC_Fuart0,
	IO_CKC_Fuart1,
	IO_CKC_Fuart2,	// TCC77X only
	IO_CKC_Fgsio,
	IO_CKC_Ftimert,
	IO_CKC_Ftimerx,
	IO_CKC_Ftimerz,
	IO_CKC_Fadc,
	IO_CKC_Fdai,
	IO_CKC_Frfr,	// SDRAM
	IO_CKC_Fsdmmc,
	IO_CKC_Fspdif,
	IO_CKC_Fspis,
	IO_CKC_Flcd,
	IO_CKC_Fcam,
	IO_CKC_FcamScaler,
#ifdef TCC_EHI_CLK_USED
	IO_CKC_Fehi,
#endif
#ifdef TCC83XX
	IO_CKC_Fuart3,
	IO_CKC_Fspims0,
	IO_CKC_Fspims1,
#endif
	IO_CKC_Flast
};

#ifdef TCC77X
	#define	IO_CKC_HwPCLKCFG_EN_1		Hw15
	#define	IO_CKC_HwPCLKCFG_SEL_1		Hw12
	#define	IO_CKC_GetDIV(X)			(((X) & 0x7FF) + 1)
	#define	IO_CKC_HwCLKDIVC_P0E_EN	Hw31
	#define	IO_CKC_HwCLKDIVC_P0P_EN	Hw30
	#define	IO_CKC_HwCLKDIVC_P1E_EN	Hw31
	#define	IO_CKC_HwCLKDIVC_P1P_EN	Hw30
	#define	IO_CKC_PLL0					0
	#define	IO_CKC_PLL1					0
	#define	IO_CKC_HwCLKDIVC_P0DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P0P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC_P1DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P1P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC1			0
#endif
#ifdef TCC87XX
	#define	IO_CKC_HwPCLKCFG_EN_1		Hw15
	#define	IO_CKC_HwPCLKCFG_SEL_1		Hw12
	#define	IO_CKC_GetDIV(X)			(((X) & 0x7FF) + 1)
	#define	IO_CKC_HwCLKDIVC_P0E_EN		Hw31
	#define	IO_CKC_HwCLKDIVC_P0P_EN		Hw30
	#define	IO_CKC_HwCLKDIVC_P1E_EN		Hw31
	#define	IO_CKC_HwCLKDIVC_P1P_EN		Hw30
	#define	IO_CKC_PLL0					0
	#define	IO_CKC_PLL1					0
	#define	IO_CKC_HwCLKDIVC_P0DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P0P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC_P1DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P1P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC1			0
#endif
#ifdef TCC82XX
	#define	IO_CKC_HwPCLKCFG_EN_1		Hw15
	#define	IO_CKC_HwPCLKCFG_SEL_1		Hw12
	#define	IO_CKC_GetDIV(X)			(((X) & 0x7FF) + 1)
	#define	IO_CKC_HwCLKDIVC_P0E_EN		Hw31
	#define	IO_CKC_HwCLKDIVC_P0P_EN		Hw30
	#define	IO_CKC_HwCLKDIVC_P1E_EN		Hw31
	#define	IO_CKC_HwCLKDIVC_P1P_EN		Hw30
	#define	IO_CKC_PLL0					0
	#define	IO_CKC_PLL1					0
	#define	IO_CKC_HwCLKDIVC_P0DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P0P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC_P1DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P1P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC1			0
#endif
#ifdef TCC83XX
	#define	IO_CKC_HwPCLKCFG_EN_1		Hw15
	#define	IO_CKC_HwPCLKCFG_SEL_1		Hw12
	#define	IO_CKC_GetDIV(X)			(((X) & 0x7FF) + 1)
	#define	IO_CKC_HwCLKDIVC_P0E_EN		Hw31
	#define	IO_CKC_HwCLKDIVC_P0P_EN		Hw30
	#define	IO_CKC_HwCLKDIVC_P1E_EN		Hw31
	#define	IO_CKC_HwCLKDIVC_P1P_EN		Hw30
	#define	IO_CKC_PLL0					0
	#define	IO_CKC_PLL1					0
	#define	IO_CKC_HwCLKDIVC_P0DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P0P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC_P1DIV(X)	\
		(ISONE(X, IO_CKC_HwCLKDIVC_P1P_EN) ? 2 * ((((X) >> 24) & 0x1F)+1) : ((((X) >> 24) & 0x1F)+1))
	#define	IO_CKC_HwCLKDIVC1			0
#endif /* TCC83XX */

#define	IO_CKC_HwCLKDIVC_XE_EN	HwCLKDIVC_XE_EN
#define	IO_CKC_HwCLKDIVC_XTE_EN	HwCLKDIVC_XTE_EN
#define	IO_CKC_HwCLKDIVC1_P0E_EN	Hw7
#define	IO_CKC_HwCLKDIVC1_P0P_EN	Hw6
#define	IO_CKC_HwCLKDIVC1_P1E_EN	Hw15
#define	IO_CKC_HwCLKDIVC1_P1P_EN	Hw14
#define	IO_CKC_HwCLKDIVC_XDIV(X)	\
	(ISONE(X, HwCLKDIVC_XP_EN) ? 2 * ((((X) >> 8) & 0x1F)+1) : ((((X) >> 8) & 0x1F)+1))
#define	IO_CKC_HwCLKDIVC_XTDIV(X)	\
	(ISONE(X, HwCLKDIVC_XTP_EN) ? 2 * ((((X) >> 0) & 0x1F)+1) : ((((X) >> 0) & 0x1F)+1))
#define	IO_CKC_HwCLKDIVC1_P0DIV(X)	\
	(ISONE(X, IO_CKC_HwCLKDIVC1_P0P_EN) ? 2 * ((((X) >> 0) & 0x1F)+1) : ((((X) >> 0) & 0x1F)+1))
#define	IO_CKC_HwCLKDIVC1_P1DIV(X)	\
	(ISONE(X, IO_CKC_HwCLKDIVC1_P1P_EN) ? 2 * ((((X) >> 8) & 0x1F)+1) : ((((X) >> 8) & 0x1F)+1))

/**********************************************************
*	void IO_CKC_WaitPLL(void);
*	Input		:
*	Return		:
*	Description	: Loop waiting PLL locked.
**********************************************************/
#define	IO_CKC_WaitPLL()				{ int i; for (i=0; i<0x1000; i++); }

/**********************************************************
*	void IO_CKC_BUS_SET(unsigned X);
*	Input		: X = Bitmap dedicated to each peripheral
*	Return		:
*	Description	: Enable(1)/Disable(0) the bus clock fed to each peripheral.
*				  Set bitmap of each peri to enable bus clock.
**********************************************************/
#define	IO_CKC_BUS_SET(X)			(HwBCLKCTR		= (X))

/**********************************************************
*	void IO_CKC_EnableBUS(unsigned X);
*	Input		: X = Bitmap dedicated to each peripheral
*	Return		:
*	Description	: Enable(1) the bus clock fed to each peripheral.
*				  The other bus clocks are not influenced.
**********************************************************/
#define	IO_CKC_EnableBUS(X)			(BITSET(HwBCLKCTR, (X)))

/**********************************************************
*	Derivatives of IO_CKC_EnableBUS() function.
*	Input		:
*	Return		:
*	Description	: Enable the bus clock fed to each peripheral.
**********************************************************/
#define	IO_CKC_EnableBUS_DAI()		(BITSET(HwBCLKCTR, HwBCLKCTR_DAI_ON))
#define	IO_CKC_EnableBUS_TMR()		(BITSET(HwBCLKCTR, HwBCLKCTR_TMR_ON))
#define	IO_CKC_EnableBUS_UB()		(BITSET(HwBCLKCTR, HwBCLKCTR_UB_ON))
#define	IO_CKC_EnableBUS_GS()		(BITSET(HwBCLKCTR, HwBCLKCTR_GS_ON))
#define	IO_CKC_EnableBUS_I2C()		(BITSET(HwBCLKCTR, HwBCLKCTR_I2C_ON))
#define	IO_CKC_EnableBUS_ECC()		(BITSET(HwBCLKCTR, HwBCLKCTR_ECC_ON))
#define	IO_CKC_EnableBUS_UBH()		(BITSET(HwBCLKCTR, HwBCLKCTR_UBH_ON))
#define	IO_CKC_EnableBUS_RTC()		(BITSET(HwBCLKCTR, HwBCLKCTR_RTC_ON))
#define	IO_CKC_EnableBUS_NFC()		(BITSET(HwBCLKCTR, HwBCLKCTR_ND_ON))
#define	IO_CKC_EnableBUS_SDC()		(BITSET(HwBCLKCTR, HwBCLKCTR_SD_ON))
#define	IO_CKC_EnableBUS_MSC()		(BITSET(HwBCLKCTR, HwBCLKCTR_MS_ON))
#define	IO_CKC_EnableBUS_EHI()		(BITSET(HwBCLKCTR, HwBCLKCTR_EHI_ON))
#define	IO_CKC_EnableBUS_UA0()		(BITSET(HwBCLKCTR, HwBCLKCTR_UA0_ON))
#define	IO_CKC_EnableBUS_UA1()		(BITSET(HwBCLKCTR, HwBCLKCTR_UA1_ON))
#define	IO_CKC_EnableBUS_SPI0()		(BITSET(HwBCLKCTR, HwBCLKCTR_SPIMS0_ON))
#define	IO_CKC_EnableBUS_SPI1()		(BITSET(HwBCLKCTR, HwBCLKCTR_SPIMS1_ON))
#define	IO_CKC_EnableBUS_UART()		(BITSET(HwBCLKCTR, HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON))
#define	IO_CKC_EnableBUS_INT()		(BITSET(HwBCLKCTR, HwBCLKCTR_INT_ON))
#define	IO_CKC_EnableBUS_DMA()		(BITSET(HwBCLKCTR, HwBCLKCTR_DMA3ch_ON ))
#define	IO_CKC_EnableBUS_GPIO()		(BITSET(HwBCLKCTR, HwBCLKCTR_GPIO_ON))
#define	IO_CKC_EnableBUS_UA2()		(BITSET(HwBCLKCTR, HwBCLKCTR_UA2_ON))
#define	IO_CKC_EnableBUS_usbDMA()	(BITSET(HwBCLKCTR, HwBCLKCTR_usbDMA_ON))
#define	IO_CKC_EnableBUS_CAM()		(BITSET(HwBCLKCTR, HwBCLKCTR_CAM_ON))
#define IO_CKC_EnableBUS_JPEG()		(BITSET(HwBCLKCTR, HwBCLKCTR_JPEG_ON))
#define IO_CKC_EnableBUS_MSCL()		(BITSET(HwBCLKCTR, HwBCLKCTR_MSCL_ON)) // nahwon
#define IO_CKC_EnableBUS_LCD()		(BITSET(HwBCLKCTR, HwBCLKCTR_LCD_ON))
#define IO_CKC_EnableBUS_G2D()		(BITSET(HwBCLKCTR, HwBCLKCTR_2D_ON))	
#define IO_CKC_EnableBUS_ADMA()		(BITSET(HwBCLKCTR1, HwBCLKCTR1_ADMA))

/**********************************************************
*	void IO_CKC_DisableBUS(unsigned X);
*	Input		: X = Bitmap dedicated to each peripheral
*	Return		:
*	Description	: Disable(1) the bus clock fed to each peripheral.
*				  The other bus clocks are not influenced.
**********************************************************/
#define	IO_CKC_DisableBUS(X)			(BITCLR(HwBCLKCTR, X))

/**********************************************************
*	Derivatives of IO_CKC_DisableBUS() function.
*	Input		:
*	Return		:
*	Description	: Disable the bus clock fed to each peripheral.
**********************************************************/
#define	IO_CKC_DisableBUS_DAI()			(BITCLR(HwBCLKCTR, HwBCLKCTR_DAI_ON))
#define	IO_CKC_DisableBUS_TMR()			(BITCLR(HwBCLKCTR, HwBCLKCTR_TMR_ON))
#define	IO_CKC_DisableBUS_UB()			(BITCLR(HwBCLKCTR, HwBCLKCTR_UB_ON))
#define	IO_CKC_DisableBUS_UA0()			(BITCLR(HwBCLKCTR, HwBCLKCTR_UA0_ON))
#define	IO_CKC_DisableBUS_GS()			(BITCLR(HwBCLKCTR, HwBCLKCTR_GS_ON))
#define	IO_CKC_DisableBUS_I2C()			(BITCLR(HwBCLKCTR, HwBCLKCTR_I2C_ON))
#define	IO_CKC_DisableBUS_ECC()			(BITCLR(HwBCLKCTR, HwBCLKCTR_ECC_ON))
#define	IO_CKC_DisableBUS_UBH()			(BITCLR(HwBCLKCTR, HwBCLKCTR_UBH_ON))
#define	IO_CKC_DisableBUS_RTC()			(BITCLR(HwBCLKCTR, HwBCLKCTR_RTC_ON))
#define	IO_CKC_DisableBUS_NFC()			(BITCLR(HwBCLKCTR, HwBCLKCTR_ND_ON))
#define	IO_CKC_DisableBUS_SDC()			(BITCLR(HwBCLKCTR, HwBCLKCTR_SD_ON))
#define	IO_CKC_DisableBUS_MSC()			(BITCLR(HwBCLKCTR, HwBCLKCTR_MS_ON))
#define	IO_CKC_DisableBUS_EHI()			(BITCLR(HwBCLKCTR, HwBCLKCTR_EHI_ON))
#define	IO_CKC_DisableBUS_UA1()			(BITCLR(HwBCLKCTR, HwBCLKCTR_UA1_ON))
#define	IO_CKC_DisableBUS_SPI()			(BITCLR(HwBCLKCTR, HwBCLKCTR_SPI_ON))
#if defined(TCC77X)
	#define	IO_CKC_DisableBUS_UART()	(BITCLR(HwBCLKCTR, HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_INT()		(BITCLR(HwBCLKCTR, HwBCLKCTR_INT_ON))
	#define	IO_CKC_DisableBUS_DMA()		(BITCLR(HwBCLKCTR, HwBCLKCTR_DMA2ch_ON | HwBCLKCTR_DMA1ch_ON))
	#define	IO_CKC_DisableBUS_UDMA()	(BITCLR(HwBCLKCTR, HwBCLKCTR_UDMA_ON))
	#define	IO_CKC_DisableBUS_GPIO()	(BITCLR(HwBCLKCTR, HwBCLKCTR_GPIO_ON))
	#define	IO_CKC_DisableBUS_UA2()		(BITCLR(HwBCLKCTR, HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_usbDMA()	(BITCLR(HwBCLKCTR, HwBCLKCTR_usbDMA_ON))
#elif defined(TCC87XX)
	#define	IO_CKC_DisableBUS_UART()	(BITCLR(HwBCLKCTR, HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_INT()		(BITCLR(HwBCLKCTR, HwBCLKCTR_INT_ON))
	#define	IO_CKC_DisableBUS_DMA()		(BITCLR(HwBCLKCTR, HwBCLKCTR_DMA2ch_ON | HwBCLKCTR_DMA1ch_ON))
	#define	IO_CKC_DisableBUS_UDMA()	(BITCLR(HwBCLKCTR, HwBCLKCTR_UDMA_ON))
	#define	IO_CKC_DisableBUS_GPIO()	(BITCLR(HwBCLKCTR, HwBCLKCTR_GPIO_ON))
	#define	IO_CKC_DisableBUS_UA2()		(BITCLR(HwBCLKCTR, HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_usbDMA()	(BITCLR(HwBCLKCTR, HwBCLKCTR_usbDMA_ON))
	#define IO_CKC_DisableBUS_LCD()		//TBD. bspark_014, 20061204
#elif defined(TCC82XX)
	#define	IO_CKC_DisableBUS_UART()	(BITCLR(HwBCLKCTR, HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_INT()		(BITCLR(HwBCLKCTR, HwBCLKCTR_INT_ON))
	#define	IO_CKC_DisableBUS_DMA()		(BITCLR(HwBCLKCTR, HwBCLKCTR_DMA2ch_ON | HwBCLKCTR_DMA1ch_ON))
	#define	IO_CKC_DisableBUS_UDMA()	
	#define	IO_CKC_DisableBUS_GPIO()	(BITCLR(HwBCLKCTR, HwBCLKCTR_GPIO_ON))
	#define	IO_CKC_DisableBUS_UA2()		
	#define	IO_CKC_DisableBUS_usbDMA()	(BITCLR(HwBCLKCTR, HwBCLKCTR_usbDMA_ON))
	#define	IO_CKC_DisableBUS_CAM()		(BITCLR(HwBCLKCTR, HwBCLKCTR_CAM_ON))  // hjb
	#define IO_CKC_DisableBUS_JPEG()	//(BITCLR(HwBCLKCTR, HwBCLKCTR_JPEG_ON))
	#define IO_CKC_DisableBUS_LCD()		(BITCLR(HwBCLKCTR, HwBCLKCTR_LCD_ON))
	#define IO_CKC_DisableBUS_MSCL()	(BITCLR(HwBCLKCTR, HwBCLKCTR_MSCL_ON))
	#define IO_CKC_DisableBUS_G2D()		(BITCLR(HwBCLKCTR, HwBCLKCTR_2D_ON))
#elif defined(TCC83XX)
	#define	IO_CKC_DisableBUS_UART()	(BITCLR(HwBCLKCTR, HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_INT()		(BITCLR(HwBCLKCTR, HwBCLKCTR_INT_ON))
	#define	IO_CKC_DisableBUS_DMA()		(BITCLR(HwBCLKCTR, HwBCLKCTR_DMA3ch_ON)
	#define	IO_CKC_DisableBUS_GPIO()	(BITCLR(HwBCLKCTR, HwBCLKCTR_GPIO_ON))
	#define	IO_CKC_DisableBUS_UA2()		(BITCLR(HwBCLKCTR, HwBCLKCTR_UA2_ON))
	#define	IO_CKC_DisableBUS_usbDMA()	(BITCLR(HwBCLKCTR, HwBCLKCTR_usbDMA_ON))
	#define	IO_CKC_DisableBUS_CAM()		(BITCLR(HwBCLKCTR, HwBCLKCTR_CAM_ON))
	#define IO_CKC_DisableBUS_JPEG()	(BITCLR(HwBCLKCTR, HwBCLKCTR_JPEG_ON))
	#define IO_CKC_DisableBUS_MSCL()	(BITCLR(HwBCLKCTR, HwBCLKCTR_MSCL_ON))
	#define IO_CKC_DisableBUS_LCD()		(BITCLR(HwBCLKCTR, HwBCLKCTR_LCD_ON))
	#define IO_CKC_DisableBUS_G2D()		(BITCLR(HwBCLKCTR, HwBCLKCTR_2D_ON))
#endif

#define	IO_CKC_BUS_DAI				HwBCLKCTR_DAI_ON
#define	IO_CKC_BUS_TMR				HwBCLKCTR_TMR_ON
#define	IO_CKC_BUS_UB				HwBCLKCTR_UB_ON
#define	IO_CKC_BUS_UA0				HwBCLKCTR_UA0_ON
#define	IO_CKC_BUS_GS				HwBCLKCTR_GS_ON
#define	IO_CKC_BUS_I2C				HwBCLKCTR_I2C_ON
#define	IO_CKC_BUS_ECC				HwBCLKCTR_ECC_ON
#define	IO_CKC_BUS_UBH				HwBCLKCTR_UBH_ON
#define	IO_CKC_BUS_RTC				HwBCLKCTR_RTC_ON
#define	IO_CKC_BUS_NFC				HwBCLKCTR_ND_ON
#define	IO_CKC_BUS_SDC				HwBCLKCTR_SD_ON
#define	IO_CKC_BUS_MSC				HwBCLKCTR_MS_ON
#define	IO_CKC_BUS_EHI				HwBCLKCTR_EHI_ON
#define	IO_CKC_BUS_UA1				HwBCLKCTR_UA1_ON
#define	IO_CKC_BUS_SPI				HwBCLKCTR_SPI_ON
#define	IO_CKC_BUS_ADC				HwBCLKCTR_ADC_ON
#if defined(TCC77X)
	#define	IO_CKC_BUS_RSVD			0xFC000000
	#define	IO_CKC_BUS_UART			(HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON)
	#define	IO_CKC_BUS_INT			HwBCLKCTR_INT_ON
	#define	IO_CKC_BUS_DMA			(HwBCLKCTR_DMA2ch_ON | HwBCLKCTR_DMA1ch_ON)
	#define	IO_CKC_BUS_UDMA			HwBCLKCTR_UDMA_ON
	#define	IO_CKC_BUS_GPIO			HwBCLKCTR_GPIO_ON
	#define	IO_CKC_BUS_UA2			HwBCLKCTR_UA2_ON
	#define	IO_CKC_BUS_usbDMA		HwBCLKCTR_usbDMA_ON
	#define	IO_CKC_BUS_VideoH		0
	#define	IO_CKC_BUS_VideoC		0
	#define	IO_CKC_BUS_EMC			0
	#define	IO_CKC_BUS_SPDIF		0
	#define	IO_CKC_BUS_CAM			0
	#define	IO_CKC_BUS_LCD			0
#elif defined(TCC87XX)
	#define	IO_CKC_BUS_RSVD			0xFC000000
	#define	IO_CKC_BUS_UART			(HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON)
	#define	IO_CKC_BUS_INT			HwBCLKCTR_INT_ON
	#define	IO_CKC_BUS_DMA			HwBCLKCTR_DMA2ch_ON
	#define	IO_CKC_BUS_UDMA			0
	#define	IO_CKC_BUS_GPIO			HwBCLKCTR_GPIO_ON
	#define	IO_CKC_BUS_UA2			0
	#define	IO_CKC_BUS_usbDMA		HwBCLKCTR_usbDMA_ON
	#define	IO_CKC_BUS_VideoH		0
	#define	IO_CKC_BUS_VideoC		0
	#define	IO_CKC_BUS_EMC			0
	#define	IO_CKC_BUS_SPDIF		0
	#define	IO_CKC_BUS_CAM			0
	#define	IO_CKC_BUS_LCD			0
#elif defined(TCC82XX)
	#define	IO_CKC_BUS_RSVD			0xFC000000
	#define	IO_CKC_BUS_UART			(HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON)
	#define	IO_CKC_BUS_INT			HwBCLKCTR_INT_ON
	#define	IO_CKC_BUS_DMA			HwBCLKCTR_DMA2ch_ON
	#define	IO_CKC_BUS_UDMA			0
	#define	IO_CKC_BUS_GPIO			HwBCLKCTR_GPIO_ON
	#define	IO_CKC_BUS_UA2			0
	#define	IO_CKC_BUS_usbDMA		HwBCLKCTR_usbDMA_ON
	#define	IO_CKC_BUS_VideoH		0
	#define	IO_CKC_BUS_VideoC		0
	#define	IO_CKC_BUS_EMC			0
	#define	IO_CKC_BUS_SPDIF		0
	#define	IO_CKC_BUS_CAM			HwBCLKCTR_CAM_ON
	#define	IO_CKC_BUS_LCD			0
	#define IO_CKC_BUS_JPEG			HwBCLKCTR_JPEG_ON
	#define IO_CKC_BUS_MSCL 		HwBCLKCTR_MSCL_ON
#elif defined(TCC83XX)
	#define	IO_CKC_BUS_RSVD			0xFC000000
	#define	IO_CKC_BUS_UART			(HwBCLKCTR_UA0_ON | HwBCLKCTR_UA1_ON | HwBCLKCTR_UA2_ON)
	#define	IO_CKC_BUS_INT			HwBCLKCTR_INT_ON
	#define	IO_CKC_BUS_DMA			(HwBCLKCTR_DMA3ch_ON)
	#define	IO_CKC_BUS_UDMA			HwBCLKCTR_IDE_ON
	#define	IO_CKC_BUS_GPIO			HwBCLKCTR_GPIO_ON
	#define	IO_CKC_BUS_UA2			HwBCLKCTR_UA2_ON
	#define	IO_CKC_BUS_usbDMA		HwBCLKCTR_usbDMA_ON
	#define	IO_CKC_BUS_VideoH		0
	#define	IO_CKC_BUS_VideoC		0
	#define	IO_CKC_BUS_EMC			0
	#define	IO_CKC_BUS_SPDIF		0
	#define	IO_CKC_BUS_CAM			HwBCLKCTR_CAM_ON
	#define	IO_CKC_BUS_LCD			HwBCLKCTR_ND_ON
	#define IO_CKC_BUS_JPEG			HwBCLKCTR_JPEG_ON
	#define IO_CKC_BUS_MSCL			HwBCLKCTR_MSCL_ON
#endif


/**********************************************************
*	void IO_CKC_SWRST(unsigned X);
*	Input		: X = Bitmap dedicated to each peripheral
*	Return		:
*	Description	: Make a reset signal for each peripheral.
*				  Set a corresponding bit field to make a reset signal to each peripheral.
**********************************************************/
#define	IO_CKC_SWRST(X)			(HwSWRESET	= (X))

/**********************************************************
*	Derivatives of IO_CKC_SWRST() function.
*	Input		:
*	Return		:
*	Description	: Make a reset signal for each peripheral.
**********************************************************/
#define	IO_CKC_SWRST_DAI()			(HwSWRESET	= HwSWRESET_DAI_ON)
#define	IO_CKC_SWRST_INT()			(HwSWRESET	= HwSWRESET_INT_ON)
#define	IO_CKC_SWRST_TMR()			(HwSWRESET	= HwSWRESET_TMR_ON)
#define	IO_CKC_SWRST_GPIO()			(HwSWRESET	= HwSWRESET_GPIO_ON)
#define	IO_CKC_SWRST_UB()			(HwSWRESET	= HwSWRESET_UB_ON)
#define	IO_CKC_SWRST_UA0()			(HwSWRESET	= HwSWRESET_UA0_ON)
#define	IO_CKC_SWRST_GS()			(HwSWRESET	= HwSWRESET_GS_ON)
#define	IO_CKC_SWRST_I2C()			(HwSWRESET	= HwSWRESET_I2C_ON)
#define	IO_CKC_SWRST_ECC()			(HwSWRESET	= HwSWRESET_ECC_ON)
#define	IO_CKC_SWRST_UBH()			(HwSWRESET	= HwSWRESET_UBH_ON)
#define	IO_CKC_SWRST_DMA2ch()		(HwSWRESET	= HwSWRESET_DMA2ch_ON)
#define	IO_CKC_SWRST_RTC()			(HwSWRESET	= HwSWRESET_RTC_ON)
#define	IO_CKC_SWRST_NFC()			(HwSWRESET	= HwSWRESET_ND_ON)
#define	IO_CKC_SWRST_SDC()			(HwSWRESET	= HwSWRESET_SD_ON)
#define	IO_CKC_SWRST_MSC()			(HwSWRESET	= HwSWRESET_MS_ON)
#define	IO_CKC_SWRST_EHI()			(HwSWRESET	= HwSWRESET_EHI_ON)
#define	IO_CKC_SWRST_UDMA()			(HwSWRESET	= HwSWRESET_UDMA_ON)
#define	IO_CKC_SWRST_DMA1ch()		(HwSWRESET	= HwSWRESET_DMA1ch_ON)
#define	IO_CKC_SWRST_usbDMA()		(HwSWRESET	= HwSWRESET_usbDMA_ON)
#define	IO_CKC_SWRST_UA1()			(HwSWRESET	= HwSWRESET_UA1_ON)
#define	IO_CKC_SWRST_SPI()			(HwSWRESET	= HwSWRESET_SPI_ON)
#define	IO_CKC_SWRST_UA2()			(HwSWRESET	= HwSWRESET_UA2_ON)

enum {
	IO_CKC_RFR		= Hw0,
	IO_CKC_USBH		= Hw1,
	IO_CKC_I2C		= Hw2,
	IO_CKC_MSC		= Hw3,
	IO_CKC_UA2		= Hw4,
	IO_CKC_UA1		= Hw5,
	IO_CKC_UA0		= Hw6,
	IO_CKC_TMRT		= Hw7,
	IO_CKC_GSIO		= Hw8,
	IO_CKC_TMRZ		= Hw9,
	IO_CKC_TMRX		= Hw10,
	IO_CKC_DAI		= Hw11,
	IO_CKC_ADC		= Hw12,
	IO_CKC_SDMMC	= Hw13,
	IO_CKC_LCD		= Hw14,
	IO_CKC_CAM		= Hw15,
	IO_CKC_SPDIF	= Hw16,
	IO_CKC_SPIS		= Hw17
};

#ifdef TCC77X
	#define	IO_CKC_EnableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITSET(HwPCLKCFG0, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_USBH)							\
			BITSET(HwPCLKCFG0, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_I2C)								\
			BITSET(HwPCLKCFG1, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_MSC)								\
			BITSET(HwPCLKCFG1, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_UA2)								\
			BITSET(HwPCLKCFG2, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_UA1)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_GSIO)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_ADC)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG_EN0_ON);		\
	}

	#define	IO_CKC_DisableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITCLR(HwPCLKCFG0, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_USBH)							\
			BITCLR(HwPCLKCFG0, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_I2C)								\
			BITCLR(HwPCLKCFG1, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_MSC)								\
			BITCLR(HwPCLKCFG1, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_UA2)								\
			BITCLR(HwPCLKCFG2, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_UA1)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_GSIO)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_ADC)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG_EN0_ON);		\
	}
#endif

#ifdef TCC87XX
	#define	IO_CKC_EnableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITSET(HwPCLKCFG0, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_I2C)								\
			BITSET(HwPCLKCFG1, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA1)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_GSIO)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_ADC)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG_EN0_ON);		\
	}

	#define	IO_CKC_DisableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITCLR(HwPCLKCFG0, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_I2C)								\
			BITCLR(HwPCLKCFG1, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA1)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_GSIO)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_ADC)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG_EN0_ON);		\
	}
#endif

#ifdef TCC82XX
	#define	IO_CKC_EnableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITSET(HwPCLKCFG0, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_I2C)								\
			BITSET(HwPCLKCFG1, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA1)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_GSIO)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_ADC)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG_EN0_ON);		\
	}

	#define	IO_CKC_DisableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITCLR(HwPCLKCFG0, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_I2C)								\
			BITCLR(HwPCLKCFG1, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA1)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_GSIO)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG_EN1_ON);		\
		if (x & IO_CKC_ADC)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG_EN0_ON);		\
	}
#endif

#ifdef TCC83XX
	#define	IO_CKC_EnableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITSET(HwPCLKCFG0, HwPCLKCFG0_EN1_ON);		\
		if (x & IO_CKC_I2C)								\
			BITSET(HwPCLKCFG1, HwPCLKCFG1_EN1_ON);		\
		if (x & IO_CKC_UA1)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG3_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITSET(HwPCLKCFG3, HwPCLKCFG3_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITSET(HwPCLKCFG4, HwPCLKCFG4_EN1_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG5_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITSET(HwPCLKCFG5, HwPCLKCFG5_EN0_ON);		\
		if (x & IO_CKC_DAI) {							\
			BITSET(HwPCLKCFG7, HwPCLKCFG7_DSEL_DCLK1);	\
			BITSET(HwPCLKCFG7, HwPCLKCFG7_EN_ON);		\
		}												\
		if (x & IO_CKC_ADC)								\
			BITSET(HwPCLKCFG6, HwPCLKCFG6_EN0_ON);		\
	}

	#define	IO_CKC_DisableClock(x)						\
	{													\
		if (x & IO_CKC_RFR)								\
			BITCLR(HwPCLKCFG0, HwPCLKCFG0_EN1_ON);		\
		if (x & IO_CKC_I2C)								\
			BITCLR(HwPCLKCFG1, HwPCLKCFG1_EN1_ON);		\
		if (x & IO_CKC_UA1)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG3_EN1_ON);		\
		if (x & IO_CKC_UA0)								\
			BITCLR(HwPCLKCFG3, HwPCLKCFG3_EN0_ON);		\
		if (x & IO_CKC_TMRT)							\
			BITCLR(HwPCLKCFG4, HwPCLKCFG4_EN1_ON);		\
		if (x & IO_CKC_TMRZ)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG5_EN1_ON);		\
		if (x & IO_CKC_TMRX)							\
			BITCLR(HwPCLKCFG5, HwPCLKCFG5_EN0_ON);		\
		if (x & IO_CKC_DAI)								\
			BITCLR(HwPCLKCFG7, HwPCLKCFG7_EN_ON);		\
		if (x & IO_CKC_ADC)								\
			BITCLR(HwPCLKCFG6, HwPCLKCFG6_EN0_ON);		\
	}
#endif /* TCC83XX */

/************************************************************************
*	Function Declaration (Clock Controller)
************************************************************************/
/**********************************************************
*	void IO_CKC_EnterStandbyInt(unsigned uMode)
*
*	Input		: uMode = wake-up event selection
*				     0 : using XIN clock (that means program stops here until wake-up event is occurred.)
*					In this mode, only external interrupt can wake up system. but may be unstable while waking up.
*					NOT RECOMMENDED.
*				     non-zero : using XTIN clock.
*					In this mode, program continues but polling predefined wake-up interrupt.
*					the wake-up interrupt can be defined by this value of 'uMode'
*					this represents bitmap of wake-up interrupt sources.
*	Return		:
*	Description	: Enter Standby Mode (All clocks except XTIN are stopped.)
*				  This function is located in IO_LIB_INT area and
*				  it should be located in the internal SRAM.
*********************************************************/
void IO_CKC_EnterStandbyInt(unsigned uMode);

/**********************************************************
*	void IO_CKC_InitVariable(int iFmax, int iHmax);
*
*	Input		: iFmax = Maximum frequency of CPU clock (100Hz unit)
*				  iHmax = Maximum frequency of BUS clock (100Hz unit)
*	Return		:
*	Description	: Initialize Global Variables for Clock Driver
**********************************************************/
#if 0
#ifdef TCC83XX
#ifdef PLL_200_266_CHANGE_USED
	#define	IO_CKC_Fmaxcpu	PLL_FREQ1
	#define	IO_CKC_Fmaxbus	(PLL_FREQ1/2)
#else
	#define	IO_CKC_Fmaxcpu	PLL_FREQ
	#define	IO_CKC_Fmaxbus	(PLL_FREQ/2)
#endif

	#define	IO_CKC_Fxin		120000

	#define		IO_CKC_Fpll_32KHz	PLL_FREQ
	#define		IO_CKC_Fpll_44KHz	PLL_FREQ
	#define		IO_CKC_Fpll_48KHz	PLL_FREQ
#endif /* TCC83XX */
#endif

void IO_CKC_InitDRV(int iFmax, int iHmax);

/**********************************************************
*	void IO_CKC_DisablePLL(unsigned uCH)
*
*	Input		: uCH = Channel of PLL (0 or 1)
*	Output		:
*	Return		:
*
*	Description	: 
*
**********************************************************/
void IO_CKC_DisablePLL(unsigned uCH);

/**********************************************************
*	void IO_CKC_InitPLL(unsigned uCH, unsigned uPLL);
*
*	Input		: uPLL = target frequency of PLL (100Hz unit)
*	Return		:
*	Description	: call IO_CKC_SetPLL() according to frequency value
*				  currently supported frequency. (default = 240MHz)
*					240MHz, 120MHz, 248.5714MHz, 203.1428MHz
*					221.1428MHz, 196.5MHz
*
*				  After PLL set, it is tryed to maintain the frequencies of all clocks.
*				  If certain clock is not possible to be maintained,
*					the clock frequency is set as close as previous one.
**********************************************************/
void IO_CKC_InitPLL(unsigned uCH, unsigned uPLL);

/**********************************************************
*	void IO_CKC_SetPLL(unsigned uCH, int iP, int iM, int iS);
*
*	Input		: iP, iM, iS
*	Return		:
*	Description	: Set PLL according to PMS value, and wait until PLL is stable.
*					Fpll = Fxin * (iM + 8) / ((iP + 2) * 2^iS)
*				  (The combination of (P, M, S) should be confirmed before used)
*
*				  After PLL set, It is tryed to maintain the frequencies of all clocks.
*				  If certain clock is not possible to be maintained,
*					the clock frequency is set as close as previous one.
**********************************************************/
void IO_CKC_SetPLL(unsigned uCH, int iP, int iM, int iS);

/**********************************************************
*	void IO_CKC_SetClock(int iFclk, int iHclk);
*
*	Input		: iFclk = Freq of CPU, 100Hz unit
*				  iHclk = Freq of BUS, 100Hz unit
*	Assumption	: Source clock (XIN or PLL) must be alive.
*				  (If XTIN is currently used, make sure XIN or PLL alive before calling this function)
*	Return		:
*	Description	: Set System Clock (CPU, BUS)
*				  If target frequency exceeds its own max frequency (gCKC_Fmax, gCKC_Hmax),
*				  the uIO_CKC_error flag is incremented, and system clock is not changed.
*				  If the frequency requested can not be set exactly,
*					this function set the frequency as close as possible.
*					(refer to the datasheet for possible system clock relationship.)
*				  The system clock source is selected according to the following rule.
*					1) if (Fclk >= Fpll) PLL is selected.
*					2) if (Fclk > Fxin) PLL divider is selected.
*					3) if (Fclk == Fxin) XIN is selected.
*					4) if (Fclk > Fxtin) XIN divider is selected.
*					5) if (Fclk == Fxtin) XTIN is selected.
*					6) other case, XTIN divider is selected.
**********************************************************/
void IO_CKC_SetClock(int iFclk, int iHclk);

/**********************************************************
*	void IO_CKC_SetClockDiv(int iFclk, int iHclk, int iSrc);
*
*	Input		: iFclk = Division Factor for CPU clock (acquired by Fsrc / Fcpu)
*				  iHclk = Division Factor for BUS clock (acquired by Fcpu / Fbus)
*				  iSrc = Clock source definition (same as CKSEL[2:0] of HwCLKCTRL register)
*	Assumption	: Source clock (XIN/PLL/XTIN) must be alive.
*				  (If XTIN is currently used, make sure XIN or PLL alive before calling this function)
*	Return		:
*	Description	: Set System Clock (CPU, BUS) as follows.
*				  Frequency of CPU clock (Fcpu) = Fsrc / iFclk. (Fsrc = Frequency of source clock selected by iSrc parameter)
*				  Frequency of BUS clock (Fbus) = Fcpu / iHclk.
**********************************************************/
#ifdef TCC82XX_CLOCK_SETTING
void IO_CKC_SetClockDiv(int iFdiv, int iHdiv, int iCCKdiv, int iSrc);
#else
void IO_CKC_SetClockDiv(int iFdiv, int iHdiv, int iSrc);
#endif

/**********************************************************
*	unsigned IO_CKC_GetCurrentBUSClock(void);
*
*	Input		:
*	Return		: Frequency of Current BUS Clock (100Hz unit)
*	Description	: Return the current bus clock frequency
**********************************************************/
unsigned IO_CKC_GetCurrentBUSClock(void);

/**********************************************************
*	void IO_CKC_EnterHalt(unsigned uSDEN);
*
*	Input		: uSDEN = option for SDRAM control
*					0 = don't touch sdram
*					1 = control SDRAM, (enter self-refresh mode during halt)
*	Return		:
*	Description	: Enter Halt Mode
*				  In Halt mode, only CPU halts until interrupt request occurred.
************************************************************/
void IO_CKC_EnterHalt(unsigned uSDEN);
void IO_CKC_EnterHalt_Main(void);
void IO_CKC_EnterHalt_End(void);

/**********************************************************
*	void IO_CKC_SetRefreshClock(int iFreq);
*
*	Input		: iFreq = Frequency of Refresh Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set Refresh clock frequency and Enable refresh clock
*				  It is assumed that the BUS clock freq is larger than half of XIN frequency.
*				  The refresh clock is implicitely driven by XIN clock.
***********************************************************/
void IO_CKC_SetRefreshClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetUSBHostClock(void);
*
*	Input		:
*	Return		:
*	Description	: Enable USB Host Clock, it is fixed to use XIN as USB Host clock
***********************************************************/
void IO_CKC_SetUSBHostClock(void);

/**********************************************************
*	void IO_CKC_SetI2CClock(int iFreq);
*
*	Input		: iFreq = Frequency of I2C Main Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable I2C main Clock
*				  The real I2C clock (SCL) frequency is determined by prescale register of I2C block.
*				  The I2C main clock is implicitely driven by XIN clock.
***********************************************************/
void IO_CKC_SetI2CClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetMSCClock(int iFreq);
*
*	Input		: iFreq = Frequency of MemoryStic Controller clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable Memory Stick Controller Clock
*				  The memory stic controller clock is implicitely driven by PLL clock.
***********************************************************/
void IO_CKC_SetMSCClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetUA2Clock(int iFreq);
*	void IO_CKC_SetUA1Clock(int iFreq);
*	void IO_CKC_SetUA0Clock(int iFreq);
*
*	Input		: iFreq = Frequency of UART clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable UART Clock
*				  The baud rate of UART is determined by DL register of UART block
*				  The UART clock is implicitely driven by PLL clock.
***********************************************************/
void IO_CKC_SetUA2Clock(int iFreq);
void IO_CKC_SetUA1Clock(int iFreq);
void IO_CKC_SetUA0Clock(int iFreq);

/**********************************************************
*	void IO_CKC_SetTimerTClock(int iFreq);
*
*	Input		: iFreq = Frequency of Timer-T clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable Timer-T Clock
*				  The timer-T clock manages 6 timer/counters
*				  (refer to datasheet)
*				  The Timer-T clock is implicitely driven by XIN clock.
***********************************************************/
void IO_CKC_SetTimerTClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetGSIOClock(int iFreq);
*
*	Input		: iFreq = Frequency of GSIO base clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable GSIO base clock
*				  The real GSIO clock (SCK) is determined by speed control field of GSCRx register.
*				  (refer to datasheet)
***********************************************************/
void IO_CKC_SetGSIOClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetTimerZClock(int iFreq);
*
*	Input		: iFreq = Frequency of Timer-Z clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable Timer-Z Clock
*				  The timer-Z clock manages TC32 counter block
*				  (refer to datasheet)
*				  The Timer-Z clock is implicitely driven by XTIN clock.
***********************************************************/
void IO_CKC_SetTimerZClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetTimerXClock(int iFreq);
*
*	Input		: iFreq = Frequency of Timer-X clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable Timer-X Clock
*				  The timer-X clock manages watchdog counter block
*				  (refer to datasheet)
*				  The Timer-X clock is implicitely driven by XIN clock.
***********************************************************/
void IO_CKC_SetTimerXClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetDAIClock(int iFreq);
*
*	Input		: iFreq = Frequency of DAI Main Clock (100Hz unit)
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable DAI Clock
*				  The DAI clock should be set appropriately according to audio sampling rate.
***********************************************************/
#ifdef TCC77X
	#define	IO_CKC_PLLCH_DAI	0
#endif
#ifdef TCC87XX
	#define	IO_CKC_PLLCH_DAI	0
#endif
#ifdef TCC82XX
	#define	IO_CKC_PLLCH_DAI	0
#endif
#ifdef TCC83XX
	#define	IO_CKC_PLLCH_DAI	0
#endif

void IO_CKC_SetDAIClock(int iFreq);

/**********************************************************
*	unsigned IO_CKC_GetPLLClock(int iFreq);
*
*	Input		: iFreq = Frequency of Audio Sampling Clock (Hz unit)
*	Return		: Frequency of PLL (100Hz unit)
*	Description	: Get frequency of PLL that is capable to generate iFreq.
*				  Unsupported audio sampling rate is regarded as 44100 Hz.
*				Audio Sampling Rate	PLL frequency
*				=========================
*				44100 Hz	& related		203.1428 MHz
*				48000 Hz	& related		221.1428 MHz
*				32000 Hz	& related		196.5000 MHz
***********************************************************/
unsigned IO_CKC_GetPLLClock(int iFreq);

/**********************************************************
*	unsigned IO_CKC_GetDAIClock(int iFreq, int iOSR);
*
*	Input		: iFreq = Frequency of Audio Sampling Clock (Hz unit)
*				  iOSR = Over Sampling Rate. (ex. 256, 384)
*	Return		: Frequency of DAI Clock (100Hz unit)
*	Description	: Get frequency of DAI clock that is fit to iFreq audio frequency.
***********************************************************/
unsigned IO_CKC_GetDAIClock(int iFreq, int iOSR);

/**********************************************************
*	void IO_CKC_SetADCClock(int iFreq);
*
*	Input		: iFreq = Frequency of ADC Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Return		:
*	Description	: Set frequency and Enable ADC Clock
*				  The ADC clock is implicitely driven by XIN clock.
***********************************************************/
void IO_CKC_SetADCClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetSDMMCClock(int iFreq);
*
*	Input		: iFreq = Frequency of SDMMC Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetSDMMCClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetLCDClock(int iFreq);
*
*	Input		: iFreq = Frequency of LCD Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetLCDClock(int iFreq);

#ifdef TCC_EHI_CLK_USED
/**********************************************************
*	void IO_CKC_SetEHIClock(int iFreq);
*
*	Input		: iFreq = Frequency of EHI Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetEHIClock(int iFreq);
#endif

/**********************************************************
*	void IO_CKC_SetCAMClock(int iFreq);
*
*	Input		: iFreq = Frequency of CAM Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetCAMClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetCAMScalerClock(int iFreq);
*
*	Input		: iFreq = Frequency of CAM Scaler Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetCAMScalerClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetSPDIFClock(int iFreq);
*
*	Input		: iFreq = Frequency of SPDIF Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetSPDIFClock(int iFreq);

/**********************************************************
*	void IO_CKC_SetSPISClock(int iFreq);
*
*	Input		: iFreq = Frequency of SPIS Clock, 100Hz unit
*				  Clock disabled if iFreq == 0.
*	Output		:
*	Return		:
*
*	Description	: Set frequency and Enable Clock
***********************************************************/
void IO_CKC_SetSPISClock(int iFreq);

/**********************************************************
*	void IO_CKC_UpdateFPERI(unsigned uPeri, unsigned uFperi);
*
*	Input		: uPeri = Index of peripheral clock frequency table.
*				  uFperi = New Frequency value.
*	Output		:
*	Return		:
*
*	Description	: updates pIO_CKC_Fpll table with new frequency.
***********************************************************/
void IO_CKC_UpdateFPERI(unsigned uPeri, unsigned uFperi);

void IO_CKC_SetUA3Clock(int iFreq);
void IO_CKC_SetSPI1Clock(void);
void IO_CKC_SetSPI0Clock(void);

extern unsigned	uIO_CKC_Fpll[2];			// Current PLL Frequency, 100Hz unit
extern unsigned	uIO_CKC_Fcpu;			// Current CPU Frequency, 100Hz unit
extern unsigned	uIO_CKC_Fbus;			// Current BUS Frequency, 100Hz unit
extern unsigned	uTMODE;					// image of HwTMODE register.
#ifdef TCC83XX
extern unsigned uTMODE_B;
#endif
extern unsigned	pIO_CKC_Fperi[];			// Current Peripheral Frequency, 100Hz unit
#if 0
#ifndef WINVER
extern const sPLL	pIO_CKC_PLL[2][];		// PLL Setting (PMS) Table
#endif
#endif
extern unsigned char	ucIO_CKC_FrzFdiv;	// Freeze CPU clock divider
extern unsigned char	ucIO_CKC_FrzHdiv;	// Freeze BUS clock divider

/************************************************************************
*	ARM
************************************************************************/
/**********************************************************
*	unsigned IO_ARM_DrainWBInt(void);
*
*	Input		: none
*	Return		: 0
*	Description	: Drain Write Buffer
*				  This function is located in IO_LIB_INT area and
*				  it should be located in the internal SRAM.
**********************************************************/
unsigned IO_ARM_DrainWBInt(void);

/**********************************************************
*	unsigned IO_ARM_CleanCACHEInt(unsigned uDrainWB);
*
*	Input		: uDrainWB = Write Buffer control
*					1 = Execute Drain Write Buffer
*	Return		: 0
*	Description	: Clean Data cache and/or Drain Write Buffer
*				  This function is located in IO_LIB_INT area and
*				  it should be located in the internal SRAM.
**********************************************************/
unsigned IO_ARM_CleanCACHEInt(unsigned uDrainWB);

/**********************************************************
*	int IO_ARM_LockICACHE(void *pFPTRstart, void *pFPTRend);
*
*	Input		: pFPTRstart = function start address for locking
*				  pFPTRend   = function end address for locking
*	Return		: -1 = if error
*				    n = lock index (n >= 0)
*	Description	: Lock the object function into I cache
*				  The locked region
*					starts from (pFPTRstart & ~0x7F) and and at (pFPTRend | 0x7F).
**********************************************************/
int IO_ARM_LockICACHE(void *pFPTRstart, void *pFPTRend);

/**********************************************************
*	unsigned IO_ARM_DrainWB(void);
*
*	Input		: none
*	Return		: 0
*	Description	: Drain Write Buffer
**********************************************************/
unsigned IO_ARM_DrainWB(void);

/**********************************************************
*	unsigned IO_ARM_CleanCACHE(unsigned uDrainWB);
*
*	Input		: uDrainWB = Write Buffer control
*					1 = Drain write buffer
*					0 = Don't drain write buffer
*	Return		: 0
*	Description	: Clean Data cache (and Drain Write Buffer)
**********************************************************/
unsigned IO_ARM_CleanCACHE(unsigned uDrainWB);

/**********************************************************
*	unsigned IO_ARM_SetMMU(unsigned uMMU);
*
*	Input		: uMMU = MMU control register value
*					Hw16 = DTCM control (1 = to enable)
*					Hw12 = ICache control (1 = to enable)
*					Hw2   = DCache control (1 = to enable)
*					Hw0   = Protection control (1 = to enable)
*	Return		: same as input
*	Description	: Set MMU control register
**********************************************************/
unsigned IO_ARM_SetMMU(unsigned);

/**********************************************************
*	unsigned IO_ARM_GetMMU(void);
*
*	Input		: None
*	Return		: current MMU control register (C1 register) value
*					Hw16 = DTCM control (1 = enabled)
*					Hw12 = ICache control (1 = enabled)
*					Hw2   = DCache control (1 = enabled)
*					Hw0   = Protection control (1 = enabled)
*	Description	: Get current MMU control register (C1 register)
**********************************************************/
unsigned IO_ARM_GetMMU(void);

/**********************************************************
*	unsigned IO_ARM_ClearMMU(unsigned);
*
*	Input		: Flag for MMU control
*					Hw16 = DTCM control (1 = to disable)
*					Hw12 = ICache control (1 = to disable)
*					Hw2   = DCache control (1 = to disable)
*					Hw0   = Protection control (1 = to disable)
*	Return		: MMU Settings before modifying
*	Description	: Clear MMU control flags according to argument
**********************************************************/
unsigned IO_ARM_ClearMMU(unsigned);

/**********************************************************
*	unsigned IO_ARM_FlushCACHE(void);
*
*	Input		: None
*	Return		: 0
*	Description	: Flush Data & Instruction Cache
**********************************************************/
unsigned IO_ARM_FlushCACHE(void);

/**********************************************************
*	unsigned IO_ARM_FlushICACHE(void);
*
*	Input		: None
*	Return		: 0
*	Description	: Flush Instruction Cache
**********************************************************/
unsigned IO_ARM_FlushICACHE(void);

/**********************************************************
*	unsigned IO_ARM_FlushDCACHE(void);
*
*	Input		: None
*	Return		: 0
*	Description	: Flush Data Cache
**********************************************************/
unsigned IO_ARM_FlushDCACHE(void);

/**********************************************************
*	unsigned IO_ARM_GetICACHE(void);
*
*	Input		: None
*	Return		: Current region flags for Inst Cache
*					HwX : Region X flag (1 = instruction cache enabled)
*	Description	: Set Region flags for Inst Cache
**********************************************************/
unsigned IO_ARM_GetICACHE(void);

/**********************************************************
*	unsigned IO_ARM_GetDCACHE(void);
*
*	Input		: None
*	Return		: Current region flags for Data Cache
*					HwX : Region X flag (1 = data cache enabled)
*	Description	: Set Region flags for Data Cache
**********************************************************/
unsigned IO_ARM_GetDCACHE(void);

/**********************************************************
*	unsigned IO_ARM_GetWB(void);
*
*	Input		: None
*	Return		: Current region flags for Write Buffering
*					HwX : Region X flag (1 = write buffer enabled)
*	Description	: Get Region flags for Write Buffering
**********************************************************/
unsigned IO_ARM_GetWB(void);

/**********************************************************
*	unsigned IO_ARM_SetCPSR(unsigned uCPSR);
*
*	Input		: uCPSR = value for new CPSR
*	Return		: same as uCPSR
*	Description	: Set the CPSR as uCPSR value
**********************************************************/
unsigned IO_ARM_SetCPSR(unsigned uCPSR);

/**********************************************************
*	unsigned IO_ARM_GetCPSR(void);
*
*	Input		: None
*	Return		: the CPSR register value
*	Description	: Get the CPSR register value
**********************************************************/
unsigned IO_ARM_GetCPSR(void);

/**********************************************************
*	unsigned IO_ARM_SetINT(unsigned uCPSR);
*
*	Description	: same as IO_ARM_SetCPSR()
**********************************************************/
unsigned IO_ARM_SetINT(unsigned uCPSR);

/**********************************************************
*	unsigned IO_ARM_GetINT(void);
*
*	Description	: same as IO_ARM_GetCPSR()
**********************************************************/
unsigned IO_ARM_GetINT(void);

/**********************************************************
*	unsigned IO_ARM_EnableFIQ(void);
*
*	Input		: None
*	Return		: old CPSR (before FIQ enabled)
*	Description	: Enable FIQ
**********************************************************/
unsigned IO_ARM_EnableFIQ(void);

/**********************************************************
*	unsigned IO_ARM_DisableFIQ(void);
*
*	Input		: None
*	Return		: old CPSR (before FIQ disabled)
*	Description	: Disable FIQ
**********************************************************/
unsigned IO_ARM_DisableFIQ(void);

/**********************************************************
*	unsigned IO_ARM_EnableIRQ(void);
*
*	Input		: None
*	Return		: old CPSR (before IRQ enabled)
*	Description	: Enable IRQ
**********************************************************/
unsigned IO_ARM_EnableIRQ(void);

/**********************************************************
*	unsigned IO_ARM_DisableIRQ(void);
*
*	Input		: None
*	Return		: old CPSR (before IRQ disabled)
*	Description	: Disable IRQ
**********************************************************/
unsigned IO_ARM_DisableIRQ(void);

/**********************************************************
*	unsigned IO_ARM_EnableINT(void);
*
*	Input		: None
*	Return		: old CPSR (before Interrupt enabled)
*	Description	: Enable IRQ/FIQ Interrupt
**********************************************************/
unsigned IO_ARM_EnableINT(void);

/**********************************************************
*	unsigned IO_ARM_DisableINT(void);
*
*	Input		: None
*	Return		: old CPSR (before Interrupt disabled)
*	Description	: Disable IRQ/FIQ Interrupt
**********************************************************/
unsigned IO_ARM_DisableINT(void);

/************************************************************************
*	Memory Controller
************************************************************************/
/**********************************************************
*	void IO_MC_SetCFGInt(unsigned uCS, unsigned uValue);
*
*	Input		: uCS = 0~4
*				  uValue = register value for corresponding chip select
*	Return		:
*	Description	: Set the corresponding memory configuration register (HwCSCFG0~3, HwSDCFG)
*				  This function is located in IO_LIB_INT area and
*				  it should be located in the internal SRAM.
**********************************************************/
void IO_MC_SetCFGInt(unsigned uCS, unsigned uValue);

/************************************************************************
*	DMA Controller
************************************************************************/
enum { 
IO_DMA_CH0, 
IO_DMA_CH1, 
IO_DMA_CH2,
#ifdef TCC83XX
IO_DMA_CH3,
IO_DMA_CH4,
IO_DMA_CH5
#endif
};
//append by leezzoo - 070914
#if defined(TCC77X)||defined(TCC82XX) || defined(TCC83XX)
	#define	IO_DMA_PORTCFG_NFC_READ	0
	#define	IO_DMA_PORTCFG_NFC_WRITE	0
#endif
#ifdef TCC78X
	#define	IO_DMA_PORTCFG_NFC_READ	0	//HwCHCTRL_PD_P1P0
	#define	IO_DMA_PORTCFG_NFC_WRITE	0	//HwCHCTRL_PD_P0P1
#endif

#define	IO_DMA_EnableDMA(X)		{ BITSET(((sHwDMA *)(X))->CHCTRL, HwCHCTRL_EN_EN); }
#define	IO_DMA_DisableDMA(X)	{ BITSET(((sHwDMA *)(X))->CHCTRL, HwCHCTRL_EN_EN); }

/**********************************************************
*	void IO_DMA_SetCTRL(unsigned uCH, unsigned	uCHCTRL);
*
*	Input		: uCH = Channel number (0~2)
*				  uValue = DMA control register value of corresponding channel
*	Return		:
*	Description	: Set the corresponding DMA control register (HwCHCTRL0~1 in General DMA, HwCHCTRL0 in Storage DMA)
**********************************************************/
sHwDMA *IO_DMA_SetCTRL(unsigned uCH, unsigned	uCHCTRL);

/**********************************************************
*	void IO_DMA_SetDMA(unsigned uCH, void *pSRC, unsigned uSPARAM,
*						void *pDST, unsigned uDPARAM,
*						unsigned	uCHCTRL, unsigned uSize);
*	Input		: uCH = Channel number (0~2)
*				  pSRC = source address
*				  uSPARAM = source parameter
*				  pDST = destination address
*				  uDPARAM = destination parameter
*				  uCHCTRL = DMA control register
*				  uSize = Transfer size (in byte unit)
*	Return		:
*	Description	: Set the DMA (and start to transfer)
**********************************************************/
#ifdef TCC83XX
sHwDMA *IO_DMA_SetDMA(
	unsigned uCH,
	void *pSRC, unsigned uSPARAM,
	void *pDST, unsigned uDPARAM,
	unsigned	uCHCTRL,
	unsigned 	uREQSRC,  // added for TCC83XX 2007.9.13
	unsigned uSize
	);

sHwDMA *IO_DMA_SetDMA_NAND(
	unsigned uCH,
	void *pSRC, unsigned uSPARAM,
	void *pDST, unsigned uDPARAM,
	unsigned	uCHCTRL,
	unsigned uEXTREQ,
	unsigned uSize	
);

#else	
sHwDMA *IO_DMA_SetDMA(
	unsigned uCH,
	void *pSRC, unsigned uSPARAM,	void *pDST, unsigned uDPARAM,
	unsigned	uCHCTRL, unsigned uSize
);
#endif
/************************************************************************
*	ECC Controller
************************************************************************/
#define	IO_ECC_DisableECC()	{ HwECC_CTRL &= HwECC_CTRL_SE_DIS; }
#define	IO_ECC_EnableECC()	{ HwECC_CTRL |= HwECC_CTRL_SE_EN; }

/**********************************************************
*	int IO_ECC_CheckSLC(unsigned char *pcDATA, unsigned char *pcSPARE);
*
*	Input		: pcDATA = start address of data block
*				  pcSPARE = start address of spare block
*	Return		: 0 = no error or correctable error
*				  -1 = uncorrectable error
*	Description	: Check ECC and Correct Data Error
************************************************************/
int IO_ECC_CheckSLC(unsigned char *pcDATA, unsigned char *pcSPARE);

/************************************************************************
*	NAND Flash Controller
************************************************************************/

// if MEMORY_BUS_NAND is defined, NAND flash is assumed to be attached at the memory bus. (not GPIO_C port)
#if (defined(TCC77X_DMP_ALLTEST_BOARD) || defined(TCC772_DMP_DEMO_BOARD) || defined(TCC772_SS_FLICK))
	#define	USE_MEMORY_BUS_NAND
#endif

#define	HwNAND_CMD	(pHwND->CMD)
#define	HwNAND_LADR	(pHwND->LADR)
#define	HwNAND_DATA	(pHwND->WDATA.D32)
#define	HwNAND_SDATA	((cIO_NFC_MEM) ? pHwND->WDATA.D8 : pHwND->SDATA.D32)
#define	HwNAND_SADR	(pHwND->SADR)

#define	ECC_BASEPAGE	((unsigned)&HwNAND_DATA)

typedef	struct {				// NAND Request Structure
	unsigned		CTRL;
	unsigned		PADDR;		// Physical Address
	unsigned		NFCCFG;		// image of HwNFC_CTRL register for corresponding request
	unsigned		*PBUF;		// Pointer for Page Data
	void			*SBUF;		// Pointer for Spare Data
} sNFC_REQ;

typedef	struct {				// NAND Request Master Structure
	unsigned		uSEMA;			// Semaphore
	sNFC_REQ	*pReqHead;		// request is poped from here.
	sNFC_REQ	*pReqTail;		// request is pushed to here.
} sNFC_REQMST;

#define	REQ_FAIL		((sNFC_REQ *)(-1))

// sNFC_REQ.CTRL bit-field definition
//-----------------------------------------------------------------------------------
#define	REQ_INACTIVE	(Hw31)			// Inactive, or End of Write Request (1)
#define	REQ_EOR			(Hw30)			// End of Read Request, but not copied yet. (1)
#define	REQ_ECCERR		(Hw29)			// Uncorrectable ECC error occurred. (1)
#define	REQ_SPARE		(Hw28)			// Start from the spare area. (1)
//-----------------------------------------------------------------------------------
#define	REQ_DISABLE_MK	(Hw27)			// Disable Writing ECC MARK in data transfer (1)
#define	REQ_ECCEN		(Hw26)			// Calc ECC in data transfer (1)
#define	REQ_EOW		(Hw25)			// End of Write Request, but not check OK bit yet. (1)
#define	REQ_WOK		(Hw24)			// Write OK Flag (1)
//-----------------------------------------------------------------------------------
#define	REQ_SKIPHCMD	(Hw23)			// Skip header command process (1)
#define	REQ_SKIPTCMD	(Hw22)			// Skip tail command process (1)
#define	REQ_SKIPCMD	(Hw23|Hw22)
#define	REQ_CPBACK		(Hw21)			// Copy-Back Enabled. (1)
#define	REQ_CPBACKW	(Hw20)			// Copy-Back with partial writing. (1)
//-----------------------------------------------------------------------------------
#define	REQ_SKIP10		(Hw19)			// Skip 0x10 command in writePAGE. (1)
#define	REQ_ERASEBLK	(Hw18)			// Erase Block command. (1)
#define	REQ_PRO_SPARE	(Hw17)			// Process spare area. (1)
//-----------------------------------------------------------------------------------
#define	REQ_READY		(Hw12)			// All settings for Request is ready (1)
#define	REQ_DSIZE_MASK	(REQ_READY - 1)	// [11:0] = Data Size
#define	REQ_DSIZE(X)	((X)&(REQ_DSIZE_MASK))

// uCS bit-field definition
#define	REQ_CSMASK		0x0000000F

enum { IO_NFC_READ = 0, IO_NFC_WRITE = Hw15};

void IO_NFC_InitDRV(void);
void IO_NFC_OpenREQ(sNFC_REQ *pReq, unsigned uMaxNFC);
void IO_NFC_CloseREQ(void);
void IO_NFC_PopREQ(sNFC_REQ *pReq);
void IO_NFC_EnableREQ(sNFC_REQ *pReq);
void IO_NFC_IRQHandler(void);
void IO_NFC_StartREAD(sNFC_REQ *pReq);
void IO_NFC_ReadPAGE(unsigned *pBuffer, unsigned uDSize);
void IO_NFC_ReadDATA(unsigned char *pBuffer, unsigned uDSize);
int IO_NFC_CopyPAGE(unsigned char *pDest, unsigned *pSrc, unsigned uSize);
sNFC_REQ *IO_NFC_PushRREQ(unsigned uCS, unsigned uPage, unsigned uColumn, unsigned *pSpare, unsigned uSize);
void IO_NFC_StartWRITE(sNFC_REQ *pReq);
void IO_NFC_WritePAGE(unsigned *pBuffer, unsigned uDSize);
void IO_NFC_WriteDATA(unsigned char *pBuffer, unsigned uDSize);
sNFC_REQ *IO_NFC_PushWREQ(	unsigned uCS, unsigned uPage, unsigned uColumn,
							unsigned *pData, unsigned char *pSpare, unsigned uSize);
int IO_NFC_CheckWOK(sNFC_REQ *pReq);
unsigned IO_NFC_LookupID(unsigned uDID);
unsigned IO_NFC_SetCONFIG(unsigned uType);
unsigned IO_NFC_MakeNFC(unsigned uCS);
unsigned IO_NFC_GetID(unsigned uCS);
unsigned IO_NFC_ResetNAND(unsigned uCS);
void IO_NFC_SetCYCLE(void);
void IO_NFC_WaitEOT(sNFC_REQ *pReq);

extern unsigned		uIO_NFC_CONFIG, uIO_NFC_MASK;
extern unsigned char	cIO_NFC_MEM;
extern sHwND			*pIO_NFC_HwND;

#define	IO_NFC_MakeMASK(uCS)	((uIO_NFC_CONFIG & (Hw14 << (uCS*16))) ? 0xFFFF : 0xFF)

#define	IO_NFC_SetNFC(uCS)						\
{												\
	if (cIO_NFC_MEM)								\
		HwCSCFG2	= IO_NFC_MakeNFC(uCS);		\
	else											\
		HwNFC_CTRL	= IO_NFC_MakeNFC(uCS);		\
}

#ifdef TCC77X
	#define	GPIO_ND_nCS	HwGDATA_E
	#define	GPIO_ND_nCS1Bit	Hw5
	#define	GPIO_ND_nCS0Bit	Hw4
	#define	GPIO_ND_nWP	HwGDATA_E
	#define	GPIO_ND_nWPBit	Hw2
	#define	GPIO_ND_RDY	HwGDATA_E
	#define	GPIO_ND_RDYBit	Hw3
	#define	GPIO_NFC_nWP	HwGDATA_C
	#define	GPIO_NFC_nWPBit	Hw25
#endif
#ifdef TCC87XX
	#define	GPIO_ND_nCS		HwGDATA_D
	#define	GPIO_ND_nCS1Bit	Hw5
	#define	GPIO_ND_nCS0Bit	Hw4
	#define	GPIO_ND_nWP		HwGDATA_D
	#define	GPIO_ND_nWPBit		Hw2
	#define	GPIO_ND_RDY		HwGDATA_D
	#define	GPIO_ND_RDYBit		Hw3
	#define	GPIO_NFC_nWP		HwGDATA_C
	#define	GPIO_NFC_nWPBit	Hw25
#endif
#ifdef TCC82XX
	#if defined(TCC8200_BOARD)
		#define	GPIO_ND_nCS		HwGDATA_A
		#define	GPIO_ND_nCS1Bit	Hw19
		#define	GPIO_ND_nCS0Bit	Hw18
		#define	GPIO_ND_nWP		HwGDATA_A
		#define	GPIO_ND_nWPBit	Hw17
		#define	GPIO_ND_RDY		HwGDATA_A
		#define	GPIO_ND_RDYBit	Hw24
		#define	GPIO_NFC_nWP	HwGDATA_A
		#define	GPIO_NFC_nWPBit	Hw17
	#elif defined(TCC822X_BOARD)
		#define	GPIO_ND_nCS		HwGDATA_C
		#define	GPIO_ND_nCS1Bit	Hw17
		#define	GPIO_ND_nCS0Bit	Hw16
		#define	GPIO_ND_nWP		HwGDATA_C
		#define	GPIO_ND_nWPBit	Hw25
		#define	GPIO_ND_RDY		HwGDATA_C
		#define	GPIO_ND_RDYBit	Hw22
		#define	GPIO_NFC_nWP	HwGDATA_C
		#define	GPIO_NFC_nWPBit	Hw25
	#endif
#endif
#ifdef TCC83XX
	#if defined(TCC8300_BOARD)
		#define	GPIO_ND_nCS		HwGDATA_C
		#define	GPIO_ND_nCS1Bit	Hw5
		#define	GPIO_ND_nCS0Bit	Hw4
		#define	GPIO_ND_nWP		HwGDATA_C
		#define	GPIO_ND_nWPBit	Hw2
		#define	GPIO_ND_RDY		HwGDATA_C
		#define	GPIO_ND_RDYBit	Hw3
		#define	GPIO_NFC_nWP	HwGDATA_A
		#define	GPIO_NFC_nWPBit	Hw17
	#elif defined(TCC832X_BOARD)
		#define	GPIO_ND_nCS		HwGDATA_C
		#define	GPIO_ND_nCS1Bit	Hw9
		#define	GPIO_ND_nCS0Bit	Hw8
		#define	GPIO_ND_RDY		HwGDATA_C
		#define	GPIO_ND_RDYBit	Hw14
	#endif
#endif /* TCC83XX */

#define	IO_NFC_DisableCS()								\
{														\
	if (cIO_NFC_MEM)										\
		BITSET(GPIO_ND_nCS, GPIO_ND_nCS1Bit|GPIO_ND_nCS0Bit);					\
	else													\
		BITSET(HwNFC_CTRL, HwNFC_CTRL_CFG_NOACT);		\
}

#define	IO_NFC_EnableCS(uCS)											\
{																		\
	if (cIO_NFC_MEM)														\
		BITSCLR(GPIO_ND_nCS, (uCS) ? GPIO_ND_nCS0Bit: GPIO_ND_nCS1Bit, (uCS) ? GPIO_ND_nCS1Bit: GPIO_ND_nCS0Bit);	\
	else																	\
		BITSCLR(HwNFC_CTRL, (uCS) ? Hw22: Hw23, (uCS) ? Hw23 : Hw22);		\
}

#define	IO_NFC_DisableWP()					\
{								\
	if (cIO_NFC_MEM)					\
		BITSET(GPIO_ND_nWP, GPIO_ND_nWPBit);		\
	else							\
		BITSET(GPIO_NFC_nWP, GPIO_NFC_nWPBit);		\
}

#define	IO_NFC_EnableWP()					\
{								\
	if (cIO_NFC_MEM)					\
		BITCLR(GPIO_ND_nWP, GPIO_ND_nWPBit);		\
	else							\
		BITCLR(GPIO_NFC_nWP, GPIO_NFC_nWPBit);		\
}

#define	IO_NFC_NotREADY()	\
	( (cIO_NFC_MEM) ? ISZERO(GPIO_ND_RDY, GPIO_ND_RDYBit) : ISZERO(HwNFC_CTRL, HwNFC_CTRL_RDY_RDY))

//#define	IO_NFC_WaitSTARDY()		{ while (ISZERO(HwNFC_CTRL, HwNFC_CTRL_STA_RDY)); }
#define	IO_NFC_WaitSTARDY()		{ ; }

void IO_NFC_WaitREADY(void);
void IO_NFC_WaitREADYForWriteCommand(void);

#define	IO_NFC_WaitDONE(X)		{ while (ISZERO(HwNFC_IREQ, (X))); }
//#define	IO_NFC_WaitDONE()		{ unsigned char ucError; OSSemPend(NFC_SEM, 0, &ucError); }

#define	USE_NFC_DMA
#ifndef USE_MEMORY_BUS_NAND
	#define	USE_NFC_FIFO		// only meaningful when "USE_NFC_DMA" is defined.
#endif

/************************************************************************
*	DTCM Allocation Manager
************************************************************************/
//#define	USE_DYNAMIC_DTCM

#ifdef TCC77X
	#define	DTCM_BASE					0xA0000000
	#define	DTCM_LIMIT					0xA0001000		// 4KB
	#define	IO_USB_BUFFER0_BASE			(DTCM_BASE + 0x800)
	#define	IO_USB_BUFFER1_BASE			(DTCM_BASE + 0xa00)
	#define	IO_NFC_BUFFER0_BASE			(DTCM_BASE + 0xc00)
	#define	IO_NFC_BUFFER1_BASE			(DTCM_BASE + 0xe00)
#endif
#ifdef TCC87XX
	#define	DTCM_BASE					0xA0000000
	#define	DTCM_LIMIT					0xA0001000		// 4KB
	#define	IO_USB_BUFFER0_BASE			(DTCM_BASE + 0x800)
	#define	IO_USB_BUFFER1_BASE			(DTCM_BASE + 0xa00)
	#define	IO_NFC_BUFFER0_BASE			(DTCM_BASE + 0xc00)
	#define	IO_NFC_BUFFER1_BASE			(DTCM_BASE + 0xe00)
#endif
#ifdef TCC82XX
	#define	DTCM_BASE					0xA0000000
	#define	DTCM_LIMIT					0xA0001000		// 4KB
	#define	IO_USB_BUFFER0_BASE			(DTCM_BASE + 0x800)
	#define	IO_USB_BUFFER1_BASE			(DTCM_BASE + 0xa00)
	#define	IO_NFC_BUFFER0_BASE			(DTCM_BASE + 0xc00)
	#define	IO_NFC_BUFFER1_BASE			(DTCM_BASE + 0xe00)
#endif
#ifdef TCC83XX
	#define	DTCM_BASE					0xA0000000
	#define	DTCM_LIMIT					0xA0002000		// 8KB
	#define	IO_USB_BUFFER0_BASE			(DTCM_BASE + 0x800)
	#define	IO_USB_BUFFER1_BASE			(DTCM_BASE + 0xa00)
	#define	IO_NFC_BUFFER0_BASE			(DTCM_BASE + 0xc00)
	#define	IO_NFC_BUFFER1_BASE			(DTCM_BASE + 0xe00)
	#define IO_SDMMC_BUFFER_BASE		(DTCM_BASE + 0x1e00)	// offset must be e00
	#define IO_SPI_DMA_BUFFER0_BASE		(DTCM_BASE + 0x1c00)
	#define IO_SPI_DMA_BUFFER1_BASE		(DTCM_BASE + 0x1e00)	// 512 bytes
#endif

#define	IO_HDD_BUFFER_BASE			IO_NFC_BUFFER0_BASE

#ifdef	USE_DYNAMIC_DTCM
	#define	DTCM_MaskSize		(8 * sizeof(unsigned))								// bit width of uDTCM_MAT[] table
	#define	DTCM_Mask			((unsigned)((1 << DTCM_MaskSize) - 1))				// mask pattern of uDTCM_MAT[] table
	#define	DTCM_SIZE			4096
	#define	DTCM_AUNIT			64												// minimum allocation unit.
	#define	DTCM_MAXBULK		(DTCM_SIZE / DTCM_AUNIT)						// maximum concurrent allocation.
	#define	DTCM_MATSIZE		((DTCM_SIZE / DTCM_AUNIT) / DTCM_MaskSize)		// Bitmap table for each chunk (1 = used, 0 = not-used)
#endif

void IO_DTCM_InitDRV(void);
void *IO_DTCM_Malloc(unsigned uDSize);
void IO_DTCM_Free(void *pSrc, unsigned uDSize);

unsigned IO_DTCM_AllocMAT(unsigned uASize);
unsigned IO_DTCM_FindFMAX(void);


/************************************************************************
*	SRAM ADDR
************************************************************************/
#ifdef TCC87XX
	#define	SRAM_BASE					0x00000000
	#define	SRAM_LIMIT					0x00010000
#endif
#ifdef TCC82XX
	#define	SRAM_BASE					0x00000000
	#define	SRAM_LIMIT					0x00010000
#endif
#ifdef TCC83XX
	#define	SRAM_BASE					0x00000000
	#define	SRAM_LIMIT					0x00010000
#endif

/************************************************************************
*	Interrupt Controller
************************************************************************/
#ifdef TCC77X
	#define	IO_INT_HwIEN		HwIEN
	#define	IO_INT_HwISTS		HwIREQ
	#define	IO_INT_HwMSTS		HwMREQ
	#define	IO_INT_HwICLR		HwCREQ
	#define	IO_INT_HwISEL		HwIRQSEL
	#define	IO_INT_HwTMODE		HwTMODE
	#define	IO_INT_HwTIMER		HwIEN_TC_EN
	#define	IO_INT_HwLCD		HwZERO
	#define	IO_INT_HwUART		(HwIEN_UT0_EN | HwIEN_UT1_EN)
	#define	IO_INT_HwEXT3		HwIEN_E3_EN
	#define	IO_INT_HwGSIO		HwIEN_GS_EN
	#define	IO_INT_HwCDIF		HwIEN_CDIF_EN
	#define	IO_INT_HwUSBD		HwIEN_UB_EN
	#define	IO_INT_HwUSBH		HwIEN_UBH_EN
	#define	IO_INT_HwNFC		HwIEN_ND_EN
	#define	IO_INT_HwDAITX		HwIEN_I2T_EN
	#define	IO_INT_HwDAIRX		HwIEN_I2R_EN
	#define	IO_INT_HwRTC		HwIEN_RTC_EN
	#define	IO_INT_HwDMA_CH0	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH1	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH2	HwIEN_DMA1ch_EN
	#define	IO_INT_HwEHI		HwIEN_EHI_EN
#endif /* TCC77X */

#ifdef TCC87XX
	#define	IO_INT_HwIEN		HwIEN
	#define	IO_INT_HwISTS		HwIREQ
	#define	IO_INT_HwMSTS		HwMREQ
	#define	IO_INT_HwICLR		HwCREQ
	#define	IO_INT_HwISEL		HwIRQSEL
	#define	IO_INT_HwTMODE		HwTMODE
	#define	IO_INT_HwTIMER		HwIEN_TC_EN
	#define	IO_INT_HwLCD		HwZERO
	#define	IO_INT_HwUART		(HwIEN_UT0_EN | HwIEN_UT1_EN)
	#define	IO_INT_HwEXT3		HwIEN_E3_EN
	#define	IO_INT_HwGSIO		HwIEN_GS_EN
	#define	IO_INT_HwCDIF		HwIEN_CDIF_EN
	#define	IO_INT_HwUSBD		HwIEN_UB_EN
	#define	IO_INT_HwUSBH		HwIEN_UBH_EN
	#define	IO_INT_HwNFC		HwIEN_ND_EN
	#define	IO_INT_HwDAITX		HwIEN_I2T_EN
	#define	IO_INT_HwDAIRX		HwIEN_I2R_EN
	#define	IO_INT_HwRTC		HwIEN_RTC_EN
	#define	IO_INT_HwDMA_CH0	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH1	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH2	HwIEN_DMA_EN
	#define	IO_INT_HwEHI		HwIEN_EHI_EN
#endif /* TCC87XX */

#ifdef TCC82XX
	#define	IO_INT_HwIEN		HwIEN
	#define	IO_INT_HwISTS		HwIREQ
	#define	IO_INT_HwMSTS		HwMREQ
	#define	IO_INT_HwICLR		HwCREQ
	#define	IO_INT_HwISEL		HwIRQSEL
	#define	IO_INT_HwTMODE		HwTMODE
	#define	IO_INT_HwTIMER		HwIEN_TC_EN
	#define	IO_INT_HwLCD		HwZERO
	#define	IO_INT_HwUART		(HwIEN_UT0_EN | HwIEN_UT1_EN)
	#define	IO_INT_HwEXT0		HwIEN_E0_EN
	#define	IO_INT_HwEXT3		HwIEN_E3_EN
	#define	IO_INT_HwGSIO		HwIEN_GS_EN
	#define	IO_INT_HwCDIF		HwIEN_CDIF_EN
	#define	IO_INT_HwUSBD		HwIEN_UB_EN
	#define	IO_INT_HwUSBH		HwIEN_UBH_EN
	#define	IO_INT_HwNFC		HwIEN_ND_EN
	#define	IO_INT_HwDAITX		HwIEN_I2T_EN
	#define	IO_INT_HwDAIRX		HwIEN_I2R_EN
	#define	IO_INT_HwRTC		HwIEN_RTC_EN
	#define	IO_INT_HwDMA_CH0	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH1	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH2	HwIEN_DMA_EN
	#define	IO_INT_HwSDDET		HwIEN_SDDET_EN
	#define	IO_INT_HwEHI		HwIEN_EHI_EN
#endif /* TCC82XX */

#ifdef TCC83XX
	#define	IO_INT_HwIEN		HwIEN
	#define	IO_INT_HwISTS		HwIREQ
	#define	IO_INT_HwMSTS		HwMREQ
	#define	IO_INT_HwICLR		HwCREQ
	#define	IO_INT_HwISEL		HwIRQSEL
	#define	IO_INT_HwTMODE		HwTMODE
	#define	IO_INT_HwTIMER		HwIEN_TC_EN
	#define	IO_INT_HwLCD		HwIEN_LCD_EN
	#define	IO_INT_HwUART		(HwIEN_UT0_EN | HwIEN_UT1_EN)
	#define	IO_INT_HwUART0		HwIEN_UT0_EN
	#define	IO_INT_HwUART1		HwIEN_UT1_EN
	//#define	IO_INT_HwEXT0		HwIEN_EI0 //Bruce_temp_83 ??
	//#define	IO_INT_HwEXT1		HwIEN_EI1 //Bruce_temp_83 ??
	//#define	IO_INT_HwEXT2		HwIEN_EI2 //Bruce_temp_83 ??
	//#define	IO_INT_HwEXT3		HwIEN_EI3 //Bruce_temp_83 ??
	#define	IO_INT_HwUSBD		HwIEN_USBD_EN
	#define	IO_INT_HwNFC		HwIEN_ND_EN
	#define	IO_INT_HwDAITX		HwIEN_I2T_EN
	#define	IO_INT_HwDAIRX		HwIEN_I2R_EN
	#define	IO_INT_HwRTC_A		HwIEN_RTC_A_EN
	#define	IO_INT_HwRTC_P		HwIEN_RTC_P_EN
	#define	IO_INT_HwDMA_CH0	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH1	HwIEN_DMA_EN
	#define	IO_INT_HwDMA_CH2	HwIEN_DMA_EN
	#define IO_INT_HwADMA		HwIEN_ADMA_EN
	#define	IO_INT_HwEHI		HwIEN_EHI_EN
	//#define	IO_INT_HwSC		HwIEN_SC //Bruce_temp_83 ??
#endif /* TCC83XX */

#define	IO_INT_EnableIRQ(X)					\
		{									\
			BITSET(IO_INT_HwISEL, X);		\
			BITSET(IO_INT_HwIEN, X);		\
		}
#define	IO_INT_DisableIRQ(X)				\
		{									\
			BITCLR(HwIEN, X);				\
		}

/************************************************************************
*	EHI Controller
************************************************************************/
#define	EHI_MD_68		HwEHCFG_MD_68
#define	EHI_MD_80		HwEHCFG_MD_80
#define	EHI_USE_MASK	Hw1
#define	EHI_BW_8		HwEHCFG_BW_8
#define	EHI_BW_16		HwEHCFG_BW_16
#define	EHI_USE_RDY		HwEHCFG_RDYE_RDY
#define	EHI_USE_IRQ		HwEHCFG_RDYE_IRQ
#define	EHI_HIGH_RDY	HwZERO
#define	EHI_LOW_RDY		HwEHCFG_RDYP

extern	unsigned	uEHI_CSCFG, uEHI_TACC;
extern	volatile unsigned *pEHI_CSCFG;

#ifdef	EHIS_TCC77X
	#define	EHIS_HwEHST			0x90000800	// R/W, Status register
	#define	EHIS_HwEHIINT		0x90000804	// R/W,  Internal interrupt control register
	#define	EHIS_HwEHEINT		0x90000808	// R/W,  External interrupt control register
	#define	EHIS_HwEHA			0x9000080C	// R/W,  Address register
	#define	EHIS_HwEHAM			0x90000810	// R,  Address masking register
	#define	EHIS_HwEHD			0x90000814	// R/W,  Data register
	#define	EHIS_HwEHSEM		0x90000818	// R/W,  Semaphore register
	#define	EHIS_HwEHCFG		0x9000081C	// R/W,  Configuration register
	#define	EHIS_HwEHIND		0x90000820	// W,  Index register
	#define	EHIS_HwEHRWCS		0x90000824	// R/W,  Read/Write Control/Status register
#endif

#ifdef EHI_MASTER
	/**********************************************************
	*	void IO_EHI_InitDRV(unsigned uCONFIG, unsigned uCS, unsigned uMask);
	*
	*	Input		: uCONFIG = Configuration Parameter
	*					Hw0 = 68000 (1), x86 (0) interface
	*					Hw1 = Use Mask (1), Don't used Mask (0)
	*					Hw2 = 8bit (1), 16bit (0) interface
	*					Hw3 = used as Ready signal (1), used as Interrupt signal (0)
	*					Hw4 = Active Low Ready signal (1), Active High Ready signal (0)
	*				  uCS = Chip Select number for EHI slave (0~3)
	*				  uMask = Address Mask Pattern.
	*	Return		:
	*	Description	: Initialize EHI I/F module at the Master Site.
	************************************************************/
	void IO_EHI_InitDRV(unsigned uCONFIG, unsigned uCS, unsigned uMask);

	/**********************************************************
	*	unsigned IO_EHI_SetSPEED(unsigned uTAcc);
	*
	*	Input		: uTAcc = Access time in nano second
	*	Output		:
	*	Return		: previous CSCFG value
	*
	*	Description	: Set EHI Access parameter
	************************************************************/
	unsigned IO_EHI_SetSPEED(unsigned uTAcc);

	/**********************************************************
	*	unsigned IO_EHI_IncSPEED(int iTAccDelta);
	*
	*	Input		: iTAccDelta = Increment(+)/Decrement(-) of Access time in nano second
	*	Output		:
	*	Return		: Old TAcc
	*
	*	Description	: Increment EHI Access parameter
	************************************************************/
	unsigned IO_EHI_IncSPEED(int iTAccDelta);

	/**********************************************************
	*	void IO_EHI_WriteREG(unsigned uADDR, unsigned uDATA, unsigned uSize);
	*
	*	Input		: uADDR = Address of EHI Register
	*				  uDATA = Data for EHI Register
	*				  uSize = Register Size in byte.
	*	Return		:
	*	Description	: Write EHI Register
	************************************************************/
	void IO_EHI_WriteREG(unsigned uADDR, unsigned uDATA, unsigned uSize);

	/**********************************************************
	*	unsigned IO_EHI_ReadREG(unsigned uADDR, unsigned uSize);
	*
	*	Input		: uADDR = Address of EHI Register
	*				  uSize = Register Size in byte.
	*	Return		: Register value
	*	Description	: Read EHI Register
	************************************************************/
	unsigned IO_EHI_ReadREG(unsigned uADDR, unsigned uSize);

	/**********************************************************
	*	unsigned IO_EHI_ReadST(void);
	*
	*	Input		:
	*	Output		:
	*	Return		: EHST value
	*
	*	Description	: Read EHST Register
	************************************************************/
	unsigned IO_EHI_ReadST(void);

	/**********************************************************
	*	unsigned IO_EHI_WriteDATA(unsigned uADDR, unsigned uParam1, unsigned uParam2);
	*
	*	Input		: uADDR = Address of EHI Slave memory.
	*				  uParam1 = Pointer or Data value to write.
	*				  uParam2
	*					[31]   = Non-continuous(0), Continuous(1)
	*					[30:0] = Data amount to Transfer (word unit)
	*	Return		: 0
	*	Description	: Write data to Memory of Slave
	************************************************************/
	unsigned IO_EHI_WriteDATA(unsigned uADDR, unsigned uParam1, unsigned uParam2);

	/**********************************************************
	*	unsigned IO_EHI_ReadDATA(unsigned uADDR, unsigned uParam1, unsigned uParam2);
	*
	*	Input		: uADDR = Address of EHI Slave memory.
	*				  uParam1 = Pointer to store read data.
	*				  uParam2
	*					[31]   = Non-continuous (0), Continuous (1) transfer.
	*					[30:0] = Data amount to Transfer (word unit)
	*	Return		: Data value read or Pointer to bulk of data read.
	*	Description	: Read data from Memory of Slave
	************************************************************/
	unsigned IO_EHI_ReadDATA(unsigned uADDR, unsigned uParam1, unsigned uParam2);
#else
	/**********************************************************
	*	void IO_EHI_InitDRVS(unsigned uCONFIG);
	*
	*	Input		: uCONFIG = Configuration Parameter
	*					Hw0 = 68000 (1), x86 (0) interface
	*					Hw2 = 8bit (1), 16bit (0) interface
	*					Hw3 = used as Ready signal (1), used as Interrupt signal (0)
	*					Hw4 = Active Low Ready signal (1), Active High Ready signal (0)
	*	Return		:
	*	Description	: Initialize EHI I/F module at the Slave Site.
	************************************************************/
	void IO_EHI_InitDRVS(unsigned uCONFIG);
#endif

/************************************************************************
*	FGPIO Controller
************************************************************************/
/**********************************************************
*	unsigned IO_FGP_SetDATA(unsigned uValue);
*
*	Input		: uValue = FGP DATA value
*	Return		:
*	Description	: Set Data port on the all ports
**********************************************************/
unsigned IO_FGP_SetDATA(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_InvertDATA(unsigned uValue);
*
*	Input		: uValue = 1 for inverting data port.
*	Return		:
*	Description	: Inverting Data port.
**********************************************************/
unsigned IO_FGP_InvertDATA(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_OrrDATA(unsigned uValue);
*
*	Input		: uValue = 1 for orring data port.
*	Return		:
*	Description	: Orring Data port.
**********************************************************/
unsigned IO_FGP_OrrDATA(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_ClrDATA(unsigned uValue);
*
*	Input		: uValue = value for bit clearing data port.
*	Return		:
*	Description	: Bit clearing Data port.
**********************************************************/
unsigned IO_FGP_ClrDATA(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_SetCON(unsigned uValue);
*
*	Input		: uValue = value for I/O Control.
*	Return		:
*	Description	: Set I/O control on all ports.
**********************************************************/
unsigned IO_FGP_SetCON(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_GetC0();
*
*	Input		:
*	Return		: C0 register value.
*	Description	:
**********************************************************/
unsigned IO_FGP_GetC0(void);

/**********************************************************
*	unsigned IO_FGP_SetC1(unsigned uValue);
*
*	Input		: uValue = C1 register value.
*	Return		:
*	Description	:
**********************************************************/
unsigned IO_FGP_SetC1(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_GetC1();
*
*	Input		:
*	Return		: C1 register value.
*	Description	:
**********************************************************/
unsigned IO_FGP_GetC1(void);

/**********************************************************
*	unsigned IO_FGP_GetR32(unsigned uValue);
*
*	Input		:
*	Return		: 32bit reverse value of C1 register.
*	Description	:
**********************************************************/
unsigned IO_FGP_GetR32(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_GetR12(unsigned uValue);
*
*	Input		:
*	Return		: 32bit reverse value of C1 register.
*	Description	:
**********************************************************/
unsigned IO_FGP_GetR12(unsigned uValue);

/**********************************************************
*	unsigned IO_FGP_GetCLZ(unsigned uValue);
*
*	Input		:
*	Return		: Count Leading Zero value of uValue
*	Description	:
**********************************************************/
unsigned IO_FGP_GetCLZ(unsigned uValue);

/************************************************************************
*	Timer/Counter
************************************************************************/
#if defined(TCC77X) || defined(TCC87XX) || defined(TCC82XX) || defined(TCC83XX)
	#define	IO_TMR_IREQT0	HwTIREQ_TI0
	#define	IO_TMR_IREQT1	HwTIREQ_TI1
	#define	IO_TMR_IREQT2	HwTIREQ_TI2
	#define	IO_TMR_IREQT3	HwTIREQ_TI3
	#define	IO_TMR_IREQT4	HwTIREQ_TI4
	#define	IO_TMR_IREQT5	HwTIREQ_TI5
#endif
#define	IO_TMR_ClearTIREQ(X)	{ HwTIREQ = 1 << (X); }

/**********************************************************
*	void IO_TMR_SetTIMER(unsigned uCH, unsigned uCTRL, unsigned uTREF, unsigned uTMREF);
*
*	Input		: uCH = Select timer channel. 0~5 is available.
*				  uCTRL = Timer Control Register (HwTCFG) value.
*				  uTREF = Timer Reference Register (HwTREF) value.
*				  uTMREF = Timer Middle Reference Register (HwTMREF) value.
*	Return		:
*	Description	: Set and Enable a Timer/Counter (timer is automatically enabled regardless of uCTRL value)
**********************************************************/
void IO_TMR_SetTIMER(unsigned uCH, unsigned uCTRL, unsigned uTREF, unsigned uTMREF);

/**********************************************************
*	unsigned IO_TMR_GetTIMER(unsigned uCH);
*
*	Input		: uCH = Select timer channel. 0~5 is available.
*	Return		: Current TCNT value.
*	Description	: Get the current count value of channel.
**********************************************************/
unsigned IO_TMR_GetTIMER(unsigned uCH);

/**********************************************************
*	void IO_TMR_DisableTIMER(unsigned uCH);
*
*	Input		: uCH = Select timer channel. 0~5 is available.
*	Return		:
*	Description	: Disable a Timer
**********************************************************/
void IO_TMR_DisableTIMER(unsigned uCH);

/**********************************************************
*	void IO_TMR_EnableTIMER(unsigned uCH);
*
*	Input		: uCH = Select timer channel. 0~5 is available.
*	Return		:
*	Description	: Enable a Timer
**********************************************************/
void IO_TMR_EnableTIMER(unsigned uCH);

#define	DBG_MAX_MEASURE	32
#define	START_MEASURE		1
#define	STOP_MEASURE		0

typedef	struct
{
	unsigned	uStamp;		// Time Stamp value at the start time.
	char		*pDescription;		// Timer description string.
	unsigned	uPreCH;		// Bitmap of Channels that must to be enabled ahead to measure this channel.
	unsigned	uMin;		// 1tic ~= 2.67us
	unsigned	uMax;		// 1tic ~= 2.67us
	unsigned	long long	uSum;		// summation of duration
	unsigned	uNum;		// Number of durations measured. The average duration is acquired by (uSum / uNum)
} sDBG_Timer;

extern	sDBG_Timer	DBG_Timer[DBG_MAX_MEASURE];
extern	unsigned		uMEA_CTRL, uMEA_STATE;

/**********************************************************
*	void IO_TMR_InitMEASURE(unsigned uCH);
*
*	Input		: uCH = Select a channel for initialize. (0xFFFFFFFF, 0 ~ 31 are possible, 0xFFFFFFFF means all of channel)
*						(this is not same as physical timer channel number. this uses TIMER channel 4 only)
*	Return		:
*	Description	: Initialize MEASURE variables.
*				  If all of channel should be initialized, use -1 as channel number.
**********************************************************/
void IO_TMR_InitMEASURE(unsigned uCH);

/**********************************************************
*	void IO_TMR_StartMEASURE(unsigned uCH, char *pDescriptor);
*
*	Input		: uCH = Select a channel for measure. (0 ~ 31 are possible)
*						(this is not same as physical timer channel number. this uses TIMER channel 4 only)
*				  pDescriptor = string for describing this channel.
*	Return		:
*	Description	: Start the timer for measuring a duration.
*				  This uses only timer/counter 4.
*				  It can measure the min/max/avg time duration of certain operation with limited period.
**********************************************************/
void IO_TMR_StartMEASURE(unsigned uCH, unsigned uPreCH, char *pDescriptor);

/**********************************************************
*	void IO_TMR_FinishMEASURE(unsigned uCH);
*
*	Input		: uCH = Select a channel for measure. (0 ~ 31 are possible)
*						(this is not same as physical timer channel number. this uses TIMER channel 4 only)
*	Return		:
*	Description	: Finish the timer for measuring a duration.
*				  This uses only timer/counter 4.
*				  It stops measuring, and update min/max/sum/num field.
**********************************************************/
void IO_TMR_FinishMEASURE(unsigned uCH);

/**********************************************************
*	void IO_TMR_GetMEASURE(unsigned uCH, unsigned *pAvg, unsigned *pNum, unsigned *pMin, unsigned *pMax);
*
*	Input		: uCH = Select a channel for measure. (0 ~ 31 are possible)
*						(this is not same as physical timer channel number. this uses TIMER channel 4 only)
*				  pAvg = pointer for containing average time
*				  pNum = pointer for containing number of times
*				  pMin = pointer for containing minimum time
*				  pMax = pointer for containing maximum time
*				  *) all pointers can be zero not to contain values.
*	Return		:
*	Description	:
**********************************************************/
void IO_TMR_GetMEASURE(unsigned uCH, unsigned *pAvg, unsigned *pNum, unsigned *pMin, unsigned *pMax);

// Time Duration Meter
#ifndef USE_DOMEASURE
	#define	DoMEASURE(CH, ONOFF, PreCH, Descriptor)			{;}
#else
	#define	DoMEASURE(CH, ONOFF, PreCH, Descriptor)			\
	{														\
		if (uMEA_CTRL & (1 << (CH)))							\
		{													\
			if ((ONOFF) != 0)									\
				IO_TMR_StartMEASURE(CH, PreCH, Descriptor);		\
			else												\
				IO_TMR_FinishMEASURE(CH);					\
		}													\
	}
#endif

/************************************************************************
*	UART Controller
************************************************************************/

/**********************************************************
*
*	void IO_UART_Test(unsigned uCH);
*
*	Input		: uCH = channel number (0~2)
*	Output		:
*	Return		:
*
*	Description	: Test UART functions.
**********************************************************/
void IO_UART_Test(unsigned uCH);

/**********************************************************
*
*	void IO_UART_Init(unsigned uCH);
*
*	Input		: uCH = channel number (0~2)
*	Output		:
*	Return		:
*
*	Description	: Initialize UART registers and UART Clocks.
**********************************************************/
void IO_UART_Init(unsigned uCH);

/**********************************************************
*
*	void IO_UART_WriteString(unsigned uCH, const char *ccptrString)
*
*	Input		: uCH = channel number (0~2)
*				  ccptrString = string to print
*	Output		:
*	Return		:
*
*	Description	: print argument string considering '\n' as '\r'+'n'.
**********************************************************/
void IO_UART_WriteString(unsigned uCH, const char *ccptrString);

/**********************************************************
*
*	void IO_UART_WriteByte(unsigned uCH, char cChar)
*
*	Input		: uCH = channel number (0~2)
*				  cChar = character to print
*	Output		:
*	Return		:
*
*	Description	: Print one character. Consider '\n' as '\r' + '\n'.
**********************************************************/
void IO_UART_WriteByte(unsigned uCH, char cChar);

/**********************************************************
*
*	int IO_UART_InputByte(unsigned uCH, char *cptrChar)
*
*	Input		: uCH = channel number (0~2)
*				  cptrChar = pointer for receiving character
*	Output		:
*	Return		: 0 = there is no input.
*				  1 = there exist at least one byte and it is contained at the *cptrChar.
*	Description	: Check there is at least one character in buffer, and if exist, store the code to *cptrChar and return 1.
*				   Or return 0.
**********************************************************/
int IO_UART_InputByte(unsigned uCH, char *cptrChar);

/**********************************************************
*
*	void IO_UART0_PutExtChar(unsigned uCH, const unsigned char cucChar)
*
*	Input		: uCH = channel number (0~2)
*				  cucChar = character code
*	Output		: Send one character
*	Return		:
*
*	Description	: Print cucChar if cucChar is printable code or print '.' character.
**********************************************************/
void IO_UART_PutExtChar(unsigned uCH, const unsigned char cucChar);

/**********************************************************
*
*	char IO_UART_GetChar(unsigned uCH)
*
*	Input		: uCH = channel number (0~2)
*	Output		:
*	Return		: Received character
*
*	Description	: Wait until at least one character is received & return the code with echoing.
**********************************************************/
char IO_UART_GetChar(unsigned uCH);

/**********************************************************
*
*	char IO_UART_GetCh(unsigned uCH)
*
*	Input		: uCH = channel number (0~2)
*	Output		:
*	Return		: Received character
*
*	Description	: Wait until at least one character is received & return the code without echoing.
**********************************************************/
char IO_UART_GetCh(unsigned uCH);


/************************************************************************
*	RTC Controller
************************************************************************/
typedef struct {
	unsigned	char		second;		// (0 ~ 59)
	unsigned	char		minute;		// (0 ~ 59)
	unsigned	char		hour;		// (0 ~ 23)
	unsigned	char		day;			// Day of Week (SUN=0, MON, TUE, WED, THR, FRI, SAT=6)

	unsigned	char		date;		// (1 ~ 28,29,30,31)
	unsigned	char		month;		// (1 ~ 12)
	unsigned short	year;
} IO_RTC_DATETIME;

/**********************************************************************************
*	unsigned IO_RTC_Init(void);
*
*	Input		:
*	Return		: 0
*	Description	: Initialize RTC, RTC is disabled.
*				  This function should not be called in normal case.
**********************************************************************************/
unsigned IO_RTC_Init(void);

/**********************************************************************************
*	unsigned IO_RTC_Start(void);
*
*	Input		:
*	Return		: value of RTCCON register after enabled.
*	Description	: RTC starts to operate.
**********************************************************************************/
unsigned IO_RTC_Start(void);

/**********************************************************************************
*	unsigned IO_RTC_Stop(void);
*
*	Input		:
*	Return		: value of RTCCON register after disabled.
*	Description	: RTC stops to operate.
**********************************************************************************/
unsigned IO_RTC_Stop(void);

/**********************************************************************************
*	unsigned IO_RTC_SetCON(unsigned uRTCCON);
*
*	Input		: uRTCCON = value for RTCCON register.
*	Return		: value of RTCCON register after updated.
*	Description	: Set RTCCON register as wanted value.
**********************************************************************************/
unsigned IO_RTC_SetCON(unsigned uRTCCON);

/**********************************************************************************
*	unsigned	IO_RTC_GetTIME(IO_RTC_DATETIME *pTime);
*	Input		: pTime = structure for getting RTC Time. (each element has decimal (non-BCD) value.)
*	Return		: 0 = OK
*				  1 = Read value has some error, and pTime contains predefined initial values
*					 for calling IO_RTC_SetTIME();
*	Description	: Get current time of RTC.
*				  RTC has no power-on reset feature, it has random value after power-on.
*				  It is reported by return value of 1 so user must re-set the current time.
**********************************************************************************/
unsigned	IO_RTC_GetTIME(IO_RTC_DATETIME *pTime);

/**********************************************************************************
*	unsigned	IO_RTC_IsValidTime(void);
*
*	Input		: void
*	Return		: 0 = OK
*				  1 = Read value has some error
*	Description	:This function is made to check whether current time setteing is correct or not
*				 Maybe this function will be called by only the Janus core and will be used for setting
*				 current RTC state(SET or UNSET).
**********************************************************************************/
unsigned	IO_RTC_IsValidTime(void);

/**********************************************************************************
*	unsigned	IO_RTC_SetTIME(RTC_APP_DATETIME *pTime);
*
*	Input		: pTime = structure for setting RTC Time (refer to IO_RTC_GetTIME())
*	Return		: 0
*	Description	:
**********************************************************************************/
unsigned	IO_RTC_SetTIME(IO_RTC_DATETIME *pTime);

/**********************************************************************************
*	unsigned	IO_RTC_SetBCDALARM(IO_RTC_DATETIME *pTime, unsigned uCON);
*
*	Input		: pTime = structure for setting RTC Time. (each element has BCD format)
*				  uCON = same as HwRTCALM register map (refer to datasheet)
*	Return		: 0
*	Description	: Set ALARM time. It is not supported all combination of ALARM time.
**********************************************************************************/
unsigned	IO_RTC_SetBCDALARM(IO_RTC_DATETIME *pTime, unsigned uCON);

/**********************************************************************************
*	unsigned	IO_RTC_GetBCDTIME(RTC_APP_DATETIME *pTime);
*
*	Input		: pTime = structure for getting RTC Time. (BCD format)
*	Return		: 0
*	Description	: The current time is stored to structure pointed by pTime.
**********************************************************************************/
unsigned	IO_RTC_GetBCDTIME(IO_RTC_DATETIME *pTime);

/**********************************************************************************
*	unsigned	IO_RTC_SetBCDTIME(RTC_APP_DATETIME *pTime);
*
*	Input		: pTime = structure for setting RTC Time. (BCD format)
*	Return		: 0
*	Description	:
**********************************************************************************/
unsigned	IO_RTC_SetBCDTIME(IO_RTC_DATETIME *pTime);

/**********************************************************************************
*	unsigned	IO_RTC_BCD2DEC( unsigned nBCD );
*
*	Input		: nBCD = BCD format value
*	Return		: Equivalent value of hexa-decimal format
*	Description	:
**********************************************************************************/
unsigned IO_RTC_BCD2DEC( unsigned nBCD );

/**********************************************************************************
*	unsigned	IO_RTC_DEC2BCD( unsigned uDEC );
*
*	Input		: nDEC = hexa-decimal format value
*	Return		: Equivalent value of BCD format
*	Description	:
**********************************************************************************/
unsigned IO_RTC_DEC2BCD( unsigned nDEC );

/**********************************************************************************
*	unsigned	IO_RTC_SetALARM(IO_RTC_DATETIME *pTime);
*
*	Input		: pTime = structure for setting RTC Time. (Hexa-decimal format)
*	Output		:
*	Return		: 0 = OK
*	Description	:
**********************************************************************************/
unsigned	IO_RTC_SetALARM(IO_RTC_DATETIME *pTime);

/**********************************************************************************
*	unsigned	IO_RTC_GetALARM(IO_RTC_DATETIME *pTime);
*
*	Input		: pTime = structure for setting RTC Time. (Hexa-decimal format)
*	Output		:
*	Return		: 0 = OK
*	Description	:
**********************************************************************************/
unsigned	IO_RTC_GetALARM(IO_RTC_DATETIME *pTime);

/**********************************************************************************
*	unsigned	IO_RTC_WriteREG(volatile unsigned *pReg, unsigned uValue);
*
*	Input		: pReg = Register Address (BCD register address)
*				  uValue = Register Value
*	Return		: Register Value after writing.
*	Description	:
**********************************************************************************/
unsigned	IO_RTC_WriteREG(volatile unsigned *pReg, unsigned uValue);

/************************************************************************
*	Debug Monitor
************************************************************************/
// Print Character for time stamp.
#define	ST_ON	'1'
#define	ST_OFF	'0'
#ifndef WINVER
#ifdef	USE_IO_DEBUG
	//----------definition for GLOBAL monitoring ([7:0] are allocated)
	#define	IO_DBG_Init			IO_DBG_Init_
	#define	IO_DBG_Printf		IO_DBG_Printf_
	#define	IO_DBG_SerialPrintf	IO_DBG_SerialPrintf_
	#define	IO_DBG_Putc			IO_DBG_Putc_
	#define	IO_DBG_TIME			HwTCNT4

	//----------definition for GLOBAL monitoring ([-:0] are allocated)
	#define	DBG_CTRL_USBD		Hw0
	#define	DBG_CTRL_NFC		Hw1
	#define	DBG_CTRL_DTCM		Hw2
	#define	DBG_CTRL_SSFDC		Hw3
	#define	DBG_CTRL_SSFDC_DRV	Hw4
	#define	DBG_CTRL_FILE		Hw5
	#define	DBG_CTRL_FAT		Hw6
	#define	DBG_CTRL_MP3DEC	Hw7
	//----------stamp definition ([31:-] are allocated)
	#define	DBG_CTRL_STAMP0	Hw31
	#define	DBG_CTRL_STAMP1	Hw30
	#define	DBG_CTRL_STAMP2	Hw29
	#define	DBG_CTRL_STAMP3	Hw28
	#define	DBG_CTRL_STAMP4	Hw27
	#define	DBG_CTRL_STAMP5	Hw26
	#define	DBG_CTRL_STAMP6	Hw25
	#define	DBG_CTRL_STAMP7	Hw24
#else
	// Disable All of monitoring functions
	#define	IO_DBG_Init()			{;}
	#define	IO_DBG_Printf(...)	
	#define	IO_DBG_SerialPrintf(...)	//
	#define	IO_DBG_Putc			//
	#define	IO_DBG_TIME			0

	#define	DBG_CTRL_USBD		0
	#define	DBG_CTRL_NFC		0
	#define	DBG_CTRL_DTCM		0
	#define	DBG_CTRL_SSFDC		0
	#define	DBG_CTRL_SSFDC_DRV	0
	#define	DBG_CTRL_FILE		0
	#define	DBG_CTRL_FAT		0
	#define	DBG_CTRL_MP3DEC	0
	#define	DBG_CTRL_STAMP0	0
	#define	DBG_CTRL_STAMP1	0
	#define	DBG_CTRL_STAMP2	0
	#define	DBG_CTRL_STAMP3	0
	#define	DBG_CTRL_STAMP4	0
	#define	DBG_CTRL_STAMP5	0
	#define	DBG_CTRL_STAMP6	0
	#define	DBG_CTRL_STAMP7	0
#endif
#endif
#ifdef	CHECK_SPEED
	// Stamp for
	#define	MakeSTAMP0(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP0) IO_DBG_Printf("[%05x]T0(%c)\n", IO_DBG_TIME, X);}

	// Stamp for SSFDC_FS_WriteSector() execution time.
	#define	MakeSTAMP1(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP1) IO_DBG_Printf("[%05x]T1(%c)\n", IO_DBG_TIME, X);}

	// Stamp for SSFDC_WriteSector() execution time.
	#define	MakeSTAMP2(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP2) IO_DBG_Printf("[%05x]T2(%c)\n", IO_DBG_TIME, X);}

	// Stamp for read_file() or write_file() execution time.
	#define	MakeSTAMP3(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP3) IO_DBG_Printf("[%05x]T3(%c)\n", IO_DBG_TIME, X);}

	// Stamp for
	#define	MakeSTAMP4(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP4) IO_DBG_Printf("[%05x]T4(%c)\n", IO_DBG_TIME, X);}

	// Stamp for DISK_WriteSector() execution time.
	#define	MakeSTAMP5(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP5) IO_DBG_Printf("[%05x]T5(%c)\n", IO_DBG_TIME, X);}

	// Stamp for pure NAND Data transfer execution time.
	#define	MakeSTAMP6(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP6) IO_DBG_Printf("[%05x]T6(%c)\n", IO_DBG_TIME, X);}

	// Stamp for SSFDC_IO_R/W() execution time.
	#define	MakeSTAMP7(X)	{if (uDBG_CTRL & DBG_CTRL_STAMP7) IO_DBG_Printf("[%05x]T7(%c)\n", IO_DBG_TIME, X);}
#else
	#define	MakeSTAMP0(X)	{;}
	#define	MakeSTAMP1(X)	{;}
	#define	MakeSTAMP2(X)	{;}
	#define	MakeSTAMP3(X)	{;}
	#define	MakeSTAMP4(X)	{;}
	#define	MakeSTAMP5(X)	{;}
	#define	MakeSTAMP6(X)	{;}
	#define	MakeSTAMP7(X)	{;}
#endif

// definition for USB device monitoring
#define	IO_USBD_Printf	if (uDBG_CTRL & DBG_CTRL_USBD) IO_DBG_Printf
#define	IO_USBD_Putc(x)	if (uDBG_CTRL & DBG_CTRL_USBD) IO_DBG_Putc(x)
// definition for NFC monitoring
#define	IO_NFC_Printf		if (uDBG_CTRL & DBG_CTRL_NFC) IO_DBG_Printf
#define	IO_NFC_Putc(x)	if (uDBG_CTRL & DBG_CTRL_NFC) IO_DBG_Putc(x)
// definition for DTCM monitoring
#define	IO_DTCM_Printf	if (uDBG_CTRL & DBG_CTRL_DTCM) IO_DBG_Printf
#define	IO_DTCM_Putc(x)	if (uDBG_CTRL & DBG_CTRL_DTCM) IO_DBG_Putc(x)
// definition for SSFDC monitoring
#define	IO_SSFDC_Printf	if (uDBG_CTRL & DBG_CTRL_SSFDC) IO_DBG_Printf
#define	IO_SSFDC_Putc(x)	if (uDBG_CTRL & DBG_CTRL_SSFDC) IO_DBG_Putc(x)
// definition for SSFDC_DRV monitoring
#define	IO_SSFDC_DRV_Printf		if (uDBG_CTRL & DBG_CTRL_SSFDC_DRV) IO_DBG_Printf
#define	IO_SSFDC_DRV_Putc(x)	if (uDBG_CTRL & DBG_CTRL_SSFDC_DRV) IO_DBG_Putc(x)
// definition for FILE monitoring
#define	IO_FILE_Printf		if (uDBG_CTRL & DBG_CTRL_FILE) IO_DBG_Printf
#define	IO_FILE_Putc(x)	if (uDBG_CTRL & DBG_CTRL_FILE) IO_DBG_Putc(x)
// definition for FAT monitoring
#define	IO_FAT_Printf		if (uDBG_CTRL & DBG_CTRL_FAT) IO_DBG_Printf
#define	IO_FAT_Putc(x)	if (uDBG_CTRL & DBG_CTRL_FAT) IO_DBG_Putc(x)
// definition for MP3DEC monitoring
#define	IO_MP3DEC_Printf		if (uDBG_CTRL & DBG_CTRL_MP3DEC) IO_DBG_Printf
#define	IO_MP3DEC_Putc(x)	if (uDBG_CTRL & DBG_CTRL_MP3DEC) IO_DBG_Putc(x)

void IO_DBG_Printf_(char *format, ...);
void IO_DBG_SerialPrintf_(char *format, ...);
void IO_DBG_Sprintf(char *dst, char *format, ...);
void IO_DBG_Putc_(char c);
void IO_DBG_Init_(void);

extern	const unsigned	IO_DBG_CRC_TABLE[256];
unsigned IO_DBG_CalcCRC32_s(unsigned crc_in, unsigned data, unsigned size);

/**********************************************************************************
*	int stod(char *s);
*
*	Input		: s = string of decimal or hexa-decimal format
*	Return		: converted value equivalent with s.
*	Description	: if input string starts from "0x", it is regarded as hexa-decimal format,
*				  or it is regarded as decimal format.
**********************************************************************************/
int stod(char *s);

extern char		cDBG_txbuf[];		// all of string that is printed by IO_DBG_Printf() is stored at this buffer.
								// this is ring type buffer.
extern char		cDBG_rxbuf[];
extern char		*p_prtbuf, cDirectPrint;
extern unsigned	uDBG_CTRL, uDBG_txbuf_length;

/************************************************************************
*	ADC
************************************************************************/
extern	unsigned short	uADC_DATA[8];

void IO_ADC_Init(void);
unsigned short IO_ADC_GetDATA(unsigned uCH);
void IO_ADC_Start(unsigned uCH, unsigned uNCH);
unsigned IO_ADC_Stop(unsigned uCH);

/************************************************************************
*	UART
************************************************************************/

#ifdef TCC77X
	#define	IO_UART_CH		0

	#define	IO_UART_LSR(X)	(((X) == 0) ? HwUART0_UTLSR : ((X) == 1) ? HwUART1_LSR : HwUART2_UTLSR)
	#define	IO_UART_RCVD	Hw0
	#define	IO_UART_TF(X)	(((X) == 0) ? HwUART0_UTLSR_TF_NOSTR : ((X) == 1) ? HwUART1_LSR_THRE_ON : HwUART2_UTLSR_TF_NOSTR)
	#define	IO_UART_RXD(X)	(((X) == 0) ? HwUART0_UTRXD : ((X) == 1) ? HwUART1_RBR : HwUART2_UTRXD)
	#define	IO_UART_TXD(X)	(((X) == 0) ? HwUART0_UTTXD : ((X) == 1) ? HwUART1_THR : HwUART2_UTTXD)
	#define	IO_UART_WaitTXRDY(X)										\
	{																	\
		if ((X) == 0)														\
			while (ISONE(HwUART0_UTLSR, HwUART0_UTLSR_TF_NOSTR));		\
		else if ((X) == 1)													\
			while (ISZERO(HwUART1_LSR, HwUART1_LSR_THRE_ON));			\
		else if ((X) == 2)													\
			while (ISONE(HwUART2_UTLSR, HwUART2_UTLSR_TF_NOSTR));		\
	}
	#define	IO_UART_WaitRXRDY(X)										\
	{																	\
		if ((X) == 0)														\
			while (ISZERO(HwUART0_UTLSR, HwUART0_UTLSR_RA_RECV));		\
		else if ((X) == 1)													\
			while (ISZERO(HwUART1_LSR, HwUART1_LSR_DR_ON));			\
		else if ((X) == 2)													\
			while (ISZERO(HwUART2_UTLSR, HwUART2_UTLSR_RA_RECV));		\
	}
	#define	IO_UART_RXRDY(X)	( ((X) == 0) ? HwUART0_UTLSR & IO_UART_RCVD : \
								  ((X) == 1) ? HwUART1_LSR & IO_UART_RCVD : HwUART2_UTLSR & IO_UART_RCVD )
	#define	IO_UART_LSR_ERR(X)	( ((X) == 0) ? (HwUART0_UTLSR_FE_ERR | HwUART0_UTLSR_PE_ERR) :		\
								  ((X) == 1) ? (HwUART1_LSR_FE_ON | HwUART1_LSR_PE_ON) :			\
								  			(HwUART2_UTLSR_FE_ERR | HwUART2_UTLSR_PE_ERR) )
	#define	IO_UART_ERR_Frame(X)	( ((X) == 0) ? HwUART0_UTLSR_FE_ERR : \
									  ((X) == 1) ? HwUART1_LSR_FE_ON : HwUART2_UTLSR_FE_ERR )
	#define	IO_UART_ERR_Parity(X)	( ((X) == 0) ? HwUART0_UTLSR_PE_ERR : \
									  ((X) == 1) ? HwUART1_LSR_PE_ON : HwUART2_UTLSR_PE_ERR )
#endif
#ifdef TCC87XX
	#define	IO_UART_CH		0

	#define	IO_UART_LSR(X)	(((X) == 0) ? HwUART0_UTLSR : ((X) == 1) ? HwUART1_LSR : HwUART2_UTLSR)
	#define	IO_UART_RCVD	Hw0
	#define	IO_UART_TF(X)	(((X) == 0) ? HwUART0_UTLSR_TF_NOSTR : ((X) == 1) ? HwUART1_LSR_THRE_ON : HwUART2_UTLSR_TF_NOSTR)
	#define	IO_UART_RXD(X)	(((X) == 0) ? HwUART0_UTRXD : ((X) == 1) ? HwUART1_RBR : HwUART2_UTRXD)
	#define	IO_UART_TXD(X)	(((X) == 0) ? HwUART0_UTTXD : ((X) == 1) ? HwUART1_THR : HwUART2_UTTXD)
	#define	IO_UART_WaitTXRDY(X)										\
	{																	\
		if ((X) == 0)														\
			while (ISONE(HwUART0_UTLSR, HwUART0_UTLSR_TF_NOSTR));		\
		else if ((X) == 1)													\
			while (ISZERO(HwUART1_LSR, HwUART1_LSR_THRE_ON));			\
		else if ((X) == 2)													\
			while (ISONE(HwUART2_UTLSR, HwUART2_UTLSR_TF_NOSTR));		\
	}
	#define	IO_UART_WaitRXRDY(X)										\
	{																	\
		if ((X) == 0)														\
			while (ISZERO(HwUART0_UTLSR, HwUART0_UTLSR_RA_RECV));		\
		else if ((X) == 1)													\
			while (ISZERO(HwUART1_LSR, HwUART1_LSR_DR_ON));			\
		else if ((X) == 2)													\
			while (ISZERO(HwUART2_UTLSR, HwUART2_UTLSR_RA_RECV));		\
	}
	#define	IO_UART_RXRDY(X)	( ((X) == 0) ? HwUART0_UTLSR & IO_UART_RCVD : \
								  ((X) == 1) ? HwUART1_LSR & IO_UART_RCVD : HwUART2_UTLSR & IO_UART_RCVD )
	#define	IO_UART_LSR_ERR(X)	( ((X) == 0) ? (HwUART0_UTLSR_FE_ERR | HwUART0_UTLSR_PE_ERR) :		\
								  ((X) == 1) ? (HwUART1_LSR_FE_ON | HwUART1_LSR_PE_ON) :			\
								  			(HwUART2_UTLSR_FE_ERR | HwUART2_UTLSR_PE_ERR) )
	#define	IO_UART_ERR_Frame(X)	( ((X) == 0) ? HwUART0_UTLSR_FE_ERR : \
									  ((X) == 1) ? HwUART1_LSR_FE_ON : HwUART2_UTLSR_FE_ERR )
	#define	IO_UART_ERR_Parity(X)	( ((X) == 0) ? HwUART0_UTLSR_PE_ERR : \
									  ((X) == 1) ? HwUART1_LSR_PE_ON : HwUART2_UTLSR_PE_ERR )
#endif
#ifdef TCC82XX
	#define	IO_UART_CH		0

	#define	IO_UART_LSR(X)	(((X) == 0) ? HwUART0_UTLSR : ((X) == 1) ? HwUART1_LSR : HwUART2_UTLSR)
	#define	IO_UART_RCVD	Hw0
	#define	IO_UART_TF(X)	(((X) == 0) ? HwUART0_UTLSR_TF_NOSTR : ((X) == 1) ? HwUART1_LSR_THRE_ON : HwUART2_UTLSR_TF_NOSTR)
	#define	IO_UART_RXD(X)	(((X) == 0) ? HwUART0_UTRXD : ((X) == 1) ? HwUART1_RBR : HwUART2_UTRXD)
	#define	IO_UART_TXD(X)	(((X) == 0) ? HwUART0_UTTXD : ((X) == 1) ? HwUART1_THR : HwUART2_UTTXD)
	#define	IO_UART_WaitTXRDY(X)										\
	{																	\
		if ((X) == 0)														\
			while (ISONE(HwUART0_UTLSR, HwUART0_UTLSR_TF_NOSTR));		\
		else if ((X) == 1)													\
			while (ISZERO(HwUART1_LSR, HwUART1_LSR_THRE_ON));			\
		else if ((X) == 2)													\
			while (ISONE(HwUART2_UTLSR, HwUART2_UTLSR_TF_NOSTR));		\
	}
	#define	IO_UART_WaitRXRDY(X)										\
	{																	\
		if ((X) == 0)														\
			while (ISZERO(HwUART0_UTLSR, HwUART0_UTLSR_RA_RECV));		\
		else if ((X) == 1)													\
			while (ISZERO(HwUART1_LSR, HwUART1_LSR_DR_ON));			\
		else if ((X) == 2)													\
			while (ISZERO(HwUART2_UTLSR, HwUART2_UTLSR_RA_RECV));		\
	}
	#define	IO_UART_RXRDY(X)	( ((X) == 0) ? HwUART0_UTLSR & IO_UART_RCVD : \
								  ((X) == 1) ? HwUART1_LSR & IO_UART_RCVD : HwUART2_UTLSR & IO_UART_RCVD )
	#define	IO_UART_LSR_ERR(X)	( ((X) == 0) ? (HwUART0_UTLSR_FE_ERR | HwUART0_UTLSR_PE_ERR) :		\
								  ((X) == 1) ? (HwUART1_LSR_FE_ON | HwUART1_LSR_PE_ON) :			\
								  			(HwUART2_UTLSR_FE_ERR | HwUART2_UTLSR_PE_ERR) )
	#define	IO_UART_ERR_Frame(X)	( ((X) == 0) ? HwUART0_UTLSR_FE_ERR : \
									  ((X) == 1) ? HwUART1_LSR_FE_ON : HwUART2_UTLSR_FE_ERR )
	#define	IO_UART_ERR_Parity(X)	( ((X) == 0) ? HwUART0_UTLSR_PE_ERR : \
									  ((X) == 1) ? HwUART1_LSR_PE_ON : HwUART2_UTLSR_PE_ERR )
#endif
#ifdef TCC83XX
	//TODO:
	//Bruce_temp_83.. jsh
#endif

void IO_UART_Test(unsigned uCH);
void IO_UART_Init(unsigned uCH);
void IO_UART_WriteString(unsigned uCH, const char *ccptrString);
void IO_UART_WriteByte(unsigned uCH, char cChar);
int IO_UART_InputByte(unsigned uCH, char *cptrChar);
void IO_UART_PutExtChar(unsigned uCH, const unsigned char cucChar);
char IO_UART_GetChar(unsigned uCH);
char IO_UART_GetCh(unsigned uCH);

/************************************************************************
*	GSIO & SPIS
************************************************************************/

#define	IO_GSIO_HwEN			Hw31
#define	IO_GSIO_HwMSB1ST		Hw30
#define	IO_GSIO_HwLSB1ST		HwZERO
#define	IO_GSIO_HwWSIZE(X)		(((X)-1)*Hw26)
#define	IO_GSIO_HwWSDYNAMIC	Hw25
#define	IO_GSIO_HwDIV(X)		((X) * Hw18)
#define	IO_GSIO_HwWSFIX		HwZERO
#define	IO_GSIO_HwPOSSYNC		Hw17
#define	IO_GSIO_HwNEGSYNC		HwZERO
#define	IO_GSIO_HwMASKLSCK	Hw16
#define	IO_GSIO_HwIEN			Hw15
#define	IO_GSIO_HwTXDLY(X)		((X)*Hw13)
#define	IO_GSIO_HwFRMACTHIGH	Hw12
#define	IO_GSIO_HwFRMACTLOW	HwZERO
#define	IO_GSIO_HwFRMST(X)		((X)*Hw6)
#define	IO_GSIO_HwFRMEND(X)	((X)*Hw0)

typedef	volatile struct
{
	unsigned	DO;
	unsigned	DI;
	unsigned	CTRL;
	unsigned	dummy;
} sHwGSIO;

#define	IO_SPIS_HwTXFIFOCNT(X)		(((X)-1)*Hw29)
#define	IO_SPIS_HwRXFIFOCNT(X)		(((X)-1)*Hw26)
#define	IO_SPIS_HwISRC_RXCFULL		(0*Hw8)
#define	IO_SPIS_HwISRC_RXFEMPTY	(1*Hw8)
#define	IO_SPIS_HwISRC_RXFFULL		(2*Hw8)
#define	IO_SPIS_HwISRC_TXCFULL		(4*Hw8)
#define	IO_SPIS_HwISRC_TXFEMPTY	(5*Hw8)
#define	IO_SPIS_HwISRC_TXFFULL		(6*Hw8)
#define	IO_SPIS_HwMSB1ST			Hw5
#define	IO_SPIS_HwLSB1ST			HwZERO
#define	IO_SPIS_HwWSIZE(X)			(((X)/8-1)*Hw3)
#define	IO_SPIS_HwPOSSYNC			Hw2
#define	IO_SPIS_HwNEGSYNC			HwZERO
#define	IO_SPIS_HwIEN				Hw1
#define	IO_SPIS_HwEN				Hw0
typedef struct
{
	unsigned	CTRL;
	unsigned	DO;
	unsigned	DI;
	unsigned	dummy;
} sHwSPIS;

#define	IO_GSIO_WaitBUSY(CH, Tout)		{ Tout = 300;	 while (ISONE(HwGSGCR, Hw0 << (CH)) && (Tout --));	}
#define	IO_SPIS_IsRXEMPTY(pSPIS)			ISONE((pSPIS)->CTRL, HwSPCTRL_EMP_RX)
#define	IO_SPIS_IsRXFULL(pSPIS)			ISONE((pSPIS)->CTRL, HwSPCTRL_FUL_RX)
#define	IO_SPIS_WaitRX(pSPIS, Tout)		{ Tout = 300; while (IO_SPIS_IsRXEMPTY(pSPIS) && (Tout --));	}
#define	IO_SPIS_IsTXEMPTY(pSPIS)			ISONE((pSPIS)->CTRL, HwSPCTRL_EMP_TX)
#define	IO_SPIS_IsTXFULL(pSPIS)			ISONE((pSPIS)->CTRL, HwSPCTRL_FUL_TX)
#define	IO_SPIS_WaitTX(pSPIS, Tout)		{ Tout = 300; while (IO_SPIS_IsTXFULL(pSPIS) && (Tout --));	}

/**********************************************************
*	void IO_GSIO_InitCH(unsigned uCH, unsigned uCONTROL, unsigned uSCKfreq);
*
*	Input		: uCH = Select GSIO Master channel. 0~1 is available.
*				  uCONTROL = GSIO control flags
*					[31] = Enable(1)
*					[30] = MSB First (1)
*					[29:26] = Word Size (bit unit)
*					[25] = Word size is dynamically controlled by GSDO register (1)
*					[17] = Data transition occurs at the SCK rising (1)
*					[16] = Mask out the last SCK (1)
*					[15]	= Enable Interrupt (1)
*					[14:13] = Transmission starting delay (1~3 is available)
*					[12] = FRM is high active pulse
*					[11:6] = FRM pulse start position
*					[5:0] = FRM pulse end position
*				  uSCKfreq = GSIO SCK clock frequency
*	Return		:
*	Description	: Set GSIO host channel
**********************************************************/
void IO_GSIO_InitCH(unsigned uCH, unsigned uCONTROL, unsigned uSCKfreq);

/**********************************************************
*	void IO_SPIS_InitCH(unsigned uCH, unsigned uCONTROL);
*
*	Input		: uCH = Select SPI slave channel. 0 is available.
*				  uCONTROL = GSIO control flags
*					[31:29] = TX FIFO count
*					[28:26] = RX FIFO count
*					[10:8] = Interrupt Source Selection
*						0 : RX FIFO Counter Full
*						1 : RX FIFO Empty
*						2 : RX FIFO Full
*						4 : TX FIFO Counter Full
*						5 : TX FIFO Empty
*						6 : TX FIFO Full
*					[5] = MSB First (1)
*					[4:3] = Word Size
*						0 : 8bit, 1 : 16bit, 2 : 24bit, 3 : 32bit
*					[2] = Data transition occurs at the SCK rising (1)
*					[1] = Enable Interrupt (1)
*					[0] = Enable (1)
*	Return		:
*	Description	: Set SPI slave channel
**********************************************************/
void IO_SPIS_InitCH(unsigned uCH, unsigned uCONTROL);

enum
{
	LCD_18BIT_SET,
	LCD_16BIT_SET,	
	LCD_8BIT_SET
};

/*
 * SPI Functions
 */
void IO_SPI_Init(unsigned uCH, unsigned uSPICTL);
void IO_SPI_SetFrameLength(unsigned uCH, unsigned uFrameLen);
void IO_SPI_ReadBytes(unsigned uCH, unsigned uLength, unsigned char *pData);
void IO_SPI_WriteBytes(unsigned uCH, unsigned uLength, unsigned char* pData);
void IO_SPI_ClearTxFIFO(unsigned uCH);
void IO_SPI_ClearRxFIFO(unsigned uCH);
void IO_CKC_SetSPIClock(unsigned uCH);

void IO_LCD_Init(unsigned char ucCH, unsigned char ucBusWidth, unsigned long ulStp, unsigned long ulPw, unsigned long ulHold );
void IO_LCD_SendCmd0(unsigned long ulCmd);
void IO_LCD_SendCmd1(unsigned long ulCmd);
void IO_LCD_SendData0(unsigned long ulData);
void IO_LCD_SendData1(unsigned long ulData);

#endif	// of __IO_TCC7XX_H
