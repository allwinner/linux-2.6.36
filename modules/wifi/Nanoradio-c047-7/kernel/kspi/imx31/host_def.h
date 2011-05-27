/**
*
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H


/*************************/
/* General configuration */
/*************************/
#define USE_FW_DOWNLOAD
#define USE_POWER_MANAGEMENT
//#define JTAG_CONNECTION

#include <asm/hardware.h>	// for IOMUX_TO_IRQ
#define TARGET_IRQ_NUM	IOMUX_TO_IRQ(MX31_PIN_GPIO1_6)

/* SDIO IRQ (level IRQ signal) would work only if the kernel SPI driver
 * could hold Chip Select asserted (LOW) continuously after driver init,
 * because the target can assert IRQ only if Chip Select is active.
 * It would also work with future baseband versions supporting
 * continuous SPI interrupt mode (CSPI feature).
 */
//#define USE_SDIO_IRQ


/***********************************/
/* SPI kernel driver configuration */
/***********************************/
#define SPI_DRIVER_NAME		"nano_spi"

#define SPI_SPEED_HZ 		8000000

/* Some nanoradio devices need a slower clock frequency
 * when pulled out of power save mode with CMD52.
 * This is not the case - fortunately, as IMX31 kernel driver
 * fails when SPI_SPEED_HZ_SLOW < SPI_SPEED_HZ
 */
#define SPI_SPEED_HZ_SLOW	SPI_SPEED_HZ

#define SPI_CHIP_SELECT		0

#define SPI_BUS_NUM 		1

/* Nanoradio target device needs CLK phase 0 and CLK polarity 0.
 * Normally, this mode of operation is selected when SPI_MODE is set to 0.
 * But on IMX31 ADS platform, SPI_MODE must be set to 2
 * to operate SPI bus with CLK phase 0 and CLK polarity 0.
 */
#define SPI_MODE			2



/*****************/
/* Other options */
/*****************/

/* SPI driver for IMX31 ADS platform does not support DMA. */
//#define USE_DMA

/*
 * On IMX31 ADS platform, the kernel SPI driver works reliably
 * when an spi_transfer structure with .rx_buf == NULL is encountered,
 * so option USE_DUMMY_RX_BUFF is not needed.
 */
//#define USE_DUMMY_RX_BUFF
#define USE_DUMMY_TX_BUFF

/* If not defined, we assume that CMD52 and CMD53 are always successful. */
#define CHECK_SPI

#endif /* _HOST_DEF_H */
