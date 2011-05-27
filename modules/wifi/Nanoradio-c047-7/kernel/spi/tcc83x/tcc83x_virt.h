/************************************************************************
*	TCC83x Board Support Package
*	------------------------------------------------
*
*	FUNCTION	: TCC83x REGISTER DEFINE FUNCTION
*	MODEL		: TCC83x Linux
*	CPU	NAME	: TCC8301
*	SOURCE		: tcc83x_virt.h
*
*	START DATE	: 2008 Jan. 29
*	MODIFY DATE	:
*	DEVISION	: DEPT. SYSTEM 4-3 TEAM
*				: TELECHIPS, INC.
************************************************************************/

/************************************************************************
*	TCC83x Internal Register Definition File
************************************************************************/
#ifndef	__TCC83x_H__
#define	__TCC83x_H__

/************************************************************************
*	Bit Field Definition
************************************************************************/
//////////
//Done...
//#define TCC_FONTFILE_NAME	"770Font_.TCC" 

#define	Hw32	0x100000000ULL
#define	Hw31	0x80000000
#define	Hw30	0x40000000
#define	Hw29	0x20000000
#define	Hw28	0x10000000
#define	Hw27	0x08000000
#define	Hw26	0x04000000
#define	Hw25	0x02000000
#define	Hw24	0x01000000
#define	Hw23	0x00800000
#define	Hw22	0x00400000
#define	Hw21	0x00200000
#define	Hw20	0x00100000
#define	Hw19	0x00080000
#define	Hw18	0x00040000
#define	Hw17	0x00020000
#define	Hw16	0x00010000
#define	Hw15	0x00008000
#define	Hw14	0x00004000
#define	Hw13	0x00002000
#define	Hw12	0x00001000
#define	Hw11	0x00000800
#define	Hw10	0x00000400
#define	Hw9		0x00000200
#define	Hw8		0x00000100
#define	Hw7		0x00000080
#define	Hw6		0x00000040
#define	Hw5		0x00000020
#define	Hw4		0x00000010
#define	Hw3		0x00000008
#define	Hw2		0x00000004
#define	Hw1		0x00000002
#define	Hw0		0x00000001
#define	HwZERO	0x00000000

#define	HwVERSION		0

//Done...
/////////

/************************************************************************
*	DAI & SPDIF & Audio DMA & CDIF Register Define					(Base Addr = 0xF1000000)
************************************************************************/
//////////
//Done...
#define	HwDAI_BASE				*(volatile unsigned long *)0xF1000000		// R, Digital Audio Left Input Register 0

#define	HwDADI_L0				*(volatile unsigned long *)0xF1000000		// R, Digital Audio Left Input Register 0

#define	HwDADI_R0				*(volatile unsigned long *)0xF1000004		// R, Digital Audio Right Input Register 0

#define	HwDADI_L1				*(volatile unsigned long *)0xF1000008		// R, Digital Audio Left Input Register 1

#define	HwDADI_R1				*(volatile unsigned long *)0xF100000C		// R, Digital Audio Right Input Register 1

#define	HwDADI_L2				*(volatile unsigned long *)0xF1000010		// R, Digital Audio Left Input Register 2

#define	HwDADI_R2				*(volatile unsigned long *)0xF1000014		// R, Digital Audio Right Input Register 2

#define	HwDADI_L3				*(volatile unsigned long *)0xF1000018		// R, Digital Audio Left Input Register 3

#define	HwDADI_R3				*(volatile unsigned long *)0xF100001C		// R, Digital Audio Right Input Register 3

#define	HwDADO_L0				*(volatile unsigned long *)0xF1000020		// R/W, Digital Audio Left output Register 0

#define	HwDADO_R0				*(volatile unsigned long *)0xF1000024		// R/W, Digital Audio Right output Register 0

#define	HwDADO_L1				*(volatile unsigned long *)0xF1000028		// R/W, Digital Audio Left output Register 1

#define	HwDADO_R1				*(volatile unsigned long *)0xF100002C		// R/W, Digital Audio Right output Register 1

#define	HwDADO_L2				*(volatile unsigned long *)0xF1000030		// R/W, Digital Audio Left output Register 2

#define	HwDADO_R2				*(volatile unsigned long *)0xF1000034		// R/W, Digital Audio Right output Register 2

#define	HwDADO_L3				*(volatile unsigned long *)0xF1000038		// R/W, Digital Audio Left output Register 3

#define	HwDADO_R3				*(volatile unsigned long *)0xF100003C		// R/W, Digital Audio Right output Register 3

#define	HwDAMR				*(volatile unsigned long *)0xF1000040		// R/W, Digital Audio Mode Register
#define	HwDAMR_BPS_EN		Hw31				// BCLK source select BCLK Pad
#define	HwDAMR_BPS_DIS		~Hw31				// BCLK direct at Master mode
#define	HwDAMR_LPS_EN			Hw30				// LRCK source select LRCK Pad 
#define	HwDAMR_LPS_DIS		~Hw30				// LRCK direct at master mode
#define	HwDAMR_NMD_EN		Hw24				// NEW DAI Bus Mode(LSB Mode) 
#define	HwDAMR_NMD_DIS		~Hw24				// NEW DAI Bus Mode(IIS or MSB)
#define	HwDAMR_RXE_EN		Hw22				// DAI RX Data Sign Extension Enable(sign bit extension)
#define	HwDAMR_RXE_DIS		~Hw22				// DAI RX Data Sign Extension Disable(zero extension)
#define	HwDAMR_RXS_MSB24		~(Hw21|Hw20)		// Bit-pack MSB and 24bit mode
#define	HwDAMR_RXS_MSB16		Hw20				// Bit-pack MSB and 16bit mode
#define	HwDAMR_RXS_LSB24		Hw21				// Bit-pack LSB and 24bit mode
#define	HwDAMR_RXS_LSB16		(Hw21|Hw20)			// Bit-pack LSB and 16bit mode
#define	HwDAMR_TXS_MSB		~Hw19				// Bit-pack MSB mode
#define	HwDAMR_TXS_LSB24		Hw19				// Bit-pack LSB and 24bit mode
#define	HwDAMR_TXS_LSB16		(Hw19|Hw18)			// Bit-pack LSB and 16bit mode
#define	HwDAMR_EN_ON			Hw15				// Enable (DAI Master Enable)
#define	HwDAMR_EN_OFF		~Hw15				// Disable (DAI Master Enable)
#define	HwDAMR_TE_EN			Hw14				// DAI Transimit Enable
#define	HwDAMR_TE_DIS			~Hw14				// DAI Transimit Disable
#define	HwDAMR_RE_EN			Hw13				// DAI Receiver Enable
#define	HwDAMR_RE_DIS			~Hw13				// DAI Receiver Disable
#define	HwDAMR_MD_MSB		Hw12				// Set DAI bus as MSB justified mode
#define	HwDAMR_MD_IIS			~Hw12				// Set DAI bus as IIS bus mode
#define	HwDAMR_SM_INT		Hw11				// DAI system clock is generated by the clock generator block
#define	HwDAMR_SM_EXT		~Hw11				// DAI system clock from external pin
#define	HwDAMR_BM_INT		Hw10				// DAI bit clock is generated by dividing DAI system clock
#define	HwDAMR_BM_EXT		~Hw10			      	// DAI bit clock is come from external pin
#define	HwDAMR_FM_INT		Hw9					// DAI Frame Clock is generated by dividing DAI bit clock
#define	HwDAMR_FM_EXT		~Hw9				// DAI Frame Clock from external pin
#define	HwDAMR_CC_EN			Hw8					// Enable CDIF Clock master mode
#define	HwDAMR_CC_DIS			~Hw8				// Disable CDIF Clock master mode
#define	HwDAMR_BD_DIV4		~(Hw7|Hw6)			// Select Div4 (256fs->64fs)
#define	HwDAMR_BD_DIV6		Hw6					// Select Div6 (384fs->64fs)
#define	HwDAMR_BD_DIV8		Hw7					// Select Div8 (512fs->64fs, 384fs->48fs, 256fs->32fs)
#define	HwDAMR_BD_DIV16		(Hw7|Hw6)			// Select Div16 (512fs->32fs)
#define	HwDAMR_FD_DIV32		~(Hw5|Hw4)			// Select Div32 (32fs->fs)
#define	HwDAMR_FD_DIV48		Hw4					// Select Div48 (48fs->fs)
#define	HwDAMR_FD_DIV64		Hw5					// Select Div64 (64fs->fs)
#define	HwDAMR_BP_NEGA		Hw3					// Set that data is captured at negative edge of bit clock
#define	HwDAMR_BP_POSI		~Hw3				// Set that data is captured at positive edge of bit clock
#define	HwDAMR_CM_DIS		Hw2					// CDIF Monitor Mode is Disable
#define	HwDAMR_CM_EN			~Hw2				// CDIF Monitor Mode is Enable
#define	HwDAMR_MM_EN			Hw1					// Enable DAI monitor mode. TE bit should be enable.
#define	HwDAMR_MM_DIS		~Hw1				// Disable DAI monitor mode.
#define	HwDAMR_LB_EN			Hw0					// Enable DAI Loop back mode
#define	HwDAMR_LB_DIS			~Hw0				// Disable DAI Loop back mode


#define	HwDAVC					*(volatile unsigned long *)0xF1000044		// R/W, Digital Audio Volume Control Register
#define	HwDAVC_0dB			HwZERO				// 0dB Volume
#define	HwDAVC_6dB			Hw0					// -6dB Volume
#define	HwDAVC_12dB			Hw1					// -12dB Volume
#define	HwDAVC_18dB			(Hw1|Hw0)			// -18dB Volume
#define	HwDAVC_24dB			Hw2					// -24dB Volume
#define	HwDAVC_30dB			(Hw2|Hw0)			// -30dB Volume
#define	HwDAVC_36dB			(Hw2|Hw1)			// -36dB Volume
#define	HwDAVC_42dB			(Hw2|Hw1|Hw0)		// -42dB Volume
#define	HwDAVC_48dB			Hw3					// -48dB Volume
#define	HwDAVC_54dB			(Hw3|Hw0)			// -54dB Volume
#define	HwDAVC_60dB			(Hw3|Hw1)			// -60dB Volume
#define	HwDAVC_66dB			(Hw3|Hw1|Hw0)		// -66dB Volume
#define	HwDAVC_72dB			(Hw3|Hw2)			// -72dB Volume
#define	HwDAVC_78dB			(Hw3|Hw2|Hw0)		// -78dB Volume
#define	HwDAVC_84dB			(Hw3|Hw2|Hw1)		// -84dB Volume
#define	HwDAVC_90dB			(Hw3|Hw2|Hw1|Hw0)	// -90dB Volume
#define	HwDAVC_96dB			Hw4					// -96dB Volume

// CDIF Register Map
#define	HwCDDI_0				*(volatile unsigned long *)0xF1000080		// CD Digital Audio Input Register 0

#define	HwCDDI_1				*(volatile unsigned long *)0xF1000084		// CD Digital Audio Input Register 1

#define	HwCDDI_2				*(volatile unsigned long *)0xF1000088		// CD Digital Audio Input Register 2

#define	HwCDDI_3				*(volatile unsigned long *)0xF100008C		// CD Digital Audio Input Register 3

#define	HwCDDI_4				*(volatile unsigned long *)0xF1000090		// CD Digital Audio Input Register 4

#define	HwCDDI_5				*(volatile unsigned long *)0xF1000094		// CD Digital Audio Input Register 5

#define	HwCDDI_6				*(volatile unsigned long *)0xF1000098		// CD Digital Audio Input Register 6

#define	HwCDDI_7				*(volatile unsigned long *)0xF100009C		// CD Digital Audio Input Register 7

#define	HwCICR					*(volatile unsigned long *)0xF10000A0		//R/W,  CD Interface Control Register

#if 0
/************************************************************************
*	CDIF Register Define			
************************************************************************/
#define	HwCDDI_0				*(volatile unsigned long *)0xF1001480		// CD Digital Audio Input Register 0

#define	HwCDDI_1				*(volatile unsigned long *)0xF1001484		// CD Digital Audio Input Register 1

#define	HwCDDI_2				*(volatile unsigned long *)0xF1001488		// CD Digital Audio Input Register 2

#define	HwCDDI_3				*(volatile unsigned long *)0xF100148C		// CD Digital Audio Input Register 3

#define	HwCDDI_4				*(volatile unsigned long *)0xF1001490		// CD Digital Audio Input Register 4

#define	HwCDDI_5				*(volatile unsigned long *)0xF1001494		// CD Digital Audio Input Register 5

#define	HwCDDI_6				*(volatile unsigned long *)0xF1001498		// CD Digital Audio Input Register 6

#define	HwCDDI_7				*(volatile unsigned long *)0xF100149C		// CD Digital Audio Input Register 7

#define	HwCICR					*(volatile unsigned long *)0xF10014A0		//R/W,  CD Interface Control Register
#endif

/************************************************************************
*	SPDIF Transmitter Register Define			
************************************************************************/
#define	HwTxVersion			*(volatile unsigned long *)0xF1001000		// R, Version Register
#define	HwTxVersion_CSB		Hw12								// Channel Status Buffer Available bit
#define	HwTxVersion_UDB		Hw11								// User Data Buffer Available bit
#define	HwTxVersion_DW		Hw4									// Value of Data Width
 
#define	HwTxConfig				*(volatile unsigned long *)0xF1001004		// R/W, Configuration Register
#define	HwTxConfig_IEN			Hw2									// Interrupt Output Enable
#define	HwTxConfig_TXD			Hw1									// Data Vaild
#define	HwTxConfig_TXEN		Hw0									// Transmitter Enable

#define	HwTxChStat				*(volatile unsigned long *)0xF1001008		// R/W, Channel Status Control Register
#define	HwTxChStat_FREQ_441	HwZERO								// 44.1Khz
#define	HwTxChStat_FREQ_48	Hw6									// 48Khz
#define	HwTxChStat_FREQ_32	Hw7									// 32Khz
#define	HwTxChStat_FREQ_CON	(Hw7+Hw6)							// Sample Rate Converter
#define	HwTxChStat_GSTS		Hw3									// Status Generation
#define	HwTxChStat_PRE			Hw2									// Pre-emphasis
#define	HwTxChStat_CPY			Hw1									// Copyright
#define	HwTxChStat_AU			Hw0									// Data Format

#define	HwTxIntMask			*(volatile unsigned long *)0xF100100C		// R/W, Interrupt Mask Register
#define	HwTxIntMask_HCSB		Hw4									// Higher Channel Status/User Data Buffer Empty
#define	HwTxIntMask_LCSB		Hw3									// Lower Channel Status/User Data Buffer Empty
#define	HwTxIntMask_HSB		Hw2									// Higher Data Buffer Empty
#define	HwTxIntMask_LSB		Hw1									// Lower Data Buffer Empty

#define	HwTxIntStat				*(volatile unsigned long *)0xF1001010		// R/W, Interrupt Status Register
#define	HwTxIntStat_HCSB		Hw4									// Higher Channel Status/User Data Buffer Empty
#define	HwTxIntStat_LCSB		Hw3									// Lower Channel Status/User Data Buffer Empty
#define	HwTxIntStat_HSB		Hw2									// Higher Data Buffer Empty
#define	HwTxIntStat_LSB			Hw1									// Lower Data Buffer Empty

#define	HwUserData				*(volatile unsigned long *)0xF1001080		// W, User Data Buffer 

#define 	HwChStatus				*(volatile unsigned long *)0xF1001100		// W, Channel Status Buffer

#define	HwTxBuffer				*(volatile unsigned long *)0xF1001200		// W, Transmit Data Buffer

#define	HwDMACFG				*(volatile unsigned long *)0xF1001400		// R/W, Additional Configuration for DMA

#define	HwCSBUDB				*(volatile unsigned long *)0xF1001680		// W, Merged Window for CSB/UDB


/************************************************************************
*	Audio DMA Register Define			
************************************************************************/

#define	HwRxDaDar				*(volatile unsigned long *)0xF1002000		// R/W, DAI Rx(Right) Data Destination Address

#define	HwRxDaParam			*(volatile unsigned long *)0xF1002004		// R/W, DAI Rx Parameters

#define	HwRxDaTCnt				*(volatile unsigned long *)0xF1002008		// R/W, DAI Rx Transmission Counter Register

#define	HwRxDaCdar				*(volatile unsigned long *)0xF100200C		// R, DAI Rx(Right) Data Current Destination Address

#define	HwRxCdDar				*(volatile unsigned long *)0xF1002010		// R/W, CDIF Rx(Right) Data Destination Address

#define	HwRxCdParam			*(volatile unsigned long *)0xF1002014		// R/W, CDIF Rx Parameters

#define	HwRxCdTCnt				*(volatile unsigned long *)0xF1002018		// R/W, CDIF Rx Transmission Counter Register

#define	HwRxCdCdar				*(volatile unsigned long *)0xF100201C		// R, CDIF Rx(Right) Data Current Destination Address

#define	HwRxDaDarL				*(volatile unsigned long *)0xF1002028		// R/W, DAI Rx Left Data Destination Address

#define	HwRxDaCdarL			*(volatile unsigned long *)0xF100202C		// R, DAI Rx Left Data Current Destination Address

#define	HwRxCdDarL				*(volatile unsigned long *)0xF1002030		// R/W, CDIF Rx Left Data Destination Address

#define	HwRxCdCdarL			*(volatile unsigned long *)0xF1002034		// R, CDIF Rx Left Data Current Destination Address

#define	HwTxDaSar				*(volatile unsigned long *)0xF1002040		// R/W, DAI Tx(Right) Data Source Address

#define	HwTxDaParam			*(volatile unsigned long *)0xF1002044		// R/W, DAI Tx Parameters

#define	HwTxDaTCnt				*(volatile unsigned long *)0xF1002048		// R/W, DAI Tx Transmission Counter Register

#define	HwTxDaCsar				*(volatile unsigned long *)0xF100204C		// R, DAI Tx(Right)  Data Current Source Address

#define	HwTxSpSar				*(volatile unsigned long *)0xF1002050		// R/W, SPDIF Tx(Right) Data Source Address

#define	HwTxSpParam			*(volatile unsigned long *)0xF1002054		// R/W, SPDIF Tx Parameters

#define	HwTxSpTCnt				*(volatile unsigned long *)0xF1002058		// R/W, SPDIF Tx Transmission Counter Register

#define	HwTxSpCsar				*(volatile unsigned long *)0xF100205C		// R, SPDIF Tx(Right)  Data Current Source Address

#define	HwTxDaSarL				*(volatile unsigned long *)0xF1002068		// R/W, DAI Tx Left Data Source Address

#define	HwTxDaCsarL			*(volatile unsigned long *)0xF100206C		// R, DAT Tx Left Data Current Source Address

#define	HwTxSpSarL				*(volatile unsigned long *)0xF1002070		// R/W, SPDIF Tx Left Data Source Address

#define	HwTxSpCsarL			*(volatile unsigned long *)0xF1002074		// R, SPDIF Tx Left Data Current Source Address





#define	HwTransCtrl				*(volatile unsigned long *)0xF1002038		// R/W, DMA Transfer Control Register

#define	HwTransCtrl_RCN_DIS			~Hw29				//Issue Continuous Transfer of Rx DMA,DMA transfer begins from source/destination address	
#define	HwTransCtrl_RCN_EN			Hw29					//Issue Continuous Transfer of Rx DMA,DMA transfer begins from current source/destination address	
#define	HwTransCtrl_TCN_DIS			~Hw28				//Issue Continuous Transfer of Tx DMA,DMA transfer begins from source/destination address
#define	HwTransCtrl_TCN_EN			Hw28					//Issue Continuous Transfer of Tx DMA,DMA transfer begins from current source/destination address

#define	HwTransCtrl_CRLCK			Hw27					//Issue Locked Transfer of CDIF Rx DMA
#define	HwTransCtrl_DRLCK			Hw26					//Issue Locked Transfer of DAI Rx DMA
#define	HwTransCtrl_STLCK			Hw25					//Issue Locked Transfer of SPDIF Tx DMA
#define	HwTransCtrl_DTLCK			Hw24					//Issue Locked Transfer of DAI Tx DMA

#define	HwTransCtrl_CRTRG_SE		~Hw23					//Trigger Type of Rx DMA,SINGLE edge-triggered detection
#define	HwTransCtrl_CRTRG_SL		Hw23					//Trigger Type of Rx DMA,SINGLE level-triggered detection
#define	HwTransCtrl_DRTRG_SE		~Hw22					//Trigger Type of Tx DMA,SINGLE edge-triggered  detection
#define	HwTransCtrl_DRTRG_SL		Hw22					//Trigger Type of Tx DMA,SINGLE level-triggered detection
#define	HwTransCtrl_STTRG_SE		~Hw21					//Trigger Type of Rx DMA,SINGLE edge-triggered  detection
#define	HwTransCtrl_STTRG_SL		Hw21					//Trigger Type of Rx DMA,SINGLE level-triggered detection
#define	HwTransCtrl_DTTRG_SE		~Hw20					//Trigger Type of Tx DMA,SINGLE edge-triggered  detection
#define	HwTransCtrl_DTTRG_SL		Hw20					//Trigger Type of Tx DMA,SINGLE level-triggered detection

#define	HwTransCtrl_CRRPT_DIS		~Hw19					//Repeat Mode Control of CDIF Rx DMA,After all of hop transfer has executed, the DMA channel is disabled
#define	HwTransCtrl_CRRPT_EN		Hw19					//Repeat Mode Control of CDIF Rx DMA,The DMA channel remains enabled. When another DMA request hasoccurred, the DMA channel start transfer data again with the samemanner (type, address, increment, mask) as the latest transfer of thatchannel.	
#define	HwTransCtrl_DRRPT_DIS		~Hw18					//Repeat Mode Control of DAI Rx DMA,After all of hop transfer has executed, the DMA channel is disabled
#define	HwTransCtrl_DRRPT_EN		Hw18					//Repeat Mode Control of DAI Rx DMA,The DMA channel remains enabled. When another DMA request hasoccurred, the DMA channel start transfer data again with the samemanner (type, address, increment, mask) as the latest transfer of thatchannel.	
#define	HwTransCtrl_STRPT_DIS		~Hw17					//Repeat Mode Control of SPDIF Tx DMA,After all of hop transfer has executed, the DMA channel is disabled
#define	HwTransCtrl_STRPT_EN		Hw17					//Repeat Mode Control of SPDIF Tx DMA,The DMA channel remains enabled. When another DMA request hasoccurred, the DMA channel start transfer data again with the samemanner (type, address, increment, mask) as the latest transfer of thatchannel.		
#define	HwTransCtrl_DTRPT_DIS		~Hw16					//Repeat Mode Control of DAI Tx DMA,After all of hop transfer has executed, the DMA channel is disabled
#define	HwTransCtrl_DTRPT_EN		Hw16					//Repeat Mode Control of DAI Tx DMA,The DMA channel remains enabled. When another DMA request hasoccurred, the DMA channel start transfer data again with the samemanner (type, address, increment, mask) as the latest transfer of thatchannel.	

#define	HwTransCtrl_CRBSIZE_1		~(Hw14|Hw15)			//Burst Size of CDIF Rx DMA,1 Burst transfer consists of 1 read or write cycle.
#define	HwTransCtrl_CRBSIZE_2		Hw14					//Burst Size of CDIF Rx DMA,1 Burst transfer consists of 2 read or write cycles
#define	HwTransCtrl_CRBSIZE_4		Hw15					//Burst Size of CDIF Rx DMA,1 Burst transfer consists of 4 read or write cycles
#define	HwTransCtrl_CRBSIZE_8		(Hw14|Hw15)				//Burst Size of CDIF Rx DMA,1 Burst transfer consists of 8 read or write cycles

#define	HwTransCtrl_DRBSIZE_1		~(Hw12|Hw13)			//Burst Size of DAI Rx DMA,1 Burst transfer consists of 1 read or write cycle.
#define	HwTransCtrl_DRBSIZE_2		Hw12					//Burst Size of DAI Rx DMA,1 Burst transfer consists of 2 read or write cycles
#define	HwTransCtrl_DRBSIZE_4		Hw13					//Burst Size of DAI Rx DMA,1 Burst transfer consists of 4 read or write cycles
#define	HwTransCtrl_DRBSIZE_8		(Hw12|Hw13)				//Burst Size of DAI Rx DMA,1 Burst transfer consists of 8 read or write cycles

#define	HwTransCtrl_STBSIZE_1		~(Hw10|Hw11)			//Burst Size of SPDIF Tx DMA,1 Burst transfer consists of 1 read or write cycle.
#define	HwTransCtrl_STBSIZE_2		Hw10					//Burst Size of SPDIF Tx DMA,1 Burst transfer consists of 2 read or write cycles
#define	HwTransCtrl_STBSIZE_4		Hw11					//Burst Size of SPDIF Tx DMA,1 Burst transfer consists of 4 read or write cycles
#define	HwTransCtrl_STBSIZE_8		(Hw10|Hw11)				//Burst Size of SPDIF Tx DMA,1 Burst transfer consists of 8 read or write cycles

#define	HwTransCtrl_DTBSIZE_1		~(Hw8|Hw9)				//Burst Size of DAI Tx DMA,1 Burst transfer consists of 1 read or write cycle.
#define	HwTransCtrl_DTBSIZE_2		Hw8						//Burst Size of DAI Tx DMA,1 Burst transfer consists of 2 read or write cycles
#define	HwTransCtrl_DTBSIZE_4		Hw9						//Burst Size of DAI Tx DMA,1 Burst transfer consists of 4 read or write cycles
#define	HwTransCtrl_DTBSIZE_8		(Hw8|Hw9)				//Burst Size of DAI Tx DMA,1 Burst transfer consists of 8 read or write cycles

#define	HwTransCtrl_CRWSIZE_8			~(Hw6|Hw7)				//Word Size of CDIF Rx DMA,Each cycle read or write 8bit data
#define	HwTransCtrl_CRWSIZE_16		Hw6						//Word Size of CDIF Rx DMA,Each cycle read or write 16bit data
#define	HwTransCtrl_CRWSIZE_32		(Hw6|Hw7)				//Word Size of CDIF Rx DMA,Each cycle read or write 32bit data

#define	HwTransCtrl_DRWSIZE_8			~(Hw4|Hw5)				//Word Size of DAI Rx DMA,Each cycle read or write 8bit data
#define	HwTransCtrl_DRWSIZE_16		Hw4						//Word Size of DAI Rx DMA,Each cycle read or write 16bit data
#define	HwTransCtrl_DRWSIZE_32		(Hw4|Hw5)				//Word Size of DAI Rx DMA,Each cycle read or write 32bit data

#define	HwTransCtrl_STWSIZE_8			~(Hw2|Hw3)				//Word Size of SPDIF TxDMA,Each cycle read or write 8bit data
#define	HwTransCtrl_STWSIZE_16		Hw2						//Word Size of SPDIF Tx DMA,Each cycle read or write 16bit data
#define	HwTransCtrl_STWSIZE_32		(Hw2|Hw3)				//Word Size of SPDIF Tx DMA,Each cycle read or write 32bit data

#define	HwTransCtrl_DTWSIZE_8			~(Hw0|Hw1)				//Word Size of DAI Txx DMA,Each cycle read or write 8bit data
#define	HwTransCtrl_DTWSIZE_16		Hw0						//Word Size of DAI Tx DMA,Each cycle read or write 16bit data
#define	HwTransCtrl_DTWSIZE_32		(Hw0|Hw1)				//Word Size of DAI Tx DMA,Each cycle read or write 32bit data	



#define	HwRptCtrl				*(volatile unsigned long *)0xF100203C		// R/W, DMA Repeat Control Register

#define	HwRptCtrl_DRI_DIS				~Hw31				//Disable Repeat Interrupt,DMA Interrupt is occurred when the end of each Repeated DMAoperation.
#define	HwRptCtrl_DRI_EN				Hw31				//Disable Repeat Interrupt,DMA Interrupt occur is occurred when the last DMA Repeated DMA



#define	HwChCtrl				*(volatile unsigned long *)0xF1002078		// R/W, DMA Channel Control Register

#define 	HwChCtrl_CREN_DIS				~Hw31				//DMA Channel Enable of CDIF Rx,DMA channel is terminated and disabled.
#define 	HwChCtrl_CREN_EN				Hw31				//DMA Channel Enable of CDIF Rx,DMA channel is enabled.
#define 	HwChCtrl_DREN_DIS				~Hw30				//DMA Channel Enable of DAI Rx,DMA channel is terminated and disabled.
#define 	HwChCtrl_DREN_EN				Hw30				//DMA Channel Enable of DAI Rx,DMA channel is enabled.			
#define 	HwChCtrl_STEN_DIS				~Hw29				//DMA Channel Enable of SPDIF Tx,DMA channel is terminated and disabled.
#define 	HwChCtrl_STEN_EN				Hw29				//DMA Channel Enable of SPDIF Tx,DMA channel is enabled.
#define 	HwChCtrl_DTEN_DIS				~Hw28				//DMA Channel Enable of DAI Tx,DMA channel is terminated and disabled.	
#define 	HwChCtrl_DTEN_EN				Hw28				//DMA Channel Enable of DAI Tx,DMA channel is enabled.	

#define 	HwChCtrl_CRLR_DIS				~Hw19				//Left/Right Data Mode of CDIF Rx,Disable LRMode.
#define 	HwChCtrl_CRLR_EN				Hw19				//Left/Right Data Mode of CDIF Rx,Enable LRMode.
#define 	HwChCtrl_DRLR_DIS				~Hw18				//Left/Right Data Mode of DAI Rx,Disable LRMode.
#define 	HwChCtrl_DRLR_EN				Hw18				//Left/Right Data Mode of DAI Rx,Enable LRMode.
#define 	HwChCtrl_STLR_DIS				~Hw17				//Left/Right Data Mode of SPDIF Tx,Disable LRMode.
#define 	HwChCtrl_STLR_EN				Hw17				//Left/Right Data Mode of SPDIF Tx,Enable LRMode.
#define 	HwChCtrl_DTLR_DIS				~Hw16				//Left/Right Data Mode of DAI Tx,Disable LRMode.	
#define 	HwChCtrl_DTLR_EN				Hw16				//Left/Right Data Mode of DAI Tx,Enable LRMode.

#define 	HwChCtrl_CRDW_24				~Hw15				//Width of Audio Data of CDIF Rx,Assume width of audio data is 24bits.	
#define 	HwChCtrl_CRDW_16				Hw15				//Width of Audio Data of CDIF Rx,Assume width of audio data is 16bits.	
#define 	HwChCtrl_DRDW_24				~Hw14				//Width of Audio Data of DAI Rx,Assume width of audio data is 24bits.	
#define 	HwChCtrl_DRDW_16				Hw14				//Width of Audio Data of DAI Rx,Assume width of audio data is 16bits.
#define 	HwChCtrl_STDW_24				~Hw13				//Width of Audio Data of SPDIF Tx,Assume width of audio data is 24bits.	
#define 	HwChCtrl_STDW_16				Hw13				//Width of Audio Data of SPDIF Tx,Assume width of audio data is 16bits.
#define 	HwChCtrl_DTDW_24				~Hw12				//Width of Audio Data of DAI Tx,Assume width of audio data is 24bits.	
#define 	HwChCtrl_DTDW_16				Hw12				//Width of Audio Data of DAI Tx,Assume width of audio data is 16bits.

#define 	HwChCtrl_CRSEN_DIS				~Hw11				//Swapping Half-word/Byte align of CDIF Rx,Disable swapping,Disable swapping
#define 	HwChCtrl_CRSEN_EN				Hw11				//Swapping Half-word/Byte align of CDIF Rx,Disable swapping,Swaps half-word or byte align according to WB bits
#define 	HwChCtrl_DRSEN_DIS				~Hw10				//Swapping Half-word/Byte align of DAI Rx,Disable swapping,Disable swapping
#define 	HwChCtrl_DRSEN_EN				Hw10				//Swapping Half-word/Byte align of DAI Rx,Disable swapping,Swaps half-word or byte align according to WB bits
#define 	HwChCtrl_STSEN_DIS				~Hw9				//Swapping Half-word/Byte align of SPDIF Tx,Disable swapping,Disable swapping	
#define 	HwChCtrl_STSEN_EN				Hw9					//Swapping Half-word/Byte align of SPDIF Tx,Disable swapping,Swaps half-word or byte align according to WB bits
#define 	HwChCtrl_DTSEN_DIS				~Hw8				//Swapping Half-word/Byte align of DAI Tx,Disable swapping,Disable swapping	
#define 	HwChCtrl_DTSEN_EN				Hw8					//Swapping Half-word/Byte align of DAI Tx,Disable swapping,Swaps half-word or byte align according to WB bits

#define 	HwChCtrl_CRWB_DIS				~Hw7				//Choose Half-word/Byte of CDIF Rx,Byte swapping. This is available only when SWP is enabled.	
#define 	HwChCtrl_CRWB_EN				Hw7					//Choose Half-word/Byte of CDIF Rx,Half-word swapping. This is available only when SWP is enabled.
#define 	HwChCtrl_DRWB_DIS				~Hw6				//Choose Half-word/Byte of DAI Rx,Byte swapping. This is available only when SWP is enabled.		
#define 	HwChCtrl_DRWB_EN				Hw6					//Choose Half-word/Byte of DAI Rx,Half-word swapping. This is available only when SWP is enabled.
#define 	HwChCtrl_STWB_DIS				~Hw5				//Choose Half-word/Byte of SPDIF Tx,Byte swapping. This is available only when SWP is enabled.		
#define 	HwChCtrl_STWB_EN				Hw5					//Choose Half-word/Byte of SPDIF Tx,Half-word swapping. This is available only when SWP is enabled.
#define 	HwChCtrl_DTWB_DIS				~Hw4				//Choose Half-word/Byte of DAI Tx,Byte swapping. This is available only when SWP is enabled.		
#define 	HwChCtrl_DTWB_EN				Hw4					//Choose Half-word/Byte of DAI Tx,Half-word swapping. This is available only when SWP is enabled.

#define 	HwChCtrl_CRIEN				Hw3					//Interrupt Enable of CDIF Rx,At the same time the FLAG goes to 1, DMA interrupt requestis generated.	
#define 	HwChCtrl_DRIEN				Hw2					//Interrupt Enable of DAI Rx,At the same time the FLAG goes to 1, DMA interrupt requestis generated.
#define 	HwChCtrl_STIEN				Hw1					//Interrupt Enable of SPDIF Tx,At the same time the FLAG goes to 1, DMA interrupt requestis generated.
#define 	HwChCtrl_DTIEN				Hw0					//Interrupt Enable of DAI Tx,At the same time the FLAG goes to 1, DMA interrupt requestis generated.



#define	HwIntStatus				*(volatile unsigned long *)0xF100207C		// R/W, DMA Interrupt Status Register

#define	HwIntStatus_CRI_DIS				~Hw7			//DMA Interrupt Status of CDIF Rx,No interrupt occurred while CDIF Tx DMA transfer.	
#define	HwIntStatus_CRI_EN					Hw7				//DMA Interrupt Status of CDIF Rx,Interrupt occurred while CDIF Tx DMA transfer.
#define	HwIntStatus_DRI_DIS				~Hw6			//DMA Interrupt Status of DAI Rx,No interrupt occurred while DAI Tx DMA transfer.	
#define	HwIntStatus_DRI_EN					Hw6				//DMA Interrupt Status of DAI Rx,Interrupt occurred while DAI Tx DMA transfer.
#define	HwIntStatus_STI_DIS				~Hw5			//DMA Interrupt Status of SPDIF Tx,No interrupt occurred while SPDIF Tx DMA transfer.	
#define	HwIntStatus_STI_EN					Hw5				//DMA Interrupt Status of SPDIF Tx,Interrupt occurred while SPDIF Tx DMA transfer.
#define	HwIntStatus_DTI_DIS				~Hw4			//DMA Interrupt Status of DAI Tx,No interrupt occurred while DAI Tx DMA transfer.	
#define	HwIntStatus_DTI_EN					Hw4				//DMA Interrupt Status of DAI Tx,Interrupt occurred while DAI Tx DMA transfer.
#define	HwIntStatus_CRMI_DIS				~Hw3			//DMA Masked Interrupt Status of CDIF Rx,No interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_CRMI_EN				Hw3				//DMA Masked Interrupt Status of CDIF Rx,Interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_DRMI_DIS				~Hw2			//DMA Masked Interrupt Status of DAI Rx,No interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_DRMI_EN				Hw2				//DMA Masked Interrupt Status of DAI Rx,No interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_STMI_DIS				~Hw1			//DMA Masked Interrupt Status of SPDIF Tx,No interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_STMI_EN				Hw1				//DMA Masked Interrupt Status of SPDIF Tx,No interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_DTMI_DIS				~Hw0			//DMA Masked Interrupt Status of DAI Tx,No interrupt occurred while CDIF Rx DMA transfer.
#define	HwIntStatus_DTMI_EN				Hw0				//DMA Masked Interrupt Status of DAI Tx,No interrupt occurred while CDIF Rx DMA transfer.





//Done...
/////////

/************************************************************************
*	Interrupt controller Register Define		(Base Addr = 0xF0001000)
************************************************************************/
//////////
//Done...

typedef volatile struct {
	unsigned	IEN;			// (0x00) Enable Interrupt
	unsigned	ICLR;			// (0x04) Clear Interrupt
	unsigned	ISTS;			// (0x08) Interrupt Status
	unsigned	ISEL;			// (0x0C) IRQ/FIQ selection
	unsigned	ICFG;			// (0x10) Test interrupt selection
	unsigned	MSTS;			// (0x14) Masked Interrupt Status
	unsigned	TREQ;			// (0x18) Test interrupt request
	unsigned	dummy1C;           // (0x1C) 
	unsigned	IRQ;			// (0x20) IRQ interrupt status
	unsigned	FIQ;			// (0x24) FIQ interrupt status
	unsigned	MIRQ;			// (0x28) Masked IRQ interrupt status
	unsigned	MFIQ;			// (0x2C) Masked FIQ interrupt status
	unsigned	MODE;			// (0x30) Trigger mode selection (0 = edge, 1 = level)
	unsigned	SYNC;			// (0x34) Enable Synchronizing
	unsigned	WKEN;			// (0x38) Select Wakeup signal
	unsigned	SDCFG;                // (0x3C)
	unsigned	POL;                  	// (0x40)
	unsigned	dummy44;           // (0x44)
	unsigned	dummy48;           // (0x48)
	unsigned	dummy4C;           // (0x4C)
	
	unsigned	IENB;			// (0x50) Enable Interrupt
	unsigned	ICLRB;			// (0x54) Clear Interrupt
	unsigned	ISTSB;			// (0x58) Interrupt Status
	unsigned	ISELB;			// (0x5C) IRQ/FIQ selection
	unsigned	dummy60;           // (0x60)
	unsigned	MSTSB;			// (0x64) Masked Interrupt Status
	unsigned	TREQB;			// (0x68) Test interrupt request
	unsigned	dummy6C;           // (0x6C)
	unsigned	IRQB;			// (0x70) IRQ interrupt status
	unsigned	FIQB;			// (0x74) FIQ interrupt status
	unsigned	MIRQB;			// (0x78) Masked IRQ interrupt status
	unsigned	MFIQB;			// (0x7C) Masked FIQ interrupt status
	unsigned	MODEB;			// (0x80) Trigger mode selection (0 = edge, 1 = level)
	unsigned	SYNCB;			// (0x84) Enable Synchronizing
	unsigned	WKENB;			// (0x88) Select Wakeup signal	
	unsigned	POLB;                  // (0x8C)	
	unsigned	dummy;	                 //
} sHwINT;
	
//IRQA enum list
enum {
	
	IRQ_INTB,		
	IRQ_BDMA,		
	IRQ_ADMA,			
	IRQ_GDMA1,			
	IRQ_DAIRX,			
	IRQ_DAITX,		
	IRQ_TC0,		
	IRQ_UT0,			
	IRQ_UD,	   	
	IRQ_SPDTX,	       
	IRQ_UDMA,	       
	IRQ_SD3,			
	IRQ_CAM,		
	IRQ_DMA,		
	IRQ_TC32,		
	IRQ_LCD	,		
	IRQ_ADC,			
	IRQ_I2C,			
	IRQ_RTC_P,		
	IRQ_RTC	,		
	IRQ_NFC,			
	IRQ_SD,			
	IRQ_HPI_W,		
	IRQ_HPI,		
	//IRQ_USBH,	       
	IRQ_UH,	       
	IRQ_JPEG,	       
	IRQ_G2D,	       	
	IRQ_ECC	,		
	IRQ_SPI_R,		
	IRQ_UT1,			
	IRQ_SC			
	
};

//IRQB enum list
enum {

	IRQ_EI0 = Hw0,	
	IRQ_EI1 = Hw1,	
	IRQ_EI2 = Hw2,	
	IRQ_EI3 = Hw3,
	IRQ_EI4 = Hw4,
	IRQ_EI5 = Hw5,
	IRQ_EI6 = Hw6,
	IRQ_EI7 = Hw7,
	IRQ_UT2 = Hw8,
	IRQ_UT3 = Hw9,
	IRQ_SPI1_T = Hw10,
	IRQ_SPI1_R = Hw11,
	IRQ_SD1D = Hw12,
	IRQ_SPDIF = Hw13,
	IRQ_CDIF = Hw14,
	IRQ_VBON = Hw15,
	IRQ_VBOFF = Hw16
};

	
#define	HwVIC_BASE				*(volatile unsigned long *)0xF1003000

#define	HwIEN					*(volatile unsigned long *)0xF1003000		// (IEN_A)R/W, Interrupt Enable Register

#define 	HwIEN_MEN			Hw31					// MEN interrupt select enable
#define 	HwIEN_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwIEN_UT1			Hw29					// UART1 interrupt control enable
#define	HwIEN_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwIEN_ECC			Hw27					// ECC interrupt control enable
#define	HwIEN_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwIEN_JPEG	        	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwIEN_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwIEN_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwIEN_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwIEN_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwIEN_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwIEN_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwIEN_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwIEN_I2C			Hw17					// I2C interrupt control enable
#define	HwIEN_ADC			Hw16					// ADC interrupt control enable
#define	HwIEN_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwIEN_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwIEN_DMA			Hw13					// General DMA interrupt control enable
#define	HwIEN_CIF			Hw12					// CIF interrupt control Enable
#define	HwIEN_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwIEN_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwIEN_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwIEN_UD	         	Hw8						// USB device interrupt control enable
#define	HwIEN_UT0		 	Hw7						// UART0 interrupt control enable
#define	HwIEN_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwIEN_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwIEN_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwIEN_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwIEN_ADMA		Hw2						// Audio DMA interrupt control
#define	HwIEN_BDMA		Hw1						// Bitsream DMA interrupt control



#define	HwCLR					*(volatile unsigned long *)0xF1003004		//(CREQ_A) W, Clear Interrupt Register.

#define 	HwCLR_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwCLR_UT1			Hw29					// UART1 interrupt control enable
#define	HwCLR_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwCLR_ECC			Hw27					// ECC interrupt control enable
#define	HwCLR_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwCLR_JPEG	        	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwCLR_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwCLR_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwCLR_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwCLR_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwCLR_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwCLR_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwCLR_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwCLR_I2C			Hw17					// I2C interrupt control enable
#define	HwCLR_ADC			Hw16					// ADC interrupt control enable
#define	HwCLR_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwCLR_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwCLR_DMA			Hw13					// General DMA interrupt control enable
#define	HwCLR_CIF			Hw12					// CIF interrupt control Enable
#define	HwCLR_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwCLR_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwCLR_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwCLR_UD	         	Hw8						// USB device interrupt control enable
#define	HwCLR_UT0		 	Hw7						// UART0 interrupt control enable
#define	HwCLR_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwCLR_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwCLR_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwCLR_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwCLR_ADMA		Hw2						// Audio DMA interrupt control
#define	HwCLR_BDMA		Hw1						// Bitsream DMA interrupt control



#define	HwSTS					*(volatile unsigned long *)0xF1003008	// (IREQ_A)R, Interrupt Request Register

#define 	HwSTS_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwSTS_UT1			Hw29					// UART1 interrupt control enable
#define	HwSTS_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwSTS_ECC			Hw27					// ECC interrupt control enable
#define	HwSTS_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwSTS_JPEG	        	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwSTS_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwSTS_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwSTS_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwSTS_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwSTS_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwSTS_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwSTS_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwSTS_I2C			Hw17					// I2C interrupt control enable
#define	HwSTS_ADC			Hw16					// ADC interrupt control enable
#define	HwSTS_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwSTS_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwSTS_DMA			Hw13					// General DMA interrupt control enable
#define	HwSTS_CIF			Hw12					// CIF interrupt control Enable
#define	HwSTS_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwSTS_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwSTS_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwSTS_UD	         	Hw8						// USB device interrupt control enable
#define	HwSTS_UT0		 	Hw7						// UART0 interrupt control enable
#define	HwSTS_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwSTS_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwSTS_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwSTS_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwSTS_ADMA		Hw2						// Audio DMA interrupt control
#define	HwSTS_BDMA		Hw1						// Bitsream DMA interrupt control
#define	HwSTS_INTB			Hw0						// value of IREQ_B register OR. 



#define	HwSEL					*(volatile unsigned long *)0xF100300C		// (IRQSEL_A)R/W, IRQ/FIQ Select Register

#define 	HwSEL_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwSEL_UT1			Hw29					// UART1 interrupt control enable
#define	HwSEL_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwSEL_ECC			Hw27					// ECC interrupt control enable
#define	HwSEL_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwSEL_JPEG	        	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwSEL_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwSEL_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwSEL_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwSEL_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwSEL_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwSEL_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwSEL_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwSEL_I2C			Hw17					// I2C interrupt control enable
#define	HwSEL_ADC			Hw16					// ADC interrupt control enable
#define	HwSEL_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwSEL_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwSEL_DMA			Hw13					// General DMA interrupt control enable
#define	HwSEL_CIF			Hw12					// CIF interrupt control Enable
#define	HwSEL_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwSEL_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwSEL_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwSEL_UD	         	Hw8						// USB device interrupt control enable
#define	HwSEL_UT0		 	Hw7						// UART0 interrupt control enable
#define	HwSEL_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwSEL_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwSEL_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwSEL_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwSEL_ADMA		Hw2						// Audio DMA interrupt control
#define	HwSEL_BDMA		Hw1						// Bitsream DMA interrupt control
				

#define	HwICFG						*(volatile unsigned long *)0xF1003010		// R/W, External Interrupt Configuration Register

#define	HwICFG_FE7_DIS		Hw31					// Noise filter disable
#define	HwICFG_FE7_EN			~Hw31					// Noise filter enable
#define	HwICFG_DTYP7_FEdge	~(Hw30|Hw29)			// Falling edge triggered external interrupt
#define	HwICFG_DTYP7_REdge	Hw29					// Rising edge triggered external interrupt
#define	HwICFG_DTYP7_BEdge	Hw30					// Both edge triggered external interrupt
#define	HwICFG_DTYP7_HLLevel	(Hw30|Hw29)				// Level high/low triggered external interrupt
#define	HwICFG_FT7				Hw28					// Reserved
#define	HwICFG_FE6_DIS		Hw27					// Noise filter disable
#define	HwICFG_FE6_EN			~Hw27					// Noise filter enable
#define	HwICFG_DTYP6_FEdge	(Hw26|Hw25)				// Falling edge triggered external interrupt
#define	HwICFG_DTYP6_REdge	Hw25					// Rising edge triggered external interrupt
#define	HwICFG_DTYP6_BEdge	Hw26					// Both edge triggered external interrupt
#define	HwICFG_DTYP6_HLLevel	(Hw26|Hw25)				// Level high/low triggered external interrupt
#define	HwICFG_FT6				Hw24					// Reserved
#define	HwICFG_FE5_DIS		Hw23					// Noise filter disable
#define	HwICFG_FE5_EN			~Hw23					// Noise filter enable
#define	HwICFG_DTYP5_FEdge	~(Hw22|Hw21)			// Falling edge triggered external interrupt
#define	HwICFG_DTYP5_REdge	Hw21					// Rising edge triggered external interrupt
#define	HwICFG_DTYP5_BEdge	Hw22					// Both edge triggered external interrupt
#define	HwICFG_DTYP5_HLLevel	(Hw22|Hw21)				// Level high/low triggered external interrupt
#define	HwICFG_FT5				Hw20					// Reserved
#define	HwICFG_FE4_DIS		Hw19					// Noise filter disable
#define	HwICFG_FE4_EN			~Hw19					// Noise filter enable
#define	HwICFG_DTYP4_FEdge	~(Hw18|Hw17)			// Falling edge triggered external interrupt
#define	HwICFG_DTYP4_REdge	Hw17					// Rising edge triggered external interrupt
#define	HwICFG_DTYP4_BEdge	Hw18					// Both edge triggered external interrupt
#define	HwICFG_DTYP4_HLLevel	(Hw18|Hw17)				// Level high/low triggered external interrupt
#define	HwICFG_FT4				Hw16					// Reserved
#define	HwICFG_FE3_DIS		Hw15					// Noise filter disable
#define	HwICFG_FE3_EN			~Hw15					// Noise filter enable
#define	HwICFG_DTYP3_FEdge	~(Hw14|Hw13)			// Falling edge triggered external interrupt
#define	HwICFG_DTYP3_REdge	Hw13					// Rising edge triggered external interrupt
#define	HwICFG_DTYP3_BEdge	Hw14					// Both edge triggered external interrupt
#define	HwICFG_DTYP3_HLLevel	(Hw14|Hw13)				// Level high/low triggered external interrupt
#define	HwICFG_FT3				Hw12					// Reserved
#define	HwICFG_FE2_DIS		Hw11					// Noise filter disable
#define	HwICFG_FE2_EN			~Hw11					// Noise filter enable
#define	HwICFG_DTYP2_FEdge	(Hw10|Hw9)				// Falling edge triggered external interrupt
#define	HwICFG_DTYP2_REdge	Hw9						// Rising edge triggered external interrupt
#define	HwICFG_DTYP2_BEdge	Hw10					// Both edge triggered external interrupt
#define	HwICFG_DTYP2_HLLevel	(Hw10|Hw9)				// Level high/low triggered external interrupt
#define	HwICFG_FT2				Hw8						// Reserved
#define	HwICFG_FE1_DIS		Hw7						// Noise filter disable
#define	HwICFG_FE1_EN			~Hw7					// Noise filter enable
#define	HwICFG_DTYP1_FEdge	~(Hw6|Hw5)				// Falling edge triggered external interrupt
#define	HwICFG_DTYP1_REdge	Hw5						// Rising edge triggered external interrupt
#define	HwICFG_DTYP1_BEdge	Hw6						// Both edge triggered external interrupt
#define	HwICFG_DTYP1_HLLevel	(Hw6|Hw5)				// Level high/low triggered external interrupt
#define	HwICFG_FT1				Hw4						// Reserved
#define	HwICFG_FE0_DIS		Hw3						// Noise filter disable
#define	HwICFG_FE0_EN			~Hw3					// Noise filter enable
#define	HwICFG_DTYP0_FEdge	~(Hw2|Hw1)				// Falling edge triggered external interrupt
#define	HwICFG_DTYP0_REdge	Hw1						// Rising edge triggered external interrupt
#define	HwICFG_DTYP0_BEdge	Hw2						// Both edge triggered external interrupt
#define	HwICFG_DTYP0_HLLevel	(Hw2|Hw1)				// Level high/low triggered external interrupt
#define	HwICFG_FT0				Hw0						// Reserved



#define	HwMSTS					*(volatile unsigned long *)0xF1003014	// (MREQ_A)R, Masked Interrupt Request Flag Register

#define HwMSTS_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwMSTS_UT1		Hw29					// UART1 interrupt control enable
#define	HwMSTS_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwMSTS_ECC		Hw27					// ECC interrupt control enable
#define	HwMSTS_G2D	       Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwMSTS_JPEG	       Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwMSTS_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwMSTS_HPI		Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwMSTS_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwMSTS_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwMSTS_NFC		Hw20					// NAND Flash Controller interrupt control enable
#define	HwMSTS_RTC		Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwMSTS_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwMSTS_I2C			Hw17					// I2C interrupt control enable
#define	HwMSTS_ADC		Hw16					// ADC interrupt control enable
#define	HwMSTS_LCD		Hw15					// LCD Controller interrupt control enable
#define	HwMSTS_TC32		Hw14					// 32bit timer interrupt control enable
#define	HwMSTS_DMA		Hw13					// General DMA interrupt control enable
#define	HwMSTS_CIF			Hw12					// CIF interrupt control Enable
#define	HwMSTS_SD3		Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define HwMSTS_UDMA	       Hw10					// USB DMA interrupt control enable
#define HwMSTS_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define HwMSTS_UD	         	Hw8						// USB device interrupt control enable
#define	HwMSTS_UT0		Hw7						// UART0 interrupt control enable
#define	HwMSTS_TC0		Hw6						// Timer/Counter interrupt control enable
#define	HwMSTS_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwMSTS_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwMSTS_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwMSTS_ADMA		Hw2						// Audio DMA interrupt control
#define	HwMSTS_BDMA		Hw1						// Bitsream DMA interrupt control
#define	HwMSTS_INTB		Hw0						// value of IREQ_B register OR. 
	


#define	HwTIG					*(volatile unsigned long *)0xF1003018		// R/W, Test mode Register*/

#define HwTIG_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwTIG_UT1			Hw29					// UART1 interrupt control enable
#define	HwTIG_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwTIG_ECC			Hw27					// ECC interrupt control enable
#define	HwTIG_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwTIG_JPEG	       	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwTIG_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwTIG_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwTIG_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwTIG_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwTIG_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwTIG_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwTIG_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwTIG_I2C			Hw17					// I2C interrupt control enable
#define	HwTIG_ADC			Hw16					// ADC interrupt control enable
#define	HwTIG_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwTIG_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwTIG_DMA			Hw13					// General DMA interrupt control enable
#define	HwTIG_CIF			Hw12					// CIF interrupt control Enable
#define	HwTIG_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define HwTIG_UDMA	       Hw10					// USB DMA interrupt control enable
#define HwTIG_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define HwTIG_UD	         	Hw8						// USB device interrupt control enable
#define	HwTIG_UT0			Hw7						// UART0 interrupt control enable
#define	HwTIG_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwTIG_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwTIG_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwTIG_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwTIG_ADMA		Hw2						// Audio DMA interrupt control
#define	HwTIG_BDMA		Hw1						// Bitsream DMA interrupt control



#define	HwIRQ					*(volatile unsigned long *)0xF1003020		// R, IRQ Raw Status Register

#define HwIRQ_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwIRQ_UT1			Hw29					// UART1 interrupt control enable
#define	HwIRQ_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwIRQ_ECC			Hw27					// ECC interrupt control enable
#define	HwIRQ_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwIRQ_JPEG	       	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwIRQ_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwIRQ_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwIRQ_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwIRQ_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwIRQ_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwIRQ_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwIRQ_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwIRQ_I2C			Hw17					// I2C interrupt control enable
#define	HwIRQ_ADC			Hw16					// ADC interrupt control enable
#define	HwIRQ_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwIRQ_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwIRQ_DMA			Hw13					// General DMA interrupt control enable
#define	HwIRQ_CIF			Hw12					// CIF interrupt control Enable
#define	HwIRQ_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwIRQ_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwIRQ_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwIRQ_UD	         	Hw8						// USB device interrupt control enable
#define	HwIRQ_UT0			Hw7						// UART0 interrupt control enable
#define	HwIRQ_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwIRQ_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwIRQ_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwIRQ_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwIRQ_ADMA		Hw2						// Audio DMA interrupt control
#define	HwIRQ_BDMA		Hw1						// Bitsream DMA interrupt control
#define	HwIRQ_INTB			Hw0						// value of IREQ_B register OR. 



#define	HwFIQ					*(volatile unsigned long *)0xF1003024		// R, FIQ Raw Stataus Register

#define HwFIQ_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwFIQ_UT1			Hw29					// UART1 interrupt control enable
#define	HwFIQ_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwFIQ_ECC			Hw27					// ECC interrupt control enable
#define	HwFIQ_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwFIQ_JPEG	       	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwFIQ_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwFIQ_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwFIQ_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwFIQ_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwFIQ_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwFIQ_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwFIQ_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwFIQ_I2C			Hw17					// I2C interrupt control enable
#define	HwFIQ_ADC			Hw16					// ADC interrupt control enable
#define	HwFIQ_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwFIQ_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwFIQ_DMA			Hw13					// General DMA interrupt control enable
#define	HwFIQ_CIF			Hw12					// CIF interrupt control Enable
#define	HwFIQ_SD3			Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define HwFIQ_UDMA	       Hw10					// USB DMA interrupt control enable
#define HwFIQ_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define HwFIQ_UD	         	Hw8						// USB device interrupt control enable
#define	HwFIQ_UT0			Hw7						// UART0 interrupt control enable
#define	HwFIQ_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwFIQ_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwFIQ_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwFIQ_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwFIQ_ADMA		Hw2						// Audio DMA interrupt control
#define	HwFIQ_BDMA		Hw1						// Bitsream DMA interrupt control
#define	HwFIQ_INTB			Hw0						// value of IREQ_B register OR. 		             



#define	HwMIRQ					*(volatile unsigned long *)0xF1003028		// R, Masked IRQ Status Register

#define HwMIRQ_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwMIRQ_UT1		Hw29					// UART1 interrupt control enable
#define	HwMIRQ_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwMIRQ_ECC		Hw27					// ECC interrupt control enable
#define	HwMIRQ_G2D	       Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwMIRQ_JPEG	       Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwMIRQ_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwMIRQ_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwMIRQ_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwMIRQ_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwMIRQ_NFC		Hw20					// NAND Flash Controller interrupt control enable
#define	HwMIRQ_RTC		Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwMIRQ_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwMIRQ_I2C			Hw17					// I2C interrupt control enable
#define	HwMIRQ_ADC		Hw16					// ADC interrupt control enable
#define	HwMIRQ_LCD		Hw15					// LCD Controller interrupt control enable
#define	HwMIRQ_TC32		Hw14					// 32bit timer interrupt control enable
#define	HwMIRQ_DMA		Hw13					// General DMA interrupt control enable
#define	HwMIRQ_CIF			Hw12					// CIF interrupt control Enable
#define	HwMIRQ_SD3		Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define HwMIRQ_UDMA	       Hw10					// USB DMA interrupt control enable
#define HwMIRQ_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define HwMIRQ_UD	         	Hw8						// USB device interrupt control enable
#define	HwMIRQ_UT0		Hw7						// UART0 interrupt control enable
#define	HwMIRQ_TC0		Hw6						// Timer/Counter interrupt control enable
#define	HwMIRQ_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwMIRQ_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwMIRQ_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwMIRQ_ADMA		Hw2						// Audio DMA interrupt control
#define	HwMIRQ_BDMA		Hw1						// Bitsream DMA interrupt control
#define	HwMIRQ_INTB		Hw0						// value of IREQ_B register OR. 



#define	HwMFIQ					*(volatile unsigned long *)0xF100302C		// R, Masked FIQ Status Register

#define HwMFIQ_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwMFIQ_UT1		Hw29					// UART1 interrupt control enable
#define	HwMFIQ_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwMFIQ_ECC		Hw27					// ECC interrupt control enable
#define	HwMFIQ_G2D	       Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwMFIQ_JPEG	       Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwMFIQ_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwMFIQ_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwMFIQ_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwMFIQ_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwMFIQ_NFC		Hw20					// NAND Flash Controller interrupt control enable
#define	HwMFIQ_RTC		Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwMFIQ_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwMFIQ_I2C			Hw17					// I2C interrupt control enable
#define	HwMFIQ_ADC		Hw16					// ADC interrupt control enable
#define	HwMFIQ_LCD		Hw15					// LCD Controller interrupt control enable
#define	HwMFIQ_TC32		Hw14					// 32bit timer interrupt control enable
#define	HwMFIQ_DMA		Hw13					// General DMA interrupt control enable
#define	HwMFIQ_CIF			Hw12					// CIF interrupt control Enable
#define	HwMFIQ_SD3		Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwMFIQ_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwMFIQ_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwMFIQ_UD	         	Hw8						// USB device interrupt control enable
#define	HwMFIQ_UT0		Hw7						// UART0 interrupt control enable
#define	HwMFIQ_TC0		Hw6						// Timer/Counter interrupt control enable
#define	HwMFIQ_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwMFIQ_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwMFIQ_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwMFIQ_ADMA		Hw2						// Audio DMA interrupt control
#define	HwMFIQ_BDMA		Hw1						// Bitsream DMA interrupt control
#define	HwMFIQ_INTB		Hw0						// value of IREQ_B register OR.             
													


#define	HwMODE					*(volatile unsigned long *)0xF1003030	// R/W, Trigger Mode Register 0:edge, 1:level

#define HwMODE_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwMODE_UT1		Hw29					// UART1 interrupt control enable
#define	HwMODE_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwMODE_ECC		Hw27					// ECC interrupt control enable
#define	HwMODE_G2D	       Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwMODE_JPEG	       Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwMODE_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwMODE_HPI		Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwMODE_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwMODE_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwMODE_NFC		Hw20					// NAND Flash Controller interrupt control enable
#define	HwMODE_RTC		Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwMODE_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwMODE_I2C		Hw17					// I2C interrupt control enable
#define	HwMODE_ADC		Hw16					// ADC interrupt control enable
#define	HwMODE_LCD		Hw15					// LCD Controller interrupt control enable
#define	HwMODE_TC32		Hw14					// 32bit timer interrupt control enable
#define	HwMODE_DMA		Hw13					// General DMA interrupt control enable
#define	HwMODE_CIF		Hw12					// CIF interrupt control Enable
#define	HwMODE_SD3		Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwMODE_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwMODE_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwMODE_UD	         	Hw8						// USB device interrupt control enable
#define	HwMODE_UT0		Hw7						// UART0 interrupt control enable
#define	HwMODE_TC0		Hw6						// Timer/Counter interrupt control enable
#define	HwMODE_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwMODE_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwMODE_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwMODE_ADMA		Hw2						// Audio DMA interrupt control
#define	HwMODE_BDMA		Hw1						// Bitsream DMA interrupt control



#define	HwSYNC					*(volatile unsigned long *)0xF1003034		// R/W, Synchronizer Control Register

#define 	HwSYNC_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwSYNC_UT1		Hw29					// UART1 interrupt control enable
#define	HwSYNC_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwSYNC_ECC		Hw27					// ECC interrupt control enable
#define	HwSYNC_G2D	       Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwSYNC_JPEG	       Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwSYNC_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwSYNC_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwSYNC_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwSYNC_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwSYNC_NFC		Hw20					// NAND Flash Controller interrupt control enable
#define	HwSYNC_RTC		Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwSYNC_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwSYNC_I2C			Hw17					// I2C interrupt control enable
#define	HwSYNC_ADC		Hw16					// ADC interrupt control enable
#define	HwSYNC_LCD		Hw15					// LCD Controller interrupt control enable
#define	HwSYNC_TC32		Hw14					// 32bit timer interrupt control enable
#define	HwSYNC_DMA		Hw13					// General DMA interrupt control enable
#define	HwSYNC_CIF			Hw12					// CIF interrupt control Enable
#define	HwSYNC_SD3		Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwSYNC_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwSYNC_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwSYNC_UD	         	Hw8						// USB device interrupt control enable
#define	HwSYNC_UT0		Hw7						// UART0 interrupt control enable
#define	HwSYNC_TC0		Hw6						// Timer/Counter interrupt control enable
#define	HwSYNC_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwSYNC_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwSYNC_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwSYNC_ADMA		Hw2						// Audio DMA interrupt control
#define	HwSYNC_BDMA		Hw1						// Bitsream DMA interrupt control



#define	HwWKEN				*(volatile unsigned long *)0xF1003038	// R/W, Wakeup Control Register

#define 	HwWKEN_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwWKEN_UT1		Hw29					// UART1 interrupt control enable
#define	HwWKEN_SPI_R		Hw28					// SPI Rx interrupt control enable
#define	HwWKEN_ECC		Hw27					// ECC interrupt control enable
#define	HwWKEN_G2D	       Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwWKEN_JPEG	       Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwWKEN_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwWKEN_HPI		Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwWKEN_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwWKEN_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwWKEN_NFC		Hw20					// NAND Flash Controller interrupt control enable
#define	HwWKEN_RTC		Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwWKEN_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwWKEN_I2C		Hw17					// I2C interrupt control enable
#define	HwWKEN_ADC		Hw16					// ADC interrupt control enable
#define	HwWKEN_LCD		Hw15					// LCD Controller interrupt control enable
#define	HwWKEN_TC32		Hw14					// 32bit timer interrupt control enable
#define	HwWKEN_DMA		Hw13					// General DMA interrupt control enable
#define	HwWKEN_CIF		Hw12					// CIF interrupt control Enable
#define	HwWKEN_SD3		Hw11					// Sequre Digital/MMC Detect interrupt control enable
#define 	HwWKEN_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwWKEN_SPI_T	       Hw9						// SPI Tx interrupt control enable
#define 	HwWKEN_UD	         	Hw8						// USB device interrupt control enable
#define	HwWKEN_UT0		Hw7						// UART0 interrupt control enable
#define	HwWKEN_TC0		Hw6						// Timer/Counter interrupt control enable
#define	HwWKEN_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwWKEN_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwWKEN_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwWKEN_ADMA		Hw2						// Audio DMA interrupt control
#define	HwWKEN_BDMA		Hw1						// Bitsream DMA interrupt control

#define	HwSDDCFG				*(volatile unsigned long *)0xF100303C	// R/W, SD Detect Configuration Register

#define	HwPOL					*(volatile unsigned long *)0xF1003040	// R/W, SD Detect Configuration Register

#define 	HwPOL_SC			Hw30					// Memory to Memory Scalor Enable
#define	HwPOL_UT1			Hw29					// UART1 interrupt control enable
#define	HwPOL_SPI_R		Hw28					// SPI0 Rx interrupt control enable
#define	HwPOL_ECC			Hw27					// ECC interrupt control enable
#define	HwPOL_G2D	       	Hw26					// Graphic 2D Interrupt Control Enalbe
#define	HwPOL_JPEG	       	Hw25					// Jpeg Codec Interrupt Control Enalbe
#define	HwPOL_USBH	       Hw24					// SAD Calculator Interrupt Control Enalbe
#define	HwPOL_HPI			Hw23					// (EHI)External Host Interface interrupt control enable
#define	HwPOL_HPI_W		Hw22					// (EHI)External Host Interface wake-up interrupt control enable
#define	HwPOL_SD			Hw21					// Sequre Digital / MMC Controller interrupt control enable
#define	HwPOL_NFC			Hw20					// NAND Flash Controller interrupt control enable
#define	HwPOL_RTC			Hw19					// Real Time Clock Alarm interrupt control enable
#define	HwPOL_RTC_P		Hw18					// Real Time Clock Power Dowm Wakeup interrupt control enable
#define	HwPOL_I2C			Hw17					// I2C interrupt control enable
#define	HwPOL_ADC			Hw16					// ADC interrupt control enable
#define	HwPOL_LCD			Hw15					// LCD Controller interrupt control enable
#define	HwPOL_TC32			Hw14					// 32bit timer interrupt control enable
#define	HwPOL_DMA			Hw13					// General DMA interrupt control enable
#define	HwPOL_CIF			Hw12					// CIF interrupt control Enable
//#define	HwPOL_SD3		Hw11					// [ Not Avalible ] 
#define 	HwPOL_UDMA	       Hw10					// USB DMA interrupt control enable
#define 	HwPOL_SPI_T	       Hw9						// SPI0 Tx interrupt control enable
#define 	HwPOL_UD	         	Hw8						// USB device interrupt control enable
#define	HwPOL_UT0			Hw7						// UART0 interrupt control enable
#define	HwPOL_TC0			Hw6						// Timer/Counter interrupt control enable
#define	HwPOL_DAITX		Hw5						// I2S Tx interrupt control enable
#define	HwPOL_DAIRX		Hw4						// I2S Rx interrupt control enable
#define	HwPOL_GDMA1		Hw3						// GDMA1 interrupt control
#define	HwPOL_ADMA		Hw2						// Audio DMA interrupt control
#define	HwPOL_BDMA		Hw1						// Bitsream DMA interrupt control




#define	HwIEN_B				*(volatile unsigned long *)0xF1003050		// (IEN_B)R/W, Interrupt Enable Register

#define	HwIEN_VBOFF		Hw16					// VBUS off interrupt control
#define	HwIEN_VBON		Hw15					// VBUS on interrupt control
#define	HwIEN_CDIF			Hw14					// CDIF interrupt control
#define	HwIEN_SPDIF		Hw13					// SPDIF interrupt control
#define	HwIEN_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwIEN_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwIEN_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwIEN_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwIEN_UT2	         	Hw8						// UART2 interrupt control enable
#define	HwIEN_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwIEN_EI6			Hw6						// External interrupt request 6 control enable
#define	HwIEN_EI5			Hw5						// External interrupt request 5 control enable
#define	HwIEN_EI4			Hw4						// External interrupt request 4 control enable
#define	HwIEN_EI3			Hw3						// External interrupt request 3 control enable
#define	HwIEN_EI2			Hw2						// External interrupt request 2 control enable
#define	HwIEN_EI1			Hw1						// External interrupt request 1 control enable
#define	HwIEN_EI0			Hw0						// External interrupt request 0 control enable



#define	HwCLR_B				*(volatile unsigned long *)0xF1003054		//(CREQ_B) W, Clear Interrupt Register.

#define	HwCLR_VBOFF		Hw16					// VBUS off interrupt control
#define	HwCLR_VBON		Hw15					// VBUS on interrupt control
#define	HwCLR_CDIF			Hw14					// CDIF interrupt control
#define	HwCLR_SPDIF		Hw13					// SPDIF interrupt control
#define	HwCLR_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwCLR_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwCLR_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwCLR_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwCLR_UT2	         	Hw8						// UART2 interrupt control enable
#define	HwCLR_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwCLR_EI6			Hw6						// External interrupt request 6 control enable
#define	HwCLR_EI5			Hw5						// External interrupt request 5 control enable
#define	HwCLR_EI4			Hw4						// External interrupt request 4 control enable
#define	HwCLR_EI3			Hw3						// External interrupt request 3 control enable
#define	HwCLR_EI2			Hw2						// External interrupt request 2 control enable
#define	HwCLR_EI1			Hw1						// External interrupt request 1 control enable
#define	HwCLR_EI0			Hw0						// External interrupt request 0 control enable



#define	HwSTS_B				*(volatile unsigned long *)0xF1003058	// (IREQ_B)R, Interrupt Request Register

#define	HwSTS_VBOFF		Hw16					// VBUS off interrupt control
#define	HwSTS_VBON		Hw15					// VBUS on interrupt control
#define	HwSTS_CDIF			Hw14					// CDIF interrupt control
#define	HwSTS_SPDIF		Hw13					// SPDIF interrupt control
#define	HwSTS_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwSTS_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwSTS_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwSTS_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwSTS_UT2	         	Hw8						// UART2 interrupt control enable
#define	HwSTS_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwSTS_EI6			Hw6						// External interrupt request 6 control enable
#define	HwSTS_EI5			Hw5						// External interrupt request 5 control enable
#define	HwSTS_EI4			Hw4						// External interrupt request 4 control enable
#define	HwSTS_EI3			Hw3						// External interrupt request 3 control enable
#define	HwSTS_EI2			Hw2						// External interrupt request 2 control enable
#define	HwSTS_EI1			Hw1						// External interrupt request 1 control enable
#define	HwSTS_EI0			Hw0						// External interrupt request 0 control enable



#define	HwSEL_B				*(volatile unsigned long *)0xF100305C		// (IRQSEL_B)R/W, IRQ/FIQ Select Register

#define	HwSEL_VBOFF		Hw16					// VBUS off interrupt control
#define	HwSEL_VBON		Hw15					// VBUS on interrupt control
#define	HwSEL_CDIF			Hw14					// CDIF interrupt control
#define	HwSEL_SPDIF		Hw13					// SPDIF interrupt control
#define	HwSEL_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwSEL_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwSEL_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwSEL_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwSEL_UT2	         	Hw8						// UART2 interrupt control enable
#define	HwSEL_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwSEL_EI6			Hw6						// External interrupt request 6 control enable
#define	HwSEL_EI5			Hw5						// External interrupt request 5 control enable
#define	HwSEL_EI4			Hw4						// External interrupt request 4 control enable
#define	HwSEL_EI3			Hw3						// External interrupt request 3 control enable
#define	HwSEL_EI2			Hw2						// External interrupt request 2 control enable
#define	HwSEL_EI1			Hw1						// External interrupt request 1 control enable
#define	HwSEL_EI0			Hw0						// External interrupt request 0 control enable
				


#define	HwMSTS_B				*(volatile unsigned long *)0xF1003064	// (MREQ_B)R, Masked Interrupt Request Flag Register

#define	HwMSTS_VBOFF		Hw16					// VBUS off interrupt control
#define	HwMSTS_VBON		Hw15					// VBUS on interrupt control
#define	HwMSTS_CDIF		Hw14					// CDIF interrupt control
#define	HwMSTS_SPDIF		Hw13					// SPDIF interrupt control
#define	HwMSTS_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwMSTS_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwMSTS_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwMSTS_UT3	       Hw9						// UART3 interrupt control enable
#define 	HwMSTS_UT2	       Hw8						// UART2 interrupt control enable
#define	HwMSTS_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwMSTS_EI6			Hw6						// External interrupt request 6 control enable
#define	HwMSTS_EI5			Hw5						// External interrupt request 5 control enable
#define	HwMSTS_EI4			Hw4						// External interrupt request 4 control enable
#define	HwMSTS_EI3			Hw3						// External interrupt request 3 control enable
#define	HwMSTS_EI2			Hw2						// External interrupt request 2 control enable
#define	HwMSTS_EI1			Hw1						// External interrupt request 1 control enable
#define	HwMSTS_EI0			Hw0						// External interrupt request 0 control enable
	


#define	HwTIG_B				*(volatile unsigned long *)0xF1003068		// R/W, Test mode Register*/

#define	HwTIG_VBOFF		Hw16					// VBUS off interrupt control
#define	HwTIG_VBON		Hw15					// VBUS on interrupt control
#define	HwTIG_CDIF			Hw14					// CDIF interrupt control
#define	HwTIG_SPDIF		Hw13					// SPDIF interrupt control
#define	HwTIG_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwTIG_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwTIG_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwTIG_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwTIG_UT2	       	Hw8						// UART2 interrupt control enable
#define	HwTIG_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwTIG_EI6			Hw6						// External interrupt request 6 control enable
#define	HwTIG_EI5			Hw5						// External interrupt request 5 control enable
#define	HwTIG_EI4			Hw4						// External interrupt request 4 control enable
#define	HwTIG_EI3			Hw3						// External interrupt request 3 control enable
#define	HwTIG_EI2			Hw2						// External interrupt request 2 control enable
#define	HwTIG_EI1			Hw1						// External interrupt request 1 control enable
#define	HwTIG_EI0			Hw0						// External interrupt request 0 control enable



#define	HwIRQ_B				*(volatile unsigned long *)0xF1003070		// R, IRQ Raw Status Register

#define	HwIRQ_VBOFF		Hw16					// VBUS off interrupt control
#define	HwIRQ_VBON		Hw15					// VBUS on interrupt control
#define	HwIRQ_CDIF			Hw14					// CDIF interrupt control
#define	HwIRQ_SPDIF		Hw13					// SPDIF interrupt control
#define	HwIRQ_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwIRQ_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwIRQ_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwIRQ_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwIRQ_UT2	       	Hw8						// UART2 interrupt control enable
#define	HwIRQ_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwIRQ_EI6			Hw6						// External interrupt request 6 control enable
#define	HwIRQ_EI5			Hw5						// External interrupt request 5 control enable
#define	HwIRQ_EI4			Hw4						// External interrupt request 4 control enable
#define	HwIRQ_EI3			Hw3						// External interrupt request 3 control enable
#define	HwIRQ_EI2			Hw2						// External interrupt request 2 control enable
#define	HwIRQ_EI1			Hw1						// External interrupt request 1 control enable
#define	HwIRQ_EI0			Hw0						// External interrupt request 0 control enable



#define	HwFIQ_B				*(volatile unsigned long *)0xF1003074		// R, FIQ Raw Stataus Register

#define	HwFIQ_VBOFF		Hw16					// VBUS off interrupt control
#define	HwFIQ_VBON		Hw15					// VBUS on interrupt control
#define	HwFIQ_CDIF			Hw14					// CDIF interrupt control
#define	HwFIQ_SPDIF		Hw13					// SPDIF interrupt control
#define	HwFIQ_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwFIQ_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwFIQ_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwFIQ_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwFIQ_UT2	       	Hw8						// UART2 interrupt control enable
#define	HwFIQ_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwFIQ_EI6			Hw6						// External interrupt request 6 control enable
#define	HwFIQ_EI5			Hw5						// External interrupt request 5 control enable
#define	HwFIQ_EI4			Hw4						// External interrupt request 4 control enable
#define	HwFIQ_EI3			Hw3						// External interrupt request 3 control enable
#define	HwFIQ_EI2			Hw2						// External interrupt request 2 control enable
#define	HwFIQ_EI1			Hw1						// External interrupt request 1 control enable
#define	HwFIQ_EI0			Hw0						// External interrupt request 0 control enable		             



#define	HwMIRQ_B				*(volatile unsigned long *)0xF1003078		// R, Masked IRQ Status Register

#define	HwMIRQ_VBOFF		Hw16					// VBUS off interrupt control
#define	HwMIRQ_VBON		Hw15					// VBUS on interrupt control
#define	HwMIRQ_CDIF		Hw14					// CDIF interrupt control
#define	HwMIRQ_SPDIF		Hw13					// SPDIF interrupt control
#define	HwMIRQ_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwMIRQ_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwMIRQ_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwMIRQ_UT3	       Hw9						// UART3 interrupt control enable
#define 	HwMIRQ_UT2	       Hw8						// UART2 interrupt control enable
#define	HwMIRQ_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwMIRQ_EI6			Hw6						// External interrupt request 6 control enable
#define	HwMIRQ_EI5			Hw5						// External interrupt request 5 control enable
#define	HwMIRQ_EI4			Hw4						// External interrupt request 4 control enable
#define	HwMIRQ_EI3			Hw3						// External interrupt request 3 control enable
#define	HwMIRQ_EI2			Hw2						// External interrupt request 2 control enable
#define	HwMIRQ_EI1			Hw1						// External interrupt request 1 control enable
#define	HwMIRQ_EI0			Hw0						// External interrupt request 0 control enable



#define	HwMFIQ_B				*(volatile unsigned long *)0xF100307C		// R, Masked FIQ Status Register

#define	HwMFIQ_VBOFF		Hw16					// VBUS off interrupt control
#define	HwMFIQ_VBON		Hw15					// VBUS on interrupt control
#define	HwMFIQ_CDIF		Hw14					// CDIF interrupt control
#define	HwMFIQ_SPDIF		Hw13					// SPDIF interrupt control
#define	HwMFIQ_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwMFIQ_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwMFIQ_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwMFIQ_UT3	       Hw9						// UART3 interrupt control enable
#define 	HwMFIQ_UT2	       Hw8						// UART2 interrupt control enable
#define	HwMFIQ_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwMFIQ_EI6			Hw6						// External interrupt request 6 control enable
#define	HwMFIQ_EI5			Hw5						// External interrupt request 5 control enable
#define	HwMFIQ_EI4			Hw4						// External interrupt request 4 control enable
#define	HwMFIQ_EI3			Hw3						// External interrupt request 3 control enable
#define	HwMFIQ_EI2			Hw2						// External interrupt request 2 control enable
#define	HwMFIQ_EI1			Hw1						// External interrupt request 1 control enable
#define	HwMFIQ_EI0			Hw0						// External interrupt request 0 control enable            
													


#define	HwMODE_B				*(volatile unsigned long *)0xF1003080	// R/W, Trigger Mode Register 0:edge, 1:level

#define	HwMODE_VBOFF		Hw16					// VBUS off interrupt control
#define	HwMODE_VBON		Hw15					// VBUS on interrupt control
#define	HwMODE_CDIF		Hw14					// CDIF interrupt control
#define	HwMODE_SPDIF		Hw13					// SPDIF interrupt control
#define	HwMODE_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwMODE_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwMODE_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwMODE_UT3	       Hw9						// UART3 interrupt control enable
#define 	HwMODE_UT2	       Hw8						// UART2 interrupt control enable
#define	HwMODE_EI7		Hw7						// External interrupt request 7 control enable
#define	HwMODE_EI6		Hw6						// External interrupt request 6 control enable
#define	HwMODE_EI5		Hw5						// External interrupt request 5 control enable
#define	HwMODE_EI4		Hw4						// External interrupt request 4 control enable
#define	HwMODE_EI3		Hw3						// External interrupt request 3 control enable
#define	HwMODE_EI2		Hw2						// External interrupt request 2 control enable
#define	HwMODE_EI1		Hw1						// External interrupt request 1 control enable
#define	HwMODE_EI0		Hw0						// External interrupt request 0 control enable       



#define	HwSYNC_B				*(volatile unsigned long *)0xF1003084		// R/W, Synchronizer Control Register

#define	HwSYNC_VBOFF		Hw16					// VBUS off interrupt control
#define	HwSYNC_VBON		Hw15					// VBUS on interrupt control
#define	HwSYNC_CDIF		Hw14					// CDIF interrupt control
#define	HwSYNC_SPDIF		Hw13					// SPDIF interrupt control
#define	HwSYNC_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwSYNC_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwSYNC_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwSYNC_UT3	       Hw9						// UART3 interrupt control enable
#define 	HwSYNC_UT2	       Hw8						// UART2 interrupt control enable
#define	HwSYNC_EI7		 	Hw7						// External interrupt request 7 control enable
#define	HwSYNC_EI6			Hw6						// External interrupt request 6 control enable
#define	HwSYNC_EI5			Hw5						// External interrupt request 5 control enable
#define	HwSYNC_EI4			Hw4						// External interrupt request 4 control enable
#define	HwSYNC_EI3			Hw3						// External interrupt request 3 control enable
#define	HwSYNC_EI2			Hw2						// External interrupt request 2 control enable
#define	HwSYNC_EI1			Hw1						// External interrupt request 1 control enable
#define	HwSYNC_EI0			Hw0						// External interrupt request 0 control enable      



#define	HwWKEN_B				*(volatile unsigned long *)0xF1003088	// R/W, Wakeup Control Register

#define	HwWKEN_VBOFF		Hw16					// VBUS off interrupt control
#define	HwWKEN_VBON		Hw15					// VBUS on interrupt control
#define	HwWKEN_CDIF		Hw14					// CDIF interrupt control
#define	HwWKEN_SPDIF		Hw13					// SPDIF interrupt control
#define	HwWKEN_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwWKEN_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwWKEN_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwWKEN_UT3	       Hw9						// UART3 interrupt control enable
#define 	HwWKEN_UT2	       Hw8						// UART2 interrupt control enable
#define	HwWKEN_EI7		Hw7						// External interrupt request 7 control enable
#define	HwWKEN_EI6		Hw6						// External interrupt request 6 control enable
#define	HwWKEN_EI5		Hw5						// External interrupt request 5 control enable
#define	HwWKEN_EI4		Hw4						// External interrupt request 4 control enable
#define	HwWKEN_EI3		Hw3						// External interrupt request 3 control enable
#define	HwWKEN_EI2		Hw2						// External interrupt request 2 control enable
#define	HwWKEN_EI1		Hw1						// External interrupt request 1 control enable
#define	HwWKEN_EI0		Hw0						// External interrupt request 0 control enable    



#define	HwPOL_B					*(volatile unsigned long *)0xF100308C	// R/W, SD Detect Configuration Register

#define	HwPOL_VBOFF		Hw16					// VBUS off interrupt control
#define	HwPOL_VBON		Hw15					// VBUS on interrupt control
#define	HwPOL_CDIF			Hw14					// CDIF interrupt control
#define	HwPOL_SPDIF		Hw13					// SPDIF interrupt control
#define	HwPOL_SD1D		Hw12					// SD/MMC1 data3 interrupt control
#define	HwPOL_SPI1_R		Hw11					// SPI1 Rx interrupt control enable
#define 	HwPOL_SPI1_T	       Hw10					// SPI1 Tx interrupt control enable
#define 	HwPOL_UT3	       	Hw9						// UART3 interrupt control enable
#define 	HwPOL_UT2	       	Hw8						// UART2 interrupt control enable


//To Do...

//Done...
/////////
												
/***********************************************************************
*	Timer/Counter Register Define				(Base Addr = 0xF1004000)
************************************************************************/
/////////
//Done...
#define	HwTCFG0				*(volatile unsigned long *)0xF1004000	// R/W, Timer Configuration Register

#define	HwTCFG0_STOP_EN	      	Hw9							// Continuous counting mode
#define	HwTCFG0_STOP_DIS	      	~Hw9						// If TCNTn is equal to the TREFn, the TCNTn counter stop to increment.
#define	HwTCFG0_CC_ZERO	      	Hw8							// TCNT0 is cleared to zero
#define	HwTCFG0_CC_NOT	      	~Hw8						// TCNT0 is not cleared
#define	HwTCFG0_POL_F			Hw7							// TCNT0 is incremented at falling edge of the selected counting clock
#define	HwTCFG0_POL_R			~Hw7						// TCNT0 is incremented at rising edge of the selected counting clock

#define	HwTCFG0_IEN_EN		Hw3							// Enable Timer/Counter interrupt
#define	HwTCFG0_IEN_DIS		~Hw3						// Disable Timer/Counter interrupt
#define	HwTCFG0_PWM_EN		Hw2							// Enable PWM mode
#define	HwTCFG0_PWM_DIS		~Hw2						// Disable PWM mode
#define	HwTCFG0_CON_ON		Hw1							// The TCNT0 continues counting from the TREF0
#define	HwTCFG0_CON_OFF		~Hw1						// When the TCNT0 is reached to TREF0, TCNT0 restarts counting from 0 at the next pulse of selected clock source
#define	HwTCFG0_EN_ON			Hw0							// Timer counter is enabled
#define	HwTCFG0_EN_OFF		~Hw0						// Timer counter is disabled

#define	HwTCNT0				*(volatile unsigned long *)0xF1004004	// R/W, Timer Counter Register

#define	HwTREF0  				*(volatile unsigned long *)0xF1004008	// R/W, Timer Reference Register

#define	HwTMREF0 				*(volatile unsigned long *)0xF100400C	// R/W, Timer Middle reference Register

#define	HwTCFG1				*(volatile unsigned long *)0xF1004010	// R/W, Timer Configuration Register

#define	HwTCFG1_STOP_EN	      	Hw9							// Continuous counting mode
#define	HwTCFG1_STOP_DIS	      	~Hw9						// If TCNTn is equal to the TREFn, the TCNTn counter stop to increment.
#define	HwTCFG1_CC_ZERO		Hw8							// TCNT1 is cleared to zero
#define	HwTCFG1_CC_NOT		~Hw8						// TCNT1 is not cleared
#define	HwTCFG1_POL_F			Hw7							// TCNT1 is incremented at falling edge of the selected counting clock
#define	HwTCFG1_POL_R			~Hw7						// TCNT1 is incremented at rising edge of the selected counting clock

#define	HwTCFG1_IEN_EN		Hw3							// Enable Timer/Counter interrupt
#define	HwTCFG1_IEN_DIS		~Hw3						// Disable Timer/Counter interrupt
#define	HwTCFG1_PWM_EN		Hw2							// Enable PWM mode
#define	HwTCFG1_PWM_DIS		~Hw2						// Disable PWM mode
#define	HwTCFG1_CON_ON		Hw1							// The TCNT1 continues counting from the TREF1
#define	HwTCFG1_CON_OFF		~Hw1						// When the TCNT1 is reached to TREF1, TCNT1 restarts counting from 0 at the next pulse of selected clock source
#define	HwTCFG1_EN_ON			Hw0							// Timer counter is enabled
#define	HwTCFG1_EN_OFF		~Hw0						// Timer counter is disabled

#define	HwTCNT1				*(volatile unsigned long *)0xF1004014	// R/W, Timer Counter Register

#define	HwTREF1				*(volatile unsigned long *)0xF1004018	// R/W, Timer Reference Register

#define	HwTMREF1				*(volatile unsigned long *)0xF100401C	 // R/W, Timer Middle reference Register

#define	HwTCFG2				*(volatile unsigned long *)0xF1004020	// R/W, Timer Configuration Register

#define	HwTCFG2_STOP_EN	      	Hw9							// Continuous counting mode
#define	HwTCFG2_STOP_DIS	      	~Hw9						// If TCNTn is equal to the TREFn, the TCNTn counter stop to increment.
#define	HwTCFG2_CC_ZERO		Hw8							// TCNT2 is cleared to zero
#define	HwTCFG2_CC_NOT		~Hw8						// TCNT2 is not cleared
#define	HwTCFG2_POL_F			Hw7							// TCNT2 is incremented at falling edge of the selected counting clock
#define	HwTCFG2_POL_R			~Hw7						// TCNT2 is incremented at rising edge of the selected counting clock

#define	HwTCFG2_IEN_EN		Hw3							// Enable Timer/Counter interrupt
#define	HwTCFG2_IEN_DIS		~Hw3						// Disable Timer/Counter interrupt
#define	HwTCFG2_PWM_EN		Hw2							// Enable PWM mode
#define	HwTCFG2_PWM_DIS		~Hw2						// Disable PWM mode
#define	HwTCFG2_CON_ON		Hw1							// The TCNT2 continues counting from the TREF2
#define	HwTCFG2_CON_OFF		~Hw1						// When the TCNT2 is reached to TREF2, TCNT2 restarts counting from 0 at the next pulse of selected clock source
#define	HwTCFG2_EN_ON			Hw0							// Timer counter is enabled
#define	HwTCFG2_EN_OFF		~Hw0						// Timer counter is disabled

#define	HwTCNT2				*(volatile unsigned long *)0xF1004024	// R/W, Timer Counter Register

#define	HwTREF2				*(volatile unsigned long *)0xF1004028	// R/W, Timer Reference Register

#define	HwTMREF2				*(volatile unsigned long *)0xF100402C	// R/W, Timer Middle reference Register

#define	HwTCFG3				*(volatile unsigned long *)0xF1004030	// R/W, Timer Configuration Register

#define	HwTCFG3_STOP_EN	      	Hw9							// Continuous counting mode
#define	HwTCFG3_STOP_DIS	      	~Hw9						// If TCNTn is equal to the TREFn, the TCNTn counter stop to increment.
#define	HwTCFG3_CC_ZERO		Hw8							// TCNT3 is cleared to zero
#define	HwTCFG3_CC_NOT		~Hw8						// TCNT3 is not cleared
#define	HwTCFG3_POL_F			Hw7							// TCNT3 is incremented at falling edge of the selected counting clock
#define	HwTCFG3_POL_R			~Hw7						// TCNT3 is incremented at rising edge of the selected counting clock

#define	HwTCFG3_IEN_EN		Hw3							// Enable Timer/Counter interrupt
#define	HwTCFG3_IEN_DIS		~Hw3						// Disable Timer/Counter interrupt
#define	HwTCFG3_PWM_EN		Hw2							// Enable PWM mode
#define	HwTCFG3_PWM_DIS		~Hw2						// Disable PWM mode
#define	HwTCFG3_CON_ON		Hw1							// The TCNT3 continues counting from the TREF3
#define	HwTCFG3_CON_OFF		~Hw1						// When the TCNT3 is reached to TREF3, TCNT3 restarts counting from 0 at the next pulse of selected clock source
#define	HwTCFG3_EN_ON			Hw0							// Timer counter is enabled
#define	HwTCFG3_EN_OFF		~Hw0						// Timer counter is disabled

#define	HwTCNT3				*(volatile unsigned long *)0xF1004034	// R/W, Timer Counter Register

#define	HwTREF3				*(volatile unsigned long *)0xF1004038	// R/W, Timer Reference Register

#define	HwTMREF3				*(volatile unsigned long *)0xF100403C 	// R/W, Timer Middle reference Register

#define	HwTCFG4				*(volatile unsigned long *)0xF1004040	// R/W, Timer Configuration Register

#define	HwTCFG4_STOP_EN	      	Hw9							// Continuous counting mode
#define	HwTCFG4_STOP_DIS	      	~Hw9						// If TCNTn is equal to the TREFn, the TCNTn counter stop to increment.
#define	HwTCFG4_CC_ZERO		Hw8							// TCNT4 is cleared to zero
#define	HwTCFG4_CC_NOT		~Hw8						// TCNT4 is not cleared
#define	HwTCFG4_POL_F			Hw7							// TCNT4 is incremented at falling edge of the selected counting clock
#define	HwTCFG4_POL_R			~Hw7						// TCNT4 is incremented at rising edge of the selected counting clock

#define	HwTCFG4_IEN_EN		Hw3							// Enable Timer/Counter interrupt
#define	HwTCFG4_IEN_DIS		~Hw3						// Disable Timer/Counter interrupt
#define	HwTCFG4_PWM_EN		Hw2							// Enable PWM mode
#define	HwTCFG4_PWM_DIS		~Hw2						// Disable PWM mode
#define	HwTCFG4_CON_ON		Hw1							// The TCNT4 continues counting from the TREF4
#define	HwTCFG4_CON_OFF		~Hw1						// When the TCNT4 is reached to TREF4, TCNT4 restarts counting from 0 at the next pulse of selected clock source
#define	HwTCFG4_EN_ON			Hw0							// Timer counter is enabled
#define	HwTCFG4_EN_OFF		~Hw0						// Timer counter is disabled

#define	HwTCNT4				*(volatile unsigned long *)0xF1004044	// R/W, Timer Counter Register

#define	HwTREF4				*(volatile unsigned long *)0xF1004048	// R/W, Timer Reference Register

#define	HwTCFG5				*(volatile unsigned long *)0xF1004050	// R/W, Timer Configuration Register

#define	HwTCFG5_STOP_EN	      	Hw9							// Continuous counting mode
#define	HwTCFG5_STOP_DIS	      	~Hw9						// If TCNTn is equal to the TREFn, the TCNTn counter stop to increment.
#define	HwTCFG5_CC_ZERO		Hw8							// TCNT5 is cleared to zero
#define	HwTCFG5_CC_NOT		~Hw8						// TCNT5 is not cleared
#define	HwTCFG5_POL_F			Hw7							// TCNT5 is incremented at falling edge of the selected counting clock
#define	HwTCFG5_POL_R			~Hw7						// TCNT5 is incremented at rising edge of the selected counting clock

#define	HwTCFG5_IEN_EN		Hw3							// Enable Timer/Counter interrupt
#define	HwTCFG5_IEN_DIS		~Hw3						// Disable Timer/Counter interrupt
#define	HwTCFG5_PWM_EN		Hw2							// Enable PWM mode
#define	HwTCFG5_PWM_DIS		~Hw2						// Disable PWM mode
#define	HwTCFG5_CON_ON		Hw1							// The TCNT5 continues counting from the TREF5
#define	HwTCFG5_CON_OFF		~Hw1						// When the TCNT5 is reached to TREF5, TCNT5 restarts counting from 0 at the next pulse of selected clock source
#define	HwTCFG5_EN_ON			Hw0							// Timer counter is enabled
#define	HwTCFG5_EN_OFF		~Hw0						// Timer counter is disabled

#define	HwTCNT5				*(volatile unsigned long *)0xF1004054	// R/W, Timer Counter Register

#define	HwTREF5				*(volatile unsigned long *)0xF1004058	// R/W, Timer Reference Register

typedef	volatile struct {
	unsigned	TCFG;
	#define	HwTCFG_CC_ZERO		Hw8						// TCNT is cleared to zero
	#define	HwTCFG_POL_F			Hw7						// TCNT is incremented at falling edge of the selected counting clock
	#define	HwTCFG_TCK_DIV2		(0 * Hw4)				// Counter clock is come from TCLK divided by 2.
	#define	HwTCFG_TCK_DIV4		(1 * Hw4)				// Counter clock is come from TCLK divided by 4.
	#define	HwTCFG_TCK_DIV8		(2 * Hw4)				// Counter clock is come from TCLK divided by 8.
	#define	HwTCFG_TCK_DIV16		(3 * Hw4)				// Counter clock is come from TCLK divided by 16.
	#define	HwTCFG_TCK_DIV32		(4 * Hw4)				// Counter clock is come from TCLK divided by 32.
	#define	HwTCFG_TCK_DIV1K		(5 * Hw4)				// Counter clock is come from TCLK divided by 1024.
	#define	HwTCFG_TCK_DIV4K		(6 * Hw4)				// Counter clock is come from TCLK divided by 4096.
	#define	HwTCFG_TCK_EXTCLK		(7 * Hw4)				// Counter clock is come from external clock.
	#define	HwTCFG_IEN_EN			Hw3						// Enable Timer/Counter interrupt
	#define	HwTCFG_PWM_EN		Hw2						// Enable PWM mode
	#define	HwTCFG_CON_ON		Hw1						// The TCNT3 continues counting without return to zero.
	#define	HwTCFG_EN_ON			Hw0						// Timer counter is enabled
	unsigned	TCNT;
	unsigned	TREF;
	unsigned	TMREF;
} sHwTMR;

#define	HwTIREQ				*(volatile unsigned long *)0xF1004060	// R/W, Timer Interrupt Request Register
#define	HwTIREQ_TWF			Hw14						// Watchdog Timer Flag
#define	HwTIREQ_TF5			Hw13						// Timer5 Flag
#define	HwTIREQ_TF4			Hw12						// Timer4 Flag
#define	HwTIREQ_TF3			Hw11						// Timer3 Flag
#define	HwTIREQ_TF2			Hw10						// Timer2 Flag
#define	HwTIREQ_TF1			Hw9							// Timer1 Flag
#define	HwTIREQ_TF0			Hw8							// Timer0 Flag
#define	HwTIREQ_TWI			Hw6							// Watchdog Timer Interrupt Request Flag
#define	HwTIREQ_TI5			Hw5							// Timer5 Interrupt Request Flag
#define	HwTIREQ_TI4			Hw4							// Timer4 Interrupt Request Flag
#define	HwTIREQ_TI3			Hw3							// Timer3 Interrupt Request Flag
#define	HwTIREQ_TI2			Hw2							// Timer2 Interrupt Request Flag
#define	HwTIREQ_TI1			Hw1							// Timer1 Interrupt Request Flag
#define	HwTIREQ_TI0			Hw0							// Timer0 Interrupt Request Flag

#define	HwTWDCFG				*(volatile unsigned long *)0xF1004070		// R/W, Watchdog Timer Configuration Register
#define	HwTWDCFG_TCKSEL2		Hw6							// Watchdog timer duration 2
#define	HwTWDCFG_TCKSEL1		Hw5							// Watchdog timer duration 1
#define	HwTWDCFG_TCKSEL0		Hw4							// Watchdog timer duration 0
#define	HwTWDCFGT_IEN_ON		Hw3							// Watchdog timer interrupt is enabled
#define	HwTWDCFGT_IEN_OFF	~Hw3						// Watchdog timer interrupt is disabled
#define	HwTWDCFG_ISEL_NOGEN	Hw1							// Watchdog timer does not generate reset signal although it reaches to the reference value, and it continue counting from 0.
#define	HwTWDCFG_ISEL_GEN	~Hw1						// Watchdog timer generates the reset signal when it reaches to the reference value, the reset signal is applied to every component in the chip.
#define	HwTWDCFG_EN_ON		Hw0							// Watchdog timer is enabled
#define	HwTWDCFG_EN_OFF		~Hw0						// Watchdog timer is disabled

#define	HwTWDCLR				*(volatile unsigned long *)0xF1004074		// W, Watchdog Timer Clear Register

#define	HwTC32EN				*(volatile unsigned long *)0xF1004080		// R/W, 32bit Counter enable / Pre-scale value
#define	HwTC32EN_LDM1_ON		Hw29						// Reload counter when the counter value matched with CMP1
#define	HwTC32EN_LDM1_OFF	~Hw29						// Reload counter when the counter value matched with CMP1
#define	HwTC32EN_LDM0_ON		Hw28						// Reload counter when the counter value matched with CMP0
#define	HwTC32EN_LDM0_OFF	~Hw28						// Reload counter when the counter value matched with CMP0
#define	HwTC32EN_STPMOD_ON	Hw26						// Stop Mode
#define	HwTC32EN_STPMOD_OFF	~Hw26						// Free running Mode
#define	HwTC32EN_LDZERO_ON	Hw25						// By default, counter starts form LOADVAL(HwTC32LDV)
#define	HwTC32EN_LDZERO_OFF	~Hw25						// By default, counter starts form LOADVAL(HwTC32LDV)
#define	HwTC32EN_ENABLE_ON	Hw24						// Counter enable
#define	HwTC32EN_ENABLE_OFF	~Hw24						// Counter disable

#define	HwTC32LDV				*(volatile unsigned long *)0xF1004084	// R/W, 32bit Counter load value Register

#define	HwTC32CMP0			*(volatile unsigned long *)0xF1004088	// R/W, 32bit Counter match value 0 Register

#define	HwTC32CMP1			*(volatile unsigned long *)0xF100408C	// R/W, 32bit Counter match value 1 Register

#define	HwTC32PCNT			*(volatile unsigned long *)0xF1004090	// R/W, 32bit Counter current value (pre-scale counter) Register

#define	HwTC32MCNT			*(volatile unsigned long *)0xF1004094	// R/W, 32bit Counter current value (main counter) Register

#define	HwTC32IRQ				*(volatile unsigned long *)0xF1004098	// R/W, 32bit Counter interrupt control Register
#define	HwTC32IRQ_IRQCLR_WRITE	Hw31					// Interrupt clear control
#define	HwTC32IRQ_IRQCLR_READ	~Hw31					// Interrupt clear control
#define	HwTC32IRQ_RSYNC_DIS		Hw30					// Synchronization control
#define	HwTC32IRQ_RSYNC_EN		~Hw30					// Synchronization control
#define	HwTC32IRQ_IRQEN4_EN		Hw20					// Enable interrupt at the rising edge of a counter bit selected by BITSEL
#define	HwTC32IRQ_IRQEN4_DIS		~Hw20					// Enable interrupt at the rising edge of a counter bit selected by BITSEL
#define	HwTC32IRQ_IRQEN3_EN		Hw19					// Enable interrupt at the end of prescale count
#define	HwTC32IRQ_IRQEN3_DIS		~Hw19					// Enable interrupt at the end of prescale count
#define	HwTC32IRQ_IRQEN2_EN		Hw18					// Enable interrupt at the end of count
#define	HwTC32IRQ_IRQEN2_DIS		~Hw18					// Enable interrupt at the end of count
#define	HwTC32IRQ_IRQEN1_EN		Hw17					// Enable interrupt when the counter value matched with CMP1
#define	HwTC32IRQ_IRQEN1_DIS		~Hw17					// Enable interrupt when the counter value matched with CMP1
#define	HwTC32IRQ_IRQEN0_EN		Hw16					// Enable interrupt when the counter value matched with CMP0
#define	HwTC32IRQ_IRQEN0_DIS		~Hw16					// Enable interrupt when the counter value matched with CMP0

//Done...
/////////


/************************************************************************
*	GPIO Port Register Define					(Base Addr = 0xF1005000)
************************************************************************/
/////////
//Done...


//--------------------------
//	G P I O   A
//--------------------------
#define	HwGDATA_A				*(volatile unsigned long *)0xF1005000	// R/W, GPIO_A Data
#define	HwGPADAT				*(volatile unsigned long *)0xF1005000	// R/W, GPIO_A Data
#define	HwGIOCON_A	        	*(volatile unsigned long *)0xF1005004	// R/W, GPIO_A Direction Control
#define	HwGSEL_A				*(volatile unsigned long *)0xF1005008	// R/W, GPIO_A Function Select
#define	HwNGSEL_A				*(volatile unsigned long *)0xF100500C	// R/W, GPIO_A Normal GPIO Selection

#define	HwGSEL_A_HDD	        	Hw4		// GPIO_A[0:15, 18:25] pins are working as 16 bit HDD function
#define	HwGSEL_A_NFC8	        	Hw3		// GPIO_A[0:7, 18:25] pins are working as 8 bit NFC function
#define	HwGSEL_A_NFC16	      	Hw2		// GPIO_A[0:15, 18:25] pins are working as 16 bit NFC function
#define 	HwGSEL_A_EHI8	        	Hw1		// GPIO_A[0:7, 18:22] pins are working as 8bit EHI function
#define 	HwGSEL_A_EHI16	      	Hw0		// GPIO_A[0:15, 18:22] pins are working as 16bit EHI function
#define    HwGSEL_A_NORMAL_GPIO	HwZERO  //GPIO_A Pins are working as normal gpio

//--------------------------
//	G P I O   B
//--------------------------
#define	HwGDATA_B				*(volatile unsigned long *)0xF1005010	// R/W, GPIO_B Data
#define	HwGIOCON_B	        	*(volatile unsigned long *)0xF1005014	// R/W, GPIO_B Direction Control
#define	HwGSEL_B				*(volatile unsigned long *)0xF1005018	// R/W, GPIO_B Function Select
#define	HwNGSEL_B				*(volatile unsigned long *)0xF100501C	// R/W, GPIO_B Normal GPIO Selection


#define	HwGSEL_B_CDIF			Hw20	
#define	HwGSEL_B_TCO1			Hw19
#define	HwGSEL_B_TCO0			Hw18

#define 	HwGSEL_B_ND_WEN	     	Hw17	 // GPIO_B[7] pins are working as NAND Flash Write Enable signal of External memory contoller
#define 	HwGSEL_B_IDE_CSN		Hw16	 // GPIO_B[6] pins are working as IDE chip select signal of external memory controller
#define 	HwGSEL_B_N3_SPI		Hw15	 // GPIO_B[12:15] pins are working as Nibble3 SPI Function Select
#define 	HwGSEL_B_N0_SPI		Hw14	 // GPIO_B[0:3] pins are working as Nibble0 SPI Function Select

#define 	HwGSEL_B_N3_UT1		Hw13	 // GPIO_B[12:15] pins are working as Nibble2 UART1 Function Select
#define 	HwGSEL_B_N1_UT1		Hw12	 // GPIO_B[4:7] pins are working as Nibble2 UART1 Function Select
#define 	HwGSEL_B_N2_UT0		Hw11	 // GPIO_B[8:11] pins are working as Nibble2 UART0 Function Select
#define 	HwGSEL_B_N0_UT0		Hw10	 // GPIO_B[0:3] pins are working as Nibble0 UART0 Function Select

#define 	HwGSEL_B_N2_EI7		Hw9	        // GPIO_B[11] pins are working as Nibble2 EINT3 Function Select
#define 	HwGSEL_B_N2_EI6		Hw8	        // GPIO_B[10] pins are working as Nibble2 EINT3 Function Select
#define 	HwGSEL_B_N2_EI5		Hw7	        // GPIO_B[9] pins are working as Nibble2 EINT3 Function Select
#define 	HwGSEL_B_N2_EI4		Hw6	        // GPIO_B[8] pins are working as Nibble2 EINT3 Function Select

#define 	HwGSEL_B_N1_EI3		Hw5	        // GPIO_B[7] pins are working as Nibble1 EINT3 Function Select
#define 	HwGSEL_B_N1_EI2		Hw4	        // GPIO_B[6] pins are working as Nibble1 EINT2 Function Select
#define 	HwGSEL_B_N1_EI1		Hw3	        // GPIO_B[5] pins are working as Nibble1 EINT1 Function Select
#define 	HwGSEL_B_N1_EI0		Hw2	        // GPIO_B[4] pins are working as Nibble1 EINT0 Function Select

//--------------------------
//	G P I O   C
//--------------------------
#define	HwGDATA_C				*(volatile unsigned long *)0xF1005020	// R/W, GPIO_C Data
#define	HwGIOCON_C	        	*(volatile unsigned long *)0xF1005024	// R/W, GPIO_C Direction Control
#define	HwGSEL_C				*(volatile unsigned long *)0xF1005028	// R/W, GPIO_C Function Select
#define 	HwGSEL_C_UT2_2_UT2_2      	Hw11				// UART2 Interface 
#define 	HwGSEL_C_UT2_2_GPIO	      	~Hw11				// Normal GPIO
#define 	HwGSEL_C_UT2_4_UT2_4      	Hw10				// UART2 Interface 
#define 	HwGSEL_C_UT2_4_GPIO	      	~Hw10				// Normal GPIO
#define 	HwGSEL_C_AIN3     			Hw9				
#define 	HwGSEL_C_AIN2      			Hw8				
#define 	HwGSEL_C_NFC8_NFC8      	Hw7				// NFC8 Interface 
#define 	HwGSEL_C_NFC8_GPIO	      	~Hw7				// Normal GPIO
#define 	HwGSEL_C_HDD8_HDD8     		Hw6				// HDD8 Interface 
#define 	HwGSEL_C_HDD8_GPIO	      	~Hw6				// Normal GPIO
#define 	HwGSEL_C_SD8_SD8     		Hw5				// 1ch(8bit) SD/MMC Function Select
#define 	HwGSEL_C_SD8_GPIO	      	~Hw5				// Normal GPIO
#define 	HwGSEL_C_SD4_4_SD4_4     	Hw4				// 2ch(4bit/4bit) SD/MMC Function Select
#define 	HwGSEL_C_SD4_4_GPIO	      	~Hw4				// Normal GPIO
#define 	HwGSEL_C_SD4_1_SD4_1     	Hw3				// 2ch (4 bit/1 bit) SD/MMC Interface
#define 	HwGSEL_C_SD4_1_GPIO	      	~Hw3				// Normal GPIO
#define 	HwGSEL_C_SD4_0_SD4_0     	Hw2				// 1ch (4 bit) SD/MMC Interface
#define 	HwGSEL_C_SD4_0_GPIO	      	~Hw2				// Normal GPIO
#define 	HwGSEL_C_SD1_1_SD1_1     	Hw1				// 2ch (1 bit/1bit) SD/MMC Interface
#define 	HwGSEL_C_SD1_1_GPIO	      	~Hw1				// Normal GPIO
#define 	HwGSEL_C_SD1_0_SD1_0     	Hw0				// 1ch (1 bit) SD/MCC Interface
#define 	HwGSEL_C_SD1_0_GPIO	      	~Hw0				// Normal GPIO


#define	HwNGSEL_C				*(volatile unsigned long *)0xF100502C	// R/W, GPIO_C Normal GPIO Selection

#define	HwGSEL_C_UT2_2	       Hw11
#define	HwGSEL_C_UT2_4	       Hw10
#define	HwGSEL_C_AIN3	        	Hw9
#define	HwGSEL_C_AIN2	        	Hw8

#define	HwGSEL_C_NFC8	        	Hw7	
#define	HwGSEL_C_HDD	        	Hw6	

#define 	HwGSEL_C_SD8	        	Hw5
#define 	HwGSEL_C_SD4_2        	Hw4
#define 	HwGSEL_C_SD4_1        	Hw3
#define 	HwGSEL_C_SD4_0        	Hw2
#define 	HwGSEL_C_SD1_1	       Hw1
#define 	HwGSEL_C_SD1_0	       Hw0

#define    HwGSEL_C_NORMAL_GPIO	HwZERO  //GPIO_C Pins are working as normal GPIO

//--------------------------
//	G P I O   E
//--------------------------
#define	HwGDATA_E				*(volatile unsigned long *)0xF1005040	// R/W, GPIO_E Data
#define	HwGIOCON_E	        	*(volatile unsigned long *)0xF1005044	// R/W, GPIO_E Direction Control
#define	HwGSEL_E				*(volatile unsigned long *)0xF1005048	// R/W, GPIO_E Function Select
#define	HwNGSEL_E				*(volatile unsigned long *)0xF100504C	// R/W, GPIO_E Normal GPIO Selection


#define 	HwGSEL_E_SPDIF		Hw2
#define 	HwGSEL_E_I2C			Hw1		// GPIO_E[5:6] pins are working as I2C function [E6:SCL, E5:SDA]
#define 	HwGSEL_E_DAI			Hw0		// GPIO_E[0:4] pins are working as DAI function [E3:DAI, E4:DAO, E2:MCLK, E1:LRCK, E0:BCLK]

#define   HwGSEL_E_GPIO7            Hw7
#define   HwGSEL_E_GPIO6            Hw6
#define   HwGSEL_E_GPIO5            Hw5
#define   HwGSEL_E_GPIO4            Hw4
#define   HwGSEL_E_GPIO3            Hw3
#define   HwGSEL_E_GPIO2            Hw2
#define   HwGSEL_E_GPIO1            Hw1
#define   HwGSEL_E_GPIO0            Hw0


//--------------------------
//	G P I O   F
//--------------------------
#define	HwGDATA_F				*(volatile unsigned long *)0xF1005050	// R/W, GPIO_F Data
#define	HwGIOCON_F	        	*(volatile unsigned long *)0xF1005054	// R/W, GPIO_F Direction Control
#define	HwGSEL_F				*(volatile unsigned long *)0xF1005058	// R/W, GPIO_F Function Select
#define	HwNGSEL_F				*(volatile unsigned long *)0xF100505C	// R/W, GPIO_F Normal GPIO Selection


#define 	HwGSEL_F_ND_WEN	     	Hw17		// GPIO_F[7] pins are working as NAND Flash Write Enable signal of External memory contoller
#define 	HwGSEL_F_IDE_CSN		Hw16		// GPIO_F[6] pins are working as IDE chip select signal of external memory controller
#define 	HwGSEL_F_N3_SPI1	 	Hw15		// GPIO_F[12:15] pins are working as Nibble3 SPI Function Select
#define 	HwGSEL_F_N0_SPI0	 	Hw14		// GPIO_F[0:3] pins are working as Nibble0 SPI Function Select

#define 	HwGSEL_F_N3_UT1	 	Hw13		// GPIO_F[12:15] pins are working as Nibble2 UART1 Function Select
#define 	HwGSEL_F_N1_UT3	 	Hw12		// GPIO_F[4:7] pins are working as Nibble2 UART1 Function Select
#define 	HwGSEL_F_N2_UT2	 	Hw11		// GPIO_F[8:11] pins are working as Nibble2 UART0 Function Select
#define 	HwGSEL_F_N0_UT2	 	Hw10		// GPIO_F[0:3] pins are working as Nibble0 UART0 Function Select

#define 	HwGSEL_F_N2_EI7	 	Hw9	        // GPIO_F[11] pins are working as Nibble2 EINT3 Function Select
#define 	HwGSEL_F_N2_EI6	 	Hw8	        // GPIO_F[10] pins are working as Nibble2 EINT3 Function Select
#define 	HwGSEL_F_N2_EI5	 	Hw7	        // GPIO_F[9] pins are working as Nibble2 EINT3 Function Select
#define 	HwGSEL_F_N2_EI4	 	Hw6	        // GPIO_F[8] pins are working as Nibble2 EINT3 Function Select

#define 	HwGSEL_F_N1_EI3	 	Hw5	        // GPIO_F[7] pins are working as Nibble1 EINT3 Function Select
#define 	HwGSEL_F_N1_EI2	 	Hw4	        // GPIO_F[6] pins are working as Nibble1 EINT2 Function Select
#define 	HwGSEL_F_N1_EI1	 	Hw3	        // GPIO_F[5] pins are working as Nibble1 EINT1 Function Select
#define 	HwGSEL_F_N1_EI0	 	Hw2	        // GPIO_F[4] pins are working as Nibble1 EINT0 Function Select

#define 	HwGSEL_F_I2C			Hw1		// GPIO_F[14:15] pins are working as I2C function [F15:SCL, F14:SDA]
#define 	HwGSEL_F_CIF			Hw0		// GPIO_F[0:12] pins are working as Camera Interface Function Select

//--------------------------
//	G P I O   G
//--------------------------
#define	HwGDATA_G				*(volatile unsigned long *)0xF1005060	// R/W, GPIO_E Data
#define	HwGIOCON_G	        	*(volatile unsigned long *)0xF1005064	// R/W, GPIO_E Direction Control
#define	HwGSEL_G				*(volatile unsigned long *)0xF1005068	// R/W, GPIO_E Function Select
#define	HwNGSEL_G				*(volatile unsigned long *)0xF100506C	// R/W, GPIO_E Normal GPIO Selection


#define 	HwGSEL_G_SRST			Hw6

#define 	HwGSEL_G_LCI8			Hw5		// GPIO_G[0:7, 18:21] pins are working as 8bit LCD CCIR Interface Function Select
#define 	HwGSEL_G_LCI16		Hw4		// GPIO_G[0:15, 18:21] pins are working as 16bit LCD CCIR Interface Function Select
#define 	HwGSEL_G_LCI18		Hw3		// GPIO_G[0:17, 18:21] pins are working as 18bit LCD CCIR Interface Function Select

#define 	HwGSEL_G_LMI8			Hw2		// GPIO_G[0:7, 18:22] pins are working as 8bit LCD Memory Interface Function Select
#define 	HwGSEL_G_LMI16		Hw1		// GPIO_G[0:15, 18:22] pins are working as 16bit LCD Memory Interface Function Select
#define 	HwGSEL_G_LMI18		Hw0		// GPIO_G[0:17, 18:22] pins are working as 18bit LCD Memory Interface Function Select

//--------------------------
//	G P I O   XA
//--------------------------
#define	HwGDATA_XA		*(volatile unsigned long *)0xF1005070	// R/W, GPIO_XA Data
#define	HwGIOCON_XA		*(volatile unsigned long *)0xF1005074	// R/W, GPIO_XA Direction Control
#define	HwGSEL_XA			*(volatile unsigned long *)0xF1005078	// R/W, GPIO_XA Function Select

#define	HwGSEL_XA_XA31		Hw31	// GPIO_XA[31] pin is working as External Chip Select2 Function Select
#define	HwGSEL_XA_nCS2		Hw31	// GPIO_XA[31] pin is working as External Chip Select2 Function Select
#define	HwGSEL_XA_XA30		Hw30	// GPIO_XA[30] pin is working as External Chip Select1 Function Select
#define	HwGSEL_XA_nCS1		Hw30	// GPIO_XA[30] pin is working as External Chip Select1 Function Select
#define	HwGSEL_XA_XA29		Hw29	// GPIO_XA[29] pin is working as External Chip Select0 Function Select
#define	HwGSEL_XA_nCS0		Hw29	// GPIO_XA[29] pin is working as External Chip Select0 Function Select

#define	HwGSEL_XA_XA28	   	Hw28	// GPIO_XA[28] pin is working as SDRAM Clock Enable Function Select
#define	HwGSEL_XA_SDR_CKE	Hw28	// GPIO_XA[28] pin is working as SDRAM Clock Enable Function Select
#define	HwGSEL_XA_XA27	    	Hw27	// GPIO_XA[27] pin is working as SDRAM Clock Function Select
#define	HwGSEL_XA_SDR_CLK	Hw27	// GPIO_XA[27] pin is working as SDRAM Clock Function Select
#define	HwGSEL_XA_XA26	    	Hw26	// GPIO_XA[26] pin is working as SDRAM nCS Function Select
#define	HwGSEL_XA_SDR_nCS	Hw26	// GPIO_XA[26] pin is working as SDRAM nCS Function Select

#define	HwGSEL_XA_XA25	    	Hw25	// GPIO_XA[25] pin is working as nWE Function Select
#define	HwGSEL_XA_nWE		Hw25	// GPIO_XA[25] pin is working as nWE Function Select

#define	HwGSEL_XA_XA24	   	Hw24	// GPIO_XA[24] pin is working as nOE Function Select
#define	HwGSEL_XA_nOE			Hw24	// GPIO_XA[24] pin is working as nOE Function Select

#define	HwGSEL_XA_XA23		Hw23	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA22		Hw22	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA21		Hw21	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA20		Hw20	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA19		Hw19	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA18		Hw18	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA17		Hw17	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA16		Hw16	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA15		Hw15	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA14		Hw14	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA13		Hw13	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA12		Hw12	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA11		Hw11	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA10		Hw10	// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA9			Hw9		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA8			Hw8		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA7			Hw7		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA6			Hw6		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA5			Hw5		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA4			Hw4		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA3			Hw3		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA2			Hw2		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA1			Hw1		// GPIO_XA[x] pin is working as XAx function
#define	HwGSEL_XA_XA0			Hw0		// GPIO_XA[x] pin is working as XAx function


//--------------------------
//	G P I O   XD
//--------------------------
#define	HwGDATA_XD			*(volatile unsigned long *)0xF1005080	// R/W, GPIO_XD Data	
#define	HwGIOCON_XD			*(volatile unsigned long *)0xF1005084	// R/W, GPIO_XD Direction Control
#define	HwGSEL_XD				*(volatile unsigned long *)0xF1005088	// R/W, GPIO_XD Function Select

#define	HwGSEL_XD_XD31		Hw31		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD30		Hw30		// GPIO_XD[x] pin is working as X[x] function

#define	HwGSEL_XD_XD29		Hw29		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD28		Hw28		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD27		Hw27		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD26		Hw26		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD25		Hw25		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD24		Hw24		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD23		Hw23		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD22		Hw22		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD21		Hw21		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD20		Hw20		// GPIO_XD[x] pin is working as X[x] function

#define	HwGSEL_XD_XD19		Hw19		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD18		Hw18		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD17		Hw17		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD16		Hw16		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD15		Hw15		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD14		Hw14		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD13		Hw13		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD12		Hw12		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD11		Hw11		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD10		Hw10		// GPIO_XD[x] pin is working as X[x] function

#define	HwGSEL_XD_XD9			Hw9		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD8			Hw8		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD7			Hw7		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD6			Hw6		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD5			Hw5		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD4			Hw4		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD3			Hw3		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD2			Hw2		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD1			Hw1		// GPIO_XD[x] pin is working as X[x] function
#define	HwGSEL_XD_XD0			Hw0		// GPIO_XD[x] pin is working as X[x] function


//--------------------------
//	G P I O   XAA
//--------------------------
#define	HwGDATA_XAA			*(volatile unsigned long *)0xF1005090	// R/W, GPIO_XD Data	
#define	HwGIOCON_XAA			*(volatile unsigned long *)0xF1005094	// R/W, GPIO_XD Direction Control
#define	HwGSEL_XAA			*(volatile unsigned long *)0xF1005098	// R/W, GPIO_XD Function Select


//--------------------------
//	G P I O   Pull Enable/Select Register
//--------------------------
#define	HwGPIOB_PE			*(volatile unsigned long *)0xF10050B8   //Pull Enable Register	
#define	HwGPIOB_PS			*(volatile unsigned long *)0xF10050BC	//Pull Select Register

#define	HwGPIOC_PE			*(volatile unsigned long *)0xF10050C0   //Pull Enable Register	
#define	HwGPIOC_PS			*(volatile unsigned long *)0xF10050C4	//Pull Select Register

#define	HwGPIOE_PE				*(volatile unsigned long *)0xF10050D0   //Pull Enable Register	
#define	HwGPIOE_PS				*(volatile unsigned long *)0xF10050D4	//Pull Select Register

#define	HwGPIOF_PE				*(volatile unsigned long *)0xF10050D8   //Pull Enable Register	
#define	HwGPIOF_PS				*(volatile unsigned long *)0xF10050DC	//Pull Select Register

#define	HwGPIOG_PE			*(volatile unsigned long *)0xF10050E0   //Pull Enable Register	
#define	HwGPIOG_PS			*(volatile unsigned long *)0xF10050EC	//Pull Select Register

#define	HwXA_PE		        	*(volatile unsigned long *)0xF10050E8   //Pull Enable Register	
#define	HwXA_PS		        	*(volatile unsigned long *)0xF10050EC	//Pull Select Register

#define	HwXD_PE		        	*(volatile unsigned long *)0xF10050F0   //Pull Enable Register	
#define	HwXD_PS		        	*(volatile unsigned long *)0xF10050F4	//Pull Select Register

//Done...
//////////


/************************************************************************
*	Clock Generator Register Define				(Base Addr = 0xF1006000)
************************************************************************/
/////////
//Done...

#define	HwCLKCTRL					*(volatile unsigned long *)0xF1006000	// R/W, CPU & Bus Clock Control Register
#define	HwCLKCTRL_XE_EN			Hw31			// XIN Oscillator Enabled
#define	HwCLKCTRL_XE_DIS			~Hw31			// XIN Oscillator Disabled

#define	HwCLKCTRL_MB		Hw29				// Fcpu = Fbus

#define	HwCLKCTRL_CKSEL_Fpll0		HwZERO			// Direct output from PLL0(Fsrc = Fpll0)
#define	HwCLKCTRL_CKSEL_Fpll1		Hw0				// Direct output from PLL1(Fsrc = Fpll0)
#define	HwCLKCTRL_CKSEL_Fpll0div	Hw1				// Divided output from PLL0(Fsrc = Fpll0div)
#define	HwCLKCTRL_CKSEL_Fpll1div	(Hw1|Hw0)		// Divided output from PLL1(Fsrc = Fpll0div)
#define	HwCLKCTRL_CKSEL_Fxin		Hw2				// XIN from external main oscillator(shared by UTMI clock)(Fsrc = Fxin)
#define	HwCLKCTRL_CKSEL_Fxindiv	(Hw2|Hw0)		// Divided output from XIN(Fsrc = Fxindiv)
#define	HwCLKCTRL_CKSEL_Fxtin		(Hw2|Hw1)		// XTIN from external sub oscillator(shared by RTC)(Fsrc = Fxtin)
#define	HwCLKCTRL_CKSEL_Fxtindiv	(Hw2|Hw1|Hw0)	        // Divided output from XTIN(Fsrc = Fxtindiv)

#define	HwPLL0CFG				*(volatile unsigned long *)0xF1006004	// R/W, PLL0 Configuration Register
#define	HwPLL0CFG_PD_DIS		Hw31					// PLL Disable
#define	HwPLL0CFG_PD_EN		~Hw31					// PLL Enable

#define	HwPLL1CFG				*(volatile unsigned long *)0xF1006008	// R/W, PLL1 Configuration Register
#define	HwPLL1CFG_PD_DIS		Hw31					// PLL Disable
#define	HwPLL1CFG_PD_EN		~Hw31					// PLL Enable

#define	HwCLKDIVC				*(volatile unsigned long *)0xF100600C	// R/W, Divided Clock Configuration Register
#define	HwCLKDIVC_P0E_EN		Hw31					// PLL0 Divider Enable
#define	HwCLKDIVC_P0E_DIS		~Hw31					// PLL0 Divider Disable
#define	HwCLKDIVC_P0P_EN		Hw30					// Post PLL0 Divider Enable
#define	HwCLKDIVC_P0P_DIS		~Hw30					// Post PLL0 Divider Disable
#define	HwCLKDIVC_XE_EN		Hw15					// XIN Divider Enable
#define	HwCLKDIVC_XE_DIS		~Hw15					// XIN Divider Disable
#define	HwCLKDIVC_XP_EN		Hw14					// Post XIN Divider Enable
#define	HwCLKDIVC_XP_DIS		~Hw14					// Post XIN Divider Disable
#define	HwCLKDIVC_XTE_EN		Hw7						// XTIN Divider Enable
#define	HwCLKDIVC_XTE_DIS		~Hw7					// XTIN Divider Disable
#define	HwCLKDIVC_XTP_EN		Hw6						// Post XTIN Divider Enable
#define	HwCLKDIVC_XTP_DIS		~Hw6					// Post XTIN Divider Disable
		
#define	HwMODECTR					*(volatile unsigned long *)0xF1006010	// R/W, Operating Mode Control Register
#define	HwMODECTR_PDCK_XTIN		Hw9					// XTIN Selected
#define	HwMODECTR_PDCK_XIN		~Hw9				// XIN Selected
#define	HwMODECTR_PD_ON			Hw8					// Enter Power Down mode
#define	HwMODECTR_PD_OFF			~Hw8				// Stay in Normal mode
#define	HwMODECTR_HALT_ON		Hw0					// Enter Halt mode
#define	HwMODECTR_HALT_OFF		~Hw0				// Stay in Normal mode

#define	HwBCLKCTR					*(volatile unsigned long *)0xF1006014	// R/W, Bus Clock Enable Register
#define 	HwBCLKCTR_SPIMS1_ON		Hw30				// Enable the Bus Clock for Internal Memory  Controller
#define 	HwBCLKCTR_SPIMS1_OFF 		~Hw30				// Disable the Bus Clock for Internal Memory  Controller
#define 	HwBCLKCTR_IMEM_ON			Hw28				// Enable the Bus Clock for Internal Memory  Controller
#define 	HwBCLKCTR_IMEM_OFF 		~Hw28				// Disable the Bus Clock for Internal Memory  Controller
#define 	HwBCLKCTR_EMEM_ON		Hw27				// Enable the Bus Clock for External Memory  Controller
#define 	HwBCLKCTR_EMEM_OFF 		~Hw27				// Disable the Bus Clock for External Memory  Controller
#define 	HwBCLKCTR_MSCL_ON		Hw25				// Enable the Bus Clock for Memory Scaler Controller
#define 	HwBCLKCTR_MSCL_OFF 		~Hw25 				// Disable the Bus Clock for Memory Scaler Controller
#define	HwBCLKCTR_SPIMS0_ON		Hw24				// Enable the Bus Clock for SPI Slave Interface Controller
#define	HwBCLKCTR_SPIMS0_OFF		~Hw24				// Disable the Bus Clock for SPI Slave Interface Controller
#define	HwBCLKCTR_UA1_ON			Hw23				// Enable the Bus Clock for UART1 Controller
#define	HwBCLKCTR_UA1_OFF		~Hw23				// Disable the Bus Clock for UART1 Controller
#define	HwBCLKCTR_usbDMA_ON		Hw22				// Enable the Bus Clock for USB DMA Controller(GDMA1)
#define	HwBCLKCTR_usbDMA_OFF	~Hw22				// Disable the Bus Clock for USB DMA Controller

//DAI_DMA1_CH0_INCLUDE
#define	HwBCLKCTR_GDMA1_ON		Hw22				// Enable the Bus Clock for USB DMA Controller(GDMA1)
#define	HwBCLKCTR_GDMA1_OFF		~Hw22				// Disable the Bus Clock for USB DMA Controller

#define	HwBCLKCTR_CIC_ON			Hw21			
#define	HwBCLKCTR_CIC_OFF			~Hw21			

#define	HwBCLKCTR_IDE_ON			Hw20			
#define	HwBCLKCTR_IDE_OFF			~Hw20			
//#define	HwBCLKCTR_HDD_ON			Hw20			
//#define	HwBCLKCTR_HDD_OFF		~Hw20			

#define	HwBCLKCTR_EHI_ON			Hw19				// Enable the Bus Clock for External Host Interface Controller
#define	HwBCLKCTR_EHI_OFF		~Hw19				// Disable the Bus Clock for External Host Interface Controller
#define	HwBCLKCTR_G2D_ON			Hw18				
#define	HwBCLKCTR_G2D_OFF		~Hw18				
#define	HwBCLKCTR_SD_ON			Hw17				// Enable the Bus Clock for SD/MMC Controller
#define	HwBCLKCTR_SD_OFF			~Hw17				// Disable the Bus Clock for SD/MMC Controller
#define	HwBCLKCTR_ND_ON			Hw16					// Enable the Bus Clock for NAND Flash/ LCD/ HDD Controller
#define	HwBCLKCTR_ND_OFF			~Hw16				// Disable the Bus Clock for NAND Flash/ LCD/ HDD Controller
#define	HwBCLKCTR_RTC_ON			Hw15				// Enable the Bus Clock for RTC Controller
#define	HwBCLKCTR_RTC_OFF		~Hw15				// Disable the Bus Clock for RTC Controller

#define	HwBCLKCTR_JPG_ON			Hw14	
#define	HwBCLKCTR_JPG_OFF		~Hw14	
#define	HwBCLKCTR_LCD_ON		Hw13	
#define	HwBCLKCTR_LCD_OFF		~Hw13	

#define	HwBCLKCTR_DMA3ch_ON		Hw12				// Enable the Bus Clock for 3 Channel DMA Controller(GDMA0)
#define	HwBCLKCTR_DMA3ch_OFF	~Hw12				// Disable the Bus Clock for 3 Channel DMA Controller

#define	HwBCLKCTR_UBH_ON	Hw11	
#define	HwBCLKCTR_UBH_OFF	~Hw11	

#define	HwBCLKCTR_ADC_ON			Hw10				// Enable the Bus Clock for ADC controller
#define	HwBCLKCTR_ADC_OFF		~Hw10				// Disable the Bus Clock for ADC Controller
#define	HwBCLKCTR_ECC_ON			Hw9					// Enable the Bus Clock for ECC Controller
#define	HwBCLKCTR_ECC_OFF		~Hw9				// Disable the Bus Clock for ECC Controller

#define	HwBCLKCTR_UA3_ON			Hw8					// Enable the Bus Clock for ECC Controller
#define	HwBCLKCTR_UA3_OFF			~Hw8				// Disable the Bus Clock for ECC Controller

#define	HwBCLKCTR_I2C_ON			Hw7					// Enable the Bus Clock for I2C Controller
#define	HwBCLKCTR_I2C_OFF			~Hw7				// Disable the Bus Clock for I2C Controller

#define	HwBCLKCTR_UA2_ON			Hw6					// Enable the Bus Clock for ECC Controller
#define	HwBCLKCTR_UA2_OFF			~Hw6				// Disable the Bus Clock for ECC Controller

#define	HwBCLKCTR_UA0_ON			Hw5					// Enable the Bus Clock for UART0 Controller
#define	HwBCLKCTR_UA0_OFF		~Hw5				// Disable the Bus Clock for UART0 Controller
#define	HwBCLKCTR_UB_ON			Hw4					// Enable the Bus Clock for USB2.0 Device Controller
#define	HwBCLKCTR_UB_OFF			~Hw4				// Disable the Bus Clock for USB2.0 Device Controller
#define	HwBCLKCTR_GPIO_ON		Hw3					// Enable the Bus Clock for GPIO Controller
#define	HwBCLKCTR_GPIO_OFF		~Hw3				// Disable the Bus Clock for GPIO Controller
#define	HwBCLKCTR_TMR_ON			Hw2					// Enable the Bus Clock for Timer Controller
#define	HwBCLKCTR_TMR_OFF		~Hw2				// Disable the Bus Clock for Timer Controller
#define	HwBCLKCTR_INT_ON			Hw1					// Enable the Bus Clock for Interrupt Controller
#define	HwBCLKCTR_INT_OFF			~Hw1				// Disable the Bus Clock for Interrupt Controller
#define	HwBCLKCTR_DAI_ON			Hw0					// Enable the Bus Clock for DAI Controller
#define	HwBCLKCTR_DAI_OFF		~Hw0				// Disable the Bus Clock for DAI Controller



#define	HwSWRESET					*(volatile unsigned long *)0xF1006018	// R/W, UART Clock
#define	HwSWRESET_ARM_ON		Hw31				// ARM Controller Reset
#define	HwSWRESET_ARM_OFF		~Hw31				// ARM Controller not Reset
#define	HwSWRESET_SPIMS1_ON		Hw30				// SPI1 Master/Slave Controller Reset
#define	HwSWRESET_SPIMS1_OFF		~Hw30				// SPI1 Master/Slave Controller not Reset
#define	HwSWRESET_DTCM_ON		Hw29				// DTMC Memory Controller Reset
#define	HwSWRESET_DTCM_OFF		~Hw29				// DTMC Memory Controller not Reset


#define	HwSWRESET_INTMEM_ON		Hw28				// Internal Memory Controller Reset
#define	HwSWRESET_INTMEM_OFF	~Hw28				// Internal Memory Controller not Reset
#define	HwSWRESET_EXTMEM_ON		Hw27				// External Memory Controller Reset
#define	HwSWRESET_EXTMEM_OFF	~Hw27				// External Memory Controller not Reset
#define	HwSWRESET_MBUS_ON		Hw26				// Main Bus Components(should be '1') Reset
#define	HwSWRESET_MBUS_OFF		~Hw26				// Main Bus Components(should be '1') not Reset

#define	HwSWRESET_MSC_ON		Hw25
#define	HwSWRESET_MSC_OFF		~Hw25

#define	HwSWRESET_SPIMS0_ON		Hw24				// SPI0 Master/Slave  Interface Controller Reset
#define	HwSWRESET_SPIMS0_OFF		~Hw24				// SPI0 Master/Slave Interface Controller not Reset
#define	HwSWRESET_UA1_ON			Hw23				// UART1 Controller Reset
#define	HwSWRESET_UA1_OFF		~Hw23				// UART1 Controller not Reset
#define	HwSWRESET_usbDMA_ON		Hw22				// USB DMA Controller Reset (GDMA1)
#define	HwSWRESET_usbDMA_OFF		~Hw22				// USB DMA Controller not Reset(GDMA1)

#define	HwSWRESET_CIF_ON			Hw21
#define	HwSWRESET_CIF_OFF		~Hw21

#define	HwSWRESET_HDD_ON		Hw20
#define	HwSWRESET_HDD_OFF		~Hw20
#define	HwSWRESET_IDE_ON			Hw20
#define	HwSWRESET_IDE_OFF		~Hw20

#define	HwSWRESET_EHI_ON			Hw19				// External Host Interface Controller Reset
#define	HwSWRESET_EHI_OFF		~Hw19				// External Host Interface Controller not Reset

/*
#define	HwSWRESET_G2D_ON			Hw18
#define	HwSWRESET_G2D_OFF		~Hw18
*/
#define	HwSWRESET_2D				Hw18

#define	HwSWRESET_SD_ON			Hw17				// SD/MMC Controller Reset
#define	HwSWRESET_SD_OFF			~Hw17				// SD/MMC Controller not Reset
#define	HwSWRESET_ND_ON			Hw16				// NAND Flash Controller Reset
#define	HwSWRESET_ND_OFF			~Hw16				// NAND Flash Controller not Reset
#define	HwSWRESET_RTC_ON			Hw15				// RTC Controller Reset
#define	HwSWRESET_RTC_OFF		~Hw15				// RTC Controller not Reset

#define	HwSWRESET_JPG_ON			Hw14	
#define	HwSWRESET_JPG_OFF	 	~Hw14		    

//#define	HwSWRESET_LCDC_ON		Hw13	
//#define	HwSWRESET_LCDC_OFF	    	~Hw13		
#define	HwSWRESET_LCD_ON		Hw13	
#define	HwSWRESET_LCD_OFF	    	~Hw13		

#define	HwSWRESET_DMA3ch_ON		Hw12				//GDMA0
#define	HwSWRESET_DMA3ch_OFF	~Hw12		

#define	HwSWRESET_USB11Host_ON	Hw11	
#define	HwSWRESET_USB11Host_OFF	~Hw11		

#define	HwSWRESET_ADC_ON			Hw10				// ADC Controller Reset
#define	HwSWRESET_ADC_OFF		~Hw10				// ADC Controller not Reset
#define	HwSWRESET_ECC_ON			Hw9					// ECC Controller Reset
#define	HwSWRESET_ECC_OFF		~Hw9				// ECC Controller not Reset

#define	HwSWRESET_UA3_ON			Hw8					// UART3 Controller Reset
#define	HwSWRESET_UA3_OFF		~Hw8				// UART3 Controller not Reset

#define	HwSWRESET_I2C_ON			Hw7					// I2C Controller Reset
#define	HwSWRESET_I2C_OFF		~Hw7				// I2C Controller not Reset

#define	HwSWRESET_UA2_ON			Hw6					// UART2Controller Reset
#define	HwSWRESET_UA2_OFF		~Hw6				// UART2 Controller not Reset

#define	HwSWRESET_UA0_ON			Hw5					// UART0 Controller Reset
#define	HwSWRESET_UA0_OFF		~Hw5				// UART0 Controller not Reset
#define	HwSWRESET_UB_ON			Hw4					// USB2.0 Device Controller Reset
#define	HwSWRESET_UB_OFF			~Hw4				// USB2.0 Device Controller not Reset
#define	HwSWRESET_GPIO_ON		Hw3					// GPIO Controller Reset
#define	HwSWRESET_GPIO_OFF		~Hw3				// GPIO Controller not Reset
#define	HwSWRESET_TMR_ON			Hw2					// Timer Controller Reset
#define	HwSWRESET_TMR_OFF		~Hw2				// Timer Controller not Reset
#define	HwSWRESET_INT_ON			Hw1					// Interrupt Controller Reset
#define	HwSWRESET_INT_OFF		~Hw1				// Interrupt Controller not Reset
#define	HwSWRESET_DAI_ON			Hw0					// DAI Controller Reset
#define	HwSWRESET_DAI_OFF		~Hw0				// DAI Controller not Reset

// R/W, SDRAM Refresh Clock Control Register
#define	HwPCLKCFG0		        	*(volatile unsigned long *)0xF100601C	
#define	HwPCLKCFG0_EN1_ON		Hw31			
#define	HwPCLKCFG0_EN1_OFF		~Hw31			
#define	HwPCLKCFG0_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG0_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG0_SEL1_XIN		Hw30			
#define	HwPCLKCFG0_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG0_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG0_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG0_P1_ON			Hw27			
#define	HwPCLKCFG0_P1_OFF		~Hw27			

// R/W, I2C Controller Clock 	
#define	HwPCLKCFG1		        	*(volatile unsigned long *)0xF1006020	
#define	HwPCLKCFG1_EN1_ON		Hw31			
#define	HwPCLKCFG1_EN1_OFF		~Hw31			
#define	HwPCLKCFG1_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG1_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG1_SEL1_XIN		Hw30			
#define	HwPCLKCFG1_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG1_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG1_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG1_P1_ON			Hw27			
#define	HwPCLKCFG1_P1_OFF		~Hw27			

// R/W, SPI1/0 Master/Slave Interface Clock 
#define	HwPCLKCFG2		        	*(volatile unsigned long *)0xF1006024	
#define	HwPCLKCFG2_EN1_ON		Hw31			
#define	HwPCLKCFG2_EN1_OFF		~Hw31			
#define	HwPCLKCFG2_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG2_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG2_SEL1_XIN		Hw30			
#define	HwPCLKCFG2_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG2_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG2_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG2_P1_ON			Hw27			
#define	HwPCLKCFG2_P1_OFF		~Hw27			

#define	HwPCLKCFG2_EN0_ON			Hw15		
#define	HwPCLKCFG2_EN0_OFF		~Hw15		
#define	HwPCLKCFG2_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG2_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG2_SEL0_XIN		Hw14		
#define	HwPCLKCFG2_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG2_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG2_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)
#define	HwPCLKCFG2_P0_ON			Hw11		
#define	HwPCLKCFG2_P0_OFF			~Hw11		

// R/W, UART Controller Clock 
//Channel 1
#define	HwPCLKCFG3		        	*(volatile unsigned long *)0xF1006028	
#define	HwPCLKCFG3_EN1_ON		Hw31			
#define	HwPCLKCFG3_EN1_OFF		~Hw31			
#define	HwPCLKCFG3_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG3_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG3_SEL1_XIN		Hw30			
#define	HwPCLKCFG3_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG3_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG3_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG3_P1_ON			Hw27			
#define	HwPCLKCFG3_P1_OFF		~Hw27			
//Channel 0
#define	HwPCLKCFG3_EN0_ON		Hw15		
#define	HwPCLKCFG3_EN0_OFF		~Hw15		
#define	HwPCLKCFG3_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG3_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG3_SEL0_XIN		Hw14		
#define	HwPCLKCFG3_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG3_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG3_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)
#define	HwPCLKCFG3_P0_ON			Hw11		
#define	HwPCLKCFG3_P0_OFF		~Hw11		

// R/W, Timer-T Clock & SPI Master Interface Clock
#define	HwPCLKCFG4		        	*(volatile unsigned long *)0xF100602C	
#define	HwPCLKCFG4_EN1_ON		Hw31			
#define	HwPCLKCFG4_EN1_OFF		~Hw31			
#define	HwPCLKCFG4_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG4_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG4_SEL1_XIN		Hw30			
#define	HwPCLKCFG4_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG4_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG4_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG4_P1_ON			Hw27			
#define	HwPCLKCFG4_P1_OFF		~Hw27		
//SPI Master Interface Clock
#define	HwPCLKCFG4_EN0_ON			Hw15		
#define	HwPCLKCFG4_EN0_OFF		~Hw15		
#define	HwPCLKCFG4_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG4_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG4_SEL0_XIN		Hw14		
#define	HwPCLKCFG4_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG4_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG4_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)
#define	HwPCLKCFG4_P0_ON			Hw11		
#define	HwPCLKCFG4_P0_OFF			~Hw11		

// R/W, Timer-Z/X Controller Clock 
//Timer-Z
#define	HwPCLKCFG5		        	*(volatile unsigned long *)0xF1006030	
#define	HwPCLKCFG5_EN1_ON		Hw31			
#define	HwPCLKCFG5_EN1_OFF		~Hw31			
#define	HwPCLKCFG5_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG5_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG5_SEL1_XIN		Hw30			
#define	HwPCLKCFG5_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG5_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG5_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG5_P1_ON			Hw27			
#define	HwPCLKCFG5_P1_OFF		~Hw27			
//Timer-X
#define	HwPCLKCFG5_EN0_ON		Hw15		
#define	HwPCLKCFG5_EN0_OFF		~Hw15		
#define	HwPCLKCFG5_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG5_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG5_SEL0_XIN		Hw14		
#define	HwPCLKCFG5_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG5_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG5_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)
#define	HwPCLKCFG5_P0_ON			Hw11		
#define	HwPCLKCFG5_P0_OFF		~Hw11	

// R/W, DAI/ADC Controller Clock 
//DAI (DCLK0)
#define	HwPCLKCFG6		        	*(volatile unsigned long *)0xF1006034	
#define	HwPCLKCFG6_EN1_ON		Hw31			
#define	HwPCLKCFG6_EN1_OFF		~Hw31			
#define	HwPCLKCFG6_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG6_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG6_SEL1_XIN		Hw30			
#define	HwPCLKCFG6_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG6_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG6_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG6_P1_ON			Hw27			
#define	HwPCLKCFG6_P1_OFF		~Hw27			
//ADC
#define	HwPCLKCFG6_EN0_ON		Hw15		
#define	HwPCLKCFG6_EN0_OFF		~Hw15		
#define	HwPCLKCFG6_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG6_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG6_SEL0_XIN		Hw14		
#define	HwPCLKCFG6_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG6_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG6_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)
#define	HwPCLKCFG6_P0_ON			Hw11		
#define	HwPCLKCFG6_P0_OFF		~Hw11	

// R/W, DAI Controller Clock 
//DAI (DCLK1)
#define	HwPCLKCFG7		        	*(volatile unsigned long *)0xF1006038	
#define	HwPCLKCFG7_DSEL_ON		Hw21			// DAI Clock is DCLK1 (PCLKCFG7)
#define	HwPCLKCFG7_DSEL_OFF		~Hw21			// DAI Clock is DCLK0 (PCLKCFG6) 
#define	HwPCLKCFG7_DCLK1_EN_ON	Hw20			// DAI Clock1 (DCLK1) is enable
#define	HwPCLKCFG7_DCLK1_EN_OFF	~Hw20			// DAI Clock1 (DCLK1) is disable
#define	HwPCLKCFG7_DCLK1_PLL0	~(Hw19|Hw18|Hw17)			// DAI Clock1 direct output from PLL0
#define	HwPCLKCFG7_DCLK1_PLL1	Hw17			// DAI Clock1 direct output from PLL1
#define	HwPCLKCFG7_DCLK1_DIVPLL0	Hw18			// DAI Clock1 deviced output from PLL0
#define	HwPCLKCFG7_DCLK1_DIVPLL1	(Hw18|Hw17)		// DAI Clock1 deviced output from PLL1
#define	HwPCLKCFG7_DCLK1_XIN		Hw19			// DAI Clock1 XIN from external main oscillator
#define	HwPCLKCFG7_DCLK1_DIVXIN	(Hw19|Hw17)		// DAI Clock1 Divided output from XIN 
#define	HwPCLKCFG7_DCLK1_XTIN	(Hw19|Hw18)		// DAI Clock1 XTIN from external sub oscillator
#define	HwPCLKCFG7_DCLK1_DIVXTIN	(Hw19|Hw18|Hw17)	// DAI Clock1 Divided output from XTIN
#define	HwPCLKCFG7_DCLK1_DVM		Hw16				// Divider Mode  
#define	HwPCLKCFG7_DCLK1_DCOM	~Hw16  				// DCO Mode
// R/W, CIF Controller Clock 
//CIF Scaler Clock
#define	HwPCLKCFG8		        	*(volatile unsigned long *)0xF100603C	
#define	HwPCLKCFG8_EN1_ON		Hw31			
#define	HwPCLKCFG8_EN1_OFF		~Hw31			
#define	HwPCLKCFG8_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG8_SEL1_DIVPLL0	      Hw29			
#define	HwPCLKCFG8_SEL1_XIN		Hw30			
#define	HwPCLKCFG8_SEL1_DIVXIN	        (Hw30|Hw28)		
#define	HwPCLKCFG8_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG8_SEL1_DIVXTIN	        (Hw30|Hw29|Hw28)	
#define	HwPCLKCFG8_P1_ON		Hw27			
#define	HwPCLKCFG8_P1_OFF		~Hw27			
//CIF Master Clock
#define	HwPCLKCFG8_EN0_ON		Hw15		
#define	HwPCLKCFG8_EN0_OFF		~Hw15		
#define	HwPCLKCFG8_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG8_SEL0_DIVPLL0	        Hw13		
#define	HwPCLKCFG8_SEL0_XIN		Hw14		
#define	HwPCLKCFG8_SEL0_DIVXIN	        (Hw14|Hw12)	
#define	HwPCLKCFG8_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG8_SEL0_DIVXTIN	        (Hw14|Hw13|Hw12)
#define	HwPCLKCFG8_P0_ON		Hw11		
#define	HwPCLKCFG8_P0_OFF		~Hw11	

// R/W, LCD Controller Clock 
#define	HwPCLKCFG9		        *(volatile unsigned long *)0xF1006040	
#define	HwPCLKCFG9_EN1_ON		Hw31			
#define	HwPCLKCFG9_EN1_OFF		~Hw31			
#define	HwPCLKCFG9_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG9_SEL1_DIVPLL0	        Hw29			
#define	HwPCLKCFG9_SEL1_XIN		Hw30			
#define	HwPCLKCFG9_SEL1_DIVXIN	        (Hw30|Hw28)		
#define	HwPCLKCFG9_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG9_SEL1_DIVXTIN	        (Hw30|Hw29|Hw28)	
#define	HwPCLKCFG9_P1_ON		Hw27			
#define	HwPCLKCFG9_P1_OFF		~Hw27	
// HPI Clock Controller
#define	HwPCLKCFG9_EN0_ON			Hw15		
#define	HwPCLKCFG9_EN0_OFF		~Hw15		
#define	HwPCLKCFG9_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG9_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG9_SEL0_XIN		Hw14		
#define	HwPCLKCFG9_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG9_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG9_SEL0_DIVXTIN	 (Hw14|Hw13|Hw12)
#define	HwPCLKCFG9_P0_ON			Hw11		
#define	HwPCLKCFG9_P0_OFF			~Hw11	

// UART2/3 Peri-Clock Configuration Register 
//Channel 3
#define	HwPCLKCFG10		        *(volatile unsigned long *)0xF1006044
#define	HwPCLKCFG10_EN1_ON		Hw31			
#define	HwPCLKCFG10_EN1_OFF		~Hw31			
#define	HwPCLKCFG10_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG10_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG10_SEL1_XIN		Hw30			
#define	HwPCLKCFG10_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG10_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG10_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG10_P1_ON			Hw27			
#define	HwPCLKCFG10_P1_OFF		~Hw27			
//Channel 2
#define	HwPCLKCFG10_EN0_ON		Hw15		
#define	HwPCLKCFG10_EN0_OFF		~Hw15		
#define	HwPCLKCFG10_SEL0_PLL0		HwZERO		
#define	HwPCLKCFG10_SEL0_DIVPLL0	Hw13		
#define	HwPCLKCFG10_SEL0_XIN		Hw14		
#define	HwPCLKCFG10_SEL0_DIVXIN	(Hw14|Hw12)	
#define	HwPCLKCFG10_SEL0_XTIN		(Hw14|Hw13)	
#define	HwPCLKCFG10_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)
#define	HwPCLKCFG10_P0_ON			Hw11		
#define	HwPCLKCFG10_P0_OFF		~Hw11		

// SPDIF Peri-Clock Configuration Register 
#define	HwPCLKCFG11		        *(volatile unsigned long *)0xF1006048
#define	HwPCLKCFG11_EN1_ON		Hw31			
#define	HwPCLKCFG11_EN1_OFF		~Hw31			
#define	HwPCLKCFG11_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG11_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG11_SEL1_XIN		Hw30			
#define	HwPCLKCFG11_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG11_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG11_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG11_P1_ON			Hw27			
#define	HwPCLKCFG11_P1_OFF		~Hw27			

// USBH Peri-Clock Configuration Register 
#define	HwPCLKCFG12		        *(volatile unsigned long *)0xF100604C
#define	HwPCLKCFG12_EN1_ON		Hw31			
#define	HwPCLKCFG12_EN1_OFF		~Hw31			
#define	HwPCLKCFG12_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG12_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG12_SEL1_XIN		Hw30			
#define	HwPCLKCFG12_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG12_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG12_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG12_P1_ON			Hw27			
#define	HwPCLKCFG12_P1_OFF		~Hw27

// USBH Peri-Clock Configuration Register 
#define	HwPCLKCFG13		        *(volatile unsigned long *)0xF1006050
//#define	HwPCLK_SDMMC		        *(volatile unsigned long *)0xF1006050
#define	HwPCLKCFG13_EN1_ON		Hw31			
#define	HwPCLKCFG13_EN1_OFF		~Hw31			
#define	HwPCLKCFG13_SEL1_PLL0		HwZERO			
#define	HwPCLKCFG13_SEL1_DIVPLL0	Hw29			
#define	HwPCLKCFG13_SEL1_XIN		Hw30			
#define	HwPCLKCFG13_SEL1_DIVXIN	(Hw30|Hw28)		
#define	HwPCLKCFG13_SEL1_XTIN		(Hw30|Hw29)		
#define	HwPCLKCFG13_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	
#define	HwPCLKCFG13_P1_ON			Hw27			
#define	HwPCLKCFG13_P1_OFF		~Hw27			

//To Do...
#define	HwPCLKCFG_EN1_ON			Hw31				// SDRAM Refresh Clock enable
#define	HwPCLKCFG_SEL1_PLL0		HwZERO							// SDRAM Direct output from PLL0
#define	HwPCLKCFG_SEL1_DIVPLL0	Hw29				// SDRAM Divided output from PLL0
#define	HwPCLKCFG_SEL1_XIN		Hw30				// SDRAM XIN from external main oscillator
#define	HwPCLKCFG_SEL1_DIVXIN	(Hw30|Hw28)			// SDRAM Divided outpur from XIN
#define	HwPCLKCFG_SEL1_XTIN		(Hw30|Hw29)			// SDRAM XTIN from external sub oscillator
#define	HwPCLKCFG_SEL1_DIVXTIN	(Hw30|Hw29|Hw28)	// SDRAM Divided output form XTIN
#define	HwPCLKCFG_P1_ON			Hw27				// SDRAM Post divider enable
#define	HwPCLKCFG_EN0_ON			Hw15				// USB Host Clock enable
#define	HwPCLKCFG_SEL0_PLL0		HwZERO							// USB Host Direct output from PLL0
#define	HwPCLKCFG_SEL0_DIVPLL0	Hw13				// USB Host Divided output from PLL0
#define	HwPCLKCFG_SEL0_XIN		Hw14				// USB Host XIN from external main oscillator
#define	HwPCLKCFG_SEL0_DIVXIN	(Hw14|Hw12)			// USB Host Divided outpur from XIN
#define	HwPCLKCFG_SEL0_XTIN		(Hw14|Hw13)			// USB Host XTIN from external sub oscillator
#define	HwPCLKCFG_SEL0_DIVXTIN	(Hw14|Hw13|Hw12)	// USB Host Divided output form XTIN
#define	HwPCLKCFG_P0_ON			Hw11				// USB Host Post divider enable

#define	HwPCLKCFG_H				16					// Upper Half of Register
#define	HwPCLKCFG_L				HwZERO							// Lower Half of Register
#define	HwPCLKCFG_EN_ON			Hw15				// Peripheral Clock enable
#define	HwPCLKCFG_SEL_PLL0		HwZERO							// Peripheral Direct output from PLL0
#define	HwPCLKCFG_SEL_DIVPLL0	Hw13				// Peripheral Divided output from PLL0
#define	HwPCLKCFG_SEL_PLL1		Hw12							// Peripheral Direct output from PLL0
#define	HwPCLKCFG_SEL_DIVPLL1	Hw29				// Peripheral Divided output from PLL0
#define	HwPCLKCFG_SEL_XIN			Hw14				// Peripheral XIN from external main oscillator
#define	HwPCLKCFG_SEL_DIVXIN		(Hw14|Hw12)			// Peripheral Divided outpur from XIN
#define	HwPCLKCFG_SEL_XTIN		(Hw14|Hw13)			// Peripheral XTIN from external sub oscillator
#define	HwPCLKCFG_SEL_DIVXTIN	(Hw14|Hw13|Hw12)	// Peripheral Divided output form XTIN
#define	HwPCLKCFG_POST_ON		Hw11				// Peripheral Post divider enable
//To Do...

#define	HwBCLKCTR1					*(volatile unsigned long *)0xF1006060	// R/W, Bus Clock Enable Register
#define	HwBCLKCTR1_CHIPID_ON			Hw4			
#define	HwBCLKCTR1_CHIPID_OFF			~Hw4			
#define	HwBCLKCTR1_SCFG_ON			Hw3		
#define	HwBCLKCTR1_SCFG_OFF			~Hw3			
#define	HwBCLKCTR1_SPDIFTx_ON			Hw2		
#define	HwBCLKCTR1_SPDIFTx_OFF		~Hw2			
#define	HwBCLKCTR1_AudioDMA_ON		Hw1	
#define	HwBCLKCTR1_AudioDMA_OFF		~Hw1			
#define	HwBCLKCTR1_BitrstreamDMA_ON	Hw0	
#define	HwBCLKCTR1_BitrstreamDMA_OFF	~Hw0	

#define	HwSWRESET1				*(volatile unsigned long *)0xF1006064	// R/W, UART Clock
#define	HwSWRESET1_CHIPID_ON			Hw4			
#define	HwSWRESET1_CHIPID_OFF			~Hw4			
#define	HwSWRESET1_SCFG_ON			Hw3		
#define	HwSWRESET1_SCFG_OFF			~Hw3			
#define	HwSWRESET1_SPDIFTx_ON		Hw2		
#define	HwSWRESET1_SPDIFTx_OFF		~Hw2			
#define	HwSWRESET1_AUDIODMA_ON		Hw1	
#define	HwSWRESET1_AUDIODMA_OFF		~Hw1			
#define	HwSWRESET1_BitrstreamDMA_ON	Hw0	
#define	HwSWRESET1_BitrstreamDMA_OFF	~Hw0	
//Done....
//////////

/***********************************************************************
*	UART0, UART1 UART2, UART3 Controller Register Define				(Base Addr = 0xF0005000)
************************************************************************/
//////////
//Done....

#define	HwUART0_RBR	*(volatile unsigned long *)0xF1007000	// R, Receiver Buffer Register (DLAB = 0)
#define	HwUART1_RBR	*(volatile unsigned long *)0xF1008000	// R, Receiver Buffer Register (DLAB = 0)
#define	HwUART2_RBR	*(volatile unsigned long *)0xF1009000	// R, Receiver Buffer Register (DLAB = 0)
#define	HwUART3_RBR	*(volatile unsigned long *)0xF100A000	// R, Receiver Buffer Register (DLAB = 0)

#define	HwUART0_THR	*(volatile unsigned long *)0xF1007000	// W, Transmitter Holding Register (DLAB = 0)
#define	HwUART1_THR	*(volatile unsigned long *)0xF1008000	// W, Transmitter Holding Register (DLAB = 0)
#define	HwUART2_THR	*(volatile unsigned long *)0xF1009000	// W, Transmitter Holding Register (DLAB = 0)
#define	HwUART3_THR	*(volatile unsigned long *)0xF100A000	// W, Transmitter Holding Register (DLAB = 0)

#define	HwUART0_DLL	*(volatile unsigned long *)0xF1007000	// W, Divisor Latch Register (DLAB = 1)
#define	HwUART1_DLL	*(volatile unsigned long *)0xF1008000	// W, Divisor Latch Register (DLAB = 1)
#define	HwUART2_DLL	*(volatile unsigned long *)0xF1009000	// W, Divisor Latch Register (DLAB = 1)
#define	HwUART3_DLL	*(volatile unsigned long *)0xF100A000	// W, Divisor Latch Register (DLAB = 1)

#define	HwUART0_DLM	*(volatile unsigned long *)0xF1007004	// W, Divisor Latch Register (DLAB = 1)
#define	HwUART1_DLM	*(volatile unsigned long *)0xF1008004	// W, Divisor Latch Register (DLAB = 1)
#define	HwUART2_DLM	*(volatile unsigned long *)0xF1009004	// W, Divisor Latch Register (DLAB = 1)
#define	HwUART3_DLM	*(volatile unsigned long *)0xF100a004	// W, Divisor Latch Register (DLAB = 1)

#define	HwUART0_IER	*(volatile unsigned long *)0xF1007004	// W, Interrupt Enable Register (DLAB = 0)
#define	HwUART1_IER	*(volatile unsigned long *)0xF1008004	// W, Interrupt Enable Register (DLAB = 0)
#define	HwUART2_IER	*(volatile unsigned long *)0xF1009004	// W, Interrupt Enable Register (DLAB = 0)
#define	HwUART3_IER	*(volatile unsigned long *)0xF100a004	// W, Interrupt Enable Register (DLAB = 0)
#define	HwUART_IER_EMSI_EN		Hw3					// Enable Modem status interrupt
#define	HwUART_IER_EMSI_DIS		~Hw3				// Disable Modem status interrupt
#define	HwUART_IER_ELSI_EN		Hw2					// Enable receiver line status interrupt
#define	HwUART_IER_ELSI_DIS		~Hw2				// Disable receiver line status interrupt
#define	HwUART_IER_ETXI_EN		Hw1					// Enable transmitter holding register empty interrupt
#define	HwUART_IER_ETXI_DIS		~Hw1				// Disable transmitter holding register empty interrupt
#define	HwUART_IER_ERXI_EN		Hw0					// Enable received data available interrupt
#define	HwUART_IER_ERXI_DIS		~Hw0				// Disable received data available interrupt

#define	HwUART0_IIR	*(volatile unsigned long *)0xF1007008	// R, Interrupt Ident. Register
#define	HwUART1_IIR	*(volatile unsigned long *)0xF1008008	// R, Interrupt Ident. Register
#define	HwUART2_IIR	*(volatile unsigned long *)0xF1009008	// R, Interrupt Ident. Register
#define	HwUART3_IIR	*(volatile unsigned long *)0xF100a008	// R, Interrupt Ident. Register

#define	HwUART0_FCR	*(volatile unsigned long *)0xF1007008	// W, FIFO Control Register
#define	HwUART1_FCR	*(volatile unsigned long *)0xF1008008	// W, FIFO Control Register
#define	HwUART2_FCR	*(volatile unsigned long *)0xF1009008	// W, FIFO Control Register
#define	HwUART3_FCR	*(volatile unsigned long *)0xF100a008	// W, FIFO Control Register
#define	HwUART_FCR_RXT_1		~(Hw7|Hw6)			// RX FIFO Trigger Level 1byte
#define	HwUART_FCR_RXT_4		Hw6					// RX FIFO Trigger Level 4bytes
#define	HwUART_FCR_RXT_8		Hw7					// RX FIFO Trigger Level 8bytes
#define	HwUART_FCR_RXT_14		(Hw7|Hw6)			// RX FIFO Trigger Level 16bytes
#define	HwUART_FCR_DMS_EN		Hw3					// DMA Mode Enabled
#define	HwUART_FCR_DMS_DIS		~Hw3				// DMA Mode Disabled
#define	HwUART_FCR_TXFR_EN		Hw2					// Reset TX FIFO counter and FIFO Data
#define	HwUART_FCR_TXFR_DIS		~Hw2				//
#define	HwUART_FCR_RXFR_EN		Hw1					// Reset RX FIFO counter and FIFO Data
#define	HwUART_FCR_RXFR_DIS		~Hw1				//
#define	HwUART_FCR_FE_EN		Hw0					// Enable TX and RX FIFOs
#define	HwUART_FCR_FE_DIS		~Hw0				// Disable TX and RX FIFOs

#define	HwUART0_LCR	*(volatile unsigned long *)0xF100700C	// R, Line Control Register
#define	HwUART1_LCR	*(volatile unsigned long *)0xF100800C	// R, Line Control Register
#define	HwUART2_LCR	*(volatile unsigned long *)0xF100900C	// R, Line Control Register
#define	HwUART3_LCR	*(volatile unsigned long *)0xF100a00C	// R, Line Control Register
#define	HwUART_LCR_DLAB_ON		Hw7					// Access the divisor latches of the baud generator
#define	HwUART_LCR_DLAB_OFF		~Hw7				// Access the receiver buff, the transmitter holding register, or the interrupt enable register
#define	HwUART_LCR_SB_ON		Hw6					// The serial output is forced to the spacing (logic 0) state
#define	HwUART_LCR_SB_OFF		~Hw6				// Disable the break
#define	HwUART_LCR_SP_ON		Hw5					// When bits 3, 4 and 5 are logic 1 the parity bits is transmitted and checked as a logic 0. If bits 3 and 5 are 1 and bit 4 is a logic 0 then the parity bit is transmitted and checked as a logic 1
#define	HwUART_LCR_SP_OFF		~Hw5				// Disable stick parity
#define	HwUART_LCR_EPS_EVEN		Hw4					// Generate or check even parity
#define	HwUART_LCR_EPS_ODD		~Hw4				// Generate or check odd parity
#define	HwUART_LCR_PEN_ON		Hw3					// A parity bit is generated (TX) or Checked (RX)
#define	HwUART_LCR_PEN_OFF		~Hw3				// Disabled
#define	HwUART_LCR_STB_ONE		Hw2					// One stop bit is generated in the transmitted data
#define	HwUART_LCR_STB_WLS		~Hw2				// When 5bit word length is selected, one and a half stop bits are generated. When either a 6-, 7-, or 8-bit word length is selected, two stop bits are generated.
#define	HwUART_LCR_WLS_5		~(Hw1|Hw0)			// 5 bits word length
#define	HwUART_LCR_WLS_6		Hw0					// 6 bits word length
#define	HwUART_LCR_WLS_7		Hw1					// 7 bits word length
#define	HwUART_LCR_WLS_8		(Hw1|Hw0)			// 8 bits word length

#define	HwUART0_MCR	*(volatile unsigned long *)0xF1007010	// R/W, MODEM Control Register
#define	HwUART1_MCR	*(volatile unsigned long *)0xF1008010	// R/W, MODEM Control Register
#define	HwUART2_MCR	*(volatile unsigned long *)0xF1009010	// R/W, MODEM Control Register
#define	HwUART3_MCR	*(volatile unsigned long *)0xF100a010	// R/W, MODEM Control Register
#define HwUART_MCR_RTS_DCC_ON		Hw6
#define HwUART_MCR_RTS_DCC_OFF		~Hw6
#define HwUART_MCR_HW_AUTOFLOW_ON	Hw5
#define HwUART_MCR_HW_AUTOFLOW_OFF	~Hw5
#define	HwUART_MCR_LOOP_ON		Hw4					//
#define	HwUART_MCR_LOOP_OFF	        ~Hw4				//
#define	HwUART_MCR_RTS_ON			Hw1					
#define	HwUART_MCR_RTS_OFF			~Hw1				


#define	HwUART0_LSR	*(volatile unsigned long *)0xF1007014	// R/W, Line Status Register
#define	HwUART1_LSR	*(volatile unsigned long *)0xF1008014	// R/W, Line Status Register
#define	HwUART2_LSR	*(volatile unsigned long *)0xF1009014	// R/W, Line Status Register
#define	HwUART3_LSR	*(volatile unsigned long *)0xF100A014	// R/W, Line Status Register
#define	HwUART_LSR_ERF_FIFO		Hw7					// In the FIFO mode this bit is set when there is at least one parity error, framing error or break indication in the FIFO
#define	HwUART_LSR_ERF_16450	        ~Hw7				// In the 16450 mode
#define	HwUART_LSR_TEMT_ON		Hw6					// Transmitter holding register and the transmitter shift register are both empty
#define	HwUART_LSR_TEMT_OFF		~Hw6				//
#define	HwUART_LSR_THRE_ON		Hw5					// UART is ready to accept a new char for transmission
#define	HwUART_LSR_THRE_OFF		~Hw5				//
#define	HwUART_LSR_BI_ON		Hw4					// The received data input is held in the spacing (logic 0) state for longer than a full word transmission time
#define	HwUART_LSR_BI_OFF		~Hw4				//
#define	HwUART_LSR_FE_ON		Hw3					// The received character did not have a valid stop bit
#define	HwUART_LSR_FE_OFF		~Hw3				//
#define	HwUART_LSR_PE_ON		Hw2					// The received data character does not have the correct even or odd parity
#define	HwUART_LSR_PE_OFF		~Hw2				//
#define	HwUART_LSR_OE_ON		Hw1					// The receiver buffer register was not read by the CPU before the next character was transferred into the receiver buffer register
#define	HwUART_LSR_OE_OFF		~Hw1				//
#define	HwUART_LSR_DR_ON		Hw0					// The receiver data ready
#define	HwUART_LSR_DR_OFF		~Hw0				//

#define	HwUART0_MSR	*(volatile unsigned long *)0xF1007018	// R/W, MODEM Status Register
#define	HwUART1_MSR	*(volatile unsigned long *)0xF1008018	// R/W, MODEM Status Register
#define	HwUART2_MSR	*(volatile unsigned long *)0xF1009018	// R/W, MODEM Status Register
#define	HwUART3_MSR	*(volatile unsigned long *)0xF100A018	// R/W, MODEM Status Register
#define HwUART_MSR_CTS_ON		Hw4
#define HwUART_MSR_CTS_OFF		~Hw4
#define HwUART_MSR_DCTS_ON		Hw0
#define HwUART_MSR_DTS_OFF		~Hw0

#define	HwUART0_SCR	*(volatile unsigned long *)0xF100701C	// R/W, Scratch Register
#define	HwUART1_SCR	*(volatile unsigned long *)0xF100801C	// R/W, Scratch Register
#define	HwUART2_SCR	*(volatile unsigned long *)0xF100901C	// R/W, Scratch Register
#define	HwUART3_SCR	*(volatile unsigned long *)0xF100A01C	// R/W, Scratch Register

#define	HwUART0_AFT	*(volatile unsigned long *)0xF1007020	// AFC trigger level register [NEW]
#define	HwUART1_AFT	*(volatile unsigned long *)0xF1008020	// AFC trigger level register [NEW]
#define	HwUART2_AFT	*(volatile unsigned long *)0xF1009020	// AFC trigger level register [NEW]
#define	HwUART3_AFT	*(volatile unsigned long *)0xF100a020	// AFC trigger level register [NEW]

#define	HwUART0_UCR	*(volatile unsigned long *)0xF1007024	// UART1 control register
#define	HwUART1_UCR	*(volatile unsigned long *)0xF1008024	// UART1 control register
#define	HwUART2_UCR	*(volatile unsigned long *)0xF1009024	// UART1 control register
#define	HwUART3_UCR	*(volatile unsigned long *)0xF100A024	// UART1 control register
#define	HwUART_UCR_RWA		Hw3						// Rx fifo access to byte/word [ 0:byte, 1:word]
#define	HwUART_UCR_TWA		Hw2						// Tx fifo access to byte/word [ 0:byte, 1:word]
#define	HwUART_UCR_RXDE		Hw1					// Rx DMA enable
#define	HwUART_UCR_TXDE		Hw0					// Tx DMA enable

///////////////////
// SRBR, STHR, SDLL, SIER reg. are copy of the RBR, THR, DLL, DLM, IER reg. 
// These reg. can be accessed without concern of DLAB state
//////////////////

#define HwUART0_SRBR	*(volatile unsigned long *)0xF1007040			// [ReadOnly] Uart1 receiver buffer register
#define HwUART1_SRBR	*(volatile unsigned long *)0xF1008040			// [ReadOnly] Uart1 receiver buffer register
#define HwUART2_SRBR	*(volatile unsigned long *)0xF1009040			// [ReadOnly] Uart1 receiver buffer register
#define HwUART3_SRBR	*(volatile unsigned long *)0xF100A040			// [ReadOnly] Uart1 receiver buffer register


#define HwUART0_STHR	*(volatile unsigned long *)0xF1007044			// [WrtieOnly] Uart1 transmitter holding register
#define HwUART1_STHR	*(volatile unsigned long *)0xF1008044			// [WrtieOnly] Uart1 transmitter holding register
#define HwUART2_STHR	*(volatile unsigned long *)0xF1009044			// [WrtieOnly] Uart1 transmitter holding register
#define HwUART3_STHR	*(volatile unsigned long *)0xF100A044			// [WrtieOnly] Uart1 transmitter holding register

#define HwUART0_SDLL	*(volatile unsigned long *)0xF1007048			// Uart1 Divisor latch register
#define HwUART1_SDLL	*(volatile unsigned long *)0xF1008048			// Uart1 Divisor latch register
#define HwUART2_SDLL	*(volatile unsigned long *)0xF1009048			// Uart1 Divisor latch register
#define HwUART3_SDLL	*(volatile unsigned long *)0xF100A048			// Uart1 Divisor latch register

#define HwUART0_SDLM	*(volatile unsigned long *)0xF100704C			// Uart1 Divisor latch register
#define HwUART1_SDLM	*(volatile unsigned long *)0xF100804C			// Uart1 Divisor latch register
#define HwUART2_SDLM	*(volatile unsigned long *)0xF100904C			// Uart1 Divisor latch register
#define HwUART3_SDLM	*(volatile unsigned long *)0xF100A04C			// Uart1 Divisor latch register

#define HwUART0_SIER	*(volatile unsigned long *)0xF1007050			// Uart1 Interrupt register
#define HwUART1_SIER	*(volatile unsigned long *)0xF1008050			// Uart1 Interrupt register
#define HwUART2_SIER	*(volatile unsigned long *)0xF1009050			// Uart1 Interrupt register
#define HwUART3_SIER	*(volatile unsigned long *)0xF100A050			// Uart1 Interrupt register

#define HwUART0_SCCR	*(volatile unsigned long *)0xF1007060
#define HwUART1_SCCR	*(volatile unsigned long *)0xF1008060
#define HwUART2_SCCR	*(volatile unsigned long *)0xF1009060
#define HwUART3_SCCR	*(volatile unsigned long *)0xF100A060

#define HwUART0_STC		*(volatile unsigned long *)0xF1007064
#define HwUART1_STC		*(volatile unsigned long *)0xF1008064
#define HwUART2_STC		*(volatile unsigned long *)0xF1009064
#define HwUART3_STC		*(volatile unsigned long *)0xF100A064

#define HwUART0_IRCFG	*(volatile unsigned long *)0xF1007080			
#define HwUART1_IRCFG	*(volatile unsigned long *)0xF1008080			
#define HwUART2_IRCFG	*(volatile unsigned long *)0xF1009080			
#define HwUART3_IRCFG	*(volatile unsigned long *)0xF100A080			
//Done....
//////////

/************************************************************************
*	GSIO/SDO/SPI Port Register Define					(Base Addr = 0xF1003000)
************************************************************************/
/////////
//Done...
#define	HwGSDO0		*(volatile unsigned long *)0xF1003000	// R/W, GSIO0 Output Data Register
#define	HwSDO0		*(volatile unsigned long *)0xF1003000	// R/W, GSIO0 Output Data Register

#define	HwGSDI0		*(volatile unsigned long *)0xF1003004	// R/W, GSIO0 Input Data Register
#define	HwSDI0		*(volatile unsigned long *)0xF1003004	// R/W, GSIO0 Input Data Register

//#define	HwSBCR		*(volatile unsigned long *)0xF1003008	

#define	HwGSCR0		*(volatile unsigned long *)0xF100300C    // R/W, GSIO0 Control Register
#define	HwSCR0		*(volatile unsigned long *)0xF100300C    // R/W, GSIO0 Control Register
//To Do...

//#define	HwSPCTRL	*(volatile unsigned long *)0xF1003010	// R/W, SPI Interrupt Control Register
//To Do...

//#define	HwSPSTS  	*(volatile unsigned long *)0xF1003014	// R/W, SPI Interrupt Status Register
//To Do...

//Done...
/////////

/***************************************************************************************
*    SPI0 Controller Register Define                        (Base Addr = 0x9000C000)
*
****************************************************************************************/
#define HwSDO       *(volatile unsigned long *)0xF100C000   // R/W, SPI0 Output Data Register

#define HwSDI       *(volatile unsigned long *)0xF100C004   // R/W, SPI0 Input Data Register

#define HwSBCR  *(volatile unsigned long *)0xF100C008// R/W, SPI0 Base Clock Register

#define HwSPCR      *(volatile unsigned long *)0xF100C00C// R/W, SPI0 Control Register
#define HwSPCR_EN_ON                Hw31            // Enable SPI
#define HwSPCR_EN_OFF               ~Hw31           // Disable SPI
#define HwSPCR_MS_MASTER            Hw30            // SPI works as Master
#define HwSPCR_MS_SLAVE         ~Hw30           // SPI works as Slave
#define HwSPCR_CPL_RISE         Hw29            // Starts transfer a half SCK later after /SS is low
#define HwSPCR_CPL_FALL         ~Hw29           // Starts transfer as soon as /SS is low
#define HwSPCR_CPH_HIGH         Hw28            // Starts transfer a half SCK later after /SS is low
#define HwSPCR_CPH_LOW          ~Hw28           // Starts transfer as soon as /SS is low
#define HwSPCR_ML_MSB               Hw27            // Send MSB firstly
#define HwSPCR_ML_LSB               ~Hw27           // Send LSB firstly
#define HwSPCR_TOE_EN               Hw26            // TDO is enabled or disabled depending on asserting or disasserting of /SS.
#define HwSPCR_TOE_DIS          ~Hw26           // TDO is always disabled.
#define HwSPCR_EDN_BIG          Hw25            // Big Endian transfer mode
#define HwSPCR_EDN_LIT          ~Hw25           // Little Endian transfer mode
#define HwSPCR_CON_CON          Hw24            // Run in the continuous transfer mode
#define HwSPCR_CON_SIN          ~Hw24           // Run in the single transfer mode
#define HwSPCR_TMOD_EN          Hw14            // Make Tx trigger depending on the status of Tx FIFO
#define HwSPCR_TMOD_DIS         ~Hw14           // Don't make Tx trigger
#define HwSPCR_EFT_EN               Hw13            // Enable External Frame Trigger.
#define HwSPCR_EFT_DIS          ~Hw13           // Disable External Frame Trigger
#define HwSPCR_FP_HIGH          Hw12            // FRM has high active pulse.
#define HwSPCR_FP_LOW               ~Hw12           // FRM has low active pulse
#define HwSPCR_FX_EN                Hw11            // Extends head and tail of FRM signal by one cycle of SCK
#define HwSPCR_FX_DIS               ~Hw11           // Don't extend FRM signal
#define HwSPCR_CRF_EMPTY            Hw10            // Empties Rx FIFO
#define HwSPCR_CRF_NOEMPTY      ~Hw10           // Don't empty Rx FIFO
#define HwSPCR_CTF_EMPTY            Hw9             // Empties Tx FIFO
#define HwSPCR_CTF_NOEMPTY      ~Hw9            // Don't empty Tx FIFO
#define HwSPCR_TRGPOS_TAIL      Hw8             // Regard tail of frame signal as start point.
#define HwSPCR_TRGPOS_HEAD      ~Hw8            // Regard head of frame signal as start point
#define HwSPCR_SDI                  Hw7             // As soon as the frame signal is detected, SDI is captured

#define HwSPFRMPOS  *(volatile unsigned long *)0xF100C010   // R/W, SPI0 Frame Signal Position Register

#define HwSPCTRL        *(volatile unsigned long *)0xF100C014   // R/W, SPI0 Interrupt Control Register
#define HwSPCTRL_RCFULEN_EN     Hw12            // Enable RX FIFO count full interrupt
#define HwSPCTRL_RCFULEN_DIS    ~Hw12           // Disable RX FIFO count full interrupt
#define HwSPCTRL_REMPEN_EN      Hw11            // Enable RX FIFO empty interrupt
#define HwSPCTRL_REMPEN_DIS     ~Hw11           // Disable RX FIFO empty interrupt
#define HwSPCTRL_RFULEN_EN      Hw10            // Enable RX FIFO full interrupt
#define HwSPCTRL_RFULEN_DIS     ~Hw10           // Disable RX FIFO full interrupt
#define HwSPCTRL_ROREN_EN       Hw9             // Enable RX FIFO overrun error interrupt
#define HwSPCTRL_ROREN_DIS      ~Hw9            // Disable RX FIFO overrun error interrupt
#define HwSPCTRL_RUREN_EN       Hw8             // Enable RX FIFO underrun error interrupt
#define HwSPCTRL_RUREN_DIS      ~Hw8            // Disable RX FIFO underrun error interrupt
#define HwSPCTRL_TCEMPEN_EN     Hw3             // Enable TX FIFO count empty interrupt
#define HwSPCTRL_TCEMPEN_DIS    ~Hw3            // Disable TX FIFO count empty interrupt
#define HwSPCTRL_TFULEN_EN      Hw2             // Enable TX FIFO full interrupt
#define HwSPCTRL_TFULEN_DIS     ~Hw2            // Disable TX FIFO full interrupt
#define HwSPCTRL_TOREN_EN       Hw1             // Enable RX FIFO overrun error interrupt
#define HwSPCTRL_TOREN_DIS      ~Hw1            // Disable RX FIFO overrun error interrupt
#define HwSPCTRL_TUREN_EN       Hw0             // Enable RX FIFO underrun error interrupt
#define HwSPCTRL_TUREN_DIS      ~Hw0            // Disable RX FIFO underrun error interrupt

#define HwSPSTS     *(volatile unsigned long *)0xF100C018   // R/W, SPI0 Interrupt Status Register
#define HwSPSTS_RCFUL           Hw12            // Rx FIFO count full status flag(read only)
#define HwSPSTS_REMP                Hw11            // Rx FIFO empty status flag (read only)
#define HwSPSTS_RFUL                Hw10            // Rx FIFO full status flag (read only)
#define HwSPSTS_ROR             Hw9             // Rx FIFO overrun error status flag (read only)
#define HwSPSTS_RUR             Hw8             // Rx FIFO underrun error status flag (read only)
#define HwSPSTS_TCEMP           Hw4             // Tx FIFO count empty status flag (read only)
#define HwSPSTS_TEMP                Hw3             // Tx FIFO empty status flag (read only)
#define HwSPSTS_TFUL                Hw2             // Tx FIFO full status flag (read only)
#define HwSPSTS_TOR             Hw1             // Tx FIFO overrun error status flag (read only)
#define HwSPSTS_TUR             Hw0             // Tx FIFO underrun error status flag (read only)

/***************************************************************************************
*    SPI1 Controller Register Define                        (Base Addr = 0x9000E000)
*
****************************************************************************************/
#define HwSDO1      *(volatile unsigned long *)0xF100E000   // R/W, SPI1 Output Data Register

#define HwSDI1      *(volatile unsigned long *)0xF100E004   // R/W, SPI1 Input Data Register

#define HwSBCR1 *(volatile unsigned long *)0xF100E008// R/W, SPI1 Base Clock Register

#define HwSPCR1     *(volatile unsigned long *)0xF100E00C// R/W, SPI1 Control Register
#define HwSPCR1_EN_ON               Hw31            // Enable SPI
#define HwSPCR1_EN_OFF              ~Hw31           // Disable SPI
#define HwSPCR1_MS_MASTER           Hw30            // SPI works as Master
#define HwSPCR1_MS_SLAVE            ~Hw30           // SPI works as Slave
#define HwSPCR1_CPL_HALF            Hw29            // Starts transfer a half SCK later after /SS is low
#define HwSPCR1_CPL_SOON            ~Hw29           // Starts transfer as soon as /SS is low
#define HwSPCR1_CPH_HALF            Hw28            // Starts transfer a half SCK later after /SS is low
#define HwSPCR1_CPH_SOON            ~Hw28           // Starts transfer as soon as /SS is low
#define HwSPCR1_ML_MSB              Hw27            // Send MSB firstly
#define HwSPCR1_ML_LSB              ~Hw27           // Send LSB firstly
#define HwSPCR1_TOE_EN              Hw26            // TDO is enabled or disabled depending on asserting or disasserting of /SS.
#define HwSPCR1_TOE_DIS         ~Hw26           // TDO is always disabled.
#define HwSPCR1_EDN_BIG         Hw25            // Big Endian transfer mode
#define HwSPCR1_EDN_LIT         ~Hw25           // Little Endian transfer mode
#define HwSPCR1_CON_CON         Hw24            // Run in the continuous transfer mode
#define HwSPCR1_CON_SIN         ~Hw24           // Run in the single transfer mode
#define HwSPCR1_TMOD_EN         Hw14            // Make Tx trigger depending on the status of Tx FIFO
#define HwSPCR1_TMOD_DIS            ~Hw14           // Don't make Tx trigger
#define HwSPCR1_EFT_EN              Hw13            // Enable External Frame Trigger.
#define HwSPCR1_EFT_DIS         ~Hw13           // Disable External Frame Trigger
#define HwSPCR1_FP_HIGH         Hw12            // FRM has high active pulse.
#define HwSPCR1_FP_LOW              ~Hw12           // FRM has low active pulse
#define HwSPCR1_FX_EN               Hw11            // Extends head and tail of FRM signal by one cycle of SCK
#define HwSPCR1_FX_DIS              ~Hw11           // Don't extend FRM signal
#define HwSPCR1_CRF_EMPTY           Hw10            // Empties Rx FIFO
#define HwSPCR1_CRF_NOEMPTY     ~Hw10           // Don't empty Rx FIFO
#define HwSPCR1_CTF_EMPTY           Hw9             // Empties Tx FIFO
#define HwSPCR1_CTF_NOEMPTY     ~Hw9            // Don't empty Tx FIFO
#define HwSPCR1_TRGPOS_TAIL     Hw8             // Regard tail of frame signal as start point.
#define HwSPCR1_TRGPOS_HEAD     ~Hw8            // Regard head of frame signal as start point
#define HwSPCR1_SDI                 Hw7             // As soon as the frame signal is detected, SDI is captured

#define HwSPFRMPOS1 *(volatile unsigned long *)0xF100E010   // R/W, SPI1 Frame Signal Position Register

#define HwSPCTRL1       *(volatile unsigned long *)0xF100E014   // R/W, SPI1 Interrupt Control Register
#define HwSPCTRL1_RCFULEN_EN        Hw12            // Enable RX FIFO count full interrupt
#define     HwSPCTRL1_RCFULEN_DIS   ~Hw12           // Disable RX FIFO count full interrupt
#define HwSPCTRL1_REMPEN_EN     Hw11            // Enable RX FIFO empty interrupt
#define     HwSPCTRL1_REMPEN_DIS        ~Hw11           // Disable RX FIFO empty interrupt
#define HwSPCTRL1_RFULEN_EN     Hw10            // Enable RX FIFO full interrupt
#define     HwSPCTRL1_RFULEN_DIS        ~Hw10           // Disable RX FIFO full interrupt
#define HwSPCTRL1_ROREN_EN      Hw9             // Enable RX FIFO overrun error interrupt
#define     HwSPCTRL1_ROREN_DIS     ~Hw9            // Disable RX FIFO overrun error interrupt
#define HwSPCTRL1_RUREN_EN      Hw8             // Enable RX FIFO underrun error interrupt
#define     HwSPCTRL1_RUREN_DIS     ~Hw8            // Disable RX FIFO underrun error interrupt
#define HwSPCTRL1_TCEMPEN_EN        Hw3             // Enable TX FIFO count empty interrupt
#define     HwSPCTRL1_TCEMPEN_DIS   ~Hw3            // Disable TX FIFO count empty interrupt
#define HwSPCTRL1_TFULEN_EN     Hw2             // Enable TX FIFO full interrupt
#define     HwSPCTRL1_TFULEN_DIS        ~Hw2            // Disable TX FIFO full interrupt
#define HwSPCTRL1_TOREN_EN      Hw1             // Enable RX FIFO overrun error interrupt
#define     HwSPCTRL1_TOREN_DIS     ~Hw1            // Disable RX FIFO overrun error interrupt
#define HwSPCTRL1_TUREN_EN      Hw0             // Enable RX FIFO underrun error interrupt
#define     HwSPCTRL1_TUREN_DIS     ~Hw0            // Disable RX FIFO underrun error interrupt

#define HwSPSTS1        *(volatile unsigned long *)0xF100E018   // R/W, SPI1 Interrupt Status Register
#define     HwSPSTS1_RCFUL          Hw12            // Rx FIFO count full status flag(read only)
#define     HwSPSTS1_REMP               Hw11            // Rx FIFO empty status flag (read only)
#define     HwSPSTS1_RFUL               Hw10            // Rx FIFO full status flag (read only)
#define     HwSPSTS1_ROR                Hw9             // Rx FIFO overrun error status flag (read only)
#define     HwSPSTS1_RUR                Hw8             // Rx FIFO underrun error status flag (read only)
#define     HwSPSTS1_TCEMP          Hw4             // Tx FIFO count empty status flag (read only)
#define     HwSPSTS1_TEMP               Hw3             // Tx FIFO empty status flag (read only)
#define     HwSPSTS1_TFUL               Hw2             // Tx FIFO full status flag (read only)
#define     HwSPSTS1_TOR                Hw1             // Tx FIFO overrun error status flag (read only)
#define     HwSPSTS1_TUR                Hw0             // Tx FIFO underrun error status flag (read only)


/************************************************************************
*	I2C Register Define							(Base Addr = 0xF100B000)
***********************************************************************/
#define	HwI2CM_CH0_BASE					*(volatile unsigned long *)0xF100B000
#define	HwI2CM_CH1_BASE					*(volatile unsigned long *)0xF100B040


typedef volatile struct {
	unsigned	int PRES;
	unsigned	int CTRL;
	unsigned	int TXR;
	unsigned	int CMD;
	unsigned	int RXR;
	unsigned	int SR;
	unsigned	int TR;
} sHwI2CM;

//4 Controll Register
#define	HwI2CM_CTRL_EN_ON					Hw7										// Enabled
#define	HwI2CM_CTRL_EN_OFF					HwZERO									// Disabled
#define	HwI2CM_CTRL_IEN_ON					Hw6										// Enabled
#define	HwI2CM_CTRL_IEN_OFF					HwZERO									// Disabled
#define	HwI2CM_CTRL_MOD_16					Hw5										// 16bit Mode
#define	HwI2CM_CTRL_MOD_8					HwZERO									// 8bit Mode

//4 Command Register
#define	HwI2CM_CMD_STA_EN					Hw7 									// Start Condition Generation Enabled
#define	HwI2CM_CMD_STA_DIS				HwZERO 									// Start Condition Generation Disabled
#define	HwI2CM_CMD_STO_EN					Hw6 									// Stop Condition Generation Enabled
#define	HwI2CM_CMD_STO_DIS				HwZERO 									// Stop Condition Generation Disabled
#define	HwI2CM_CMD_RD_EN					Hw5 									// Read From Slave Enabled
#define	HwI2CM_CMD_RD_DIS					HwZERO 									// Read From Slave Disabled
#define	HwI2CM_CMD_WR_EN					Hw4 									// Wrtie to Slabe Enabled
#define	HwI2CM_CMD_WR_DIS					HwZERO 									// Wrtie to Slabe Disabled
#define	HwI2CM_CMD_ACK_EN					Hw3 									// Sent ACK Enabled
#define	HwI2CM_CMD_ACK_DIS				HwZERO 									// Sent ACK Disalbed
#define	HwI2CM_CMD_IACK_CLR				Hw0 									// Clear a pending interrupt

//4 Status Register
#define	HwI2CM_SR_RxACK					Hw7										// 1:Acknowledge received, 0:No Acknowledge received
#define	HwI2CM_SR_BUSY					Hw6										// 1:Start signal detected, 0:Stop signal detected
#define	HwI2CM_SR_AL						Hw5										// 1:The core lost arbitration, 0:The core don't lost arbitration
#define	HwI2CM_SR_TIP						Hw1										// 1:Transfer Complete, 0:Transferring Data
#define	HwI2CM_SR_IF						Hw0										// 1:Interrupt is pending

//4 Timing Register
#define	HwI2CM_TR_CKSEL					Hw5										// Clock Source Select


#define	HwI2CMCH0_PRES							*(volatile unsigned long *)0xF100B000	// R/W, I2C Clock Prescale Register
                                    		
#define	HwI2CMCH0_CTRL							*(volatile unsigned long *)0xF100B004	// R/W, I2C Control Register
#define	HwI2CMCH0_CTRL_EN_ON					Hw7										// Enabled
#define	HwI2CMCH0_CTRL_EN_OFF					~Hw7									// Disabled
#define	HwI2CMCH0_CTRL_IEN_ON					Hw6										// Enabled
#define	HwI2CMCH0_CTRL_IEN_OFF					~Hw6									// Disabled
#define	HwI2CMCH0_CTRL_MOD_16					Hw5										// 16bit Mode
#define	HwI2CMCH0_CTRL_MOD_8					~Hw5									// 8bit Mode
                                    		
#define	HwI2CMCH0_TXR							*(volatile unsigned long *)0xF100B008	// W, I2C Transmit Register
                                    		
#define	HwI2CMCH0_CMD							*(volatile unsigned long *)0xF100B00C	// W, I2C Command Register

#define	HwI2CMCH0_CMD_STA_EN					Hw7 									// Start Condition Generation Enabled
#define	HwI2CMCH0_CMD_STA_DIS					~Hw7 									// Start Condition Generation Disabled
#define	HwI2CMCH0_CMD_STO_EN					Hw6 									// Stop Condition Generation Enabled
#define	HwI2CMCH0_CMD_STO_DIS					~Hw6 									// Stop Condition Generation Disabled
#define	HwI2CMCH0_CMD_RD_EN						Hw5 									// Read From Slave Enabled
#define	HwI2CMCH0_CMD_RD_DIS					~Hw5 									// Read From Slave Disabled
#define	HwI2CMCH0_CMD_WR_EN						Hw4 									// Wrtie to Slabe Enabled
#define	HwI2CMCH0_CMD_WR_DIS					~Hw4 									// Wrtie to Slabe Disabled
#define	HwI2CMCH0_CMD_ACK_EN					Hw3 									// Sent ACK Enabled
#define	HwI2CMCH0_CMD_ACK_DIS					~Hw3 									// Sent ACK Disalbed
#define	HwI2CMCH0_CMD_IACK_CLR					Hw0 									// Clear a pending interrupt
                                    		
#define	HwI2CMCH0_RXR							*(volatile unsigned long *)0xF100B010	// R, I2C Receive Register
                                    		
#define	HwI2CMCH0_SR							*(volatile unsigned long *)0xF100B014	// R, I2C Status Register
#define	HwI2CMCH0_SR_RxACK						Hw7										// 1:Acknowledge received, 0:No Acknowledge received
#define	HwI2CMCH0_SR_BUSY						Hw6										// 1:Start signal detected, 0:Stop signal detected
#define	HwI2CMCH0_SR_AL							Hw5										// 1:The core lost arbitration, 0:The core don't lost arbitration
#define	HwI2CMCH0_SR_TIP						Hw1										// 1:Transfer Complete, 0:Transferring Data
#define	HwI2CMCH0_SR_IF							Hw0										// 1:Interrupt is pending

//4 Timing Register
#define	HwI2CMCH0_TR							*(volatile unsigned long *)0xF100B018	// Timing Register
#define	HwI2CMCH0_TR_CKSEL						Hw5										// Clock Source Select


                                			
#define	HwI2CMCH1_PRES							*(volatile unsigned long *)0xF100B040	// R/W, I2C Clock Prescale Register
                                    		
#define	HwI2CMCH1_CTRL							*(volatile unsigned long *)0xF100B044	// R/W, I2C Control Register
#define	HwI2CMCH1_CTRL_EN_ON					Hw7										// Enabled
#define	HwI2CMCH1_CTRL_EN_OFF					~Hw7									// Disabled
#define	HwI2CMCH1_CTRL_IEN_ON					Hw6										// Enabled
#define	HwI2CMCH1_CTRL_IEN_OFF					~Hw6									// Disabled
#define	HwI2CMCH1_CTRL_MOD_16					Hw5										// 16bit Mode
#define	HwI2CMCH1_CTRL_MOD_8					~Hw5									// 8bit Mode
                                    		
#define	HwI2CMCH1_TXR							*(volatile unsigned long *)0xF100B048	// W, I2C Transmit Register
                                    		
#define	HwI2CMCH1_CMD							*(volatile unsigned long *)0xF100B04C	// W, I2C Command Register
#define	HwI2CMCH1_CMD_STA_EN					Hw7 									// Start Condition Generation Enabled
#define	HwI2CMCH1_CMD_STA_DIS					~Hw7 									// Start Condition Generation Disabled
#define	HwI2CMCH1_CMD_STO_EN					Hw6 									// Stop Condition Generation Enabled
#define	HwI2CMCH1_CMD_STO_DIS					~Hw6 									// Stop Condition Generation Disabled
#define	HwI2CMCH1_CMD_RD_EN						Hw5 									// Read From Slave Enabled
#define	HwI2CMCH1_CMD_RD_DIS					~Hw5 									// Read From Slave Disabled
#define	HwI2CMCH1_CMD_WR_EN						Hw4 									// Wrtie to Slabe Enabled
#define	HwI2CMCH1_CMD_WR_DIS					~Hw4 									// Wrtie to Slabe Disabled
#define	HwI2CMCH1_CMD_ACK_EN					Hw3 									// Sent ACK Enabled
#define	HwI2CMCH1_CMD_ACK_DIS					~Hw3 									// Sent ACK Disalbed
#define	HwI2CMCH1_CMD_IACK_CLR					Hw0 									// Clear a pending interrupt
                                    		
#define	HwI2CMCH1_RXR							*(volatile unsigned long *)0xF100B050	// R, I2C Receive Register
                                    		
#define	HwI2CMCH1_SR							*(volatile unsigned long *)0xF100B054	// R, I2C Status Register
#define	HwI2CMCH1_SR_RxACK						Hw7										// 1:Acknowledge received, 0:No Acknowledge received
#define	HwI2CMCH1_SR_BUSY						Hw6										// 1:Start signal detected, 0:Stop signal detected
#define	HwI2CMCH1_SR_AL							Hw5										// 1:The core lost arbitration, 0:The core don't lost arbitration
#define	HwI2CMCH1_SR_TIP						Hw1										// 1:Transfer Complete, 0:Transferring Data
#define	HwI2CMCH1_SR_IF							Hw0										// 1:Interrupt is pending
                                    		
#define	HwI2CMCH1_TR							*(volatile unsigned long *)0xF100B058	// Timing Register
#define	HwI2CMCH1_TR_CKSEL						Hw5										// Clock Source Select
                                    		
#define	HwI2CS_DPORT							*(volatile unsigned long *)0xF100B080	// Data Port
                                    		
#define	HwI2CS_CTL								*(volatile unsigned long *)0xF100B084	// Control Register
#define	HwI2CS_CTL_SLV_2M						HwZERO									// 2 master operation
#define	HwI2CS_CTL_SLV_1M1S						Hw30									// 1 master / 1 slave
#define	HwI2CS_CTL_RCLR							Hw5										// clear interrupt status at read cycle
#define	HwI2CS_CTL_WS							Hw4										// wait status control by SCL stretching
#define	HwI2CS_CTL_SDA							Hw3										//
#define	HwI2CS_CTL_CLR							Hw2										// Clear FIFO
#define	HwI2CS_CTL_EN							Hw0										// Enable for this slave core
                                    		
#define	HwI2CS_ADDR								*(volatile unsigned long *)0xF100B088	// Address Register
                                    		
#define	HwI2CS_INT								*(volatile unsigned long *)0xF100B08C	// Interrupt Register
#define	HwI2CS_INT_STAT_R_BYTE					Hw27									//
#define	HwI2CS_INT_STAT_W_BYTE					Hw26									//
#define	HwI2CS_INT_STAT_R_BUFF					Hw25									//
#define	HwI2CS_INT_STAT_W_BUFF					Hw24									//
#define	HwI2CS_INT_STAT_TXUR					Hw23									//
#define	HwI2CS_INT_STAT_RXOR					Hw22									//
#define	HwI2CS_INT_STAT_TXB						Hw21									//
#define	HwI2CS_INT_STAT_RXF						Hw20									//
#define	HwI2CS_INT_STAT_TXE						Hw19									//
#define	HwI2CS_INT_STAT_RXNE					Hw18									//
#define	HwI2CS_INT_STAT_TXL						Hw17									//
#define	HwI2CS_INT_STAT_RXL						Hw16									//
#define	HwI2CS_INT_EN_R_BYTE					Hw11									//
#define	HwI2CS_INT_EN_W_BYTE					Hw10									//
#define	HwI2CS_INT_EN_R_BUFF					Hw9										//
#define	HwI2CS_INT_EN_W_BUFF					Hw8										//
#define	HwI2CS_INT_EN_TXUR						Hw7										//
#define	HwI2CS_INT_EN_RXOR						Hw6										//
#define	HwI2CS_INT_EN_TXB						Hw5										//
#define	HwI2CS_INT_EN_RXF						Hw4										//
#define	HwI2CS_INT_EN_TXE						Hw3										//
#define	HwI2CS_INT_EN_RXNE						Hw2										//
#define	HwI2CS_INT_EN_TXL						Hw1										//
#define	HwI2CS_INT_EN_RXL						Hw0										//
                                    		
#define	HwI2CS_STAT								*(volatile unsigned long *)0xF100B090	// Status Register
#define	HwI2CS_STAT_DDIR						Hw23									// Data Direction
                                    		
#define	HwI2CS_MBF								*(volatile unsigned long *)0xF100B09C	// Buffer Valid Flag Register
                                    		
#define	HwI2CS_MB0								*(volatile unsigned long *)0xF100B0A0	// Data Buffer 0
                                    		
#define	HwI2CS_MB1								*(volatile unsigned long *0xF100B0A4	// Data Buffer 1

#define	HwI2C_IRQSTR							*(volatile unsigned long *)0xF100B0C0	// IRQ Status
#define	HwI2C_IRQSTR_ST2						Hw2										//
#define	HwI2C_IRQSTR_ST1						Hw1										//
#define	HwI2C_IRQSTR_ST0						Hw0										//
/*
#define	HwI2CMCH0_PRES	*(volatile unsigned long *)0xF0008000	// R/W, I2C Clock Prescale Register

#define	HwI2CMCH0_CTL	*(volatile unsigned long *)0xF0008004	// R/W, I2C Control Register
#define	HwI2CM_CTRL_EN_ON					Hw7					//
#define	HwI2CM_CTRL_EN_OFF					~Hw7				//
#define	HwI2CM_CTRL_IEN_ON					Hw6					//
#define	HwI2CM_CTRL_IEN_OFF					~Hw6				//
#define	HwI2CM_CTRL_MOD_16					Hw5					//
#define	HwI2CM_CTRL_MOD_8					~Hw5				//

#define	HwI2CMCH0_TXR	*(volatile unsigned long *)0xF0008008	// W, I2C Transmit Register

#define	HwI2CMCH0_CMD	*(volatile unsigned long *)0xF000800C	// W, I2C Command Register
#define	HwI2CM_CMD_STA_EN					Hw7 				//
#define	HwI2CM_CMD_STA_DIS					~Hw7 				//
#define	HwI2CM_CMD_STO_EN					Hw6 				//
#define	HwI2CM_CMD_STO_DIS					~Hw6 				//
#define	HwI2CM_CMD_RD_EN					Hw5 				//
#define	HwI2CM_CMD_RD_DIS					~Hw5 				//
#define	HwI2CM_CMD_WR_EN					Hw4 				//
#define	HwI2CM_CMD_WR_DIS					~Hw4 				//
#define	HwI2CM_CMD_ACK_EN					Hw3 				//
#define	HwI2CM_CMD_ACK_DIS					~Hw3 				//
#define	HwI2CM_CMD_IACK_CLR					Hw0 				//

#define	HwI2CMCH0_RXR	*(volatile unsigned long *)0xF0008010	// R, I2C Receive Register

#define	HwI2CMCH0_SR	*(volatile unsigned long *)0xF0008014	// R, I2C Status Register
#define	HwI2CM_SR_RxACK						Hw7					//
#define	HwI2CM_SR_BUSY						Hw6					//
#define	HwI2CM_SR_AL						Hw5					//
#define	HwI2CM_SR_TIP						Hw1					//
#define	HwI2CM_SR_IF						Hw0					//
*/
//Done...
/////////

/************************************************************************
*	ECC Register Define							(Base Addr = 0xF100D000)
************************************************************************/
/////////
//Done...

#define	HwECC_CTRL				*(volatile unsigned long *)0xF100D000	// R/W, ECC Control Register
#define	HwECC_CTRL_IEN_MECC8_EN		Hw31					// MLC ECC8 Decoding Interrupt Enable
#define	HwECC_CTRL_IEN_MECC8_DIS		~Hw31					// MLC ECC8 Decoding Interrupt Disable
#define	HwECC_CTRL_IEN_MECC4_EN		Hw30					// MLC ECC4 Decoding Interrupt Enable
#define	HwECC_CTRL_IEN_MECC4_DIS		~Hw30					// MLC ECC4 Decoding Interrupt Disable
#define	HwECC_CTRL_DEN_EN			Hw29					// ECC DMA Request Enable
#define	HwECC_CTRL_DEN_DIS			~Hw29					// ECC DMA Request Disable
#define	HwECC_CTRL_EN_DIS				~(Hw2|Hw1)				// ECC Disable

#define HwECC_CTRL_ENC_DEC			Hw0						// ECC Decoding Enable
#define HwECC_CTRL_ENC_ENC			~Hw0					// ECC Encoding Enable
#define HwECC_CTRL_SE_EN			Hw1
#define HwECC_CTRL_M4EN_EN			Hw2
#define HwECC_CTRL_M8EN_EN			(Hw2|Hw1)

#define	HwECC_CTRL_EN_SLCEN			Hw1						// SLC ECC Encoding Enable
#define	HwECC_CTRL_EN_SLCDE			(Hw1|Hw0)				// SLC ECC Decoding Enable
#define	HwECC_CTRL_EN_MCL4EN			Hw2						// MLC ECC4 Encoding Enable
#define	HwECC_CTRL_EN_MCL4DE			(Hw2|Hw0)				// MLC ECC4 Decoding Enable
#define	HwECC_CTRL_EN_MCL8EN			(Hw2|Hw1)				// MLC ECC8 Encoding Enable
#define	HwECC_CTRL_EN_MCL8DE			(Hw2|Hw1|Hw0)			// MLC ECC8 Decoding Enable

#define	HwECC_BASE			*(volatile unsigned long *)0xF100D004	// R/W, Base Address for ECC Calculation Register
#define	HwECC_MASK			*(volatile unsigned long *)0xF100D008	// R/W, Address Mask for ECC Area Register
#define	HwECC_CLR				*(volatile unsigned long *)0xF100D00C	// W, Clear ECC Output Register
#define	HwSLC_ECC0				*(volatile unsigned long *)0xF100D010	// R/w, 1st SLC ECC Code Register
#define	HwSLC_ECC1				*(volatile unsigned long *)0xF100D014	// R/w, 2nd SLC ECC Code Register
#define	HwSLC_ECC2				*(volatile unsigned long *)0xF100D018	// R/w, 3nd SLC ECC Code Register
#define	HwSLC_ECC3				*(volatile unsigned long *)0xF100D01C	// R/w, 4th SLC ECC Code Register
#define	HwSLC_ECC4				*(volatile unsigned long *)0xF100D020	// R/w, 5th SLC ECC Code Register
#define	HwSLC_ECC5				*(volatile unsigned long *)0xF100D024	// R/w, 6th SLC ECC Code Register
#define	HwSLC_ECC6				*(volatile unsigned long *)0xF100D028	// R/w, 7th SLC ECC Code Register
#define	HwSLC_ECC7				*(volatile unsigned long *)0xF100D02C	// R/w, 8th SLC ECC Code Register
#define	HwSLC_ECC8				*(volatile unsigned long *)0xF100D030	// R/w, 9th SLC ECC Code Register
#define	HwSLC_ECC9				*(volatile unsigned long *)0xF100D034	// R/w, 10th SLC ECC Code Register
#define	HwSLC_ECC10			*(volatile unsigned long *)0xF100D038	// R/w, 11th SLC ECC Code Register
#define	HwSLC_ECC11			*(volatile unsigned long *)0xF100D03C	// R/w, 12th SLC ECC Code Register
#define	HwSLC_ECC12			*(volatile unsigned long *)0xF100D040	// R/w, 13th SLC ECC Code Register
#define	HwSLC_ECC13			*(volatile unsigned long *)0xF100D044	// R/w, 14th SLC ECC Code Register
#define	HwSLC_ECC14			*(volatile unsigned long *)0xF100D048	// R/w, 15th SLC ECC Code Register
#define	HwSLC_ECC15			*(volatile unsigned long *)0xF100D04C	// R/w, 16th SLC ECC Code Register

#define	HwMLC_ECC4_0			*(volatile unsigned long *)0xF100D010	// R/W, 1st MLC ECC4 Code Register
#define	HwMLC_ECC4_1			*(volatile unsigned long *)0xF100D014	// R/W, 2nd MLC ECC4 Code Register
#define	HwMLC_ECC4_2			*(volatile unsigned long *)0xF100D018	// R/W, 3rd MLC ECC4 Code Register

#define	HwMLC_ECC8_0			*(volatile unsigned long *)0xF100D010	// R/W, 1st MLC ECC8 Code Register
#define	HwMLC_ECC8_1			*(volatile unsigned long *)0xF100D014	// R/W, 2nd MLC ECC8 Code Register
#define	HwMLC_ECC8_2			*(volatile unsigned long *)0xF100D018	// R/W, 3rd MLC ECC8 Code Register
#define	HwMLC_ECC8_3			*(volatile unsigned long *)0xF100D01C	// R/W, 4th MLC ECC8 Code Register
#define	HwMLC_ECC8_4			*(volatile unsigned long *)0xF100D020	// R/W, 5th MLC ECC8 Code Register

#define	HwSECC_EADDR0	 		*(volatile unsigned long *)0xF100D050	// R, SLC ECC Error Address Register 0
#define	HwSECC_EADDR1	 		*(volatile unsigned long *)0xF100D054	// R, SLC ECC Error Address Register 1
#define	HwSECC_EADDR2	 		*(volatile unsigned long *)0xF100D058	// R, SLC ECC Error Address Register 2
#define	HwSECC_EADDR3	 		*(volatile unsigned long *)0xF100D05C	// R, SLC ECC Error Address Register 3 
#define	HwSECC_EADDR4	 		*(volatile unsigned long *)0xF100D060	// R, SLC ECC Error Address Register 4
#define	HwSECC_EADDR5	 		*(volatile unsigned long *)0xF100D064	// R, SLC ECC Error Address Register 5
#define	HwSECC_EADDR6	 		*(volatile unsigned long *)0xF100D068	// R, SLC ECC Error Address Register 6
#define	HwSECC_EADDR7	 		*(volatile unsigned long *)0xF100D06C	// R, SLC ECC Error Address Register 7
#define	HwSECC_EADDR8	 		*(volatile unsigned long *)0xF100D070	// R, SLC ECC Error Address Register 8
#define	HwSECC_EADDR9	 		*(volatile unsigned long *)0xF100D074	// R, SLC ECC Error Address Register 9
#define	HwSECC_EADDR10		*(volatile unsigned long *)0xF100D078	// R, SLC ECC Error Address Register 10
#define	HwSECC_EADDR11		*(volatile unsigned long *)0xF100D07C	// R, SLC ECC Error Address Register 11
#define	HwSECC_EADDR12		*(volatile unsigned long *)0xF100D080	// R, SLC ECC Error Address Register 12
#define	HwSECC_EADDR13		*(volatile unsigned long *)0xF100D084	// R, SLC ECC Error Address Register 13
#define	HwSECC_EADDR14		*(volatile unsigned long *)0xF100D088	// R, SLC ECC Error Address Register 14
#define	HwSECC_EADDR15		*(volatile unsigned long *)0xF100D08C	// R, SLC ECC Error Address Register 15

#define	HwERRADDR0 			*(volatile unsigned long *)0xF100D050	// R, MLC ECC Error Address Register 0
#define	HwERRADDR1			*(volatile unsigned long *)0xF100D054// R, MLC ECC Error Address Register 1
#define	HwERRADDR2			*(volatile unsigned long *)0xF100D058// R, MLC ECC Error Address Register 2
#define	HwERRADDR3			*(volatile unsigned long *)0xF100D05C// R, MLC ECC Error Address Register 3

#define	HwMECC4_EDATA0 		*(volatile unsigned long *)0xF100D070	// R, MLC ECC Error Data Register 0
#define	HwMECC4_EDATA1		*(volatile unsigned long *)0xF100D074// R, MLC ECC Error Data Register 1
#define	HwMECC4_EDATA2		*(volatile unsigned long *)0xF100D078// R, MLC ECC Error Data Register 2
#define	HwMECC4_EDATA3		*(volatile unsigned long *)0xF100D07C// R, MLC ECC Error Data Register 3 

#define	HwMECC8_EADDR0		*(volatile unsigned long *)0xF100D050// R, MLC ECC8 Error Address Register0
#define	HwMECC8_EADDR1		*(volatile unsigned long *)0xF100D054// R, MLC ECC8 Error Address Register1
#define	HwMECC8_EADDR2		*(volatile unsigned long *)0xF100D058// R, MLC ECC8 Error Address Register2
#define	HwMECC8_EADDR3		*(volatile unsigned long *)0xF100D05C// R, MLC ECC8 Error Address Register3
#define	HwMECC8_EADDR4		*(volatile unsigned long *)0xF100D060// R, MLC ECC8 Error Address Register4
#define	HwMECC8_EADDR5		*(volatile unsigned long *)0xF100D064// R, MLC ECC8 Error Address Register5
#define	HwMECC8_EADDR6		*(volatile unsigned long *)0xF100D068// R, MLC ECC8 Error Address Register6
#define	HwMECC8_EADDR7		*(volatile unsigned long *)0xF100D06C// R, MLC ECC8 Error Address Register7

#define	HwMECC8_EDATA0		*(volatile unsigned long *)0xF100D070// R, MLC ECC8 Error Data Register0
#define	HwMECC8_EDATA1		*(volatile unsigned long *)0xF100D074// R, MLC ECC8 Error Data Register1
#define	HwMECC8_EDATA2		*(volatile unsigned long *)0xF100D078// R, MLC ECC8 Error Data Register2
#define	HwMECC8_EDATA3		*(volatile unsigned long *)0xF100D07C// R, MLC ECC8 Error Data Register3
#define	HwMECC8_EDATA4		*(volatile unsigned long *)0xF100D080// R, MLC ECC8 Error Data Register4
#define	HwMECC8_EDATA5		*(volatile unsigned long *)0xF100D084// R, MLC ECC8 Error Data Register5
#define	HwMECC8_EDATA6		*(volatile unsigned long *)0xF100D088// R, MLC ECC8 Error Data Register6
#define	HwMECC8_EDATA7		*(volatile unsigned long *)0xF100D08C// R, MLC ECC8 Error Data Register7

#define	HwERR_NUM			*(volatile unsigned long *)0xF100D090// R, ECC Error Number Register
#define	HwERR_NUM_ERR1			Hw0				// Correctable Error(SLC), Error Occurred(MLC3), 1 Error Occurred(MLC4)
#define	HwERR_NUM_ERR2			Hw1				// 2 Error Occurred(MLC4)
#define	HwERR_NUM_ERR3			(Hw1|Hw0)		// 3 Error Occurred(MLC4)
#define	HwERR_NUM_ERR4			Hw2				// 4 Error Occurred(MLC4)
#define	HwERR_NUM_ERR5			(Hw2|Hw0)		// 5 Error Occurred(MLC4)
#define	HwERR_NUM_ERR6			(Hw2|Hw1)		// 5 Error Occurred(MLC4)
#define	HwERR_NUM_ERR7			(Hw2|Hw1|Hw0)		// 5 Error Occurred(MLC4)
#define	HwERR_NUM_ERR8			Hw3		// 5 Error Occurred(MLC4)


#define	HwERR_NUM_NOERR			HwZERO			// No Error
#define	HwERR_NUM_CORIMP		(Hw1|Hw0)		// Correction Impossible(SLC, MLC4)

#define	HwECC_IREQ				*(volatile unsigned long *)0xF100D094// R/w, ECC Interrupt Control Register
#define HwECC_IREQ_SEF			Hw21			// SLC ECC Encoding Flag Register
#define HwECC_IREQ_SDF			Hw20			// SLC ECC Decoding Flag Register

#define HwECC_IREQ_M4EF			Hw19			// MLC ECC4 Encoding Flag Register
#define HwECC_IREQ_M4DF			Hw18			// MLC ECC4 Decoding Flag Register

#define HwECC_IREQ_M8EF			Hw17			// MLC ECC8 Encoding Flag Register
#define HwECC_IREQ_M8DF			Hw16			// MLC ECC8 Decoding Flag Register

#define HwECC_IREQ_M4DI			Hw2				// MLC ECC4 Decoding Interrupt Request Register
#define HwECC_IREQ_M8DI			Hw0				// MLC ECC8 Decoding Interrupt Request Register

#define HwECC_IREQ_CLR			(Hw21|Hw20|Hw19|Hw18|Hw17|Hw16|Hw2|Hw0)

#define	HwECC_FSMSTATE		*(volatile unsigned long *)0xF100D098// R, ECC FSM State Register
#define	HwENCSEED				*(volatile unsigned long *)0xF100D0F0// w, Test Mode Register
#define	HwENCMASK				*(volatile unsigned long *)0xF100D0F4// w, Test Mode Register
#define	HwENCDATA				*(volatile unsigned long *)0xF100D0F8// R/w, ECC Interrupt Control Register

/************************************************************************
*	ADC Register Define							(Base Addr = 0xF000A000)
************************************************************************/
//////////
//Done...

#define	HwADCCON		*(volatile unsigned long *)0xF1011000	// R/W, ADC Control
#define	HwADCCON_STB_ON			Hw4					// ADC goes Standby mode
#define	HwADCCON_STB_OFF			~Hw4				// ADC Start operating
#define	HwADCCON_ASEL_CH0		HwZERO		//
#define	HwADCCON_ASEL_CH1		Hw0					//
#define	HwADCCON_ASEL_CH2		Hw1					//
#define	HwADCCON_ASEL_CH3		(Hw1|Hw0)			//
#define	HwADCCON_ASEL_CH4		Hw2					//
#define	HwADCCON_ASEL_CH5		(Hw2|Hw0)			//
#define	HwADCCON_ASEL_CH6		(Hw2|Hw1)			//
#define	HwADCCON_ASEL_CH7		(Hw2|Hw1|Hw0)		//

#define	HwADDATA		*(volatile unsigned long *)0xF1011004	// R, ADC Data
#define	HwADDATA_FLG				Hw0					//

#define	HwADCCONA		*(volatile unsigned long *)0xF1011080	// R/W, ADC Control Register A
#define	HwADCCONA_STB_ON			Hw4					// ADC goes Standby mode
#define	HwADCCONA_STB_OFF		~Hw4				// ADC Start operating
#define	HwADCCONA_ASEL_CH0		HwZERO		//
#define	HwADCCONA_ASEL_CH1		Hw0					//
#define	HwADCCONA_ASEL_CH2		Hw1					//
#define	HwADCCONA_ASEL_CH3		(Hw1|Hw0)			//
#define	HwADCCONA_ASEL_CH4		Hw2					//
#define	HwADCCONA_ASEL_CH5		(Hw2|Hw0)			//
#define	HwADCCONA_ASEL_CH6		(Hw2|Hw1)			//
#define	HwADCCONA_ASEL_CH7		(Hw2|Hw1|Hw0)		//

#define	HwADCSTATUS	*(volatile unsigned long *)0xF1011084	// R/W, ADC Status Register

#define	HwADCCFG		*(volatile unsigned long *)0xF1011088	// R/W, Configuration Register
#define	HwADCCFG_NEOC			~Hw7				//
#define	HwADCCFG_IRQE_ON			Hw3					//
#define	HwADCCFG_IRQE_OFF		~Hw3				//
#define	HwADCCFG_R8_8BIT			Hw2					//
#define	HwADCCFG_R8_10BIT		~Hw2				//
#define	HwADCCFG_APD_ON			Hw1					//
#define	HwADCCFG_APD_OFF			~Hw1				//
#define	HwADCCFG_SM_EN			Hw0					//
#define	HwADCCFG_SM_DIS			~Hw0				//

//Done...
//////////


/************************************************************************
*	RTC Register Define							(Base Addr = 0xF1012000)
************************************************************************/
///////////
//To Do...

#define	HwRTCCON		*(volatile unsigned long *)0xF1012000	// R/W
#define	HwRTCCON_HALT			Hw0
#define	HwRTCCON_RTCEN		Hw1				// RTC Register write enabled
#define	HwRTCCON_CLKSEL_NORM	HwZERO			// Counting clock is 1Hz.
#define	HwRTCCON_CLKSEL_FAST	Hw2				// Counting clock is 32.768kHz
#define	HwRTCCON_CNTSEL_NORM	HwZERO			// BCD counter acts normally
#define	HwRTCCON_CNTSEL_SEP	Hw3				// BCD counter acts separately.
#define	HwRTCCON_CLKRST_ON	Hw4				// BCD counter is reseted.
#define	HwRTCCON_OSCEN_TEN	Hw5				// Oscillator & Divider circuit test enabled.
#define	HwRTCCON_AIOUT_EN	Hw6				// Oscillator & Divider circuit test enabled.
#define	HwRTCCON_WUOUT_EN	Hw7				// Oscillator & Divider circuit test enabled.

#define	HwINTCON		*(volatile unsigned long *)0xF1012004	// R/W
//#define	HwRTCINTCON		*(volatile unsigned long *)0xF000B004	// R/W
//#define	HwRTCRST_INTWR_EN		Hw0				// Round-up enabled
#define	HwINTCON_INTWREN		Hw0				// Round-up enabled

#define	HwRTCALM		*(volatile unsigned long *)0xF1012008	// R/W
#define	HwRTCALM_SECEN_EN		Hw0				// Second Alarm interrupt enabled
#define	HwRTCALM_MINEN_EN		Hw1				// Minute Alarm interrupt enabled
#define	HwRTCALM_HOUREN_EN	Hw2				// Hour Alarm interrupt enabled
#define	HwRTCALM_DATEEN_EN		Hw3				// Date Alarm interrupt enabled
#define	HwRTCALM_DAYEN_EN		Hw4				// Day of Week Alarm interrupt enabled
#define	HwRTCALM_MONEN_EN		Hw5				// Month Alarm interrupt enabled
#define	HwRTCALM_YEAREN_EN		Hw6				// Year Alarm interrupt enabled
#define	HwRTCALM_ALMEN_EN		Hw7				// Alarm interrupt enabled (global enable)
#define	HwRTCALM_ALLEN			0xFF

#define	HwALMSEC		*(volatile unsigned long *)0xF101200C	// R/W

#define	HwALMMIN		*(volatile unsigned long *)0xF1012010	// R/W

#define	HwALMHOUR		*(volatile unsigned long *)0xF1012014	// R/W

#define	HwALMDATE		*(volatile unsigned long *)0xF1012018	// R/W

#define	HwALMDAY		*(volatile unsigned long *)0xF101201C	// R/W
enum {	HwALMDAY_SUN	= 0,
		HwALMDAY_MON,
		HwALMDAY_TUE,
		HwALMDAY_WED,
		HwALMDAY_THR,
		HwALMDAY_FRI,
		HwALMDAY_SAT
};
#define	HwALMMON		*(volatile unsigned long *)0xF1012020	// R/W

#define	HwALMYEAR		*(volatile unsigned long *)0xF1012024	// R/W

#define	HwBCDSEC		*(volatile unsigned long *)0xF1012028	// R/W

#define	HwBCDMIN		*(volatile unsigned long *)0xF101202C	// R/W

#define	HwBCDHOUR		*(volatile unsigned long *)0xF1012030	// R/W

#define	HwBCDDATE		*(volatile unsigned long *)0xF1012034	// R/W

#define	HwBCDDAY		*(volatile unsigned long *)0xF1012038	// R/W
enum {	HwBCDDAY_SUN	= 0,
		HwBCDDAY_MON,
		HwBCDDAY_TUE,
		HwBCDDAY_WED,
		HwBCDDAY_THR,
		HwBCDDAY_FRI,
		HwBCDDAY_SAT
};

#define	HwBCDMON		*(volatile unsigned long *)0xF101203C	// R/W

#define	HwBCDYEAR		*(volatile unsigned long *)0xF1012040	// R/W

#define	HwRTCIM		*(volatile unsigned long *)0xF1012044	// R/W
#define	HwRTCIM_ALMINT_EDGE_EN		Hw0
#define	HwRTCIM_ALMINT_LEVEL_EN	(Hw1+Hw0)
#define	HwRTCIM_ALMINT_DIS			HwZERO
#define	HwRTCIM_PMWKUP_ACTIVE_HIGH			Hw2
#define	HwRTCIM_PMWKUP_ACTIVE_LOW			~Hw2
#define	HwRTCIM_PWDN_POWERDOWN_MODE		Hw3
#define	HwRTCIM_PWDN_NORMAL_OP_MODE		~Hw3
#if 0
#define	HwRTCIM_PEINT_EN			Hw2
#define	HwRTCIM_PEINT_DIS			HwZERO
#define	HwRTCIM_PEINT_1P0SEC		(0 << 3)			// no pending interrupt generated
#define	HwRTCIM_PEINT_1P256SEC		(1 << 3)			// 1/256 sec
#define	HwRTCIM_PEINT_1P64SEC		(2 << 3)			// 1/64 sec
#define	HwRTCIM_PEINT_1P16SEC		(3 << 3)			// 1/16 sec
#define	HwRTCIM_PEINT_1P4SEC		(4 << 3)			// 1/4 sec
#define	HwRTCIM_PEINT_1P2SEC		(5 << 3)			// 1/2 sec
#define	HwRTCIM_PEINT_1SEC			(6 << 3)			// 1 sec
#endif

#define	HwRTCPEND		*(volatile unsigned long *)0xF1012048	// R/W
#define	HwRTCPEND_PENDING			Hw0

//To Do...
///////////

#if 0
/***********************************************************************
*	UART2 Controller Register Define				(Base Addr = 0xF000C000)
************************************************************************/
///////////
//To Do...

#define	HwUART2_UTRXD	*(volatile unsigned long *)0xF000C000	// R, Receiver Buffer Register

#define	HwUART2_UTTXD	*(volatile unsigned long *)0xF000C000	// W, Transmitter Holding Register

#define	HwUART2_UTDL	*(volatile unsigned long *)0xF000C004	// W, Divisor Latch Register

#define	HwUART2_UTIR	*(volatile unsigned long *)0xF000C008	// R/W, Interrupt Register
#define	HwUART2_UTIR_ERS_EN		Hw10				// Receiver Line Status interrupt is enabled
#define	HwUART2_UTIR_ERS_DIS		~Hw10				// Receiver Line Status interrupt is disabled
#define	HwUART2_UTIR_ETX_EN		Hw9					// Transmitter Holding Register Empty interrupt is enabled
#define	HwUART2_UTIR_ETX_DIS		~Hw9				// Transmitter Holding Register Empty interrupt is disabled
#define	HwUART2_UTIR_ERX_EN		Hw8					// Receiver Data Available interrupt is enabled
#define	HwUART2_UTIR_ERX_DIS		~Hw8				// Receiver Data Available interrupt is disabled
#define	HwUART2_UTIR_FRS_CHG		Hw6					// Indicated that Receiver Line Status has changed
#define	HwUART2_UTIR_FRS_NOTCHG	~Hw6				// Indicated that Receiver Line Status has not changed
#define	HwUART2_UTIR_FTX_GEN		Hw5					// Interrupt has generated, but not cleared
#define	HwUART2_UTIR_FTX_NOTGEN	~Hw5				// Interrupt has not generated
#define	HwUART2_UTIR_FRX_GEN		Hw4					// Interrupt has generated, but not cleared
#define	HwUART2_UTIR_FRX_NOTGEN	~Hw4				// Interrupt has not generated
#define	HwUART2_UTIR_QRS_GEN		Hw2					// Indicated that Receiver Line Status interrupt has generated
#define	HwUART2_UTIR_QRS_NOTGEN	~Hw2				// Indicated that Receiver Line Status interrupt has not generated
#define	HwUART2_UTIR_QTX_GEN		Hw1					// Transmitter Holding Register Empty interrupt has generated
#define	HwUART2_UTIR_QTX_NOTGEN	~Hw1				// Transmitter Holding Register Empty interrupt has not generated
#define	HwUART2_UTIR_QRX_GEN		Hw0					// Indicated that Receiver Data Available interrupt has generated
#define	HwUART2_UTIR_QRX_NOTGEN	~Hw0				// Indicated that Receiver Data Available interrupt has not generated

#define	HwUART2_UTCR	*(volatile unsigned long *)0xF000C00C	// R/W, UART Control Register
#define	HwUART2_UTCR_NO_NOTCHK	Hw9					// Don't check the pulse width of start bit (test or boot mode only)
#define	HwUART2_UTCR_NO_CHK		~Hw9				// Check if the pulse width of start bit is more than 0.5 bit duration of baud rate
#define	HwUART2_UTCR_BK_THR		Hw8					// Bit '0' is transmitted regardless of THR
#define	HwUART2_UTCR_BK_NORMAL	~Hw8				// Normal operation
#define	HwUART2_UTCR_TF_CLR		Hw7					// The transmitter FIFO is cleared
#define	HwUART2_UTCR_TF_NOTCLR	~Hw7				//
#define	HwUART2_UTCR_RF_CLR		Hw6					// The receiver FIFO is cleared
#define	HwUART2_UTCR_RF_NOTCLR	~Hw6				//
#define	HwUART2_UTCR_FIFO_1		~(Hw5|Hw4)			// 1byte FIFO
#define	HwUART2_UTCR_FIFO_2		Hw4					// 2byte FIFO
#define	HwUART2_UTCR_FIFO_4		Hw5					// 4byte FIFO
#define	HwUART2_UTCR_FIFO_7		(Hw6|Hw5)			// 7byte FIFO
#define	HwUART2_UTCR_PR_EVEN		~(Hw3|Hw2)			// Even parity
#define	HwUART2_UTCR_PR_ODD		Hw2					// Odd parity
#define	HwUART2_UTCR_PR_DIS		Hw3					// Parity is disabled
#define	HwUART2_UTCR_ST_2		Hw1					// 2 Stop bit
#define	HwUART2_UTCR_ST_1		~Hw1				// 1 Stop bit

#define	HwUART2_UTLSR	*(volatile unsigned long *)0xF000C010	// R, Status Register
#define	HwUART2_UTLSR_TE_NOSTR	Hw4					// No data is stored in transmitter FIFO
#define	HwUART2_UTLSR_TE_STR		~Hw4				// At least 1 byte is stored in transmitter FIFO
#define	HwUART2_UTLSR_TF_NOSTR	Hw3					// No data is stored in transmitter FIFO
#define	HwUART2_UTLSR_TF_STR		~Hw3				// At least 1 byte is stored in transmitter FIFO
#define	HwUART2_UTLSR_FE_ERR		Hw2					// The received data in the FIFO don't have valid stop bit
#define	HwUART2_UTLSR_FE_GOOD	~Hw2				// Stop bit is received and correct
#define	HwUART2_UTLSR_PE_ERR		Hw1					// The received data in the FIFO don't have valid parity bit
#define	HwUART2_UTLSR_PE_GOOD	~Hw1				// Parity bit is received and correct
#define	HwUART2_UTLSR_RA_RECV	Hw0					// At least 1 received data is stored in the FIFO
#define	HwUART2_UTLSR_RA_NORECV	~Hw0				// No data has been received

#define	HwUART2_IrDACFG1	*(volatile unsigned long *)0xF000C014	// R/W, IrDA Configuration Register1
#define	HwUART2_IrDACFG1_EN_ON		Hw15				// IrDA TX is enable
#define	HwUART2_IrDACFG1_EN_OFF		~Hw15				// IrDA TX is disable, UART mode is used
#define	HwUART2_IrDACFG1_P1_BASE	Hw14				// Pulse width is proportional to UART base clock speed
#define	HwUART2_IrDACFG1_P1_BAUD	~Hw14				// Pulse width is proportional to selected baud used
#define	HwUART2_IrDACFG1_POL_LOW	Hw13				// TX '0' data is converted to level low pulse
#define	HwUART2_IrDACFG1_POL_HIGH	~Hw13				// TX '0' data is converted to level high pulse
#define	HwUART2_IrDACFG1_LB_RX		Hw12				// Transmitted data is feedback to RX port
#define	HwUART2_IrDACFG1_LB_NORMAL	~Hw12				// Normal operation

#define	HwUART2_IrDACFG2	*(volatile unsigned long *)0xF000C018	// R/W, IrDA Configuration Register2
#define	HwUART2_IrDACFG2_EN_ON		Hw15				// IrDA RX is enable
#define	HwUART2_IrDACFG2_EN_OFF		~Hw15				// IrDA RX is disable, UART mode is used
#define	HwUART2_IrDACFG2_P1_BASE	Hw14				// Received Pulse width is proportional to UART base clock speed
#define	HwUART2_IrDACFG2_P1_BAUD	~Hw14				// Received Pulse width is proportional to selected baud used
#define	HwUART2_IrDACFG2_POL_LOW	Hw13				// The polarity of received data is inverted
#define	HwUART2_IrDACFG2_POL_HIGH	~Hw13				// The polarity of received data is not inverted

//To Do...
///////////
#endif

/************************************************************************
*	USBH Register Define						(Base Addr = 0xF000D000)
************************************************************************/
///////////
//To Do...


#define	HwHcRevision			*(volatile unsigned long *)0xF000B000	// Control and status register

#define	HwHcControl				*(volatile unsigned long *)0xF000B004	// Control and status register

#define	HwHcCommandStatus	*(volatile unsigned long *)0xF000B008	// Control and status register

#define	HwHcInterruptStatus		*(volatile unsigned long *)0xF000B00C	// Control and status register

#define	HwHcInterruptEnable		*(volatile unsigned long *)0xF000B010	// Control and status register

#define	HwHcInterruptDisable	*(volatile unsigned long *)0xF000B014	// Control and status register

#define	HwHcHCCA				*(volatile unsigned long *)0xF000B018	// Memory pointer register

#define	HwHcPeroidCurrentED	*(volatile unsigned long *)0xF000B01C	// Memory pointer register

#define	HwHcControlHeadED		*(volatile unsigned long *)0xF000B020	// Memory pointer register

#define	HwHcControlCurrentED	*(volatile unsigned long *)0xF000B024	// Memory pointer register

#define	HwHcBulkHeadED		*(volatile unsigned long *)0xF000B028	// Memory pointer register

#define	HwHcBulkCurrentED		*(volatile unsigned long *)0xF000B02C	// Memory pointer register

#define	HwHcDoneHead			*(volatile unsigned long *)0xF000B030	// Memory pointer register

#define	HwHcRmInterval			*(volatile unsigned long *)0xF000B034	// Frame counter register

#define	HwHcFmRemaining		*(volatile unsigned long *)0xF000B038	// Frame counter register

#define	HwHcFmNumber			*(volatile unsigned long *)0xF000B03C	// Frame counter register

#define	HwHcPeriodStart			*(volatile unsigned long *)0xF000B040	// Frame counter register

#define	HwHcLSThreshold		*(volatile unsigned long *)0xF000B044	// Frame counter register

#define	HwRhDescriptorA		*(volatile unsigned long *)0xF000B048	// Root hub register

#define	HwRhDescriptorB			*(volatile unsigned long *)0xF000B04C	// Root hub register

#define	HwRhStatus				*(volatile unsigned long *)0xF000B050	// Root hub register

#define	HwRhPortStatus1			*(volatile unsigned long *)0xF000B054	// Root hub register

#define	HwRhPortStatus2			*(volatile unsigned long *)0xF000B058	// Root hub register

#define	HwUHINT_ST			HwHcInterruptStatus
#define	HwUHINT_EN			HwHcInterruptEnable
#define	HwUHINT_DIS			HwHcInterruptDisable
//#define	HwUH_RHP0				HwHcRhPortStatus1
#define	HwUH_RHP0			HwRhPortStatus1
#define	HwUHCMD_ST			HwHcCommandStatus

//To Do....
///////////

/************************************************************************
*	3-CH/USB DMA Controller Register Define				(Base Addr = 0xF000E000)
************************************************************************/
/////////
//Done...

/////////////////
// 3ch DMA Channel Config
/////////////////
#define	Hw3CHCONFIG		        	*(volatile unsigned long *)0xF0004090	//DMA0 Channel Configuration Register
#define	HwDMA1_CHCONFIG		       *(volatile unsigned long *)0xF000A090	//DMA1 Channel Configuration Register
#define	Hw3CHCONFIG_IS2_ON		Hw22					
#define	Hw3CHCONFIG_IS2_OFF		~Hw22					
#define	Hw3CHCONFIG_IS1_ON		Hw21					
#define	Hw3CHCONFIG_IS1_OFF		~Hw21					
#define	Hw3CHCONFIG_IS0_ON		Hw20					
#define	Hw3CHCONFIG_IS0_OFF		~Hw20					

#define	Hw3CHCONFIG_MIS2_ON		Hw18					
#define	Hw3CHCONFIG_MIS2_OFF		~Hw18					
#define	Hw3CHCONFIG_MIS1_ON		Hw17					
#define	Hw3CHCONFIG_MIS1_OFF		~Hw17					
#define	Hw3CHCONFIG_MIS0_ON		Hw16					
#define	Hw3CHCONFIG_MIS0_OFF		~Hw16					

#define	Hw3CHCONFIG_SWP2_ON		Hw10
#define	Hw3CHCONFIG_SWP2_OFF	~Hw10
#define	Hw3CHCONFIG_SWP1_ON		Hw9
#define	Hw3CHCONFIG_SWP1_OFF	~Hw9
#define	Hw3CHCONFIG_SWP0_ON		Hw8
#define	Hw3CHCONFIG_SWP0_OFF	~Hw8
#define	Hw3CHCONFIG_FIX_FPM		Hw0						
#define	Hw3CHCONFIG_FIX_RRM		~Hw0					

/////////////////
// usb DMA Channel Config
/////////////////

#define	Hw3CHCONFIG_USB			*(volatile unsigned long *)0xF000882C	// Channel Configuration Register


/////////////////
// DMA0 Channel 0/1/2/USB
/////////////////

#define	Hw3ST_SADR0		*(volatile unsigned long *)0xF0004000	// R/W, Start Address of Source Block Register
#define	Hw3ST_SADR1		*(volatile unsigned long *)0xF0004030	// R/W, Start Address of Source Block Register
#define	Hw3ST_SADR2		*(volatile unsigned long *)0xF0004060	// R/W, Start Address of Source Block Register
#define	Hw3ST_SADR_USB	*(volatile unsigned long *)0xF0008800	// R/W, Start Address of Source Block Register
#define	HwST_SADR0		*(volatile unsigned long *)0xF0004000	// R/W, Start Address of Source Block Register
#define	HwST_SADR1		*(volatile unsigned long *)0xF0004030	// R/W, Start Address of Source Block Register
#define	HwST_SADR2		*(volatile unsigned long *)0xF0004060	// R/W, Start Address of Source Block Register
#define	HwST_SADR_USB		*(volatile unsigned long *)0xF0008800	// R/W, Start Address of Source Block Register

#define	Hw3ST_DADR0		*(volatile unsigned long *)0xF0004010	// R/W, Start Address of Destination Block Register
#define	Hw3ST_DADR1		*(volatile unsigned long *)0xF0004040	// R/W, Start Address of Destination Block Register
#define	Hw3ST_DADR2		*(volatile unsigned long *)0xF0004070	// R/W, Start Address of Destination Block Register
#define	Hw3ST_DADR_USB	*(volatile unsigned long *)0xF0008810	// R/W, Start Address of Destination Block Register
#define	HwST_DADR0		*(volatile unsigned long *)0xF0004010	// R/W, Start Address of Destination Block Register
#define	HwST_DADR1		*(volatile unsigned long *)0xF0004040	// R/W, Start Address of Destination Block Register
#define	HwST_DADR2		*(volatile unsigned long *)0xF0004070	// R/W, Start Address of Destination Block Register
#define	HwST_DADR_USB		*(volatile unsigned long *)0xF0008810	// R/W, Start Address of Destination Block Register

#define	Hw3SPARAM0		*(volatile unsigned long *)0xF0004004	// R/W, Parameter of Source Block Register
#define	Hw3SPARAM1		*(volatile unsigned long *)0xF0004034	// R/W, Parameter of Source Block Register
#define	Hw3SPARAM2		*(volatile unsigned long *)0xF0004064	// R/W, Parameter of Source Block Register
#define	Hw3SPARAM_USB	*(volatile unsigned long *)0xF0008804	// R/W, Parameter of Source Block Register
#define	HwSPARAM0			*(volatile unsigned long *)0xF0004004	// R/W, Parameter of Source Block Register
#define	HwSPARAM1			*(volatile unsigned long *)0xF0004034	// R/W, Parameter of Source Block Register
#define	HwSPARAM2			*(volatile unsigned long *)0xF0004064	// R/W, Parameter of Source Block Register
#define	HwSPARAM_USB		*(volatile unsigned long *)0xF0008804	// R/W, Parameter of Source Block Register

#define	Hw3DPARAM0		*(volatile unsigned long *)0xF0004014	// R/W, Parameter of Destination Block Register
#define	Hw3DPARAM1		*(volatile unsigned long *)0xF0004044	// R/W, Parameter of Destination Block Register
#define	Hw3DPARAM2		*(volatile unsigned long *)0xF0004074	// R/W, Parameter of Destination Block Register
#define	Hw3DPARAM_USB	*(volatile unsigned long *)0xF0008814	// R/W, Parameter of Destination Block Register
#define	HwDPARAM0			*(volatile unsigned long *)0xF0004014	// R/W, Parameter of Destination Block Register
#define	HwDPARAM1			*(volatile unsigned long *)0xF0004044	// R/W, Parameter of Destination Block Register
#define	HwDPARAM2			*(volatile unsigned long *)0xF0004074	// R/W, Parameter of Destination Block Register
#define	HwDPARAM_USB		*(volatile unsigned long *)0xF0008814	// R/W, Parameter of Destination Block Register

#define	Hw3C_SADR0		*(volatile unsigned long *)0xF000400C	// R, Current Address of Source Block Register
#define	Hw3C_SADR1		*(volatile unsigned long *)0xF000403C	// R, Current Address of Source Block Register
#define	Hw3C_SADR2		*(volatile unsigned long *)0xF000406C	// R, Current Address of Source Block Register
#define	Hw3C_SADR_USB		*(volatile unsigned long *)0xF000880C	// R, Current Address of Source Block Register
#define	HwC_SADR0			*(volatile unsigned long *)0xF000400C	// R, Current Address of Source Block Register
#define	HwC_SADR1			*(volatile unsigned long *)0xF000403C	// R, Current Address of Source Block Register
#define	HwC_SADR2			*(volatile unsigned long *)0xF000406C	// R, Current Address of Source Block Register
#define	HwC_SADR_USB		*(volatile unsigned long *)0xF000880C	// R, Current Address of Source Block Register

#define	Hw3C_DADR0		*(volatile unsigned long *)0xF000401C	// R, Current Address of Destination Block Register
#define	Hw3C_DADR1		*(volatile unsigned long *)0xF000404C	// R, Current Address of Destination Block Register
#define	Hw3C_DADR2		*(volatile unsigned long *)0xF000407C	// R, Current Address of Destination Block Register
#define	Hw3C_DADR_USB		*(volatile unsigned long *)0xF000881C	// R, Current Address of Destination Block Register
#define	HwC_DADR0			*(volatile unsigned long *)0xF000401C	// R, Current Address of Destination Block Register
#define	HwC_DADR1			*(volatile unsigned long *)0xF000404C	// R, Current Address of Destination Block Register
#define	HwC_DADR2			*(volatile unsigned long *)0xF000407C	// R, Current Address of Destination Block Register
#define	HwC_DADR_USB		*(volatile unsigned long *)0xF000881C	// R, Current Address of Destination Block Register

#define	Hw3HCOUNT0		*(volatile unsigned long *)0xF0004020	// R/W, Initial and Current Hop Count Register
#define	Hw3HCOUNT1		*(volatile unsigned long *)0xF0004050	// R/W, Initial and Current Hop Count Register
#define	Hw3HCOUNT2		*(volatile unsigned long *)0xF0004080	// R/W, Initial and Current Hop Count Register
#define	Hw3HCOUNT_USB	*(volatile unsigned long *)0xF0008820	// R/W, Initial and Current Hop Count Register
#define	HwHCOUNT0			*(volatile unsigned long *)0xF0004020	// R/W, Initial and Current Hop Count Register
#define	HwHCOUNT1			*(volatile unsigned long *)0xF0004050	// R/W, Initial and Current Hop Count Register
#define	HwHCOUNT2			*(volatile unsigned long *)0xF0004080	// R/W, Initial and Current Hop Count Register
#define	HwHCOUNT_USB		*(volatile unsigned long *)0xF0008820	// R/W, Initial and Current Hop Count Register

#define	HwCHCTRL0		*(volatile unsigned long *)0xF0004024	// R/W, Channel Control Register
#define	HwCHCTRL1		*(volatile unsigned long *)0xF0004054	// R/W, Channel Control Register
#define	HwCHCTRL2		*(volatile unsigned long *)0xF0004084	// R/W, Channel Control Register
#define	HwCHCTRL_USB	*(volatile unsigned long *)0xF0008824	// R/W, Channel Control Register
#define	Hw3CHCTRL0		*(volatile unsigned long *)0xF0004024	// R/W, Channel Control Register
#define	Hw3CHCTRL1		*(volatile unsigned long *)0xF0004054	// R/W, Channel Control Register
#define	Hw3CHCTRL2		*(volatile unsigned long *)0xF0004084	// R/W, Channel Control Register
#define	Hw3CHCTRL_USB	*(volatile unsigned long *)0xF0008824	// R/W, Channel Control Register

#if 0
#define	Hw3CHCTRL_TIM_ON			Hw31				// Timer DMA Interrupte Enable
#define	Hw3CHCTRL_TIM_OFF		~Hw31				// Timer DMA Interrupt Disable
#define	Hw3CHCTRL_UA1T_ON		Hw30				// UART1 Transmite DMA Interrupt Enable
#define	Hw3CHCTRL_UA1T_OFF		~Hw30				// UART1 Transmite DMA Interrupt Disable
#define	Hw3CHCTRL_UA1R_ON		Hw29				// UART1 Receive DMA Interrupt Enable
#define	Hw3CHCTRL_UA1R_OFF		~Hw29				// UART1 Receive DMA Interrupt Disable
#define	Hw3CHCTRL_LCD_ON			Hw28				// LCD DMA Interrupt Enable
#define	Hw3CHCTRL_LCD_OFF		~Hw28				// LCD DMA Interrupt Disable
#define	Hw3CHCTRL_SD_ON			Hw27				// SD/MMC DMA Interrupte Enable
#define	Hw3CHCTRL_SD_OFF			~Hw27				// SD/MMC DMA Interrupt Disable
#define	Hw3CHCTRL_ECC_ON			Hw26				// ECC DMA Interrupte Enable
#define	Hw3CHCTRL_ECC_OFF		~Hw26				// ECCDMA Interrupt Disable
#define	Hw3CHCTRL_SPI_ON			Hw25				// SPI DMA Interrupte Enable
#define	Hw3CHCTRL_SPI_OFF			~Hw25				// SPI DMA Interrupt Disable
#define	Hw3CHCTRL_ND_ON			Hw24				// NAND DMA Interrupte Enable
#define	Hw3CHCTRL_ND_OFF			~Hw24				// NAND DMA Interrupt Disable
#define	Hw3CHCTRL_UA0_ON			Hw23				// UART0 DMA Interrupte Enable
#define	Hw3CHCTRL_UA0_OFF		~Hw23				// UART0 DMA Interrupt Disable
#define	Hw3CHCTRL_GSIO_ON		Hw22				// GSIO DMA Interrupte Enable
#define	Hw3CHCTRL_GSIO_OFF		~Hw22				// GSIO DMA Interrupt Disable
#define	Hw3CHCTRL_I2ST_ON			Hw21				// I2S Transmite DMA Interrupte Enable
#define	Hw3CHCTRL_I2ST_OFF		~Hw21				// I2S Transmite DMA Interrupt Disable
#define	Hw3CHCTRL_I2SR_ON			Hw20				// I2S Receive DMA Interrupte Enable
#define	Hw3CHCTRL_I2SR_OFF		~Hw20				// I2S Receive DMA Interrupt Disable
#define	Hw3CHCTRL_EXT3_ON		Hw19				// External Interrupt 3 DMA Interrupte Enable
#define	Hw3CHCTRL_EXT3_OFF		~Hw19				// External Interrupt 3 DMA Interrupt Disable
#define	Hw3CHCTRL_EXT2_ON		Hw18				// External Interrupt 2 DMA Interrupte Enable
#define	Hw3CHCTRL_EXT2_OFF		~Hw18				// External Interrupt 2 DMA Interrupt Disable
#define	Hw3CHCTRL_EXT1_ON		Hw17				// External Interrupt 1 DMA Interrupte Enable
#define	Hw3CHCTRL_EXT1_OFF		~Hw17				// External Interrupt 1 DMA Interrupt Disable
#define	Hw3CHCTRL_EXT0_ON		Hw16				// External Interrupt 0 DMA Interrupte Enable
#define	Hw3CHCTRL_EXT0_OFF		~Hw16				// External Interrupt 0 DMA Interrupt Disable
#endif

#define	Hw3CHCTRL_CONT_C			Hw15					// DMA transfer begins from C_SADR/C_DADR Address. It must be used after the former transfer has been executed, so that C_SADR and C_DADR contain a meaningful value.
#define	Hw3CHCTRL_CONT_ST		~Hw15					// DMA trnaster begins from ST_SADR/ST_DADR Address
#define	Hw3CHCTRL_DTM_ON			Hw14					// Differential Transfer Mode Enable
#define	Hw3CHCTRL_DTM_OFF		~Hw14					// Differential Transfer Mode Disable
#define	Hw3CHCTRL_SYNC_ON		Hw13					// Synchronize HardWare Request
#define	Hw3CHCTRL_SYNC_OFF		~Hw13					// Do not Synchronize HardWare Request
#define	Hw3CHCTRL_HRD_WR			Hw12					// ACK/EOT signals are issued When DMA-Write Operation
#define	Hw3CHCTRL_HRD_RD			~Hw12					// ACK/EOT signals are issued When DMA-Read Operation
#define	Hw3CHCTRL_LOCK_ON		Hw11					// DMA transfer executed with lock transfer
#define	Hw3CHCTRL_LOCK_OFF		~Hw11					//
#define	Hw3CHCTRL_BST_BURST		Hw10					// DMA transfer executed with no arbitration(burst operation)
#define	Hw3CHCTRL_BST_ARB		~Hw10					// DMA transfer executed wth arbitration
#define	Hw3CHCTRL_TYPE_SINGE	        ~(Hw9|Hw8)				// SINGLE transfer with edge-triggered detection
#define	Hw3CHCTRL_TYPE_HW		Hw8						// SINGLE transfer with level-triggered detection
#define	Hw3CHCTRL_TYPE_SW		Hw9						// HW transfer
#define	Hw3CHCTRL_TYPE_SINGL	        (Hw9|Hw8)				// SW transfer
#define	Hw3CHCTRL_BSIZE_1			~(Hw7|Hw6)				// 1 Burst transfer consists of 1 read or write cycle
#define	Hw3CHCTRL_BSIZE_2			Hw6						// 1 Burst transfer consists of 2 read or write cycles
#define	Hw3CHCTRL_BSIZE_4			Hw7						// 1 Burst transfer consists of 4 read or write cycles
#define	Hw3CHCTRL_BSIZE_8			(Hw6|Hw7)				// 1 Burst transfer consists of 8 read or write cycles
#define	Hw3CHCTRL_WSIZE_8		~(Hw5|Hw4)				// Each cycle read or write 8bit data
#define	Hw3CHCTRL_WSIZE_16		Hw4						// Each cycle read or write 16bit data
#define	Hw3CHCTRL_WSIZE_32		Hw5						// Each cycle read or write 32bit data
#define	Hw3CHCTRL_FLAG			Hw3						// Clears FLAG to 0
#define	Hw3CHCTRL_IEN_ON			Hw2						// At the same time the FLAG goes to 1, DMA interrupt request is generated
#define	Hw3CHCTRL_IEN_OFF			~Hw2					//
#define	Hw3CHCTRL_REP_EN			Hw1						// The DMA channel remains enabled
#define	Hw3CHCTRL_REP_DIS			~Hw1					// After all of hop transfer has executed, the DMA channel is disabled
#define	Hw3CHCTRL_EN_ON		        Hw0						// DMA channel is Enabled
#define	Hw3CHCTRL_EN_OFF			~Hw0					// DMA channel is terminated and disabled}}}


#define	HwCHCTRL0_DMASEL_TIMER_ON	Hw31				// Timer DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_UART1_TX	Hw30				// UART1 Transmite DMA Interrupt Enable
#define	HwCHCTRL0_DMASEL_UART1_RX	Hw29				// UART1 Receive DMA Interrupt Enable
#define	HwCHCTRL0_DMASEL_LCD			Hw28				// LCD DMA Interrupt Enable
#define	HwCHCTRL0_DMASEL_SDMMC		Hw27				// SD/MMC DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_ECC			Hw26				// ECC DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_SPI			Hw25				// SPI DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_NFC			Hw24				// NAND DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_UART0		Hw23				// UART0 DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_GSIO		Hw22				// GSIO DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_I2S_TX		Hw21				// I2S Transmite DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_I2S_RX		Hw20				// I2S Receive DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_EXT3_ON	Hw19				// External Interrupt 3 DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_EXT2_ON	Hw18				// External Interrupt 2 DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_EXT1_ON	Hw17				// External Interrupt 1 DMA Interrupte Enable
#define	HwCHCTRL0_DMASEL_EXT0_ON	Hw16				// External Interrupt 0 DMA Interrupte Enable

#define	HwCHCTRL0_CONT_C				Hw15				// DMA transfer begins from C_SADR/C_DADR Address. It must be used after the former transfer has been executed, so that C_SADR and C_DADR contain a meaningful value.
#define	HwCHCTRL0_DTM_EN				Hw14				// Differential Transfer Mode Enable
#define	HwCHCTRL0_SYNC_EN			Hw13				// Synchronize HardWare Request
#define	HwCHCTRL0_HRD_W				Hw12				// ACK/EOT signals are issued When DMA-Write Operation
#define	HwCHCTRL0_LOCK_EN			Hw11				// DMA transfer executed with lock transfer
#define	HwCHCTRL0_BST_NOARB			Hw10				// DMA transfer executed with no arbitration(burst operation)
#define	HwCHCTRL0_TYPE_SE	        		~(Hw9|Hw8)			// SINGLE transfer with edge-triggered detection
#define	HwCHCTRL0_TYPE_HW			Hw8					// SINGLE transfer with level-triggered detection
#define	HwCHCTRL0_TYPE_SW			Hw9					// HW transfer
#define	HwCHCTRL0_TYPE_SL	        		(Hw9|Hw8)			// SW transfer
#define	HwCHCTRL0_BSIZE_1				~(Hw7|Hw6)			// 1 Burst transfer consists of 1 read or write cycle
#define	HwCHCTRL0_BSIZE_2				Hw6					// 1 Burst transfer consists of 2 read or write cycles
#define	HwCHCTRL0_BSIZE_4				Hw7					// 1 Burst transfer consists of 4 read or write cycles
#define	HwCHCTRL0_BSIZE_8				(Hw6|Hw7)			// 1 Burst transfer consists of 8 read or write cycles
#define	HwCHCTRL0_WSIZE_8			~(Hw5|Hw4)			// Each cycle read or write 8bit data
#define	HwCHCTRL0_WSIZE_16			Hw4					// Each cycle read or write 16bit data
#define	HwCHCTRL0_WSIZE_32			Hw5					// Each cycle read or write 32bit data
#define	HwCHCTRL0_FLAG				Hw3					// Clears FLAG to 0
#define	HwCHCTRL0_IEN_EN				Hw2					// At the same time the FLAG goes to 1, DMA interrupt request is generated
#define	HwCHCTRL0_REP_EN				Hw1					// The DMA channel remains enabled
#define	HwCHCTRL0_EN_EN		        	Hw0					// DMA channel is Enabled

#define	HwCHCTRL1_DMASEL_TIMER_ON	Hw31				// Timer DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_UART1_TX	Hw30				// UART1 Transmite DMA Interrupt Enable
#define	HwCHCTRL1_DMASEL_UART1_RX	Hw29				// UART1 Receive DMA Interrupt Enable
#define	HwCHCTRL1_DMASEL_LCD			Hw28				// LCD DMA Interrupt Enable
#define	HwCHCTRL1_DMASEL_SDMMC		Hw27				// SD/MMC DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_ECC			Hw26				// ECC DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_SPI			Hw25				// SPI DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_NFC			Hw24				// NAND DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_UART0		Hw23				// UART0 DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_GSIO		Hw22				// GSIO DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_I2S_TX		Hw21				// I2S Transmite DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_I2S_RX		Hw20				// I2S Receive DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_EXT3_ON	Hw19				// External Interrupt 3 DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_EXT2_ON	Hw18				// External Interrupt 2 DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_EXT1_ON	Hw17				// External Interrupt 1 DMA Interrupte Enable
#define	HwCHCTRL1_DMASEL_EXT0_ON	Hw16				// External Interrupt 0 DMA Interrupte Enable

#define	HwCHCTRL1_CONT_C				Hw15				// DMA transfer begins from C_SADR/C_DADR Address. It must be used after the former transfer has been executed, so that C_SADR and C_DADR contain a meaningful value.
#define	HwCHCTRL1_DTM_EN				Hw14				// Differential Transfer Mode Enable
#define	HwCHCTRL1_SYNC_EN			Hw13				// Synchronize HardWare Request
#define	HwCHCTRL1_HRD_W				Hw12				// ACK/EOT signals are issued When DMA-Write Operation
#define	HwCHCTRL1_LOCK_EN			Hw11				// DMA transfer executed with lock transfer
#define	HwCHCTRL1_BST_NOARB			Hw10				// DMA transfer executed with no arbitration(burst operation)
#define	HwCHCTRL1_TYPE_SE	        		~(Hw9|Hw8)			// SINGLE transfer with edge-triggered detection
#define	HwCHCTRL1_TYPE_HW			Hw8					// SINGLE transfer with level-triggered detection
#define	HwCHCTRL1_TYPE_SW			Hw9					// HW transfer
#define	HwCHCTRL1_TYPE_SL	        		(Hw9|Hw8)			// SW transfer
#define	HwCHCTRL1_BSIZE_1				~(Hw7|Hw6)			// 1 Burst transfer consists of 1 read or write cycle
#define	HwCHCTRL1_BSIZE_2				Hw6					// 1 Burst transfer consists of 2 read or write cycles
#define	HwCHCTRL1_BSIZE_4				Hw7					// 1 Burst transfer consists of 4 read or write cycles
#define	HwCHCTRL1_BSIZE_8				(Hw6|Hw7)			// 1 Burst transfer consists of 8 read or write cycles
#define	HwCHCTRL1_WSIZE_8			~(Hw5|Hw4)			// Each cycle read or write 8bit data
#define	HwCHCTRL1_WSIZE_16			Hw4					// Each cycle read or write 16bit data
#define	HwCHCTRL1_WSIZE_32			Hw5					// Each cycle read or write 32bit data
#define	HwCHCTRL1_FLAG				Hw3					// Clears FLAG to 0
#define	HwCHCTRL1_IEN_EN				Hw2					// At the same time the FLAG goes to 1, DMA interrupt request is generated
#define	HwCHCTRL1_REP_EN				Hw1					// The DMA channel remains enabled
#define	HwCHCTRL1_EN_EN		        	Hw0					// DMA channel is Enabled

#define	HwCHCTRL2_DMASEL_TIMER_ON	Hw31				// Timer DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_UART1_TX	Hw30				// UART1 Transmite DMA Interrupt Enable
#define	HwCHCTRL2_DMASEL_UART1_RX	Hw29				// UART1 Receive DMA Interrupt Enable
#define	HwCHCTRL2_DMASEL_LCD			Hw28				// LCD DMA Interrupt Enable
#define	HwCHCTRL2_DMASEL_SDMMC		Hw27				// SD/MMC DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_ECC			Hw26				// ECC DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_SPI			Hw25				// SPI DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_NFC			Hw24				// NAND DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_UART0		Hw23				// UART0 DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_GSIO		Hw22				// GSIO DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_I2S_TX		Hw21				// I2S Transmite DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_I2S_RX		Hw20				// I2S Receive DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_EXT3_ON	Hw19				// External Interrupt 3 DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_EXT2_ON	Hw18				// External Interrupt 2 DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_EXT1_ON	Hw17				// External Interrupt 1 DMA Interrupte Enable
#define	HwCHCTRL2_DMASEL_EXT0_ON	Hw16				// External Interrupt 0 DMA Interrupte Enable

#define	HwCHCTRL2_CONT_C				Hw15				// DMA transfer begins from C_SADR/C_DADR Address. It must be used after the former transfer has been executed, so that C_SADR and C_DADR contain a meaningful value.
#define	HwCHCTRL2_DTM_EN				Hw14				// Differential Transfer Mode Enable
#define	HwCHCTRL2_SYNC_EN			Hw13				// Synchronize HardWare Request
#define	HwCHCTRL2_HRD_W				Hw12				// ACK/EOT signals are issued When DMA-Write Operation
#define	HwCHCTRL2_LOCK_EN			Hw11				// DMA transfer executed with lock transfer
#define	HwCHCTRL2_BST_NOARB			Hw10				// DMA transfer executed with no arbitration(burst operation)
#define	HwCHCTRL2_TYPE_SE	        		~(Hw9|Hw8)			// SINGLE transfer with edge-triggered detection
#define	HwCHCTRL2_TYPE_HW			Hw8					// SINGLE transfer with level-triggered detection
#define	HwCHCTRL2_TYPE_SW			Hw9					// HW transfer
#define	HwCHCTRL2_TYPE_SL	        		(Hw9|Hw8)			// SW transfer
#define	HwCHCTRL2_BSIZE_1				~(Hw7|Hw6)			// 1 Burst transfer consists of 1 read or write cycle
#define	HwCHCTRL2_BSIZE_2				Hw6					// 1 Burst transfer consists of 2 read or write cycles
#define	HwCHCTRL2_BSIZE_4				Hw7					// 1 Burst transfer consists of 4 read or write cycles
#define	HwCHCTRL2_BSIZE_8				(Hw6|Hw7)			// 1 Burst transfer consists of 8 read or write cycles
#define	HwCHCTRL2_WSIZE_8			~(Hw5|Hw4)			// Each cycle read or write 8bit data
#define	HwCHCTRL2_WSIZE_16			Hw4					// Each cycle read or write 16bit data
#define	HwCHCTRL2_WSIZE_32			Hw5					// Each cycle read or write 32bit data
#define	HwCHCTRL2_FLAG				Hw3					// Clears FLAG to 0
#define	HwCHCTRL2_IEN_EN				Hw2					// At the same time the FLAG goes to 1, DMA interrupt request is generated
#define	HwCHCTRL2_REP_EN				Hw1					// The DMA channel remains enabled
#define	HwCHCTRL2_EN_EN		        	Hw0					// DMA channel is Enabled

#define	HwCHCTRL_TIM_ON				Hw31				// Timer DMA Interrupte Enable
#define	HwCHCTRL_TIM_OFF				~Hw31				// Timer DMA Interrupt Disable
#define	HwCHCTRL_UA1T_ON				Hw30				// UART1 Transmite DMA Interrupt Enable
#define	HwCHCTRL_UA1T_OFF			~Hw30				// UART1 Transmite DMA Interrupt Disable
#define	HwCHCTRL_UA1R_ON				Hw29				// UART1 Receive DMA Interrupt Enable
#define	HwCHCTRL_UA1R_OFF			~Hw29				// UART1 Receive DMA Interrupt Disable
#define	HwCHCTRL_DMASEL_TIMER_ON	Hw31				// Timer DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_UART1_TX	Hw30				// UART1 Transmite DMA Interrupt Enable
#define	HwCHCTRL_DMASEL_UART1_RX	Hw29				// UART1 Receive DMA Interrupt Enable
#define	HwCHCTRL_LCD_ON				Hw28				// LCD DMA Interrupt Enable
#define	HwCHCTRL_SD_ON				Hw27				// SD/MMC DMA Interrupte Enable
#define	HwCHCTRL_ECC_ON				Hw26				// ECC DMA Interrupte Enable
#define	HwCHCTRL_SPI_ON				Hw25				// SPI DMA Interrupte Enable
#define	HwCHCTRL_ND_ON				Hw24				// NAND DMA Interrupte Enable
#define	HwCHCTRL_UA0_ON				Hw23				// UART0 DMA Interrupte Enable
#define	HwCHCTRL_GSIO_ON				Hw22				// GSIO DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_LCD			Hw28				// LCD DMA Interrupt Enable
#define	HwCHCTRL_DMASEL_SDMMC		Hw27				// SD/MMC DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_ECC			Hw26				// ECC DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_SPI			Hw25				// SPI DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_NFC			Hw24				// NAND DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_UART0		Hw23				// UART0 DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_GSIO			Hw22				// GSIO DMA Interrupte Enable
#define	HwCHCTRL_I2ST_ON				Hw21				// I2S Transmite DMA Interrupte Enable
#define	HwCHCTRL_I2SR_ON				Hw20				// I2S Receive DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_DAIT			Hw21				// Connected Hardware = DAI Transmitter
#define	HwCHCTRL_DMASEL_DAIR			Hw20				// Connected Hardware = DAI Receiver
#define	HwCHCTRL_EXT3_ON				Hw19				// External Interrupt 3 DMA Interrupte Enable
#define	HwCHCTRL_EXT2_ON				Hw18				// External Interrupt 2 DMA Interrupte Enable
#define	HwCHCTRL_EXT1_ON				Hw17				// External Interrupt 1 DMA Interrupte Enable
#define	HwCHCTRL_EXT0_ON				Hw16				// External Interrupt 0 DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_EXT3_ON		Hw19				// External Interrupt 3 DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_EXT2_ON		Hw18				// External Interrupt 2 DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_EXT1_ON		Hw17				// External Interrupt 1 DMA Interrupte Enable
#define	HwCHCTRL_DMASEL_EXT0_ON		Hw16				// External Interrupt 0 DMA Interrupte Enable

#define	HwCHCTRL_CONT_C				Hw15				// DMA transfer begins from C_SADR/C_DADR Address. It must be used after the former transfer has been executed, so that C_SADR and C_DADR contain a meaningful value.
#define	HwCHCTRL_CONT_ST				(0)					// DMA trnaster begins from ST_SADR/ST_DADR Address
#define	HwCHCTRL_DTM_EN				Hw14				// Differential Transfer Mode Enable
#define	HwCHCTRL_DTM_ON				Hw14				// Differential Transfer Mode Enable
#define	HwCHCTRL_DTM_OFF				(0)					// Differential Transfer Mode Disable
#define	HwCHCTRL_SYNC_ON				Hw13				// Synchronize HardWare Request
#define	HwCHCTRL_SYNC_OFF			(0)					// Do not Synchronize HardWare Request
#define 	HwCHCTRL_SYNC_EN				Hw13				// Synchronize Hardware Request
#define	HwCHCTRL_HRD_W				Hw12				// ACK/EOT signals are issued When DMA-Write Operation
#define	HwCHCTRL_LOCK_EN				Hw11				// DMA transfer executed with lock transfer
#define	HwCHCTRL_BST_NOARB			Hw10				// DMA transfer executed with no arbitration(burst operation)
#define	HwCHCTRL_HRD_WR				Hw12				// ACK/EOT signals are issued When DMA-Write Operation
#define	HwCHCTRL_HRD_RD				(0)					// ACK/EOT signals are issued When DMA-Read Operation
#define	HwCHCTRL_LOCK_ON				Hw11				// DMA transfer executed with lock transfer
#define	HwCHCTRL_LOCK_OFF			(0)					//
#define	HwCHCTRL_BST_BURST			Hw10				// DMA transfer executed with no arbitration(burst operation)
#define	HwCHCTRL_BST_ARB				(0)					// DMA transfer executed wth arbitration
#define	HwCHCTRL_TYPE_SINGE			(0)					// SINGLE transfer with edge-triggered detection
#define	HwCHCTRL_TYPE_HW				Hw8					// HW Transfer
#define	HwCHCTRL_TYPE_SW				Hw9					// SW transfer
#define	HwCHCTRL_TYPE_SINGL			(Hw9|Hw8)			// SINGLE transfer with level-triggered detection
#define	HwCHCTRL_TYPE_SL				(Hw9|Hw8)			// SINGLE transfer with level-triggered detection
#define 	HwCHCTRL_TYPE_SE				HwZERO				// SINGLE transfer with edge-triggered detection
#define	HwCHCTRL_BSIZE_1				(0)					// 1 Burst transfer consists of 1 read or write cycle
#define	HwCHCTRL_BSIZE_2				Hw6					// 1 Burst transfer consists of 2 read or write cycles
#define	HwCHCTRL_BSIZE_4				Hw7					// 1 Burst transfer consists of 4 read or write cycles
#define	HwCHCTRL_BSIZE_8				(Hw6|Hw7)			// 1 Burst transfer consists of 8 read or write cycles
#define	HwCHCTRL_WSIZE_8				(0)					// Each cycle read or write 8bit data
#define	HwCHCTRL_WSIZE_16			Hw4					// Each cycle read or write 16bit data
#define	HwCHCTRL_WSIZE_32			Hw5					// Each cycle read or write 32bit data
#define	HwCHCTRL_FLAG					Hw3					// Clears FLAG to 0
#define	HwCHCTRL_IEN_ON				Hw2					// At the same time the FLAG goes to 1, DMA interrupt request is generated
#define HwCHCTRL_IEN_EN				Hw2					// At the same time the FLAG goes to 1, DMA interrupt request is generated
#define	HwCHCTRL_IEN_OFF			~Hw2				//
#define	HwCHCTRL_REP_EN				Hw1					// The DMA channel remains enabled
#define	HwCHCTRL_REP_DIS				~Hw1				// After all of hop transfer has executed, the DMA channel is disabled
#define	HwCHCTRL_EN_ON				Hw0					// DMA channel is Enabled
#define	HwCHCTRL_EN_OFF				~Hw0				// DMA channel is terminated and disabled/*}}}*/
#define 	HwCHCTRL_EN_EN				Hw0					// DMA channel is enabled. If software type transfer is selected, this bit generates DMA request directly, or if hardware type transfer is used, the selected interrupt request flag generate DMA request
 

#define	Hw3RPTCTRL0					*(volatile unsigned long *)0xF0004028	// R/W, Repeat Control Register
#define	Hw3RPTCTRL1					*(volatile unsigned long *)0xF0004058	// R/W, Repeat Control Register
#define	Hw3RPTCTRL2					*(volatile unsigned long *)0xF0004088	// R/W, Repeat Control Register
#define	Hw3RPTCTRL_USB				*(volatile unsigned long *)0xF0008828	// R/W, Repeat Control Register
#define 	Hw3RPTCTRL_DRI				Hw31				// R/W DMA interrupt is occured when the last DMA repeated DMA operation. [0:each, 1:last]
#define 	Hw3RPTCTRL_EOT				Hw30				// R/W EOT Signal is occured when the last repeated DMA operation in HW(including single) transfer mode

#define	HwRPTCTRL0						*(volatile unsigned long *)0xF0004028	// R/W, Repeat Control Register
#define 	HwRPTCTRL0_DRI_LAST		       Hw31				// R/W DMA interrupt is occured when the last DMA repeated DMA operation. [0:each, 1:last]
#define 	HwRPTCTRL0_DEOT_LAST		       Hw30				// R/W EOT Signal is occured when the last repeated DMA operation in HW(including single) transfer mode

#define	HwRPTCTRL1						*(volatile unsigned long *)0xF0004058	// R/W, Repeat Control Register
#define 	HwRPTCTRL1_DRI_LAST			Hw31				// DAM interrupt occur is  occurred at the last DMA Repeated DMA operation
#define 	HwRPTCTRL1_DEOT_LAST			Hw30				// EOT signal is occurred at the last Repeated DAM operation in HW(including Single) transfer Mode

#define	HwRPTCTRL2						*(volatile unsigned long *)0xF0004088	// R/W, Repeat Control Register
#define 	HwRPTCTRL2_DRI_LAST			Hw31				// DAM interrupt occur is  occurred at the last DMA Repeated DMA operation
#define 	HwRPTCTRL2_DEOT_LAST			Hw30	

#define	HwRPTCTRL_USB					*(volatile unsigned long *)0xF0008828	// R/W, Repeat Control Register

/////////////////
// DMA1 Channel 0/1/2
/////////////////
#define	HwDMA1_ST_SADR0		*(volatile unsigned long *)0xF000A000	// DMA1:R/W, Start Address of Source Block Register
#define	HwDMA1_ST_SADR1		*(volatile unsigned long *)0xF000A030	// DMA1:R/W, Start Address of Source Block Register
#define	HwDMA1_ST_SADR2		*(volatile unsigned long *)0xF000A060	// DMA1:R/W, Start Address of Source Block Register

#define	HwDMA1_ST_DADR0		*(volatile unsigned long *)0xF000A010	// DMA1:R/W, Start Address of Destination Block Register
#define	HwDMA1_ST_DADR1		*(volatile unsigned long *)0xF000A040	// DMA1:R/W, Start Address of Destination Block Register
#define	HwDMA1_ST_DADR2		*(volatile unsigned long *)0xF000A070	// DMA1:R/W, Start Address of Destination Block Register

#define	HwDMA1_SPARAM0		*(volatile unsigned long *)0xF000A004	// DMA1:R/W, Parameter of Source Block Register
#define	HwDMA1_SPARAM1		*(volatile unsigned long *)0xF000A034	// DMA1:R/W, Parameter of Source Block Register
#define	HwDMA1_SPARAM2		*(volatile unsigned long *)0xF000A064	// DMA1:R/W, Parameter of Source Block Register

#define	HwDMA1_DPARAM0		*(volatile unsigned long *)0xF000A014	// DMA1:R/W, Parameter of Destination Block Register
#define	HwDMA1_DPARAM1		*(volatile unsigned long *)0xF000A044	// DMA1:R/W, Parameter of Destination Block Register
#define	HwDMA1_DPARAM2		*(volatile unsigned long *)0xF000A074	// DMA1:R/W, Parameter of Destination Block Register

#define	HwDMA1_C_SADR0		*(volatile unsigned long *)0xF000A00C	// DMA1:R, Current Address of Source Block Register
#define	HwDMA1_C_SADR1		*(volatile unsigned long *)0xF000A03C	// DMA1:R, Current Address of Source Block Register
#define	HwDMA1_C_SADR2		*(volatile unsigned long *)0xF000A06C	// DMA1:R, Current Address of Source Block Register

#define	HwDMA1_C_DADR0		*(volatile unsigned long *)0xF000A01C	// DMA1:R, Current Address of Destination Block Register
#define	HwDMA1_C_DADR1		*(volatile unsigned long *)0xF000A04C	// DMA1:R, Current Address of Destination Block Register
#define	HwDMA1_C_DADR2		*(volatile unsigned long *)0xF000A07C	// DMA1:R, Current Address of Destination Block Register

#define	HwDMA1_HCOUNT0		*(volatile unsigned long *)0xF000A020	// DMA1:R/W, Initial and Current Hop Count Register
#define	HwDMA1_HCOUNT1		*(volatile unsigned long *)0xF000A050	// DMA1:R/W, Initial and Current Hop Count Register
#define	HwDMA1_HCOUNT2		*(volatile unsigned long *)0xF000A080	// DMA1:R/W, Initial and Current Hop Count Register

#define	HwDMA1_CHCTRL0		*(volatile unsigned long *)0xF000A024	// DMA1:R/W, Channel Control Register
#define	HwDMA1_CHCTRL1		*(volatile unsigned long *)0xF000A054	// DMA1:R/W, Channel Control Register
#define	HwDMA1_CHCTRL2		*(volatile unsigned long *)0xF000A084	// DMA1:R/W, Channel Control Register

#define	HwDMA1_RPTCTRL0		*(volatile unsigned long *)0xF000A028	// DMA1:R/W, Repeat Control Register
#define	HwDMA1_RPTCTRL1		*(volatile unsigned long *)0xF000A058	// DMA1:R/W, Repeat Control Register
#define	HwDMA1_RPTCTRL2		*(volatile unsigned long *)0xF000A088	// DMA1:R/W, Repeat Control Register

/////////////////
// DMA0/DMA1 External DMA Request Register
/////////////////
#define	HwEXTREQ0				*(volatile unsigned long *)0xF000402C	// R/W, Repeat Control Register
#define	HwEXTREQ1				*(volatile unsigned long *)0xF000405C	// R/W, Repeat Control Register
#define	HwEXTREQ2				*(volatile unsigned long *)0xF000408C	// R/W, Repeat Control Register

#define	HwDMA1_EXTREQ0		*(volatile unsigned long *)0xF000A02C	// DMA1:R/W, Repeat Control Register
#define	HwDMA1_EXTREQ1		*(volatile unsigned long *)0xF000A05C	// DMA1:R/W, Repeat Control Register
#define	HwDMA1_EXTREQ2		*(volatile unsigned long *)0xF000A08C	// DMA1:R/W, Repeat Control Register

#define	HwEXTREQ_I2C1			Hw30							// I2C1 DREQ
#define	HwEXTREQ_I2C0			Hw29							// I2C0 DREQ
#define	HwEXTREQ_EINT7		Hw28							// External Interrupt 7
#define	HwEXTREQ_EINT6		Hw27							// External Interrupt 6
#define	HwEXTREQ_EINT5		Hw26							// External Interrupt 5
#define	HwEXTREQ_EINT4		Hw25							// External Interrupt 4
#define	HwEXTREQ_SF_USR		Hw23							// SPDIF USR DREQ
#define	HwEXTREQ_SF_PKT		Hw22							// SPDIF PKT DREQ
#define	HwEXTREQ_UT3_RX		Hw21							// UART3 Receive
#define	HwEXTREQ_UT3_TX		Hw20							// UART3 Transmite
#define	HwEXTREQ_UT2_RX		Hw19							// UART2 Receive
#define	HwEXTREQ_UT2_TX		Hw18							// UART2 Transmite
#define	HwEXTREQ_UT1_TX		Hw17							// UART1 Transmite
#define	HwEXTREQ_UT0_TX		Hw16							// UART1 Transmite
#define	HwEXTREQ_TC			Hw15							// TC DMA Request
#define	HwEXTREQ_UT1_RX		Hw14							// UART1 Receive
#define	HwEXTREQ_UT0_RX		Hw13							// UART0 Receive
#define	HwEXTREQ_SP1_RX		Hw12							// SPII Receive
#define	HwEXTREQ_SP1_TX		Hw11							// SPII Transmite
#define	HwEXTREQ_ECC			Hw10							// ECC Controller
#define	HwEXTREQ_SP0_RX		Hw9								// SPI0 Receive
#define 	HwEXTREQ_NFC			Hw8								// NFC Controller
#define 	HwEXTREQ_CD_RX		Hw7								// CD Receive
#define	HwEXTREQ_SP0_TX		Hw6								// SPI0 Transmite
#define 	HwEXTREQ_I2S_TX		Hw5								// I2S Transmite
#define 	HwEXTREQ_I2S_RX		Hw4								// I2S Receive
#define 	HwEXTREQ_EINT3		Hw3								// External Interrupt3
#define 	HwEXTREQ_EINT2		Hw2								// External Interrupt2
#define 	HwEXTREQ_EINT1		Hw1								// External Interrupt1
#define 	HwEXTREQ_EINT0		Hw0								// External Interrupt0	


/* TCC82XX
typedef	volatile struct {
	unsigned	ST_SADR;
	unsigned	SPARAM;
	unsigned	_SPARAM;
	unsigned	C_SADR;
	unsigned	ST_DADR;
	unsigned	DPARAM;
	unsigned	_DPARAM;
	unsigned	C_DADR;
	unsigned	HCOUNT;
	unsigned	CHCTRL;
	unsigned	RPTCTRL;
} sHwDMA;
*/
typedef	volatile struct {
	unsigned	ST_SADR;
	unsigned	SPARAM;
	unsigned	_SPARAM;
	unsigned	C_SADR;
	unsigned	ST_DADR;
	unsigned	DPARAM;
	unsigned	_DPARAM;
	unsigned	C_DADR;
	unsigned	HCOUNT;
	unsigned	CHCTRL;
	unsigned	RPTCTRL;
	unsigned EXTREQ;
} sHwDMA;


//Done ...
//////////

/***********************************************************************
*	 NAND Flash Controller Register Define				(Base Addr = 0xF1000000)
************************************************************************/
///////////
//Done...

typedef	volatile struct {
	unsigned	CMD;
	unsigned	LADR;
	unsigned	BADR;
	unsigned	SADR;
	union {
		unsigned char		D8;
		unsigned short	D16;
		unsigned	int		D32;
	} WDATA;
	unsigned	uDummy[3+8];
	union {
		unsigned char		D8;
		unsigned	short	D16;
		unsigned	int		D32;
	} SDATA;
} sHwND;

#define	HwNFC_CMD							*(volatile unsigned long *)0xF000D000	// W, Nand Flash Command Register

#define	HwNFC_LADDR							*(volatile unsigned long *)0xF000D004	// W, Nand Flash Linear Address Register

#define	HwNFC_BADDR							*(volatile unsigned long *)0xF000D008	// W, Nand Flash Block Address Register

#define	HwNFC_SADDR							*(volatile unsigned long *)0xF000D00C	// W, Nand Flash Signal Address Register

#define	HwNFC_WDATA							*(volatile unsigned long *)0xF000D010	// R/W, Nand Flash Word Data Register

#define	HwNFC_LDATA							*(volatile unsigned long *)0xF000D020	// R/W, Nand Flash Linear Data Register

#define	HwNFC_SDATA							*(volatile unsigned long *)0xF000D040	// R/W, Nand Flash Single Data Register

#define	HwNFC_CTRL							*(volatile unsigned long *)0xF000D050	// R/W, Nand Flash Control Register
#define	HwNFC_CTRL_RDYIEN_EN				Hw31					///*{{{*/
#define	HwNFC_CTRL_RDYIEN_DIS				~Hw31					//
#define	HwNFC_CTRL_PROGIEN_EN				Hw30					//
#define	HwNFC_CTRL_PROGIEN_DIS				~Hw30					//
#define	HwNFC_CTRL_READIEN_EN				Hw29					//
#define	HwNFC_CTRL_READIEN_DIS				~Hw29					//
#define	HwNFC_CTRL_DEN_EN					Hw28					//
#define	HwNFC_CTRL_DEN_DIS					~Hw28					//
#define	HwNFC_CTRL_FS_RDY					Hw27					//
#define	HwNFC_CTRL_FS_BUSY					~Hw27					//
#define	HwNFC_CTRL_BW_16					Hw26					//
#define	HwNFC_CTRL_BW_8						0						//

#define	HwNFC_CTRL_CFG_nCS3					Hw25					//
#define	HwNFC_CTRL_CFG_nCS2					Hw24					//
#define	HwNFC_CTRL_CFG_nCS1					Hw23					//
#define	HwNFC_CTRL_CFG_nCS0					Hw22					//
#define	HwNFC_CTRL_CFG_NOACT				(Hw25|Hw24|Hw23|Hw22)	//

#define	HwNFC_CTRL_RDY_RDY					Hw21					//
#define	HwNFC_CTRL_RDY_BUSY					~Hw21					//

#define	HwNFC_CTRL_BSIZE_1					0						//
#define	HwNFC_CTRL_BSIZE_2					Hw19					//
#define	HwNFC_CTRL_BSIZE_4					Hw20					//
#define	HwNFC_CTRL_BSIZE_8					(Hw20|Hw19)				//

#define	HwNFC_CTRL_PSIZE_256				0						//
#define	HwNFC_CTRL_PSIZE_512				Hw16					//
#define	HwNFC_CTRL_PSIZE_1024				Hw17					//
#define	HwNFC_CTRL_PSIZE_2048				(Hw17|Hw16)				//
#define	HwNFC_CTRL_PSIZE_4096				(Hw18|Hw17|Hw16)		//

#define	HwNFC_CTRL_MSK						Hw15
#define	HwNFC_CTRL_MSK_NOT					~Hw15

#define	HwNFC_CTRL_CADDR					Hw12

#define HwNFC_CTRL_STA_RDY					0		// Dummy

#define	HwNFC_PSTART						*(volatile unsigned long *)0xF000D054	// W, Nand Flash Program Start Register

#define	HwNFC_RSTART						*(volatile unsigned long *)0xF000D058	// W, Nand Flash Read Start Register

#define	HwNFC_DSIZE							*(volatile unsigned long *)0xF000D05C	// R/W, Nand Flash Data Size Register

#define	HwNFC_IREQ							*(volatile unsigned long *)0xF000D060	// R/W, Nand Flash Interrupt Request Register
#define	HwNFC_IREQ_FLAG2					Hw6						//
#define	HwNFC_IREQ_FLAG1					Hw5						//
#define	HwNFC_IREQ_FLAG0					Hw4						//
#define	HwNFC_IREQ_IRQ2						Hw2						// Ready Interrupt
#define	HwNFC_IREQ_IRQ1						Hw1						// Program Interrupt
#define	HwNFC_IREQ_IRQ0						Hw0						// Reading Interrupt

#define	HwNFC_RST							*(volatile unsigned long *)0xF000D064	// W, Nand Flash Controller Reset Register

//#define	HwNFC_CTRL1			*(volatile unsigned long *)0xF000D068	// W, Nand Flash Control register 1

//Done...
/////////


/***********************************************************************
*	 SD/MMC Controller Register Define				(Base Addr = 0xF1001000)
************************************************************************/
#define HwSDMMC_CH0_BASE          *(volatile unsigned long *)0xF000E000   // R/W, SDMMC Channel 0 Base Register
//#define HwSDMMC_CH0_BASE          0xF000E000   // R/W, SDMMC Channel 0 Base Register
// SLOT0
#define HwSD0_SDMA_ADDR			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x000) 
#define HwSD0_SDMA_ADDR_LOW		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x000) 
#define HwSD0_SDMA_ADDR_HIGH	*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x002) 

#define HwSD0_BLOCK				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x004) 
#define HwSD0_BLOCK_SIZE		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x004) 
#define HwSD0_BLOCK_COUNT		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x006) 

#define HwSD0_ARGUMENT			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x008) 
#define HwSD0_ARGUMENT0			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x008) 
#define HwSD0_ARGUMENT1			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x00A) 

#define HwSD0_COM_TRANS			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x00C) 
#define HwSD_COM_TRANS_DATSEL	Hw21		// data present select
#define HwSD_COM_TRANS_CICHK	Hw20		// command index check enable
#define HwSD_COM_TRANS_CRCHK	Hw19		// command CRC check enable
#define HwSD_COM_TRANS_SPI		Hw7			// SPI mode
#define HwSD_COM_TRANS_ATACMD	Hw6			// cmd completion enable for CE-ATA
#define HwSD_COM_TRANS_MS		Hw5			// multi/single block select
#define HwSD_COM_TRANS_DIR		Hw4			// data transfer direction select
#define HwSD_COM_TRANS_ACMD12	Hw2			// auto CMD12 enable
#define HwSD_COM_TRANS_BCNTEN	Hw1			// block count enable
#define HwSD_COM_TRANS_DMAEN	Hw0			// DMA Enable

#define HwSD0_TRANS_MODE		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x00C)
#define HwSD0_COMMAND			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x00E) 

#define HwSD0_RESPONSE1_0		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x010) 
#define HwSD0_RESPONSE0			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x010) 
#define HwSD0_RESPONSE1			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x012) 

#define HwSD0_RESPONSE3_2		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x014) 
#define HwSD0_RESPONSE2			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x014) 
#define HwSD0_RESPONSE3			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x016) 

#define HwSD0_RESPONSE5_4		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x018) 
#define HwSD0_RESPONSE4			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x018) 
#define HwSD0_RESPONSE5			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x01A) 

#define HwSD0_RESPONSE7_6		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x01C) 
#define HwSD0_RESPONSE6			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x01C) 
#define HwSD0_RESPONSE7			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x01E) 

#define HwSD0_BUFFER			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x020) 
#define HwSD0_BUFFER0			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x020) 
#define HwSD0_BUFFER1			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x022) 

#define HwSD0_STATE				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x024)
#define HwSD_STATE_NODAT		Hw1			// data inhibit
#define HwSD_STATE_NOCMD		Hw0			// command inhibit

#define HwSD0_CONT1				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x028) 
#define HwSD0_POWER				*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x028)
#define HwSD_POWER_POW			Hw8			// SD bus power
#define HwSD_POWER_SD8			Hw5			// SD 8-bit mode
#define HwSD_POWER_HS			Hw2			// high speed enable
#define HwSD_POWER_SD4			Hw1			// data transfer width

#define HwSD0_WAKEUP			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x02A) 

#define HwSD0_CONT2				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x02C) 
#define HwSD0_CLOCK				*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x02C) 
#define	HwSDCLKSEL_DIV_256		0x80
#define	HwSDCLKSEL_DIV_128		0x40
#define	HwSDCLKSEL_DIV_64		0x20
#define	HwSDCLKSEL_DIV_32		0x10
#define	HwSDCLKSEL_DIV_16		0x08
#define	HwSDCLKSEL_DIV_8		0x04
#define	HwSDCLKSEL_DIV_4		0x02
#define	HwSDCLKSEL_DIV_2		0x01
#define	HwSDCLKSEL_DIV_0		0x00
#define HwSDCLKSEL_SCK_EN		Hw2
#define HsSDCLKSEL_INCLK_STABLE	Hw1
#define HwSDCLKSEL_INCLK_EN		Hw0

#define HwSD0_TIMEOUT			*(volatile unsigned char *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x02E) 
#define HwSD0_SRESET			*(volatile unsigned char *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x02F)
#define HwSD_SRESET_RSTALL		Hw0		// software reset for All
#define HwSD_SRESET_RSTCMD		Hw1		// software reset for CMD line
#define HwSD_SRESET_RSTDAT		Hw2		// software reset for DAT line

#define HwSD0_INT_STATUS		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x030)
#define HwSDINT_STATUS_DATEND	Hw22		// data end bit error
#define HwSDINT_STATUS_DATCRC	Hw21		// data crc error
#define HwSDINT_STATUS_DATTIME	Hw20		// data timeout error
#define HwSDINT_STATUS_CINDEX	Hw19		// command index error
#define HwSDINT_STATUS_CMDEND	Hw18		// command command end bit error
#define HwSDINT_STATUS_CMDCRC	Hw17		// command crc error
#define HwSDINT_STATUS_CMDTIME	Hw16		// command timeout error
#define HwSDINT_STATUS_ERR		Hw15		// error interrupt
#define HwSDINT_STATUS_CDINT	Hw8			// card interrupt
#define HwSDINT_STATUS_CDOUT	Hw7			// card removal
#define HwSDINT_STATUS_CDIN		Hw6			// card insertion
#define HwSDINT_STATUS_RDRDY	Hw5			// buffer read ready
#define HwSDINT_STATUS_WRRDY	Hw4			// buffer write ready
#define HwSDINT_STATUS_DMA		Hw3			// DMA interrupt
#define	HwSDINT_STATUS_BLKGAP	Hw2			// block gap event
#define HwSDINT_STATUS_TDONE	Hw1			// transfer complete
#define HwSDINT_STATUS_CDONE	Hw0			// command complete

#define HwSD0_NORMAL_INT		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x030) 
#define HwSD0_ERROR_INT			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x032) 

#define HwSD0_INT_STATUS_EN		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x034)
#define HwSDINT_EN_ADMA			Hw25		// ADMA error signal enable
#define HwSDINT_EN_ACMD12		Hw24		// auto CMD12 error signal enable
#define HwSDINT_EN_CLIMIT		Hw23		// current limit error signal enable
#define HwSDINT_EN_DATEND		Hw22		// data end bit error signal enable
#define HwSDINT_EN_DATCRC		Hw21		// data crc error signal enable
#define HwSDINT_EN_DATTIME		Hw20		// data timeout error signal enable
#define HwSDINT_EN_CINDEX		Hw19		// command index error signal enable
#define HwSDINT_EN_CMDEND		Hw18		// command end bit error signal enable
#define HwSDINT_EN_CMDCRC		Hw17		// command crc error signal enable
#define HwSDINT_EN_CMDTIME		Hw16		// command timeout error signal enable
#define HwSDINT_EN_CDINT		Hw8			// card interrupt signal enable
#define HwSDINT_EN_CDOUT		Hw7			// card removal signal enable
#define HwSDINT_EN_CDIN			Hw6			// card insertion signal enable
#define HwSDINT_EN_RDRDY		Hw5			// buffer read ready signal enable
#define HwSDINT_EN_WRRDY		Hw4			// buffer write ready signal enable
#define HwSDINT_EN_DMA			Hw3			// DMA interrupt signal enable
#define HwSDINT_EN_BLKGAP		Hw2			// block gap event signal enable
#define HwSDINT_EN_TDONE		Hw1			// transfer complete signal enable
#define HwSDINT_EN_CDONE		Hw0			// command complete signal enable

#define HwSD0_NORMAL_INT_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x034) 
#define HwSD0_ERROR_INT_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x036) 

#define HwSD0_INT_SIGNAL_EN		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x038) 
#define HwSD0_NORMAL_SIG_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x038) 
#define HwSD0_ERROR_SIG_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x03A) 

#define HwSD0_CMD12_ERROR		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x03C) 

#define HwSD0_CAPABILITIES		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x040) 

#define HwSD0_MAX_CAP			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x048) 

#define HwSD0_FORCE_EVENT		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x050) 
#define HwSD0_FOR_CMD12_ERR		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x050) 
#define HwSD0_FOR_ERR_INT		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x052) 

#define HwSD0_ADMA_ERROR		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x054) 

#define HwSD0_ADMA_ADDR_LOW		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x058) 
#define HwSD0_ADMA_ADDR_HIGH	*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x05C) 

#define HwSD0_SLOT_STATUS		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x0FC) 
#define HwSD0_SLOT_INT			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x0FC) 
#define HwSD0_HOST_VER			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x0FE) 

// SLOT1
#define HwSD1_SDMA_ADDR			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x100) 
#define HwSD1_SDMA_ADDR_LOW		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x100) 
#define HwSD1_SDMA_ADDR_HIGH	*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x102) 

#define HwSD1_BLOCK				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x104) 
#define HwSD1_BLOCK_SIZE		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x104) 
#define HwSD1_BLOCK_COUNT		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x106) 

#define HwSD1_ARGUMENT			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x108) 
#define HwSD1_ARGUMENT0			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x108) 
#define HwSD1_ARGUMENT1			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x10A) 

#define HwSD1_COM_TRANS			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x10C) 
#define HwSD1_TRANS_MODE		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x10C) 
#define HwSD1_COMMAND			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x10E) 

#define HwSD1_RESPONSE1_0		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x110) 
#define HwSD1_RESPONSE0			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x110) 
#define HwSD1_RESPONSE1			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x112) 

#define HwSD1_RESPONSE3_2		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x114) 
#define HwSD1_RESPONSE2			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x114) 
#define HwSD1_RESPONSE3			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x116) 

#define HwSD1_RESPONSE5_4		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x118) 
#define HwSD1_RESPONSE4			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x118) 
#define HwSD1_RESPONSE5			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x11A) 

#define HwSD1_RESPONSE7_6		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x11C) 
#define HwSD1_RESPONSE6			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x11C) 
#define HwSD1_RESPONSE7			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x11E) 

#define HwSD1_BUFFER			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x120) 
#define HwSD1_BUFFER0			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x120) 
#define HwSD1_BUFFER1			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x122) 

#define HwSD1_STATE				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x124) 

#define HwSD1_CONT1				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x128) 
#define HwSD1_POWER				*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x128) 
#define HwSD1_WAKEUP			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x12A) 

#define HwSD1_CONT2				*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x12C) 
#define HwSD1_CLOCK				*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x12C) 
#define HwSD1_TIMEOUT			*(volatile unsigned char *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x12E) 
#define HwSD1_SRESET			*(volatile unsigned char *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x12F) 

#define HwSD1_INT_STATUS		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x130) 
#define HwSD1_NORMAL_INT		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x130) 
#define HwSD1_ERROR_INT			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x132) 

#define HwSD1_INT_STATUS_EN		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x134) 
#define HwSD1_NORMAL_INT_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x134) 
#define HwSD1_ERROR_INT_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x136) 

#define HwSD1_INT_SIGNAL_EN		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x138) 
#define HwSD1_NORMAL_SIG_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x138) 
#define HwSD1_ERROR_SIG_EN		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x13A) 

#define HwSD1_CMD12_ERROR		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x13C) 

#define HwSD1_CAPABILITIES		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x140) 

#define HwSD1_MAX_CAP			*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x148) 

#define HwSD1_FORCE_EVENT		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x150) 
#define HwSD1_FOR_CMD12_ERR		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x150) 
#define HwSD1_FOR_ERR_INT		*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x152) 

#define HwSD1_ADMA_ERROR		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x154) 

#define HwSD1_ADMA_ADDR_LOW		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x158) 
#define HwSD1_ADMA_ADDR_HIGH	*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x15C) 

#define HwSD1_SLOT_STATUS		*(volatile unsigned int *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x1FC) 
#define HwSD1_SLOT_INT			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x1FC) 
#define HwSD1_HOST_VER			*(volatile unsigned short *)(((unsigned)&HwSDMMC_CH0_BASE) + 0x1FE) 


#define HwSDMMC_CH1_BASE          *(volatile unsigned long *)0xF000E100   // R/W, SDMMC Channel 1 Base Register

#define HwSDMMC_BASE(X)           *(volatile unsigned long *)(0xF000E000+(X)*0x100)       // R/W, SDMMC Channel Base Register

typedef volatile struct 
{ 
        unsigned        SDMA;                           // R/W, SDMA System Address 
        unsigned        BPARAM;                         // R/W, Block Size & Count 

        unsigned        ARG;                            // R/W, Argument (specified as bit39~8 of Command Format) 
        unsigned        TCMD;                           // R/W, Command & Transfer Mode 

        unsigned        RESP[4];                                // R, Response Words [127:0] 
        unsigned        DATA;                           // R/W, Buffer Data Port 
        unsigned        STATE;                          // R, Present State 

        unsigned        CTRL;                           // R/W, (Power, Host, Wakeup, Block Gap) Control 

        unsigned        CLK;                            // R/W, Clock control / Reset & TimeOut Control 

        unsigned        STS;                            // R, Normal Interrupt Status 

        unsigned        STSEN;                          // R/W, Normal Interrupt Status Enable 
        unsigned        INTEN;                          // R/W, Normal Interrupt Signal Enable 

        unsigned        CMD12ERR;                       // R, Auto CMD12 Error Status 

        unsigned        CA;                                     // R, Capabilities 

        unsigned        _RSV0; 
        unsigned        CUR;                            // R, Maximum Current Capabilities 
        unsigned        _RSV1; 
        unsigned        FORCE;                          // W, Force Event for Error 

        unsigned        ADMAERR;                        // R/W, ADMA Error Status 

        unsigned        ADDR[2];                        // R/W, ADMA Address 
        unsigned        _RSV2[(0xF0-0x60)/4]; 
        unsigned        SPIINT;                         // R, SPI Interrupt Support 
        unsigned        _RSV3[2]; 
        unsigned short  VERSION;        // R, Host Controller Version 
        unsigned short  SLOTINT;                // R, Slot Interrupt Status 
} sHwSDMMC; 

#define HwSD_BPARAM_BCNT(X)                     ((X)*Hw16)                                              // Block Count for current transfer (Valid only in multiple block transfers)

#define HwSD_BPARAM_BCNT_MASK                   HwSD_BPARAM_BCNT(65535) 
#define HwSD_BPARAM_SDMABUF(X)                  ((X)*Hw12) 
#define HwSD_BPARAM_SDMABUF_4KB                 HwSD_BPARAM_SDMABUF(0) 
#define HwSD_BPARAM_SDMABUF_8KB                 HwSD_BPARAM_SDMABUF(1) 
#define HwSD_BPARAM_SDMABUF_16KB                HwSD_BPARAM_SDMABUF(2) 
#define HwSD_BPARAM_SDMABUF_32KB                HwSD_BPARAM_SDMABUF(3) 
#define HwSD_BPARAM_SDMABUF_64KB                HwSD_BPARAM_SDMABUF(4) 
#define HwSD_BPARAM_SDMABUF_128KB               HwSD_BPARAM_SDMABUF(5) 
#define HwSD_BPARAM_SDMABUF_256KB               HwSD_BPARAM_SDMABUF(6) 
#define HwSD_BPARAM_SDMABUF_512KB               HwSD_BPARAM_SDMABUF(7) 
#define HwSD_BPARAM_SDMABUF_MASK                HwSD_BPARAM_SDMABUF(7) 
#define HwSD_BPARAM_BSIZE(X)                    ((X)*Hw0)                                               // 0~2048 bytes

#define HwSD_BPARAM_BSIZE_512                   HwSD_BPARAM_BSIZE(512) 
#define HwSD_BPARAM_BSIZE_1KB                   HwSD_BPARAM_BSIZE(1024) 
#define HwSD_BPARAM_BSIZE_2KB                   HwSD_BPARAM_BSIZE(2048) 
#define HwSD_BPARAM_BSIZE_MASK                  HwSD_BPARAM_BSIZE(4095) 

#define HwSD_TCMD_CMDIDX(X)                     ((X)*Hw24)                                              // Command Index

#define HwSD_TCMD_CMDIDX_MASK                   HwSD_TCMD_CMDIDX(63) 
#define HwSD_TCMD_CTYPE(X)                      ((X)*Hw22)                                              // Suspend, Resume, Abort, Normal

#define HwSD_TCMD_CTYPE_ABORT                   HwSD_TCMD_CTYPE(3) 
#define HwSD_TCMD_CTYPE_RESUME                  HwSD_TCMD_CTYPE(2) 
#define HwSD_TCMD_CTYPE_SUSPEND                 HwSD_TCMD_CTYPE(1) 
#define HwSD_TCMD_CTYPE_NORMAL                  HwSD_TCMD_CTYPE(0) 
#define HwSD_TCMD_CTYPE_MASK                    HwSD_TCMD_CTYPE(3) 
#define HwSD_TCMD_DATSEL_PRESENT                        Hw21                                                    // Data is present and shall be transferred

#define HwSD_TCMD_DATSEL_NODATA                 HwZERO 
#define HwSD_TCMD_CICHK_EN                              Hw20                                                    // Check the Index field in the response

#define HwSD_TCMD_CRCHK_EN                              Hw19                                                    // Check the CRC field in the response

#define HwSD_TCMD_RTYPE_HC                              Hw18                                                    // Response with 48 bytes (HC)

#define HwSD_TCMD_RTYPE(X)                                      ((X)*Hw16)                                              // Response Type

#define HwSD_TCMD_RTYPE_NORESP                  HwSD_TCMD_RTYPE(0)                      // No Response 
#define HwSD_TCMD_RTYPE_R136                            HwSD_TCMD_RTYPE(1)                      // Response with 136 bytes

#define HwSD_TCMD_RTYPE_R48                             HwSD_TCMD_RTYPE(2)                      // Response with 48 bytes

#define HwSD_TCMD_RTYPE_R48CHK                  HwSD_TCMD_RTYPE(3)                      // Response with 48 bytes - check Busy after response

#define HwSD_TCMD_SPI_MODE                              Hw7                                                             // SPI Mode

#define HwSD_TCMD_SD_MODE                               HwZERO                                                  // SD Mode

#define HwSD_TCMD_ATACMD                                        Hw6                                                             // CE-ATA Mode (Device will send Command Completion Signal)

#define HwSD_TCMD_MS_MULTI                              Hw5                                                             // Multiple Block Mode

#define HwSD_TCMD_MS_SINGLE                             HwZERO                                                  // Single Block Mode

#define HwSD_TCMD_DIR_READ                              Hw4                                                             // Read Mode (Transfer from Card to Host)

#define HwSD_TCMD_DIR_WRITE                             HwZERO                                                  // Write Mode (Transfer from Host to Card)

#define HwSD_TCMD_ACMD12_EN                             Hw2                                                             // CMD12 is issued automatically after last block transfer is completed.

#define HwSD_TCMD_BCNTEN_EN                             Hw1                                                             // Enable Block Count Register (BCNT). only relevant for multiple block transfers

#define HwSD_TCMD_DMAEN_EN                              Hw0                                                             // DMA Enabled (can be enabled only if CA register tells it support DMA)

#define HwSD_STATE_DAT7_HIGH                            Hw28                                                    // State of DAT7 line

#define HwSD_STATE_DAT6_HIGH                            Hw27                                                    // State of DAT6 line

#define HwSD_STATE_DAT5_HIGH                            Hw26                                                    // State of DAT5 line

#define HwSD_STATE_DAT4_HIGH                            Hw25                                                    // State of DAT4 line

#define HwSD_STATE_CMD_HIGH                             Hw24                                                    // State of CMD line

#define HwSD_STATE_DAT3_HIGH                            Hw23                                                    // State of DAT3 line

#define HwSD_STATE_DAT2_HIGH                            Hw22                                                    // State of DAT2 line

#define HwSD_STATE_DAT1_HIGH                            Hw21                                                    // State of DAT1 line

#define HwSD_STATE_DAT0_HIGH                            Hw20                                                    // State of DAT0 line

#define HwSD_STATE_SDWP_HIGH                            Hw19                                                    // State of WP line

#define HwSD_STATE_SDCD_DET                             Hw18                                                    // Inverse State of CD# line (Card Detected State)

#define HwSD_STATE_CDST_STABLE                  Hw17                                                    // Card Detect Pin level is stable

#define HwSD_STATE_CDIN_INSERT                          Hw16                                                    // Card has been inserted

#define HwSD_STATE_RDEN_EN                                      Hw11                                                    // Buffer Read Enable (Readable data exist in the buffer)

#define HwSD_STATE_WREN_EN                              Hw10                                                    // Buffer Write Enable (Data can be written to the buffer)

#define HwSD_STATE_RDACT_ACTIVE                 Hw9                                                             // Read transfer is active

#define HwSD_STATE_WRACT_ACTIVE                 Hw8                                                             // Write transfer is active

#define HwSD_STATE_DATACT_ACTIVE                        Hw2                                                             // DAT[7:0] line on SD bus is in use

#define HwSD_STATE_NODAT_INHIBIT                        Hw1                                                             // DAT line or Read Transfer is active (No Command using DAT is allowed)

#define HwSD_STATE_NOCMD_INHIBIT                        Hw0                                                             // CMD line is in use (No Command is allowed)

#define HwSD_CTRL_WKOUT_EN                              Hw26                                                    // Enable Wakeup event on Card Removal

#define HwSD_CTRL_WKIN_EN                                       Hw25                                                    // Enable Wakeup event on Card Insertion

#define HwSD_CTRL_WKINT_EN                                      Hw24                                                    // Enable Wakeup event on Card Interrupt

#define HwSD_CTRL_BGINT_EN                                      Hw19                                                    // Enable interrupt detection at the block gap for multiple block transfer (valid only 4bit SDIO mode)

#define HwSD_CTRL_RDWAIT_EN                             Hw18                                                    // Enable use of read-wait protocol to stop read data using DAT2 line

#define HwSD_CTRL_CONREQ_EN                             Hw17                                                    // Restart a transaction that was stopped by BGSTOP request

#define HwSD_CTRL_BGSTOP_EN                             Hw16                                                    // Executing transaction stops at the next block gap for non-DMA, SDMA, ADMA transfers.

#define HwSD_CTRL_VOLTSEL(X)                            ((X)*Hw9)                                               // Selects voltage level for the SD Card

#define HwSD_CTRL_VOLTSEL_V33                           HwSD_CTRL_VOLTSEL(7)                    // 3.3V Flattop

#define HwSD_CTRL_VOLTSEL_V30                           HwSD_CTRL_VOLTSEL(6)                    // 3.0V (typical)

#define HwSD_CTRL_VOLTSEL_V18                           HwSD_CTRL_VOLTSEL(5)                    // 1.8V (typical)

#define HwSD_CTRL_VOLTSEL_MASK                  HwSD_CTRL_VOLTSEL(7) 
#define HwSD_CTRL_POW_ON                                        Hw8                                                             // SD Bus Power On (VOLTSEL should be set ahead)

#define HwSD_CTRL_DETSEL_TST                            Hw7                                                             // Card detect test level is selected (test purpose only)

#define HwSD_CTRL_DETSEL_SDCD                           HwZERO                                                  // SDCD# line is selected (normal case)

#define HwSD_CTRL_SD8_EN                                        Hw5                                                             // SD 8bit mode is selected

#define HwSD_CTRL_SELDMA(X)                             ((X)*Hw3)                                               // SD DMA Selection

#define HwSD_CTRL_SELDMA_SDMA                           HwSD_CTRL_SELDMA(0)                     // SDMA is selected

#define HwSD_CTRL_SELDMA_ADMA1                  HwSD_CTRL_SELDMA(1)                     // ADMA1 (32bit address) is selected

#define HwSD_CTRL_SELDMA_ADMA2_32                       HwSD_CTRL_SELDMA(2)                     // ADMA2 (32bit address) is selected

#define HwSD_CTRL_SELDMA_ADMA2_64                       HwSD_CTRL_SELDMA(3)                     // ADMA2 (64bit address) is selected

#define HwSD_CTRL_SELDMA_MASK                           HwSD_CTRL_SELDMA(3) 
#define HwSD_CTRL_HS_EN                                 Hw2                                                             // Enable High Speed

#define HwSD_CTRL_SD4_EN                                        Hw1                                                             // SD 4bit mode is selected

#define HwSD_CTRL_SD1_EN                                        HwZERO                                                  // SD 1bit mode is selected

#define HwSD_CTRL_LED_ON                                        Hw0                                                             // LED ON

#define HwSD_CLK_RSTDAT_EN                              Hw26                                                    // SW Reset for DAT line

                                                                                                                                                        // DATA, STATE(RDEN, WREN, RDACT, WRACT, DATACT, NODAT) are cleared

                                                                                                                                                        // CTRL(CONREQ, BGSTOP), STS(RDRDY,WRRDY,BLKGAP,TDONE) are cleared

#define HwSD_CLK_RSTCMD_EN                              Hw25                                                    // SW Reset for CMD line

                                                                                                                                                        // STATE(NOCMD), STS(CDONE) are cleared

#define HwSD_CLK_RSTALL_EN                                      Hw24                                                    // SW Reset for All (except Card Detection Circuit)

#define HwSD_CLK_TIMEOUT(X)                             ((X)*Hw16)                                              // Time Out Selection

#define HwSD_CLK_TIMEOUT_TM8K                           HwSD_CLK_TIMEOUT(0)                     // Timeout = TMCLK * 8K

#define HwSD_CLK_TIMEOUT_TM16K                  HwSD_CLK_TIMEOUT(1)                     // Timeout = TMCLK * 16K

#define HwSD_CLK_TIMEOUT_TM32K                  HwSD_CLK_TIMEOUT(2)                     // Timeout = TMCLK * 32K

#define HwSD_CLK_TIMEOUT_TM64K                  HwSD_CLK_TIMEOUT(3)                     // Timeout = TMCLK * 64K

#define HwSD_CLK_TIMEOUT_TM128K                 HwSD_CLK_TIMEOUT(4)                     // Timeout = TMCLK * 128K

#define HwSD_CLK_TIMEOUT_TM256K                 HwSD_CLK_TIMEOUT(5)                     // Timeout = TMCLK * 256K

#define HwSD_CLK_TIMEOUT_TM512K                 HwSD_CLK_TIMEOUT(6)                     // Timeout = TMCLK * 512K

#define HwSD_CLK_TIMEOUT_TM1M                           HwSD_CLK_TIMEOUT(7)                     // Timeout = TMCLK * 1M

#define HwSD_CLK_TIMEOUT_TM2M                           HwSD_CLK_TIMEOUT(8)                     // Timeout = TMCLK * 2M

#define HwSD_CLK_TIMEOUT_TM4M                           HwSD_CLK_TIMEOUT(9)                     // Timeout = TMCLK * 4M

#define HwSD_CLK_TIMEOUT_TM8M                           HwSD_CLK_TIMEOUT(10)                    // Timeout = TMCLK * 8M

#define HwSD_CLK_TIMEOUT_TM16M                  HwSD_CLK_TIMEOUT(11)                    // Timeout = TMCLK * 16M

#define HwSD_CLK_TIMEOUT_TM32M                  HwSD_CLK_TIMEOUT(12)                    // Timeout = TMCLK * 32M

#define HwSD_CLK_TIMEOUT_TM64M                  HwSD_CLK_TIMEOUT(13)                    // Timeout = TMCLK * 64M

#define HwSD_CLK_TIMEOUT_TM128M                 HwSD_CLK_TIMEOUT(14)                    // Timeout = TMCLK * 128M

#define HwSD_CLK_TIMEOUT_MASK                           HwSD_CLK_TIMEOUT(15) 
#define HwSD_CLK_SDCLKSEL(X)                            ((X)*Hw8)                                               // SDCLK Frequency Selection

#define HwSD_CLK_SDCLKSEL_D256                  HwSD_CLK_SDCLKSEL(128)          // SDCLK = base clock / 256

#define HwSD_CLK_SDCLKSEL_D128                  HwSD_CLK_SDCLKSEL(64)                   // SDCLK = base clock / 128

#define HwSD_CLK_SDCLKSEL_D64                           HwSD_CLK_SDCLKSEL(32)                   // SDCLK = base clock / 64

#define HwSD_CLK_SDCLKSEL_D32                           HwSD_CLK_SDCLKSEL(16)                   // SDCLK = base clock / 32

#define HwSD_CLK_SDCLKSEL_D16                           HwSD_CLK_SDCLKSEL(8)                    // SDCLK = base clock / 16

#define HwSD_CLK_SDCLKSEL_D8                            HwSD_CLK_SDCLKSEL(4)                    // SDCLK = base clock / 8

#define HwSD_CLK_SDCLKSEL_D4                            HwSD_CLK_SDCLKSEL(2)                    // SDCLK = base clock / 4

#define HwSD_CLK_SDCLKSEL_D2                            HwSD_CLK_SDCLKSEL(1)                    // SDCLK = base clock / 2

#define HwSD_CLK_SDCLKSEL_D1                            HwSD_CLK_SDCLKSEL(0)                    // SDCLK = base clock (10MHz ~ 63MHz)

#define HwSD_CLK_SDCLKSEL_MASK                  HwSD_CLK_SDCLKSEL(255) 
#define HwSD_CLK_SCKEN_EN                                       Hw2                                                             // SDCLK Enable

#define HwSD_CLK_CLKRDY_STABLE                  Hw1                                                             // R, Internal base clock is stable

#define HwSD_CLK_CLKEN_EN                                       Hw0                                                             // Internal base clock Enable

#define HwSD_STS_VENDOR3_ERR                            Hw31                                                    // Vendor specific error status3

#define HwSD_STS_VENDOR2_ERR                            Hw30                                                    // Vendor specific error status2

#define HwSD_STS_VENDOR1_ERR                            Hw29                                                    // Vendor specific error status1

#define HwSD_STS_VENDOR0_ERR                            Hw28                                                    // Vendor specific error status0

#define HwSD_STS_ADMA_ERR                                       Hw25                                                    // Error detected when ADMA Transfers

#define HwSD_STS_ACMD12_ERR                             Hw24                                                    // One of CMD12ERR register has been set to 1

#define HwSD_STS_CLIMIT_ERR                             Hw23                                                    // Exceeding Current limit

#define HwSD_STS_DATEND_ERR                             Hw22                                                    // 0 detected at the end bit position of read

#define HwSD_STS_DATCRC_ERR                             Hw21                                                    // CRC error detected

#define HwSD_STS_DATTIME_ERR                            Hw20                                                    // Data Timeout error detected

#define HwSD_STS_CINDEX_ERR                             Hw19                                                    // Command Index error detected in the command response

#define HwSD_STS_CMDEND_ERR                             Hw18                                                    // 0 deteced at the end bit position of a command response

#define HwSD_STS_CMDCRC_ERR                             Hw17                                                    // Command CRC error detected

#define HwSD_STS_CMDTIME_ERR                            Hw16                                                    // Command Timeout error detected (no response returned within 64 SDCLKs)

#define HwSD_STS_ERR                                            Hw15                                                    // Error detected (One of above flags is set)

#define HwSD_STS_CDINT                                          Hw8                                                             // Card Interrupt is generated

#define HwSD_STS_CDOUT                                  Hw7                                                             // Card Removed (STATE.CDIN changes from 1 to 0)

#define HwSD_STS_CDIN                                           Hw6                                                             // Card Inserted (STATE.CDIN changes from 0 to 1)

#define HwSD_STS_RDRDY                                  Hw5                                                             // Buffer Read Ready (STATE.RDEN changes from 0 to 1, and ready to read)

#define HwSD_STS_WRRDY                                  Hw4                                                             // Buffer Write Ready (STATE.WREN changes from 0 to 1, and ready to read)

#define HwSD_STS_DMAINT                                 Hw3                                                             // DMA Interrupt

#define HwSD_STS_BLKGAP                                 Hw2                                                             // Block Gap Event (STATE.DATACT falling in read transfer, STATE.WRACT falling in write transfer)

#define HwSD_STS_TDONE                                          Hw1                                                             // Transfer Complete

#define HwSD_STS_CDONE                                  Hw0                                                             // The end bit of the command response acquired (except Auto CMD12)

#define HwSD_STSEN_VENDOR3_ERR                  Hw31                                                    // Enable STS.VENDOR3

#define HwSD_STSEN_VENDOR2_ERR                  Hw30                                                    // Enable STS.VENDOR2

#define HwSD_STSEN_VENDOR1_ERR                  Hw29                                                    // Enable STS.VENDOR1

#define HwSD_STSEN_VENDOR0_ERR                  Hw28                                                    // Enable STS.VENDOR0

#define HwSD_STSEN_ADMA_ERR                             Hw25                                                    // Enable STS.ADMA

#define HwSD_STSEN_ACMD12_ERR                           Hw24                                                    // Enable STS.ACMD12

#define HwSD_STSEN_CLIMIT_ERR                           Hw23                                                    // Enable STS.CLIMIT

#define HwSD_STSEN_DATEND_ERR                           Hw22                                                    // Enable STS.DATEND

#define HwSD_STSEN_DATCRC_ERR                           Hw21                                                    // Enable STS.DATCRC

#define HwSD_STSEN_DATTIME_ERR                  Hw20                                                    // Enable STS.DATTIME

#define HwSD_STSEN_CINDEX_ERR                           Hw19                                                    // Enable STS.CINDEX

#define HwSD_STSEN_CMDEND_ERR                           Hw18                                                    // Enable STS.CMDEND

#define HwSD_STSEN_CMDCRC_ERR                   Hw17                                                    // Enable STS.CMDCRC

#define HwSD_STSEN_CMDTIME_ERR                  Hw16                                                    // Enable STS.CMDTIME

#define HwSD_STSEN_CDINT                                        Hw8                                                             // Enable STS.CDINT

#define HwSD_STSEN_CDOUT                                        Hw7                                                             // Enable STS.CDOUT

#define HwSD_STSEN_CDIN                                 Hw6                                                             // Enable STS.CDIN

#define HwSD_STSEN_RDRDY                                        Hw5                                                             // Enable STS.RDRDY

#define HwSD_STSEN_WRRDY                                        Hw4                                                             // Enable STS.WRRDY

#define HwSD_STSEN_DMAINT                                       Hw3                                                             // Enable STS.DMAINT

#define HwSD_STSEN_BLKGAP                                       Hw2                                                             // Enable STS.BLKGAP

#define HwSD_STSEN_TDONE                                        Hw1                                                             // Enable STS.TDONE

#define HwSD_STSEN_CDONE                                        Hw0                                                             // Enable STS.CDONE

#define HwSD_INTEN_VENDOR3_ERR                  Hw31                                                    // Enable Interrupt of INT.VENDOR3

#define HwSD_INTEN_VENDOR2_ERR                  Hw30                                                    // Enable Interrupt of INT.VENDOR2

#define HwSD_INTEN_VENDOR1_ERR                  Hw29                                                    // Enable Interrupt of INT.VENDOR1

#define HwSD_INTEN_VENDOR0_ERR                  Hw28                                                    // Enable Interrupt of INT.VENDOR0

#define HwSD_INTEN_ADMA_ERR                             Hw25                                                    // Enable Interrupt of INT.ADMA

#define HwSD_INTEN_ACMD12_ERR                           Hw24                                                    // Enable Interrupt of INT.ACMD12

#define HwSD_INTEN_CLIMIT_ERR                           Hw23                                                    // Enable Interrupt of INT.CLIMIT

#define HwSD_INTEN_DATEND_ERR                           Hw22                                                    // Enable Interrupt of INT.DATEND

#define HwSD_INTEN_DATCRC_ERR                           Hw21                                                    // Enable Interrupt of INT.DATCRC

#define HwSD_INTEN_DATTIME_ERR                          Hw20                                                    // Enable Interrupt of INT.DATTIME

#define HwSD_INTEN_CINDEX_ERR                           Hw19                                                    // Enable Interrupt of INT.CINDEX

#define HwSD_INTEN_CMDEND_ERR                           Hw18                                                    // Enable Interrupt of INT.CMDEND

#define HwSD_INTEN_CMDCRC_ERR                           Hw17                                                    // Enable Interrupt of INT.CMDCRC

#define HwSD_INTEN_CMDTIME_ERR                  Hw16                                                    // Enable Interrupt of INT.CMDTIME

#define HwSD_INTEN_CDINT                                        Hw8                                                             // Enable Interrupt of INT.CDINT

#define HwSD_INTEN_CDOUT                                        Hw7                                                             // Enable Interrupt of INT.CDOUT

#define HwSD_INTEN_CDIN                                 Hw6                                                             // Enable Interrupt of INT.CDIN

#define HwSD_INTEN_RDRDY                                        Hw5                                                             // Enable Interrupt of INT.RDRDY

#define HwSD_INTEN_WRRDY                                        Hw4                                                             // Enable Interrupt of INT.WRRDY

#define HwSD_INTEN_DMAINT                                       Hw3                                                             // Enable Interrupt of INT.DMAINT

#define HwSD_INTEN_BLKGAP                                       Hw2                                                             // Enable Interrupt of INT.BLKGAP

#define HwSD_INTEN_TDONE                                        Hw1                                                             // Enable Interrupt of INT.TDONE

#define HwSD_INTEN_CDONE                                        Hw0                                                             // Enable Interrupt of INT.CDONE

#define HwSD_CMD12ERR_NOCMD                             Hw7                                                             // CMD_wo_DAT is not executed due to an Auto CMD12 Error

#define HwSD_CMD12ERR_INDEX_ERR                 Hw4                                                             // Command Index error occurs

#define HwSD_CMD12ERR_ENDBIT_ERR                        Hw3                                                             // 0 is detected at the end bit of command response

#define HwSD_CMD12ERR_CRC_ERR                   Hw2                                                             // CRC error is detected in command response

#define HwSD_CMD12ERR_TIMEOUT_ERR                       Hw1                                                             // CMD12 Timeout error detected (no response returned within 64 SDCLKs)

#define HwSD_CMD12ERR_NORUN_ERR                 Hw0                                                             // Auto CMD12 cannot be issued due to some error.

#define HwSD_CA_SPIBLK                                          Hw30                                                    // SPI Block Mode Supported

#define HwSD_CA_SPI                                             Hw29                                                    // SPI Mode Supported

#define HwSD_CA_BUS64                                           Hw28                                                    // 64bit System Bus Supported

#define HwSD_CA_INT                                             Hw27                                                    // Interrupt Mode Supported

#define HwSD_CA_V18                                             Hw26                                                    // 1.8V Supported

#define HwSD_CA_V30                                             Hw25                                                    // 3.0V Supported

#define HwSD_CA_V33                                             Hw24                                                    // 3.3V Supported

#define HwSD_CA_RESUME                                  Hw23                                                    // Suspend/Resume Supported

#define HwSD_CA_SDMA                                            Hw22                                                    // SDMA Supported

#define HwSD_CA_HS                                                      Hw21                                                    // HS Supported

#define HwSD_CA_ADMA2                                           Hw19                                                    // ADMA2 Supported

#define HwSD_CA_EXTBUS                                          Hw18                                                    // Extended Media Bus Supported

#define HwSD_CA_MAXBLK(X)                                       ((X)*Hw16)                                              // Max Block Supported

#define HwSD_CA_MAXBLK_512                              HwSD_CA_MAXBLK(0)                               // Max Block is 512 bytes

#define HwSD_CA_MAXBLK_1K                                       HwSD_CA_MAXBLK(1)                               // Max Block is 1K bytes

#define HwSD_CA_MAXBLK_2K                                       HwSD_CA_MAXBLK(2)                               // Max Block is 2K bytes

#define HwSD_CA_MAXBLK_4K                                       HwSD_CA_MAXBLK(3)                               // Max Block is 4K bytes

#define HwSD_CA_MAXBLK_MASK                             HwSD_CA_MAXBLK(3) 
#define HwSD_CA_BASECLK(X)                                      ((X)*Hw8)                                               // Base Clock Frequency (1 ~ 63 MHz)

#define HwSD_CA_BASECLK_MASK                            HwSD_CA_BASECLK(63) 
#define HwSD_CA_TUNIT_MHZ                                       Hw7                                                             // MHz Unit for base clock frequency used to detect Data Timeout Error

#define HwSD_CA_TUNIT_KHZ                                       HwZERO                                                  // KHz Unit for base clock frequency used to detect Data Timeout Error

#define HwSD_CA_TMCLK(X)                                        ((X)*Hw0)                                               // Base clock frequency used to detect Data Timeout Error (1~63)

#define HwSD_CA_TMCLK_MASK                              HwSD_CA_TMCLK(63) 

#define HwSD_CUR_MAXCV18(X)                             ((X)*Hw16)                                              // Maximum Current for 1.8V

#define HwSD_CUR_MAXCV18_MASK                   HwSD_CUR_MAXCV18(255) 
#define HwSD_CUR_MAXCV30(X)                             ((X)*Hw8)                                               // Maximum Current for 3.0V

#define HwSD_CUR_MAXCV30_MASK                   HwSD_CUR_MAXCV30(255) 
#define HwSD_CUR_MAXCV33(X)                             ((X)*Hw0)                                               // Maximum Current for 3.3V

#define HwSD_CUR_MAXCV33_MASK                   HwSD_CUR_MAXCV33(255) 

#define HwSD_FORCE_VENDOR3_ERR                  Hw31                                                    // Force Event for INT.VENDOR3

#define HwSD_FORCE_VENDOR2_ERR                  Hw30                                                    // Force Event for INT.VENDOR2

#define HwSD_FORCE_VENDOR1_ERR                  Hw29                                                    // Force Event for INT.VENDOR1

#define HwSD_FORCE_VENDOR0_ERR                  Hw28                                                    // Force Event for INT.VENDOR0

#define HwSD_FORCE_ADMA_ERR                             Hw25                                                    // Force Event for INT.ADMA

#define HwSD_FORCE_ACMD12_ERR                           Hw24                                                    // Force Event for INT.ACMD12

#define HwSD_FORCE_CLIMIT_ERR                           Hw23                                                    // Force Event for INT.CLIMIT

#define HwSD_FORCE_DATEND_ERR                           Hw22                                                    // Force Event for INT.DATEND

#define HwSD_FORCE_DATCRC_ERR                           Hw21                                                    // Force Event for INT.DATCRC

#define HwSD_FORCE_DATTIME_ERR                  Hw20                                                    // Force Event for INT.DATTIME

#define HwSD_FORCE_CINDEX_ERR                           Hw19                                                    // Force Event for INT.CINDEX

#define HwSD_FORCE_CMDEND_ERR                   Hw18                                                    // Force Event for INT.CMDEND

#define HwSD_FORCE_CMDCRC_ERR                   Hw17                                                    // Force Event for INT.CMDCRC

#define HwSD_FORCE_CMDTIME_ERR                  Hw16                                                    // Force Event for INT.CMDTIME

#define HwSD_FORCE_CDINT                                        Hw8                                                             // Force Event for INT.CDINT

#define HwSD_FORCE_CDOUT                                        Hw7                                                             // Force Event for INT.CDOUT

#define HwSD_FORCE_CDIN                                 Hw6                                                             // Force Event for INT.CDIN

#define HwSD_FORCE_RDRDY                                        Hw5                                                             // Force Event for INT.RDRDY

#define HwSD_FORCE_WRRDY                                        Hw4                                                             // Force Event for INT.WRRDY

#define HwSD_FORCE_DMAINT                                       Hw3                                                             // Force Event for INT.DMAINT

#define HwSD_FORCE_BLKGAP                                       Hw2                                                             // Force Event for INT.BLKGAP

#define HwSD_FORCE_TDONE                                        Hw1                                                             // Force Event for INT.TDONE

#define HwSD_FORCE_CDONE                                        Hw0                                                             // Force Event for INT.CDONE

#define HwSD_ADMAERR_LEN_ERR                            Hw2                                                             // ADMA Length Mismatch Error

#define HwSD_ADMAERR_STATE(X)                           ((X)*Hw0) 
#define HwSD_ADMAERR_STATE_STOP                 HwSD_ADMAERR_STATE(0)           // 
#define HwSD_ADMAERR_STATE_FDS                  HwSD_ADMAERR_STATE(1)           // 
#define HwSD_ADMAERR_STATE_TFR                  HwSD_ADMAERR_STATE(3)           // 
#define HwSD_ADMAERR_STATE_MASK                 HwSD_ADMAERR_STATE(3) 

#define HwSD_VERSION_SPEC(X)                            ((X)*Hw0)                                               // Specification Number (0 = V1.0, 1 = V2.0, 2 = V2.0 with ADMA)

#define HwSD_VERSION_SPEC_MASK                  (Hw8-Hw0) 
#define HwSD_VERSION_VENDOR(X)                  ((X)*Hw8)                                               // Vendor Number

#define HwSD_VERSION_VENDOR_MASK                        (Hw16-Hw8) 

#define HwSD_SLOTINT_1                                          Hw0 
#define HwSD_SLOTINT_2                                          Hw1 
#define HwSD_SLOTINT_3                                          Hw2 
#define HwSD_SLOTINT_4                                          Hw3 
#define HwSD_SLOTINT_5                                          Hw4 
#define HwSD_SLOTINT_6                                          Hw5 
#define HwSD_SLOTINT_7                                          Hw6 
#define HwSD_SLOTINT_8                                          Hw7 

//////////
//Done...
#define	HwSDICLK		*(volatile unsigned long *)0xF1001004	// R/W, Clock control register
#define	HwSDICLK_BP_ON			Hw13					//
#define	HwSDICLK_BP_OFF			~Hw13					//
#define	HwSDICLK_EN_ON			Hw12					//
#define	HwSDICLK_EN_OFF			~Hw12					//

#define	HwSDIARGU		*(volatile unsigned long *)0xF1001008	// R/W, Command argument register

#define	HwSDICMD		*(volatile unsigned long *)0xF100100C	// R/W, Command index and type register
#define	HwSDICMD_EN_ON			Hw11					//
#define	HwSDICMD_EN_OFF			~Hw11					//
#define	HwSDICMD_APP_ON			Hw10					//
#define	HwSDICMD_APP_OFF			~Hw10					//
#define	HwSDICMD_RT_1				Hw7						//
#define	HwSDICMD_RT_1B			Hw8						//
#define	HwSDICMD_RT_2				(Hw8|Hw7)				//
#define	HwSDICMD_RT_3				Hw9						//
#define	HwSDICMD_RT_6				(Hw9|Hw7)				//
#define	HwSDICMD_WR_RES			Hw6						//
#define	HwSDICMD_WR_NORES		~Hw6					//


#define	HwSDIRSPCMD		*(volatile unsigned long *)0xF1001010	// R, Response index register

#define	HwSDIRSPARGU0		*(volatile unsigned long *)0xF1001014	// R, Response argument register0

#define	HwSDIRSPARGU1		*(volatile unsigned long *)0xF1001018	// R, Response argument register1

#define	HwSDIRSPARGU2		*(volatile unsigned long *)0xF100101C	// R, Response argument register2

#define	HwSDIRSPARGU3		*(volatile unsigned long *)0xF1001020	// R, Response argument register3

#define	HwSDIDITIMER		*(volatile unsigned long *)0xF1001024	// R/W, Wait cycles for data transfer

#define	HwSDIDCTRL2		*(volatile unsigned long *)0xF1001028	// R/W, Data path control register 2
#define HwSDIDCTRL2_ST		Hw3						// Stop indication when SDIO multi block transfer
#define HwSDIDCTRL2_BM		Hw2						// Block unit data transfer for SDIO mode
#define HwSDIDCTRL2_DD		Hw1						// Write Data to SDIO device [ 0: read, 1:write]
#define HwSDIDCTRL2_MODE	Hw0						// SDMMC controller operates as SDIO mode [ 0: SDMMC, 1:SDIO]

#define	HwSDIDCTRL		*(volatile unsigned long *)0xF100102C	// R/W, Data path control register
#define	HwSDIDCTRL_BE_BIG		Hw24						//
#define	HwSDIDCTRL_BE_LITTLE	~Hw24						//
#define	HwSDIDCTRL_FSR_OP		Hw12						//
#define	HwSDIDCTRL_FSR_RST	~Hw12						//
#define	HwSDIDCTRL_WB_1		~(Hw3|Hw2)					//
#define	HwSDIDCTRL_WB_4		Hw2							//
#define	HwSDIDCTRL_WB_8		Hw3							//
#define	HwSDIDCTRL_DTM_STM	Hw1							//
#define	HwSDIDCTRL_DTM_BLK	~Hw1						//
#define	HwSDIDCTRL_DEN_ON	Hw0							//
#define	HwSDIDCTRL_DEN_OFF	~Hw0						//

#define	HwSDISTATUS		*(volatile unsigned long *)0xF1001030	// R, Status register
#define	HwSDISTATUS_MBE		Hw25						//
#define	HwSDISTATUS_SBE		Hw24						//
#define	HwSDISTATUS_FD		Hw23						//
#define	HwSDISTATUS_DPR		Hw22						//
#define	HwSDISTATUS_CPR		Hw21						//
#define	HwSDISTATUS_RFU		Hw20						//
#define	HwSDISTATUS_TFO		Hw19						//
#define	HwSDISTATUS_FFR		Hw18						//
#define	HwSDISTATUS_FLR		Hw17						//
#define	HwSDISTATUS_FCF		Hw16						//
#define	HwSDISTATUS_FF		Hw15						//
#define	HwSDISTATUS_FE		Hw14						//
#define	HwSDISTATUS_RA		Hw13						//
#define	HwSDISTATUS_TA		Hw12						//
#define	HwSDISTATUS_RDBE		Hw10						//
#define	HwSDISTATUS_TDBE		Hw9							//
#define	HwSDISTATUS_CRE		Hw7							//
#define	HwSDISTATUS_FWE		Hw6							//
#define	HwSDISTATUS_DTO		Hw4							//
#define	HwSDISTATUS_CTO		Hw3							//
#define	HwSDISTATUS_RDCF		Hw2							//
#define	HwSDISTATUS_TDCF		Hw1							//
#define	HwSDISTATUS_CCF		Hw0							//

#define	HwSDIIFLAG		*(volatile unsigned long *)0xF1001034	// R/W, Interrupt flag register
#define	HwSDIIFLAG_DRI		Hw8							//
#define	HwSDIIFLAG_CRI			Hw7							//
#define	HwSDIIFLAG_FWEI		Hw6							//
#define	HwSDIIFLAG_SBEI		Hw5							//
#define	HwSDIIFLAG_DTOI		Hw4							//
#define	HwSDIIFLAG_CTOI		Hw3							//
#define	HwSDIIFLAG_RDCFI		Hw2							//
#define	HwSDIIFLAG_TDCFI		Hw1							//
#define	HwSDIIFLAG_CCFI		Hw0							//

#define	HwSDIWDATA		*(volatile unsigned long *)0xF1001038	// R/W, Transmit data register

#define	HwSDIRDATA		*(volatile unsigned long *)0xF100103C	// R, Receive data register

#define	HwSDIIENABLE		*(volatile unsigned long *)0xF1001040	// R/W, Interrupt enable register
#define	HwSDIIENABLE_DRI_EN		Hw8						//
#define	HwSDIIENABLE_DRI_DIS		~Hw8					//
#define	HwSDIIENABLE_CRI_EN		Hw7						//
#define	HwSDIIENABLE_CRI_DIS		~Hw7					//
#define	HwSDIIENABLE_FWEI_EN		Hw6						//
#define	HwSDIIENABLE_FWEI_DIS	~Hw6					//
#define	HwSDIIENABLE_SBEI_EN		Hw5						//
#define	HwSDIIENABLE_SBEI_DIS		~Hw5					//
#define	HwSDIIENABLE_DTOI_EN		Hw4						//
#define	HwSDIIENABLE_DTOI_DIS	~Hw4					//
#define	HwSDIIENABLE_CTOI_EN		Hw3						//
#define	HwSDIIENABLE_CTOI_DIS		~Hw3					//
#define	HwSDIIENABLE_RDCFI_EN	Hw2						//
#define	HwSDIIENABLE_RDCFI_DIS	~Hw2					//
#define	HwSDIIENABLE_TDCFI_EN	Hw1						//
#define	HwSDIIENABLE_TDCFI_DIS	~Hw1					//
#define	HwSDIIENABLE_CCFI_EN		Hw0						//
#define	HwSDIIENABLE_CCFI_DIS		~Hw0					//

//Done...
/////////
/***********************************************************************             
*	 2D Accelerator Controller Register Define		(Base Addr = 0xF0001000) 		
************************************************************************/            

#define Hw2D_DMA_BASE_ADDR				0xF0001000					

#define	Hw2D_FCHO_SADDR0 	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR 	 ) 		// Front-End Channel 0 Source Address 0
#define	Hw2D_FCHO_SADDR1  	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x04)	// Front-End Channel 0 Source Address 1
#define	Hw2D_FCHO_SADDR2    	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x08) 	// Front-End Channel 0 Source Address 2
#define	Hw2D_FCHO_SFSIZE  	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x0C) 	// Front-End Channel 0 Source Frame Pixel Size
#define	Hw2D_FCHO_SOFF    		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x10) 	// Front-End Channel 0 Source Pixel Offset
#define	Hw2D_FCHO_SISIZE    	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x14) 	// Front-End Channel 0 Source Image Pixel Size
#define	Hw2D_FCHO_WOFF  		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x18) 	// Front-End Channel 0 Window Pixel Offset
#define	Hw2D_FCHO_SCTRL    	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x1C) 	// Front-End Channel 0 Control

#define	Hw2D_FCH1_SADDR0  	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x20) 	// Front-End Channel 1 Source Address 0
#define	Hw2D_FCH1_SADDR1    	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x24) 	// Front-End Channel 1 Source Address 1
#define	Hw2D_FCH1_SADDR2   	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x28) 	// Front-End Channel 1 Source Address 2
#define	Hw2D_FCH1_SFSIZE 		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x2C) 	// Front-End Channel 1 Source Frame Pixel Size
#define	Hw2D_FCH1_SOFF   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x30) 	// Front-End Channel 1 Source Pixel Offset
#define	Hw2D_FCH1_SISIZE 		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x34) 	// Front-End Channel 1 Source Image Pixel Size
#define	Hw2D_FCH1_WOFF 		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x38) 	// Front-End Channel 1 Window Pixel Offset
#define	Hw2D_FCH1_SCTRL 		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x3C) 	// Front-End Channel 1 Control

#define	Hw2D_FCH2_SADDR0    	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x40) 	// Front-End Channel 2 Source Address 0
#define	Hw2D_FCH2_SADDR1  	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x44) 	// Front-End Channel 2 Source Address 1
#define	Hw2D_FCH2_SADDR2  	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x48) 	// Front-End Channel 2 Source Address 2
#define	Hw2D_FCH2_SFSIZE  		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x4C) 	// Front-End Channel 2 Source Frame Pixel Size
#define	Hw2D_FCH2_SOFF   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x50) 	// Front-End Channel 2 Source Pixel Offset
#define	Hw2D_FCH2_SISIZE   	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x54) 	// Front-End Channel 2 Source Image Pixel Size
#define	Hw2D_FCH2_WOFF     	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x58) 	// Front-End Channel 2 Window Pixel Offset
#define	Hw2D_FCH2_SCTRL   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x5C) 	// Front-End Channel 2 Control

#define	Hw2D_FCH_SCTRL_OPMODE  	(Hw8+Hw9+Hw10)		//Operation Mode
#define	Hw2D_FCH_SCTRL_ZF  		(Hw5)				//Zero Fill
#define	Hw2D_FCH_SCTRL_SDFRM  	(Hw0+Hw1+Hw2+Hw3+Hw4)			//Source Data Format	




#define	Hw2D_S0_CHROMA   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x60) 	// Source 0 Chroma-Key Parameter
#define	Hw2D_S0_PAR 			*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x64) 	// Source 0 Arithmetic Parameter
#define	Hw2D_S1_CHROMA  		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x68) 	// Source 1 Chroma-Key Parameter
#define	Hw2D_S1_PAR  			*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x6C) 	// Source 1 Arithmetic Parameter
#define	Hw2D_S2_CHROMA   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x70) 	// Source 2 Chroma-Key Parameter
#define	Hw2D_S2_PAR   			*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x74) 	// Source 2 Arithmetic Parameter
#define	Hw2D_SCTRL   			*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x78) 	// Source Control Register

#define   Hw2D_SCTRL_S2_ARITHMODE 	(Hw27+Hw26+Hw25)
#define   Hw2D_SCTRL_S1_ARITHMODE 	(Hw24+Hw23+Hw22)
#define   Hw2D_SCTRL_S0_ARITHMODE 	(Hw21+Hw20+Hw19)
#define   Hw2D_SCTRL_S2_Y2REN 		(Hw18)
#define   Hw2D_SCTRL_S1_Y2REN 		(Hw17)
#define   Hw2D_SCTRL_S0_Y2REN 		(Hw16)
#define   Hw2D_SCTRL_S2_Y2RMODE 	(Hw14+Hw13)
#define   Hw2D_SCTRL_S1_Y2RMODE 	(Hw12+Hw11)
#define   Hw2D_SCTRL_S0_Y2RMODE 	(Hw10+Hw9)
#define   Hw2D_SCTRL_S2_CHROMAEN	(Hw8)
#define   Hw2D_SCTRL_S1_CHROMAEN	(Hw7)
#define   Hw2D_SCTRL_S0_CHROMAEN	(Hw6)
#define   Hw2D_SCTRL_S2_SEL			(Hw5+Hw4)
#define   Hw2D_SCTRL_S1_SEL			(Hw3+Hw2)
#define   Hw2D_SCTRL_S0_SEL			(Hw1+Hw0)



#define	Hw2D_OP0_PAT   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x80) 	// Source Operator 0 Pattern
#define	Hw2D_OP1_PAT   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x84) 	// Source Operator 1 Pattern
#define	Hw2D_OP_CTRL   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x88) 	// Source Operation Control Register

#define	Hw2D_OP_CTRL_CSEL1  		(Hw22+Hw21)
#define	Hw2D_OP_CTRL_OP1_MODE  	(Hw20+Hw19+Hw18+Hw17+Hw16)
#define	Hw2D_OP_CTRL_CSEL0  		(Hw6+Hw5)
#define	Hw2D_OP_CTRL_OP0_MODE  	(Hw4+Hw3+Hw2+Hw1+Hw0)



#define	Hw2D_BCH_DADDR0   	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x90) 	// Back-End Channel Destination Address 0
#define	Hw2D_BCH_DADDR1   	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x94) 	// Back -End Channel Destination Address 1
#define	Hw2D_BCH_DADDR2   	*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x98) 	// Back -End Channel Destination Address 2
#define	Hw2D_BCH_DFSIZE   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x9C) 	// Back -End Channel Destination Frame Pixel Size
#define	Hw2D_BCH_DOFF   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0xA0) 	// Back -End Channel Destination Pixel Offset
#define	Hw2D_BCH_DCTRL   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0xA4) 	// Back -End Channel Control

#define   Hw2D_BCH_DCTRL_YSEL		(Hw18)
#define   Hw2D_BCH_DCTRL_XSEL		(Hw17+Hw16)
#define   Hw2D_BCH_DCTRL_Y2R		(Hw15)
#define   Hw2D_BCH_DCTRL_Y2RMODE	(Hw14+Hw13)
#define   Hw2D_BCH_DCTRL_OPMODE	(Hw10+Hw9+Hw8)
#define   Hw2D_BCH_DCTRL_DDFRM	(Hw4+Hw3+Hw2+Hw1+Hw0)



#define	Hw2D_GE_CTRL   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0xB0) 	// Graphic Engine Control

#define	Hw2D_GE_CTRL_IEN			(Hw16)
#define	Hw2D_GE_CTRL_EN			(Hw0+Hw1+Hw2)



#define	Hw2D_GE_IREQ   		*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0xB4) 	// Graphic Engine Interrupt Request

#define	Hw2D_GE_IREQ_FLG			(Hw16)   
#define	Hw2D_GE_IREQ_IRQ			(Hw0)


/*
#define	Hw2D_CONTROL_BDTP   			Hw31 
#define	Hw2D_CONTROL_BSTP   			Hw30   
#define	Hw2D_CONTROL_BIEN   			Hw29   
#define	Hw2D_CONTROL_BFILL   			Hw28   
#define	Hw2D_CONTROL_BEN   				Hw27   
#define	Hw2D_CONTROL_FDTP   			Hw25   
#define	Hw2D_CONTROL_FSTP   			Hw24   
#define	Hw2D_CONTROL_FIEN   			Hw23   
//#define	Hw2D_CONTROL_FFIL   			Hw22   
#define	Hw2D_CONTROL_FFILL   			Hw22   
#define	Hw2D_CONTROL_FEN   				Hw21   
#define	Hw2D_CONTROL_CHEN   			Hw20   
#define	Hw2D_CONTROL_ROPFUNC   			(Hw16+Hw17+Hw18+Hw19)
#define	Hw2D_CONTROL_SCALARFUNC   		(Hw12+Hw13+Hw14+Hw15)   
#define	Hw2D_CONTROL_GEOFUNC   			(Hw8+Hw9+Hw10+Hw11)   
#define	Hw2D_CONTROL_MEMSIZE   			(Hw4+Hw5)   
#define	Hw2D_CONTROL_DMAG   			Hw3   
#define	Hw2D_CONTROL_LEN   				Hw2   
#define	Hw2D_CONTROL_IEN   				Hw1   
#define	Hw2D_CONTROL_MEN   				Hw0  
#define	Hw2D_INT       					*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x74) 			//
#define	Hw2D_INT_CB   					Hw10 
#define	Hw2D_INT_CF   					Hw9            
#define	Hw2D_INT_CD   					Hw8  
#define	Hw2D_INT_B   					Hw2  
#define	Hw2D_INT_F   					Hw1  
#define	Hw2D_INT_DONE   				Hw0  
#define	Hw2D_INTEN     					*(volatile unsigned long *)(Hw2D_DMA_BASE_ADDR +0x78) 			//
#define	Hw2D_INTEN_B   					Hw2   
#define	Hw2D_INTEN_F   					Hw1  
#define	Hw2D_INTEN_DONE   				Hw0  
*/

/***********************************************************************
*	 Camera Interface Register Define			(Base Addr = 0xF104000)
************************************************************************/
#define	HwICPCR1								*(volatile unsigned long *)0xF0000000	// W, Input Image Color/Pattern Configuration Register 1
#define	HwICPCR1_ON							Hw31									// 0 : Can't operate CIF, 1 : Operating CIF
#define	HwICPCR1_PWD							Hw30									// Power down mode in camera
#define	HwICPCR1_BPS							Hw23									// Bypass Scaler
#define	HwICPCR1_POL							Hw21									// PXCLK Polarity
#define	HwICPCR1_NOT_SKIP						HwZERO									// Not Skip
#define	HwICPCR1_M420_NOT_CVT				HwZERO									// Not Convert
#define	HwICPCR1_M420_ODD_SKIP				Hw17									// Converted in odd line skip
#define	HwICPCR1_M420_EVEN_SKIP				(Hw17+Hw16)								// Converted in even line skip
#define	HwICPCR1_BP							Hw15									// Bypass
#define	HwICPCR1_BBS_LSB8						Hw14									// When bypass 16bits mode, LSB 8bits are stored in first
#define	HwICPCR1_C656							Hw13									// Convert 656 format
#define	HwICPCR1_CP_RGB						Hw12									// RGB(555,565,bayer) color pattern
#define	HwICPCR1_PF_444						HwZERO									// 4:4:4 format
#define	HwICPCR1_PF_422						Hw10									// 4:2:2 format
#define	HwICPCR1_PF_420						Hw11									// 4:2:0 format or RGB(555,565,bayer) mode
#define	HwICPCR1_RGBM_BAYER					HwZERO									// Bayer RGB Mode
#define	HwICPCR1_RGBM_RGB555					Hw8										// RGB555 Mode
#define	HwICPCR1_RGBM_RGB565					Hw9										// RGB565 Mode
#define	HwICPCR1_RGBBM_16					HwZERO									// 16bit mode
#define	HwICPCR1_RGBBM_8DISYNC				Hw6										// 8bit disable sync
#define	HwICPCR1_RGBBM_8						Hw7										// 8bit mode
#define	HwICPCR1_CS_RGBMG					HwZERO									// 555RGB:RGB(MG), 565RGB:RGB, 444/422/420:R/Cb/U first, Bayer RGB:BG->GR, CCIR656:YCbYCr
#define	HwICPCR1_CS_RGBLG					Hw4										// 555RGB:RGB(LG), 565RGB:RGB, 444/422/420:R/Cb/U first, Bayer RGB:GR->BG, CCIR656:YCrYCb
#define	HwICPCR1_CS_BGRMG					Hw5										// 555RGB:BGR(MG), 565RGB:BGR, 444/422/420:B/Cr/V first, Bayer RGB:RG->GB, CCIR656:CbYCrY
#define	HwICPCR1_CS_BGRLG					(Hw5+Hw4)								// 555RGB:BGR(LG), 565RGB:BGR, 444/422/420:B/Cr/V first, Bayer RGB:GB->RG, CCIR656:CrYCbY
#define	HwICPCR1_BO_SW						Hw2										// Switch the MSB/LSB 8bit Bus
#define	HwICPCR1_HSP_HIGH					Hw1										// Active high
#define	HwICPCR1_VSP_HIGH					Hw0										// Active high
                                    		
#define	Hw656FCR1								*(volatile unsigned long *)0xF0000004	// W, CCIR656 Format Configuration Register 1
#define	Hw656FCR1_PSL_1ST						HwZERO									// The status word is located the first byte of EAV & SAV
#define	Hw656FCR1_PSL_2ND					Hw25									// The status word is located the second byte of EAV & SAV
#define	Hw656FCR1_PSL_3RD						Hw26									// The status word is located the third byte of EAV & SAV
#define	Hw656FCR1_PSL_4TH						(Hw26+Hw25)								// The status word is located the forth byte of EAV & SAV
                                    		
#define	Hw656FCR2								*(volatile unsigned long *)0xF0000008	// W, CCIR656 Format Configuration Register 2
                                    		
#define	HwIIS									*(volatile unsigned long *)0xF000000C	// W, Input Image Size
                                    		
#define	HwIIW1									*(volatile unsigned long *)0xF0000010	// W, Input Image Windowing 1
                                    		
#define	HwIIW2									*(volatile unsigned long *)0xF0000014	// W, Input Image Windowing 2
                                    		
#define	HwCDCR1								*(volatile unsigned long *)0xF0000018	// W, DMA Configuratin Register 1
#define	HwCDCR1_TM_INC						Hw3										// INC Transfer
#define	HwCDCR1_LOCK_ON						Hw2										// Lock Transfer
#define	HwCDCR1_BS_1							HwZERO									// The DMA transfers the image data as 1 word to memory
#define	HwCDCR1_BS_2							Hw0										// The DMA transfers the image data as 2 word to memory
#define	HwCDCR1_BS_4							Hw1										// The DMA transfers the image data as 4 word to memory
#define	HwCDCR1_BS_8							(Hw1+Hw0)								// The DMA transfers the image data as 8 word to memory (default)
                                    		
#define	HwCDCR2								*(volatile unsigned long *)0xF000001C	// W, DMA Configuration Register 2
                                    		
#define	HwCDCR3								*(volatile unsigned long *)0xF0000020// W, DMA Configuration Register 3
                                    		
#define	HwCDCR4								*(volatile unsigned long *)0xF0000024	// W, DMA Configuration Register 4

#define	HwCDCR5								*(volatile unsigned long *)0xF0000028	// W, DMA Configuration Register 5

#define	HwCDCR6								*(volatile unsigned long *)0xF000002C	// W, DMA Configuration Register 6
                                    		
#define	HwCDCR7								*(volatile unsigned long *)0xF0000030	// W, DMA Configuration Register 7
                                    		
#define	HwFIFOSTATE							*(volatile unsigned long *)0xF0000034	// R, FIFO Status Register
#define	HwFIFOSTATE_CLR						Hw21									// 1:Clear, 0:Not Clear
#define	HwFIFOSTATE_REO						Hw19									// 1:The empty signal of input overlay FIFO and read enable signal are High, 0:The empty signal of overlay FIFO is low, or empty is High and read enable signal is Low.
#define	HwFIFOSTATE_REV						Hw18									// 1:The empty signal of input V(B) channel FIFO and read enable signal are High, 0:The empty signal of V(B) channel FIFO is Low, or empty is High and read enable signal is Low.
#define	HwFIFOSTATE_REU						Hw17									// 1:The empty signal of input U(R) channel FIFO and read enable signal are High, 0:The empty signal of U(R) channel FIFO is Low, or empty is High and read enable signal is Low.
#define	HwFIFOSTATE_REY						Hw16									// 1:The empty signal of input Y(G) channel FIFO and read enable signal are High, 0:The empty signal of Y(G) channel FIFO is Low, or empty is High and read enable signal is Low.
#define	HwFIFOSTATE_WEO						Hw13									// 1:The full signal of overlay FIFO and write enable signal are High, 0:The full signal of overlay FIFO is Low, or full is High and write enable signal is Low.
#define	HwFIFOSTATE_WEV						Hw12									// 1:The full signal of V(B) channel FIFO and write enable signal are High, 0:The full signal of V(B) channel FIFO is Low, or full is High and write enable signal is Low.
#define	HwFIFOSTATE_WEU						Hw11									// 1:The full signal of U(R) channel FIFO and write enable signal are High, 0:The full signal of U(R) channel FIFO is Low, or full is High and write enable signal is Low.
#define	HwFIFOSTATE_WEY						Hw10									// 1:The full signal of Y channel FIFO and write enable signal are High, 0:The full signal of Y channel FIFO is Low, or full is High and write enable signal is Low.
#define	HwFIFOSTATE_EO						Hw8										// 1:The state of overlay FIFO is empty, 0:The state of overlay FIFO is non-empty.
#define	HwFIFOSTATE_EV						Hw7										// 1:The state of V(B) channel FIFO is empty, 0:The state of V(B) channel FIFO is non-empty.
#define	HwFIFOSTATE_EU						Hw6										// 1:The state of U(R) channel FIFO is empty, 0:The state of U(R) channel FIFO is non-empty.
#define	HwFIFOSTATE_EY						Hw5										// 1:The state of Y channel FIFO is empty, 0:The state of Y channel FIFO is non-empty.
#define	HwFIFOSTATE_FO						Hw3										// 1:The state of overlay FIFO is full, 0:The state of overlay FIFO is non-full.
#define	HwFIFOSTATE_FV						Hw2										// 1:The state of V(B) channel FIFO is full, 0:The state of V(B) channel FIFO is non-full.
#define	HwFIFOSTATE_FU						Hw1										// 1:The state of U(R) channel FIFO is full, 0:The state of U(R) channel FIFO is non-full.
#define	HwFIFOSTATE_FY						Hw0										// 1:The state of Y channel FIFO is full, 0:The state of Y channel FIFO is non-full.
                                    		
#define	HwCIRQ									*(volatile unsigned long *)0xF0000038	// R/W, Interrupt & CIF Operating Register
#define	HwCIRQ_IEN_EN							Hw31									// Interrupt Enable
#define	HwCIRQ_URV							Hw30									// Update Register in VSYNC
#define	HwCIRQ_ITY								Hw29									// Interrupt Type
#define	HwCIRQ_ICR								Hw28									// Interrupt Clear
#define	HwCIRQ_MVN							Hw26									// Mask Interrupt of VS negative edge
#define	HwCIRQ_MVP							Hw25									// Mask Interrupt of VS positive edge
#define	HwCIRQ_MVIT							Hw24									// Mask Interrupt of VCNT INterrupt
#define	HwCIRQ_MSE							Hw23									// Mask Interrupt of Scaler Error
#define	HwCIRQ_MSF							Hw22									// Mask Interrupt of Scaler finish
#define	HwCIRQ_MENS							Hw21									// Mask Interrupt of Encoding start
#define	HwCIRQ_MRLV							Hw20									// Mask Interrupt of Rolling V address
#define	HwCIRQ_MRLU							Hw19									// Mask Interrupt of Rolling U address
#define	HwCIRQ_MRLY							Hw18									// Mask Interrupt of Rolling Y address
#define	HwCIRQ_MSCF							Hw17									// Mask Interrupt of Capture frame
#define	HwCIRQ_MSOF							Hw16									// Mask Interrupt of Stored one frame
#define	HwCIRQ_VSS							Hw12									// Status of vertical sync 
#define	HwCIRQ_VN								Hw10									// VS negative
#define	HwCIRQ_VP								Hw9										// VS positive
#define	HwCIRQ_VIT								Hw8										// VCNT Interrupt
#define	HwCIRQ_SE								Hw7										// Scaler Error
#define	HwCIRQ_SF								Hw6										// Scaler Finish
#define	HwCIRQ_ENS							Hw5										// Encoding start stauts
#define	HwCIRQ_ROLV							Hw4										// Rolling V address status
#define	HwCIRQ_ROLU							Hw3										// Rolling U address status
#define	HwCIRQ_ROLY							Hw2										// Rolling Y address status
#define	HwCIRQ_SCF							Hw1										// Stored captured frame
#define	HwCIRQ_SOF							Hw0										// Stored one frame
                                    		
#define	HwOCTRL1								*(volatile unsigned long *)0xF000003C	// W, Overlay Control 1
#define	HwOCTRL1_OCNT_MAX					(Hw27+Hw26+Hw25+Hw24+Hw23)				//
#define	HwOCTRL1_OM_BLOCK					Hw16									// Block image overlay
#define	HwOCTRL1_OE_EN						Hw12									// Overlay enable
#define	HwOCTRL1_XR1_100						Hw10									// 100%
#define	HwOCTRL1_XR0_100						Hw9										// 100%
#define	HwOCTRL1_AP1_25						HwZERO									// 25%			
#define	HwOCTRL1_AP1_50						Hw6										// 50%
#define	HwOCTRL1_AP1_75						Hw7										// 75%				
#define	HwOCTRL1_AP1_100						(Hw7+Hw6)								// 100%		
#define	HwOCTRL1_AP0_25						HwZERO									// 25%			
#define	HwOCTRL1_AP0_50						Hw4										// 50%
#define	HwOCTRL1_AP0_75						Hw5										// 75%				
#define	HwOCTRL1_AP0_100						(Hw5+Hw4)								// 100%
#define 	HwOCTRL1_AEN_EN						Hw2										// Alpha enable
#define 	HwOCTRL1_CEN_EN						Hw0										// Chroma key enable
                                    		
#define	HwOCTRL2								*(volatile unsigned long *)0xF0000040	// W, Overlay Control 4
#define	HwOCTRL2_CONV						Hw3										// Color Converter Enable
#define	HwOCTRL2_RGB_565						HwZERO									// 565RGB
#define	HwOCTRL2_RGB_555						Hw1										// 555RGB
#define	HwOCTRL2_RGB_444						Hw2										// 444RGB
#define	HwOCTRL2_RGB_332						(Hw2+Hw1)								// 332RGB
#define	HwOCTRL2_MD							Hw0										// Color Mode, 0 = YUB, 1 = RGB
                                    		
#define	HwOCTRL3								*(volatile unsigned long *)0xF0000044	// W, Overlay Control 2
#define	HwOCTRL3_KEYR_MAX					0x00FF0000
#define	HwOCTRL3_KEYG_MAX					0x0000FF00
#define	HwOCTRL3_KEYB_MAX					0x000000FF
                                    		
#define	HwOCTRL4								*(volatile unsigned long *)0xF0000048	// W, Overlay Control 3
#define	HwOCTRL4_MKEYR_MAX					0x00FF0000
#define	HwOCTRL4_MKEYG_MAX					0x0000FF00
#define	HwOCTRL4_MKEYB_MAX					0x000000FF
                                    		
#define	HwOIS									*(volatile unsigned long *)0xF000004C	// W, Overlay Image Size
                                    		
#define	HwOIW1									*(volatile unsigned long *)0xF0000050	// W, Overlay Image Windowing 1
                                    		
#define	HwOIW2									*(volatile unsigned long *)0xF0000054	// W, Overlay Image Windowing 2
                                    		
#define	HwCOBA									*(volatile unsigned long *)0xF0000058	// W, Overlay Base Address
                                    		
#define	HwCDS									*(volatile unsigned long *)0xF000005C	// W, Camera Down Scaler
#define	HwCDS_SFH_1							HwZERO									// 1/1 down scale
#define	HwCDS_SFH_2							Hw4										// 1/2 down scale
#define	HwCDS_SFH_4							Hw5										// 1/4 down scale
#define	HwCDS_SFH_8							(Hw5+Hw4)								// 1/8 down scale
#define	HwCDS_SFV_1							HwZERO									// 1/1 down scale
#define	HwCDS_SFV_2							Hw2										// 1/2 down scale
#define	HwCDS_SFV_4							Hw3										// 1/4 down scale
#define	HwCDS_SFV_8							(Hw3+Hw2)								// 1/8 down scale
#define HwCDS_SEN_EN							Hw0										// Scale enable
                                    		
#define	HwCCM1								*(volatile unsigned long *)0xF0000060	// R/W, Capture Mode 1
#define	HwCCM1_CB								Hw10									// Capture Busy
#define	HwCCM1_EIT							Hw9										// Encoding INT count
#define	HwCCM1_UES							Hw8										// Using Encoding Start Address
#define	HwCCM1_RLV							Hw3										// Rolling address V
#define	HwCCM1_RLU							Hw2										// Rolling address U
#define	HwCCM1_RLY							Hw1										// Rolling address Y
#define	HwCCM1_CAP							Hw0										// Image Capture
                                    		
#define	HwCCM2								*(volatile unsigned long *)0xF0000064	// R/W, Capture Mode 2
#define	HwCCM2_VEN							Hw0										// VCNT folling enable
                                    		
#define	HwCESA									*(volatile unsigned long *)0xF0000068	// R/W, Encoding Start Address
                                    		
#define	HwCR2Y									*(volatile unsigned long *)0xF000006C	// R/W, R2Y Configuration
#define	HwCR2Y_565RGB_SEQ16					HwZERO									// 16bit 565RGB (RGB sequence)
#define	HwCR2Y_565BGR_SEQ16					Hw1										// 16bit 565RGB (BGR sequence)
#define	HwCR2Y_555RGB_GAR16					Hw3										// 16bit 555RGB	(RGB-garbage)
#define	HwCR2Y_555BGR_GAR16					(Hw3+Hw1)								// 16bit 555RGB (BGR-garbage)
#define	HwCR2Y_555GAR_RGB16					(Hw3+Hw2)								// 16bit 555RGB	(garbage-RGB)
#define	HwCR2Y_555GAR_BGR16					(Hw3+Hw2+Hw1)							// 16bit 555RGB (garbage-BGR)
#define	HwCR2Y_565RGB_SEQ8					Hw4										// 8bit 565RGB (RGB sequence)
#define	HwCR2Y_565BGR_SEQ8					(Hw4+Hw1)								// 8bit 565RGB (BGR sequence)
#define	HwCR2Y_555RGB_GAR8					(Hw4+Hw3)								// 8bit 555RGB (RGB-garbage)
#define	HwCR2Y_555BGR_GAR8					(Hw4+Hw3+Hw1)							// 8bit 555RGB (BGR-garbage)
#define	HwCR2Y_555GAR_RGB8					(Hw4+Hw3+Hw2)							// 8bit 555RGB (garbage-RGB)
#define	HwCR2Y_555GAR_BGR8					(Hw4+Hw3+Hw2+Hw1)						// 8bit 555RGB (garbage-BGR)
#define	HwCR2Y_EN								Hw0										// R2Y Enable
                                    		
#define	HwCCYA									*(volatile unsigned long *)0xF0000070	// R, Current Y Address
                                    		
#define	HwCCUA									*(volatile unsigned long *)0xF0000074	// R, Current U Address
                                    		
#define	HwCCVA									*(volatile unsigned long *)0xF0000078	// R, Current V Address
                                    		
#define	HwCCLC									*(volatile unsigned long *)0xF000007C// R, Current Line Count
                                    		
#define	HwCEM									*(volatile unsigned long *)0xF0000100	// R/W, Effect Mode Register
#define	HwCEM_UVS								Hw15									// UV Swap
#define	HwCEM_VB								Hw14									// V Bias (V Channel value offset)
#define	HwCEM_UB								Hw13									// U Bias (U Channel value offset)
#define	HwCEM_YB								Hw12									// Y Bias (Y Channel value offset)
#define	HwCEM_YCS								Hw11									// YC Swap
#define	HwCEM_IVY								Hw10									// Invert Y
#define	HwCEM_STC								Hw9										// Strong C
#define	HwCEM_YCL								Hw8										// Y Clamp (Y value clipping)
#define	HwCEM_CS								Hw7										// C Select (Color filter)
#define	HwCEM_SKT								Hw6										// Sketch Enable
#define	HwCEM_EMM							Hw5										// Emboss Mode
#define	HwCEM_EMB								Hw4										// Emboss
#define	HwCEM_NEGA							Hw3										// Nagative Mode
#define	HwCEM_GRAY							Hw2										// Gray Mode
#define	HwCEM_SEPI							Hw1										// Sepia Mode
#define	HwCEM_NOR								Hw0										// Normal Mode
                                    		
#define	HwCSUV									*(volatile unsigned long *)0xF0000104	// Sepia UV
                                    		
#define 	HwCCS									*(volatile unsigned long *)0xF0000108	// Color Selection
                                    		
#define	HwCHFC									*(volatile unsigned long *)0xF000010C	// H filter Coefficient
                                    		
#define	HwCST									*(volatile unsigned long *)0xF0000110	// Sketch Threshold
                                    		
#define	HwCCT									*(volatile unsigned long *)0xF0000114	// Clamp Threshold
                                    		
#define	HwCBR									*(volatile unsigned long *)0xF0000118	// Bias Register
                                    		
#define	HwCEIS									*(volatile unsigned long *)0xF000011C	// Effect Image Size
                                    		
#define	HwCIC									*(volatile unsigned long *)0xF0000140	// Inpath Control
#define	HwCIC_INPR								Hw3										//
#define	HwCIC_FA								Hw2										// Flush All
#define	HwCIC_INE								Hw1										// Inpath Enable
#define	HwCIC_INP								Hw0										// Inpath Mode
                                    		
#define	HwCISA1								*(volatile unsigned long *)0xF0000144	// Inpath SRC Address 1
                                    		
#define	HwCISA2								*(volatile unsigned long *)0xF0000148	// Inpath SRC Address 2
#define	HwCISA2_SRCTYPE_422SEQ0				HwZERO									// 4:2:2 SEQ0
#define	HwCISA2_SRCTYPE_422SEQ1				Hw28									// 4:2:2 SEQ1
#define	HwCISA2_SRCTYPE_422SEP				Hw29									// 4:2:2 Separate
#define	HwCISA2_SRCTYPE_420SEP				(Hw29+Hw28)								// 4:2:0 Separate
                                    		
#define	HwCISA3								*(volatile unsigned long *)0xF000014C	// Inpath SRC Address 3
                                    		
#define	HwCISS									*(volatile unsigned long *)0xF0000150	// Inpath SRC Size
                                    		
#define	HwCISO									*(volatile unsigned long *)0xF0000154	// Inpath SRC Offset
                                    		
#define	HwCIDS									*(volatile unsigned long *)0xF0000158	// Inpath DST Size
                                    		
#define	HwCIS									*(volatile unsigned long *)0xF000015C	// Inpath Scale
                                    		
#define	HwCSC									*(volatile unsigned long *)0xF0000200	// Scaler CTRL

#define	HwCSC_EN								Hw0										// Scaler Enable
                                    		
#define	HwCSSF									*(volatile unsigned long *)0xF0000204	// Scaler Scale Factor
                                    		
#define	HwCSSO									*(volatile unsigned long *)0xF0000208	// Scaler SRC Offset
                                    		
#define	HwCSSS									*(volatile unsigned long *)0xF000020C	// Scaler SRC Size
                                    		
#define	HwCSDS									*(volatile unsigned long *)0xF0000210	// Scaler DST Size




/***********************************************************************
*	 JPEG Codec Controller Register Define				(Base Addr = 0xF1006000)
************************************************************************/
#define	HwJP_RST						*(volatile unsigned long *) 0xF0002000		/* R, JPEG Block Soft Reset Register */
#define	HwJP_RST_BLOCK					Hw0										// Block Reset Includes All Tables.

#define	HwJP_MOD						*(volatile unsigned long *) 0xF0002004		/* R/W, JPEG Codec Mode Register */
#define	HwJP_MOD_JPC					HwZERO								/* JPEG Encoder Mode */
#define	HwJP_MOD_JPD					Hw16									/* JPEG Decoder Mode */
#define	HwJP_MOD_DCT					Hw17									/* DCT Mode */
#define	HwJP_MOD_IDCT					(Hw16|Hw17)						/* IDCT Mode */
#define	HwJP_MOD_MASTER					HwZERO								/* Master Mode */  
#define	HwJP_MOD_SLAVE					Hw0										/* Slave Mode */
	
#define	HwJP_INT_MASK					*(volatile unsigned long *) 0xF0002008		/* R/W, Interrupt Mask Register */
#define HwJP_INT_MASK_OPERATION_END     Hw0       /* Encode/Decode Complete */
#define HwJP_INT_MASK_ERROR             Hw1       /* Encode/Decode Error */
#define HwJP_INT_MASK_OUTPUT_FIFO       Hw2       /* Output FIFO Buffer Status */
#define HwJP_INT_MASK_INPUT_FIFO        Hw3       /* Input FIFO Buffer Status */
#define HwJP_INT_MASK_CODED_BUFFER      Hw4       /* Output Buffer's Full/Empty Status */

#define	HwJP_INT_LEVEL					*(volatile unsigned long *) 0xF000200C		/* R/W, FIFO Interrupt Level Register */

#define	HwJP_TRG_MOD					*(volatile unsigned long *) 0xF0002010		/* R/W, Polling or Interrupt Mode Selection Register */
#define	HwJP_TRG_MOD_POLL				HwZERO								/* Polling Mode. No Interrupt Generated */
#define	HwJP_TRG_MOD_INT				Hw0										/* Interrupt Mode */
	
#define	HwJP_R_YBUF_ADDR				*(volatile unsigned long *) 0xF0002020		/* R/W, Raw Data Buffer Y Address Register */

#define	HwJP_R_UBUF_ADDR				*(volatile unsigned long *) 0xF0002024		/* R/W, Raw Data Buffer U Address Register */

#define	HwJP_R_VBUF_ADDR				*(volatile unsigned long *) 0xF0002028		/* R/W, Raw Data Buffer V Address Register */

#define	HwJP_R_BUF_INFO					*(volatile unsigned long *) 0xF000202C		/* R/W, Raw Data Buffer Information Register */
	
#define	HwJP_SIZE						*(volatile unsigned long *) 0xF0002030		/* R/W, Image Size Information Register */
	
#define	HwJP_CHROMA						*(volatile unsigned long *) 0xF0002034		/* R/W, Image Format Information Register */
#define	HwJP_CHROMA_YONLY				HwZERO								/* Y Only */
#define	HwJP_CHROMA_420					Hw0										/* YUV420 (Y,U,V Separated Mode) */
#define	HwJP_CHROMA_422					Hw1										/* YUV422 (Y,U,V Separated Mode) */
#define	HwJP_CHROMA_444					(Hw0|Hw1)							/* YUV444 (Y,U,V Separated Mode) */
#define	HwJP_CHROMA_422S				Hw2										/* YUV422S (Y,U,V Separated Mode) */

#define	HwJP_CBUF_ADDR					*(volatile unsigned long *) 0xF0002038		/* R/W, Coded Buffer Address Register */

#define	HwJP_CBUF_SIZE					*(volatile unsigned long *) 0xF000203C		/* R/W, Coded Buffer Size Register */

#define	HwJPD_TBL_ID					*(volatile unsigned long *) 0xF0002050		/* R/W, Decoder Table Index Register */

#define	HwJPD_RST_INTV					*(volatile unsigned long *) 0xF0002054		/* R/W, Decoder Reset Interval Register */

#define	HwJPD_OUT_SCL					*(volatile unsigned long *) 0xF0002058		/* R/W, Decoder Output Scaling Register */
#define	HwJPD_OUT_SCL_4					Hw0										/* 1/4 (Area Ratio) */
#define	HwJPD_OUT_SCL_16				Hw1										/* 1/16 (Area Ratio) */
	
#define	HwJP_SBUF_RP_A					*(volatile unsigned long *) 0xF0002060		/* R/W, Source Read Pointer Address Register */

#define	HwJP_DBUF_WP_A					*(volatile unsigned long *) 0xF0002064		/* R/W, Destination Write Pointer Address Register */
	
#define	HwJP_START						*(volatile unsigned long *) 0xF0002070		/* W, Codec Start Command Register */
#define	HwJP_START_RUN					Hw0										// Codec Start

#define	HwJP_SBUF_WCNT					*(volatile unsigned long *) 0xF0002080		/* R/W, Source Buffer Write Count Register */

#define	HwJP_SBUF_RCNT					*(volatile unsigned long *) 0xF0002084		/* R, Source Buffer Read Count Register */

#define	HwJP_DBUF_WCNT					*(volatile unsigned long *) 0xF0002088		/* R/W, Destination Buffer Write Count Register */

#define	HwJP_DBUF_RCNT					*(volatile unsigned long *) 0xF000208C		/* R, Destination Buffer Read Count Register */
	
#define	HwJP_IFIFO_ST					*(volatile unsigned long *) 0xF0002090		/* R, Input FIFO Status Register */
	
#define	HwJP_OFIFO_ST					*(volatile unsigned long *) 0xF0002094		/* R, Output FIFO Status Register */
	
#define	HwJP_INT_FLAG					*(volatile unsigned long *) 0xF00020A0		/* R, Interrupt Flag Register */
#define	HwJP_INT_FLAG_CODED_BUF_STAT	Hw4					/* Coded Buffer Status */
#define	HwJP_INT_FLAG_IFIFO_STAT		Hw3					/* Input FIFO Status */
#define	HwJP_INT_FLAG_OFIFO_STAT		Hw2					/* Output FIFO Status */
#define	HwJP_INT_FLAG_DECODING_ERR		Hw1					/* Decoding Error */
#define	HwJP_INT_FLAG_JOB_FINISHED		Hw0					/* Job Finished */
	
#define	HwJP_INT_ACK					*(volatile unsigned long *) 0xF00020A4		/* R, Interrupt ACK Register */

#define	HwJP_IFIFO_WD					*(volatile unsigned long *) 0xF00020C0		/* W, Input FIFO Write Data Register */

#define	HwJP_OFIFO_RD					*(volatile unsigned long *) 0xF00020E0		/* R, Output FIFO Read Data Register */
	
#define	HwJPC_QTAB0						*(volatile unsigned long *) 0xF0002100		/* W, Encoder Q Table0 (64 Entries) */

#define	HwJPC_QTAB1						*(volatile unsigned long *) 0xF0002200		/* W, Encoder Q Tabel1 (64 Entries) */

#define	HwJPD_IQTAB0					*(volatile unsigned long *) 0xF0002300		/* W, Decoder IQ Table0 (64 Entries) */

#define	HwJPD_IQTAB1					*(volatile unsigned long *) 0xF0002400		/* W, Decoder IQ Table1 (64 Entries) */

#define	HwJPD_IQTAB2					*(volatile unsigned long *) 0xF0002500		/* W, Decoder IQ Table2 (64 Entries) */

#define	HwJPD_HT_DC0_C					*(volatile unsigned long *) 0xF0002600		/* W, Decoder Huffman Table (DC0 Code, 16 Entries) */

#define	HwJPD_HT_AC0_C					*(volatile unsigned long *) 0xF0002640		/* W, Decoder Huffman Table (AC0 Code, 16 Entries) */

#define	HwJPD_HT_DC1_C					*(volatile unsigned long *) 0xF0002680		/* W, Decoder Huffman Table (DC1 Code, 16 Entries) */

#define	HwJPD_HT_AC1_C					*(volatile unsigned long *) 0xF00026C0		/* W, Decoder Huffman Table (AC1 Code, 16 Entries) */

#define	HwJPD_HT_DC0_A					*(volatile unsigned long *) 0xF0002700		/* W, Decoder Huffman Table (DC0 Addr, 16 Entries) */

#define	HwJPD_HT_AC0_A					*(volatile unsigned long *) 0xF0002740		/* W, Decoder Huffman Table (AC0 Addr, 16 Entries) */

#define	HwJPD_HT_DC1_A					*(volatile unsigned long *) 0xF0002780		/* W, Decoder Huffman Table (DC1 Addr, 16 Entries) */

#define	HwJPD_HT_AC1_A					*(volatile unsigned long *) 0xF00027C0		/* W, Decoder Huffman Table (AC1 Addr, 16 Entries) */

#define	HwJPD_HT_DC0_V					*(volatile unsigned long *) 0xF0002800		/* W, Decoder Huffman Table (DC0 Var, 12 Entries) */

#define	HwJPD_HT_AC0_V					*(volatile unsigned long *) 0xF0002840		/* W, Decoder Huffman Table (AC0 Var, 162 Entries) */

#define	HwJPD_HT_DC1_V					*(volatile unsigned long *) 0xF0002C00		/* W, Decoder Huffman Table (DC1 Var, 12 Entries) */

#define	HwJPD_HT_AC1_V					*(volatile unsigned long *) 0xF0002C40		/* W, Decoder Huffman Table (AC1 Var, 162 Entries) */



/***********************************************************************
*	External Host Interface Register Define				(Base Addr = 0xF1008000)
************************************************************************/
/////////
//Done...
#define	HwEHST			*(volatile unsigned long *)0xF1008000	// R/W, Status register
#define	HwEHST_RDY				Hw7						//

#define	HwEHIINT		*(volatile unsigned long *)0xF1008004	// R/W,  Internal interrupt control register
#define	HwEHIINT_IIRQ				Hw0						//

#define	HwEHEINT		*(volatile unsigned long *)0xF1008008	// R/W,  External interrupt control register
#define	HwEHEINT_EIRQ				Hw0						//

#define	HwEHA			*(volatile unsigned long *)0xF100800C	// R/W,  Address register

#define	HwEHAM			*(volatile unsigned long *)0xF1008010	// R,  Address masking register

#define	HwEHD			*(volatile unsigned long *)0xF1008014	// R/W,  Data register

#define	HwEHSEM			*(volatile unsigned long *)0xF1008018	// R/W,  Semaphore register
#define	HwEHSEM_FLG_NOT			HwZERO				//
#define	HwEHSEM_FLG_RWERTCC77X	Hw0						//
#define	HwEHSEM_FLG_RWTCC77XRE	Hw1						//
#define	HwEHSEM_FLG_NA			(Hw1|Hw0)				//

#define	HwEHCFG			*(volatile unsigned long *)0xF100801C	// R/W,  Configuration register
#define	HwEHCFG_RDYP				Hw4						//
#define	HwEHCFG_RDYE_RDY			Hw3						//
#define	HwEHCFG_RDYE_IRQ			HwZERO					//
#define	HwEHCFG_BW_16			Hw2						//
#define	HwEHCFG_BW_8				HwZERO					//
#define	HwEHCFG_MD_68			Hw0						//
#define	HwEHCFG_MD_80			HwZERO					//

#define	HwEHIND			*(volatile unsigned long *)0xF1008020	// W,  Index register

#define	HwEHRWCS		*(volatile unsigned long *)0xF1008024	// R/W,  Read/Write Control/Status register
#define	HwEHRWCS_AI				Hw7						//
#define	HwEHRWCS_LOCK_ON			Hw6						//
#define	HwEHRWCS_LOCK_OFF		~Hw6					//
#define	HwEHRWCS_RW_EHI			~(Hw5|Hw4)				//
#define	HwEHRWCS_RW_WAHB		Hw4						//
#define	HwEHRWCS_RW_RAHB		Hw5						//
#define	HwEHRWCS_RW_NA			(Hw5|Hw4)				//

//Done...
/////////

/***********************************************************************
*	 IDE Controller Register Define				(Base Addr = 0xF1009000)
************************************************************************/
/////////
//Done...

#define	HwCS0n			*(volatile unsigned long *)0xF1009000	// R/W, PIO CS0n Access register
#define	HwCS00				0xF1009000						// PIO CS00 Register Address

#define	REG_PTR    			*(volatile unsigned short *)			// get the content of pointer
#define	IDE_BASE_CS0		0xF1009000
#define	HwIDE_RD_DATA		(REG_PTR(IDE_BASE_CS0+(0x00<<2)+ 0x00))
#define	HwIDE_WR_DATA	HwIDE_RD_DATA

#define	HwCS1n				*(volatile unsigned long *)0xF1009020	// R/W, PIO CS1n Access register
#define	HwCS10				0xF1009020						// PIO CS10 Register Address

#define	HwPIOCTRL			*(volatile unsigned long *)0xF1009040	// R/W, PIO Mode control register
#define	HwPIOCTRL_SYNC_BYPASS	HwZERO					// Bypass
#define	HwPIOCTRL_SYNC_1			Hw21					// 1 Sync
#define	HwPIOCTRL_SYNC_2			Hw22					// 2 Sync
#define	HwPIOCTRL_MD_PIO			HwZERO					// PIO Mode
#define	HwPIOCTRL_MD_UDMA		Hw20					// UDMA(IN/OUT) Mode
#define	HwPIOCTRL_RDY_IRR			HwZERO					// PW cycles is irrelative of IORDY
#define	HwPIOCTRL_RDY_EXT			Hw0						// PW cycles are extended by IORDY
//Done...
/////////

/***********************************************************************
*	USB Controller Register Define				(Base Addr = 0xF0008000)
************************************************************************/
/////////
//Done...
#define	HwIR			*(volatile unsigned long *)0xF0008000	// Index Register

#define	HwEIR			*(volatile unsigned long *)0xF0008004	// Endpoint Interrupt Register
#define	HwEIR_EP0INT		Hw0
#define	HwEIR_EP1INT		Hw1
#define	HwEIR_EP2INT		Hw2
#define	HwEIR_EP3INT		Hw3

#define	HwEIER			*(volatile unsigned long *)0xF0008008	// Endpoint Interrupt Enable Register
#define	HwEIER_EP0INT_EN	Hw0
#define	HwEIER_EP0INT_DIS	~Hw0
#define	HwEIER_EP1INT_EN	Hw1
#define	HwEIER_EP1INT_DIS	~Hw1
#define	HwEIER_EP2INT_EN	Hw2
#define	HwEIER_EP2INT_DIS	~Hw2
#define	HwEIER_EP3INT_EN	Hw3
#define	HwEIER_EP3INT_DIS	~Hw3

#define	HwFAR			*(volatile unsigned long *)0xF000800C	// Function Address Register

#define HwFNR			*(volatile unsigned long *)0xF0008010	// Frame Number Register
#define HwFNR_FTL			Hw14							// Frame Timer Lock
#define HwFNR_SM			Hw13							// SOF Missing
#define HwFNR_FN_MASK		0x7FF							// Frame Number Mask

#define	HwEDR			*(volatile unsigned long *)0xF0008014	// Endpoint Direction Register
#define	HwEDR_EP1_TX		Hw1
#define	HwEDR_EP1_RX		~Hw1
#define	HwEDR_EP2_TX		Hw2
#define	HwEDR_EP2_RX		~Hw2
#define	HwEDR_EP3_TX		Hw3
#define	HwEDR_EP3_RX		~Hw3

#define	HwUTST			*(volatile unsigned long *)0xF0008018	// Test Register
#define	HwUTST_VBUS_ON		Hw15
#define	HwUTST_VBUS_OFF		~Hw15
#define	HwUTST_EUERR		Hw13							// EB underrun error in transceiver
#define	HwUTST_PERR			Hw12							// PID Error Flag
#define HwUTST_FDWR_EN		Hw11							// FIFO Direct Write Read Enable
#define HwUTST_FDWR_DIS		~Hw11							// FIFO Direct Write Read Disable
#define HwUTST_SPDSEL_RESET	((~Hw7)&(~Hw6))					// SPEED Select Mode ; reset, Normal Operation
#define HwUTST_SPDSEL_FS	Hw7								// SPEED Select Mode ; Full speed fixed, Hw6 have to be '0'
#define HwUTST_SPDSEL_HS	Hw6								// SPPED Select Mode ; High speed fixed, Hw7 have to be '0'
#define HwUTST_TMD_ON		Hw4								// Test Mode On
#define HwUTST_TMD_OFF		~Hw4							// Test Mode Off
#define	HwUTST_TPS_ON		Hw3								// Transmit test packets
#define	HwUTST_TPS_OFF		~Hw3							// Stop Transmitting
#define	HwUTST_TKS_ON		Hw2								// Transmit test K packets
#define	HwUTST_TKS_OFF		~Hw2							// Stop Transmitting
#define	HwUTST_TJS_ON		Hw1								// Transmit test J packets
#define	HwUTST_TJS_OFF		~Hw1							// Stop Transmitting
#define	HwUTST_TSNS_ON		Hw0								// Transmit test SE0 NAK
#define	HwUTST_TSNS_OFF		~Hw0							// Stop Transmitting

#define	HwSSR			*(volatile unsigned long *)0xF000801C	// System Status Register
#define	HwSSR_BAERR		Hw15							// Byte-Align Error
#define	HwSSR_TMERR		Hw14							// Timeout Error
#define	HwSSR_BSERR		Hw13							// Bit Stuff Error
#define	HwSSR_TCERR		Hw12							// Token CRC Error
#define	HwSSR_DCERR		Hw11							// Data CRC Error
#define	HwSSR_EOERR		Hw10							// EB Overrun Error
#define	HwSSR_VBUS_OFF	Hw9								// VBUS is in Off state.
															// 	(active when VBUS OFF interrupt is enabled)
#define	HwSSR_VBUS_ON		Hw8								// VBUS is in ON state.
															// 	(active when VBUS ON interrupt is enabled)
#define	HwSSR_TBM			Hw7								// Toggle Bit Mismatch Error
#define	HwSSR_DP_HIGH		Hw6								// D+ == High State
#define	HwSSR_DP_LOW		~Hw6							// D+ == Low State
#define	HwSSR_DM_HIGH		Hw5								// D- == High State
#define	HwSSR_DM_LOW		~Hw5							// D- == Low State
#define	HwSSR_HSP_HIGH	Hw4								// Host is High Speed.
#define	HwSSR_HSP_FULL	~Hw4							// Host is Full Speed.
#define	HwSSR_SDE_END		Hw3								// Speed Detection is Ended.
#define	HwSSR_HFRM		Hw2								// Host sends Resume signaling.
#define	HwSSR_HFSUSP		Hw1								// Host sends Suspend signaling.
#define	HwSSR_HFRES		Hw0								// Host sends Reset signaling.

#define	HwSCR			*(volatile unsigned long *)0xF0008020	// System Control Register
#define	HwSCR_DTZIEN_EN		Hw14						// DMA Total Count Zero Interrupt Enabled
#define	HwSCR_DTZIEN_DIS		~Hw14						// DMA Total Count Zero Interrupt Disabled
#define	HwSCR_DIEN_EN			Hw12						// Dual Interrupt Enabled
#define	HwSCR_DIEN_DIS		~Hw12						// Dual Interrupt Disabled
#define	HwSCR_VBUSOFF_EN		Hw11						// HwSSR(VBUS OFF flag) Enabled
#define	HwSCR_VBUSOFF_DIS	~Hw11						// HwSSR(VBUS OFF flag) Disabled
#define	HwSCR_VBUSON_EN		Hw10						// HwSSR(VBUS ON flag) Enabled
#define	HwSCR_VBUSON_DIS		~Hw10						// HwSSR(VBUS ON flag) Disabled
#define	HwSCR_RWDE_EN		Hw9							// High byte data [15:8] is sent first.
#define	HwSCR_RWDE_DIS		~Hw9						// Low byte data [7:0] is sent first.
#define	HwSCR_EIE_EN			Hw8							// Error Interrupt Enable
#define	HwSCR_EIE_DIS			~Hw8						// Error Interrupt Disable
#define	HwSCR_SDE_EN			Hw6							// Speed Detection End Interrupt Enable
#define	HwSCR_SDE_DIS			~Hw6						// Speed Detection End Interrupt Disable
#define	HwSCR_RRDE_EN			Hw5							// First received data is loaded in Low byte [7:0]
#define	HwSCR_RRDE_DIS		~Hw5						// First received data is loaded in High byte [15:8]
#define	HwSCR_IPS_HIGH		Hw4							// Active High Interrupt Polarity
#define	HwSCR_IPS_LOW			~Hw4						// Active Low Interrupt Polarity
#define	HwSCR_MFRM_ON		Hw2							// USB Core generates Resume signaling.
#define	HwSCR_MFRM_OFF		~Hw2						// USB Core stop generating Resume signaling.
#define	HwSCR_HSUSPE_EN		Hw1							// USB Core can repond to the Suspend signaling from HOST.
#define	HwSCR_HSUSPE_DIS		~Hw1						// USB Core cannot repond to the Suspend signaling from HOST.
#define	HwSCR_HRESE_EN		Hw0							// USB Core can repond to the Reset signaling from HOST.
#define	HwSCR_HRESE_DIS		~Hw0						// USB Core cannot repond to the Reset signaling from HOST.

#define	HwEP0SR			*(volatile unsigned long *)0xF0008024	// EP0 Status Register
#define	HwEP0SR_LWO		Hw6								// Last Word Odd
#define	HwEP0SR_SHT		Hw4								// Stall Handshake Transmitted
#define	HwEP0SR_TST			Hw1							// TX Successfully Transmitted
#define	HwEP0SR_RSR		Hw0								// RX Successfully Received

#define	HwEP0CR			*(volatile unsigned long *)0xF0008028	// EP0 Control Register
#define	HwEP0CR_ESS_SET	Hw1								// Send STALL Handshake to Host
#define	HwEP0CR_ESS_CLR	~Hw1							// Stop sending STALL Handshake
#define	HwEP0CR_TZLS_SET	Hw0								// Transmit Zero Length Data (active when TTE is set)
#define	HwEP0CR_TZLS_CLR	~Hw0							// Stop Transmitting Zero Length Data (active when TTE is set)


#define	HwESR			*(volatile unsigned long *)0xF000802C	// Endpoint Status Register
#define	HwESR_FUDR		Hw15							// FIFO Underflow
#define	HwESR_FOVF		Hw14							// FIFO Overflow
#define	HwESR_FPID_ON		Hw11							// First OUT packet cannot generate interrupt in OUT DMA operation.
#define	HwESR_FPID_OFF	~Hw11							// First OUT packet generate interrupt in OUT DMA operation.
#define	HwESR_OSD			Hw10							// OUT packet is first received after DMA registers are set.
#define	HwESR_DTCZ		Hw9								// DMA Total Count Zero
#define	HwESR_SPT			Hw8								// Short Packet Received during OUT DMA operation
#define	HwESR_DOM			Hw7								// Dual Operation Mode (Max packet = FIFO size / 2)
#define	HwESR_FFS			Hw6								// FIFO Flushed (Cleared by FLUSH of HwECR register)
#define	HwESR_FSC			Hw5								// Function Stall Condition is sent to Host.
#define	HwESR_LWO			Hw4								// Last Word Odd
#define	HwESR_PSIF_NP		0								// No Packet in FIFO
#define	HwESR_PSIF_1P		Hw2								// 1 Packet in FIFO
#define	HwESR_PSIF_2P		Hw3								// 2 Packets in FIFO
#define	HwESR_TPS			Hw1								// TX Packet Success
#define	HwESR_RPS			Hw0								// RX Packet Success

#define	HwECR			*(volatile unsigned long *)0xF0008030	// Endpoint Control Register
#define HwECR_SPE_EN	Hw15							// Software Reset Enable
#define HwECR_SPE_DIS	~Hw15							// Software Reset Disable
#define	HwECR_INHLD_EN	Hw12							// In-Packet Hold Enable (USB sends NAK regardless of IN FIFO status)
#define	HwECR_INHLD_DIS	~Hw12							// In-Packet Hold Disable (USB sends IN data)
#define	HwECR_OUTHLD_EN	Hw11							// Out-Packet Hold Enable (USB doesn't accept OUT data)
#define	HwECR_OUTHLD_DIS	~Hw11							// Out-Packet Hold Disable (USB accept OUT data)
#define	HwECR_TNPMF_1T	Hw9								// 1 Transaction Per MicroFrame
#define	HwECR_TNPMF_2T	Hw10							// 2 Transactions Per MicroFrame
#define	HwECR_TNPMF_3T	(Hw10+Hw9)						// 3 Transactions Per MicroFrame
#define	HwECR_DUEN_EN		Hw7								// Dual FIFO Mode Enable
#define	HwECR_DUEN_DIS	~Hw7							// Dual FIFO Mode Disable
#define	HwECR_FLUSH_EN	Hw6								// FIFO is Flushed (automatically cleared after writing)
#define	HwECR_TTE_EN		Hw5								// TX Toggle Forcing Enable
#define	HwECR_TTE_DIS		~Hw5							// TX Toggle Forcing Disable
#define	HwECR_TTS_PID0	0								// TX Data Toggle bit = PID 0
#define	HwECR_TTS_PID1	Hw3								// TX Data Toggle bit = PID 1
#define	HwECR_TTS_PID2	Hw4								// TX Data Toggle bit = PID 2 (ISO only)
#define	HwECR_TTS_PIDM	(Hw4+Hw3)						// TX Data Toggle bit = PID M (ISO only)
#define	HwECR_CDP_CLR		Hw2								// Clear Data PID register
#define	HwECR_ESS_SET		Hw1								// send STALL handshake to Host
#define	HwECR_ESS_CLR		~Hw1							// stop sending STALL handshake
#define	HwECR_TZLS_SET	Hw0								// Transmit Zero Length Data (active when TTE is set)
#define	HwECR_TZLS_CLR	~Hw0							// Stop Transmitting Zero Length Data (active when TTE is set)

#define	HwBRCR			*(volatile unsigned long *)0xF0008034	// Byte Read Counter (Half-word unit)

#define	HwBWCR			*(volatile unsigned long *)0xF0008038	// Byte Write Counter (Byte unit)

#define	HwMPR			*(volatile unsigned long *)0xF000803C	// Max Packet Register (Byte unit)

#define	HwDCR			*(volatile unsigned long *)0xF0008040	// DMA Control Register
#define	HwDCR_ARDRD_ON	Hw5								// Auto RX DMA Run Disable On
#define	HwDCR_ARDRD_OFF	~Hw5							// Auto RX DMA Run Disable Off
#define	HwDCR_FMDE_EN	Hw4								// Fly Mode DMA Enable
#define	HwDCR_FMDE_DIS	~Hw4							// Fly Mode DMA Disable
#define	HwDCR_DMDE_EN	Hw3								// Demand Mode DMA Enable
#define	HwDCR_DMDE_DIS	~Hw3							// Demand Mode DMA Disable
#define	HwDCR_TDR_RUN		Hw2								// TX DMA Run
#define	HwDCR_TDR_STOP	~Hw2							// TX DMA Stop
#define	HwDCR_RDR_RUN		Hw1								// RX DMA Run
#define	HwDCR_RDR_STOP	~Hw1							// RX DMA Stop
#define	HwDCR_DEN_EN		Hw0								// DMA Enable
#define	HwDCR_DEN_DIS		~Hw0							// DMA Disable

#define	HwDTCR			*(volatile unsigned long *)0xF0008044	// DMA Transfer Counter Register (Byte unit)

#define	HwDFCR			*(volatile unsigned long *)0xF0008048	// DMA FIFO Counter Register (Byte unit)

#define	HwDTTCR1		*(volatile unsigned long *)0xF000804C	// Lower-Half of DMA Total Transfer Counter Register

#define	HwDTTCR2		*(volatile unsigned long *)0xF0008050	// Upper-Half of DMA Total Transfer Counter Register

#define	HwEP0BUF		*(volatile unsigned long *)0xF0008060	// EP0 Buffer Register

#define	HwEP1BUF		*(volatile unsigned long *)0xF0008064	// EP1 Buffer Register

#define	HwEP2BUF		*(volatile unsigned long *)0xF0008068	// EP2 Buffer Register

#define	HwEP3BUF		*(volatile unsigned long *)0xF000806C	// EP3 Buffer Register

#define HwPLICR			*(volatile unsigned long *)0xF00080A0	// PHYLINK Interface Control Register

#define HwPCR			*(volatile unsigned long *)0xF00080A4	// PHY Control Register
#define HwPCR_URST_EN		~Hw7							// uTMI_RESET enable
#define HwPCR_URST_DIS		Hw7								// UTMI_RESET disable
#define HwPCR_SIDC_1		Hw6								// SIDDQ Control ; SIDDQ = 1
#define HwPCR_SIDC_0		~Hw6							// SIDDQ Control ; SIDDQ = 0
#define HwPCR_OPMC_RESET	((~Hw5)&(~Hw4))					// OPMODE Control ; reset, reserved
#define HwPCR_OPMC_DIS		Hw4								// OPMODE Control ; Disable bit stuffing and NRZI encoding
#define HwPCR_OPMC_ND		Hw5								// OPMODE Control ; Non-Driving
#define HwPCR_OPMC_NORMAL	(Hw5+Hw4)						// OPMODE Control ; Normal
#define HwPCR_TMSC_1		Hw3								// TERMSEL Control ; TERMSEL = 1
#define HwPCR_TMSC_0		~Hw3							// TERMSEL Control ; TERMSEL = 0
#define HwPCR_XCRC_1		Hw2								// XCVRSEL = 1
#define HwPCR_XCRC_0		~Hw2							// XCVRSEL = 0
#define HwPCR_SUSPC_1		Hw1								// SUSPENDM = 1
#define HwPCR_SUSPC_0		~Hw1							// SUSPENDM = 0
#define HwPCR_PCE_EN		Hw0								// Control Enable
#define HwPCR_PCE_DIS		~Hw0							// Control Disable

#define HwUPCFG0		*(volatile unsigned long *)0xF00080C8	// USB PHY Configuration Register 0
#define HwUPCFG0_PR_EN		Hw14							// Rer-Port Reset Enable
#define HwUPCFG0_PR_DIS		~Hw14							// Rer-Port Reset Disable
#define HwUPCFG0_CM_EN		Hw13							// Common Block Power Down Enable
#define HwUPCFG0_CM_DIS		~Hw13							// Common Block Power Down Disable
#define HwUPCFG0_RCS_11		(Hw12+Hw11)						// Reference Clock Select for PLL Block ; The PLL uses CLKCORE as reference
#define HwUPCFG0_RCS_10		Hw12							// Reference Clock Select for PLL Block ; The PLL uses CLKCORE as reference
#define HwUPCFG0_RCS_01		Hw11							// Reference Clock Select for PLL Block ; The XO block uses an external clock supplied on the XO pin
#define HwUPCFG0_RCS_00		((~Hw12)&(~Hw11))				// Reference Clock Select for PLL Block ; The XO block uses the clock from a crystal
#define HwUPCFG0_RCD_48		Hw10							// Reference Clock Frequency Select ; 48MHz
#define HwUPCFG0_RCD_24		Hw9								// Reference Clock Frequency Select ; 24MHz
#define HwUPCFG0_RCD_12		((~Hw10)&(~Hw9))				// Reference Clock Frequency Select ; 12MHz
#define HwUPCFG0_SDI_EN		Hw8								// IDDQ Test Enable ; The analog blocks are powered down
#define HwUPCFG0_SDI_DIS	~Hw8							// IDDQ Test Disable ; The analog blocks are not powered down
#define HwUPCFG0_FO_SI		Hw7								// UTMI/Serial Interface Select ; Serial Interface
#define HwUPCFG0_FO_UTMI	~Hw7							// UTMI/Serial Interface Select ; UTMI

#define HwUPCFG1		*(volatile unsigned long *)0xF00080CC	// USB PHY Configuration Register 1

#define HwUPCFG2		*(volatile unsigned long *)0xF00080D0	// USB PHY Configuration Register 2

#define HwUPCFG3		*(volatile unsigned long *)0xF00080D4	// USB PHY Configuration Register 3

//#define	HwDLYCTRL		*(volatile unsigned long *)0xF000D080	// Delay Control Register

//#define HwDTSR			*(volatile unsigned long *)0xF000D0C0	// DMA Transfer Status Register

//#define	HwUBCFG			*(volatile unsigned long *)0xF00080C4	// USB Configuration Register

//Done...
/////////
/************************************************************************
*	LCD INTERFACE Register Define				(Base Addr = 0xF100D000)
************************************************************************/
/*TCC82x
#define	HwLCTRL						*(volatile unsigned long *)0xF100D000	// W, LCD Control Register
#define	HwLCTRL_Y2R2_EN				Hw31									// YUV to RGB Channel Converter Enable 2
#define	HwLCTRL_AEN2_EN				Hw30									// Alpha Blend Enable 2
#define	HwLCTRL_CEN2_EN				Hw29									// Chroma Key Enable 2
#define	HwLCTRL_Y2R1_EN				Hw28									// YUV to RGB Channel Converter Enable 1
#define	HwLCTRL_AEN1_EN				Hw27									// Alpha Blend Enable 1
#define	HwLCTRL_CEN1_EN				Hw26									// Chroma Key Enable 1
#define	HwLCTRL_Y2R0_EN				Hw25									// YUV to RGB Channel Converter Enable 0
#define	HwLCTRL_656_EN				Hw24									// CCIR 601 to 656 Enable
#define	HwLCTRL_BPP_1				HwZERO									// Bit Per Pixel = 1bbp
#define	HwLCTRL_BPP_2				Hw20									// Bit Per Pixel = 2bbp 
#define	HwLCTRL_BPP_4				Hw21									// Bit Per Pixel = 4bbp
#define	HwLCTRL_BPP_332				(Hw21+Hw20)								// Bit Per Pixel = 332bbp
#define	HwLCTRL_BPP_444				Hw22									// Bit Per Pixel = 444bbp
#define	HwLCTRL_BPP_565				(Hw22+Hw20)								// Bit Per Pixel = 565bbp
#define	HwLCTRL_BPP_555				(Hw22+Hw21)								// Bit Per Pixel = 555bbp
#define	HwLCTRL_BPP_888				(Hw22+Hw21+Hw20)						// Bit Per Pixel = 888bbp
#define	HwLCTRL_PXDW_4				HwZERO									// Pixel Data Width = 4pxdw
#define	HwLCTRL_PXDW_8				Hw16									// Pixel Data Width = 8pxdw
#define	HwLCTRL_PXDW_888			Hw17									// Pixel Data Width = 888pxdw
#define	HwLCTRL_PXDW_565			(Hw17+Hw16)								// Pixel Data Width = 565pxdw
#define	HwLCTRL_PXDW_555			Hw18									// Pixel Data Width = 555pxdw
#define	HwLCTRL_PXDW_18				(Hw18+Hw16)								// Pixel Data Width = 18pxdw
#define	HwLCTRL_PXDW_8UY			(Hw18+Hw17)								// Pixel Data Width = 8pxdw(UY)
#define	HwLCTRL_PXDW_8VY			(Hw18+Hw17+Hw16)						// Pixel Data Width = 8pxdw(VY)
#define	HwLCTRL_PXDW_16YU			Hw19									// Pixel Data Width = 16pxdw(YU)
#define	HwLCTRL_PXDW_16YV			(Hw19+Hw16)								// Pixel Data Width = 16pxdw(YV)
#define	HwLCTRL_ID_INVERTED			Hw15									// Inverted ACBIAS
#define HwLCTRL_IV_INVERTED			Hw14									// Inverted Vertical Sync
#define	HwLCTRL_IH_INVERTED			Hw13									// Inverted Horizontal Sync
#define	HwLCTRL_IP_FEDGE			Hw12									// Data is driven onto the LCD's data pins on the falling edge of pixel clock pin
#define HwLCTRL_CLEN_EN				Hw11									// Clippin Enable
#define HwLCTRL_R2Y_EN				Hw10									// RGB to YUV Channel Converter Enable
#define HwLCTRL_DP_2				Hw9										// One pixel data per 2 PXCLK cycle is output
#define	HwLCTRL_NI_PMODE			Hw8										// Non-interlace Mode (Progressive Mode)
#define	HwLCTRL_TV					Hw7										// TV Mode. In this mode, all values of LVTIMEn registers are divided by 2
#define	HwLCTRL_TFT					Hw6										// TFT-LCD Mode
#define	HwLCTRL_STN					Hw5										// STN-LCD Mode
#define	HwLCTRL_MSEL				Hw4										// Master Select
#define	HwLCTRL_IEN2_EN				Hw3										// Fetch Enable 2
#define	HwLCTRL_IEN1_EN				Hw2										// Fetch Enable 1
#define HwLCTRL_IEN0_EN				Hw1										// Fetch Enable 0
#define HwLCTRL_LEN_EN				Hw0										// LCD Controller Enable

#define	HwLBC						*(volatile unsigned long *)0xF100D004	// W, LCD Background Color Register

#define	HwLCLKDIV					*(volatile unsigned long *)0xF100D008	// W, LCD Clock Divider Register
#define HwLCLKDIV_CS				Hw31									// Clock Source      

#define	HwLHTIME1					*(volatile unsigned long *)0xF100D00C	// W, LCD Horizontal Timing Register 1

#define	HwLHTIME2					*(volatile unsigned long *)0xF100D010	// W, LCD Horizontal Timing Register 2

#define	HwLVTIME1					*(volatile unsigned long *)0xF100D014	// W, LCD Vertical Timing Register 1
#define	HwLVTIME1_VDB				Hw27				 					// Back porchVSYNC delay    
#define	HwLVTIME1_VDF				Hw22				 					// Front porch of VSYNC delay    
#define	HwLVTIME1_FPW 				Hw16				 					// Frame pulse width     
#define	HwLVTIME1_FLC 				Hw0				 						// Frame line count     

#define	HwLVTIME2					*(volatile unsigned long *)0xF100D018	// W, LCD Vertical Timing Register 2

#define	HwLVTIME3					*(volatile unsigned long *)0xF100D01C	// W, LCD Vertical Timing Register 3

#define	HwLVTIME4					*(volatile unsigned long *)0xF100D020	// W, LCD Vertical Timing Register 4

#define	HwLLUTR						*(volatile unsigned long *)0xF100D024	// W, LCD Lookup Register for Red

#define	HwLLUTG						*(volatile unsigned long *)0xF100D028	// W, LCD Lookup Register for Green

#define	HwLLUTB						*(volatile unsigned long *)0xF100D02C	// W, LCD Lookup Register for Blue

#define	HwLDP7L						*(volatile unsigned long *)0xF100D030	// W, LCD Modulo 7 Dithering Pattern (Low)

#define	HwLDP7H						*(volatile unsigned long *)0xF100D034	// W, LCD Modulo 7 Dithering Pattern (High)

#define	HwLDP5						*(volatile unsigned long *)0xF100D038	// W, LCD Modulo 5 Dithering Pattern Register

#define	HwLDP4						*(volatile unsigned long *)0xF100D03C	// W, LCD Modulo 4 Dithering Pattern Register

#define	HwLDP3						*(volatile unsigned long *)0xF100D040	// W, LCD 3-bit Dithering Pattern Register

#define	HwLCP1						*(volatile unsigned long *)0xF100D044	// W, LCD Clipping Register

#define	HwLCP2						*(volatile unsigned long *)0xF100D048	// W, LCD Clipping Register

#define	HwLK1						*(volatile unsigned long *)0xF100D04C	// W, LCD Keying Register 1
#define	HwLK1_A10_MAX				(Hw25+Hw24)

#define	HwLK2						*(volatile unsigned long *)0xF100D050	// W, LCD Keying Register 2
#define	HwLK2_A20_MAX				(Hw25+Hw24)

#define	HwLKM1						*(volatile unsigned long *)0xF100D054	// W, LCD Keying Mask Register 1
#define	HwLKM1_A11_MAX			(Hw25+Hw24)

#define	HwLKM2						*(volatile unsigned long *)0xF100D058	// W, LCD Keying Mask Register 2
#define	HwLKM2_A21_MAX			(Hw25+Hw24)

#define	HwLDS						*(volatile unsigned long *)0xF100D05C	// W, LCD Display Size Register

#define	HwLSTATUS					*(volatile unsigned long *)0xF100D060	// R/Clr, LCD Status Register
#define HwLSTATUS_ITY				Hw12									// Interrupt Type
#define HwLSTATUS_ICR				Hw8										// Interrupt Clear 
#define HwLSTATUS_BY				Hw6										// Busy signal

#define HwLSTATUS_EF				Hw5										// Even-Field(Read Only). 0:Odd field or frame, 1:Even field or frame
#define HwLSTATUS_DD				Hw4										// Disable Done(Read/Clear). If LEN is disabled, DD will be 1 after current frame has been displayed. As MDD of LIM register is cleared, it can be LCD interrupt source
#define HwLSTATUS_RU				Hw3										// Register Update(Read/Clear). It indicates that all registers programmed are applied to current frame data. As MRU of LIM register is cleared, it can be LCD interrupt source
#define HwLSTATUS_FU				Hw0										// FIFO underrun(Read/Clear). It indicates that FIFO underrun has been occurred. In this case, LCLK frequency must be lower. As MFU of LIM register is cleared, it can be LCD interrupt source

#define	HwLIM						*(volatile unsigned long *)0xF100D064	// W, LCD Interrupt Register
#define HwLIM_MDD					Hw4										// Masking disable done interrupt
#define HwLIM_MRU					Hw3										// Masking register update interrupt
#define HwLIM_MFU					Hw0										// Masking FIFO underrun interrupt

#define	HwLI0C						*(volatile unsigned long *)0xF100D068	// W, LCD Image 0 Control Register
#define HwLI0C_OP_0					HwZERO									// Image 0 - image 1 - image 2  
#define HwLI0C_OP_1					Hw12									// Image 0 - image 2 - image 1  
#define HwLI0C_OP_2					Hw13									// Image 1 - image 0 - image 2  
#define HwLI0C_OP_3					(Hw13+Hw12)								// Image 2 - image 0 - image 1  
#define HwLI0C_OP_4					Hw14									// Image 1 - image 2 - image 0  
#define HwLI0C_OP_5					(Hw14+Hw12)								// Image 2 - image 1 - image 0  
#define HwLI0C_BR_BE				Hw7										// Big Endian pixel data
#define HwLI0C_YUV_420				Hw4										// YUV 4:2:0
#define HwLI0C_YUV_422				(Hw5+Hw4)								// YUV 4:2:2
#define HwLI0C_YUV_422P				(Hw6+Hw5+Hw4)							// YUV 4:2:2 Patch
#define HwLI0C_BPP_1				HwZERO									// Bit Per Pixel = 1bpp
#define HwLI0C_BPP_2				Hw0										// Bit Per Pixel = 2bpp
#define HwLI0C_BPP_4				Hw1										// Bit Per Pixel = 4bpp
#define HwLI0C_BPP_332				(Hw1+Hw0)								// Bit Per Pixel = 332bpp
#define HwLI0C_BPP_444				Hw2										// Bit Per Pixel = 444bpp
#define HwLI0C_BPP_565				(Hw2+Hw0)								// Bit Per Pixel = 565bpp
#define HwLI0C_BPP_555				(Hw2+Hw1)								// Bit Per Pixel = 555bpp
#define HwLI0C_BPP_888				(Hw2+Hw1+Hw0)							// Bit Per Pixel = 888bpp

#define HwLI0P						*(volatile unsigned long *)0xF100D06C	// W, LCD Image 0 Position Register
	
#define	HwLI0S						*(volatile unsigned long *)0xF100D070	// W, LCD Image 0 Size Register

#define	HwLI0BA0					*(volatile unsigned long *)0xF100D074	// W, LCD Image 0 Base Address 0 Register

#define	HwLI0CA						*(volatile unsigned long *)0xF100D078	// W, LCD Image 0 Current Address Register

#define	HwLI0BA1					*(volatile unsigned long *)0xF100D07C	// W, LCD Image 0 Base Address 1 Register

#define HwLI0BA2					*(volatile unsigned long *)0xF100D080	// W, LCD Image 0 Base Address 2 Register

#define	HwLI0O						*(volatile unsigned long *)0xF100D084	// W, LCD Image 0 Offset Register

#define HwLI0SCALE					*(volatile unsigned long *)0xF100D088	// W, LCD Image 0 Scale Ratio
#define	HwLI0SCALE_Y_NS				HwZERO									// Non-Scalable
#define	HwLI0SCALE_Y_D2				Hw4										// DownScale by 2
#define	HwLI0SCALE_Y_D3				Hw5										// DownScale by 3
#define HwLI0SCALE_Y_D4				(Hw5+Hw4)								// DownScale by 4
#define	HwLI0SCALE_Y_D8				(Hw6+Hw5+Hw4)							// DownScale by 8
#define	HwLI0SCALE_Y_U2				(Hw7+Hw4)								// UpScale by 2
#define	HwLI0SCALE_Y_U3				(Hw7+Hw5)								// UpScale by 3
#define	HwLI0SCALE_Y_U4				(Hw7+Hw5+Hw4)							// UpScale by 4
#define	HwLI0SCALE_Y_U8				(Hw7+Hw6+Hw5+Hw4)						// UpScale by 8
#define	HwLI0SCALE_X_NS				HwZERO									// Non-Scalable
#define	HwLI0SCALE_X_D2				Hw0										// DownScale by 2
#define	HwLI0SCALE_X_D3				Hw1										// DownScale by 3
#define HwLI0SCALE_X_D4				(Hw1+Hw0)								// DownScale by 4
#define	HwLI0SCALE_X_D8				(Hw2+Hw1+Hw0)							// DownScale by 8
#define	HwLI0SCALE_X_U2				(Hw3+Hw0)								// UpScale by 2
#define	HwLI0SCALE_X_U3				(Hw3+Hw1)								// UpScale by 3
#define	HwLI0SCALE_X_U4				(Hw3+Hw1+Hw0)							// UpScale by 4
#define	HwLI0SCALE_X_U8				(Hw3+Hw2+Hw1+Hw0)						// UpScale by 8

#define	HwLI1C						*(volatile unsigned long *)0xF100D08C	// W, LCD Image 1 Control Register
#define HwLI1C_BR_BE				Hw7										// Big Endian pixel data
#define HwLI1C_YUV_420				Hw4										// YUV 4:2:0
#define HwLI1C_YUV_422				(Hw5+Hw4)								// YUV 4:2:2
#define HwLI1C_YUV_422P				(Hw6+Hw5+Hw4)							// YUV 4:2:2 Patch
#define HwLI1C_BPP_1				HwZERO									// Bit Per Pixel = 1bpp
#define HwLI1C_BPP_2				Hw0										// Bit Per Pixel = 2bpp
#define HwLI1C_BPP_4				Hw1										// Bit Per Pixel = 4bpp
#define HwLI1C_BPP_332				(Hw1+Hw0)								// Bit Per Pixel = 332bpp
#define HwLI1C_BPP_444				Hw2										// Bit Per Pixel = 444bpp
#define HwLI1C_BPP_565				(Hw2+Hw0)								// Bit Per Pixel = 565bpp
#define HwLI1C_BPP_555				(Hw2+Hw1)								// Bit Per Pixel = 555bpp
#define HwLI1C_BPP_888				(Hw2+Hw1+Hw0)							// Bit Per Pixel = 888bpp

#define HwLI1P						*(volatile unsigned long *)0xF100D090	// W, LCD Image 1 Position Register

#define	HwLI1S						*(volatile unsigned long *)0xF100D094	// W, LCD Image 1 Size Register

#define	HwLI1BA0					*(volatile unsigned long *)0xF100D098	// W, LCD Image 1 Base Address 0 Register

#define	HwLI1CA						*(volatile unsigned long *)0xF100D09C	// W, LCD Image 1 Current Address Register

#define	HwLI1BA1					*(volatile unsigned long *)0xF100D0A0	// W, LCD Image 1 Base Address 1 Register

#define HwLI1BA2					*(volatile unsigned long *)0xF100D0A4	// W, LCD Image 1 Base Address 2 Register

#define	HwLI1O						*(volatile unsigned long *)0xF100D0A8	// W, LCD Image 1 Offset Register

#define HwLI1SCALE					*(volatile unsigned long *)0xF100D0AC	// W, LCD Image 1 Scale Ratio
#define	HwLI1SCALE_Y_NS				HwZERO									// Non-Scalable
#define	HwLI1SCALE_Y_D2				Hw4										// DownScale by 2
#define	HwLI1SCALE_Y_D3				Hw5										// DownScale by 3
#define HwLI1SCALE_Y_D4				(Hw5+Hw4)								// DownScale by 4
#define	HwLI1SCALE_Y_D8				(Hw6+Hw5+Hw4)							// DownScale by 8
#define	HwLI1SCALE_Y_U2				(Hw7+Hw4)								// UpScale by 2
#define	HwLI1SCALE_Y_U3				(Hw7+Hw5)								// UpScale by 3
#define	HwLI1SCALE_Y_U4				(Hw7+Hw5+Hw4)							// UpScale by 4
#define	HwLI1SCALE_Y_U8				(Hw7+Hw6+Hw5+Hw4)						// UpScale by 8
#define	HwLI1SCALE_X_NS				HwZERO									// Non-Scalable
#define	HwLI1SCALE_X_D2				Hw0										// DownScale by 2
#define	HwLI1SCALE_X_D3				Hw1										// DownScale by 3
#define HwLI1SCALE_X_D4				(Hw1+Hw0)								// DownScale by 4
#define	HwLI1SCALE_X_D8				(Hw2+Hw1+Hw0)							// DownScale by 8
#define	HwLI1SCALE_X_U2				(Hw3+Hw0)								// UpScale by 2
#define	HwLI1SCALE_X_U3				(Hw3+Hw1)								// UpScale by 3
#define	HwLI1SCALE_X_U4				(Hw3+Hw1+Hw0)							// UpScale by 4
#define	HwLI1SCALE_X_U8				(Hw3+Hw2+Hw1+Hw0)						// UpScale by 8

#define	HwLI2C						*(volatile unsigned long *)0xF100D0B0	// W, LCD Image 2 Control Register
#define	HwLI2C_LUT_EN				Hw9										// Use the Color Lookup Table
#define HwLI2C_BR_BE				Hw7										// Big Endian pixel data
#define HwLI2C_YUV_420				Hw4										// YUV 4:2:0
#define HwLI2C_YUV_422				(Hw5+Hw4)								// YUV 4:2:2
#define HwLI2C_YUV_422P				(Hw6+Hw5+Hw4)							// YUV 4:2:2 Patch
#define HwLI2C_BPP_1				HwZERO									// Bit Per Pixel = 1bpp
#define HwLI2C_BPP_2				Hw0										// Bit Per Pixel = 2bpp
#define HwLI2C_BPP_4				Hw1										// Bit Per Pixel = 4bpp
#define HwLI2C_BPP_332				(Hw1+Hw0)								// Bit Per Pixel = 332bpp
#define HwLI2C_BPP_444				Hw2										// Bit Per Pixel = 444bpp
#define HwLI2C_BPP_565				(Hw2+Hw0)								// Bit Per Pixel = 565bpp
#define HwLI2C_BPP_555				(Hw2+Hw1)								// Bit Per Pixel = 555bpp
#define HwLI2C_BPP_888				(Hw2+Hw1+Hw0)							// Bit Per Pixel = 888bpp

#define HwLI2P						*(volatile unsigned long *)0xF100D0B4	// W, LCD Image 2 Position Register

#define	HwLI2S						*(volatile unsigned long *)0xF100D0B8	// W, LCD Image 2 Size Register

#define	HwLI2BA0					*(volatile unsigned long *)0xF100D0BC	// W, LCD Image 2 Base Address 0 Register

#define	HwLI2CA						*(volatile unsigned long *)0xF100D0C0	// W, LCD Image 2 Current Address Register

#define	HwLI2BA1					*(volatile unsigned long *)0xF100D0C4	// W, LCD Image 2 Base Address 1 Register

#define HwLI2BA2					*(volatile unsigned long *)0xF100D0C8	// W, LCD Image 2 Base Address 2 Register

#define	HwLI2O						*(volatile unsigned long *)0xF100D0CC	// W, LCD Image 2 Offset Register

#define HwLI2SCALE					*(volatile unsigned long *)0xF100D0D0	// W, LCD Image 2 Scale Ratio
#define	HwLI2SCALE_Y_NS				HwZERO									// Non-Scalable
#define	HwLI2SCALE_Y_D2				Hw4										// DownScale by 2
#define	HwLI2SCALE_Y_D3				Hw5										// DownScale by 3
#define HwLI2SCALE_Y_D4				(Hw5+Hw4)								// DownScale by 4
#define	HwLI2SCALE_Y_D8				(Hw6+Hw5+Hw4)							// DownScale by 8
#define	HwLI2SCALE_Y_U2				(Hw7+Hw4)								// UpScale by 2
#define	HwLI2SCALE_Y_U3				(Hw7+Hw5)								// UpScale by 3
#define	HwLI2SCALE_Y_U4				(Hw7+Hw5+Hw4)							// UpScale by 4
#define	HwLI2SCALE_Y_U8				(Hw7+Hw6+Hw5+Hw4)						// UpScale by 8
#define	HwLI2SCALE_X_NS				HwZERO									// Non-Scalable
#define	HwLI2SCALE_X_D2				Hw0										// DownScale by 2
#define	HwLI2SCALE_X_D3				Hw1										// DownScale by 3
#define HwLI2SCALE_X_D4				(Hw1+Hw0)								// DownScale by 4
#define	HwLI2SCALE_X_D8				(Hw2+Hw1+Hw0)							// DownScale by 8
#define	HwLI2SCALE_X_U2				(Hw3+Hw0)								// UpScale by 2
#define	HwLI2SCALE_X_U3				(Hw3+Hw1)								// UpScale by 3
#define	HwLI2SCALE_X_U4				(Hw3+Hw1+Hw0)							// UpScale by 4
#define	HwLI2SCALE_X_U8				(Hw3+Hw2+Hw1+Hw0)						// UpScale by 8

#define	HwLCDLUT					*(volatile unsigned long *)0xF100DC00	// W, LCD Lookup Table

#define HwDLCTRL					*(volatile unsigned long *)0xF100D0D4	// Dual LCD Control  
#define	HwDLCTRL_DRE				Hw4										// Dual LCD Register Update Enable  
#define	HwDLCTRL_DLE				Hw0										// Dual LCD Operation Enable               

#define HwDLCSA0					*(volatile unsigned long *)0xF100D0DC	// Dual LCD Configuration Start Address 0                 

#define HwDLCSA1					*(volatile unsigned long *)0xF100D0E0	// Dual LCD Configuration Start Address 1              

#define HwY2RP0						*(volatile unsigned long *)0xF100D0E4	// YCbCr to RGB Conversion Parameter 0 

#define HwY2RP1						*(volatile unsigned long *)0xF100D0E8	// YCbCr to RGB Conversion Parameter 1            
#define	HwY2RP1_CR2					Hw24									// R = 1.164(Y-16)+1.596(Cr-128) = Y + Cr-233 + 0.164*Y + 0.596*Cr
#define	HwY2RP1_CR1					Hw16									//  
#define	HwY2RP1_CB2					Hw8										//  
#define	HwY2RP1_CB1					Hw0										//  
  
//	LCD System Interface Register Define		(Base Addr = 0xF100D000)
#define	HwLCDSICTRL0				*(volatile unsigned long *)0xF100D400	// R/W, Control Register for LCDSI
#define HwLCDSICTRL0_IA_LOW			Hw15									// LACBIAS(Data Enable) signal is active low
#define HwLCDSICTRL0_IVS_LOW		Hw14									// LYSYNC signal is active low
#define HwLCDSICTRL0_CS_1			Hw7										// If IM is high, CS1 is active during operationgs. Otherwise, it is no applicable. These bits are only available when IM is high
#define HwLCDSICTRL0_RSP_HIGH		Hw6										// If IM is high, RS is high. Otherwise, it is not applicable
#define HwLCDSICTRL0_FMT_8RGB565	HwZERO									// LCDC pixel data output : D1[15:0] (RGB565)         
                                                                            // LCDSI pixel data output(8bits) : D1[7:0], D1[15:8]
                                                                            // LCDSI CTRL1-4.WBW must be 1.                      
#define HwLCDSICTRL0_FMT_16RGB565	(Hw3+Hw4)								// LCDC pixel data output : D1[15:0] (RGB565)   
                                                                            // LCDSI pixel data output (16bits) : D1[15:0]
                                                                            // LCDSI CTRL1-4.WBW must be 0.               
#define HwLCDSICTRL0_FMT_8RGB888	Hw2										// LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)  
                                                                            // LCDSI pixel data output (8bits): D1[7:0], D2[7:0], D3[7:0] 
                                                                            // LCDSI CTRL1-4.WBW must be 0.                               
#define HwLCDSICTRL0_FMT_9RGB888	(Hw2+Hw3)								// LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)            
                                                                            // LCDSI pixel data output (9bits) : {D1[7:2],D2[7:5]}, {D2[4:2], D3[7:2]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                                           
#define HwLCDSICTRL0_FMT_16RGB888_1	(Hw2+Hw4)								//LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)            
                                                                            //LCDSI pixel data output (16bits): D1[7:6], {D1[5:2], D2[7:2], D3[7:2]}
                                                                            //LCDSI CTRL1-4.WBW must be 0.                                          
#define HwLCDSICTRL0_FMT_16RGB888_2	(Hw2+Hw3+Hw4)							// LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)  
                                                                            // LCDSI pixel data output (16bits): {D1[7:3], D2[7:2], D3[7:3]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                                 
#define HwLCDSICTRL0_FMT_16RGB666_1	Hw5										// LCDC pixel data output : D1[17:0] (RGB666)               
                                                                            // LCDSI pixel data output (16bits): {D1[17:9]}, {D2[8:0]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                           
#define HwLCDSICTRL0_FMT_16RGB666_2	(Hw5+Hw3)								// LCDC pixel data output : D1[17:0] (RGB666)               
                                                                            // LCDSI pixel data output (16bits): {D1[17:2]}, {D2[1:0]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                           
#define HwLCDSICTRL0_FMT_16RGB666_3	(Hw5+Hw4)								// LCDC pixel data output : D1[17:0] (RGB666)     
                                                                            // LCDSI pixel data output (16bits): {D1[17:0]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                



#define HwLCDSICTRL0_OM			Hw1																		
#define HwLCDSICTRL0_IM			Hw0																			

#define	HwLCDSICTRL1				*(volatile unsigned long *)0xF100D800	// R/W, Control Register for nCS0 when RS = 0
#define	HwLCDSICTRL1_BW1		Hw31										// LCDXD data width is 8bits. Prefix W means writing operation
#define HwLCDSICTRL1_BW0		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation

#define	HwLCDSICTRL2				*(volatile unsigned long *)0xF100D804	// R/W, Control Register for nCS0 when RS = 1
#define	HwLCDSICTRL2_BW1		Hw31										// LCDXD data width is 8bits. Prefix W means writing operation
#define HwLCDSICTRL2_BW0		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation

#define	HwLCDSICTRL3				*(volatile unsigned long *)0xF100D808	// R/W, Control Register for nCS1 when RS = 0
#define	HwLCDSICTRL3_BW1		Hw31										// LCDXD data width is 8bits. Prefix W means writing operation
#define HwLCDSICTRL3_BW0			Hw15									// LCDXD data width is 8bits. Prefix R means reading operation

#define	HwLCDSICTRL4				*(volatile unsigned long *)0xF100D80C	// R/W, Control Register for nCS0 when RS = 1
#define	HwLCDSICTRL4_BW1		Hw31										// LCDXD data width is 8bits. Prefix W means writing operation
#define HwLCDSICTRL4_BW0		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation

#define	HwLCDSICS0RS0				*(volatile unsigned long *)0xF100D810	// R/W, If this register is read or written, reading or writing operations are generated on nCS0 while RS = 0            
                                                                                                                                                                                             
#define	HwLCDSICS0RS1				*(volatile unsigned long *)0xF100D818	// R/W, If this register is read or written, reading or writing operations are generated on nCS0 while RS = 1            
                                                                                                                                                                                             
#define	HwLCDSICS1RS0				*(volatile unsigned long *)0xF100D820	// R/W, If this register is read or written, reading or writing operations are generated on nCS1 while RS = 0            
                                                                                                                                                                                             
#define	HwLCDSICS1RS1				*(volatile unsigned long *)0xF100D828	// R/W, If this register is read or written, reading or writing operations are generated on nCS1 while RS = 1            

//for lcd access path using LCD clock
#define	HwLCDSICTRL5				*(volatile unsigned long *)0xF100D830			// Control register for nCS0 when RS=0
#define	HwLCDSICTRL5_WBW						Hw31									//

#define	HwLCDSICTRL6				*(volatile unsigned long *)0xF100D834			// Control register for nCS0 when RS=1
#define	HwLCDSICTRL6_WBW						Hw31									//
#define	HwLCDSICTRL7				*(volatile unsigned long *)0xF100D838			// Control register for nCS1 when RS=0 
#define	HwLCDSICTRL7_WBW						Hw31									//
#define	HwLCDSICTRL8				*(volatile unsigned long *)0xF100D83C			// Control register for nCS1 when RS=1 
#define	HwLCDSICTRL8_WBW						Hw31									//
*/

#define	HwLCTRL						*(volatile unsigned long *)0xF0003000	// W, LCD Control Register
#define	HwLCTRL_Y2R2_EN				Hw31									// YUV to RGB Channel Converter Enable 2
#define	HwLCTRL_AEN2_EN				Hw30									// Alpha Blend Enable 2
#define	HwLCTRL_CEN2_EN				Hw29									// Chroma Key Enable 2
#define	HwLCTRL_Y2R1_EN				Hw28									// YUV to RGB Channel Converter Enable 1
#define	HwLCTRL_AEN1_EN				Hw27									// Alpha Blend Enable 1
#define	HwLCTRL_CEN1_EN				Hw26									// Chroma Key Enable 1
#define	HwLCTRL_Y2R0_EN				Hw25									// YUV to RGB Channel Converter Enable 0
#define	HwLCTRL_656_EN				Hw24									// CCIR 601 to 656 Enable
#define	HwLCTRL_BPP_1				HwZERO									// Bit Per Pixel = 1bbp
#define	HwLCTRL_BPP_2				Hw20									// Bit Per Pixel = 2bbp 
#define	HwLCTRL_BPP_4				Hw21									// Bit Per Pixel = 4bbp
#define	HwLCTRL_BPP_332				(Hw21+Hw20)								// Bit Per Pixel = 332bbp
#define	HwLCTRL_BPP_444				Hw22									// Bit Per Pixel = 444bbp
#define	HwLCTRL_BPP_565				(Hw22+Hw20)								// Bit Per Pixel = 565bbp
#define	HwLCTRL_BPP_555				(Hw22+Hw21)								// Bit Per Pixel = 555bbp
#define	HwLCTRL_BPP_888				(Hw22+Hw21+Hw20)						// Bit Per Pixel = 888bbp
#define	HwLCTRL_PXDW_4				HwZERO									// Pixel Data Width = 4pxdw
#define	HwLCTRL_PXDW_8				Hw16									// Pixel Data Width = 8pxdw
#define	HwLCTRL_PXDW_888			Hw17									// Pixel Data Width = 888pxdw
#define	HwLCTRL_PXDW_565			(Hw17+Hw16)								// Pixel Data Width = 565pxdw
#define	HwLCTRL_PXDW_555			Hw18									// Pixel Data Width = 555pxdw
#define	HwLCTRL_PXDW_18				(Hw18+Hw16)								// Pixel Data Width = 18pxdw
#define	HwLCTRL_PXDW_8UY			(Hw18+Hw17)								// Pixel Data Width = 8pxdw(UY)
#define	HwLCTRL_PXDW_8VY			(Hw18+Hw17+Hw16)						// Pixel Data Width = 8pxdw(VY)
#define	HwLCTRL_PXDW_16YU			Hw19									// Pixel Data Width = 16pxdw(YU)
#define	HwLCTRL_PXDW_16YV			(Hw19+Hw16)								// Pixel Data Width = 16pxdw(YV)
#define	HwLCTRL_ID_INVERTED			Hw15									// Inverted ACBIAS
#define 	HwLCTRL_IV_INVERTED			Hw14									// Inverted Vertical Sync
#define	HwLCTRL_IH_INVERTED			Hw13									// Inverted Horizontal Sync
#define	HwLCTRL_IP_FEDGE			Hw12									// Data is driven onto the LCD's data pins on the falling edge of pixel clock pin
#define 	HwLCTRL_CLEN				Hw11									// Clippin Enable
#define 	HwLCTRL_R2Y_EN				Hw10									// RGB to YUV Channel Converter Enable
#define 	HwLCTRL_DP_2				Hw9										// One pixel data per 2 PXCLK cycle is output
#define	HwLCTRL_NI_PMODE			Hw8										// Non-interlace Mode (Progressive Mode)
#define	HwLCTRL_TV					Hw7										// TV Mode. In this mode, all values of LVTIMEn registers are divided by 2
#define	HwLCTRL_TFT					Hw6										// TFT-LCD Mode
#define	HwLCTRL_STN					Hw5										// STN-LCD Mode
#define	HwLCTRL_MSEL				Hw4										// Master Select
#define	HwLCTRL_IEN2_EN				Hw3										// Fetch Enable 2
#define	HwLCTRL_IEN1_EN				Hw2										// Fetch Enable 1
#define 	HwLCTRL_IEN0_EN				Hw1										// Fetch Enable 0
#define 	HwLCTRL_LEN_EN				Hw0										// LCD Controller Enable

#define	HwLBC						*(volatile unsigned long *)0xF0003004	// W, LCD Background Color Register

#define	HwLCLKDIV					*(volatile unsigned long *)0xF0003008	// W, LCD Clock Divider Register
#define 	HwLCLKDIV_CS				Hw31									// Clock Source      

#define	HwLHTIME1					*(volatile unsigned long *)0xF000300C	// W, LCD Horizontal Timing Register 1

#define	HwLHTIME2					*(volatile unsigned long *)0xF0003010	// W, LCD Horizontal Timing Register 2

#define	HwLVTIME1					*(volatile unsigned long *)0xF0003014	// W, LCD Vertical Timing Register 1

#define	HwLVTIME2					*(volatile unsigned long *)0xF0003018	// W, LCD Vertical Timing Register 2

#define	HwLVTIME3					*(volatile unsigned long *)0xF000301C	// W, LCD Vertical Timing Register 3

#define	HwLVTIME4					*(volatile unsigned long *)0xF0003020	// W, LCD Vertical Timing Register 4

#define	HwLLUTR						*(volatile unsigned long *)0xF0003024	// W, LCD Lookup Register for Red

#define	HwLLUTG						*(volatile unsigned long *)0xF0003028	// W, LCD Lookup Register for Green

#define	HwLLUTB						*(volatile unsigned long *)0xF000302C	// W, LCD Lookup Register for Blue

#define	HwLDP7L						*(volatile unsigned long *)0xF0003030	// W, LCD Modulo 7 Dithering Pattern (Low)

#define	HwLDP7H						*(volatile unsigned long *)0xF0003034	// W, LCD Modulo 7 Dithering Pattern (High)

#define	HwLDP5						*(volatile unsigned long *)0xF0003038	// W, LCD Modulo 5 Dithering Pattern Register

#define	HwLDP4						*(volatile unsigned long *)0xF000303C	// W, LCD Modulo 4 Dithering Pattern Register

#define	HwLDP3						*(volatile unsigned long *)0xF0003040	// W, LCD 3-bit Dithering Pattern Register

#define	HwLCP1						*(volatile unsigned long *)0xF0003044	// W, LCD Clipping Register

#define	HwLCP2						*(volatile unsigned long *)0xF0003048	// W, LCD Clipping Register

#define	HwLK1						*(volatile unsigned long *)0xF000304C	// W, LCD Keying Register 1
#define	HwLK1_A10_MAX				(Hw28-Hw24)

#define	HwLK2						*(volatile unsigned long *)0xF0003050	// W, LCD Keying Register 2
#define	HwLK2_A20_MAX				(Hw28-Hw24)

#define	HwLKM1						*(volatile unsigned long *)0xF0003054	// W, LCD Keying Mask Register 1
#define	HwLKM1_A11_MAX			(Hw28-Hw24)

#define	HwLKM2						*(volatile unsigned long *)0xF0003058	// W, LCD Keying Mask Register 2
#define	HwLKM2_A21_MAX			(Hw28-Hw24)

#define	HwLDS						*(volatile unsigned long *)0xF000305C	// W, LCD Display Size Register

#define	HwLSTATUS					*(volatile unsigned long *)0xF0003060	// R/Clr, LCD Status Register
#define	HwLSTATUS_VS				Hw15									// Monitoring vertical sync.
#define 	HwLSTATUS_ITY				Hw12									// Interrupt Type
#define 	HwLSTATUS_ICR				Hw8										// Interrupt Clear 
#define 	HwLSTATUS_BY				Hw6										// Busy signal
#define HwLSTATUS_EF				Hw5										// Even-Field(Read Only). 0:Odd field or frame, 1:Even field or frame
#define HwLSTATUS_DD				Hw4										// Disable Done(Read/Clear). If LEN is disabled, DD will be 1 after current frame has been displayed. As MDD of LIM register is cleared, it can be LCD interrupt source
#define HwLSTATUS_RU				Hw3										// Register Update(Read/Clear). It indicates that all registers programmed are applied to current frame data. As MRU of LIM register is cleared, it can be LCD interrupt source
#define HwLSTATUS_FU				Hw0										// FIFO underrun(Read/Clear). It indicates that FIFO underrun has been occurred. In this case, LCLK frequency must be lower. As MFU of LIM register is cleared, it can be LCD interrupt source

#define	HwLIM						*(volatile unsigned long *)0xF0003064	// W, LCD Interrupt Register
#define HwLIM_MDD					Hw4										// Masking disable done interrupt
#define HwLIM_MRU					Hw3										// Masking register update interrupt
#define HwLIM_MFU					Hw0										// Masking FIFO underrun interrupt

#define	HwLI0C						*(volatile unsigned long *)0xF0003068	// W, LCD Image 0 Control Register
#define 	HwLIOC_IMG012				HwZERO									// Image 0 - image 1 - image 2  
#define 	HwLIOC_IMG021				Hw12									// Image 0 - image 2 - image 1  
#define 	HwLIOC_IMG102				Hw13									// Image 1 - image 0 - image 2  
#define 	HwLIOC_IMG201				(Hw13+Hw12)								// Image 2 - image 0 - image 1  
#define 	HwLIOC_IMG120				Hw14									// Image 1 - image 2 - image 0  
#define 	HwLIOC_IMG210				(Hw14+Hw12)								// Image 2 - image 1 - image 0  
#define HwLI0C_BR_BE				Hw7										// Big Endian pixel data
#define HwLI0C_YUV_420				Hw4										// YUV 4:2:0
#define HwLI0C_YUV_422				(Hw5+Hw4)								// YUV 4:2:2
#define HwLI0C_YUV_422P				(Hw6+Hw5+Hw4)							// YUV 4:2:2 Patch
#define HwLI0C_BPP_1				HwZERO									// Bit Per Pixel = 1bpp
#define HwLI0C_BPP_2				Hw0										// Bit Per Pixel = 2bpp
#define HwLI0C_BPP_4				Hw1										// Bit Per Pixel = 4bpp
#define HwLI0C_BPP_332				(Hw1+Hw0)								// Bit Per Pixel = 332bpp
#define HwLI0C_BPP_444				Hw2										// Bit Per Pixel = 444bpp
#define HwLI0C_BPP_565				(Hw2+Hw0)								// Bit Per Pixel = 565bpp
#define HwLI0C_BPP_555				(Hw2+Hw1)								// Bit Per Pixel = 555bpp
#define HwLI0C_BPP_888				(Hw2+Hw1+Hw0)							// Bit Per Pixel = 888bpp

#define HwLI0P						*(volatile unsigned long *)0xF000306C	// W, LCD Image 0 Position Register
	
#define	HwLI0S						*(volatile unsigned long *)0xF0003070	// W, LCD Image 0 Size Register

#define	HwLI0BA0					*(volatile unsigned long *)0xF0003074	// W, LCD Image 0 Base Address 0 Register

#define	HwLI0CA						*(volatile unsigned long *)0xF0003078	// W, LCD Image 0 Current Address Register

#define	HwLI0BA1					*(volatile unsigned long *)0xF000307C	// W, LCD Image 0 Base Address 1 Register

#define HwLI0BA2					*(volatile unsigned long *)0xF0003080	// W, LCD Image 0 Base Address 2 Register

#define	HwLI0O						*(volatile unsigned long *)0xF0003084	// W, LCD Image 0 Offset Register

#define HwLI0SCALE					*(volatile unsigned long *)0xF0003088	// W, LCD Image 0 Scale Ratio
#define	HwLI0SCALE_Y_NS				HwZERO									// Non-Scalable
#define	HwLI0SCALE_Y_D2				Hw4										// DownScale by 2
#define	HwLI0SCALE_Y_D3				Hw5										// DownScale by 3
#define HwLI0SCALE_Y_D4				(Hw5+Hw4)								// DownScale by 4
#define	HwLI0SCALE_Y_D8				(Hw6+Hw5+Hw4)							// DownScale by 8
#define	HwLI0SCALE_Y_U2				(Hw7+Hw4)								// UpScale by 2
#define	HwLI0SCALE_Y_U3				(Hw7+Hw5)								// UpScale by 3
#define	HwLI0SCALE_Y_U4				(Hw7+Hw5+Hw4)							// UpScale by 4
#define	HwLI0SCALE_Y_U8				(Hw7+Hw6+Hw5+Hw4)						// UpScale by 8
#define	HwLI0SCALE_X_NS				HwZERO									// Non-Scalable
#define	HwLI0SCALE_X_D2				Hw0										// DownScale by 2
#define	HwLI0SCALE_X_D3				Hw1										// DownScale by 3
#define HwLI0SCALE_X_D4				(Hw1+Hw0)								// DownScale by 4
#define	HwLI0SCALE_X_D8				(Hw2+Hw1+Hw0)							// DownScale by 8
#define	HwLI0SCALE_X_U2				(Hw3+Hw0)								// UpScale by 2
#define	HwLI0SCALE_X_U3				(Hw3+Hw1)								// UpScale by 3
#define	HwLI0SCALE_X_U4				(Hw3+Hw1+Hw0)							// UpScale by 4
#define	HwLI0SCALE_X_U8				(Hw3+Hw2+Hw1+Hw0)						// UpScale by 8

#define	HwLI1C						*(volatile unsigned long *)0xF000308C	// W, LCD Image 1 Control Register
#define HwLI1C_BR_BE				Hw7										// Big Endian pixel data
#define HwLI1C_YUV_420				Hw4										// YUV 4:2:0
#define HwLI1C_YUV_422				(Hw5+Hw4)								// YUV 4:2:2
#define HwLI1C_YUV_422P				(Hw6+Hw5+Hw4)							// YUV 4:2:2 Patch
#define HwLI1C_BPP_1				HwZERO									// Bit Per Pixel = 1bpp
#define HwLI1C_BPP_2				Hw0										// Bit Per Pixel = 2bpp
#define HwLI1C_BPP_4				Hw1										// Bit Per Pixel = 4bpp
#define HwLI1C_BPP_332				(Hw1+Hw0)								// Bit Per Pixel = 332bpp
#define HwLI1C_BPP_444				Hw2										// Bit Per Pixel = 444bpp
#define HwLI1C_BPP_565				(Hw2+Hw0)								// Bit Per Pixel = 565bpp
#define HwLI1C_BPP_555				(Hw2+Hw1)								// Bit Per Pixel = 555bpp
#define HwLI1C_BPP_888				(Hw2+Hw1+Hw0)							// Bit Per Pixel = 888bpp

#define HwLI1P						*(volatile unsigned long *)0xF0003090	// W, LCD Image 1 Position Register

#define	HwLI1S						*(volatile unsigned long *)0xF0003094	// W, LCD Image 1 Size Register

#define	HwLI1BA0					*(volatile unsigned long *)0xF0003098	// W, LCD Image 1 Base Address 0 Register

#define	HwLI1CA						*(volatile unsigned long *)0xF000309C	// W, LCD Image 1 Current Address Register

#define	HwLI1BA1					*(volatile unsigned long *)0xF00030A0	// W, LCD Image 1 Base Address 1 Register

#define HwLI1BA2					*(volatile unsigned long *)0xF00030A4	// W, LCD Image 1 Base Address 2 Register

#define	HwLI1O						*(volatile unsigned long *)0xF00030A8	// W, LCD Image 1 Offset Register

#define HwLI1SCALE					*(volatile unsigned long *)0xF00030AC	// W, LCD Image 1 Scale Ratio
#define	HwLI1SCALE_Y_NS				HwZERO									// Non-Scalable
#define	HwLI1SCALE_Y_D2				Hw4										// DownScale by 2
#define	HwLI1SCALE_Y_D3				Hw5										// DownScale by 3
#define HwLI1SCALE_Y_D4				(Hw5+Hw4)								// DownScale by 4
#define	HwLI1SCALE_Y_D8				(Hw6+Hw5+Hw4)							// DownScale by 8
#define	HwLI1SCALE_Y_U2				(Hw7+Hw4)								// UpScale by 2
#define	HwLI1SCALE_Y_U3				(Hw7+Hw5)								// UpScale by 3
#define	HwLI1SCALE_Y_U4				(Hw7+Hw5+Hw4)							// UpScale by 4
#define	HwLI1SCALE_Y_U8				(Hw7+Hw6+Hw5+Hw4)						// UpScale by 8
#define	HwLI1SCALE_X_NS				HwZERO									// Non-Scalable
#define	HwLI1SCALE_X_D2				Hw0										// DownScale by 2
#define	HwLI1SCALE_X_D3				Hw1										// DownScale by 3
#define HwLI1SCALE_X_D4				(Hw1+Hw0)								// DownScale by 4
#define	HwLI1SCALE_X_D8				(Hw2+Hw1+Hw0)							// DownScale by 8
#define	HwLI1SCALE_X_U2				(Hw3+Hw0)								// UpScale by 2
#define	HwLI1SCALE_X_U3				(Hw3+Hw1)								// UpScale by 3
#define	HwLI1SCALE_X_U4				(Hw3+Hw1+Hw0)							// UpScale by 4
#define	HwLI1SCALE_X_U8				(Hw3+Hw2+Hw1+Hw0)						// UpScale by 8

#define	HwLI2C						*(volatile unsigned long *)0xF00030B0	// W, LCD Image 2 Control Register
#define	HwLI2C_LUT_EN				Hw9										// Use the Color Lookup Table
#define HwLI2C_BR_BE				Hw7										// Big Endian pixel data
#define HwLI2C_YUV_420				Hw4										// YUV 4:2:0
#define HwLI2C_YUV_422				(Hw5+Hw4)								// YUV 4:2:2
#define HwLI2C_YUV_422P				(Hw6+Hw5+Hw4)							// YUV 4:2:2 Patch
#define HwLI2C_BPP_1				HwZERO									// Bit Per Pixel = 1bpp
#define HwLI2C_BPP_2				Hw0										// Bit Per Pixel = 2bpp
#define HwLI2C_BPP_4				Hw1										// Bit Per Pixel = 4bpp
#define HwLI2C_BPP_332				(Hw1+Hw0)								// Bit Per Pixel = 332bpp
#define HwLI2C_BPP_444				Hw2										// Bit Per Pixel = 444bpp
#define HwLI2C_BPP_565				(Hw2+Hw0)								// Bit Per Pixel = 565bpp
#define HwLI2C_BPP_555				(Hw2+Hw1)								// Bit Per Pixel = 555bpp
#define HwLI2C_BPP_888				(Hw2+Hw1+Hw0)							// Bit Per Pixel = 888bpp

#define HwLI2P						*(volatile unsigned long *)0xF00030B4	// W, LCD Image 2 Position Register

#define	HwLI2S						*(volatile unsigned long *)0xF00030B8	// W, LCD Image 2 Size Register

#define	HwLI2BA0					*(volatile unsigned long *)0xF00030BC	// W, LCD Image 2 Base Address 0 Register

#define	HwLI2CA						*(volatile unsigned long *)0xF00030C0	// W, LCD Image 2 Current Address Register

#define	HwLI2BA1					*(volatile unsigned long *)0xF00030C4	// W, LCD Image 2 Base Address 1 Register

#define HwLI2BA2					*(volatile unsigned long *)0xF00030C8	// W, LCD Image 2 Base Address 2 Register

#define	HwLI2O						*(volatile unsigned long *)0xF00030CC	// W, LCD Image 2 Offset Register

#define HwLI2SCALE					*(volatile unsigned long *)0xF00030D0	// W, LCD Image 2 Scale Ratio
#define	HwLI2SCALE_Y_NS				HwZERO									// Non-Scalable
#define	HwLI2SCALE_Y_D2				Hw4										// DownScale by 2
#define	HwLI2SCALE_Y_D3				Hw5										// DownScale by 3
#define HwLI2SCALE_Y_D4				(Hw5+Hw4)								// DownScale by 4
#define	HwLI2SCALE_Y_D8				(Hw6+Hw5+Hw4)							// DownScale by 8
#define	HwLI2SCALE_Y_U2				(Hw7+Hw4)								// UpScale by 2
#define	HwLI2SCALE_Y_U3				(Hw7+Hw5)								// UpScale by 3
#define	HwLI2SCALE_Y_U4				(Hw7+Hw5+Hw4)							// UpScale by 4
#define	HwLI2SCALE_Y_U8				(Hw7+Hw6+Hw5+Hw4)						// UpScale by 8
#define	HwLI2SCALE_X_NS				HwZERO									// Non-Scalable
#define	HwLI2SCALE_X_D2				Hw0										// DownScale by 2
#define	HwLI2SCALE_X_D3				Hw1										// DownScale by 3
#define HwLI2SCALE_X_D4				(Hw1+Hw0)								// DownScale by 4
#define	HwLI2SCALE_X_D8				(Hw2+Hw1+Hw0)							// DownScale by 8
#define	HwLI2SCALE_X_U2				(Hw3+Hw0)								// UpScale by 2
#define	HwLI2SCALE_X_U3				(Hw3+Hw1)								// UpScale by 3
#define	HwLI2SCALE_X_U4				(Hw3+Hw1+Hw0)							// UpScale by 4
#define	HwLI2SCALE_X_U8				(Hw3+Hw2+Hw1+Hw0)						// UpScale by 8

#define HwDLCTRL					*(volatile unsigned long *)0xF00030D4	// Dual LCD Control  
#define	HwDLCTRL_DRE				Hw4										// Dual LCD Register Update Enable  
#define	HwDLCTRL_DLE				Hw0										// Dual LCD Operation Enable               

#define HwDLCSA0					*(volatile unsigned long *)0xF00030DC	// Dual LCD Configuration Start Address 0                 

#define HwDLCSA1					*(volatile unsigned long *)0xF00030E0	// Dual LCD Configuration Start Address 1              

#define HwY2RP0						*(volatile unsigned long *)0xF00030E4	// YCbCr to RGB Conversion Parameter 0 

#define HwY2RP1						*(volatile unsigned long *)0xF00030E8	// YCbCr to RGB Conversion Parameter 1            

#define	HwLCDLUT					*(volatile unsigned long *)0xF0003C00	// W, LCD Lookup Table

/************************************************************************
*	LCD System Interface Register Define		(Base Addr = 0xF0003400)
************************************************************************/
#define	HwLCDSICTRL0				*(volatile unsigned long *)0xF0003400	// R/W, Control Register for LCDSI
#define HwLCDSICTRL0_IA_LOW			Hw15									// LACBIAS(Data Enable) signal is active low
#define HwLCDSICTRL0_IVS_LOW		Hw14									// LYSYNC signal is active low
#define HwLCDSICTRL0_CS_1			Hw7										// If IM is high, CS1 is active during operationgs. Otherwise, it is no applicable. These bits are only available when IM is high
#define HwLCDSICTRL0_RSP_HIGH		Hw6										// If IM is high, RS is high. Otherwise, it is not applicable
#define HwLCDSICTRL0_FMT_8RGB565	HwZERO									// LCDC pixel data output : D1[15:0] (RGB565)         
                                                                            // LCDSI pixel data output(8bits) : D1[7:0], D1[15:8]
                                                                            // LCDSI CTRL1-4.WBW must be 1.                      
#define HwLCDSICTRL0_FMT_16RGB565	(Hw3+Hw4)								// LCDC pixel data output : D1[15:0] (RGB565)   
                                                                            // LCDSI pixel data output (16bits) : D1[15:0]
                                                                            // LCDSI CTRL1-4.WBW must be 0.               
#define HwLCDSICTRL0_FMT_8RGB888	Hw2										// LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)  
                                                                            // LCDSI pixel data output (8bits): D1[7:0], D2[7:0], D3[7:0] 
                                                                            // LCDSI CTRL1-4.WBW must be 0.                               
#define HwLCDSICTRL0_FMT_9RGB888	(Hw2+Hw3)								// LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)            
                                                                            // LCDSI pixel data output (9bits) : {D1[7:2],D2[7:5]}, {D2[4:2], D3[7:2]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                                           
#define HwLCDSICTRL0_FMT_16RGB888	(Hw2+Hw4)								//LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)            
                                                                            //LCDSI pixel data output (16bits): D1[7:6], {D1[5:2], D2[7:2], D3[7:2]}
                                                                            //LCDSI CTRL1-4.WBW must be 0.                                          
#define HwLCDSICTRL0_FMT_18RGB888	(Hw2+Hw3+Hw4)							// LCDC pixel data output : D1[7:0], D2[7:0], D3[7:0] (RGB888)  
                                                                            // LCDSI pixel data output (16bits): {D1[7:3], D2[7:2], D3[7:3]}
                                                                            // LCDSI CTRL1-4.WBW must be 0.                                 

#define HwLCDSICTRL0_OM			Hw1																		
#define HwLCDSICTRL0_IM			Hw0																			

#define	HwLCDSICTRL1				*(volatile unsigned long *)0xF0003800	// R/W, Control Register for nCS0 when RS = 0
#define	HwLCDSICTRL1_BW_8		HwZERO										// LCDXD data width is 8bits. Prefix W means writing operation
#define 	HwLCDSICTRL1_BW_16		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation
#define 	HwLCDSICTRL1_BW_18		(Hw31+Hw15)								// Data width is 18 bits

#define	HwLCDSICTRL2				*(volatile unsigned long *)0xF0003804	// R/W, Control Register for nCS0 when RS = 1
#define	HwLCDSICTRL2_BW_8		HwZERO										// LCDXD data width is 8bits. Prefix W means writing operation
#define 	HwLCDSICTRL2_BW_16		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation
#define 	HwLCDSICTRL2_BW_18		(Hw31+Hw15)								// Data width is 18 bits

#define	HwLCDSICTRL3				*(volatile unsigned long *)0xF0003808	// R/W, Control Register for nCS1 when RS = 0
#define	HwLCDSICTRL3_BW_8		HwZERO										// LCDXD data width is 8bits. Prefix W means writing operation
#define 	HwLCDSICTRL3_BW_16		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation
#define 	HwLCDSICTRL3_BW_18		(Hw31+Hw15)								// Data width is 18 bits

#define	HwLCDSICTRL4				*(volatile unsigned long *)0xF000380C	// R/W, Control Register for nCS0 when RS = 1
#define	HwLCDSICTRL4_BW_8		HwZERO										// LCDXD data width is 8bits. Prefix W means writing operation
#define 	HwLCDSICTRL4_BW_16		Hw15										// LCDXD data width is 8bits. Prefix R means reading operation
#define 	HwLCDSICTRL4_BW_18		(Hw31+Hw15)								// Data width is 18 bits

#define	HwLCDSICS0RS0				*(volatile short *)0xF0003810	// R/W, If this register is read or written, reading or writing operations are generated on nCS0 while RS = 0            
                                                                                                                                                                                             
#define	HwLCDSICS0RS1				*(volatile short *)0xF0003818	// R/W, If this register is read or written, reading or writing operations are generated on nCS0 while RS = 1            
                                                                                                                                                                                             
#define	HwLCDSICS1RS0				*(volatile short *)0xF0003820	// R/W, If this register is read or written, reading or writing operations are generated on nCS1 while RS = 0            
                                                                                                                                                                                             
#define	HwLCDSICS1RS1				*(volatile short *)0xF0003828	// R/W, If this register is read or written, reading or writing operations are generated on nCS1 while RS = 1            

#define	HwLCDSICTRL5				*(volatile unsigned long *)0xF0003830			// Control register for nCS0 when RS=0
#define	HwLCDSICTRL5_WBW						Hw31									//

#define	HwLCDSICTRL6				*(volatile unsigned long *)0xF0003834			// Control register for nCS0 when RS=1
#define	HwLCDSICTRL6_WBW						Hw31									//
#define	HwLCDSICTRL7				*(volatile unsigned long *)0xF0003838			// Control register for nCS1 when RS=0 
#define	HwLCDSICTRL7_WBW						Hw31									//
#define	HwLCDSICTRL8				*(volatile unsigned long *)0xF000383C			// Control register for nCS1 when RS=1 
#define	HwLCDSICTRL8_WBW						Hw31									//
/***********************************************************************
*	MSC(Memory-to-Memory Scaler) Define			(Base Addr = 0xF0007000)
************************************************************************/
#define	HwSRCBASEY								*(volatile unsigned long *)0xF0007000	// R/W, Source Base Address for Y
                                        	
#define	HwSRCBASEU								*(volatile unsigned long *)0xF0007004	// R/W, Source Base Address for U(Cb)
                                        	
#define	HwSRCBASEV								*(volatile unsigned long *)0xF0007008	// R/W, Source Base Address for V(Cr)
                                        	
#define	HwSRCSIZE								*(volatile unsigned long *)0xF000700C	// R/W, Source Size Information Register
                                        	
#define	HwSRCOFF								*(volatile unsigned long *)0xF0007010	// R/W, Source Offset Information Register
                                        	
#define	HwSRCCFG								*(volatile unsigned long *)0xF0007014	// R/W, Source Configuration Register
#define	HwSRCCFG_422SEQ0						HwZERO									// 4:2:2 Sequential Mode 0
#define	HwSRCCFG_422SEQ1						Hw0										// 4:2:2 Sequential Mode 1
#define	HwSRCCFG_422SEP							Hw1										// 4:2:2 Separate Mode
#define	HwSRCCFG_420SEP							(Hw1+Hw0)								// 4:2:0 Separate Mode
#define HwSRCCFG_RGB565                         Hw2
#define HwSRCCFG_RGB555                         (Hw2+Hw0)
#define HwSRCCFG_RGB444                         (Hw2+Hw1)
#define HwSRCCFG_RGB454                         (Hw2+Hw1+Hw0)
                                        	
#define	HwDSTBASEY								*(volatile unsigned long *)0xF0007020	// R/W, Destination Base Address for Y
                                        	
#define	HwDSTBASEU								*(volatile unsigned long *)0xF0007024	// R/W, Destination Base Address for U(Cb)
                                        	
#define	HwDSTBASEV								*(volatile unsigned long *)0xF0007028	// R/W, Destination Base Address for V(Cr)
                                        	
#define	HwDSTSIZE								*(volatile unsigned long *)0xF000702C	// R/W, Destination Size Information Register
                                        	
#define	HwDSTOFF								*(volatile unsigned long *)0xF0007030	// R/W, Destination Offset Information Register
                                        	
#define	HwDSTCFG								*(volatile unsigned long *)0xF0007034	// R/W, Destination Configuration Register																						// 1 : Y0->U0->Y1->V0->Y2->U1...
 #define  HwMSC_DST_CFG_WAIT                 	(Hw7 | Hw8 | Hw9)//0x00000700                // WAIT   [10:8]
#define	HwDSTCFG_RDY							Hw6										// Access Wait Control Register
#define	HwDSTCFG_PATH							Hw4										// Destination Type Register
#define	HwDSTCFG_422SEQ0						HwZERO									// 4:2:2 Sequential Mode 0
#define	HwDSTCFG_422SEQ1						Hw0										// 4:2:2 Sequential Mode 1
#define	HwDSTCFG_422SEP							Hw1										// 4:2:2 Separate Mode
#define	HwDSTCFG_420SEP							(Hw1 | Hw0)								// 4:2:0 Separate Mode
                                        	
#define	HwMSCINF								*(volatile unsigned long *)0xF0007040	// R/W, Scaling Information Register
                                        	
#define	HwMSCCTR								*(volatile unsigned long *)0xF0007044	// R/W, Control Register
#define	HwMSCCTR_FLS							Hw31									// Flush the internal FIFOs
#define	HwMSCCTR_TST							Hw30									// Should be zero for test purpose
#define	HwMSCCTR_IEN_BUSY						Hw2										// Interrupt Enable Register
#define	HwMSCCTR_IEN_RDY						Hw1										// Interrupt Enable Register
#define	HwMSCCTR_EN								Hw0										// Enable Register
                                        	
#define	HwMSCSTR								*(volatile unsigned long *)0xF0007048	// R/W, Status Register
#define	HwMSCSTR_IBUSY							Hw5										// Busy Interrupt Flag
#define	HwMSCSTR_IRDY							Hw4										// Ready Interrupt Flag
//#define	HwMSCSTR_RDY							Hw4										// Ready Status Register
#define	HwMSCSTR_BUSY							Hw1										// Busy Status Register
#define	HwMSCSTR_RDY							Hw0										// Ready Status Register


/***********************************************************************
*	 Storage DMA Controller Register Define				(Base Addr = 0xF1000A00)
************************************************************************/
#define	Hw					*(volatile unsigned long *)0xF1000A00	//TCC82XX  //To Do...  
#define	HwSAD				*(volatile unsigned long *)0xF1000A00	//

/************************************************************************
*	Channel 0 Memory Controller Register Define		(Base Addr = 0xF0000000)
************************************************************************/
//////////
//To Do...

#define	HwSDCFG					*(volatile unsigned long *)0xF2000000	// R/W, SDRAM Configuration
#define	HwSDCFG_CL				Hw31					// CAS Latency
#define	HwSDCFG_BW				Hw30					// Bus Width 0:32bit, 1:16bit
#define	HwSDCFG_CW8				(HwZERO|Hw28)			// 8bit is used for CAS address					
#define	HwSDCFG_CW9				Hw29					// 9bit is used for CAS address					
#define	HwSDCFG_CW10				(Hw29+Hw28)				// 10bit is used for CAS address					
#define	HwSDCFG_RW11				Hw11					
#define	HwSDCFG_RW12				HwZERO					
#define	HwSDCFG_RW13				Hw10					
#define	HwSDCFG_RW_MAX			(Hw11+Hw10)				 
#define	HwSDCFG_CW_MAX			(Hw29+Hw28)				 
#define 	HwSDCFG_AM				Hw3
#define 	HwSDCFG_PPD				Hw1
#define 	HwSDCFG_SR				Hw0

#define	HwSDCFG1					*(volatile unsigned long *)0xF2000028	 // R/W, Extra SDRAM Configuration

#define	HwSDFSM					*(volatile unsigned long *)0xF2000004	// R, SDRAM FSM Status

#define	HwMCFG						*(volatile unsigned long *)0xF2000008	// R/W, Misc. Configuration

#define	HwMCFG_RDY				Hw15					// Bus Width Flag
#define	HwMCFG_XDM				Hw14					//
#define	HwMCFG_BW					(Hw12+Hw11)
#define	HwMCFG_SDW				Hw7						//
#define 	HwMCFG_ENCK				Hw6
#define	HwMCFG_JTEN				Hw5						// JTAG port Enable
#define	HwMCFG_ECKE				HwMCFG_JTEN			
#define	HwMCFG_SDEN				Hw4						// SDRAM Enable
#define	HwMCFG_RE					Hw3						// SDRAM Emergent Control bit
#define	HwMCFG_RP					Hw2						// SDRAN Clock Masking bit 
#define	HwMCFG_CKM				Hw1						// SDRAN Clock Masking bit 
#define	HwMCFG_RFR				Hw0					//
#define	HwMCFG_ERF				HwMCFG_RFR				

#define	HwTST				*(volatile unsigned long *)0xF200000C	// W, Test mode

#define	HwCSCFG0			*(volatile unsigned long *)0xF2000010	// R/W, Ext. CS Configuration
#define	HwCSCFG1			*(volatile unsigned long *)0xF2000014	// R/W, Ext. CS Configuration
#define	HwCSCFG2			*(volatile unsigned long *)0xF2000018	// R/W, Ext. CS Configration
#define	HwCSCFG3			*(volatile unsigned long *)0xF200001C	// R/W, Ext. CS Configration

//#define	HwCSCFG_BW_8				(Hw29|Hw28)
//#define	HwCSCFG_BW_16			(Hw29)
#define	HwCSCFG_OD				Hw27					//
#define	HwCSCFG_WD				Hw26					//

#define	HwCSCFG_SIZE_32			(0|Hw24)
#define	HwCSCFG_SIZE_16			Hw25
#define	HwCSCFG_SIZE_8			(Hw25+Hw24)
#define	HwCSCFG_TYPE_NAND		(0)
#define	HwCSCFG_TYPE_IDE			Hw22
#define	HwCSCFG_TYPE_SMEM0		Hw23
#define	HwCSCFG_TYPE_SMEM1		(Hw22+Hw23)
#define	HwCSCFG_URDY				Hw21					// Use Ready
#define	HwCSCFG_RDY				Hw20					// Ready/ Busy Select
#define	HwCSCFG_AMSK				Hw19					// Address Mask

#define	HwCSCFG_PSIZE_256			(0)
#define	HwCSCFG_PSIZE_512			(Hw17)
#define	HwCSCFG_PSIZE_1024		(Hw18)
#define	HwCSCFG_PSIZE_2048		(Hw18|Hw17)
#define	HwCSCFG_CADDR				(Hw14)
#define	HwCSCFG_LA				HwCSCFG_CADDR

#define	HwCLKCFG			*(volatile unsigned long *)0xF2000020	// R/W,

#define	HwSDCMD			*(volatile unsigned long *)0xF2000024	// W,
#define	HwSDCMD_CKE				Hw17
#define	HwSDCMD_A10				Hw16
#define	HwSDCMD_CSN				Hw15
#define	HwSDCMD_RAS				Hw14
#define	HwSDCMD_CAS				Hw13
#define	HwSDCMD_WEN				Hw12
//To Do...
//////////

/************************************************************************
*	Virtual MMU Register Define					(Base Addr = 0xF000C000)
************************************************************************/
#define	HwVMT_REGION0							*(volatile unsigned long *)0xF000C000	// R/W, Configuration Register for Region 0

#define	HwVMT_SZ(X)							(((X)-1)*Hw12)
#define	SIZE_4GB								32
#define	SIZE_2GB								31
#define	SIZE_1GB								30
#define	SIZE_512MB								29
#define	SIZE_256MB								28
#define	SIZE_128MB								27
#define	SIZE_64MB								26
#define	SIZE_32MB								25
#define	SIZE_16MB								24
#define	SIZE_8MB								23
#define	SIZE_4MB								22
#define	SIZE_2MB								21
#define	SIZE_1MB								20
#define	HwVMT_REGION_AP_ALL					(Hw11+Hw10)
#define	HwVMT_DOMAIN(X)						((X)*Hw5)
#define	HwVMT_REGION_EN						Hw9										// Region Enable Register
#define	HwVMT_CACHE_ON						Hw3										// Cacheable Register
#define	HwVMT_CACHE_OFF						HwZERO
#define	HwVMT_BUFF_ON						Hw2										// Bufferable Register
#define	HwVMT_BUFF_OFF						HwZERO

#define	HwVMT_REGION0_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION0_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION0_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION1							*(volatile unsigned long *)0xF000C004	// R/W, Configuration Register for Region 1
#define	HwVMT_REGION1_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION1_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION1_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION2							*(volatile unsigned long *)0xF000C008	// R/W, Configuration Register for Region 2
#define	HwVMT_REGION2_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION2_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION2_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION3							*(volatile unsigned long *)0xF000C00C	// R/W, Configuration Register for Region 3
#define	HwVMT_REGION3_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION3_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION3_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION4							*(volatile unsigned long *)0xF000C010	// R/W, Configuration Register for Region 4
#define	HwVMT_REGION4_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION4_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION4_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION5							*(volatile unsigned long *)0xF000C014	// R/W, Configuration Register for Region 5
#define	HwVMT_REGION5_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION5_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION5_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION6							*(volatile unsigned long *)0xF000C018	// R/W, Configuration Register for Region 6
#define	HwVMT_REGION6_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION6_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION6_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_REGION7							*(volatile unsigned long *)0xF000C01C	// R/W, Configuration Register for Region 7
#define	HwVMT_REGION7_EN						Hw9										// Region Enable Register
#define	HwVMT_REGION7_CA						Hw3										// Cacheable Register
#define	HwVMT_REGION7_BU						Hw2										// Bufferable Register
                                    			
#define	HwVMT_TABBASE							*(volatile unsigned long *)0x8001C000	// R, MMU Table Base Address

/************************************************************************
*	NAND Flash Register Define					(Base Addr = N * 0x10000000)
************************************************************************/
//{{{
#define	NAND_CS			0x60000000

#define	HwNDCMD			*(volatile unsigned long *)(NAND_CS + 0x00000000)	// R/W, Command Cycle

#define	HwNDLADR			*(volatile unsigned long *)(NAND_CS + 0x00000004)	// W, Linear Address Cycle

#define	HwNDRADR			*(volatile unsigned long *)(NAND_CS + 0x00000008)	// W, Row Address Cycle

#define	HwNDIADR			*(volatile unsigned long *)(NAND_CS + 0x0000000C)	// W, Single Address Cycle

#define	HwNDDATA			*(volatile unsigned long *)(NAND_CS + 0x00000010)	// R/W, Data Access Cycle

#define	HwNDDATA8			*(volatile unsigned char *)(NAND_CS + 0x00000010)	// R/W, Data Access Cycle

#define	HwNDDATA16			*(volatile unsigned short *)(NAND_CS + 0x00000010)	// R/W, Data Access Cycle

#define	NANDWaitTilNotBusy()	{ while(!(HwGDATA_E & Hw3)); }
//}}}

/************************************************************************
*	System Configuration Register Define	(Base Addr = 0xF1013000)
************************************************************************/
#define HwBMI						*(volatile unsigned long *)0xF1013000		// Bootmode Configuration

#define	HwDTCMWAIT					*(volatile unsigned long *)0xF101300C		// DTCM Wait Control Register
#define	HwDTCMWAIT_EN							Hw0							// If CPU Clock 160MHz to above operational

#define	HwECC_SEL						*(volatile unsigned long *)0xF1013010		//ECC Monitoring Bus Selection Register
#define	HwECC_SEL_EXTERNAL_MEMORY_BUS		~(Hw1|Hw0)					// Select External Memory Bus
#define	HwECC_SEL_DTCM_BUS						Hw0							// Select DTCM Bus
#define	HwECC_SEL_INTERNALSRAM_BUS			Hw1							// Select Interanl SRAM Bus
#define	HwECC_SEL_AHB_BUS						(Hw1|Hw0)					// Select AHB Bus

#define	HwSD_CFG						*(volatile unsigned long *)0xF1013018		//SD/MMC controller configuration Register
#define	HwSD_CFG_EDIAN						Hw16						// By default, EDIAN == 0
#define	HwSD_CFG_WP1							Hw24						// Slot1 Write Protection
#define	HwSD_CFG_WP0							Hw25						// Slot0 Write Protection
#define	HwSD_CFG_DET1							Hw26						// Slot1 Detection
#define	HwSD_CFG_DET0							Hw27						// Slot0 Detection
#define	HwSD_CFG_WP1_E						Hw28						// Slot1 WP Enable
#define	HwSD_CFG_WP0_E						Hw29						// Slot0 WP Enable
#define	HwSD_CFG_D_DET1						Hw30						// Slot1 Direct Detection Enable
#define	HwSD_CFG_D_DET0						Hw31						// Slot0  Direct Detection Enable

#define	HwREMAP						*(volatile unsigned long *)0xF1013020		//SD/MMC controller configuration Register
#define	HwREMAP_REMAP30						~(Hw2+Hw1+Hw0)				// Remap 0x3... to 0x0...(Internal DRAM)
#define	HwREMAP_REMAP20						Hw0							// Remap 0x2... to 0x0...(External SDRAM)
#define	HwREMAP_REMAP40						Hw1							// Remap 0x4... to 0x0...(External SRAM)
#define	HwREMAP_REMAP70						(Hw1+Hw0)					// Remap 0x7... to 0x0...(External CS3_NOR)
#define	HwREMAP_REMAPE0						(Hw2+Hw1+Hw0)				// Remap 0xE... to 0x0...(Internal Boot ROM)

#define	HwNFC_ACK_SEL					*(volatile unsigned long *)0xF1013024		//NFC ACK Selection Register Register
#define	HwNFC_ACK_SEL_ASEL					Hw0							// 0 -Select GDMA0 ACK , 1 -Select GDMA1 ACK

#define	HwGP_B_DRV					*(volatile unsigned long *)0xF1013080		//GPIO_B Drive Strength Control Register Register
#define 	HwGP_B_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_B_DRV_6mA						0x55555555					// ~6mA - Default
#define 	HwGP_B_DRV_8mA						0xAAAAAAAA					// ~8mA
#define 	HwGP_B_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA

#define	HwGP_C_DRV					*(volatile unsigned long *)0xF1013084		//GPIO_C Drive Strength Control Register Register
#define 	HwGP_C_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_C_DRV_6mA						0x55555555					// ~6mA - Default
#define 	HwGP_C_DRV_8mA						0xAAAAAAAA					// ~8mA
#define 	HwGP_C_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA

#define	HwGP_E_DRV						*(volatile unsigned long *)0xF1013088		//GPIO_E Drive Strength Control Register Register
#define 	HwGP_E_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_E_DRV_6mA						0x00005555					// ~6mA - Default
#define 	HwGP_E_DRV_8mA						0x0000AAAA					// ~8mA
#define 	HwGP_E_DRV_11_5mA					0x0000FFFF					// ~11.5mA

#define	HwGP_F_DRV						*(volatile unsigned long *)0xF101308C		//GPIO_F Drive Strength Control Register Register
#define 	HwGP_F_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_F_DRV_6mA						0x55555555					// ~6mA - Default
#define 	HwGP_F_DRV_8mA						0xAAAAAAAA					// ~8mA
#define 	HwGP_F_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA

#define	HwGP_XA0_DRV					*(volatile unsigned long *)0xF1013090		//XA0 Drive Strength Control Register Register
#define 	HwGP_XA0_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_XA0_DRV_6mA						0x55555555					// ~6mA 
#define 	HwGP_XA0_DRV_8mA						0xAAAAAAAA					// ~8mA - Default
#define 	HwGP_XA0_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA

#define	HwGP_XA1_DRV					*(volatile unsigned long *)0xF1013094		//XA1 Drive Strength Control Register Register
#define 	HwGP_XA1_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_XA1_DRV_6mA						0x55555555					// ~6mA 
#define 	HwGP_XA1_DRV_8mA						0xAAAAAAAA					// ~8mA - Default
#define 	HwGP_XA1_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA
/* HwGP_XA1_DRV
CD1[24],CD0[24] --> CS_nCS0 port drive strength control
CD1[25],CD0[25] --> CS_nCS1 port drive strength control
CD1[26],CD0[26] --> CS_nCS2 port drive strength control
CD1[27],CD0[27] --> CS_nCS3 port drive strength control
CD1[28],CD0[28] --> nOE port drive strength control
CD1[29],CD0[29] --> nWE port drive strength control
CD1[30],CD0[30] --> SDR_nCS port drive strength control
CD1[31],CD0[31] --> SDR_CLK port drive strength control -Fix ~8mA
*/

#define	HwGP_XD0_DRV					*(volatile unsigned long *)0xF1013098		//XD0 Drive Strength Control Register Register
#define 	HwGP_XD0_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_XD0_DRV_6mA						0x55555555					// ~6mA 
#define 	HwGP_XD0_DRV_8mA						0xAAAAAAAA					// ~8mA - Default
#define 	HwGP_XD0_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA

#define	HwGP_XD1_DRV					*(volatile unsigned long *)0xF1013098		//XD0 Drive Strength Control Register Register
#define 	HwGP_XD1_DRV_3mA						0x00000000					// ~3mA
#define 	HwGP_XD1_DRV_6mA						0x55555555					// ~6mA 
#define 	HwGP_XD1_DRV_8mA						0xAAAAAAAA					// ~8mA - Default
#define 	HwGP_XD1_DRV_11_5mA					0xFFFFFFFF					// ~11.5mA

/************************************************************************
*	ETC Define
************************************************************************/
#define PLL_FREQ				2200000//2040000//3360000//2880000//2040000//2031428//2660000 100Hz
#define 	PLL1_FREQ	   			1560000
#define	BUS_CLK_DIV			2

#define 	IO_CKC_DTCMWait		1600000

#define	IO_CKC_Fmaxcpu			PLL_FREQ
#define	IO_CKC_Fmaxbus			(PLL_FREQ/BUS_CLK_DIV)

#define	IO_CKC_Fpll_44KHz_HIGH	PLL_FREQ		// 203.1428 MHz for TCC77x

#define	IO_CKC_Fxin				120000

#define	IO_CKC_Fpll_32KHz		1920000		// 196.8000 MHz for TCC75x
//#define	IO_CKC_Fpll_44KHz		1920000		// 192.0000 MHz for TCC75x
#define	IO_CKC_Fpll_44KHz		1130000		// 1128960
#define	IO_CKC_Fpll_48KHz		1920000		// 196.8000 MHz for TCC75x


#define 	MAX_PLL0_CLK			PLL_FREQ
#define 	MAX_CPU_CLK			(PLL_FREQ/10000)
#define 	MAX_BUS_CLK			(MAX_CPU_CLK/BUS_CLK_DIV)

#define 	RETURN_PLL 				IO_CKC_Fpll_44KHz
#define 	RETURN_CPU_CLK			(RETURN_PLL/10000)
#define 	RETURN_BUS_CLK			(RETURN_CPU_CLK/2)

/************************************************************************
*	ARM
************************************************************************/
#define	IO_ARM_MMU_DTCM			Hw16
#define	IO_ARM_MMU_ICACHE		Hw12
#define	IO_ARM_MMU_DCACHE		Hw2
#define	IO_ARM_MMU_PROT			Hw0
#define	IO_ARM_MMU_ALL			(Hw16+Hw12+Hw2+Hw0)

#ifndef BITSET
#define	BITSET(X, MASK)				( (X) |= (unsigned long)(MASK) )
#endif
#ifndef BITSCLR
#define	BITSCLR(X, SMASK, CMASK)	( (X) = ((((unsigned long)(X)) | ((unsigned long)(SMASK))) & ~((unsigned long)(CMASK))) )
#endif
#ifndef BITCSET
#define	BITCSET(X, CMASK, SMASK)	( (X) = ((((unsigned long)(X)) & ~((unsigned long)(CMASK))) | ((unsigned long)(SMASK))) )
#endif
#ifndef BITCLR
#define	BITCLR(X, MASK)				( (X) &= ~((unsigned long)(MASK)) )
#endif
#ifndef BITXOR
#define	BITXOR(X, MASK)				( (X) ^= (unsigned long)(MASK) )
#endif
#ifndef ISZERO
#define	ISZERO(X, MASK)				(  ! (((unsigned long)(X)) & ((unsigned long)(MASK))) )
#endif
#ifndef ISSET
#define	ISSET(X, MASK)				( (unsigned long)(X) & ((unsigned long)(MASK)) )
#endif
#ifndef ISCLR
#define	ISCLR(X, MASK)				(  ! (((unsigned long)(X)) & ((unsigned long)(MASK))) )
#endif

#endif /* __TCC83x_H__ */

