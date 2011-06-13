/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_key.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 15:16
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_KEY_H__
#define __STANDBY_KEY_H__

#include "standby_cfg.h"


extern __s32 standby_key_init(void);
extern __s32 standby_key_exit(void);
extern __s32 standby_query_key(void);


#endif  /* __STANDBY_KEY_H__ */

