/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_power.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"



//==============================================================================
// POWER CHECK FOR SYSTEM STANDBY
//==============================================================================


/*
*********************************************************************************************************
*                           standby_power_init
*
* Description: init power for standby.
*
* Arguments  : none;
*
* Returns    : result;
*********************************************************************************************************
*/
__s32 standby_power_init(void)
{
    return 0;
}


/*
*********************************************************************************************************
*                           standby_power_exit
*
* Description: exit power for standby.
*
* Arguments  : none;
*
* Returns    : result;
*********************************************************************************************************
*/
__s32 standby_power_exit(void)
{
    return 0;
}

/*
*********************************************************************************************************
*                           standby_set_voltage
*
*Description: set voltage for standby;
*
*Arguments  : type      voltage type, defined as "enum power_vol_type_e";
*             voltage   voltage value, based on "mv";
*
*Return     : none;
*
*Notes      :
*
*********************************************************************************************************
*/
void  standby_set_voltage(enum power_vol_type_e type, __s32 voltage)
{
    return;
}


/*
*********************************************************************************************************
*                           standby_get_voltage
*
*Description: get voltage for standby;
*
*Arguments  : type  voltage type, defined as "enum power_vol_type_e";
*
*Return     : voltage value, based on "mv";
*
*Notes      :
*
*********************************************************************************************************
*/
__u32 standby_get_voltage(enum power_vol_type_e type)
{
    return 0;
}


