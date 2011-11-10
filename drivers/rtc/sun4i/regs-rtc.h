/* arch/arm/mach-s3c2410/include/mach/regs-rtc.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 Internal RTC register definition
*/

#ifndef __ASM_ARCH_REGS_RTC_H
#define __ASM_ARCH_REGS_RTC_H __FILE__

#include <mach/hardware.h>

#define RTC_ADDR_SIZE		        0x3ff

#define RTC_BASE_ADDR_START        (0x01C20C00)
#define RTC_BASE_ADDR_END          (RTC_BASE_ADDR_START + RTC_ADDR_SIZE)


#define AW1623_TIMER_IRQ_EN_REG                        (0x00)                                                              
#define AW1623_LOSC_CTRL_REG                           (0x100)

#define AW1623_RTC_DATE_REG                            (0x104)
#define AW1623_RTC_TIME_REG                            (0x108)

#define AW1623_RTC_ALARM_DD_HH_MM_SS_REG               (0x10C)
#define AW1623_RTC_WEEK_HH_MM_SS_REG                   (0x110)

#define AW1623_ALARM_EN_REG                            (0x114)
#define AW1623_ALARM_INT_CTRL_REG                      (0x118)
#define AW1623_ALARM_INT_STATUS_REG                    (0x11C)

/*interrupt control*/
/*rtc week interrupt control*/
#define RTC_ALARM_MONDAY_INT_EN 			 			0x00000001
#define RTC_ALARM_TUESDAY_INT_EN 			 			0x00000002
#define RTC_ALARM_WEDNESDAY_INT_EN 			 			0x00000004
#define RTC_ALARM_THURSDAY_INT_EN 			 			0x00000008
#define RTC_ALARM_FRIDAY_INT_EN 			 			0x00000010
#define RTC_ALARM_SATURDAY_INT_EN 			 			0x00000020
#define RTC_ALARM_SUNDAY_INT_EN 			 			0x00000040

#define RTC_ENABLE_WK_IRQ            					0x00000002
#define RTC_DISABLE_WK_IRQ          	 				0xfffffffd

/*rtc count interrupt control*/
#define RTC_ALARM_COUNT_INT_EN 			 				0x00000100

#define RTC_ENABLE_CNT_IRQ        						0x00000001
#define RTC_DISABLE_CNT_IRQ       						0xfffffffe

/*Crystal Control*/
#define REG_LOSCCTRL_MAGIC		    					0x16aa0000
#define REG_CLK32K_AUTO_SWT_EN  						(0x00004000)
#define RTC_SOURCE_INTERNAL         					0x00000000
#define RTC_SOURCE_EXTERNAL         					0x00000001
#define ALM_DDHHMMSS_ACCESS         					0x00000200
#define RTC_HHMMSS_ACCESS           					0x00000100
#define RTC_YYMMDD_ACCESS           					0x00000080
#define EXT_LOSC_GSM                          			(0x00000008)

/*Date Value*/
#define DATE_GET_DAY_VALUE(x)       					((x) &0x0000001f)
#define DATE_GET_MON_VALUE(x)       					(((x)&0x00000f00) >> 8 )
#define DATE_GET_YEAR_VALUE(x)      					(((x)&0x003f0000) >> 16)

#define DATE_SET_DAY_VALUE(x)       					DATE_GET_DAY_VALUE(x)
#define DATE_SET_MON_VALUE(x)       					(((x)&0x0000000f) << 8 )
#define DATE_SET_YEAR_VALUE(x)      					(((x)&0x0000003f) << 16)
#define LEAP_SET_VALUE(x)           					(((x)&0x00000001) << 22)

/*Time Value*/
#define TIME_GET_SEC_VALUE(x)       					((x) &0x0000003f)
#define TIME_GET_MIN_VALUE(x)       					(((x)&0x00003f00) >> 8 )
#define TIME_GET_HOUR_VALUE(x)      					(((x)&0x001f0000) >> 16)
#define TIME_GET_WEEK_VALUE(x)							(((x)&0xf0000000) >> 29)

#define TIME_SET_SEC_VALUE(x)       					TIME_GET_SEC_VALUE(x)
#define TIME_SET_MIN_VALUE(x)       					(((x)&0x0000003f) << 8 )
#define TIME_SET_HOUR_VALUE(x)      					(((x)&0x0000001f) << 16)
#define TIME_SET_WEEK_VALUE(x)							(((x)&0xf0000000) << 29)

/*ALARM Value*/
#define ALARM_GET_SEC_VALUE(x)      					((x) &0x0000003f)
#define ALARM_GET_MIN_VALUE(x)      					(((x)&0x00003f00) >> 8 )
#define ALARM_GET_HOUR_VALUE(x)     					(((x)&0x001f0000) >> 16)
#define ALARM_GET_DAY_VALUE(x)      					(((x)&0xffe00000) >> 21)
#define ALARM_SET_SEC_VALUE(x)      					((x) &0x0000003f)
#define ALARM_SET_MIN_VALUE(x)      					(((x)&0x0000003f) << 8 )
#define ALARM_SET_HOUR_VALUE(x)     					(((x)&0x0000001f) << 16)
#define ALARM_SET_DAY_VALUE(x)      					(((x)&0x00000ffe) << 21)

#endif /* __ASM_ARCH_REGS_RTC_H */
