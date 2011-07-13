/*
*********************************************************************************************************
* File    : dram.c
* By      : Berg.Xing
* Date    : 2011-06-01
* Descript: dram  for AW1623 chipset
* Update  : date          auther      ver     notes
*     2011-06-01      			Berg        1.0     create file
*********************************************************************************************************
*/
#include "dram_i.h"


/*
*********************************************************************************************************
*                 DRAM ENTER SELF REFRESH
*
* Description: dram enter/exit self-refresh;
*
* Arguments  : none
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
void DRAMC_enter_selfrefresh(void)
{
	__u32 i;
	__u32 reg_val;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x12U<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	standby_delay(0x100);
}
void mctl_mode_exit(void)
{
	__u32 i;
	__u32 reg_val;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x17U<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	standby_delay(0x100);
}

void DRAMC_exit_selfrefresh(void)
{
	mctl_mode_exit();
}

/*
*********************************************************************************************************
*                 DRAM POWER DOWN
*
* Description: enter/exit dram power down state
*
* Arguments  :
*
* Returns    : none;
*
* Note       :
*********************************************************************************************************
*/
void DRAMC_enter_power_down(void)
{
	__u32 i;
	__u32 reg_val;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x1eU<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	standby_delay(0x100);
}

void DRAMC_exit_power_down(void)
{
    mctl_mode_exit();
}

/*
**********************************************************************************************************************
*                 DRAM HOSTPORT CONTROL
*
* Description: dram host port enable/ disable
*
* Arguments  : __u32 port_idx		host port index   (0,1,...31)
*				__u32 on		enable or disable (0: diable, 1: enable)
*
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
void DRAMC_hostport_on_off(__u32 port_idx, __u32 on)
{
    __u32   reg_val;

    if(port_idx<=31)
    {
	    reg_val = mctl_read_w(SDR_HPCR + (port_idx<<2));
	    if(on)
	    	reg_val |= 0x1;
	    else
	    	reg_val &= ~(0x1);
	    mctl_write_w(SDR_HPCR + (port_idx<<2), reg_val);
	}
}
/*
**********************************************************************************************************************
*                 DRAM GET HOSTPORT STATUS
*
* Description: dram get AHB FIFO status
*
* Arguments  : __u32 port_idx		host port index   	(0,1,...31)
*
* Returns    : __u32 ret_val		AHB FIFO status 	(0: FIFO not empty ,1: FIFO empty)
*
* Notes      :
*
**********************************************************************************************************************
*/
__u32 DRAMC_hostport_check_ahb_fifo_status(__u32 port_idx)
{
    __u32   reg_val;

    if(port_idx<=31)
    {
	    reg_val = mctl_read_w(SDR_CFSR);
	    return ( (reg_val>>port_idx)&0x1 );
	}
	else
	{
		return 0;
	}
}
/*
**********************************************************************************************************************
*                 DRAM GET HOSTPORT STATUS
*
* Description: dram get AHB FIFO status
*
* Arguments  : 	__u32 port_idx				host port index   	(0,1,...31)
*				__u32 port_pri_level		priority level		(0,1,2,3)
*
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
void DRAMC_hostport_setup(__u32 port_idx, __u32 port_pri_level, __u32 port_wait_cycle, __u32 cmd_num)
{
    __u32   reg_val;

    if(port_idx<=31)
    {
	    reg_val = mctl_read_w(SDR_HPCR + (port_idx<<2));
	    reg_val &= ~(0x3<<2);
	    reg_val |= (port_pri_level&0x3)<<2;
	    reg_val &= ~(0xf<<4);
	    reg_val |= (port_wait_cycle&0xf)<<4;
	    reg_val &= ~(0xff<<8);
	    reg_val |= (cmd_num&0x3)<<8;
	    mctl_write_w(SDR_HPCR + (port_idx<<2), reg_val);
	}
}
/*
*********************************************************************************************************
*                 DRAM power save process
*
* Description: We can save power by disable DRAM PLL.
*			   After putting external SDRAM into self-refresh state,the function
*			   DRAMC_power_save_process() is called to disable DRAMC ITM and DLL, then disable PLL to save power;
*			   Before exit SDRAM self-refresh state, we should enable DRAM PLL and make sure that it is stable clock.
*			   Then call function DRAMC_exit_selfrefresh() to exit self-refresh state. Before access external SDRAM,
*              the function DRAMC_power_up_process() should be called to enable DLL and re-training DRAM controller.
*
* Arguments  : none
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/

void dram_power_save_process(void)
{
	__u32 reg_val;

	mctl_itm_disable();
	mctl_disable_dll();
}
__u32 dram_power_up_process(void)
{
	__u32 i;
	__u32 ret_val;

	mctl_itm_disable();

	mctl_enable_dll0();
	standby_delay(0x10);

	mctl_enable_dllx();
	standby_delay(0x10);

	//scan read pipe value
	mctl_itm_enable();
	ret_val = DRAMC_scan_readpipe();

	return (ret_val);
}


void dram_enter_selfrefresh(void)
{
    DRAMC_enter_selfrefresh();
}


void dram_exit_selfrefresh(void)
{
    DRAMC_exit_selfrefresh();
}


void dram_enter_power_down(void)
{
    DRAMC_enter_power_down();
}


void dram_exit_power_down(void)
{
    DRAMC_exit_power_down();
}


void dram_hostport_on_off(__u32 port_idx, __u32 on)
{
    DRAMC_hostport_on_off(port_idx, on);
}


__u32 dram_hostport_check_ahb_fifo_status(__u32 port_idx)
{
    return DRAMC_hostport_check_ahb_fifo_status(port_idx);
}


void dram_hostport_setup(__u32 port, __u32 prio, __u32 wait_cycle, __u32 cmd_num)
{
    DRAMC_hostport_setup(port, prio, wait_cycle, cmd_num);
}

