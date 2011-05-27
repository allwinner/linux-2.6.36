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

#ifdef USE_DVL6A
int get_gpio(int gpio_num, int is_output)
{
	int status;

	status = pnx_request_gpio(gpio_num);
	if (status) {
		TRSP_MSG("Failed to allocate pin %d (status = %d)\n", gpio_num, status);
		return status;
	}

	pnx_set_gpio_mode(gpio_num,
						is_output ? GPIO_MODE_MUX1 : GPIO_MODE_MUX2);
	pnx_set_gpio_direction(gpio_num,
						is_output ? GPIO_DIR_OUTPUT : GPIO_DIR_INPUT);
	return 0;
}
#endif /* USE_DVL6A */

int host_init(void)
{
	int status;

	KDEBUG(TRANSPORT, "ENTRY");

#ifdef USE_DVL6A
	/* With DVL6a, GPIO pins are not allocated for SPI by the kernel driver. */
	status  = get_gpio(GPIO_A27, 1);
	status |= get_gpio(GPIO_A28, 1);
	status |= get_gpio(GPIO_A31, 0);
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
#if PLAT_VERS == PLAT_VERS_AU3_PR2	
	pnx_set_gpio_debounce(TARGET_IRQ_NUM, 0);
#elif PLAT_VERS == PLAT_VERS_K2_PCR
	pnx_set_gpio_irq_debounce(TARGET_IRQ_NUM, 0);
#endif /* PLAT_VERS */
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
	int gpio_num;
	int status;

	/* Translate to gpio_id to gpio_num */
	switch (gpio_id)
	{
		case GPIO_ID_CPU_LOAD:
			return;	/* Ignore for now */

		case GPIO_ID_POWER:
#if PLAT_VERS == PLAT_VERS_AU3_PR2
			/* AU3 platform does not support power control */
			return;
#elif PLAT_VERS == PLAT_VERS_K2_PCR
			gpio_num = PIN_POWER;
			break;
#endif

		case GPIO_ID_RESET:
			gpio_num = PIN_RESET;
			break;
			
		default:
			BUG();
	}

	switch (gpio_op) {
		case GPIO_OP_ENABLE:
			status = pnx_request_gpio(gpio_num);
			if (status) {
				TRSP_ASSERT(0);
				return;
			}
			pnx_set_gpio_mode(gpio_num, GPIO_MODE_MUX1);
			pnx_set_gpio_direction(gpio_num, GPIO_DIR_OUTPUT);
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
	pnx_free_gpio(GPIO_A27);
	pnx_free_gpio(GPIO_A28);
	pnx_free_gpio(GPIO_A31);
#endif /* USE_DVL6A */
}

/* MIB values for gpioActiveMode */
#define MIB_INPUT   0x00000000 /* first word from mib */
#define MIB_OUTPUT  0x80800050 /* second word from mib */
#define MIB_HIGH    0x00000000 /* third word from mib */
#define MIB_DISABLE 0x7e7fffbf

#define NR_LE_BYTES4(X) (((X) >> 0) & 0xff), (((X) >> 8) & 0xff), \
						(((X) >> 16) & 0xff), (((X) >> 24) & 0xff)

static const unsigned char target_params[] = {
     0x87, 0x01,  // Magic number
     
     // 0x80=MIB, 0x6a = len
     0x80, 0x6a,  

     // Binary from Nanoloader.
     0xfe, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 

     /* 5.26.1 gpioActiveMode */
     0x1c, 0x00, 
     0x05, 0x1a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
     NR_LE_BYTES4(MIB_INPUT),
     NR_LE_BYTES4(MIB_OUTPUT),
     NR_LE_BYTES4(MIB_HIGH),
     NR_LE_BYTES4(MIB_DISABLE),
     0x01, 0x02, 0x00, 0x00, 
     
     /* 5.26.2 gpioPSMode */
     0x1c, 0x00,
     0x05, 0x1a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,  
     0x0e, 0x80, 0x00, 0x00, 
     0xd1, 0x40, 0xcf, 0xfd, 
     0x00, 0x40, 0x83, 0x00, 
     0x00, 0x00, 0x00, 0x00,      
     0x01, 0x02, 0x00, 0x00, 

     /* 5.26.3 pinShutdownMode */
     0x14, 0x00, 
     0x05, 0x1a, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
     0xf8, 0xc8, 0xdb, 0x6d, 
     0x00, 0xc0, 0x8b, 0x00, 
     0x02, 0x00, 0x00, 0x00, 

     /* 5.7.3 HFC_Boost_XCO_RTRIM */
     0x0c, 0x00,
     0x05, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x20, 0x00, /* HFC boost time */
     0x00, 0x00, /* XCO Rtrim */
     
     0xff, 0xff,
     
     /* bt coex */
     0x81, 0x09,
     0x01, /* enable/disable */
     0x02, /* BT vendor */
     0x01, /* PTA mode */
     0x11, 0x10, 0x12, 0x13, 0x00, /* PTA conf */
     0x0a, /* antenna */

     0x78
};

#ifdef USE_TARGET_PARAMS
size_t host_get_target_params(const void** params_buf)
{
	*params_buf = target_params;
	return sizeof(target_params);
}
#else
#warning "NRG800 needs configuration parameters!"
#endif
