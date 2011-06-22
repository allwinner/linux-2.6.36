/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: ehci0_sun4i.c
*
* Author 		: yangnaitian
*
* Description 	: SoftWinner EHCI0 Driver
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    yangnaitian      2011-5-24            1.0          create this file
*
*************************************************************************************
*/

#include <linux/platform_device.h>

#include <mach/gpio_v2.h>
#include <mach/script_v2.h>
#include <linux/clk.h>

#include "sw_hci_sun4i.h"

/*.......................................................................................*/
//                               全局信息定义
/*.......................................................................................*/

#define  SW_EHCI0_NAME		"sw-ehci0"
static const char ehci0_name[] = SW_EHCI0_NAME;

static struct sw_hci_hcd g_sw_ehci_hcd;


/*.......................................................................................*/
//                                      函数区
/*.......................................................................................*/

extern int usb_disabled(void);

/*
*******************************************************************************
*                     pin_init
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
static __s32 pin_init(struct sw_hci_hcd *sw_ehci)
{
	__s32 ret = 0;

	/* request gpio */
	ret = script_parser_fetch("usbc1", "usb_drv_vbus_gpio", (int *)&sw_ehci->drv_vbus_gpio_set, 64);
	if(ret != 0){
		DMSG_PANIC("ERR: get usbc1(drv vbus) id failed\n");
		return -1;
	}

	sw_ehci->Drv_vbus_Handle = gpio_request(&sw_ehci->drv_vbus_gpio_set, 1);
	if(sw_ehci->Drv_vbus_Handle == 0){
		DMSG_PANIC("ERR: gpio_request failed\n");
		return -1;
	}

	/* set config, ouput */
	gpio_set_one_pin_io_status(sw_ehci->Drv_vbus_Handle, 1, NULL);

	/* reserved is pull down */
	gpio_set_one_pin_pull(sw_ehci->Drv_vbus_Handle, 2, NULL);

	return 0;
}

/*
*******************************************************************************
*                     pin_exit
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
static __s32 pin_exit(struct sw_hci_hcd *sw_ehci)
{
	gpio_release(sw_ehci->Drv_vbus_Handle, 0);
	sw_ehci->Drv_vbus_Handle = 0;

	return 0;
}

/*
*******************************************************************************
*                     ehci_clock_init
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
static s32 ehci_clock_init(struct sw_hci_hcd *sw_ehci)
{
	sw_ehci->sie_clk = clk_get(NULL, "ahb_usb1");
	if (IS_ERR(sw_ehci->sie_clk)){
		DMSG_PANIC("ERR: get usb sie clk failed.\n");
		goto failed;
	}

	sw_ehci->phy_gate = clk_get(NULL, "usb_phy");
	if (IS_ERR(sw_ehci->phy_gate)){
		DMSG_PANIC("ERR: get usb phy_gate failed.\n");
		goto failed;
	}

	sw_ehci->phy_reset = clk_get(NULL, "usb_phy1");
	if (IS_ERR(sw_ehci->phy_reset)){
		DMSG_PANIC("ERR: get usb phy_reset failed.\n");
		goto failed;
	}

	return 0;

failed:
	if(sw_ehci->sie_clk){
		clk_put(sw_ehci->sie_clk);
		sw_ehci->sie_clk = NULL;
	}

	if(sw_ehci->phy_gate){
		clk_put(sw_ehci->phy_gate);
		sw_ehci->phy_gate = NULL;
	}

	if(sw_ehci->phy_reset){
		clk_put(sw_ehci->phy_reset);
		sw_ehci->phy_reset = NULL;
	}

	return -1;
}

/*
*******************************************************************************
*                     ehci_clock_exit
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
static s32 ehci_clock_exit(struct sw_hci_hcd *sw_ehci)
{
	if(sw_ehci->sie_clk){
		clk_put(sw_ehci->sie_clk);
		sw_ehci->sie_clk = NULL;
	}

	if(sw_ehci->phy_gate){
		clk_put(sw_ehci->phy_gate);
		sw_ehci->phy_gate = NULL;
	}

	if(sw_ehci->phy_reset){
		clk_put(sw_ehci->phy_reset);
		sw_ehci->phy_reset = NULL;
	}

	return 0;
}

/*
*******************************************************************************
*                     open_ehci_clock
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
static int open_ehci_clock(struct sw_hci_hcd *sw_ehci)
{
 	DMSG_INFO("[%s]: open_ehci_clock\n", SW_EHCI0_NAME);

	if(sw_ehci->sie_clk && sw_ehci->phy_gate && sw_ehci->phy_reset && !sw_ehci->clk_is_open){
	   	clk_enable(sw_ehci->sie_clk);
		msleep(10);

	    clk_enable(sw_ehci->phy_gate);
	    clk_enable(sw_ehci->phy_reset);
		clk_reset(sw_ehci->phy_reset, 0);
		msleep(10);

		sw_ehci->clk_is_open = 1;
	}else{
		DMSG_INFO("ERR: open ehci clock failed, (0x%p, 0x%p, 0x%p, %d)\n", 
			      sw_ehci->sie_clk, sw_ehci->phy_gate, sw_ehci->phy_reset, sw_ehci->clk_is_open);
	}

	return 0;
}

/*
*******************************************************************************
*                     close_ehci_clock
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
static int close_ehci_clock(struct sw_hci_hcd *sw_ehci)
{
 	DMSG_INFO("[%s]: close_ehci_clock\n", SW_EHCI0_NAME);

	if(sw_ehci->sie_clk && sw_ehci->phy_gate && sw_ehci->phy_reset && sw_ehci->clk_is_open){
		clk_reset(sw_ehci->phy_reset, 1);
	    clk_disable(sw_ehci->phy_reset);
	    clk_disable(sw_ehci->phy_gate);
	    clk_disable(sw_ehci->sie_clk);
		sw_ehci->clk_is_open = 0;
	}else{
		DMSG_INFO("ERR: close ehci clock failed, (0x%p, 0x%p, 0x%p, %d)\n", 
			      sw_ehci->sie_clk, sw_ehci->phy_gate, sw_ehci->phy_reset, sw_ehci->clk_is_open);
	}

	return 0;
}

/*
*******************************************************************************
*                     sw_hcd_board_set_vbus
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
static void sw_hcd_board_set_vbus(struct sw_hci_hcd *sw_ehci, int is_on)
{
	/* set gpio data */
	if(is_on){
		gpio_write_one_pin_value(sw_ehci->Drv_vbus_Handle, 1, NULL);
		DMSG_INFO("INFO : USB VBus for EHCI power ON\n");
	}else{
		gpio_write_one_pin_value(sw_ehci->Drv_vbus_Handle, 0, NULL);
		DMSG_INFO("INFO : USB VBus for EHCI power OFF\n");
	}

	return;
}

/*
*******************************************************************************
*                     sw_start_ehc
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
static void sw_start_ehc(struct sw_hci_hcd *sw_ehci)
{
	unsigned long reg_value = 0;

  	open_ehci_clock(sw_ehci);

	/*enable passby*/
	reg_value = USBC_Readl(sw_ehci->usb_vbase + SW_USB_PMU_IRQ_ENABLE); 
	reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
	reg_value |= (1 << 9);     	/* AHB Master interface burst type INCR4 enable */
	reg_value |= (1 << 8);     	/* AHB Master interface INCRX align enable */
	reg_value |= (1 << 0);     	/* ULPI bypass enable */
	USBC_Writel(reg_value, (sw_ehci->usb_vbase + SW_USB_PMU_IRQ_ENABLE)); 

	/* ehci port configure */
	reg_value = USBC_Readl(sw_ehci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1);
	reg_value |= (1 << SW_SDRAM_BP_HPCR_ACCESS_EN);
	USBC_Writel(reg_value, (sw_ehci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1));

	sw_hcd_board_set_vbus(sw_ehci, 1);

	return;
}

/*
*******************************************************************************
*                     sw_stop_ehc
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
static void sw_stop_ehc(struct sw_hci_hcd *sw_ehci)
{
	unsigned long reg_value = 0;

	sw_hcd_board_set_vbus(sw_ehci, 0);

	/* ehci port configure */
	reg_value = USBC_Readl(sw_ehci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1);
	reg_value &= ~(1 << SW_SDRAM_BP_HPCR_ACCESS_EN);
	USBC_Writel(reg_value, (sw_ehci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1));

	/*disable passby*/
	reg_value = USBC_Readl(sw_ehci->usb_vbase + SW_USB_PMU_IRQ_ENABLE); 
	reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
	reg_value &= ~(1 << 9);     /* AHB Master interface burst type INCR4 disable */
	reg_value &= ~(1 << 8);     /* AHB Master interface INCRX align disable */
	reg_value &= ~(1 << 0);     /* ULPI bypass disable */
	USBC_Writel(reg_value, (sw_ehci->usb_vbase + SW_USB_PMU_IRQ_ENABLE)); 

	close_ehci_clock(sw_ehci);

	return;
}

/*
*******************************************************************************
*                     sw_ehci_setup
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
static int sw_ehci_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int ret = ehci_init(hcd);

	ehci->need_io_watchdog = 0;

	return ret;
}

static const struct hc_driver sw_ehci0_hc_driver = {
	.description			= hcd_name,
	.product_desc			= "SW USB2.0 'Enhanced' Host Controller (EHCI) Driver",
	.hcd_priv_size			= sizeof(struct ehci_hcd),

	 /*
	 * generic hardware linkage
	 */
	 .irq					=  ehci_irq,
	 .flags					=  HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset					= sw_ehci_setup,
	.start					= ehci_run,
	.stop					= ehci_stop,
	.shutdown				= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue			= ehci_urb_enqueue,
	.urb_dequeue			= ehci_urb_dequeue,
	.endpoint_disable		= ehci_endpoint_disable,
	.endpoint_reset			= ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number		= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data		= ehci_hub_status_data,
	.hub_control			= ehci_hub_control,
	.bus_suspend			= ehci_bus_suspend,
	.bus_resume				= ehci_bus_resume,
	.relinquish_port		= ehci_relinquish_port,
	.port_handed_over		= ehci_port_handed_over,
 
	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

#if 0

/*
*******************************************************************************
*                     sw_get_io_resource
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
static int sw_get_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ehci)
{
	__s32 ret = 0;

	sw_ehci->irq_no = SW_INT_SRC_EHCI0;

	/*usb pass by enable */
	sw_ehci->usb_passby_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 5);
	if(sw_ehci->usb_passby_base_res == NULL)
		{
			DMSG_PANIC("ERR: platform_get_resource for usb pass by failed\n");
		  ret = -ENODEV;
		  goto io_failed;
		}
		
	sw_ehci->usb_passby_reg_start  = sw_ehci->usb_passby_base_res->start;
	sw_ehci->usb_passby_reg_length = resource_size(sw_ehci->usb_passby_base_res);
	sw_ehci->usb_passby_base_req = request_mem_region(sw_ehci->usb_passby_base_res->start, sw_ehci->usb_passby_reg_length, pdev->name);
	if(sw_ehci->usb_passby_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for usb pass by failed\n");
		ret = -ENODEV;
		goto io_failed;
	 }

	 sw_ehci->usb_passby_vbase = ioremap(sw_ehci->usb_passby_base_res->start, sw_ehci->usb_passby_reg_length);
	 if(sw_ehci->usb_passby_vbase == NULL){
		 DMSG_PANIC("ERR: ioremap for usb pass by failed\n");
		 ret = -ENOMEM;
		 goto io_failed;
	  }
	
	/* USB */
	sw_ehci->usb_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(sw_ehci->usb_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for usb failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->usb_reg_start  = sw_ehci->usb_base_res->start;
	sw_ehci->usb_reg_length = resource_size(sw_ehci->usb_base_res);
	sw_ehci->usb_base_req = request_mem_region(sw_ehci->usb_base_res->start, sw_ehci->usb_reg_length, pdev->name);
	if(sw_ehci->usb_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for usb failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->usb_vbase = ioremap(sw_ehci->usb_base_res->start, sw_ehci->usb_reg_length);
	if(sw_ehci->usb_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for usb failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	/* sram */
	sw_ehci->sram_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(sw_ehci->sram_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for sram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->sram_reg_start  = sw_ehci->sram_base_res->start;
	sw_ehci->sram_reg_length = resource_size(sw_ehci->sram_base_res);
	sw_ehci->sram_base_req = request_mem_region(sw_ehci->sram_base_res->start, sw_ehci->sram_reg_length, pdev->name);
	if(sw_ehci->sram_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for sram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->sram_vbase = ioremap(sw_ehci->sram_base_res->start, sw_ehci->sram_reg_length);
	if(sw_ehci->sram_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for sram failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	/* clock */
	sw_ehci->clock_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if(sw_ehci->clock_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for clock failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->clock_reg_start  = sw_ehci->clock_base_res->start;
	sw_ehci->clock_reg_length = resource_size(sw_ehci->clock_base_res);
	sw_ehci->clock_base_req = request_mem_region(sw_ehci->clock_base_res->start, sw_ehci->clock_reg_length, pdev->name);
	if(sw_ehci->clock_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for clock failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->clock_vbase = ioremap(sw_ehci->clock_base_res->start, sw_ehci->clock_reg_length);
	if(sw_ehci->clock_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for clock failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	/* gpio */
	sw_ehci->gpio_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if(sw_ehci->gpio_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for gpio failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->gpio_reg_start  = sw_ehci->gpio_base_res->start;
	sw_ehci->gpio_reg_length = resource_size(sw_ehci->gpio_base_res);
	sw_ehci->gpio_base_req = request_mem_region(sw_ehci->gpio_base_res->start, sw_ehci->gpio_reg_length, pdev->name);
	if(sw_ehci->gpio_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for gpio failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->gpio_vbase = ioremap(sw_ehci->gpio_base_res->start, sw_ehci->gpio_reg_length);
	if(sw_ehci->gpio_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for gpio failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	/* sdram */
	sw_ehci->sdram_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	if(sw_ehci->sdram_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for sdram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->sdram_reg_start  = sw_ehci->sdram_base_res->start;
	sw_ehci->sdram_reg_length = resource_size(sw_ehci->sdram_base_res);
	sw_ehci->sdram_base_req = request_mem_region(sw_ehci->sdram_base_res->start, sw_ehci->sdram_reg_length, pdev->name);
	if(sw_ehci->sdram_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for sdram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ehci->sdram_vbase = ioremap(sw_ehci->sdram_base_res->start, sw_ehci->sdram_reg_length);
	if(sw_ehci->sdram_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for sdram failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	DMSG_INFO("[%s]: usb_vbase = 0x%x, sram_vbase = 0x%x, clock_vbase = 0x%x, gpio_vbase = 0x%x, sdram_vbase = 0x%x\n", 
		   ehci0_name, 
		   sw_ehci->usb_vbase,
		   sw_ehci->sram_vbase,
		   sw_ehci->clock_vbase,
		   sw_ehci->gpio_vbase,
		   sw_ehci->sdram_vbase);

	return 0;

io_failed:
	/*USB PASS BY*/
	if(sw_ehci->usb_passby_vbase){
		iounmap(sw_ehci->usb_passby_vbase);
		sw_ehci->usb_passby_vbase = NULL;
	}

	if(sw_ehci->usb_passby_base_req){
		release_mem_region(sw_ehci->usb_passby_base_req->start, sw_ehci->usb_passby_reg_length);
		sw_ehci->usb_passby_base_req = NULL;
	}

	if(sw_ehci->usb_passby_base_res){
		release_resource(sw_ehci->usb_passby_base_res);
		sw_ehci->usb_passby_base_res = NULL;
	}

	sw_ehci->usb_passby_reg_start  = 0;
	sw_ehci->usb_passby_reg_length = 0;
	
	/* USB */
	if(sw_ehci->usb_vbase){
		iounmap(sw_ehci->usb_vbase);
		sw_ehci->usb_vbase = NULL;
	}

	if(sw_ehci->usb_base_req){
		release_mem_region(sw_ehci->usb_base_res->start, sw_ehci->usb_reg_length);
		sw_ehci->usb_base_req = NULL;
	}

	if(sw_ehci->usb_base_res){
		release_resource(sw_ehci->usb_base_res);
		sw_ehci->usb_base_res = NULL;
	}

	sw_ehci->usb_reg_start  = 0;
	sw_ehci->usb_reg_length = 0;

	/* sram */
	if(sw_ehci->sram_vbase){
		iounmap(sw_ehci->sram_vbase);
		sw_ehci->sram_vbase = NULL;
	}

	if(sw_ehci->sram_base_req){
		release_mem_region(sw_ehci->sram_base_res->start, sw_ehci->sram_reg_length);
		sw_ehci->sram_base_req = NULL;
	}

	if(sw_ehci->sram_base_res){
		release_resource(sw_ehci->sram_base_res);
		sw_ehci->sram_base_res = NULL;
	}

	sw_ehci->sram_reg_start  = 0;
	sw_ehci->sram_reg_length = 0;

	/* clock */
	if(sw_ehci->clock_vbase){
		iounmap(sw_ehci->clock_vbase);
		sw_ehci->clock_vbase = NULL;
	}

	if(sw_ehci->clock_base_req){
		release_mem_region(sw_ehci->clock_base_res->start, sw_ehci->clock_reg_length);
		sw_ehci->clock_base_req = NULL;
	}

	if(sw_ehci->clock_base_res){
		release_resource(sw_ehci->clock_base_res);
		sw_ehci->clock_base_res = NULL;
	}

	sw_ehci->clock_reg_start  = 0;
	sw_ehci->clock_reg_length = 0;

	/* gpio */
	if(sw_ehci->gpio_vbase){
		iounmap(sw_ehci->gpio_vbase);
		sw_ehci->gpio_vbase = NULL;
	}

	if(sw_ehci->gpio_base_req){
		release_mem_region(sw_ehci->gpio_base_res->start, sw_ehci->gpio_reg_length);
		sw_ehci->gpio_base_req = NULL;
	}

	if(sw_ehci->gpio_base_res){
		release_resource(sw_ehci->gpio_base_res);
		sw_ehci->gpio_base_res = NULL;
	}

	sw_ehci->gpio_reg_start  = 0;
	sw_ehci->gpio_reg_length = 0;

	/* sdram */
	if(sw_ehci->sdram_vbase){
		iounmap(sw_ehci->sdram_vbase);
		sw_ehci->sdram_vbase = NULL;
	}

	if(sw_ehci->sdram_base_req){
		release_mem_region(sw_ehci->sdram_base_res->start, sw_ehci->sdram_reg_length);
		sw_ehci->sdram_base_req = NULL;
	}

	if(sw_ehci->sdram_base_res){
		release_resource(sw_ehci->sdram_base_res);
		sw_ehci->sdram_base_res = NULL;
	}

	sw_ehci->sdram_reg_start  = 0;
	sw_ehci->sdram_reg_length = 0;

	return ret;
}

/*
*******************************************************************************
*                     sw_release_io_resource
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
static int sw_release_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ehci)
{
	/* USB pass by */
	if(sw_ehci->usb_passby_vbase){
		iounmap(sw_ehci->usb_passby_vbase);
		sw_ehci->usb_passby_vbase = NULL;
	}

	if(sw_ehci->usb_passby_base_req){
		release_mem_region(sw_ehci->usb_passby_base_res->start, sw_ehci->usb_passby_reg_length);
		sw_ehci->usb_passby_base_req = NULL;
	}

	if(sw_ehci->usb_passby_base_res){
		release_resource(sw_ehci->usb_passby_base_res);
		sw_ehci->usb_passby_base_res = NULL;
	}

	sw_ehci->usb_passby_reg_start  = 0;
	sw_ehci->usb_passby_reg_length = 0;
	
	/* USB */
	if(sw_ehci->usb_vbase){
		iounmap(sw_ehci->usb_vbase);
		sw_ehci->usb_vbase = NULL;
	}

	if(sw_ehci->usb_base_req){
		release_mem_region(sw_ehci->usb_base_res->start, sw_ehci->usb_reg_length);
		sw_ehci->usb_base_req = NULL;
	}

	if(sw_ehci->usb_base_res){
		release_resource(sw_ehci->usb_base_res);
		sw_ehci->usb_base_res = NULL;
	}

	sw_ehci->usb_reg_start  = 0;
	sw_ehci->usb_reg_length = 0;

	/* sram */
	if(sw_ehci->sram_vbase){
		iounmap(sw_ehci->sram_vbase);
		sw_ehci->sram_vbase = NULL;
	}

	if(sw_ehci->sram_base_req){
		release_mem_region(sw_ehci->sram_base_res->start, sw_ehci->sram_reg_length);
		sw_ehci->sram_base_req = NULL;
	}

	if(sw_ehci->sram_base_res){
		release_resource(sw_ehci->sram_base_res);
		sw_ehci->sram_base_res = NULL;
	}

	sw_ehci->sram_reg_start  = 0;
	sw_ehci->sram_reg_length = 0;

	/* clock */
	if(sw_ehci->clock_vbase){
		iounmap(sw_ehci->clock_vbase);
		sw_ehci->clock_vbase = NULL;
	}

	if(sw_ehci->clock_base_req){
		release_mem_region(sw_ehci->clock_base_res->start, sw_ehci->clock_reg_length);
		sw_ehci->clock_base_req = NULL;
	}

	if(sw_ehci->clock_base_res){
		release_resource(sw_ehci->clock_base_res);
		sw_ehci->clock_base_res = NULL;
	}

	sw_ehci->clock_reg_start  = 0;
	sw_ehci->clock_reg_length = 0;

	/* gpio */
	if(sw_ehci->gpio_vbase){
		iounmap(sw_ehci->gpio_vbase);
		sw_ehci->gpio_vbase = NULL;
	}

	if(sw_ehci->gpio_base_req){
		release_mem_region(sw_ehci->gpio_base_res->start, sw_ehci->gpio_reg_length);
		sw_ehci->gpio_base_req = NULL;
	}

	if(sw_ehci->gpio_base_res){
		release_resource(sw_ehci->gpio_base_res);
		sw_ehci->gpio_base_res = NULL;
	}

	sw_ehci->gpio_reg_start  = 0;
	sw_ehci->gpio_reg_length = 0;

	/* sdram */
	if(sw_ehci->sdram_vbase){
		iounmap(sw_ehci->sdram_vbase);
		sw_ehci->sdram_vbase = NULL;
	}

	if(sw_ehci->sdram_base_req){
		release_mem_region(sw_ehci->sdram_base_res->start, sw_ehci->sdram_reg_length);
		sw_ehci->sdram_base_req = NULL;
	}

	if(sw_ehci->sdram_base_res){
		release_resource(sw_ehci->sdram_base_res);
		sw_ehci->sdram_base_res = NULL;
	}

	sw_ehci->sdram_reg_start  = 0;
	sw_ehci->sdram_reg_length = 0;

	return 0;
}

#else

static int sw_get_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ehci)
{
	sw_ehci->irq_no = SW_INT_SRC_EHCI0;

	sw_ehci->usb_vbase		= (void __iomem	*)SW_VA_USB1_IO_BASE;
	sw_ehci->sram_vbase		= (void __iomem	*)SW_VA_SRAM_IO_BASE;
	sw_ehci->clock_vbase	= (void __iomem	*)SW_VA_CCM_IO_BASE;
	sw_ehci->gpio_vbase		= (void __iomem	*)SW_VA_PORTC_IO_BASE;
	sw_ehci->sdram_vbase	= (void __iomem	*)SW_VA_DRAM_IO_BASE;

	DMSG_INFO("[%s]: usb_vbase = 0x%p, sram_vbase = 0x%p, clock_vbase = 0x%p, gpio_vbase = 0x%p, sdram_vbase = 0x%p,get_io_resource_finish\n", 
		   ehci0_name, 
		   sw_ehci->usb_vbase,
		   sw_ehci->sram_vbase,
		   sw_ehci->clock_vbase,
		   sw_ehci->gpio_vbase,
		   sw_ehci->sdram_vbase);

	return 0;
}

static int sw_release_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ehci)
{

	sw_ehci->irq_no = 0;

	sw_ehci->usb_vbase		= NULL;
	sw_ehci->sram_vbase		= NULL;
	sw_ehci->clock_vbase	= NULL;
	sw_ehci->gpio_vbase		= NULL;
	sw_ehci->sdram_vbase	= NULL;

	return 0;
}

#endif

/*
*******************************************************************************
*                     sw_ehci_hcd_probe
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
static int sw_ehci_hcd_probe(struct platform_device *pdev)
{
	struct usb_hcd 	*hcd 	= NULL;
	struct ehci_hcd *ehci	= NULL;
	int ret = 0;

	/* if usb is disabled, can not probe */
	if (usb_disabled()) {
		DMSG_PANIC("ERR: usb hcd is disabled\n");
		return -ENODEV;
	}

	memset(&g_sw_ehci_hcd, 0, sizeof(struct sw_hci_hcd));

	ret = pin_init(&g_sw_ehci_hcd);
	if(ret != 0){
		DMSG_PANIC("ERR: pin_init failed\n");
		return -ENODEV;
	}

	ret = ehci_clock_init(&g_sw_ehci_hcd);
	if(ret != 0){
		DMSG_PANIC("ERR: ehci_clock_init failed\n");
		ret = -ENODEV;
		goto ERR0;
	}

	/* creat a usb_hcd for the ehci controller */
	hcd = usb_create_hcd(&sw_ehci0_hc_driver, &pdev->dev, "Au1xxx");
	if (!hcd){
		DMSG_PANIC("ERR: usb_create_hcd failed\n");
		ret = -ENOMEM;
		goto ERR1;
	}

	/* get io resource */
	sw_get_io_resource(pdev, &g_sw_ehci_hcd);
	g_sw_ehci_hcd.ehci_base = g_sw_ehci_hcd.usb_vbase + SW_USB_EHCI_BASE_OFFSET;
	g_sw_ehci_hcd.ehci_reg_length = SW_USB_EHCI_LEN;

  	hcd->rsrc_start = (u32)g_sw_ehci_hcd.ehci_base;
	hcd->rsrc_len = g_sw_ehci_hcd.ehci_reg_length;
	hcd->regs = g_sw_ehci_hcd.ehci_base;

	/* echi start to work */
	sw_start_ehc(&g_sw_ehci_hcd);

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
  
	/* cache this readonly data, minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	ret = usb_add_hcd(hcd, g_sw_ehci_hcd.irq_no, IRQF_DISABLED | IRQF_SHARED);
	if (ret != 0) {
		DMSG_PANIC("ERR: usb_add_hcd failed\n");
		ret = -ENOMEM;
		goto ERR2;
	}

	platform_set_drvdata(pdev, hcd);

	return 0;

ERR2:
	sw_release_io_resource(pdev, &g_sw_ehci_hcd);
 	sw_stop_ehc(&g_sw_ehci_hcd);
	usb_put_hcd(hcd);

ERR1:
	ehci_clock_init(&g_sw_ehci_hcd);

ERR0:
	pin_exit(&g_sw_ehci_hcd);

	return ret;
}

/*
*******************************************************************************
*                     sw_ehci_hcd_remove
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
static int sw_ehci_hcd_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	
	sw_release_io_resource(pdev, &g_sw_ehci_hcd);
	
	usb_put_hcd(hcd);

	sw_stop_ehc(&g_sw_ehci_hcd);

	ehci_clock_exit(&g_sw_ehci_hcd);

	pin_exit(&g_sw_ehci_hcd);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM

/*
*******************************************************************************
*                     sw_ehci_hcd_suspend
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
static int sw_ehci_hcd_suspend(struct device *dev)
{
	struct sw_hci_hcd *sw_ehci = &g_sw_ehci_hcd;
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags = 0;
	int rc = 0;

	DMSG_INFO("[sw ehci0]: sw_ehci_hcd_suspend\n");

	spin_lock_irqsave(&ehci->lock, flags);
	ehci_prepare_ports_for_controller_suspend(ehci, device_may_wakeup(dev));
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	sw_stop_ehc(sw_ehci);
	spin_unlock_irqrestore(&ehci->lock, flags);

	return 0;
}

/*
*******************************************************************************
*                     sw_ehci_hcd_resume
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
static int sw_ehci_hcd_resume(struct device *dev)
{
	struct sw_hci_hcd *sw_ehci = &g_sw_ehci_hcd;
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	DMSG_INFO("[sw ehci0]: sw_ehci_hcd_resume\n");

	sw_start_ehc(sw_ehci);
	
	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		ehci_prepare_ports_for_controller_resume(ehci);

		if (!hcd->self.root_hub->do_remote_wakeup){
			mask &= ~STS_PCD;
		}
		
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);

		return 0;
	}

	DMSG_INFO("[sw ehci0]: lost power, restarting\n");

	usb_root_hub_lost_power(hcd->self.root_hub);

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having let BIOS kick in during reboot.
	 */
	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;

}

static const struct dev_pm_ops  aw_ehci_pmops = {
	.suspend	= sw_ehci_hcd_suspend,
	.resume		= sw_ehci_hcd_resume,
};

#define SW_EHCI_PMOPS 	&aw_ehci_pmops

#else

#define SW_EHCI_PMOPS 	NULL

#endif

static struct platform_driver sw_ehci0_hcd_driver ={
  .probe  	= sw_ehci_hcd_probe,
  .remove	= sw_ehci_hcd_remove,
  .shutdown = usb_hcd_platform_shutdown,
  .driver = {
		.name	= ehci0_name,
		.owner	= THIS_MODULE,
		.pm		= SW_EHCI_PMOPS,
  	}
};


