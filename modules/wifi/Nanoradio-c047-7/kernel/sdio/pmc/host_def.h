
#define USE_POWER_MANAGEMENT
#define USE_NO_CLOCK

#if 0
 #define USE_GPIO_CPU_MEASURE
#endif

#define USE_IRQ_ID_RESPONSE_COMPLETION


/* DMA Seems to be stable without this one
 * Note that we'll always wait for FIFO READ READY
 * in case of CPU transfers. See host.c.
 */
#if 0
 #define USE_IRQ_ID_FIFO_READ_READY
#endif

/* DMA Seems to be stable without this one.
 * If we start getting tx timeouts (has not happend yet),
 * then try again with this interrupt enabled.
 *
 * Note that we'll always wait for FIFO WRITE READY
 * in case of CPU transfers. See host.c.
 */
#if 0
 #define USE_IRQ_ID_FIFO_WRITE_READY
#endif

/* If this one is not defined we'll eventually lose data confirm which
 * will lead to tx timeouts (after some hours of throughput testing).
 */
#define USE_IRQ_ID_FIFO_WRITE_READY
#define USE_IRQ_ID_RW_ACCESS_COMPLETION

#define HOST_SDIO_IRQ_NUM 58
