/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: ehci_sun4i.c
*
* Author 		: yangnaitian
*
* Description 	: SoftWinner EHCI Driver
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
#include <linux/time.h>
#include <linux/timer.h>

#include <mach/gpio_v2.h>
#include <mach/script_v2.h>
#include <linux/clk.h>

#include "sw_hci_sun4i.h"

//#define  SW_USB_EHCI_DEBUG

/*.......................................................................................*/
//                               全局信息定义
/*.......................................................................................*/

#define  SW_EHCI_NAME				"sw-ehci"
static const char ehci_name[] 		= SW_EHCI_NAME;

static char* usbc_name[3] 			= {"usbc0", "usbc1", "usbc2"};
static char* usbc_ahb_name[3] 		= {"ahb_usb0", "ahb_usb1", "ahb_usb2"};
static char* usbc_phy_gate_name[3] 	= {"usb_phy", "usb_phy", "usb_phy"};
static char* usbc_phy_reset_name[3] = {"usb_phy0", "usb_phy1", "usb_phy2"};

static u32 usbc_base[3] 			= {SW_VA_USB0_IO_BASE, SW_VA_USB1_IO_BASE, SW_VA_USB2_IO_BASE};
static u32 ehci_irq_no[2] 			= {SW_INT_SRC_EHCI0, SW_INT_SRC_EHCI1};
static struct sw_hci_hcd *g_sw_ehci[2];

/*.......................................................................................*/
//                                      函数区
/*.......................................................................................*/

extern int usb_disabled(void);

/*
*******************************************************************************
*                     USBC_Phy_GetCsr
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
static __u32 USBC_Phy_GetCsr(__u32 usbc_no)
{
	__u32 val = 0x0;

	switch(usbc_no){
		case 0:
			val = SW_VA_USB0_IO_BASE + 0x404;
		break;

		case 1: 
			val = SW_VA_USB0_IO_BASE + 0x404;
		break;

		case 2:
			val = SW_VA_USB0_IO_BASE + 0x404;
		break;

		default:
		break;
	}

	return val;
}

/*
*******************************************************************************
*                     USBC_Phy_TpRead
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
static __u32 USBC_Phy_TpRead(__u32 usbc_no, __u32 addr, __u32 len)
{
	__u32 temp = 0, ret = 0;
	__u32 i=0;
	__u32 j=0;	

	for(j = len; j > 0; j--)
	{
		/* set  the bit address to be read */
		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0xff << 8);
		temp |= ((addr + j -1) << 8);
		USBC_Writel(temp, USBC_Phy_GetCsr(usbc_no));

		for(i = 0; i < 0x4; i++);

		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		ret <<= 1;
		ret |= ((temp >> (16 + usbc_no)) & 0x1); 
	}

	return ret;
}

/*
*******************************************************************************
*                     USBC_Phy_TpWrite
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
static __u32 USBC_Phy_TpWrite(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	__u32 temp = 0, dtmp = 0;
//	__u32 i=0;
	__u32 j=0;

	dtmp = data;
	for(j = 0; j < len; j++)
	{
		/* set  the bit address to be write */
		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0xff << 8);
		temp |= ((addr + j) << 8);
		USBC_Writel(temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		temp &= ~(0x1 << (usbc_no << 1));
		USBC_Writeb(temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp |= (0x1 << (usbc_no << 1));
		USBC_Writeb( temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0x1 << (usbc_no <<1 ));
		USBC_Writeb(temp, USBC_Phy_GetCsr(usbc_no));
		dtmp >>= 1;
	}

	return data;
}

/*
*******************************************************************************
*                     USBC_Phy_Read
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
static __u32 USBC_Phy_Read(__u32 usbc_no, __u32 addr, __u32 len)
{
	return USBC_Phy_TpRead(usbc_no, addr, len);	
}

/*
*******************************************************************************
*                     USBC_Phy_Write
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
static __u32 USBC_Phy_Write(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	return USBC_Phy_TpWrite(usbc_no, addr, data, len);
}

/*
*******************************************************************************
*                     UsbPhyInit
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
static void UsbPhyInit(__u32 usbc_no)
{
	DMSG_INFO("csr1: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Readl(USBC_Phy_GetCsr(usbc_no)));

	USBC_Phy_Write(usbc_no, 0x2a, 3, 2);

	DMSG_INFO("csr2: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x2a, 2));
	DMSG_INFO("csr3: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Readl(USBC_Phy_GetCsr(usbc_no)));

	return;
}


/*
*******************************************************************************
*                     get_usb_cfg
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
static s32 get_usb_cfg(struct sw_hci_hcd *sw_ehci)
{
	__s32 ret = 0;

	/* request gpio */
	ret = script_parser_fetch(usbc_name[sw_ehci->usbc_no], "usb_drv_vbus_gpio", (int *)&sw_ehci->drv_vbus_gpio_set, 64);
	if(ret != 0){
		DMSG_PANIC("ERR: get usbc%d(%s) id failed\n", sw_ehci->usbc_no, usbc_name[sw_ehci->usbc_no]);
		return -1;
	}

	/* usbc_init_state */
	ret = script_parser_fetch(usbc_name[sw_ehci->usbc_no], "usbc_init_state", (int *)&(sw_ehci->usbc_init_state), 64);
	if(ret != 0){
		DMSG_PANIC("ERR: script_parser_fetch usbc_init_state failed\n");
		return -1;
	}

#ifdef  SW_USB_EHCI_DEBUG
	DMSG_INFO("\n------%s%d config------\n", ehci_name, sw_ehci->usbc_no);
	DMSG_INFO("usbc_name            = %s\n", usbc_name[sw_ehci->usbc_no]);
	DMSG_INFO("usbc_ahb_name        = %s\n", usbc_ahb_name[sw_ehci->usbc_no]);
	DMSG_INFO("usbc_phy_gate_name   = %s\n", usbc_phy_gate_name[sw_ehci->usbc_no]);
	DMSG_INFO("usbc_phy_reset_name  = %s\n", usbc_phy_reset_name[sw_ehci->usbc_no]);
	DMSG_INFO("usbc_base            = 0x%x\n", usbc_base[sw_ehci->usbc_no]);
	DMSG_INFO("ehci_irq_no          = %d\n", ehci_irq_no[sw_ehci->usbc_no - 1]);

	DMSG_INFO("gpio_name            = %s\n", sw_ehci->drv_vbus_gpio_set.gpio_name);
	DMSG_INFO("port                 = %x\n", sw_ehci->drv_vbus_gpio_set.port);
	DMSG_INFO("port_num             = %x\n", sw_ehci->drv_vbus_gpio_set.port_num);
	DMSG_INFO("mul_sel              = %x\n", sw_ehci->drv_vbus_gpio_set.mul_sel);
	DMSG_INFO("pull                 = %x\n", sw_ehci->drv_vbus_gpio_set.pull);
	DMSG_INFO("drv_level            = %x\n", sw_ehci->drv_vbus_gpio_set.drv_level);
	DMSG_INFO("data                 = %x\n", sw_ehci->drv_vbus_gpio_set.data);

	DMSG_INFO("usbc_init_state  	= %x\n", sw_ehci->usbc_init_state);
#endif

	return 0;
}

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
		DMSG_INFO("INFO : USB VBus for EHCI%d power ON\n", sw_ehci->usbc_no);
	}else{
		gpio_write_one_pin_value(sw_ehci->Drv_vbus_Handle, 0, NULL);
		DMSG_INFO("INFO : USB VBus for EHCI%d power OFF\n", sw_ehci->usbc_no);
	}

	return;
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
	sw_ehci->sie_clk = clk_get(NULL, usbc_ahb_name[sw_ehci->usbc_no]);
	if (IS_ERR(sw_ehci->sie_clk)){
		DMSG_PANIC("ERR: get usb%d sie clk failed.\n", sw_ehci->usbc_no);
		goto failed;
	}

	sw_ehci->phy_gate = clk_get(NULL, usbc_phy_gate_name[sw_ehci->usbc_no]);
	if (IS_ERR(sw_ehci->phy_gate)){
		DMSG_PANIC("ERR: get usb%d phy_gate failed.\n", sw_ehci->usbc_no);
		goto failed;
	}

	sw_ehci->phy_reset = clk_get(NULL, usbc_phy_reset_name[sw_ehci->usbc_no]);
	if (IS_ERR(sw_ehci->phy_reset)){
		DMSG_PANIC("ERR: get usb%d phy_reset failed.\n", sw_ehci->usbc_no);
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
 	DMSG_INFO("[%s%d]: open_ehci_clock\n", SW_EHCI_NAME, sw_ehci->usbc_no);

	if(sw_ehci->sie_clk && sw_ehci->phy_gate && sw_ehci->phy_reset && !sw_ehci->clk_is_open){
	   	clk_enable(sw_ehci->sie_clk);
		msleep(10);

	    clk_enable(sw_ehci->phy_gate);
	    clk_enable(sw_ehci->phy_reset);
		clk_reset(sw_ehci->phy_reset, 0);
		msleep(10);

		sw_ehci->clk_is_open = 1;
	}else{
		DMSG_INFO("[%s%d]: ERR: open ehci clock failed, (0x%p, 0x%p, 0x%p, %d)\n",
			      SW_EHCI_NAME, sw_ehci->usbc_no,
			      sw_ehci->sie_clk, sw_ehci->phy_gate, 
			      sw_ehci->phy_reset, sw_ehci->clk_is_open);
	}

	UsbPhyInit(sw_ehci->usbc_no);

#ifdef  SW_USB_EHCI_DEBUG
	DMSG_INFO("[%s%d]: open_ehci_clock, 0x60(0x%x), 0xcc(0x%x)\n", 
		      SW_EHCI_NAME, sw_ehci->usbc_no,
		      (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0x60),
		      (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0xcc));
#endif

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
 	DMSG_INFO("[%s%d]: close_ehci_clock\n", SW_EHCI_NAME, sw_ehci->usbc_no);

	if(sw_ehci->sie_clk && sw_ehci->phy_gate && sw_ehci->phy_reset && sw_ehci->clk_is_open){
		clk_reset(sw_ehci->phy_reset, 1);
	    clk_disable(sw_ehci->phy_reset);
	    clk_disable(sw_ehci->phy_gate);
	    clk_disable(sw_ehci->sie_clk);
		sw_ehci->clk_is_open = 0;
	}else{
		DMSG_INFO("[%s%d]: ERR: close ehci clock failed, (0x%p, 0x%p, 0x%p, %d)\n", 
				  SW_EHCI_NAME, sw_ehci->usbc_no,
			      sw_ehci->sie_clk, sw_ehci->phy_gate,
			      sw_ehci->phy_reset, sw_ehci->clk_is_open);
	}

#ifdef  SW_USB_EHCI_DEBUG
	DMSG_INFO("[%s%d]: close_ehci_clock, 0x60(0x%x), 0xcc(0x%x)\n", 
		      SW_EHCI_NAME, sw_ehci->usbc_no,
		      (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0x60),
		      (u32)USBC_Readl(SW_VA_CCM_IO_BASE + 0xcc));
#endif

	return 0;
}

/*
*******************************************************************************
*                     sw_ehci_port_configure
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
static void sw_ehci_port_configure(struct sw_hci_hcd *sw_ehci, u32 enable)
{
	unsigned long reg_value = 0;
	u32 usbc_sdram_hpcr = 0;

	if(sw_ehci->usbc_no == 1){
		usbc_sdram_hpcr = SW_SDRAM_REG_HPCR_USB1;
	}else if(sw_ehci->usbc_no == 2){
		usbc_sdram_hpcr = SW_SDRAM_REG_HPCR_USB2;
	}else{
		DMSG_PANIC("EER: unkown usbc_no(%d)\n", sw_ehci->usbc_no);
		return;
	}

	reg_value = USBC_Readl(sw_ehci->sdram_vbase + usbc_sdram_hpcr);
	if(enable){
		reg_value |= (1 << SW_SDRAM_BP_HPCR_ACCESS_EN);
	}else{
		reg_value &= ~(1 << SW_SDRAM_BP_HPCR_ACCESS_EN);
	}
	USBC_Writel(reg_value, (sw_ehci->sdram_vbase + usbc_sdram_hpcr));

	return;
}

/*
*******************************************************************************
*                     sw_start_ehci
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
	sw_ehci->irq_no = ehci_irq_no[sw_ehci->usbc_no - 1];

	sw_ehci->usb_vbase		= (void __iomem	*)usbc_base[sw_ehci->usbc_no];
	sw_ehci->sram_vbase		= (void __iomem	*)SW_VA_SRAM_IO_BASE;
	sw_ehci->clock_vbase	= (void __iomem	*)SW_VA_CCM_IO_BASE;
	sw_ehci->gpio_vbase		= (void __iomem	*)SW_VA_PORTC_IO_BASE;
	sw_ehci->sdram_vbase	= (void __iomem	*)SW_VA_DRAM_IO_BASE;

#ifdef  SW_USB_EHCI_DEBUG
	DMSG_INFO("[%s%d]: usb_vbase = 0x%p, sram_vbase = 0x%p, clock_vbase = 0x%p, gpio_vbase = 0x%p, sdram_vbase = 0x%p,get_io_resource_finish\n", 
		   ehci_name, sw_ehci->usbc_no,
		   sw_ehci->usb_vbase,
		   sw_ehci->sram_vbase,
		   sw_ehci->clock_vbase,
		   sw_ehci->gpio_vbase,
		   sw_ehci->sdram_vbase);
#endif

	return 0;
}

/*
*******************************************************************************
*                     sw_start_ehci
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

	sw_ehci->irq_no = 0;

	sw_ehci->usb_vbase		= NULL;
	sw_ehci->sram_vbase		= NULL;
	sw_ehci->clock_vbase	= NULL;
	sw_ehci->gpio_vbase		= NULL;
	sw_ehci->sdram_vbase	= NULL;

	return 0;
}

/*
*******************************************************************************
*                     sw_start_ehci
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
static void sw_start_ehci(struct sw_hci_hcd *sw_ehci)
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
	sw_ehci_port_configure(sw_ehci, 1);

	sw_hcd_board_set_vbus(sw_ehci, 1);

	return;
}

/*
*******************************************************************************
*                     sw_stop_ehci
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
static void sw_stop_ehci(struct sw_hci_hcd *sw_ehci)
{
	unsigned long reg_value = 0;

	sw_hcd_board_set_vbus(sw_ehci, 0);

	/* ehci port configure */
	sw_ehci_port_configure(sw_ehci, 0);

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

static const struct hc_driver sw_ehci_hc_driver = {
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
	struct sw_hci_hcd *sw_ehci = NULL;
	int ret = 0;

	if(pdev == NULL){
		DMSG_PANIC("ERR: Argment is invaild\n");
		return -1;
	}

	/* if usb is disabled, can not probe */
	if (usb_disabled()) {
		DMSG_PANIC("ERR: usb hcd is disabled\n");
		return -ENODEV;
	}

	sw_ehci = pdev->dev.platform_data;
	if(!sw_ehci){
		DMSG_PANIC("ERR: sw_ehci is null\n");
		ret = -ENOMEM;
		goto ERR2;
	}

	memset(sw_ehci, 0, sizeof(struct sw_hci_hcd *));
	sw_ehci->pdev = pdev;
	sw_ehci->usbc_no = pdev->id;
	g_sw_ehci[sw_ehci->usbc_no - 1] = sw_ehci;

	DMSG_INFO("[%s%d]: pdev->name: %s, pdev->id: %d, sw_ehci: 0x%p\n",
		      ehci_name, sw_ehci->usbc_no, pdev->name, pdev->id, sw_ehci);

	ret = get_usb_cfg(sw_ehci);
	if(ret != 0){
		DMSG_PANIC("ERR: pin_init failed\n");
		ret = -ENODEV;
		goto ERR3;
	}	

	ret = pin_init(sw_ehci);
	if(ret != 0){
		DMSG_PANIC("ERR: pin_init failed\n");
		ret = -ENODEV;
		goto ERR3;
	}

	ret = ehci_clock_init(sw_ehci);
	if(ret != 0){
		DMSG_PANIC("ERR: ehci_clock_init failed\n");
		ret = -ENODEV;
		goto ERR4;
	}

	/* get io resource */
	sw_get_io_resource(pdev, sw_ehci);
	sw_ehci->ehci_base 			= sw_ehci->usb_vbase + SW_USB_EHCI_BASE_OFFSET;
	sw_ehci->ehci_reg_length 	= SW_USB_EHCI_LEN;

	/* creat a usb_hcd for the ehci controller */
	hcd = usb_create_hcd(&sw_ehci_hc_driver, &pdev->dev, ehci_name);
	if (!hcd){
		DMSG_PANIC("ERR: usb_create_hcd failed\n");
		ret = -ENOMEM;
		goto ERR1;
	}

  	hcd->rsrc_start = (u32)sw_ehci->ehci_base;
	hcd->rsrc_len 	= sw_ehci->ehci_reg_length;
	hcd->regs 		= sw_ehci->ehci_base;
	sw_ehci->hcd    = hcd;

	/* echi start to work */
	sw_start_ehci(sw_ehci);

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
  
	/* cache this readonly data, minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	ret = usb_add_hcd(hcd, sw_ehci->irq_no, IRQF_DISABLED | IRQF_SHARED);
	if (ret != 0) {
		DMSG_PANIC("ERR: usb_add_hcd failed\n");
		ret = -ENOMEM;
		goto ERR5;
	}

	platform_set_drvdata(pdev, hcd);

#ifdef  SW_USB_EHCI_DEBUG
	DMSG_INFO("[%s%d]: probe, clock: 0x60(0x%x), 0xcc(0x%x); usb: 0x800(0x%x), dram:(0x%x, 0x%x)\n", 
		      SW_EHCI_NAME, sw_ehci->usbc_no,
		      (u32)USBC_Readl(sw_ehci->clock_vbase + 0x60),
		      (u32)USBC_Readl(sw_ehci->clock_vbase + 0xcc),
		      (u32)USBC_Readl(sw_ehci->usb_vbase + 0x800),
		      (u32)USBC_Readl(sw_ehci->sdram_vbase + SW_SDRAM_REG_HPCR_USB1),
		      (u32)USBC_Readl(sw_ehci->sdram_vbase + SW_SDRAM_REG_HPCR_USB2));
#endif

	return 0;

ERR5:
	ehci_clock_init(sw_ehci);

ERR4:
	pin_exit(sw_ehci);
	
ERR3:	
ERR2:
	usb_put_hcd(hcd);

ERR1:

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
	struct usb_hcd *hcd = NULL;
	struct sw_hci_hcd *sw_ehci = NULL;

	if(pdev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = platform_get_drvdata(pdev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sw_ehci = pdev->dev.platform_data;
	if(sw_ehci == NULL){
		DMSG_PANIC("ERR: sw_ehci is null\n");
		return -1;
	}

	DMSG_INFO("[%s%d]: pdev->name: %s, pdev->id: %d, sw_ehci: 0x%p\n",
		      ehci_name, sw_ehci->usbc_no, pdev->name, pdev->id, sw_ehci);

	usb_remove_hcd(hcd);

	sw_release_io_resource(pdev, sw_ehci);

	usb_put_hcd(hcd);

	sw_stop_ehci(sw_ehci);

	ehci_clock_exit(sw_ehci);

	pin_exit(sw_ehci);

	sw_ehci->hcd = NULL;
	g_sw_ehci[sw_ehci->usbc_no] = NULL;

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
	struct sw_hci_hcd *sw_ehci = NULL;
	struct usb_hcd *hcd = NULL;
	struct ehci_hcd *ehci = NULL;
	unsigned long flags = 0;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sw_ehci = dev->platform_data;
	if(sw_ehci == NULL){
		DMSG_PANIC("ERR: sw_ehci is null\n");
		return -1;
	}

	ehci = hcd_to_ehci(hcd);
	if(ehci == NULL){
		DMSG_PANIC("ERR: ehci is null\n");
		return -1;
	}

 	DMSG_INFO("[%s%d]: sw_ehci_hcd_suspend\n", SW_EHCI_NAME, sw_ehci->usbc_no);

	spin_lock_irqsave(&ehci->lock, flags);
	ehci_prepare_ports_for_controller_suspend(ehci, device_may_wakeup(dev));
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	sw_stop_ehci(sw_ehci);
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
	struct sw_hci_hcd *sw_ehci = NULL;
	struct usb_hcd *hcd = NULL;
	struct ehci_hcd *ehci = NULL;

	if(dev == NULL){
		DMSG_PANIC("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = dev_get_drvdata(dev);
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	sw_ehci = dev->platform_data;
	if(sw_ehci == NULL){
		DMSG_PANIC("ERR: sw_ehci is null\n");
		return -1;
	}

	ehci = hcd_to_ehci(hcd);
	if(ehci == NULL){
		DMSG_PANIC("ERR: ehci is null\n");
		return -1;
	}

 	DMSG_INFO("[%s%d]: sw_ehci_hcd_resume\n", SW_EHCI_NAME, sw_ehci->usbc_no);

	sw_start_ehci(sw_ehci);

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

 	DMSG_INFO("[%s%d]: lost power, restarting\n", SW_EHCI_NAME, sw_ehci->usbc_no);

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

static struct platform_driver sw_ehci_hcd_driver ={
  .probe  	= sw_ehci_hcd_probe,
  .remove	= sw_ehci_hcd_remove,
  .shutdown = usb_hcd_platform_shutdown,
  .driver = {
		.name	= ehci_name,
		.owner	= THIS_MODULE,
		.pm		= SW_EHCI_PMOPS,
  	}
};

/*
*******************************************************************************
*                     sw_usb_disable_ehci
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
int sw_usb_disable_ehci(__u32 usbc_no)
{
	struct sw_hci_hcd *sw_ehci = NULL;
	struct usb_hcd *hcd = NULL;
    struct device *dev = NULL;

	if(usbc_no != 1 && usbc_no != 2){
		DMSG_PANIC("ERR:Argmen invalid. usbc_no(%d)\n", usbc_no);
		return -1;
	}

	sw_ehci = g_sw_ehci[usbc_no - 1];
	if(sw_ehci == NULL){
		DMSG_PANIC("ERR: sw_ehci is null\n");
		return -1;
	}

	hcd = sw_ehci->hcd;
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	dev = &sw_ehci->pdev->dev;
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	DMSG_INFO("sw_usb_disable_ehci%d\n", sw_ehci->usbc_no);

#ifdef CONFIG_PM
	ehci_bus_suspend(hcd);
	udelay(1000);
	sw_ehci_hcd_suspend(dev);
	udelay(1000);
#endif

	return 0;
}
EXPORT_SYMBOL(sw_usb_disable_ehci);

/*
*******************************************************************************
*                     sw_usb_enable_ehci
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
int sw_usb_enable_ehci(__u32 usbc_no)
{
	struct sw_hci_hcd *sw_ehci = NULL;
	struct usb_hcd *hcd = NULL;
    struct device *dev = NULL;

	if(usbc_no != 1 && usbc_no != 2){
		DMSG_PANIC("ERR:Argmen invalid. usbc_no(%d)\n", usbc_no);
		return -1;
	}

	sw_ehci = g_sw_ehci[usbc_no - 1];
	if(sw_ehci == NULL){
		DMSG_PANIC("ERR: sw_ehci is null\n");
		return -1;
	}

	hcd = sw_ehci->hcd;
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	dev = &sw_ehci->pdev->dev;
	if(hcd == NULL){
		DMSG_PANIC("ERR: hcd is null\n");
		return -1;
	}

	DMSG_INFO("sw_usb_enable_ehci%d\n", sw_ehci->usbc_no);

#ifdef CONFIG_PM
	sw_ehci_hcd_resume(dev);
	ehci_bus_resume(hcd);
#endif

	return 0;
}
EXPORT_SYMBOL(sw_usb_enable_ehci);


