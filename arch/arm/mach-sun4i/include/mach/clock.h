/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : clock.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-13 16:43
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __AW_CLOCK_H__
#define __AW_CLOCK_H__

#include <linux/kernel.h>
#include "aw_ccu.h"

/* define clock type            */
typedef enum __CCU_CLK_TYPE
{
    CCU_CLK_TYPE_SYS,
    CCU_CLK_TYPE_MOD,

} __ccu_clk_type_e;

typedef enum __CCU_CLK_CHANGE
{
    CCU_CLK_CHANGE_NONE,
    CCU_CLK_CHANGE_PREPARE,
    CCU_CLK_CHANGE_DONE,

} __ccu_clk_change_e;


typedef struct clk
{
    __aw_ccu_clk_t  *clk;       /* clock handle from ccu csp                            */
    __s32           usr_cnt;    /* user count                                           */
    __s32           enable;     /* enable count, when it down to 0, it will be disalbe  */
    __s32           hash;       /* hash value, for fast search without string compare   */

    __aw_ccu_clk_t  *(*get_clk)(__s32 id);
                                /* set clock                                            */
    __aw_ccu_err_e  (*set_clk)(__aw_ccu_clk_t *clk);
                                /* get clock                                            */
    struct clk      *parent;    /* parent clock node pointer                            */
    struct clk      *child;     /* child clock node pinter                              */
    struct clk      *left;      /* left brother node pointer                            */
    struct clk      *right;     /* right bother node pointer                            */

} __ccu_clk_t;


#endif  /* #ifndef __AW_CLOCK_H__ */

