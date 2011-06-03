/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_board.c
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include  "sw_udc_config.h"
#include  "sw_udc_board.h"

//---------------------------------------------------------------
//  宏 定义
//---------------------------------------------------------------

#define res_size(_r) (((_r)->end - (_r)->start) + 1)

/*
*******************************************************************************
*                     open_usb_clock
*
* Description:
*
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
u32  open_usb_clock(sw_udc_io_t *sw_udc_io)
{
    u32 reg_value = 0;
    u32 ccmu_base = SW_VA_CCM_IO_BASE;
    
 	DMSG_INFO_EX("open_usb_clock\n");
 	
    //Gating AHB clock for USB_phy0
	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value |= (1 << 0);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

    //delay to wati SIE stable
	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy0
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	reg_value |= (1 << 8);
	reg_value |= (1 << 0);          //disable reset
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	//delay some time 
	reg_value = 10000;
	while(reg_value--);
  sw_udc_io->clk_is_open = 1;
	return 0;
}

/*
*******************************************************************************
*                     close_usb_clock
*
* Description:
*
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
u32 close_usb_clock(sw_udc_io_t *sw_udc_io)
{
    u32 reg_value = 0;
    u32 ccmu_base = SW_VA_CCM_IO_BASE;
    
	DMSG_INFO_EX("close_usb_clock\n");
	
    //Gating AHB clock for USB_phy0
	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value &= ~(1 << 0);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

    //等sie的时钟变稳
	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy0
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	reg_value &= ~(1 << 8);
	reg_value &= ~(1 << 0);          //disable reset
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	//延时
	reg_value = 10000;
	while(reg_value--);
  sw_udc_io->clk_is_open = 0;
	return 0;

}

/*
*******************************************************************************
*                     sw_udc_bsp_init
*
* Description:
*    initialize usb bsp
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 sw_udc_bsp_init(__u32 usbc_no, sw_udc_io_t *sw_udc_io)
{
	sw_udc_io->usbc.usbc_info[usbc_no].num = usbc_no;
   	sw_udc_io->usbc.usbc_info[usbc_no].base = (u32)sw_udc_io->usb_vbase;
	sw_udc_io->usbc.sram_base = (u32)sw_udc_io->sram_vbase;

	USBC_init(&sw_udc_io->usbc);
	sw_udc_io->usb_bsp_hdle = USBC_open_otg(usbc_no);
	if(sw_udc_io->usb_bsp_hdle == 0){
		DMSG_PANIC("ERR: sw_udc_init: USBC_open_otg failed\n");
		return -1;
	}

	USBC_EnhanceSignal(sw_udc_io->usb_bsp_hdle);

	USBC_EnableDpDmPullUp(sw_udc_io->usb_bsp_hdle);
    USBC_EnableIdPullUp(sw_udc_io->usb_bsp_hdle);
	USBC_ForceId(sw_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DEVICE);
	USBC_ForceVbusValid(sw_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

	USBC_SelectBus(sw_udc_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

    return 0;
}

/*
*******************************************************************************
*                     sw_udc_bsp_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 sw_udc_bsp_exit(__u32 usbc_no, sw_udc_io_t *sw_udc_io)
{
    USBC_DisableDpDmPullUp(sw_udc_io->usb_bsp_hdle);
    USBC_DisableIdPullUp(sw_udc_io->usb_bsp_hdle);
	USBC_ForceId(sw_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DISABLE);
	USBC_ForceVbusValid(sw_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_DISABLE);

	USBC_close_otg(sw_udc_io->usb_bsp_hdle);
	USBC_exit(&sw_udc_io->usbc);

    return 0;
}

/*
*******************************************************************************
*                     sw_udc_io_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 sw_udc_io_init(__u32 usbc_no, struct platform_device *pdev, sw_udc_io_t *sw_udc_io)
{
	__s32 			ret 	= 0;
	unsigned int	iosize 	= 0;

	/* usb */
	sw_udc_io->usb_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(sw_udc_io->usb_base_res == NULL){
		DMSG_PANIC("ERR: sw_udc_init, platform_get_resource for usb failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	iosize = res_size(sw_udc_io->usb_base_res);
	sw_udc_io->usb_base_req = request_mem_region(sw_udc_io->usb_base_res->start, iosize, pdev->name);
	if(sw_udc_io->usb_base_req == NULL){
		DMSG_PANIC("ERR: sw_udc_init, request_mem_region for usb failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_udc_io->usb_vbase = ioremap(sw_udc_io->usb_base_res->start, iosize);
	if(sw_udc_io->usb_vbase == NULL){
		DMSG_PANIC("ERR: sw_udc_init, ioremap for usb failed\n");
		ret = -ENOMEM;
	}

	/* sram */
	sw_udc_io->sram_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(sw_udc_io->sram_base_res == NULL){
		DMSG_PANIC("ERR: sw_udc_init, platform_get_resource for sram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	iosize = res_size(sw_udc_io->sram_base_res);

	sw_udc_io->sram_base_req = request_mem_region(sw_udc_io->sram_base_res->start, iosize, pdev->name);
	if(sw_udc_io->sram_base_req == NULL){
		DMSG_PANIC("ERR: sw_udc_init, request_mem_region for sram failed\n");
		ret = -ENODEV;
	}

	sw_udc_io->sram_vbase = ioremap(sw_udc_io->sram_base_res->start, iosize);
	if(sw_udc_io->sram_vbase == NULL){
		DMSG_PANIC("ERR: sw_udc_init, ioremap for sram failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	DMSG_INFO_EX("usb_vbase  = 0x%x\n", (u32)sw_udc_io->usb_vbase);
	DMSG_INFO_EX("sram_vbase = 0x%x\n", (u32)sw_udc_io->sram_vbase);

    /* open usb lock */
	sw_udc_io->sie_clk = clk_get(NULL, "ahb_usb0");
	if (IS_ERR(sw_udc_io->sie_clk)){
		DMSG_PANIC("ERR: get usb sie clk failed.\n");
		goto io_failed;
	}

	sw_udc_io->phy_clk = clk_get(NULL, "usb_phy0");
	if (IS_ERR(sw_udc_io->phy_clk)){
		DMSG_PANIC("ERR: get usb phy clk failed.\n");
		goto io_failed;
	}

	open_usb_clock(sw_udc_io);

    /* initialize usb bsp */
	sw_udc_bsp_init(usbc_no, sw_udc_io);

	/* config usb fifo */
	USBC_ConfigFIFO_Base(sw_udc_io->usb_bsp_hdle, (u32)sw_udc_io->sram_vbase, USBC_FIFO_MODE_8K);

    /* 配完 fifo 后, 立即释放sram map */
	if(sw_udc_io->sram_base_res){
		release_resource(sw_udc_io->sram_base_res);
		sw_udc_io->sram_base_res = NULL;
	}

	if(sw_udc_io->sram_base_req){
		release_resource(sw_udc_io->sram_base_req);
		kfree(sw_udc_io->sram_base_req);
		sw_udc_io->sram_base_req = NULL;
	}

	if(sw_udc_io->sram_vbase){
		iounmap(sw_udc_io->sram_vbase);
		sw_udc_io->sram_vbase = NULL;
	}

	return 0;

io_failed:
	/* usb */
	if(sw_udc_io->usb_base_res){
		release_resource(sw_udc_io->usb_base_res);
		sw_udc_io->usb_base_res = NULL;
	}

	if(sw_udc_io->usb_base_req){
		release_resource(sw_udc_io->usb_base_req);
		kfree(sw_udc_io->usb_base_req);
		sw_udc_io->usb_base_req = NULL;
	}

	if(sw_udc_io->usb_vbase){
		iounmap(sw_udc_io->usb_vbase);
		sw_udc_io->usb_vbase = NULL;
	}

	/* sram */
	if(sw_udc_io->sram_base_res){
		release_resource(sw_udc_io->sram_base_res);
		sw_udc_io->sram_base_res = NULL;
	}

	if(sw_udc_io->sram_base_req){
		release_resource(sw_udc_io->sram_base_req);
		kfree(sw_udc_io->sram_base_req);
		sw_udc_io->sram_base_req = NULL;
	}

	if(sw_udc_io->sram_vbase){
		iounmap(sw_udc_io->sram_vbase);
		sw_udc_io->sram_vbase = NULL;
	}

	/* clock */
	if(sw_udc_io->sie_clk){
		clk_put(sw_udc_io->sie_clk);
		sw_udc_io->sie_clk = NULL;
	}

	if(sw_udc_io->phy_clk){
		clk_put(sw_udc_io->phy_clk);
		sw_udc_io->phy_clk = NULL;
	}

	return ret;
}

/*
*******************************************************************************
*                     sw_udc_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 sw_udc_io_exit(__u32 usbc_no, struct platform_device *pdev, sw_udc_io_t *sw_udc_io)
{
	sw_udc_bsp_exit(usbc_no, sw_udc_io);

	close_usb_clock(sw_udc_io);

	/* usb */
	if(sw_udc_io->usb_base_res){
		release_resource(sw_udc_io->usb_base_res);
		sw_udc_io->usb_base_res = NULL;
	}

	if(sw_udc_io->usb_base_req){
		release_resource(sw_udc_io->usb_base_req);
		kfree(sw_udc_io->usb_base_req);
		sw_udc_io->usb_base_req = NULL;
	}

	if(sw_udc_io->usb_vbase){
		iounmap(sw_udc_io->usb_vbase);
		sw_udc_io->usb_vbase = NULL;
	}

	/* sram */
	if(sw_udc_io->sram_base_res){
		release_resource(sw_udc_io->sram_base_res);
		sw_udc_io->sram_base_res = NULL;
	}

	if(sw_udc_io->sram_base_req){
		release_resource(sw_udc_io->sram_base_req);
		kfree(sw_udc_io->sram_base_req);
		sw_udc_io->sram_base_req = NULL;
	}

	if(sw_udc_io->sram_vbase){
		iounmap(sw_udc_io->sram_vbase);
		sw_udc_io->sram_vbase = NULL;
	}

	/* clock */
	if(sw_udc_io->sie_clk){
		clk_put(sw_udc_io->sie_clk);
		sw_udc_io->sie_clk = NULL;
	}

	if(sw_udc_io->phy_clk){
		clk_put(sw_udc_io->phy_clk);
		sw_udc_io->phy_clk = NULL;
	}

	return 0;
}





