/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_key.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 15:16
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"



//==============================================================================
// QUERRY KEY FOR WAKE UP SYSTEM FROM STANDBY
//==============================================================================


/*
*********************************************************************************************************
*                                     INIT KEY FOR STANDBY
*
* Description: init key for standby.
*
* Arguments  : none
*
* Returns    : EPDK_OK;
*********************************************************************************************************
*/
__s32 standby_key_init(void)
{
    return 0;
}


/*
*********************************************************************************************************
*                                     EXIT KEY FOR STANDBY
*
* Description: exit key for standby.
*
* Arguments  : none
*
* Returns    : EPDK_OK;
*********************************************************************************************************
*/
__s32 standby_key_exit(void)
{
    return 0;
}


/*
*********************************************************************************************************
*                                     QUERY KEY FOR WAKEUP STANDBY
*
* Description: query key for wakeup standby.
*
* Arguments  : none
*
* Returns    : result;
*               EPDK_TRUE,      get a key;
*               EPDK_FALSE,     no key;
*********************************************************************************************************
*/
__s32 standby_query_key(void)
{
    return -1;
}

