/**
*
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H

//#define SPI_IF_TEST
//#define USE_SDIO_IRQ
#define USE_FW_DOWNLOAD
#define USE_NO_CLOCK
#define GPIO_DEBUG
#define USE_POWER_MANAGEMENT
//#define JTAG_CONNECTION
//#define LGIT_MODULE

//#define USE_TRACE
//#define USE_PRINTBUF

/* define types of interrupts supported */
//#define USE_IRQ_ID_RESPONSE_COMPLETION
//#define USE_IRQ_ID_FIFO_READ_READY
//#define USE_IRQ_ID_FIFO_WRITE_READY
//#define USE_IRQ_ID_RW_ACCESS_COMPLETION

#ifdef USE_SDIO_IRQ
#define HOST_ATTENTION_SPI     0x01
#else
#define HOST_ATTENTION_SPI     0x5a
#endif

#define SPI_CHIP_SELECT GPIO_A30

#if 1
#define TARGET_IRQ_NUM       IRQ_EXTINT(14)		//IRQ (generated from EXTINT) from the target
#else
#define TARGET_IRQ_NUM       IOMUX_TO_IRQ(MX31_PIN_GPIO1_6) /// [Samsung] this is for irq from the device.
#endif

#endif /* _HOST_DEF_H */
