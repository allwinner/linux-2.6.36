/* $Id: irq_control.h 9119 2008-06-09 11:50:52Z joda $ */
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

*****************************************************************************/

#ifndef IRQ_CONTROL_H_
#define IRQ_CONTROL_H_

unsigned int    irq_initialize(void);
void            irq_deinitialize(void);
void            irq_disable_interrupt(void);
void            irq_enable_interrupt(void);
void            irq_clear_interrupt(void);

#endif //IRQ_CONTROL_H_
