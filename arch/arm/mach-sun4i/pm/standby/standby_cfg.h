/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_cfg.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:29
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_CFG_H__
#define __STANDBY_CFG_H__


//config wakeup source for standby
#define DISABLE_HOSC                (0)     // if allow disable hosc

#define VCC_SLEEP_VOL               (3000)  // vcc sleep voltage, based on mini-volt
#define COREVDD_SLEEP_VOL           (1000)  // core-vdd sleep voltage, based on mini-volt
#define DRAMVDD_SLEEP_VOL           (1500)  // dram-vdd sleep voltage, based on mini-volt
#define COREVDD_DEEP_SLEEP_VOL      (800)   // core-vdd deep sleep voltage, based on mini-volt


#endif  /* __STANDBY_CFG_H__ */


