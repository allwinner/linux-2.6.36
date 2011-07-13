/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: ohci0_sun4i.c
*
* Author 		: yangnaitian
*
* Description 	: EHCI0 Driver
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
#include <linux/signal.h>

#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#include "sw_hci_sun4i.h"

/*.......................................................................................*/
//                               全局信息定义
/*.......................................................................................*/


#define   SW_OHCI0_NAME    "sw-ohci0"
static const char ohci0_name[] = SW_OHCI0_NAME;

static struct sw_hci_hcd g_sw_ohci_hcd;


/*.......................................................................................*/
//                                      函数区
/*.......................................................................................*/

extern int usb_disabled(void);


/*
*******************************************************************************
*                     sw_start_ohc
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
static void sw_start_ohc(struct sw_hci_hcd *sw_ohci)
{
	unsigned long temp = 0;

#if 0
	/*enable ohci sie clock*/
	temp = USBC_Readl(sw_ohci->clock_vbase + SW_CCMU_REG_AHB_GATING_REG0);
	temp |= (1 << SW_CCMU_BP_AHB_GATING_USBC1);
	USBC_Writel(temp, (sw_ohci->clock_vbase + SW_CCMU_REG_AHB_GATING_REG0));
	
	/*enable OHCI1 phy and setup pll for it*/
	temp = USBC_Readl(sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG);
	temp |= (1 << SW_CCMU_BP_USB_CLK_GATING_USBPHY);
	temp |= (1 << SW_CCMU_BP_USB_CLK_48M_SEL);
	temp |= (1 << SW_CCMU_BP_USB_CLK_GATING_OHCI0);
	USBC_Writel(temp, (sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG));
	
	udelay(100);
	
	temp = USBC_Readl(sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG);
	temp |= (1 << SW_CCMU_BP_USB_CLK_USBPHY1_RST);
	USBC_Writel(temp, (sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG));
	udelay(100);
	
	/*enable passby*/
	temp = USBC_Readl(sw_ohci->usb_vbase + SW_USB_PMU_IRQ_ENABLE); 
	temp |= (1 << 10);		/* AHB Master interface INCR8 enable */
	temp |= (1 << 9);     	/* AHB Master interface burst type INCR4 enable */
	temp |= (1 << 8);     	/* AHB Master interface INCRX align enable */
	temp |= (1 << 0);    	/* ULPI bypass enable */
	USBC_Writel(temp, (sw_ohci->usb_vbase + SW_USB_PMU_IRQ_ENABLE)); 

	/*ohci port cnfigure*/   
	temp = USBC_Readl(sw_ohci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1);
	temp |= (1 << SW_SDRAM_BP_HPCR_ACCESS_EN);
	USBC_Writel(temp, (sw_ohci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1));
#endif

	return;
}

/*
*******************************************************************************
*                     sw_stop_ohc
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
static void sw_stop_ohc(struct sw_hci_hcd *sw_ohci)
{
#if 0

  unsigned long temp = 0;
  
  /*ehci port configure*/
  temp = USBC_Readl(sw_ohci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1);
  temp &= ~(1 << SW_SDRAM_BP_HPCR_ACCESS_EN);
  USBC_Writel(temp, (sw_ohci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1));
  
  /*close ohci phy colck*/
  temp = USBC_Readl(sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG);
  temp &= ~(1 << SW_CCMU_BP_USB_CLK_GATING_USBPHY);
  temp &= ~(1 << SW_CCMU_BP_USB_CLK_48M_SEL);
  temp &= ~(1 << SW_CCMU_BP_USB_CLK_GATING_OHCI0);
  USBC_Writel(temp, (sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG));
  
  udelay(100);
  
  temp = USBC_Readl(sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG);
  temp &= ~(1 << SW_CCMU_BP_USB_CLK_USBPHY1_RST);
  USBC_Writel(temp, (sw_ohci->clock_vbase + SW_CCMU_REG_USB_CLK_REG));
  udelay(100);
  
  /*close ohci sie clock*/
  temp = USBC_Readl(sw_ohci->clock_vbase + SW_CCMU_REG_AHB_GATING_REG0);
  temp &= ~(1 << SW_CCMU_BP_AHB_GATING_USBC1);
  USBC_Writel(temp, (sw_ohci->clock_vbase + SW_CCMU_REG_AHB_GATING_REG0));

  /*disable passby*/
  temp = USBC_Readl(sw_ohci->usb_vbase + SW_USB_PMU_IRQ_ENABLE); 
  temp &= ~(1 << 10);	  /* AHB Master interface INCR8 disable */
  temp &= ~(1 << 9);	  /* AHB Master interface burst type INCR4 disable */
  temp &= ~(1 << 8);	  /* AHB Master interface INCRX align disable */
  temp &= ~(1 << 0);	  /* ULPI bypass disable */
  USBC_Writel(temp, (sw_ohci->usb_vbase + SW_USB_PMU_IRQ_ENABLE)); 
#endif

  return;
}


/*
*******************************************************************************
*                     sw_ohci_start
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
static int __devinit sw_ohci_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		DMSG_PANIC("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver sw_ohci0_hc_driver ={
	.description =		hcd_name,
	.product_desc =		"SW USB2.0 'Open' Host Controller (OHCI) Driver",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		sw_ohci_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,

#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
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

static int sw_get_ohci_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ohci)
{
  __s32 ret = 0;
  
  sw_ohci->irq_no = SW_INT_SRC_OHCI0;
  
  /*usb pass by enable */
	sw_ohci->usb_passby_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 5);
	if(sw_ohci->usb_passby_base_res == NULL)
		{
			DMSG_PANIC("ERR: platform_get_resource for usb pass by failed\n");
		  ret = -ENODEV;
		  goto io_failed;
		}
		
	sw_ohci->usb_passby_reg_start  = sw_ohci->usb_passby_base_res->start;
	sw_ohci->usb_passby_reg_length = resource_size(sw_ohci->usb_passby_base_res);
	sw_ohci->usb_passby_base_req = request_mem_region(sw_ohci->usb_passby_base_res->start, sw_ohci->usb_passby_reg_length, pdev->name);
	if(sw_ohci->usb_passby_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for usb pass by failed\n");
		ret = -ENODEV;
		goto io_failed;
	 }

	 sw_ohci->usb_passby_vbase = ioremap(sw_ohci->usb_passby_base_res->start, sw_ohci->usb_passby_reg_length);
	 if(sw_ohci->usb_passby_vbase == NULL){
		 DMSG_PANIC("ERR: ioremap for usb pass by failed\n");
		 ret = -ENOMEM;
		 goto io_failed;
	  }
  
  /*USB*/
  sw_ohci->usb_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if(sw_ohci->usb_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for usb failed\n");
		ret = -ENODEV;
		goto io_failed;
	}
	
  sw_ohci->usb_reg_start  = sw_ohci->usb_base_res->start;
  sw_ohci->usb_reg_length = resource_size(sw_ohci->usb_base_res);
  sw_ohci->usb_base_req = request_mem_region(sw_ohci->usb_base_res->start, sw_ohci->usb_reg_length, pdev->name);
  if(sw_ohci->usb_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for usb failed\n");
		ret = -ENODEV;
		goto io_failed;
	}
  
  sw_ohci->usb_vbase = ioremap(sw_ohci->usb_base_res->start, sw_ohci->usb_reg_length);
  if(sw_ohci->usb_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for usb failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}
  
  /*SRAM*/
  sw_ohci->sram_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
  if(sw_ohci->sram_base_res == NULL){
	  DMSG_PANIC("ERR: platform_get_resource for sram failed\n");
	  ret = -ENODEV;
	  goto io_failed;
	}

  sw_ohci->sram_reg_start  = sw_ohci->sram_base_res->start;
  sw_ohci->sram_reg_length = resource_size(sw_ohci->sram_base_res);
  sw_ohci->sram_base_req = request_mem_region(sw_ohci->sram_base_res->start, sw_ohci->sram_reg_length, pdev->name);
  if(sw_ohci->sram_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for sram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

  sw_ohci->sram_vbase = ioremap(sw_ohci->sram_base_res->start, sw_ohci->sram_reg_length);
  if(sw_ohci->sram_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for sram failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}
  
  /*CLOCK*/
  sw_ohci->clock_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
  if(sw_ohci->clock_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for clock failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

   sw_ohci->clock_reg_start  = sw_ohci->clock_base_res->start;
   sw_ohci->clock_reg_length = resource_size(sw_ohci->clock_base_res);
   sw_ohci->clock_base_req = request_mem_region(sw_ohci->clock_base_res->start, sw_ohci->clock_reg_length, pdev->name);
   if(sw_ohci->clock_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for clock failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

	sw_ohci->clock_vbase = ioremap(sw_ohci->clock_base_res->start, sw_ohci->clock_reg_length);
	if(sw_ohci->clock_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for clock failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}
  
  /*GPIO*/
  sw_ohci->gpio_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
  if(sw_ohci->gpio_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for gpio failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

  sw_ohci->gpio_reg_start  = sw_ohci->gpio_base_res->start;
  sw_ohci->gpio_reg_length = resource_size(sw_ohci->gpio_base_res);
  sw_ohci->gpio_base_req = request_mem_region(sw_ohci->gpio_base_res->start, sw_ohci->gpio_reg_length, pdev->name);
  if(sw_ohci->gpio_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for gpio failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

  sw_ohci->gpio_vbase = ioremap(sw_ohci->gpio_base_res->start, sw_ohci->gpio_reg_length);
  if(sw_ohci->gpio_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for gpio failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}
  
  /*SDRAM*/
  sw_ohci->sdram_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
  if(sw_ohci->sdram_base_res == NULL){
		DMSG_PANIC("ERR: platform_get_resource for sdram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

  sw_ohci->sdram_reg_start  = sw_ohci->sdram_base_res->start;
  sw_ohci->sdram_reg_length = resource_size(sw_ohci->sdram_base_res);
  sw_ohci->sdram_base_req = request_mem_region(sw_ohci->sdram_base_res->start, sw_ohci->sdram_reg_length, pdev->name);
  if(sw_ohci->sdram_base_req == NULL){
		DMSG_PANIC("ERR: request_mem_region for sdram failed\n");
		ret = -ENODEV;
		goto io_failed;
	}

   sw_ohci->sdram_vbase = ioremap(sw_ohci->sdram_base_res->start, sw_ohci->sdram_reg_length);
   if(sw_ohci->sdram_vbase == NULL){
		DMSG_PANIC("ERR: ioremap for sdram failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

   DMSG_INFO("[%s]: usb_vbase = 0x%x, sram_vbase = 0x%x, clock_vbase = 0x%x, gpio_vbase = 0x%x, sdram_vbase = 0x%x\n", 
		   ohci0_name, 
		   sw_ohci->usb_vbase,
		   sw_ohci->sram_vbase,
		   sw_ohci->clock_vbase,
		   sw_ohci->gpio_vbase,
		   sw_ohci->sdram_vbase);

	return 0;
  
io_failed:  
	 /*USB PASS BY*/
	if(sw_ohci->usb_passby_vbase){
		iounmap(sw_ohci->usb_passby_vbase);
		sw_ohci->usb_passby_vbase = NULL;
	}

	if(sw_ohci->usb_passby_base_req){
		release_mem_region(sw_ohci->usb_passby_base_req->start, sw_ohci->usb_passby_reg_length);
		sw_ohci->usb_passby_base_req = NULL;
	}

	if(sw_ohci->usb_passby_base_res){
		release_resource(sw_ohci->usb_passby_base_res);
		sw_ohci->usb_passby_base_res = NULL;
	}

	sw_ohci->usb_passby_reg_start  = 0;
	sw_ohci->usb_passby_reg_length = 0;
   /*USB*/
   if(sw_ohci->usb_vbase){
		iounmap(sw_ohci->usb_vbase);
		sw_ohci->usb_vbase = NULL;
	}

	if(sw_ohci->usb_base_req){
		release_mem_region(sw_ohci->usb_base_res->start, sw_ohci->usb_reg_length);
		sw_ohci->usb_base_req = NULL;
	}

	if(sw_ohci->usb_base_res){
		release_resource(sw_ohci->usb_base_res);
		sw_ohci->usb_base_res = NULL;
	}

	sw_ohci->usb_reg_start  = 0;
	sw_ohci->usb_reg_length = 0;
	
   /*SRAM*/
   	if(sw_ohci->sram_vbase){
		iounmap(sw_ohci->sram_vbase);
		sw_ohci->sram_vbase = NULL;
	}

	if(sw_ohci->sram_base_req){
		release_mem_region(sw_ohci->sram_base_res->start, sw_ohci->sram_reg_length);
		sw_ohci->sram_base_req = NULL;
	}

	if(sw_ohci->sram_base_res){
		release_resource(sw_ohci->sram_base_res);
		sw_ohci->sram_base_res = NULL;
	}

	sw_ohci->sram_reg_start  = 0;
	sw_ohci->sram_reg_length = 0;
   
   /*CLOCK*/
   if(sw_ohci->clock_vbase){
		iounmap(sw_ohci->clock_vbase);
		sw_ohci->clock_vbase = NULL;
	}

	if(sw_ohci->clock_base_req){
		release_mem_region(sw_ohci->clock_base_res->start, sw_ohci->clock_reg_length);
		sw_ohci->clock_base_req = NULL;
	}

	if(sw_ohci->clock_base_res){
		release_resource(sw_ohci->clock_base_res);
		sw_ohci->clock_base_res = NULL;
	}

	sw_ohci->clock_reg_start  = 0;
	sw_ohci->clock_reg_length = 0;
   
   /*GPIO*/
   if(sw_ohci->gpio_vbase){
		iounmap(sw_ohci->gpio_vbase);
		sw_ohci->gpio_vbase = NULL;
	}

	if(sw_ohci->gpio_base_req){
		release_mem_region(sw_ohci->gpio_base_res->start, sw_ohci->gpio_reg_length);
		sw_ohci->gpio_base_req = NULL;
	}

	if(sw_ohci->gpio_base_res){
		release_resource(sw_ohci->gpio_base_res);
		sw_ohci->gpio_base_res = NULL;
	}

	sw_ohci->gpio_reg_start  = 0;
	sw_ohci->gpio_reg_length = 0;
   
   /*SDRAM*/
   	if(sw_ohci->sdram_vbase){
		iounmap(sw_ohci->sdram_vbase);
		sw_ohci->sdram_vbase = NULL;
	}

	if(sw_ohci->sdram_base_req){
		release_mem_region(sw_ohci->sdram_base_res->start, sw_ohci->sdram_reg_length);
		sw_ohci->sdram_base_req = NULL;
	}

	if(sw_ohci->sdram_base_res){
		release_resource(sw_ohci->sdram_base_res);
		sw_ohci->sdram_base_res = NULL;
	}

	sw_ohci->sdram_reg_start  = 0;
	sw_ohci->sdram_reg_length = 0;
   
   return ret;	
}

/*
*******************************************************************************
*                     sw_release_ohci_io_resource
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
static int sw_release_ohci_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ohci)
{
	/* USB pass by */
	if(sw_ohci->usb_passby_vbase){
		iounmap(sw_ohci->usb_passby_vbase);
		sw_ohci->usb_passby_vbase = NULL;
	}

	if(sw_ohci->usb_passby_base_req){
		release_mem_region(sw_ohci->usb_passby_base_res->start, sw_ohci->usb_passby_reg_length);
		sw_ohci->usb_passby_base_req = NULL;
	}

	if(sw_ohci->usb_passby_base_res){
		release_resource(sw_ohci->usb_passby_base_res);
		sw_ohci->usb_passby_base_res = NULL;
	}

	sw_ohci->usb_passby_reg_start  = 0;
	sw_ohci->usb_passby_reg_length = 0;
    /* USB */
	if(sw_ohci->usb_vbase){
		iounmap(sw_ohci->usb_vbase);
		sw_ohci->usb_vbase = NULL;
	}

	if(sw_ohci->usb_base_req){
		release_mem_region(sw_ohci->usb_base_res->start, sw_ohci->usb_reg_length);
		sw_ohci->usb_base_req = NULL;
	}

	if(sw_ohci->usb_base_res){
		release_resource(sw_ohci->usb_base_res);
		sw_ohci->usb_base_res = NULL;
	}

	sw_ohci->usb_reg_start  = 0;
	sw_ohci->usb_reg_length = 0;

	/* sram */
	if(sw_ohci->sram_vbase){
		iounmap(sw_ohci->sram_vbase);
		sw_ohci->sram_vbase = NULL;
	}

	if(sw_ohci->sram_base_req){
		release_mem_region(sw_ohci->sram_base_res->start, sw_ohci->sram_reg_length);
		sw_ohci->sram_base_req = NULL;
	}

	if(sw_ohci->sram_base_res){
		release_resource(sw_ohci->sram_base_res);
		sw_ohci->sram_base_res = NULL;
	}

	sw_ohci->sram_reg_start  = 0;
	sw_ohci->sram_reg_length = 0;

	/* clock */
	if(sw_ohci->clock_vbase){
		iounmap(sw_ohci->clock_vbase);
		sw_ohci->clock_vbase = NULL;
	}

	if(sw_ohci->clock_base_req){
		release_mem_region(sw_ohci->clock_base_res->start, sw_ohci->clock_reg_length);
		sw_ohci->clock_base_req = NULL;
	}

	if(sw_ohci->clock_base_res){
		release_resource(sw_ohci->clock_base_res);
		sw_ohci->clock_base_res = NULL;
	}

	sw_ohci->clock_reg_start  = 0;
	sw_ohci->clock_reg_length = 0;

	/* gpio */
	if(sw_ohci->gpio_vbase){
		iounmap(sw_ohci->gpio_vbase);
		sw_ohci->gpio_vbase = NULL;
	}

	if(sw_ohci->gpio_base_req){
		release_mem_region(sw_ohci->gpio_base_res->start, sw_ohci->gpio_reg_length);
		sw_ohci->gpio_base_req = NULL;
	}

	if(sw_ohci->gpio_base_res){
		release_resource(sw_ohci->gpio_base_res);
		sw_ohci->gpio_base_res = NULL;
	}

	sw_ohci->gpio_reg_start  = 0;
	sw_ohci->gpio_reg_length = 0;

  	/* sdram */
	if(sw_ohci->sdram_vbase){
		iounmap(sw_ohci->sdram_vbase);
		sw_ohci->sdram_vbase = NULL;
	}

	if(sw_ohci->sdram_base_req){
		release_mem_region(sw_ohci->sdram_base_res->start, sw_ohci->sdram_reg_length);
		sw_ohci->sdram_base_req = NULL;
	}

	if(sw_ohci->sdram_base_res){
		release_resource(sw_ohci->sdram_base_res);
		sw_ohci->sdram_base_res = NULL;
	}

	sw_ohci->sdram_reg_start  = 0;
	sw_ohci->sdram_reg_length = 0;

	return 0;
  	
}

#else

static int sw_get_ohci_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ohci)
{
   sw_ohci->irq_no = SW_INT_SRC_OHCI0;

   sw_ohci->usb_vbase		= (void __iomem	*)SW_VA_USB1_IO_BASE;
   sw_ohci->sram_vbase		= (void __iomem	*)SW_VA_SRAM_IO_BASE;
   sw_ohci->clock_vbase		= (void __iomem	*)SW_VA_CCM_IO_BASE;
   sw_ohci->gpio_vbase		= (void __iomem	*)SW_VA_PORTC_IO_BASE;
   sw_ohci->sdram_vbase		= (void __iomem	*)SW_VA_DRAM_IO_BASE;

   DMSG_INFO("[%s]: usb_vbase = 0x%p, sram_vbase = 0x%p, clock_vbase = 0x%p, gpio_vbase = 0x%p, sdram_vbase = 0x%p\n", 
		   ohci0_name, 
		   sw_ohci->usb_vbase,
		   sw_ohci->sram_vbase,
		   sw_ohci->clock_vbase,
		   sw_ohci->gpio_vbase,
		   sw_ohci->sdram_vbase);

	return 0;	
}

static int sw_release_ohci_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ohci)
{
    sw_ohci->irq_no = 0;

	sw_ohci->usb_vbase		= NULL;
	sw_ohci->sram_vbase		= NULL;
	sw_ohci->clock_vbase	= NULL;
	sw_ohci->gpio_vbase		= NULL;
	sw_ohci->sdram_vbase	= NULL;

	return 0;	
}

#endif




/*
*******************************************************************************
*                     sw_ohci_hcd_probe
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

static int sw_ohci_hcd_probe(struct platform_device *pdev)
{
	int ret;
	struct usb_hcd *hcd;

    /*if usb is disable, probe stop*/
	if (usb_disabled())
		return -ENODEV;

	
	memset(&g_sw_ohci_hcd, 0, sizeof(struct sw_hci_hcd));

    /*creat a usb_hcd for the ohci controller*/
	hcd = usb_create_hcd(&sw_ohci0_hc_driver, &pdev->dev, "Au1xxx");
	if (!hcd)
	{
	 DMSG_PANIC("ERR: usb_ohci_create_hcd failed\n");
	 return -ENOMEM;	
	}
	
	
	/*get io resource*/
	sw_get_ohci_io_resource(pdev, &g_sw_ohci_hcd);
	g_sw_ohci_hcd.ohci_base = g_sw_ohci_hcd.usb_vbase + SW_USB_OHCI_BASE_OFFSET;
	g_sw_ohci_hcd.ohci_reg_length = SW_USB_OHCI_LEN;

	//hcd->rsrc_start = (u32)g_sw_ohci_hcd.ohci_base;
	//hcd->rsrc_len = g_sw_ohci_hcd.ohci_reg_length;
	hcd->rsrc_start = (SW_USB1_BASE + SW_USB_OHCI_BASE_OFFSET);
	hcd->rsrc_len = SW_USB_OHCI_LEN;
	
	hcd->regs = g_sw_ohci_hcd.ohci_base;

    /*ohci start to work*/
    sw_start_ohc(&g_sw_ohci_hcd);

    ohci_hcd_init(hcd_to_ohci(hcd));
    
    ret = usb_add_hcd(hcd, g_sw_ohci_hcd.irq_no, IRQF_DISABLED | IRQF_SHARED);
    if(ret != 0)
    {
      DMSG_PANIC("ERR: usb_add_hcd failed\n");
      ret = -ENOMEM;
      goto ERR;	
    }
    
    platform_set_drvdata(pdev, hcd);
    sw_release_ohci_io_resource(pdev, &g_sw_ohci_hcd);

    return 0;
 
 ERR:
   sw_release_ohci_io_resource(pdev, &g_sw_ohci_hcd);
   sw_stop_ohc(&g_sw_ohci_hcd);
   usb_put_hcd(hcd); 
    
 return ret;
}

/*
*******************************************************************************
*                     sw_ohci_hcd_remove
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

static int sw_ohci_hcd_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	
	sw_stop_ohc(&g_sw_ohci_hcd);
	
	sw_release_ohci_io_resource(pdev, &g_sw_ohci_hcd);
	
	usb_put_hcd(hcd);
	
	platform_set_drvdata(pdev, NULL);

	return 0;
}



#ifdef CONFIG_PM 

/*
*******************************************************************************
*                     sw_ohci_hcd_suspend
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
static int sw_ohci_hcd_suspend(struct device *dev)
{
   DMSG_PANIC("ERR: sw ohci not support suspend\n");

	return 0;	
} 

/*
*******************************************************************************
*                     sw_ohci_hcd_resume
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

static int sw_ohci_hcd_resume(struct device *dev)
{
   DMSG_PANIC("ERR: sw ohci not support resume\n");

	return 0;
}

static const struct dev_pm_ops aw_ohci_pmops = {
	.suspend	= sw_ohci_hcd_suspend,
	.resume		= sw_ohci_hcd_resume,
};

#define AW1623_OHCI_PMOPS  &aw_ohci_pmops

#else

#define SW_OHCI_PMOPS NULL

#endif

static struct platform_driver sw_ohci0_hcd_driver = {
	.probe		= sw_ohci_hcd_probe,
	.remove		= sw_ohci_hcd_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.name	= ohci0_name,
		.owner	= THIS_MODULE, 
		.pm	= SW_OHCI_PMOPS,
	},
};



