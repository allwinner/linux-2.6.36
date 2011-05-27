/*
 * Driver for pnx67xx SPI Controllers
 *
 * Copyright (C) 2004 Philips Semiconductors
 * Copyright (C) 2006 Atmel Corporation
 * Copyright (C) 2008 Elektrobit
 *
 * This driver implements spi controller driver for pnx67xx SoC. This code
 * is modified from atmel_spi.c by Haavard Skinnemoen <hskinnemoen@atmel.com>
 * and Philips Semiconductors source code.
 *
 * Contact: Sami Nurmenniemi <ext-sami.nurmenniemi@elektrobit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PNX_SPI_DRV_H_
#define _PNX_SPI_DRV_H_

#include <linux/kernel.h>
#include <linux/interrupt.h>
//#include <asm/arch/registers.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define BUFFER_SIZE             PAGE_SIZE
#define INVALID_DMA_ADDRESS     0xffffffff

/*
 * The core SPI transfer engine just talks to a register bank to set up
 * DMA transfers; transfer queue progress is driven by IRQs.  The clock
 * framework provides the base clock, subdivided for each spi_device.
 *
 * Newer controllers, marked with "new_1" flag, have:
 *  - CR.LASTXFER
 *  - SPI_MR.DIV32 may become FDIV or must-be-zero (here: always zero)
 *  - SPI_SR.TXEMPTY, SPI_SR.NSSR (and corresponding irqs)
 *  - SPI_CSRx.CSAAT
 *  - SPI_CSRx.SBCR allows faster clocking
 */
struct pnx67xx_spi {
	spinlock_t				lock;

	void __iomem			*regs;
	u32						phys_addr;
	int						irq;
	struct clk				*clk;
	struct platform_device	*pdev;
/*	unsigned				new_1:1;*/
	struct spi_device		*stay;

	u8						stopping;
	struct list_head		queue;
	struct spi_transfer		*current_transfer;
	unsigned long			remaining_bytes;
	int						txmode;

	void					*buffer;
	dma_addr_t				buffer_dma;

	int 					channel;
	int 					bits_per_word;
	int 					origsize;
	int 					lock_count;

	bool					spi_complete;
	bool					dma_complete;

};

/*#define ALGO_NAME "algo-pnx"
#define DRIVER_NAME "spi-pnx"
#define DRIVER_VERSION "1.5"*/

/* Heuristic : bad hack , will correct later . Masks also avoid the access to VMALLOC space at C4000000 on pnx4008 */
/* returns non-zero for non-dma memory */
//#define VALIDATE_DMA_ADDR(ptr) (((long)(ptr) & 0xFC000000 ) != 0xC0000000)

#define PNX_SPI_DMA_CHAN( id ) ( ( id == 2 ) ? 6 : 5 )
#define PNX_SPI_CLOCK( id ) ( ( id == 2 ) ? "SPI2" : "SPI1" )

//-----------------------------------------------------------------------------
// Standard include files:
//-----------------------------------------------------------------------------

struct ipxxxx_data {
	u32	*base_address;	/* Base address of the device  */
	u8	irq;		/* IRQ number for the device */
};

#ifdef __cplusplus
extern "C"
{
#endif

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------


#define SPI_CON_RATE_MASK			0x7f

#define SPI_RECEIVE                        0
#define SPI_TRANSMIT                       1

#define SPI_ENDIAN_SWAP_NO                 0
#define SPI_ENDIAN_SWAP_YES                1


// SPI block : offset addresses of registers
#define SPI_ADR_OFFSET_GLOBAL              0x0000      // R/W - Global control register
#define SPI_ADR_OFFSET_CON                 0x0004      // R/W - Control register
#define SPI_ADR_OFFSET_FRM                 0x0008      // R/W - Frame count register
#define SPI_ADR_OFFSET_IER                 0x000C      // R/W - Interrupt enable register
#define SPI_ADR_OFFSET_STAT                0x0010      // R/W - Status register
#define SPI_ADR_OFFSET_DAT                 0x0014      // R/W - Data register
#define SPI_ADR_OFFSET_DAT_MSK             0x0018      //  W  - Data register for using mask mode
#define SPI_ADR_OFFSET_MASK                0x001C      // R/W - Mask register
#define SPI_ADR_OFFSET_ADDR                0x0020      // R/W - Address register
#define SPI_ADR_OFFSET_TIMER_CTRL_REG      0x0400      // R/W - IRQ timer control register
#define SPI_ADR_OFFSET_TIMER_COUNT_REG     0x0404      // R/W - Timed interrupt period register
#define SPI_ADR_OFFSET_TIMER_STATUS_REG    0x0408      // R;R/C - RxDepth and interrupt status register

// Bit definitions for SPI_GLOBAL register
#define SPI_GLOBAL_BLRES_SPI               0x00000002  // R/W - Software reset, active high
#define SPI_GLOBAL_SPI_ON                  0x00000001  // R/W - SPI interface on

// Bit definitions for SPI_CON register
#define SPI_CON_SPI_BIDIR                  0x00800000  // R/W - SPI data in bidir. mux
#define SPI_CON_SPI_BHALT                  0x00400000  // R/W - Halt control
#define SPI_CON_SPI_BPOL                   0x00200000  // R/W - Busy signal active polarity
#define SPI_CON_SPI_SHFT                   0x00100000  // R/W - Data shifting enable in mask mode
#define SPI_CON_SPI_MSB                    0x00080000  // R/W - Transfer LSB or MSB first
#define SPI_CON_SPI_EN                     0x00040000  // R/W - SPULSE signal enable
#define SPI_CON_SPI_MODE_MASK              0x00030000  // R/W - SCK polarity and phase modes
#define SPI_CON_SPI_MODE3                  0x00030000  // R/W - SCK polarity and phase mode 3
#define SPI_CON_SPI_MODE2                  0x00020000  // R/W - SCK polarity and phase mode 2
#define SPI_CON_SPI_MODE1                  0x00010000  // R/W - SCK polarity and phase mode 1
#define SPI_CON_SPI_MODE0                  0x00000000  // R/W - SCK polarity and phase mode 0
#define SPI_CON_RxTx                       0x00008000  // R/W - Transfer direction
#define SPI_CON_THR                        0x00004000  // R/W - Threshold selection
#define SPI_CON_SHIFT_OFF                  0x00002000  // R/W - Inhibits generation of clock pulses on sck_spi pin
#define SPI_CON_BITNUM_MASK                0x00001E00  // R/W - No of bits to tx or rx in one block transfer
#define SPI_CON_CS_EN                      0x00000100  // R/W - Disable use of CS in slave mode
#define SPI_CON_MS                         0x00000080  // R/W - Selection of master or slave mode
#define SPI_CON_RATE                       0x0000007F  // R/W - Transmit/receive rate

#define SPI_CON_BITNUM_SHIFT                9
#define SPI_CON_MODE_SHIFT                 16
#define SPI_INITIAL_BITWIDTH				16

#define SPI_CON_RATE_0                     0x00
#define SPI_CON_RATE_1                     0x01
#define SPI_CON_RATE_2                     0x02
#define SPI_CON_RATE_3                     0x03
#define SPI_CON_RATE_4                     0x04
#define SPI_CON_RATE_13                     0x13

// Bit definitions for SPI_FRM register
#define SPI_FRM_SPIF                       0x0000FFFF  // R/W - Number of frames to be transfered

// Bit definitions for SPI_IER register
#define SPI_IER_SPI_INTCS                  0x00000004  // R/W - SPI CS level change interrupt enable
#define SPI_IER_SPI_INTEOT                 0x00000002  // R/W - End of transfer interrupt enable
#define SPI_IER_SPI_INTTHR                 0x00000001  // R/W - FIFO threshold interrupt enable

// Bit definitions for SPI_STAT register
#define SPI_STAT_SPI_INTCLR                0x00000100  // R/WC - Clear interrupt
#define SPI_STAT_SPI_EOT                   0x00000080  // R/W - End of transfer
#define SPI_STAT_SPI_BSY_SL                0x00000040  // R/W - Status of the input pin spi_busy
#define SPI_STAT_SPI_CSL                   0x00000020  // R/W - Indication of the edge on SPI_CS that caused an int.
#define SPI_STAT_SPI_CSS                   0x00000010  // R/W - Level of SPI_CS has changed in slave mode
#define SPI_STAT_SPI_BUSY                  0x00000008  // R/W - A data transfer is ongoing
#define SPI_STAT_SPI_BF                    0x00000004  // R/W - FIFO is full
#define SPI_STAT_SPI_THR                   0x00000002  // R/W - No of entries in Tx/Rx FIFO
#define SPI_STAT_SPI_BE                    0x00000001  // R/W - FIFO is empty

// Bit definitions for SPI_DAT register
#define SPI_DAT_SPID                       0x0000FFFF  // R/W - SPI data bits

// Bit definitions for SPI_DAT_MSK register
#define SPI_DAT_MSK_SPIDM                  0x0000FFFF  //  W  - SPI data bits to send using the masking mode

// Bit definitions for SPI_MASK register
#define SPI_MASK_SPIMSK                    0x0000FFFF  // R/W - Masking bits used for validating data bits

// Bit definitions for SPI_ADDR register
#define SPI_ADDR_SPIAD                     0x0000FFFF  // R/W - Address bits to add to the data bits

// Bit definitions for SPI_TIMER_CTRL_REG register
#define SPI_TIMER_CTRL_REG_TIRQE           0x00000004  // R/W - Timer interrupt enable
#define SPI_TIMER_CTRL_REG_PIRQE           0x00000002  // R/W - Peripheral interrupt enable
#define SPI_TIMER_CTRL_REG_MODE            0x00000001  // R/W - Mode

// Bit definitions for SPI_TIMER_COUNT_REG register
#define SPI_TIMER_COUNT_REG                0x0000FFFF  // R/W - Timed interrupt period

// Bit definitions for SPI_TIMER_STATUS_REG register
#define SPI_TIMER_STATUS_REG_INT_STAT      0x00008000  // R/C - Interrupt status
#define SPI_TIMER_STATUS_REG_FIFO_DEPTH    0x0000007F  //  R  - FIFO depth value (for debug purpose)



/* number of bytes in the SPI RX/TX fifo */
#define SPI_FIFOSIZE 64
#define SPI_THR_TX    8
#define SPI_THR_RX    56




/* less than SPI_TX_DMALIMIT bytes will be transferred by PIO even if DMA is enabled */

#if defined CONFIG_SPI_DMA

#if defined CONFIG_SPI_TX_DMA_THRESHOLD
#if CONFIG_SPI_TX_DMA_THRESHOLD > 0
#define SPI_TX_DMALIMIT CONFIG_SPI_TX_DMA_THRESHOLD
#else
#error "Invalid SPI_TX_DMA_THRESHOLD in kernel config "
#endif
#else

#define SPI_TX_DMALIMIT 30
#endif

#endif

/* There is no point making this smaller than the FIFO size because transfers
   less than FIFO size are received via PIO even in the DMA case */

#if defined CONFIG_SPI_RX_DMA_THRESHOLD
#if CONFIG_SPI_RX_DMA_THRESHOLD > SPI_FIFOSIZE
#define SPI_RX_DMALIMIT CONFIG_SPI_RX_DMA_THRESHOLD
#else
#error "Invalid SPI_RX_DMA_THRESHOLD in kernel config"
#endif
#else

#define SPI_RX_DMALIMIT SPI_FIFOSIZE
#endif




#if ! defined _LINUX_WAIT_H
#error Need linux/wait.h included before this
#endif

/**********************************************************************/
/* pin level execution tracing : can extend to multiple pins by using
  TRACE_DECL / TRACE_START for each pin in a suitable place */

#undef TRACE_SPI

#if defined TRACE_SPI
#include <asm/atomic.h>
#include <asm/arch/gpio.h>
/* define the GPIO pin in use */
#define SPI_TIME GPO_18
extern atomic_t trace;

#define TRACE_DECL(x)       atomic_t trace
#define TRACE_START(x)     {atomic_set(&trace,0);\
                            pnx_request_gpio(x);\
			    pnx_set_gpio_direction(x,0 /*GPIO_MODE_OUT*/);\
			    pnx_write_gpio_pin(x,0);}

#define TRACE_ON(x)        {if(atomic_add_return(1,&trace)==1) GPIO_WritePin(x,1);}
#define TRACE_OFF(x)       {if(atomic_sub_return(1,&trace)==0) GPIO_WritePin(x,0);}
/* OS call bracketing to stop tracing during the call */
#define DONT_TRACE(stmt,x) {if(atomic_sub_return(1,&trace)==0) GPIO_WritePin(x,0);\
                            stmt; \
			    if(atomic_add_return(1,&trace)==1) GPIO_WritePin(x,1);}


#else
/* NB no semicolon on TRACE_DECL */
#define TRACE_DECL(x)
#define TRACE_ON(x)    while(0) {}
#define TRACE_OFF(x)   while(0) {}
#define TRACE_START(x) while(0) {}
#define DONT_TRACE(stmt,x) stmt
#endif

#define SPI_MODE(n)	(n&3)

/* these definitions remain regardless of DMA/nonDMA usage */

//typedef enum  {PIO_Write,PIO_WriteComplete,PIO_Read,PIO_ReadComplete,DMA_Write,DMA_Read,DMA_ReadLast} TransferType;

//-----------------------------------------------------------------------------
// Exported functions:
//-----------------------------------------------------------------------------

extern void pnx67xx_spi_next_message(struct spi_master *master);
extern void pnx67xx_spi_next_xfer(struct spi_master *master,
									struct spi_message *msg);
extern int pnx67xx_spi_transfer(struct spi_device *spi,
								struct spi_message *msg);

extern void spi_enable_generating_clock(struct pnx67xx_spi *spi);
extern void spi_stop_generating_clock(struct pnx67xx_spi *spi);
extern void spi_set_ier( struct pnx67xx_spi *spi, int ie);
extern void spi_write_frame(struct pnx67xx_spi *spi, u16 value);
extern void spi_write_stat(struct pnx67xx_spi *spi, u16 value);
extern u16 spi_read_data(struct pnx67xx_spi *spi);
extern void spi_write_data(struct pnx67xx_spi *spi, u16 data);
extern void spi_set_thr_mode_off(struct pnx67xx_spi *spi);
extern int spi_read_stat(struct pnx67xx_spi *spi);
extern int pnx67xx_spi_hw_init(struct pnx67xx_spi *spi);
extern void pnx67xx_spi_hw_reset( struct pnx67xx_spi *spi, bool leave_on );
extern void cs_activate( struct spi_device *spi );
extern void cs_deactivate( struct spi_device *spi );
extern void pnx67xx_spi_set_bitwidth( struct pnx67xx_spi *spi, u16 bitwidth );
extern void pnx67xx_spi_set_mode( struct pnx67xx_spi *spi, u16 mode );
extern void pnx67xx_spi_set_clock( struct pnx67xx_spi *spi, u32 clock );
extern int pnx67xx_spi_setup_gpio( int gpio );
extern void pnx67xx_spi_release_gpio( int gpio );
extern void set_tx_rx( struct pnx67xx_spi *pnx_spi );

extern irqreturn_t pnx67xx_spi_interrupt(int irq, void *dev_id);
extern int pnx67xx_spi_dma_map_xfer(struct pnx67xx_spi *pnx_spi,
									struct spi_transfer *xfer);
extern void pnx67xx_do_dma_transfer( struct pnx67xx_spi *pnx_spi,
								dma_addr_t tx_dma, dma_addr_t rx_dma, u32 len );
extern int pnx67xx_spi_request_dma_channel( struct spi_master *master );
extern void pnx67xx_spi_free_dma_channel( struct pnx67xx_spi *spi );


#ifdef __cplusplus
}
#endif

#endif // #ifndef _PNX_SPI_DRV_H_

//-----------------------------------------------------------------------------
// End of file spi.h
//-----------------------------------------------------------------------------
