/**
*
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H

#define USE_FW_DOWNLOAD
#define USE_NO_CLOCK
#define USE_POWER_MANAGEMENT

#define USE_CMD3_TEST

#define USE_DMA
#define USE_NO_CACHE_COHERENCY /* Buffer address mapping is done at host.c */

/* define types of interrupts supported */
//#define USE_IRQ_ID_RESPONSE_COMPLETION
//#define USE_IRQ_ID_FIFO_READ_READY
//#define USE_IRQ_ID_FIFO_WRITE_READY
#define USE_IRQ_ID_RW_ACCESS_COMPLETION

#define HOST_SDIO_IRQ_NUM 57

#endif /* _HOST_DEF_H */
