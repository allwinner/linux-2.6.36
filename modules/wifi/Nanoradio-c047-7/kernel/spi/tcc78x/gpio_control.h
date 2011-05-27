/* $Id: gpio_control.h 9119 2008-06-09 11:50:52Z joda $ */
/*****************************************************************************

Copyright (c) 2006 by Nanoradio AB

This software is copyrighted by and is the sole property of Nanoradio AB.
All rights, title, ownership, or other interests in the
software remain the property of Nanoradio AB.  This software may
only be used in accordance with the corresponding license agreement.  Any
unauthorized use, duplication, transmission, distribution, or disclosure of
this software is expressly forbidden.

This Copyright notice may not be removed or modified without prior written
consent of Nanoradio AB.

Nanoradio AB reserves the right to modify this software without
notice.

Nanoradio AB
Torshamnsgatan 39                       info@nanoradio.se
164 40 Kista                            http://www.nanoradio.se
SWEDEN

Module Description :
==================
Contains host-specific GPIO functions

*****************************************************************************/

#ifndef GPIO_CONTROL_H_
#define GPIO_CONTROL_H_

//
// Insert Host-specific includes here
//
#include <tcc78x_ioport.h>
#include <asm/arch/tcc78x.h>

//
// Host-specific globals here
//
extern volatile TCC78X_IOPORT_REG *g_pGPIORegs;

//Prepare everything that is needed to prepare in
//order to use the rest of the functions
unsigned int gpio_initialize(void);

//Free all resources allocated by GPIO_Initialize
void gpio_deinitialize(void);

//Setting Pin levels
void gpio_set_digital_reset_pinHigh(void);
void gpio_set_digital_reset_pinLow(void);
void gpio_set_analog_reset_pinHigh(void);
void gpio_set_analog_reset_pinLow(void);
void gpio_force_spi_cs_pinHigh(void);
void gpio_spi_cs_restore(void);
void gpio_set_cs_pinLow(void);
void gpio_set_cs_pinHigh(void);
void gpio_turn_on_target_power(void);
void gpio_turn_off_target_power(void);
void gpio_enable_interrupt(void);
void gpio_disable_interrupt(void);
unsigned int gpio_get_interrupt_pinState(void);

#endif //GPIO_CONTROL_H_
