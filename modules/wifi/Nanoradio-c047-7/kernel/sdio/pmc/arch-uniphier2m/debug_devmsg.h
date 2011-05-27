/******************************************************************************/
/*          :                                     */
/*        :  debug_devmsg.h                                         */
/*          :  M_DEBUG_PRINTK_LV1                                     */
/*          :  M_DEBUG_PRINTK_LV2                                     */
/*          :  M_DEBUG_PRINTK_LV3                                     */
/*        :  2004.04.13                                             */
/*            :                                                 */
/*          :                     */
/*          :  <linux/kernel.h>                 */
/*                                                                            */
/*         Unpublished Copyright(c)  2004             */
/******************************************************************************/
/***** FOMA DTV1 ******************************************************/
/*  No.                                       */
/*                                                                            */
/*<D1IO095>              debug_devmsg.h           05.07.25     */
/*                       DATE/TIME()        */
/*<8602715>              DD       05.09.01     */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _DEBUG_DEVMSG_H_                         /* _DEBUG_DEV_MSG_H_         */
#define _DEBUG_DEVMSG_H_

/******************************************************************************/
/* [01] Key                                                 */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_KEY_DEBUG_LV1
#define M_DEBUG_KEY_MSGLV1( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_KEY_MSGLV1( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_KEY_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_KEY_DEBUG_LV2
#define M_DEBUG_KEY_MSGLV2( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_KEY_MSGLV2( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_KEY_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_KEY_DEBUG_LV3
#define M_DEBUG_KEY_MSGLV3( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_KEY_MSGLV3( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_KEY_DEBUG_LV3 */

/******************************************************************************/
/* [02] (SYS)                                         */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_SYS_DEBUG_LV1
#define M_DEBUG_SYS_MSGLV1( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_SYS_MSGLV1( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_SYS_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_SYS_DEBUG_LV2
#define M_DEBUG_SYS_MSGLV2( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_SYS_MSGLV2( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_SYS_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_SYS_DEBUG_LV3
#define M_DEBUG_SYS_MSGLV3( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_SYS_MSGLV3( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_SYS_DEBUG_LV3 */

/******************************************************************************/
/* [03] (SM)                                          */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_SM_DEBUG_LV1
#define M_DEBUG_SM_MSGLV1( fmt, args... )                 /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_SM_MSGLV1( fmt, args... )          /*pgr0671 */
#endif /* DTV1DEV_SM_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_SM_DEBUG_LV2
#define M_DEBUG_SM_MSGLV2( fmt, args... )                 /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_SM_MSGLV2( fmt, args... )          /*pgr0671 */
#endif /* DTV1DEV_SM_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_SM_DEBUG_LV3
#define M_DEBUG_SM_MSGLV3( fmt, args... )                 /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_SM_MSGLV3( fmt, args... )          /*pgr0671 */
#endif /* DTV1DEV_SM_DEBUG_LV3 */

/******************************************************************************/
/* [04] (ADCNV)                                       */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_ADCNV_DEBUG_LV1
#define M_DEBUG_ADCNV_MSGLV1( fmt, args... )              /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_ADCNV_MSGLV1( fmt, args... )       /*pgr0671 */
#endif /* DTV1DEV_ADCNV_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_ADCNV_DEBUG_LV2
#define M_DEBUG_ADCNV_MSGLV2( fmt, args... )              /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_ADCNV_MSGLV2( fmt, args... )       /*pgr0671 */
#endif /* DTV1DEV_ADCNV_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_ADCNV_DEBUG_LV3
#define M_DEBUG_ADCNV_MSGLV3( fmt, args... )              /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_ADCNV_MSGLV3( fmt, args... )       /*pgr0671 */
#endif /* DTV1DEV_ADCNV_DEBUG_LV3 */

/******************************************************************************/
/* [05] (POWFAC)                                      */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_POWFAC_DEBUG_LV1
#define M_DEBUG_POWFAC_MSGLV1( fmt, args... )             /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_POWFAC_MSGLV1( fmt, args... )      /*pgr0671 */
#endif /* DTV1DEV_POWFAC_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_POWFAC_DEBUG_LV2
#define M_DEBUG_POWFAC_MSGLV2( fmt, args... )             /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_POWFAC_MSGLV2( fmt, args... )      /*pgr0671 */
#endif /* DTV1DEV_POWFAC_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_POWFAC_DEBUG_LV3
#define M_DEBUG_POWFAC_MSGLV3( fmt, args... )             /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_POWFAC_MSGLV3( fmt, args... )      /*pgr0671 */
#endif /* DTV1DEV_POWFAC_DEBUG_LV3 */

/******************************************************************************/
/* [06] (BATCHG)                                      */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_BATCHG_DEBUG_LV1
#define M_DEBUG_BATCHG_MSGLV1( fmt, args... )             /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_BATCHG_MSGLV1( fmt, args... )      /*pgr0671 */
#endif /* DTV1DEV_BATCHG_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_BATCHG_DEBUG_LV2
#define M_DEBUG_BATCHG_MSGLV2( fmt, args... )             /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_BATCHG_MSGLV2( fmt, args... )      /*pgr0671 */
#endif /* DTV1DEV_BATCHG_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_BATCHG_DEBUG_LV3
#define M_DEBUG_BATCHG_MSGLV3( fmt, args... )             /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_BATCHG_MSGLV3( fmt, args... )      /*pgr0671 */
#endif /* DTV1DEV_BATCHG_DEBUG_LV3 */

/******************************************************************************/
/* [07] (TEMP)                                          */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_TEMP_DEBUG_LV1
#define M_DEBUG_TEMP_MSGLV1( fmt, args... )               /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_TEMP_MSGLV1( fmt, args... )        /*pgr0671 */
#endif /* DTV1DEV_TEMP_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_TEMP_DEBUG_LV2
#define M_DEBUG_TEMP_MSGLV2( fmt, args... )               /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_TEMP_MSGLV2( fmt, args... )        /*pgr0671 */
#endif /* DTV1DEV_TEMP_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_TEMP_DEBUG_LV3
#define M_DEBUG_TEMP_MSGLV3( fmt, args... )               /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_TEMP_MSGLV3( fmt, args... )        /*pgr0671 */
#endif /* DTV1DEV_TEMP_DEBUG_LV3 */

/******************************************************************************/
/* [08]                                         */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_LMP_DEBUG_LV1
#define M_DEBUG_LMP_MSGLV1( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_LMP_MSGLV1( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_LMP_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_LMP_DEBUG_LV2
#define M_DEBUG_LMP_MSGLV2( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_LMP_MSGLV2( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_LMP_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DTV1DEV_LMP_DEBUG_LV3
#define M_DEBUG_LMP_MSGLV3( fmt, args... )                /*pgr0671 */\
  printk( KERN_DEBUG "%s %d %s : " fmt,                                        \
          __FILE__,__LINE__,__FUNCTION__,## args )                             
#else
#       define M_DEBUG_LMP_MSGLV3( fmt, args... )         /*pgr0671 */
#endif /* DTV1DEV_LMP_DEBUG_LV3 */

/*<D1IO095>*//*----------------------------------------------------------------------------*/
/*<D1IO095>*//*                    */
/*<D1IO095>*//*----------------------------------------------------------------------------*/
/*<D1IO095>#ifdef  DEV_DEBUG_LV1                                                      */
/*<D1IO095>#       define M_DEBUG_MSGLV1( fmt, args... )                                 \ */
/*<D1IO095>                   printk( KERN_DEBUG "%s %s %s %d %s : " fmt,__DATE__,       \ */
/*<D1IO095>                           __TIME__,__FILE__,__LINE__,__FUNCTION__,## args )    */
/*<D1IO095>#else                                                                           */
/*<D1IO095>#       define M_DEBUG_MSGLV1( fmt, args... )                                   */
/*<D1IO095>#endif                                                                          */

/*<D1IO095>*//*----------------------------------------------------------------------------*/
/*<D1IO095>*//*                    */
/*<D1IO095>*//*----------------------------------------------------------------------------*/
/*<D1IO095>#ifdef  DEV_DEBUG_LV2                                                      */
/*<D1IO095>#       define M_DEBUG_MSGLV2( fmt, args... )                                 \ */
/*<D1IO095>                   printk( KERN_DEBUG "%s %s %s %d %s : " fmt,__DATE__,       \ */
/*<D1IO095>                           __TIME__,__FILE__,__LINE__,__FUNCTION__,## args )    */
/*<D1IO095>#else                                                                           */
/*<D1IO095>#       define M_DEBUG_MSGLV2( fmt, args... )                                   */
/*<D1IO095>#endif                                                                          */

/*<D1IO095>*//*----------------------------------------------------------------------------*/
/*<D1IO095>*//*                    */
/*<D1IO095>*//*----------------------------------------------------------------------------*/
/*<D1IO095>#ifdef  DEV_DEBUG_LV3                                                      */
/*<D1IO095>#       define M_DEBUG_MSGLV3( fmt, args... )                                 \ */
/*<D1IO095>                   printk( KERN_DEBUG "%s %s %s %d %s : " fmt,__DATE__,       \ */
/*<D1IO095>                           __TIME__,__FILE__,__LINE__,__FUNCTION__,## args )    */
/*<D1IO095>#else                                                                           */
/*<D1IO095>#       define M_DEBUG_MSGLV3( fmt, args... )                                   */
/*<D1IO095>#endif                                                                          */

/*<D1IO095>DATE/TIME                                                    */
/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DEV_DEBUG_LV1
#define M_DEBUG_MSGLV1( fmt, args... )                    /*pgr0671 */\
    printk( KERN_DEBUG "%s %d %s : " fmt,                                      \
            __FILE__,__LINE__,__FUNCTION__,## args )
#else
#       define M_DEBUG_MSGLV1( fmt, args... )             /*pgr0671 */
#endif /* DEV_DEBUG_LV1 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DEV_DEBUG_LV2
#define M_DEBUG_MSGLV2( fmt, args... )                    /*pgr0671 */\
    printk( KERN_DEBUG "%s %d %s : " fmt,                                      \
            __FILE__,__LINE__,__FUNCTION__,## args )
#else
#       define M_DEBUG_MSGLV2( fmt, args... )             /*pgr0671 */
#endif /* DEV_DEBUG_LV2 */

/*----------------------------------------------------------------------------*/
/*                    */
/*----------------------------------------------------------------------------*/
#ifdef  DEV_DEBUG_LV3
#define M_DEBUG_MSGLV3( fmt, args... )                    /*pgr0671 */\
    printk( KERN_DEBUG "%s %d %s : " fmt,                                      \
            __FILE__,__LINE__,__FUNCTION__,## args )
#else
#       define M_DEBUG_MSGLV3( fmt, args... )             /*pgr0671 */
#endif /* DEV_DEBUG_LV3 */

#endif                                           /* _DEBUG_DEVMSG_H_          */
/******************************************************************************/
/*         Unpublished Copyright(c)  2004             */
/******************************************************************************/

