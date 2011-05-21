/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : aw_clocksrc.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-10 10:55
* Descript: clock source driver for aw chips
* Update  : date                auther      ver     notes
*
*********************************************************************************************************
*/
#include <mach/hardware.h>
#include <mach/platform.h>
#include <linux/init.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include "aw_clocksrc.h"

#undef kevin_dbg
#if (0)
    #define kevin_dbg       printk
#else
    #define kevin_dbg(...)
#endif


static cycle_t aw_clksrc_read(struct clocksource *cs);
static irqreturn_t aw_clkevt_irq(int irq, void *handle);
static int aw_set_next_clkevt(unsigned long delta, struct clock_event_device *dev);
static void aw_set_clkevt_mode(enum clock_event_mode mode, struct clock_event_device *dev);


static struct clocksource aw_clocksrc =
{
    .name = "aw 64bits couter",
    .list = {NULL, NULL},
    .rating = 300,                  /* perfect clock source             */
    .read = aw_clksrc_read,         /* read clock counter               */
    .enable = 0,                    /* not define                       */
    .disable = 0,                   /* not define                       */
    .mask = CLOCKSOURCE_MASK(64),   /* 64bits mask                      */
    .mult = 0,                      /* it will be calculated by shift   */
    .shift = 10,                    /* 32bit shift for                  */
    .max_idle_ns = 1000000000000ULL,
    .flags = CLOCK_SOURCE_IS_CONTINUOUS,
};


static struct clock_event_device aw_clock_event =
{
    .name = "aw clock event device",
    .features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
    .max_delta_ns = 100000000000ULL,
    .min_delta_ns = (1000000000 + AW_HPET_CLOCK_EVENT_HZ - 1) / AW_HPET_CLOCK_EVENT_HZ,
    .mult = 100,                    /* will be calculate when init      */
    .shift = 32,
    .rating = 300,                  /* clock event is perfect           */
    .irq = SW_INT_IRQNO_TIMER1,
    .cpumask = 0,                   /* will be set when init            */
    .set_next_event = aw_set_next_clkevt,
    .set_mode = aw_set_clkevt_mode, /* set clock event mode             */
    .event_handler = 0,             /* be alloced by system framework   */
};


static struct irqaction aw_clkevt_irqact =
{
    .handler = aw_clkevt_irq,
    .flags = IRQF_TIMER | IRQF_DISABLED,
    .name = "aw clock event irq",
    .dev_id = &aw_clock_event,
    .irq = SW_INT_IRQNO_TIMER1,
};


/*
*********************************************************************************************************
*                           aw_clksrc_read
*
*Description: read cycle count of the clock source;
*
*Arguments  : cs    clock source handle.
*
*Return     : cycle count;
*
*Notes      :
*
*********************************************************************************************************
*/
static cycle_t aw_clksrc_read(struct clocksource *cs)
{
    unsigned long   flags;
    __u32           lower, upper;

    /* disable interrupt response */
    raw_local_irq_save(flags);

    /* latch 64bit counter and wait ready for read */
    TMR_REG_CNT64_CTL |= (1<<1);
    while(TMR_REG_CNT64_CTL & (1<<1));

    /* read the 64bits counter */
    lower = TMR_REG_CNT64_LO;
    upper = TMR_REG_CNT64_HI;

    /* restore interrupt response */
    raw_local_irq_restore(flags);

    return (((__u64)upper)<<32) | lower;
}


/*
*********************************************************************************************************
*                           aw_set_clkevt_mode
*
*Description: set clock event work mode.
*
*Arguments  : mode  mode for clock event work;
*             dev   clock event device;
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void aw_set_clkevt_mode(enum clock_event_mode mode, struct clock_event_device *dev)
{
    kevin_dbg("aw_set_clkevt_mode:%u\n", mode);
    switch (mode)
    {
        case CLOCK_EVT_MODE_PERIODIC:
        {
            /* set timer work with continueous mode */
            TMR_REG_TMR1_CTL &= ~(1<<0);
            /* wait hardware synchronization, 2 cycles of the hardware work clock at least  */
            ndelay((1000000000*2+AW_HPET_CLOCK_EVENT_HZ-1)/AW_HPET_CLOCK_EVENT_HZ);
            TMR_REG_TMR1_CTL &= ~(1<<7);
            TMR_REG_TMR1_CTL |= (1<<0);
            break;
        }

        case CLOCK_EVT_MODE_ONESHOT:
        {
            /* set timer work with onshot mode */
            TMR_REG_TMR1_CTL &= ~(1<<0);
            /* wait hardware synchronization, 2 cycles of the hardware work clock at least  */
            ndelay((1000000000*2+AW_HPET_CLOCK_EVENT_HZ-1)/AW_HPET_CLOCK_EVENT_HZ);
            TMR_REG_TMR1_CTL |= (1<<7);
            TMR_REG_TMR1_CTL |= (1<<0);
            break;
        }

        default:
        {
            /* disable clock event device */
            TMR_REG_TMR1_CTL &= ~(1<<0);
            /* wait hardware synchronization, 2 cycles of the hardware work clock at least  */
            ndelay((1000000000*2+AW_HPET_CLOCK_EVENT_HZ-1)/AW_HPET_CLOCK_EVENT_HZ);
            break;
        }
    }
}


/*
*********************************************************************************************************
*                           aw_set_next_clkevt
*
*Description: set next clock event.
*
*Arguments  : delta     cycle count for next clock event.
*             dev       clock event device.
*
*Return     : result,
*               0,  set next event successed;
*              !0,  set next event failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static int aw_set_next_clkevt(unsigned long delta, struct clock_event_device *dev)
{
    kevin_dbg("aw_set_next_clkevt: %u\n", (unsigned int)delta);

    #if(0)  //2011-5-20 11:08, suggested by david
    TMR_REG_IRQ_STAT = (1<<1);
    #endif

    /* disable timer and clear pending first    */
    TMR_REG_TMR1_CTL &= ~(1<<0);
    /* wait hardware synchronization, 2 cycles of the hardware work clock at least  */
    ndelay((1000000000*2+AW_HPET_CLOCK_EVENT_HZ-1)/AW_HPET_CLOCK_EVENT_HZ);

    /* set timer intervalue         */
    TMR_REG_TMR1_INTV = delta;
    /* reload the timer intervalue  */
    TMR_REG_TMR1_CTL |= (1<<1);
    /* wait hardware synchronization, 2 cycles of the hardware work clock at least  */
    ndelay((1000000000*2+AW_HPET_CLOCK_EVENT_HZ-1)/AW_HPET_CLOCK_EVENT_HZ);

    /* enable timer */
    TMR_REG_TMR1_CTL |= (1<<0);

    return 0;
}


/*
*********************************************************************************************************
*                                   aw_clkevt_irq
*
*Description: clock event interrupt handler.
*
*Arguments  : irq       interrupt number of current processed;
*             handle    device handle registered when setup irq;
*
*Return     : result,
*               IRQ_HANDLED,    irq is processed successed;
*               IRQ_NONE,       irq is not setup by us;
*
*Notes      :
*
*********************************************************************************************************
*/
static irqreturn_t aw_clkevt_irq(int irq, void *handle)
{
    if(TMR_REG_IRQ_STAT & (1<<1))
    {
        kevin_dbg("aw_clkevt_irq!\n");
        /* clock event interrupt handled */
        aw_clock_event.event_handler(&aw_clock_event);
        /* clear pending */
        TMR_REG_IRQ_STAT = (1<<1);

        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}


/*
*********************************************************************************************************
*                           aw_clksrc_init
*
*Description: clock source initialise.
*
*Arguments  : none
*
*Return     : result,
*               0,  initiate successed;
*              !0,  initiate failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_clksrc_init(void)
{
    printk("all-winners clock source init!\n");
    /* we use 64bits counter as HPET(High Precision Event Timer) */
    TMR_REG_CNT64_CTL  = 0;
    /* config clock source for 64bits counter */
    #if(AW_HPET_CLK_SRC == TMR_CLK_SRC_24MHOSC)
        TMR_REG_CNT64_CTL |= (0<<2);
    #else
        TMR_REG_CNT64_CTL |= (1<<2);
    #endif
    /* clear 64bits counter */
    TMR_REG_CNT64_CTL |= (1<<0);
    printk("register all-winners clock source!\n");
    /* calculate the mult by shift  */
    aw_clocksrc.mult = clocksource_hz2mult(AW_HPET_CLOCK_SOURCE_HZ, aw_clocksrc.shift);
    /* register clock source */
    clocksource_register(&aw_clocksrc);

    /* register clock event irq     */
    printk("set up all-winners clock event irq!\n");
    /* config clock source for timer1 */
    #if(AW_HPET_CLK_EVT == TMR_CLK_SRC_32KLOSC)
        TMR_REG_TMR1_CTL |= (0<<2);
    #else
        TMR_REG_TMR1_CTL |= (1<<2);
    #endif
    TMR_REG_TMR1_INTV = AW_HPET_CLOCK_EVENT_HZ>>8;
    TMR_REG_TMR1_CTL |= (1<<1);
    TMR_REG_TMR1_CTL &= ~(3<<2);
    TMR_REG_TMR1_CTL |= (0<<2);
    setup_irq(SW_INT_IRQNO_TIMER1, &aw_clkevt_irqact);
    /* enable timer1 irq            */
    TMR_REG_IRQ_EN |= (1<<1);

    /* register clock event device  */
    printk("register all-winners clock event device!\n");
	aw_clock_event.mult = div_sc(AW_HPET_CLOCK_EVENT_HZ, NSEC_PER_SEC, aw_clock_event.shift);
	aw_clock_event.max_delta_ns = clockevent_delta2ns((0x80000000/AW_HPET_CLOCK_EVENT_HZ), &aw_clock_event);
	aw_clock_event.min_delta_ns = clockevent_delta2ns(1, &aw_clock_event) + 1;
	aw_clock_event.cpumask = cpumask_of(0);
    clockevents_register_device(&aw_clock_event);

    return 0;
}

arch_initcall(aw_clksrc_init);
