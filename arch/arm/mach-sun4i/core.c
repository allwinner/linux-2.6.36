#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/amba/pl061.h>
#include <linux/amba/mmci.h>
#include <linux/amba/pl022.h>
#include <linux/io.h>
#include <linux/gfp.h>
#include <linux/clockchips.h>

#include <asm/clkdev.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/icst.h>
#include <asm/hardware/vic.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <mach/clkdev.h>
#include <mach/hardware.h>
#include <mach/platform.h>

#include "core.h"


/**
 * Global vars definitions
 *
 */
static void timer_set_mode(enum clock_event_mode mode, struct clock_event_device *clk)
{
	volatile u32 ctrl;

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		printk("timer_set_mode: periodic\n");
		writel(20, SW_TIMER0_INTVAL_REG); /* interval (999+1) */
		ctrl = readl(SW_TIMER0_CTL_REG);
		ctrl &= ~(1<<7);    //continuous mode
                ctrl |= 1;  //enable
                break;

	case CLOCK_EVT_MODE_ONESHOT:
		printk("timer_set_mode: oneshot\n");
		ctrl = readl(SW_TIMER0_CTL_REG);
		ctrl |= (1<<7);     //single mode
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		ctrl = readl(SW_TIMER0_CTL_REG);
		ctrl &= ~(1<<0);    //disable timer 0
		break;
	}

	writel(ctrl, SW_TIMER0_CTL_REG);
}

static int timer_set_next_event(unsigned long evt, struct clock_event_device *unused)
{
	volatile u32 ctrl;

	/* clear any pending before continue */
	ctrl = readl(SW_TIMER0_CTL_REG);
	writel(evt, SW_TIMER0_CNTVAL_REG);
    ctrl |= (1<<1);
	writel(ctrl, SW_TIMER0_CTL_REG);
	writel(ctrl | 0x1, SW_TIMER0_CTL_REG);

	return 0;
}

static struct clock_event_device timer0_clockevent = {
	.name = "timer0",
	.shift = 32,
	.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode = timer_set_mode,
	.set_next_event = timer_set_next_event,
};


static irqreturn_t softwinner_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &timer0_clockevent;

	writel(0x1, SW_TIMER_INT_STA_REG);

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction softwinner_timer_irq = {
	.name = "Softwinner Timer Tick",
	.flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler = softwinner_timer_interrupt,
};

void softwinner_irq_ack(unsigned int irq)
{
    if (irq < 32) {
        writel(readl(SW_INT_ENABLE_REG0) & ~(1<<irq), SW_INT_ENABLE_REG0);
        writel(readl(SW_INT_MASK_REG0) | (1 << irq), SW_INT_MASK_REG0);
        writel(readl(SW_INT_IRQ_PENDING_REG0) | (1<<irq), SW_INT_IRQ_PENDING_REG0);
    } else if(irq < 64) {
        irq -= 32;
        writel(readl(SW_INT_ENABLE_REG1) & ~(1<<irq), SW_INT_ENABLE_REG1);
        writel(readl(SW_INT_MASK_REG1) | (1 << irq), SW_INT_MASK_REG1);
        writel(readl(SW_INT_IRQ_PENDING_REG1) | (1<<irq), SW_INT_IRQ_PENDING_REG1);
    }else if(irq < 96){
        irq -= 64;
        writel(readl(SW_INT_ENABLE_REG2) & ~(1<<irq), SW_INT_ENABLE_REG2);
        writel(readl(SW_INT_MASK_REG2) | (1 << irq), SW_INT_MASK_REG2);
        writel(readl(SW_INT_IRQ_PENDING_REG2) | (1<<irq), SW_INT_IRQ_PENDING_REG2);
    }
}

/* Disable irq */
static void softwinner_irq_mask(unsigned int irq)
{
    if(irq < 32){
        writel(readl(SW_INT_ENABLE_REG0) & ~(1<<irq), SW_INT_ENABLE_REG0);
        writel(readl(SW_INT_MASK_REG0) | (1 << irq), SW_INT_MASK_REG0);
    } else if(irq < 64) {
        irq -= 32;
        writel(readl(SW_INT_ENABLE_REG1) & ~(1<<irq), SW_INT_ENABLE_REG1);
        writel(readl(SW_INT_MASK_REG1) | (1 << irq), SW_INT_MASK_REG1);
    }else if(irq < 96){
        irq -= 64;
        writel(readl(SW_INT_ENABLE_REG2) & ~(1<<irq), SW_INT_ENABLE_REG2);
        writel(readl(SW_INT_MASK_REG2) | (1 << irq), SW_INT_MASK_REG2);
    }
}

/* Enable irq */
static void softwinner_irq_unmask(unsigned int irq)
{
    if(irq < 32){
        writel(readl(SW_INT_ENABLE_REG0) | (1<<irq), SW_INT_ENABLE_REG0);
        writel(readl(SW_INT_MASK_REG0) & ~(1 << irq), SW_INT_MASK_REG0);
        if(irq == SW_INT_IRQNO_ENMI) /* must clear pending bit when enabled */
            writel((1 << SW_INT_IRQNO_ENMI), SW_INT_IRQ_PENDING_REG0);
    } else if(irq < 64){
        irq -= 32;
        writel(readl(SW_INT_ENABLE_REG1) | (1<<irq), SW_INT_ENABLE_REG1);
        writel(readl(SW_INT_MASK_REG1) & ~(1 << irq), SW_INT_MASK_REG1);
    }else if(irq < 96){
        irq -= 64;
        writel(readl(SW_INT_ENABLE_REG2) | (1<<irq), SW_INT_ENABLE_REG2);
        writel(readl(SW_INT_MASK_REG2) & ~(1 << irq), SW_INT_MASK_REG2);
    }
}

static struct irq_chip sw_f23_sic_chip = {
    .name   = "SW_F23_SIC",
    .ack = softwinner_irq_ack,
	.mask = softwinner_irq_mask,
	.unmask = softwinner_irq_unmask,
};

void __init softwinner_init_irq(void)
{
    u32 i = 0;

    /* Disable & clear all interrupts */
    writel(0, SW_INT_ENABLE_REG0);
    writel(0, SW_INT_ENABLE_REG1);
    writel(0, SW_INT_ENABLE_REG2);

    writel(0xffffffff, SW_INT_MASK_REG0);
    writel(0xffffffff, SW_INT_MASK_REG1);
    writel(0xffffffff, SW_INT_MASK_REG2);

    writel(0xffffffff, SW_INT_IRQ_PENDING_REG0);
    writel(0xffffffff, SW_INT_IRQ_PENDING_REG1);
    writel(0xffffffff, SW_INT_IRQ_PENDING_REG2);
    writel(0xffffffff, SW_INT_FIQ_PENDING_REG0);
    writel(0xffffffff, SW_INT_FIQ_PENDING_REG1);
    writel(0xffffffff, SW_INT_FIQ_PENDING_REG2);

    /*enable protection mode*/
    writel(0x01, SW_INT_PROTECTION_REG);
    /*config the external interrupt source type*/
    writel(0x00, SW_INT_NMI_CTRL_REG);

    for (i = SW_INT_START; i < SW_INT_END; i++) {
        set_irq_chip(i, &sw_f23_sic_chip);
        set_irq_handler(i, handle_level_irq);
        set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
    }
}

static struct map_desc softwinner_io_desc[] __initdata = {
	{ SW_VA_SRAM_BASE,	__phys_to_pfn(SW_PA_SRAM_BASE),	SZ_16K,		MT_MEMORY_ITCM},
	{SW_VA_CCM_IO_BASE,	__phys_to_pfn(SW_PA_CCM_IO_BASE),	SZ_1K,		MT_DEVICE},
	{ SW_VA_SRAM_IO_BASE,        __phys_to_pfn(SW_PA_SRAM_IO_BASE),           SZ_4K,     MT_DEVICE },
	{ SW_VA_DRAM_IO_BASE,        __phys_to_pfn(SW_PA_DRAM_IO_BASE),           SZ_4K,     MT_DEVICE },
	{ SW_VA_DMAC_IO_BASE,        __phys_to_pfn(SW_PA_DMAC_IO_BASE),           SZ_4K,     MT_DEVICE },
	{ SW_VA_NANDFLASHC_IO_BASE,  __phys_to_pfn(SW_PA_NANDFLASHC_IO_BASE),     SZ_4K,     MT_DEVICE },
	{ SW_VA_INT_IO_BASE,         __phys_to_pfn(SW_PA_INT_IO_BASE),            SZ_1K,     MT_DEVICE },
	{ SW_VA_PORTC_IO_BASE,       __phys_to_pfn(SW_PA_PORTC_IO_BASE),          SZ_1K,     MT_DEVICE },
	{ SW_VA_TIMERC_IO_BASE,      __phys_to_pfn(SW_PA_TIMERC_IO_BASE),         SZ_1K,     MT_DEVICE },
	{ SW_VA_UART0_IO_BASE,       __phys_to_pfn(SW_PA_UART0_IO_BASE),          SZ_1K,     MT_DEVICE },
};

void __init softwinner_map_io(void)
{
	iotable_init(softwinner_io_desc, ARRAY_SIZE(softwinner_io_desc));
}

struct sysdev_class sw_sysclass = {
	.name = "sw-core",
};

static struct sys_device sw_sysdev = {
	.cls = &sw_sysclass,
};

static int __init sw_core_init(void)
{
        return sysdev_class_register(&sw_sysclass);
}
core_initcall(sw_core_init);


extern int sw_register_clocks(void);
void __init softwinner_init(void)
{
	sysdev_register(&sw_sysdev);
}

static void __init softwinner_timer_init(void)
{
        volatile u32  val = 0;

        writel(20, SW_TIMER0_INTVAL_REG);
        val = 0;
        writel(val, SW_TIMER0_CTL_REG);
        val = readl(SW_TIMER0_CTL_REG); /* 2KHz */
        val |= (4 << 4);                /* LOSC 16 division */
        writel(val, SW_TIMER0_CTL_REG);
        val = readl(SW_TIMER0_CTL_REG);
        val |= 2;                       /* auto reload      */
        writel(val, SW_TIMER0_CTL_REG);
        setup_irq(SW_INT_IRQNO_TIMER0, &softwinner_timer_irq);

	    /* Enable time0 interrupt */
        val = readl(SW_TIMER_INT_CTL_REG);
        val |= (1<<0);
        writel(val, SW_TIMER_INT_CTL_REG);

        timer0_clockevent.mult = div_sc(2048, NSEC_PER_SEC, timer0_clockevent.shift);
        timer0_clockevent.max_delta_ns = clockevent_delta2ns(0xff, &timer0_clockevent);
        timer0_clockevent.min_delta_ns = clockevent_delta2ns(0x1, &timer0_clockevent);

        timer0_clockevent.cpumask = cpumask_of(0);
        clockevents_register_device(&timer0_clockevent);
}

struct sys_timer softwinner_timer = {
	.init		= softwinner_timer_init,
};

MACHINE_START(SUN4I, "sun4i")
        /* Maintainer: ARM Ltd/Deep Blue Solutions Ltd */
        .phys_io        = 0x01c00000,
        .io_pg_offst    = ((0xf1c00000) >> 18) & 0xfffc,
        .map_io         = softwinner_map_io,
        .init_irq       = softwinner_init_irq,
        .timer          = &softwinner_timer,
        .init_machine   = softwinner_init,
        .boot_params = (unsigned long)(0x40000100 + 0x8000),
MACHINE_END


