/*
*************************************************************************************
*                         			      Linux
*					                 USB Host Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_usb_debug.h
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_USB_DEBUG_H__
#define  __SW_USB_DEBUG_H__

#define  DMSG_PRINT(stuff...)		printk(stuff)
#define  DMSG_ERR(...)        		(DMSG_PRINT("WRN:L%d(%s):", __LINE__, __FILE__), DMSG_PRINT(__VA_ARGS__))
#define  DMSG_HERE					(DMSG_PRINT("here:L%d(%s)\n", __LINE__, __FILE__));
#define  DMSG_INFO_EX(...)			(DMSG_PRINT("[sw_udc]: "), DMSG_PRINT(__VA_ARGS__))

/* 测试 */
#if  0
    #define DMSG_TEST         			DMSG_PRINT
#else
    #define DMSG_TEST(...)
#endif

/* 代码调试 */
#if  0
    #define DMSG_DEBUG        			DMSG_PRINT
#else
    #define DMSG_DEBUG(...)
#endif

/* 普通信息打印 */
#if  1
    #define DMSG_INFO         			DMSG_PRINT
#else
    #define DMSG_INFO(...)
#endif

/* 严重警告 */
#if	1
    #define DMSG_PANIC        			DMSG_ERR
#else
    #define DMSG_PANIC(...)
#endif

/* 普通警告 */
#if	0
    #define DMSG_WRN        			DMSG_ERR
#else
    #define DMSG_WRN(...)
#endif

/* dma 调试打印 */
#if	0
    #define DMSG_DBG_DMA     			DMSG_PRINT
#else
    #define DMSG_DBG_DMA(...)
#endif

//---------------------------------------------------------------
//  函数 定义
//---------------------------------------------------------------
void print_usb_reg_by_ep(spinlock_t *lock, __u32 usbc_base, __s32 ep_index, char *str);
void print_all_usb_reg(spinlock_t *lock, __u32 usbc_base, __s32 ep_start, __u32 ep_end, char *str);

#endif   //__SW_USB_DEBUG_H__

