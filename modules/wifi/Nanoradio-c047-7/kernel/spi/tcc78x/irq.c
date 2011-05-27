/* $Id: irq.c 9119 2008-06-09 11:50:52Z joda $ */
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

#include <linux/types.h>
#include <linux/mm.h>
#include <irq_control.h>

//
// Insert Host-specific includes here
//
#include <asm/arch/tcc78x.h>
#include <tcc78x_intr.h>
#include <tcc78x_base_regs.h>

typedef enum {false, true} __attribute__ ((packed)) boolean;
//
// Insert Host-specific variables here
//
static volatile TCC78X_INTR_REG *s_pIntrRegs = NULL;

static unsigned int irq_map_registers(void);
static void         irq_unmap_registers(void);

unsigned int irq_initialize()
{
    if(s_pIntrRegs)
        return false;

    while(true)
    {
        if(!irq_map_registers())
            break;

        irq_disable_interrupt();

        // Interrupt Select Reg : Set EINT3 as IRQ
        s_pIntrRegs->SEL |= HwSEL_EI3;

        // Intr Mode Register : Set EI3 as edge-trigger
        s_pIntrRegs->MODE &= ~HwMODE_EI3;

        // Intr Mode Register : Set EI3 as both edges
        s_pIntrRegs->MODEA |= HwMODEA_EI3;

        irq_clear_interrupt();

        return true;
    }

    irq_deinitialize();
    return false;
}
void irq_deinitialize()
{
    irq_disable_interrupt();
    irq_unmap_registers();
}

void irq_disable_interrupt()
{
    s_pIntrRegs->IEN &= ~HwIEN_EI3;
    s_pIntrRegs->INTMSK &= ~HwIRQMSK_EI3;
}

void irq_enable_interrupt()
{
    s_pIntrRegs->IEN |= HwIEN_EI3;
    s_pIntrRegs->INTMSK |= HwIRQMSK_EI3;
}

void irq_clear_interrupt()
{
    s_pIntrRegs->CLR |= HwIEN_EI3;
}

static unsigned int irq_map_registers()
{
    //checks if the registers are allready maped
    if(s_pIntrRegs)
        return false;

    /* Physical memory is already mapped. Phys Address == Virt Address */
    s_pIntrRegs = (volatile TCC78X_INTR_REG *) (TCC78X_BASE_REG_PA_INTR);
    if (!s_pIntrRegs) {
        return false;
    }
    return true;
}

static void irq_unmap_registers()
{
    if (s_pIntrRegs) {
        s_pIntrRegs = 0;
    }
}
