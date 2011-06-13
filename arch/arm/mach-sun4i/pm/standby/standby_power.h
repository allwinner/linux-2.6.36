/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_power.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_POWER_H__
#define __STANDBY_POWER_H__

#include "standby_cfg.h"

enum power_vol_type_e{

    POWER_VOL_DCDC1,
    POWER_VOL_DCDC2,
    POWER_VOL_DCDC3,
    POWER_VOL_LDO1,
    POWER_VOL_LDO2,
    POWER_VOL_LDO3,
    POWER_VOL_LDO4,

};


extern __s32 standby_power_init(void);
extern __s32 standby_power_exit(void);
extern void  standby_set_voltage(enum power_vol_type_e type, __s32 voltage);
extern __u32 standby_get_voltage(enum power_vol_type_e type);


#endif  /* __STANDBY_POWER_H__ */


