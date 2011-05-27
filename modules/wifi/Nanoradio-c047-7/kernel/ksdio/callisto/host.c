/* host.c - Tranport driver unsing Linux SDIO stack.
 *
 * This file implements the functions declared in host.h
 * for controlling host GPIO pin connected to NRX600 SHUTDOWN_N pin
 *
 */
 
#include "host.h"
#include <linux/errno.h>
#include <mach/gpio.h>
 
int host_gpio_allocate(int gpio_num)
{
	return 0;
}

int host_gpio_free(int gpio_num)
{
	return 0;
}

int host_gpio_set_level(int gpio_num, int level)
{
   return gpio_direction_output(gpio_num, level);
}
