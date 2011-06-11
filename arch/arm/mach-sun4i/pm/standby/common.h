/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : common.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 17:21
* Descript: common lib for standby.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __COMMON_H__
#define __COMMON_H__

typedef signed char         __s8;
typedef unsigned char       __u8;
typedef signed short        __s16;
typedef unsigned short      __u16;
typedef signed int          __s32;
typedef unsigned int        __u32;
typedef signed long long    __s64;
typedef unsigned long long  __u64;




void standby_memcpy(void *dest, void *src, int n);
void standby_mdelay(int ms);


#endif  //__COMMON_H__

