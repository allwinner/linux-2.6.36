#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/clk.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#include "host.h"
#include "nanoutil.h"

/* Sanity checks */
#ifndef PLAT_VERS
#error  "Please define platform version (PLAT_VERS)"
#endif

#if PLAT_VERS == PLAT_VERS_EUROPA && defined(USE_DVL6A)
#error  "Platform version Europa does not currently support DVL6a"
#endif


/* Hardware topology definitions */
#if PLAT_VERS == PLAT_VERS_BAGEL
#define PIN_MISO	GPIO_A31
#define PIN_MOSI	GPIO_A28
#define PIN_CLK		GPIO_A27
#define PIN_POWER	GPIO_A13
#define PIN_RESET	GPIO_D16
#define PIN_MUX_POWER	GPIO_MODE_MUX0
#define PIN_MUX_RESET	GPIO_MODE_MUX1

#elif PLAT_VERS == PLAT_VERS_EUROPA
/* With Europa platform, we are not interested about MISO, MOSI, CLK
 * because kernel spi driver can allocate them correctly after DVL6a.
 */
#define PIN_POWER	GPIO_F3
#define PIN_RESET	GPIO_A26
#define PIN_MUX_POWER	GPIO_MODE_MUX1
#define PIN_MUX_RESET	GPIO_MODE_MUX0

#else /* PLAT_VERS */
#error "Unsupported platform version (PLAT_VERS)"
#endif /* PLAT_VERS */


static int get_gpio(int gpio_num, int gpio_mode, int gpio_direction)
{
	int status;

	status = pnx_request_gpio(gpio_num);
	if (status) {
		TRSP_MSG("Failed to allocate pin %d (status = %d)\n", gpio_num, status);
		return status;
	}

	pnx_set_gpio_mode(gpio_num, gpio_mode);
	pnx_set_gpio_direction(gpio_num, gpio_direction);
	return 0;
}

int host_init(void)
{
	int status;

	KDEBUG(TRANSPORT, "ENTRY");

#ifdef USE_DVL6A
	/* With DVL6a, GPIO pins are not allocated for SPI by the kernel driver. */
	status  = get_gpio(PIN_CLK, GPIO_MODE_MUX1, GPIO_DIR_OUTPUT);
	status |= get_gpio(PIN_MOSI, GPIO_MODE_MUX1, GPIO_DIR_OUTPUT);
	status |= get_gpio(PIN_MISO, GPIO_MODE_MUX2, GPIO_DIR_INPUT);
	if (status)
		return -EIO;
#endif /* USE_DVL6A */

	/* Allocate interrupt GPIO */
	status = pnx_request_gpio(EXTINT_TO_GPIO(TARGET_IRQ_NUM));
	if (status) {
		TRSP_MSG("Failed to allocate interrupt pin (status = %d)\n", status);
		return status;
	}

	pnx_set_gpio_direction(EXTINT_TO_GPIO(TARGET_IRQ_NUM), GPIO_DIR_INPUT);
	/* phth: pnx_set_gpio_debounce(TARGET_IRQ_NUM, 0); */
	set_irq_type(TARGET_IRQ_NUM, IRQ_TYPE_EDGE_BOTH);

	host_interrupt(IRQ_OP_ACKNOWLEDGE);
	
	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}

int host_interrupt(irq_op_t irq_op)
{
	unsigned int reg;
	struct irq_chip* chip_info;

	switch (irq_op) {
		case IRQ_OP_ENABLE:
			enable_irq(TARGET_IRQ_NUM);
			break;

		case IRQ_OP_DISABLE:
			disable_irq(TARGET_IRQ_NUM);
			break;

		case IRQ_OP_STATUS:
			reg = irq_desc[TARGET_IRQ_NUM].status;
			return (reg & IRQ_DISABLED) ? 0 : 1;

		case IRQ_OP_ACKNOWLEDGE :
			chip_info = get_irq_chip(TARGET_IRQ_NUM);
			chip_info->ack(TARGET_IRQ_NUM);
			break;

		default:
			BUG();
	}
	return 0;
}

void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
	int gpio_num, gpio_mode;
	int status;

	/* Translate to gpio_id to gpio_num */
	switch (gpio_id)
	{
		case GPIO_ID_CPU_LOAD: /* Ignore for now */
			return;

		case GPIO_ID_POWER:
			gpio_num = PIN_POWER;
			gpio_mode = PIN_MUX_POWER;
			break;

		case GPIO_ID_RESET:
			gpio_num = PIN_RESET;
			gpio_mode = PIN_MUX_RESET;
			break;
			
		default:
			BUG();
	}

	switch (gpio_op) {
		case GPIO_OP_ENABLE:
			status = get_gpio(gpio_num, gpio_mode, GPIO_DIR_OUTPUT);
			TRSP_ASSERT(status == 0);
			break;

		case GPIO_OP_DISABLE:
			pnx_free_gpio(gpio_num);
			break;

		case GPIO_OP_HIGH:
		case GPIO_OP_LOW:
			pnx_write_gpio_pin(gpio_num, (gpio_op == GPIO_OP_HIGH) ? 1 : 0);
			break;

		default:
			BUG();
	}
}

void host_exit(void)
{
	pnx_free_gpio(EXTINT_TO_GPIO(TARGET_IRQ_NUM));
#ifdef USE_DVL6A
	pnx_free_gpio(PIN_CLK);
	pnx_free_gpio(PIN_MOSI);
	pnx_free_gpio(PIN_MISO);
#endif /* USE_DVL6A */
}

