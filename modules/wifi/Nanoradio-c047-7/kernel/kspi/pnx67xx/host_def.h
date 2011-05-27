/**
*
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H

/************************/
/* Target configuration */
/************************/
//#define USE_DVL6A

#define PLAT_VERS_BAGEL		0
#define PLAT_VERS_EUROPA	1

#define PLAT_VERS PLAT_VERS_EUROPA


/*************************/
/* General configuration */
/*************************/
#define USE_FW_DOWNLOAD
#define USE_POWER_MANAGEMENT
//#define JTAG_CONNECTION

#include <linux/spinlock.h>
#include <asm/arch/gpio.h>	/* for GPIO_A30, IRQ_EXTINT */
#define TARGET_IRQ_NUM       IRQ_EXTINT(14)

/***********************************/
/* SPI kernel driver configuration */
/***********************************/
#define SPI_DRIVER_NAME "nano_spi"
#define SPI_SPEED_HZ	26000000
#define SPI_SPEED_HZ_SLOW	SPI_SPEED_HZ
#define SPI_CHIP_SELECT GPIO_A30
#define SPI_BUS_NUM 	1
#define SPI_MODE		3

/*****************/
/* Other options */
/*****************/

/*
 * pnx67xx kernel SPI driver does not comply to API described in
 * include/linux/spi/spi.h
 * If both spi_transfer.rx_buf and spi_transfer.tx_buf are defined,
 * it will "send" and then "receive" on the SPI bus!
 * To get conventional operation, we must not use dummy RX buff,
 * because spi_transfer.rx_buf must be NULL during "send".
 * For the same reason, we can not use option CHECK_SPI.
 * Also, we must not use dummy TX buff, as spi_transfer.tx_buf must be NULL
 * during "receive".
 */
//#define USE_DUMMY_RX_BUFF
//#define USE_DUMMY_TX_BUFF
//#define CHECK_SPI

/* MAX_TRANSFER_SIZE must not exceed 64 with the initial kernel spi
 * driver by NXP. With NXP kernel spi driver debugged, MAX_TRANSFER_SIZE
 * can be the default 512 for optimal performance.
 * #define MAX_TRANSFER_SIZE   64
 */

/* avoid multi xfer read on pnx67xx (causes immediate crash) */
#define PNX67XX

#endif /* _HOST_DEF_H */
