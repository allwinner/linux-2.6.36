/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 18:34
* Descript: platform standby fucntion.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"


extern unsigned int save_sp(void);
extern void restore_sp(unsigned int sp);
extern char *__bss_start;
extern char *__bss_end;

static sp_backup;
static void standby(void);
static __u32 vcc, core_vdd, dram_vdd;

/* parameter for standby, it will be transfered from sys_pwm module */
struct aw_pm_info  pm_info;


/*
*********************************************************************************************************
*                                   STANDBY MAIN PROCESS ENTRY
*
* Description: standby main process entry.
*
* Arguments  : arg  pointer to the parameter that transfered from sys_pwm module.
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
int main(struct aw_pm_info *arg)
{
    char    *tmpPtr = (char *)&__bss_start;

    if(!arg){
        /* standby parameter is invalid */
        return -1;
    }
    /* clear bss segment */
    do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

    /* copy standby parameter from dram */
    standby_memcpy(&pm_info, arg, sizeof(pm_info));

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    /* init module before dram enter selfrefresh */
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    /* initialise standby modules */
    standby_clk_init();
    standby_int_init();
    standby_tmr_init();
    standby_twi_init();
    standby_power_init();
    /* init some system wake source */
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_EXINT){
        standby_enable_int(INT_SOURCE_EXTNMI);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_KEY){
        standby_key_init();
        standby_enable_int(INT_SOURCE_LRADC);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_IR){
        standby_ir_init();
        standby_enable_int(INT_SOURCE_IR0);
        standby_enable_int(INT_SOURCE_IR1);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_USB){
        standby_usb_init();
        standby_enable_int(INT_SOURCE_USB0);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_TIMEOFF){
        /* set timer for power off */
        if(pm_info.standby_para.time_off) {
            standby_tmr_set(pm_info.standby_para.time_off);
            standby_enable_int(INT_SOURCE_TIMER0);
        }
    }

    /* save stack pointer registger, switch stack to sram */
    sp_backup = save_sp();
    /* enable dram enter into self-refresh */
    dram_enter_selfrefresh();
    /* process standby */
    standby();
    /* restore dram */
    dram_exit_selfrefresh();
    /* restore stack pointer register, switch stack back to dram */
    restore_sp(sp_backup);

    /* exit standby module */
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_USB){
        standby_usb_exit();
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_IR){
        standby_ir_exit();
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_KEY){
        standby_key_exit();
    }
    standby_power_exit();
    standby_twi_exit();
    standby_tmr_exit();
    standby_int_exit();
    standby_clk_init();

    /* report which wake source wakeup system */
    arg->standby_para.event = pm_info.standby_para.event;

    return 0;
}


/*
*********************************************************************************************************
*                                     SYSTEM PWM ENTER STANDBY MODE
*
* Description: enter standby mode.
*
* Arguments  : none
*
* Returns    : none;
*********************************************************************************************************
*/
static void standby(void)
{
    /* backup voltages */
    vcc = standby_get_voltage(POWER_VOL_DCDC1);
    core_vdd = standby_get_voltage(POWER_VOL_DCDC2);
    dram_vdd = standby_get_voltage(POWER_VOL_DCDC3);

    /* switch cpu clock to HOSC, and disable pll */
    standby_clk_core2hosc();
    standby_clk_plldisable();

    /* adjust voltage */
    standby_set_voltage(POWER_VOL_DCDC1, VCC_SLEEP_VOL);
    standby_set_voltage(POWER_VOL_DCDC3, DRAMVDD_SLEEP_VOL);
    standby_set_voltage(POWER_VOL_DCDC2, COREVDD_SLEEP_VOL);

    #if 0
    /* switch cpu to 32k */
    standby_clk_core2losc();
    #endif

    #if(ALLOW_DISABLE_HOSC)
    // disable HOSC, and disable LDO
    standby_clk_hoscdisable();
    standby_clk_ldodisable();
    #endif

    /* clear lradc key */
    standby_query_key();

    /* set vdd voltage to 1.0v */
    standby_set_voltage(POWER_VOL_DCDC2, COREVDD_DEEP_SLEEP_VOL);

    /* cpu enter sleep, wait wakeup by interrupt */
    asm("WFI");

    /* restore voltage to 1.2v */
    standby_set_voltage(POWER_VOL_DCDC2, COREVDD_SLEEP_VOL);
    standby_mdelay(30);

    #if(ALLOW_DISABLE_HOSC)
    /* enable LDO, enable HOSC */
    standby_clk_ldoenable();
    standby_clk_hoscenable();
    #endif

    #if 0
    /* switch clock to LOSC */
    standby_clk_core2hosc();
    #endif

    /* check system wakeup event */
    pm_info.standby_para.event = 0;
    pm_info.standby_para.event |= standby_query_int(INT_SOURCE_EXTNMI)? 0:SUSPEND_WAKEUP_SRC_EXINT;
    pm_info.standby_para.event |= standby_query_int(INT_SOURCE_USB0)? 0:SUSPEND_WAKEUP_SRC_USB;
    pm_info.standby_para.event |= standby_query_int(INT_SOURCE_LRADC)? 0:SUSPEND_WAKEUP_SRC_KEY;
    pm_info.standby_para.event |= standby_query_int(INT_SOURCE_IR0)? 0:SUSPEND_WAKEUP_SRC_IR;
    pm_info.standby_para.event |= standby_query_int(INT_SOURCE_TIMER0)? 0:SUSPEND_WAKEUP_SRC_TIMEOFF;

    /* restore voltage for exit standby */
    standby_set_voltage(POWER_VOL_DCDC1, vcc);
    standby_set_voltage(POWER_VOL_DCDC2, core_vdd);
    standby_set_voltage(POWER_VOL_DCDC3, dram_vdd);
    standby_mdelay(30);

    /* enable pll */
    standby_clk_pllenable();
    standby_mdelay(30);
    /* switch cpu clock to core pll */
    standby_clk_core2pll();

    return;
}

