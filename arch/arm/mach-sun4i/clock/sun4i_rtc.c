/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : sun4i_rtc.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-8-3 14:47
* Descript: try to restore system time by rtc periodic;
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include <linux/module.h>
#include <linux/rtc.h>

/* define rtc restore period, based on second */
#define RTC_RESTORE_PERIOD      (600)

static struct delayed_work rtc_restore_work;

extern struct rtc_device    *sw_rtc_dev;



/*
*********************************************************************************************************
*                           rtc_restore
*
*Description: rtc restore work;
*
*Arguments  :
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
static void rtc_restore(struct work_struct *work)
{
    struct rtc_time     rtc_time;
    time_t              time_second;
    struct timespec     sys_time;

    printk("%s:%s\n", __FILE__, __func__);

    /* get rtc time for rtc device */
	rtc_read_time(sw_rtc_dev, &rtc_time);
    /* convert rtc time to second */
    rtc_tm_to_time(&rtc_time, &time_second);
    /* convert second to time spec, because the precision of rtc time is just second,
       we just restore the second value, and the nano second value is ignored   */
    set_normalized_timespec(&sys_time, time_second, 0);

    /* set new system time */
    do_settimeofday(&sys_time);

    schedule_delayed_work(&rtc_restore_work, msecs_to_jiffies(RTC_RESTORE_PERIOD * 1000));
}


/*
*********************************************************************************************************
*                           rtc_restore_init
*
*Description: rtc restore init;
*
*Arguments  : none
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
static int rtc_restore_init(void)
{
    INIT_DELAYED_WORK(&rtc_restore_work, rtc_restore);
    schedule_delayed_work(&rtc_restore_work, msecs_to_jiffies(RTC_RESTORE_PERIOD * 1000));

    return 0;
}
late_initcall(rtc_restore_init);

