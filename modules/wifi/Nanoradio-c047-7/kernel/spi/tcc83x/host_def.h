/**
*
*   See generic 'host.h' for details
*/
#ifndef _HOST_DEF_H
#define _HOST_DEF_H


#define USE_FW_DOWNLOAD
//#define SPI_IF_TEST
//#define USE_NO_CLOCK
//#define USE_POWER_MANAGEMENT
//#define JTAG_CONNECTION
//#define LGIT_MODULE

#define USE_TRACE
#define USE_PRINTBUF

/* define types of interrupts supported */
//#define USE_IRQ_ID_RESPONSE_COMPLETION
//#define USE_IRQ_ID_FIFO_READ_READY
//#define USE_IRQ_ID_FIFO_WRITE_READY
//#define USE_IRQ_ID_RW_ACCESS_COMPLETION


unsigned int    irq_initialize(void);
void            irq_disable_interrupt(void);
void            irq_enable_interrupt(void);
void            irq_clear_interrupt(void);

void debug_gpio_set (void);
void debug_gpio_clr (void);
void gpio_disable_interrupt(void);
void gpio_enable_interrupt(void);
unsigned int  spi_senddata_nodma(unsigned char* data, unsigned int size);
unsigned int spi_readdata_nodma(unsigned char* data, unsigned int size);
int transport_target_reset(void);
void spi_stop(void);
void spi_start(void);

#define HOST_ATTENTION_SPI     0x5a
#define TARGET_IRQ_NUM       INT_EI7

#endif /* _HOST_DEF_H */
