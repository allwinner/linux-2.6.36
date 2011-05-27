#include "host.h"

#include <asm/io.h>
#include <asm/arch/gpio.h>
#include "imx31/mxc_spi.h"
#include "imx31/iomux.h"

#include "nanoutil.h"
#include "macro.h"

#define IMX31_SPI_MAGIC_VERSION "nanoradio wifi driver for spi linux"

/* Base address to GPIO registers */
unsigned long IMX31_GPIO_BASE = 0;

/*!
 * Requests GPIO for interrupts.
 *
 * @return 	Returns 0 on SUCCESS and error on FAILURE.
 */
static inline int interrupt_gpio_setup(void)
{
	int status;
#ifndef USE_SDIO_IRQ
	unsigned int dr, psr;
#endif

	status = mxc_request_iomux(MX31_PIN_GPIO1_6, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	if (status) {
		TRSP_MSG("mxc_request_iomux failed, status = %d"
				" - GPIO already configured?\n", status);
		return status;
	}

	// Disable interrupt from target
	and_gpio_32(GPIO_IMR,~GPIO_MASK);

#ifdef USE_SDIO_IRQ

/// 11 fall-edge sensitive
	or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_LBIT);
	or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_HBIT);

#else /* ! USE_SDIO_IRQ */

	// read the data from the bus
	dr = read_gpio_32(GPIO_DR);
	psr = read_gpio_32(GPIO_PSR);

	// must have the same value
	TRSP_ASSERT((dr == psr));

	// check the state of the GPIO
	// and change the interrupt 
	if(dr & GPIO_MASK)
	{
		/// 11 fall-edge sensitive
		or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_LBIT);
		or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_HBIT);
	}
	else
	{
		/// 10 rise-edge sensitive
		and_gpio_32(GPIO_ICR1,~GPIO_IRQ_MASK_LBIT);
		or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_HBIT);
	}

#endif /* USE_SDIO_IRQ */
	  
	return status;
}

/*!
 * Releases GPIO for interrupts.
 */
static inline void interrupt_gpio_free(void)
{
	// Disable interrupt from target
	and_gpio_32(GPIO_IMR,~GPIO_MASK);

	mxc_free_iomux(MX31_PIN_GPIO1_6, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
}

/* See host.h */
int host_init(void)
{
	int status;

	KDEBUG(TRANSPORT, "ENTRY");
	printk("Driver's Magic Version : %s\n", IMX31_SPI_MAGIC_VERSION);

    IMX31_GPIO_BASE = (unsigned long) ioremap (0x53FCC000, 0x18);
	if(!IMX31_GPIO_BASE) {
		TRSP_MSG("ioremap for IMX31_GPIO_BASE failed\n");
		return -EPERM;
	}

	status = interrupt_gpio_setup();
 
	KDEBUG(TRANSPORT, "EXIT");
	return status;
}

/* See host.h */
int host_interrupt(irq_op_t irq_op)
{
#ifndef USE_SDIO_IRQ
	volatile unsigned int dr=0;
	volatile unsigned int psr = 0;
#endif /* USE_SDIO_IRQ */
	volatile unsigned int irq_status=0;
	
	 switch (irq_op) {
        case IRQ_OP_ENABLE:
            // Enable interrupt from target
			or_gpio_32(GPIO_IMR,GPIO_MASK);
            break;

        case IRQ_OP_DISABLE:
            // Disable interrupt from target
			and_gpio_32(GPIO_IMR,~GPIO_MASK);

#ifndef USE_SDIO_IRQ
			// read the data from the bus
			dr = read_gpio_32(GPIO_DR);
			psr = read_gpio_32(GPIO_PSR);
			
			// must have the same value
			TRSP_ASSERT((dr == psr));
			
			// check the state of the GPIO
			// and change the interrupt 
			if(psr & GPIO_MASK) 
			{
				/// 11 fall-edge sensitive
				or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_LBIT);
				or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_HBIT);
			}
			else
			{
				/// 10 rise-edge sensitive
				and_gpio_32(GPIO_ICR1,~GPIO_IRQ_MASK_LBIT);
				or_gpio_32(GPIO_ICR1,GPIO_IRQ_MASK_HBIT);
			}
#endif /* USE_SDIO_IRQ */
            break;

        case IRQ_OP_STATUS:
			irq_status = read_32(GPIO_ISR) ;
            return (irq_status & GPIO_MASK);

        case IRQ_OP_ACKNOWLEDGE:
            // Acknowledge interrupt from SPI target
			or_gpio_32(GPIO_ISR,GPIO_MASK);
            break;

         default:
            // Unknown operation – this should never happen
            BUG();
    }
    return 0;
}

/* See host.h */
void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
}

/* See host.h */
void host_exit(void)
{
	KDEBUG(TRANSPORT, "ENTRY");
	interrupt_gpio_free();
	KDEBUG(TRANSPORT, "EXIT");
}
