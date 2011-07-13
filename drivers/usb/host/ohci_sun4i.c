/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: ohci_sun4i.c
*
* Author 		: yangnaitian
*
* Description 	: OHCI Driver
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

#include <linux/time.h>
#include <linux/timer.h>

#include <mach/gpio_v2.h>
#include <mach/script_v2.h>
#include <linux/clk.h>

#include "sw_hci_sun4i.h"

/*.......................................................................................*/
//                               全局信息定义
/*.......................................................................................*/

//#define  SW_USB_OHCI_DEBUG

#define   SW_OHCI_NAME    "sw-ohci"
static const char ohci_name[]       = SW_OHCI_NAME;

//static char* usbc_name[3] 			= {"usbc0", "usbc1", "usbc2"};
//static char* usbc_ahb_name[3] 		= {"ahb_usb0", "ahb_usb1", "ahb_usb2"};
//static char* usbc_phy_gate_name[3] 	= {"usb_phy", "usb_phy", "usb_phy"};
//static char* usbc_phy_reset_name[3]   = {"usb_phy0", "usb_phy1", "usb_phy2"};
static char* ohci_phy_gate_name[3]   = {"", "usb_ohci0", "usb_ohci1"};

static u32 usbc_base[3] 			= {SW_VA_USB0_IO_BASE, SW_VA_USB1_IO_BASE, SW_VA_USB2_IO_BASE};
static u32 ohci_irq_no[2] 			= {SW_INT_SRC_OHCI0, SW_INT_SRC_OHCI1};
static struct sw_hci_hcd *g_sw_ohci[2];

/*.......................................................................................*/
//                                      函数区
/*.......................................................................................*/

extern int usb_disabled(void);

/*
*******************************************************************************
*                     ohci_clock_init
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
static s32 ohci_clock_init(struct sw_hci_hcd *sw_ohci)
{
	sw_ohci->ohci_gate = clk_get(NULL, ohci_phy_gate_name[sw_ohci->usbc_no]);
	if (IS_ERR(sw_ohci->ohci_gate)){
		DMSG_PANIC("ERR: get usb%d sie clk failed.\n", sw_ohci->usbc_no);
		return -1;
	}

	return 0;
}

/*
*******************************************************************************
*                     ohci_clock_exit
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
static s32 ohci_clock_exit(struct sw_hci_hcd *sw_ohci)
{
	if(sw_ohci->ohci_gate){
		clk_put(sw_ohci->ohci_gate);
		sw_ohci->ohci_gate = NULL;
	}

	return 0;
}

/*
*******************************************************************************
*                     open_ohci_clock
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
static int open_ohci_clock(struct sw_hci_hcd *sw_ohci)
{
 	DMSG_INFO("[%s%d]: open_ohci_clock\n", SW_OHCI_NAME, sw_ohci->usbc_no);

    if(sw_ohci->ohci_gate){
        sw_ohci->clk_is_open = 1;

        clk_enable(sw_ohci->ohci_gate);
        msleep(10);

        /* for A10, SIE clock、phy gate、phy reset is opened by EHCI */
    }

#ifdef  SW_USB_OHCI_DEBUG
    DMSG_INFO("[%s%d]: open_ohci_clock, 0x60(0x%x), 0xcc(0x%x)\n",
              SW_OHCI_NAME, sw_ohci->usbc_no,
              (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0x60),
              (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0xcc));
#endif

	return 0;
}

/*
*******************************************************************************
*                     close_ohci_clock
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
static int close_ohci_clock(struct sw_hci_hcd *sw_ohci)
{
 	DMSG_INFO("[%s%d]: close_ohci_clock\n", SW_OHCI_NAME, sw_ohci->usbc_no);

    if(sw_ohci->ohci_gate){
    	sw_ohci->clk_is_open = 0;
	    clk_disable(sw_ohci->ohci_gate);
    }

    /* for A10, SIE clock、phy gate、phy reset is closed by EHCI */

#ifdef  SW_USB_OHCI_DEBUG
    DMSG_INFO("[%s%d]: close_ohci_clock, 0x60(0x%x), 0xcc(0x%x)\n",
              SW_OHCI_NAME, sw_ohci->usbc_no,
              (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0x60),
              (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0xcc));
#endif

	return 0;
}

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
static int sw_get_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ohci)
{
	sw_ohci->irq_no = ohci_irq_no[sw_ohci->usbc_no - 1];

	sw_ohci->usb_vbase		= (void __iomem	*)usbc_base[sw_ohci->usbc_no];
	sw_ohci->sram_vbase		= (void __iomem	*)SW_VA_SRAM_IO_BASE;
	sw_ohci->clock_vbase	= (void __iomem	*)SW_VA_CCM_IO_BASE;
	sw_ohci->gpio_vbase		= (void __iomem	*)SW_VA_PORTC_IO_BASE;
	sw_ohci->sdram_vbase	= (void __iomem	*)SW_VA_DRAM_IO_BASE;

#ifdef  SW_USB_OHCI_DEBUG
	DMSG_INFO("[%s%d]: usb_vbase = 0x%p, sram_vbase = 0x%p, clock_vbase = 0x%p, gpio_vbase = 0x%p, sdram_vbase = 0x%p,get_io_resource_finish\n",
		   ohci_name, sw_ohci->usbc_no,
		   sw_ohci->usb_vbase,
		   sw_ohci->sram_vbase,
		   sw_ohci->clock_vbase,
		   sw_ohci->gpio_vbase,
		   sw_ohci->sdram_vbase);
#endif

	return 0;
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
static int sw_release_io_resource(struct platform_device *pdev, struct sw_hci_hcd *sw_ohci)
{

	sw_ohci->irq_no = 0;

	sw_ohci->usb_vbase		= NULL;
	sw_ohci->sram_vbase		= NULL;
	sw_ohci->clock_vbase	= NULL;
	sw_ohci->gpio_vbase		= NULL;
	sw_ohci->sdram_vbase	= NULL;

	return 0;
}


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
  	open_ohci_clock(sw_ohci);

    /* for A10, OHCI passby is enabled by EHCI */

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

    /* for A10, OHCI passby is enabled by EHCI */

	close_ohci_clock(sw_ohci);

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

static const struct hc_driver sw_ohci_hc_driver ={
	.description        = hcd_name,
	.product_desc       = "SW USB2.0 'Open' Host Controller (OHCI) Driver",
	.hcd_priv_size      = sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq                = ohci_irq,
	.flags              = HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start              = sw_ohci_start,
	.stop               = ohci_stop,
	.shutdown           = ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue        = ohci_urb_enqueue,
	.urb_dequeue        = ohci_urb_dequeue,
	.endpoint_disable   = ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number   = ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data    = ohci_hub_status_data,
	.hub_control        = ohci_hub_control,

#ifdef	CONFIG_PM
	.bus_suspend        = ohci_bus_suspend,
	.bus_resume         = ohci_bus_resume,
#endif
	.start_port_reset   = ohci_start_port_reset,
};

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
	struct usb_hcd *hcd = NULL;
	struct sw_hci_hcd *sw_ohci = NULL;

	if(pdev == NULL){
	    DMSG_PANIC("ERR: Argment is invaild\n");
	    return -1;
    }

    /* if usb is disabled, can not probe */
    if (usb_disabled()){
        DMSG_PANIC("ERR: usb hcd is disabled\n");
        return -ENODEV;
    }

	sw_ohci = pdev->dev.platform_data;
	if(!sw_ohci){
		DMSG_PANIC("ERR: sw_ohci is null\n");
		ret = -ENOMEM;
		goto ERR1;
	}

	memset(sw_ohci, 0, sizeof(struct sw_hci_hcd *));
	sw_ohci->pdev = pdev;
	sw_ohci->usbc_no = pdev->id;
	g_sw_ohci[sw_ohci->usbc_no - 1] = sw_ohci;

	DMSG_INFO("[%s%d]: pdev->name: %s, pdev->id: %d, sw_ohci: 0x%p\n",
		      ohci_name, sw_ohci->usbc_no, pdev->name, pdev->id, sw_ohci);

	ret = ohci_clock_init(sw_ohci);
	if(ret != 0){
		DMSG_PANIC("ERR: ohci_clock_init failed\n");
		ret = -ENODEV;
		goto ERR2;
	}

	/* get io resource */
	sw_get_io_resource(pdev, sw_ohci);
	sw_ohci->ohci_base 			= sw_ohci->usb_vbase + SW_USB_OHCI_BASE_OFFSET;
	sw_ohci->ohci_reg_length 	= SW_USB_OHCI_LEN;

    /*creat a usb_hcd for the ohci controller*/
	hcd = usb_create_hcd(&sw_ohci_hc_driver, &pdev->dev, ohci_name);
	if(!hcd){
        DMSG_PANIC("ERR: usb_ohci_create_hcd failed\n");
        ret = -ENOMEM;
		goto ERR3;
	}

  	hcd->rsrc_start = (u32)sw_ohci->ohci_base;
	hcd->rsrc_len 	= sw_ohci->ohci_reg_length;
	hcd->regs 		= sw_ohci->ohci_base;
	sw_ohci->hcd    = hcd;

	/* ochi start to work */
	sw_start_ohc(sw_ohci);

    ohci_hcd_init(hcd_to_ohci(hcd));

    ret = usb_add_hcd(hcd, sw_ohci->irq_no, IRQF_DISABLED | IRQF_SHARED);
    if(ret != 0){
        DMSG_PANIC("ERR: usb_add_hcd failed\n");
        ret = -ENOMEM;
        goto ERR4;
    }

    platform_set_drvdata(pdev, hcd);

#ifdef  SW_USB_OHCI_DEBUG
    DMSG_INFO("[%s%d]: probe, clock: 0x60(0x%x), 0xcc(0x%x); usb: 0x800(0x%x), dram:(0x%x, 0x%x)\n",
              SW_OHCI_NAME, sw_ohci->usbc_no,
              (u32)USBC_Readl(sw_ohci->clock_vbase + 0x60),
              (u32)USBC_Readl(sw_ohci->clock_vbase + 0xcc),
              (u32)USBC_Readl(sw_ohci->usb_vbase + 0x800),
              (u32)USBC_Readl(sw_ohci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1),
              (u32)USBC_Readl(sw_ohci->sdram_vbase + SW_SDRAM_REG_HPCR_USB2));
#endif

    return 0;

ERR4:
	usb_put_hcd(hcd);

ERR3:
    ohci_clock_exit(sw_ohci);

ERR2:
	sw_ohci->hcd = NULL;
	g_sw_ohci[sw_ohci->usbc_no] = NULL;

ERR1:

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
	struct usb_hcd *hcd = NULL;
	struct sw_hci_hcd *sw_ohci = NULL;

	if(pdev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = platform_get_drvdata(pdev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sw_ohci = pdev->dev.platform_data;
	if(sw_ohci == NULL){
		DMSG_PANIC("ERR: sw_ohci is null\n");
		return -1;
	}

	DMSG_INFO("[%s%d]: pdev->name: %s, pdev->id: %d, sw_ohci: 0x%p\n",
		      ohci_name, sw_ohci->usbc_no, pdev->name, pdev->id, sw_ohci);

	usb_remove_hcd(hcd);

	sw_stop_ohc(sw_ohci);

	usb_put_hcd(hcd);

	sw_release_io_resource(pdev, sw_ohci);

	sw_ohci->hcd = NULL;
	g_sw_ohci[sw_ohci->usbc_no] = NULL;

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
	struct sw_hci_hcd *sw_ohci  = NULL;
	struct usb_hcd *hcd         = NULL;
	struct ohci_hcd	*ohci       = NULL;
	unsigned long flags         = 0;
	int rc                      = 0;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sw_ohci = dev->platform_data;
	if(sw_ohci == NULL){
		DMSG_PANIC("ERR: sw_ohci is null\n");
		return -1;
	}

	ohci = hcd_to_ohci(hcd);
	if(ohci == NULL){
		DMSG_PANIC("ERR: ohci is null\n");
		return -1;
	}

 	DMSG_INFO("[%s%d]: sw_ohci_hcd_suspend\n", SW_OHCI_NAME, sw_ohci->usbc_no);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto fail;
	}

    ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
    (void)ohci_readl(ohci, &ohci->regs->intrdisable);

    clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

    sw_stop_ohc(sw_ohci);

fail:
    spin_unlock_irqrestore(&ohci->lock, flags);

    return rc;
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
	struct sw_hci_hcd *sw_ohci = NULL;
	struct usb_hcd *hcd = NULL;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sw_ohci = dev->platform_data;
	if(sw_ohci == NULL){
		DMSG_PANIC("ERR: sw_ohci is null\n");
		return -1;
	}

 	DMSG_INFO("[%s%d]: sw_ohci_hcd_resume\n", SW_OHCI_NAME, sw_ohci->usbc_no);

	sw_start_ohc(sw_ohci);

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	ohci_finish_controller_resume(hcd);

    return 0;
}

static const struct dev_pm_ops sw_ohci_pmops = {
	.suspend	= sw_ohci_hcd_suspend,
	.resume		= sw_ohci_hcd_resume,
};

#define SW_OHCI_PMOPS  &sw_ohci_pmops

#else

#define SW_OHCI_PMOPS NULL

#endif

static struct platform_driver sw_ohci_hcd_driver = {
	.probe		= sw_ohci_hcd_probe,
	.remove		= sw_ohci_hcd_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.name	= ohci_name,
		.owner	= THIS_MODULE,
		.pm	    = SW_OHCI_PMOPS,
	},
};



