/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_board.h
*
* Author 		: javen
*
* Description 	: 板级控制
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_UDC_BOARD_H__
#define  __SW_UDC_BOARD_H__

/* i/o 信息 */
typedef struct sw_udc_io{
	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;			/* USB  base address 	*/

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 	*/

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 	*/

	bsp_usbc_t usbc;					/* usb bsp config 		*/
	__hdle usb_bsp_hdle;				/* usb bsp handle 		*/

	__u32 clk_is_open;					/* is usb clock open? 	*/
	struct clk	*sie_clk;				/* SIE clock handle 	*/
	struct clk	*phy_clk;				/* PHY clock handle 	*/

	long Drv_vbus_Handle;
}sw_udc_io_t;

u32 open_usb_clock(sw_udc_io_t *sw_udc_io);
u32 close_usb_clock(sw_udc_io_t *sw_udc_io);

__s32 sw_udc_io_init(__u32 usbc_no, struct platform_device *pdev, sw_udc_io_t *sw_udc_io);
__s32 sw_udc_io_exit(__u32 usbc_no, struct platform_device *pdev, sw_udc_io_t *sw_udc_io);

#endif   //__SW_UDC_BOARD_H__


