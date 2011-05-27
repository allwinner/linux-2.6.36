/**
*   $Id: host_def.h 14588 2010-03-14 11:54:16Z phth $
*
*   Host define for Neomagic MM6+
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H

#define USE_FW_DOWNLOAD
#define USE_NO_CLOCK
#define USE_CMD3_TEST
#define USE_POWER_MANAGEMENT

/* define types of interrupts supported */
#define USE_IRQ_ID_RESPONSE_COMPLETION
//#define USE_IRQ_ID_FIFO_READ_READY
//#define USE_IRQ_ID_FIFO_WRITE_READY
#define USE_IRQ_ID_RW_ACCESS_COMPLETION

/* IRQ defines */
#if defined (SDIO_SLOT_A) || defined (SDIO_BUILTIN)
#define HOST_SDIO_IRQ_NUM       26
#else
#define HOST_SDIO_IRQ_NUM       30
#endif

#endif /* _HOST_DEF_H */
