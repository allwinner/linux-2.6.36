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
static __u32 dcdc2, dcdc3, ldo1, ldo2, ldo3, ldo4;

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
    dcdc2 = standby_get_voltage(POWER_VOL_DCDC2);
    dcdc3 = standby_get_voltage(POWER_VOL_DCDC3);
    ldo1 = standby_get_voltage(POWER_VOL_LDO1);
    ldo2 = standby_get_voltage(POWER_VOL_LDO2);
    ldo3 = standby_get_voltage(POWER_VOL_LDO3);
    ldo4 = standby_get_voltage(POWER_VOL_LDO4);

    /* switch cpu clock to HOSC, and disable pll */
    standby_clk_core2hosc();
    standby_clk_plldisable();

    /* adjust voltage */
    standby_set_voltage(POWER_VOL_DCDC2, STANDBY_DCDC2_VOL);
    standby_set_voltage(POWER_VOL_DCDC3, STANDBY_DCDC3_VOL);
    standby_set_voltage(POWER_VOL_LDO1,  STANDBY_LDO1_VOL);
    standby_set_voltage(POWER_VOL_LDO2,  STANDBY_LDO2_VOL);
    standby_set_voltage(POWER_VOL_LDO3,  STANDBY_LDO3_VOL);
    standby_set_voltage(POWER_VOL_LDO4,  STANDBY_LDO4_VOL);

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

    /* cpu enter sleep, wait wakeup by interrupt */
    asm("WFI");

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
    standby_set_voltage(POWER_VOL_DCDC2, dcdc2);
    standby_set_voltage(POWER_VOL_DCDC3, dcdc3);
    standby_set_voltage(POWER_VOL_LDO1, ldo1);
    standby_set_voltage(POWER_VOL_LDO2, ldo2);
    standby_set_voltage(POWER_VOL_LDO3, ldo3);
    standby_set_voltage(POWER_VOL_LDO4, ldo4);
    standby_mdelay(30);

    /* enable pll */
    standby_clk_pllenable();
    standby_mdelay(30);
    /* switch cpu clock to core pll */
    standby_clk_core2pll();

    return;
}

