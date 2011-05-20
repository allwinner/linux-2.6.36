/*
 * drivers/input/touchscreen/tsc2007.h
 *
 * Copyright (c) 2008 MtekVision Co., Ltd.
 *	Kwangwoo Lee <kwlee@mtekvision.com>
 *
 * Using code from:
 *  - ads7846.c
 *	Copyright (c) 2005 David Brownell
 *	Copyright (c) 2006 Nokia Corporation
 *  - corgi_ts.c
 *	Copyright (C) 2004-2005 Richard Purdie
 *  - omap_ts.[hc], ads7846.h, ts_osk.c
 *	Copyright (C) 2002 MontaVista Software
 *	Copyright (C) 2004 Texas Instruments
 *	Copyright (C) 2005 Dirk Behme
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c/tsc2007.h>

#define TS_POLL_DELAY			    10/* ms delay between samples */
#define TS_POLL_PERIOD			    10 /* ms delay between samples */

#define TSC2007_MEASURE_TEMP0		(0x0 << 4)
#define TSC2007_MEASURE_AUX		    (0x2 << 4)
#define TSC2007_MEASURE_TEMP1		(0x4 << 4)
#define TSC2007_ACTIVATE_XN		    (0x8 << 4)
#define TSC2007_ACTIVATE_YN		    (0x9 << 4)
#define TSC2007_ACTIVATE_YP_XN		(0xa << 4)
#define TSC2007_SETUP			    (0xb << 4)
#define TSC2007_MEASURE_X		    (0xc << 4)
#define TSC2007_MEASURE_Y		    (0xd << 4)
#define TSC2007_MEASURE_Z1		    (0xe << 4)
#define TSC2007_MEASURE_Z2		    (0xf << 4)

#define TSC2007_POWER_OFF_IRQ_EN	(0x0 << 2)
#define TSC2007_ADC_ON_IRQ_DIS0		(0x1 << 2)
#define TSC2007_ADC_OFF_IRQ_EN		(0x2 << 2)
#define TSC2007_ADC_ON_IRQ_DIS1		(0x3 << 2)

#define TSC2007_12BIT			    (0x0 << 1)
#define TSC2007_8BIT			    (0x1 << 1)

#define	MAX_12BIT			        ((1 << 12) - 1)

#define ADC_ON_12BIT	            (TSC2007_12BIT | TSC2007_ADC_ON_IRQ_DIS0)

#define READ_Y		                (ADC_ON_12BIT | TSC2007_MEASURE_Y)
#define READ_Z1		                (ADC_ON_12BIT | TSC2007_MEASURE_Z1)
#define READ_Z2		                (ADC_ON_12BIT | TSC2007_MEASURE_Z2)
#define READ_X		                (ADC_ON_12BIT | TSC2007_MEASURE_X)
#define PWRDOWN		                (TSC2007_12BIT | TSC2007_POWER_OFF_IRQ_EN)
#define INPUT_DEV_NAME              "TSC2007 Touchscreen"
struct ts_event {
	u16	x;
	u16	y;
	u16	z1, z2;
};

struct tsc2007 {
	struct input_dev	*input;
	char			phys[32];
	struct delayed_work	work;

	struct i2c_client	*client;

	u16			model;
	u16			x_plate_ohms;

	bool        pendown;
	int			irq;

	int			(*get_pendown_state)(void);
	void        (*clear_penirq)(void);
	int         (*set_irq_mode)(void);
	int         (*set_gpio_mode)(void);
	int         (*judge_int_occur)(void);
	
};
