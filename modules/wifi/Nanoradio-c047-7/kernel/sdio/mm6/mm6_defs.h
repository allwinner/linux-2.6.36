/**
* $Id: mm6_defs.h 9119 2008-06-09 11:50:52Z joda $
*
* MM6 Register definitions and bitmasks
*/

#ifndef _MM6_DEFS_H
#define _MM6_DEFS_H
        
#define BITMASK_0       (0x1)
#define BITMASK_1       (0x1<<1)
#define BITMASK_2       (0x1<<2)
#define BITMASK_3       (0x1<<3)
#define BITMASK_4       (0x1<<4)
#define BITMASK_5       (0x1<<5)
#define BITMASK_6       (0x1<<6)
#define BITMASK_7       (0x1<<7)
#define BITMASK_8       (0x1<<8)
#define BITMASK_9       (0x1<<9)
#define BITMASK_10      (0x1<<10)
#define BITMASK_11      (0x1<<11)
#define BITMASK_12      (0x1<<12)
#define BITMASK_13      (0x1<<13)
#define BITMASK_14      (0x1<<14)
#define BITMASK_15      (0x1<<15)
#define BITMASK_16      (0x1<<16)
#define BITMASK_17      (0x1<<17)
#define BITMASK_18      (0x1<<18)
#define BITMASK_19      (0x1<<19)
#define BITMASK_20      (0x1<<20)
#define BITMASK_21      (0x1<<21)
#define BITMASK_22      (0x1<<22)
#define BITMASK_23      (0x1<<23)
#define BITMASK_24      (0x1<<24)
#define BITMASK_25      (0x1<<25)
#define BITMASK_26      (0x1<<26)
#define BITMASK_27      (0x1<<27)
#define BITMASK_28      (0x1<<28)
#define BITMASK_29      (0x1<<29)
#define BITMASK_30      (0x1<<30)
#define BITMASK_31      (0x1<<31)

/* SDCT register settings: SD Host controller settings */
#define SDCT_ENABLE     BITMASK_0  // SDC Controller: 0=disabled, 1=enabled 
#define SDCT_TC		BITMASK_1  // Transmit command 
#define SDCT_TCT	BITMASK_2  // Terminate Current Transaction 
#define SDCT_CCO	BITMASK_3  // Command CRC Override 
#define SDCT_DCO	BITMASK_4  // Data CRC Override 
#define SDCT_ROD	BITMASK_5  // Open Drain Pullup: 0=on, 1=off 
#define SDCT_CSD	BITMASK_6  // Clock Stop Disable: 0=disabled, 1=enabled 
#define SDCT_TO		BITMASK_7  // Terminate Operation if card is busy 0=don't, 1=do 
#define SDCT_DPW	BITMASK_8  // select the data path width       
#define SDCT_SDIOINT	BITMASK_9  // SDIO Interrupt Enable/Disable       
#define SDCT_IN4ME	BITMASK_10 // SDIO Interrupt in 4-bit multi block transfer
#define SDCT_ABORTIO	BITMASK_14 // issue CMD52 
#define SDCT_MIUR	BITMASK_15 // Mask Interrupt Until Response
#define SDCT_ACMD	BITMASK_31 // enable stop command for ACMD  

/* SDIM register settings: SDIO interrupt settings */
#define SDIM_CRRM       BITMASK_0   // Command Response Received Mask 
#define SDIM_DRRM       BITMASK_1   // Data Response Received Mask 
#define SDIM_DTCM       BITMASK_2   // Data Transfer Complete Mask 
#define SDIM_ETOM       BITMASK_3   // Error or Timeout Mask 
#define SDIM_TFHEM      BITMASK_4   // Transmit Fifo Half Empty Mask 
#define SDIM_RFHFM      BITMASK_5   // Receive Fifo Half Full Mask 
#define SDIM_TDM        BITMASK_6   // Transmit DMA Request Mask 
#define SDIM_RDM        BITMASK_7   // Receive DMA Request Mask 
#define SDIM_D3M        BITMASK_21  // SDDAT[3] Mask 
#define SDIM_SDINTM     BITMASK_22  // SDINT Mask

/* SDIS register settings: SD Host status register */
#define	SDIS_CRR        BITMASK_0	// Command Response Recieved   
#define	SDIS_DRR        BITMASK_1	// Data Response Received   
#define	SDIS_DTC        BITMASK_2	// Data Transfer Complete   
#define	SDIS_ETO        BITMASK_3	// Error or Timeout   
#define	SDIS_TFHE       BITMASK_4	// Tx Fifo Half Empty   
#define	SDIS_RFHF       BITMASK_5	// Rx FIFO Half Full  
#define	SDIS_SDINT      BITMASK_6	// Interrupt between Block Transfer
#define	SDIS_TFU        BITMASK_8	// Tx FIFO Underrrun   
#define	SDIS_TFF        BITMASK_9	// Tx FIFO Full   
#define	SDIS_RFO        BITMASK_10	// Rx FIFO overrun   
#define	SDIS_RFE        BITMASK_11	// Rx FIFO Empty   
#define	SDIS_RCE        BITMASK_12	// Response CRC error   
#define	SDIS_RDCE       BITMASK_13	// Read Data CRC error   
#define	SDIS_WDCE       BITMASK_14	// Write dtat CRC error   
#define	SDIS_RTO        BITMASK_15	// Response Time out error   
#define	SDIS_DTO        BITMASK_16	// Data Timeout Error   
#define	SDIS_CIDTO      BITMASK_17	// Card ID mode Timeout Error   
#define	SDIS_OKSMC      BITMASK_18	// OK to stop the clock   
#define	SDIS_TIP        BITMASK_19	// Transaction in Progress   
#define	SDIS_CBY        BITMASK_20	// Card Busy   
#define SDIS_CDT        BITMASK_21	// Card Detect
#define	SDIS_CMPBLK     (0x1F000000)	// Number of blocks Transmitted   
#define	SDIS_CLRALL     (0x0013F500)	// Clear all error source bits   

/* SDIC register settings: SD Host interrupt clear register */
#define	SDIC_CRRFC      BITMASK_0	// Command Response Recieved Flag Clear 
#define	SDIC_DRRFC      BITMASK_1	// Data Response Received Flag Clear  
#define	SDIC_DTCFC      BITMASK_2	// Data Transfer Complete Flag Clear   
#define	SDIC_ETOFC      BITMASK_3	// Error or Timeout Flag Clear


#define	SCTRL_SCPER_CTRL    (volatile unsigned long *) (VIRT_STCL_BASE + 0x0020)

#define SCTRL_GPR0              (volatile unsigned long *) (VIRT_STCL_BASE + 0x0138)
        
#if defined (SDIO_SLOT_A) || defined (SDIO_BUILTIN)
#define	SLOT_ADDR           VIRT_SDIO1_BASE
#define	PHYS_SDIO_BASE      PHYS_SDIO1_BASE
#define IRQ_NO              IRQ_SDIO_1
#define CLK_SRC             MM6PLUS_SD1CLK
#define DMA_SDC_RX          dma_sdc1_rx
#define DMA_SDC_TX          dma_sdc1_tx

#else

#define	SLOT_ADDR           VIRT_SDIO2_BASE
#define	PHYS_SDIO_BASE      PHYS_SDIO2_BASE
#define IRQ_NO              IRQ_SDIO_2
#define CLK_SRC             MM6PLUS_SD2CLK
#define DMA_SDC_RX          dma_sdc2_rx
#define DMA_SDC_TX          dma_sdc2_tx

#endif

#endif /* !_MM6_DEFS_H */
