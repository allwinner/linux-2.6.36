#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>

#include "regs-rtc.h"
#define PWM_CTRL_REG_BASE         (0xf1c20c00+0x200)
/*
 * notice: IN 23 A version, operation(eg. write date, time reg)
 * that will affect losc reg, will also affect pwm reg at the same time
 * it is a ic bug needed to be fixed,
 * right now, before write date, time reg, we need to backup pwm reg
 * after writing, we should restore pwm reg.
 */
//#define BACKUP_PWM

/* record rtc device handle for platform to restore system time */
struct rtc_device   *sw_rtc_dev = NULL;

/*说明 f23最大变化为63年的时间
该驱动支持（2010～2073）年的时间*/

static void __iomem *f23_rtc_base;
//static struct clk *rtc_clk;

static int f23_rtc_alarmno = NO_IRQ;
static int losc_err_flag   = 0;

/* IRQ Handlers, irq no. is shared with timer2 */
//marked by young
/*
static irqreturn_t f23_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *rdev = id;
	u32 val;

	_dev_info(&(rdev->dev), "f23_rtc_alarmirq\n");

    //judge the int is whether ours
    val = readl(f23_rtc_base + AW1623_ALARM_INT_STATUS_REG)&(RTC_ENABLE_WK_IRQ | RTC_ENABLE_CNT_IRQ);
    if(val)
    {
	    // Clear pending alarm
	    val = readl(f23_rtc_base + AW1623_ALARM_INT_STATUS_REG);
	    val |= (3);
	    writel(val, f23_rtc_base + AW1623_ALARM_INT_STATUS_REG);

	    rtc_update_irq(rdev, 1, RTC_AF | RTC_IRQF);
	    return IRQ_HANDLED;
    }
    else
    {
        return IRQ_NONE;
    }
}
*/

/* Update control registers */
static void f23_rtc_setaie(int to)
{
	u32 alarm_irq_val;
    u32 timer_irq_val;

	printk("%s: aie=%d\n", __func__, to);

    timer_irq_val = readl(f23_rtc_base + AW1623_TIMER_IRQ_EN_REG);
	alarm_irq_val = readl(f23_rtc_base + AW1623_ALARM_EN_REG);

	if(to) {
		alarm_irq_val |= to;
        timer_irq_val |= ALARM_TIMER2_IRQ;
        writel(timer_irq_val, f23_rtc_base + AW1623_TIMER_IRQ_EN_REG);
	    writel(alarm_irq_val, f23_rtc_base + AW1623_ALARM_EN_REG);	}
	else {
        alarm_irq_val = 0x0;
        //timer_irq_val & (~ALARM_TIMER2_IRQ); share with timer2, should not change it arbitary;
        //writel(ALARM_TIMER2_IRQ, f23_rtc_base + AW1623_TIMER_IRQ_EN_REG);
	    writel(alarm_irq_val, f23_rtc_base + AW1623_ALARM_EN_REG);

	}


}

static int f23_rtc_ioctl(struct device *dev, unsigned int cmd,
	unsigned long arg)
{
	/* We do support interrupts, they're generated
	 * using the sysfs interface.
	 */
	switch (cmd) {
	case RTC_PIE_ON:
	case RTC_PIE_OFF:
	case RTC_UIE_ON:
	case RTC_UIE_OFF:
	case RTC_AIE_ON:
	case RTC_AIE_OFF:
    case RTC_ALM_SET:
    case RTC_ALM_READ:
    case RTC_WKALM_SET:
    case RTC_WKALM_RD:
    case RTC_IRQP_SET:
    case RTC_IRQP_READ:
		return 0;
    /*Note that many of these ioctls need not actually be implemented by your
      *driver.  The common rtc-dev interface handles many of these nicely if your
      *driver returns ENOIOCTLCMD.  */
	default:
		return -ENOIOCTLCMD;
	}
}

/* Time read/write */

static int f23_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int have_retried = 0;
	void __iomem *base = f23_rtc_base;
	unsigned int date_tmp = 0;
	unsigned int time_tmp = 0;

    //only for alarm losc err occur.
    if(losc_err_flag) {
	rtc_tm->tm_sec  = 0;
	rtc_tm->tm_min  = 0;
	rtc_tm->tm_hour = 0;

	rtc_tm->tm_mday = 0;
	rtc_tm->tm_mon  = 0;
	rtc_tm->tm_year = 0;
	return -1;
    }

retry_get_time:
	_dev_info(dev,"f23_rtc_gettime\n");
    //first to get the date, then time, because the sec turn to 0 will effect the date;
	date_tmp = readl(base + AW1623_RTC_DATE_REG);
	time_tmp = readl(base + AW1623_RTC_TIME_REG);


	rtc_tm->tm_sec  = TIME_GET_SEC_VALUE(time_tmp);
	rtc_tm->tm_min  = TIME_GET_MIN_VALUE(time_tmp);
	rtc_tm->tm_hour = TIME_GET_HOUR_VALUE(time_tmp);

	rtc_tm->tm_mday = DATE_GET_DAY_VALUE(date_tmp);
	rtc_tm->tm_mon  = DATE_GET_MON_VALUE(date_tmp);
	rtc_tm->tm_year = DATE_GET_YEAR_VALUE(date_tmp);

	/* the only way to work out wether the system was mid-update
	 * when we read it is to check the second counter, and if it
	 * is zero, then we re-try the entire read
	 */
	if (rtc_tm->tm_sec == 0 && !have_retried) {
		have_retried = 1;
		goto retry_get_time;
	}

	rtc_tm->tm_year += 110;
	rtc_tm->tm_mon      -= 1;
	_dev_info(dev,"read time %d-%d-%d %d:%d:%d\n",
	       rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday,
	       rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return 0;
}

static int f23_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	void __iomem *base = f23_rtc_base;
	unsigned int date_tmp = 0;
	unsigned int time_tmp = 0;
	unsigned int crystal_data = 0;
	unsigned int timeout = 0;
	int leap_year = 0;

#ifdef BACKUP_PWM
	unsigned int pwm_ctrl_reg_backup = 0;
	unsigned int pwm_ch0_period_backup = 0;
#endif

    //int tm_year; years from 1900
    //int tm_mon; months since jan 0-11
    //the input para tm->tm_year is the offset related 1900;
	leap_year = tm->tm_year + 1900;
	if(leap_year > 2073 || leap_year < 2010) {
		dev_err(dev, "rtc only supports 63（2010～2073） years\n");
		return -EINVAL;
	}

	crystal_data = readl(base + AW1623_LOSC_CTRL_REG);

	/*Any bit of [9:7] is set, The time and date
	  register can`t be written, we re-try the entried read*/
	#if 0
	if(crystal_data & 0x380) {
		dev_err(dev, "The time or date can`t set, please try late\n");
		return -EINVAL;
	}
	#else
	{
	    //check at most 3 times.
	    int times = 3;
	    while((crystal_data & 0x380) && (times--)){
	    	printk(KERN_INFO"[RTC]canot change rtc now!\n");
	    	msleep(500);
	    	crystal_data = readl(base + AW1623_LOSC_CTRL_REG);
	    }
	}

	#endif

	/*f23 ONLY SYPPORTS 63 YEARS*/
	tm->tm_year -= 110;
	tm->tm_mon  += 1;
	_dev_info(dev, "set time %d-%d-%d %d:%d:%d\n",
	       tm->tm_year + 2010, tm->tm_mon, tm->tm_mday,
	       tm->tm_hour, tm->tm_min, tm->tm_sec);

	date_tmp = (DATE_SET_DAY_VALUE(tm->tm_mday)|DATE_SET_MON_VALUE(tm->tm_mon)
                    |DATE_SET_YEAR_VALUE(tm->tm_year));

	time_tmp = (TIME_SET_SEC_VALUE(tm->tm_sec)|TIME_SET_MIN_VALUE(tm->tm_min)
                    |TIME_SET_HOUR_VALUE(tm->tm_hour));
#ifdef BACKUP_PWM
	    pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
	    pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
		printk("[rtc-pwm] 1 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
#endif

	writel(time_tmp,  base + AW1623_RTC_TIME_REG);
	timeout = 0xffff;
	while((readl(base + AW1623_LOSC_CTRL_REG)&(RTC_HHMMSS_ACCESS))&&(--timeout))
	if(timeout == 0)
    {
        dev_err(dev, "fail to set rtc time.\n");
#ifdef BACKUP_PWM
    	    writel(pwm_ctrl_reg_backup, PWM_CTRL_REG_BASE + 0);
    	    writel(pwm_ch0_period_backup, PWM_CTRL_REG_BASE + 4);

			pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
	    	pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
			printk("[rtc-pwm] 2 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
#endif
        return -1;
    }
    /*
    #ifdef BACKUP_PWM
       writel(pwm_ctrl_reg_backup, PWM_CTRL_REG_BASE + 0);
       writel(pwm_ch0_period_backup, PWM_CTRL_REG_BASE + 4);

	   pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
	   pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
	   printk("[rtc-pwm] 3 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
    #endif*/

	if((leap_year%400==0) || ((leap_year%100!=0) && (leap_year%4==0))) {
		/*Set Leap Year bit*/
		date_tmp |= LEAP_SET_VALUE(1);
	}

	/*
	#ifdef BACKUP_PWM
    	pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
    	pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);

	   printk("[rtc-pwm] 4 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
    #endif*/

	writel(date_tmp, base + AW1623_RTC_DATE_REG);
	timeout = 0xffff;
	while((readl(base + AW1623_LOSC_CTRL_REG)&(RTC_YYMMDD_ACCESS))&&(--timeout))
	if(timeout == 0)
    {
        dev_err(dev, "fail to set rtc date.\n");
#ifdef BACKUP_PWM
            writel(pwm_ctrl_reg_backup, PWM_CTRL_REG_BASE + 0);
            writel(pwm_ch0_period_backup, PWM_CTRL_REG_BASE + 4);

			pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
	    pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
	    printk("[rtc-pwm] 5 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
#endif
        return -1;
    }

#ifdef BACKUP_PWM
       writel(pwm_ctrl_reg_backup, PWM_CTRL_REG_BASE + 0);
       writel(pwm_ch0_period_backup, PWM_CTRL_REG_BASE + 4);

 		pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
	    pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
	    printk("[rtc-pwm] 6 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
#endif

    //wait about 70us to make sure the the time is really written into target.
    udelay(70);

	return 0;
}

//marked by young, need be improved.
/*{

static int f23_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{


        	struct rtc_time *alm_tm = &alrm->time;
        	void __iomem *base = f23_rtc_base;
        	unsigned int alarm_en;
        	unsigned int alarm_tmp = 0;
        	unsigned int date_tmp = 0;

	    alarm_tmp = readl(base + AW1623_RTC_ALARM_DD_HH_MM_SS_REG);
	    date_tmp = readl(base + TMRC_REG_RTC_DATE);

	    alm_tm->tm_sec  = ALARM_GET_SEC_VALUE(alarm_tmp);
	    alm_tm->tm_min  = ALARM_GET_MIN_VALUE(alarm_tmp);
	    alm_tm->tm_hour = ALARM_GET_HOUR_VALUE(alarm_tmp);

	    alm_tm->tm_mday = DATE_GET_DAY_VALUE(date_tmp);
	    alm_tm->tm_mon  = DATE_GET_MON_VALUE(date_tmp);
	    alm_tm->tm_year = DATE_GET_YEAR_VALUE(date_tmp);

	    alm_tm->tm_year += 110;
	    alm_tm->tm_mon  -= 1;

	    alarm_en = readl(base + TMRC_REG_LOSCCTRL);
	    alrm->enabled = RTC_GET_ALARM_STATE(alarm_en);

	    _dev_info(dev,"read alarm_en %d ,alarm %x, %d, %02d:%02d:%02d\n",
	           alarm_en, alrm->enabled, alarm_tmp,
	           alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);

	    // decode the alarm enable field

	return 0;
}
}*/

//marked by young, need be improved.
/*
static int f23_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    {

        struct rtc_time *tm = &alrm->time;
        void __iomem *base = f23_rtc_base;
        unsigned int alarm_tmp = 0;
        unsigned int alarm_en;
	    if(tm->tm_mday > 1023) {
	    	dev_err(dev, "The time or date can`t set, The day range of 0 to 1023\n");
	    	return -EINVAL;
	    }

	    alarm_tmp = ALARM_SET_SEC_VALUE(tm->tm_sec) | ALARM_SET_MIN_VALUE(tm->tm_min)
	    	| ALARM_SET_HOUR_VALUE(tm->tm_hour) | ALARM_SET_DAY_VALUE(tm->tm_mday);
	    writel(alarm_tmp, base + TMRC_REG_ALARM);

        pr_debug("f23_rtc_setalarm: %d, %d, %03x/%02x/%02x %02x.%02x.%02x\n",
        	 alrm->enabled, alarm_tmp
        	 tm->tm_mday & 0xfff, tm->tm_mon & 0xff, tm->tm_year & 0xff,
        	 tm->tm_hour & 0xff, tm->tm_min & 0xff, tm->tm_sec);

        //setting alarm
	    alrm_en = readb(base + AW1623_ALARM_EN_REG) & (RTC_ENABLE_WK_IRQ | RTC_ENABLE_CNT_IRQ);
	    writeb(0x00, base + AW1623_ALARM_EN_REG);

        //need to setting sec, min, hour alarm enable bit
	    //pr_debug("setting AW1623_ALARM_EN_REG to %08x\n", alrm_en);


	    f23_rtc_setaie(alrm->enabled);

        //don't need to processing it.
	    {
             if (alrm->enabled)
                enable_irq_wake(f23_rtc_alarmno);
            else
                disable_irq_wake(f23_rtc_alarmno);

	    }

    }

	return 0;
}
*/


static int f23_rtc_open(struct device *dev)
{
	//struct platform_device *pdev = to_platform_device(dev);
	//struct rtc_device *rtc_dev = platform_get_drvdata(pdev);
	int ret = 0;

	printk ("f23_rtc_open \n");
	//marked by young, the intterupt need to be improved.
  /*
   {
    	ret = request_irq(f23_rtc_alarmno, f23_rtc_alarmirq,
    			  IRQF_DISABLED,  "f23-rtc alarm", rtc_dev);
    	if (ret) {
    		dev_err(dev, "IRQ%d error %d\n", f23_rtc_alarmno, ret);
    		return ret;
    	}

    }*/

	return ret;
}


static void f23_rtc_release(struct device *dev)
{
	/*struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);*/
    //marked by young
    /*{
        free_irq(f23_rtc_alarmno, rtc_dev);
    }*/

    return ;
}

static const struct rtc_class_ops f23_rtcops = {
	.open		= f23_rtc_open,
	.release	= f23_rtc_release,
	.read_time	= f23_rtc_gettime,
	.set_time	= f23_rtc_settime,
    .ioctl      = f23_rtc_ioctl,
};

static int __devexit f23_rtc_remove(struct platform_device *dev)
{
	struct rtc_device *rtc = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);
	rtc_device_unregister(rtc);

	f23_rtc_setaie(0);

	return 0;
}

static int __devinit f23_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	//struct resource *res;
	int ret;
	unsigned int tmp_data;

#ifdef BACKUP_PWM
	unsigned int pwm_ctrl_reg_backup = 0;
	unsigned int pwm_ch0_period_backup = 0;
#endif

    //marked by young
    /*{

        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        //find the IRQs
        f23_rtc_alarmno = platform_get_irq(pdev, 0);
        if (res == NULL || f23_rtc_alarmno < 0) {
            return -ENODEV;
        }

        f23_rtc_base = ioremap(res->start, resource_size(res));
        if (!f23_rtc_base) {
            dev_err(&(pdev->dev), "rtc ioremap failure. \n");
            ret = -EIO;
        }

    }*/

    f23_rtc_base = (void __iomem *)(SW_VA_TIMERC_IO_BASE);
    f23_rtc_alarmno = SW_INT_IRQNO_ALARM;



	/* select RTC clock source
     * on fpga board, internal 32k clk src is the default, and can not be changed
     */
    //RTC CLOCK SOURCE internal 32K HZ 
#ifdef BACKUP_PWM
	    pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
	    pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
		printk("[rtc-pwm] 1 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
#endif

        //upate by kevin, 2011-9-7 18:23
        //step1: set keyfiled
        tmp_data = readl(f23_rtc_base + AW1623_LOSC_CTRL_REG);     
        tmp_data |= (RTC_SOURCE_EXTERNAL | REG_LOSCCTRL_MAGIC); //external     32768hz osc
        writel(tmp_data, f23_rtc_base + AW1623_LOSC_CTRL_REG);
        __udelay(100);
        _dev_info(&(pdev->dev),"f23_rtc_probe tmp_data = %d\n", tmp_data);

        //step2: check set result
        tmp_data = readl(f23_rtc_base + AW1623_LOSC_CTRL_REG);
        if(!(tmp_data & RTC_SOURCE_EXTERNAL))
        {
            printk("[RTC] ERR: Set LOSC to external failed!!!\n");
            printk("[RTC] WARNING: Rtc time will be wrong!!\n");
            losc_err_flag = 1;
        }

#ifdef BACKUP_PWM
        writel(pwm_ctrl_reg_backup, PWM_CTRL_REG_BASE + 0);
        writel(pwm_ch0_period_backup, PWM_CTRL_REG_BASE + 4);
		pwm_ctrl_reg_backup = readl(PWM_CTRL_REG_BASE + 0);
		pwm_ch0_period_backup = readl(PWM_CTRL_REG_BASE + 4);
		printk("[rtc-pwm] 2 pwm_ctrl_reg_backup = %x pwm_ch0_period_backup = %x", pwm_ctrl_reg_backup, pwm_ch0_period_backup);
#endif

	device_init_wakeup(&pdev->dev, 1);

	/* register RTC and exit */
	rtc = rtc_device_register("rtc", &pdev->dev, &f23_rtcops,
				  THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_out;
	}
    sw_rtc_dev = rtc;
	platform_set_drvdata(pdev, rtc);

	return 0;

err_out:
	return ret;
}

#ifdef CONFIG_PM

/* RTC Power management control */
//rtc do not to suspend, need to keep timing.
#define f23_rtc_suspend NULL
#define f23_rtc_resume  NULL
#else
#define f23_rtc_suspend NULL
#define f23_rtc_resume  NULL
#endif

//share the irq no. with timer2
static struct resource f23_rtc_resource[] = {
	[0] = {
		.start = SW_INT_IRQNO_ALARM,
		.end   = SW_INT_IRQNO_ALARM,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device f23_device_rtc = {
	.name		    = "sun4i-rtc",
	.id		        = -1,
	.num_resources	= ARRAY_SIZE(f23_rtc_resource),
	.resource	    = f23_rtc_resource,
};


static struct platform_driver f23_rtc_driver = {
	.probe		= f23_rtc_probe,
	.remove		= __devexit_p(f23_rtc_remove),
	.suspend	= f23_rtc_suspend,
	.resume		= f23_rtc_resume,
	.driver		= {
		.name	= "sun4i-rtc",
		.owner	= THIS_MODULE,
	},
};

static int __init f23_rtc_init(void)
{
	platform_device_register(&f23_device_rtc);
	printk("sun4i RTC version 0.1 \n");
	return platform_driver_register(&f23_rtc_driver);
}

static void __exit f23_rtc_exit(void)
{
	platform_driver_unregister(&f23_rtc_driver);
}

module_init(f23_rtc_init);
module_exit(f23_rtc_exit);

MODULE_DESCRIPTION("Sochip sun4i RTC Driver");
MODULE_AUTHOR("ben");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun4i-rtc");
