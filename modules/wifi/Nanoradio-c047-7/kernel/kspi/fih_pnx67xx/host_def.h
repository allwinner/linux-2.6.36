/**
*
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H

#include <linux/spinlock.h>
#include <asm/arch/gpio.h>	/* for GPIO_xxx, IRQ_EXTINT */

/**********************/
/* Host configuration */
/**********************/
//#define USE_DVL6A

#define PLAT_VERS_AU3_PR2	0
#define PLAT_VERS_K2_PCR	1

#define PLAT_VERS PLAT_VERS_K2_PCR

#ifndef PLAT_VERS
#error  "Please define platform version (PLAT_VERS)"
#endif /* PLAT_VERS */

#if PLAT_VERS > PLAT_VERS_AU3_PR2 && defined(USE_DVL6A)
#error  "Support for DVL6a is only for platform AU3/PR2"
#endif /* PLAT_VERS > PLAT_VERS_AU3_PR2 && defined(USE_DVL6A) */

#if PLAT_VERS == PLAT_VERS_AU3_PR2
#define EXT_IRQ_NUM 14
#define PIN_IRQ		GPIO_A20
#define PIN_CS		GPIO_A13
#define PIN_RESET	GPIO_B14

#elif PLAT_VERS == PLAT_VERS_K2_PCR
#define EXT_IRQ_NUM	7
#define PIN_IRQ		GPIO_A13
#define PIN_CS		GPIO_A20
#define PIN_RESET	GPIO_C18
#define PIN_POWER	GPIO_B4

#else /* PLAT_VERS unknown value */
#error "Unsupported platform version (PLAT_VERS)"
#endif /* PLAT_VERS */


/*************************/
/* General configuration */
/*************************/
#define USE_FW_DOWNLOAD
#define USE_POWER_MANAGEMENT
//#define JTAG_CONNECTION

#define USE_TARGET_PARAMS

#define TARGET_IRQ_NUM	IRQ_EXTINT(EXT_IRQ_NUM)

/***********************************/
/* SPI kernel driver configuration */
/***********************************/
#define SPI_DRIVER_NAME "spi_nano"
#define SPI_SPEED_HZ	26000000
#define SPI_SPEED_HZ_SLOW	SPI_SPEED_HZ
#define SPI_CHIP_SELECT PIN_CS
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

#endif /* _HOST_DEF_H */
