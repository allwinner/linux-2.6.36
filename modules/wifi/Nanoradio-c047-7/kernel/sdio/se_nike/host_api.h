/// Nanoradio 
/// Samsung NIKE platform
/// api to settings the host controler
///
/// author Vassiliou Petros  

#include "sd_timer.h"
#define BUSY_WAIT_TIMEOUT_VALUE 10000

// declaration
void sd_clock_onoff(int onoff);
void sd_clock_change(short divider);
void sd_card_clock_onoff(int onoff);
void sd_select_bit_width(char bus_width);
void sd_bus_power_onoff(int onoff);
void sd_change_clk_src( char clk_src);
void sd_reset_host(void);
int  sd_check_normal_int_sta(int *NormalStatus);
int sd_wait_condition(short condition);
int sd_command( short command_reg, char response_type);
static void sd_sdio_read_from_fifo(unsigned int *buf, unsigned int total);
static void sd_sdio_write_to_fifo(unsigned int *buf, unsigned int total);
void dump_regs(const char *label, int line);

///======================================================================
void sd_clock_onoff(int onoff){
	if (onoff == SDAL_CARD_CLOCK_ON) {
		or_16(SDHC_CLK_CTRL, CLKCON_SDCLK_EN);
	}
	else {
		and_16(SDHC_CLK_CTRL, ~CLKCON_SDCLK_EN);
	}
}


///======================================================================
// Set the clock by giving the CLKCON_SDCLK_DIV*
void sd_clock_change(short divider){

	// SD clock off
	sd_clock_onoff(SDAL_CARD_CLOCK_OFF);
	
	and_16(SDHC_CLK_CTRL,~CLKCON_SDCLK_DIV_FIELD);
	or_16(SDHC_CLK_CTRL,divider);

	// SD clock on
	sd_clock_onoff(SDAL_CARD_CLOCK_ON);	
}

///======================================================================
/// switch sd clock
void sd_card_clock_onoff( int onoff)
{
	if (onoff == SDAL_CARD_CLOCK_ON) {
		or_16(SDHC_CLK_CTRL, (CLKCON_SDCLK_EN));
	}
	else {
		and_16(SDHC_CLK_CTRL, (~CLKCON_SDCLK_EN));
	}
}

///======================================================================
// select the bus width that we have
// 1bit or 4bit
void sd_select_bit_width(char bus_width)
{
	if (bus_width == SDAL_BUS_WIDTH_4BIT) {
		and_8(SDHC_HOST_CTRL, ~HOSTCON_DATA_WIDTH8);
		or_8(SDHC_HOST_CTRL, HOSTCON_DATA_WIDTH4);
	}
	else if(bus_width== SDAL_BUS_WIDTH_1BIT) {
		and_8(SDHC_HOST_CTRL, ~(HOSTCON_DATA_WIDTH8|HOSTCON_DATA_WIDTH4));
	}
}
///======================================================================
/// switch bus power
void sd_bus_power_onoff(int onoff)
{
	if (onoff == SDAL_BUS_POWER_ON) {
        or_8(SDHC_PWR_CTRL, POWCON_BUS_POWER_ON);
	}
	else {
		and_8(SDHC_PWR_CTRL, ~POWCON_BUS_POWER_ON);
	}
}

///======================================================================
// change sd clock source (s5c7329 specific function)
//00 or 01 =HCLK, 10=EPLL out Clock (from SYSCON), 11=External Clock 
//source (XTI or XEXTCLK)sd_reset_host
void sd_change_clk_src( char clk_src)
{
	// sd clock off
	sd_card_clock_onoff(SDAL_CARD_CLOCK_OFF);
	
	// disable internal clock
	and_16(SDHC_CLK_CTRL, ~CLKCON_INTCLK_EN);

	// select clock source
	and_32(SDHC_CONTROL2, ~CON2_CLK_SRC_FIELD);
	if (clk_src == SDAL_CLOCK_SRC_HCLK) {
		or_32(SDHC_CONTROL2, CON2_CLK_SRC_HCLK);
	}
	else if (clk_src == SDAL_CLOCK_SRC_EPLL) {
		or_32(SDHC_CONTROL2, CON2_CLK_SRC_SDCLK);
	}

	// enable internal clock
	write_16(SDHC_CLK_CTRL, CLKCON_INTCLK_EN);
	// wait until  "Internal Clock Stable" be ready
	while (1) {
		if ((read_16(SDHC_CLK_CTRL)) & CLKCON_INTCLK_STABLE) {
			break;
		}
	}
	
	// sd bus power on (must set after selecting voltage)
	sd_bus_power_onoff(SDAL_BUS_POWER_ON);

	sd_wait(SD_POWUP_WAIT);
}



///======================================================================
///  SD host I/F initialize
void sd_reset_host(void)
{
    // reset, clear register
	// write initial vaule for reset
	or_8(SDHC_SOFTWARE_RESET, SWRESET_DATLINE|SWRESET_CMDLINE);	
}
///======================================================================
/// check normal status register
int  sd_check_normal_int_sta(int *NormalStatus)
{
	*NormalStatus = read_16(SDHC_NORMAL_INT_STAT);

	if ((*NormalStatus) & NORINTSTA_ERR) {
		return SDR_ERR;
	}
	
	return SDR_OK;
}

///======================================================================
///  wait until satisfying given condition
///  condition = status which need to set
///  RETURN     : SDR_OK  = normal
///  			  SDR_ERR = error
int sd_wait_condition(short condition)
{
	int ret;
	int NormalStatus;
	ret = SDR_OK;
	sd_start_timer(10);
	do{
		if( SDR_OK != (ret = sd_check_normal_int_sta(&NormalStatus))) {
			break;
		}

		// check for time out
		if(SDR_OK!=(ret=sd_check_timer())){
			break;
		}
    } while (!(NormalStatus & condition));

	return ret;
}

///======================================================================
///  issue command with checking environment
///			  [in] command_reg   = command register value
///			  [in] response_type = SDAL_RESP_NONE
///								   SDAL_RESP_TYPE1
///								   SDAL_RESP_TYPE1B
///								   SDAL_RESP_TYPE2
///								   SDAL_RESP_TYPE3
///								   SDAL_RESP_TYPE6
///								   SDAL_RESP_TYPE7
///								   SDAL_RESP_TYPE_SDIO
/// RETURN     : SDR_OK  = normal
///			  SDR_ERR = error
int sd_command( short command_reg, char response_type)
{
	int ret;
	volatile long present_reg;

	ret = SDR_OK;
	
	// clear interrupt status
	sd_reset_host();

	// clear error interupt
	write_16(SDHC_ERROR_INT_STAT,~REG_CLEAR_16BIT);

	if (command_reg & CMD_DAT_PRESENT) {
		or_16(SDHC_NORMAL_INT_STAT,NORINTSTA_CMD_COMPLETE|NORINTSTA_TRANS_COMPLETE|
												NORINTSTA_BUF_READ_READY|NORINTSTA_BUF_WRITE_READY);
	}
	else {
		or_16(SDHC_NORMAL_INT_STAT,NORINTSTA_CMD_COMPLETE|NORINTSTA_TRANS_COMPLETE);
	}

	sd_wait(SD_WAIT_1MS);
	sd_start_timer(10);
	while (1) {
		present_reg = read_32(SDHC_PRESENT_STAT);

		// check for the Command Inhibit CMD
		if (response_type == SDAL_RESP_TYPE1B) {
			if (!(present_reg & PRESTATE_DATA_CMD_INHIBIT)) {
				break;
			}
		}
		else {
			if (!(present_reg & PRESTATE_NONDATA_CMD_INHIBIT)) {
				break;
			}
		}
		
		// check for time out
		if(SDR_OK!=(ret=sd_check_timer())){
			return ret;
		}

	}

	// write the command to the command register
	write_16(SDHC_COMMAND,command_reg);

    // wait response
	ret = sd_wait_condition( NORINTSTA_CMD_COMPLETE);

	return ret;
}
///===============================================================================
/// step:
/// 	Wait for Buffer Read Ready Int 
///  	Clr Buffer Read Ready Status
///		Get Block Data
///		Wait for Transfer Complete Int
///		Clr Transfer Complete Status
static void sd_sdio_read_from_fifo(unsigned int *buf, unsigned int total)
{    
    volatile unsigned int *ptr = buf;
    volatile unsigned long timeout = BUSY_WAIT_TIMEOUT_VALUE;
    volatile short err = 0;

    total = (total + sizeof(unsigned int) - 1) / sizeof(unsigned int);

    err = read_16(SDHC_ERROR_INT_STAT);
	// check if we have error
    if(err) {
		TRSP_ASSERT(!err);
		dump_regs(__FUNCTION__, __LINE__);
		
		// clean the error register by write 1 to the error field
		 do {
			write_16(SDHC_ERROR_INT_STAT, err);
		 } while((read_16(SDHC_ERROR_INT_STAT) & (err)));
		 
		 return;
    }

	// check if the buffer is ready for reading
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
       return;
      }
      timeout--;
    } while(!(read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_BUF_READ_READY)); 

	// reset time out
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	//try to clean the interupt for the buffer read ready
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
		return;
      }
      timeout--;
	  write_16(SDHC_NORMAL_INT_STAT,NORINTSTA_BUF_READ_READY);
    } while((read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_BUF_READ_READY));


	// read data
    while(total)
    {
            *ptr = read_32(SDHC_BUF_DAT_PORT);
            total--;
            ptr++;
    }

	// reset time out value
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	// wait for transaction complete interupt
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
       return;
      }
      timeout--;
    } while(!(read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_TRANS_COMPLETE)); 

	// reset time out value
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	// try to clean the transaction complete interupt
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
       return;
      }
      timeout--;
	  write_16(SDHC_NORMAL_INT_STAT,NORINTSTA_TRANS_COMPLETE);
    } while((read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_TRANS_COMPLETE));

}
///===============================================================================
/// step:
/// 	Wait for Buffer Write Ready Int 
///  	Clr Buffer Read Write Status
///		Set Block Data
///		Wait for Transfer Complete Int
///		Clr Transfer Complete Status
static void sd_sdio_write_to_fifo(unsigned int *buf, unsigned int total)
{
   
    volatile unsigned int *ptr = buf;
    volatile unsigned long timeout = BUSY_WAIT_TIMEOUT_VALUE;
    volatile uint16_t err = 0;
  
    total = (total + sizeof(unsigned int) - 1) / sizeof(unsigned int);
  
    
    err = read_16(SDHC_ERROR_INT_STAT);
	// check if we have error
    if(err) {
		TRSP_ASSERT(!err);
		dump_regs(__FUNCTION__, __LINE__);
		
		// clean the error register by write 1 to the error field
		 do {
			write_16(SDHC_ERROR_INT_STAT, err);
		 } while((read_16(SDHC_ERROR_INT_STAT) & (err)));
		 
		 return;
    }

	// check if the buffer is ready for Write
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
       return;
      }
      timeout--;
    } while(!(read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_BUF_WRITE_READY)); 

	// reset time out
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	//try to clean the interupt for the buffer write ready
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
		return;
      }
      timeout--;
	  write_16(SDHC_NORMAL_INT_STAT,NORINTSTA_BUF_WRITE_READY);
    } while((read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_BUF_WRITE_READY));

	// set data
    while (total) 
    {
			write_32(SDHC_BUF_DAT_PORT,*ptr);
            total--;
            ptr++;
    }
    
	// reset time out value
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	// wait for transaction complete interupt
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
       return;
      }
      timeout--;
    } while(!(read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_TRANS_COMPLETE)); 

	// reset time out value
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	// try to clean the transaction complete interupt
    do {
      if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
       return;
      }
      timeout--;
	  write_16(SDHC_NORMAL_INT_STAT,NORINTSTA_TRANS_COMPLETE);
    } while((read_16(SDHC_NORMAL_INT_STAT) & NORINTSTA_TRANS_COMPLETE));
}
///===============================================================================
void
dump_regs(const char *label, int line)
{
	printk("\n################## |         Dumping Regs at func [ %s ] and line : %d           | #####################\n",
			label, line);
	printk("SDHC_SYS_ADDR\t\t\t\t: 0x%x\n",read_16(SDHC_SYS_ADDR));
	printk("SDHC_BLK_SIZE\t\t\t\t: 0x%x\n",read_16(SDHC_BLK_SIZE));
	printk("SDHC_BLK_COUNT\t\t\t\t: 0x%x\n",read_16(SDHC_BLK_COUNT));
	printk("SDHC_ARG\t\t\t\t: 0x%x\n",read_16(SDHC_ARG));
	printk("SDHC_TRANS_MODE\t\t\t\t: 0x%x\n",read_16(SDHC_TRANS_MODE));
	printk("SDHC_COMMAND\t\t\t\t: 0x%x\n",read_16(SDHC_COMMAND));
	printk("SDHC_RSP01\t\t\t\t: 0x%x\n",(unsigned int)read_32(SDHC_RSP01));
	printk("SDHC_RSP23\t\t\t\t: 0x%x\n",(unsigned int)read_32(SDHC_RSP23));
	printk("SDHC_RSP45\t\t\t\t: 0x%x\n",(unsigned int)read_32(SDHC_RSP45));
	printk("SDHC_RSP67\t\t\t\t: 0x%x\n",(unsigned int)read_32(SDHC_RSP67));
	printk("SDHC_BUF_DAT_PORT\t\t\t\t: 0x%x\n",read_16(SDHC_BUF_DAT_PORT));
	printk("SDHC_PRESENT_STAT\t\t\t\t: 0x%x\n",(unsigned int)read_32(SDHC_PRESENT_STAT));
	printk("SDHC_HOST_CTRL\t\t\t\t: 0x%x\n",(unsigned int)read_8(SDHC_HOST_CTRL));
	printk("SDHC_PWR_CTRL\t\t\t\t: 0x%x\n",(unsigned int)read_8(SDHC_PWR_CTRL));
	printk("SDHC_BLOCKGAP_CTRL\t\t\t\t: 0x%x\n",(int)read_8(SDHC_BLOCKGAP_CTRL));
	printk("SDHC_WAKEUP_CTRL\t\t\t\t: 0x%x\n",(int)read_8(SDHC_WAKEUP_CTRL));
	printk("SDHC_CLK_CTRL\t\t\t\t: 0x%x\n",read_16(SDHC_CLK_CTRL));
	printk("SDHC_TIMEOUT_CTRL \t\t\t\t: 0x%x\n",(int)read_8(SDHC_TIMEOUT_CTRL));
	printk("SDHC_SOFTWARE_RESET\t\t\t\t: 0x%x\n",read_16(SDHC_SOFTWARE_RESET));
	printk("SDHC_NORMAL_INT_STAT \t\t\t\t: 0x%x\n",read_16(SDHC_NORMAL_INT_STAT));
	printk("SDHC_ERROR_INT_STAT \t\t\t\t: 0x%x\n",read_16(SDHC_ERROR_INT_STAT));
	printk("SDHC_NORMAL_INT_STAT_ENABLE\t\t\t\t: 0x%x\n",read_16(SDHC_NORMAL_INT_STAT_ENABLE));
	printk("SDHC_ERROR_INT_STAT_ENABLE  \t\t\t\t: 0x%x\n",read_16(SDHC_ERROR_INT_STAT_ENABLE));
	printk("SDHC_NORMAL_INT_SIGNAL_ENABLE \t\t\t\t: 0x%x\n",read_16(SDHC_NORMAL_INT_SIGNAL_ENABLE));
	printk("SDHC_ERROR_INT_SIGNAL_ENABLE \t\t\t\t: 0x%x\n",read_16(SDHC_ERROR_INT_SIGNAL_ENABLE));
	printk("SDHC_AUTO_CMD12_ERR_STAT\t\t\t\t: 0x%x\n",read_16(SDHC_AUTO_CMD12_ERR_STAT));
	printk("SDHC_CAPA\t\t\t\t: 0x%x\n",(int)read_32(SDHC_CAPA));
	printk("SDHC_MAX_CURRENT_CAPA\t\t\t\t: 0x%x\n",(int)read_32(SDHC_MAX_CURRENT_CAPA));
	printk("SDHC_CONTROL2\t\t\t\t: 0x%x\n",(int)read_32(SDHC_CONTROL2));
	printk("SDHC_CONTROL3\t\t\t\t: 0x%x\n",(int)read_32(SDHC_CONTROL3));
	printk("SDHC_HOST_CONTROLLER_VERSION \t\t\t\t: 0x%x\n",read_16(SDHC_HOST_CONTROLLER_VERSION));
	printk("\n########################################################################################################\n");

}
