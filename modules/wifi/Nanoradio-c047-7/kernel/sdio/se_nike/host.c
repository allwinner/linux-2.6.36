#include "host.h"

#include "sd_defs.h"
#include "nanoutil.h"
#include "nanoutil.c"
#include "macro.h"
#include "host_api.h"

#define SE_NIKE_MAGIC_VERSION "nanoradio wifi driver magic version 1.alpha"

static unsigned int is_4bit_sdio;

///===============================================================================================
/// step:
///		reset host 
///		set block size,power ctrl,timout
/// 	initialize normal interrupt status enable register
///		erase normal interrupt status register
///		Erase Error Interrupt Status Register
///		Initialize Error Interrupt Status Enable Register
///		conflict enable and hold enable 
///		set tx rx feedback delay
///  	change clk src
///		
int
host_init(transfer_mode_t transfer_mode)
{
	volatile short status;
		
	TRSP_MSG("ENTRY");       
	printk("Driver's Magic Version : %s\n", SE_NIKE_MAGIC_VERSION);
	
	switch(transfer_mode) {

	case TRANSFER_MODE_1BIT_SDIO:
		is_4bit_sdio = 0;
		break;
		
	case TRANSFER_MODE_4BIT_SDIO:
		is_4bit_sdio = 1;
		break;
		
	default:
		TRSP_ASSERT(0);
		return -EINVAL;
	}
	
	// reset sd host 
    sd_reset_host();
	write_16(SDHC_BLK_SIZE, SDAL_DEFAULT_SIZE);
	write_8(SDHC_HOST_CTRL, HOSTCON_LED_ON);
	write_8(SDHC_PWR_CTRL, POWCON_BUS_VOLTAGE33);
	write_8(SDHC_TIMEOUT_CTRL, TIMEOUTCON_DATA_TIMEOUT_27);	
	
	
	// initialize normal interrupt status enable register
	status = read_16(SDHC_NORMAL_INT_STAT_ENABLE);
	status |= (NORINTSTAEN_TRANS_COMPLETE|NORINTSTAEN_CMD_COMPLETE);
// disable interupt  for masking incorrect 
// interrupts that may occur while changing the bus width
	if (is_4bit_sdio)
		status &= ~NORINTSTAEN_GENERATE_CARD_INT;
	write_16(SDHC_NORMAL_INT_STAT_ENABLE, status);
	
	// erase normal interrupt status register
	write_16(SDHC_NORMAL_INT_STAT, ~REG_CLEAR_16BIT);

	// Erase Error Interrupt Status Register
	write_16(SDHC_ERROR_INT_STAT, ~REG_CLEAR_16BIT);
	
	// Initialize Error Interrupt Status Enable Register
	status = read_16(SDHC_ERROR_INT_STAT_ENABLE);
	status |= (ERRINTSTAEN_DATA_ENDBIT|ERRINTSTAEN_DATA_CRC|ERRINTSTAEN_DATA_TIMEOUT|
			   ERRINTSTAEN_CMD_INDEX|ERRINTSTAEN_CMD_ENDBIT|ERRINTSTAEN_CMD_CRC|
			   ERRINTSTAEN_CMD_TIMEOUT);
	write_16(SDHC_ERROR_INT_STAT_ENABLE, status);
	
	
	// conflict enable and hold enable 
	or_32(SDHC_CONTROL2, CON2_CMD_CONFLICT_MASK_EN|CON2_SDCLK_HOLD_ENABLE);

	// active delay #1 - TX delay1, RX delay1
	// normal sd - 24MHz OK
	// high sd - 27MHz OK
	// mmc+ - NG
	or_32(SDHC_CONTROL2, CON2_TX_FEEDBACK_CLK_EN|CON2_RX_FEEDBACK_CLK_EN);
	write_32(SDHC_CONTROL3, CON3_TX_FEEDBACK_CLK_DELAY1);
	
	// change clk src
	sd_change_clk_src(SDAL_CLOCK_SRC_EPLL);
	
	// set 4bit 
	if (is_4bit_sdio) {
		sd_select_bit_width(SDAL_BUS_WIDTH_4BIT);
		or_16(SDHC_NORMAL_INT_STAT_ENABLE, NORINTSTAEN_GENERATE_CARD_INT);
	}
	else
		sd_select_bit_width(SDAL_BUS_WIDTH_1BIT);
	
	TRSP_MSG( "EXIT");
	// printk("CAPABILITIES REGISTER : %x\n " , s3c_hsmmc_readl(HM_CAPAREG));
	return 0;
}
///===============================================================================================
int host_reset(){
	TRSP_MSG( "ENTRY");      

	TRSP_MSG( "EXIT");
	return 0;
}
///===============================================================================================
void host_exit(void){
	TRSP_MSG( "ENTRY");      

	TRSP_MSG( "EXIT");
}
///===============================================================================================
/// step:
/// 	set timeout
/// 	stop external clock
/// 	hold enable
/// 	set tx rx feedback delay
/// 	enable clock with division
/// 	wait to stable the internal clock
/// 	enable external clock 
/// 	wait to stable the external clock
///
void host_clock(clk_mode_t clk_mode){
	short div=0;
	short val ;
	TRSP_MSG( "ENTRY");   	
	write_8(SDHC_TIMEOUT_CTRL, TIMEOUTCON_DATA_TIMEOUT_27);
	
	// stop clock
	and_16(SDHC_CLK_CTRL,~CLKCON_SDCLK_EN);
		   
	switch(clk_mode) {
		  case CLK_MODE_FAST:
			     // Divide HCLCK for SDCLK by 1 ~48MHZ 
			     div = CLKCON_SDCLK_DIV1;
			     break;

		  case CLK_MODE_SLOW:
				  // Divide HCLCK for SDCLK by 8 ~9MHZ 
				  div = CLKCON_SDCLK_DIV8;
				  break;

		  case CLK_MODE_OFF:
				  sd_start_timer(10);
				  while (!(( val=read_16(SDHC_CLK_CTRL))& CLKCON_EXTCLK_STABLE )) {
						if(SDR_OK!=sd_check_timer()){
								TRSP_MSG( "a : error in clock stabilization: %08x\n", val);
								return;
						}
				  }
				  return;
				  
			default: 
				// anable again the clk 
				or_16(SDHC_CLK_CTRL,CLKCON_SDCLK_EN);
				TRSP_ASSERT(0);
  }
	// hold enable 
	write_32(SDHC_CONTROL2,CON2_SDCLK_HOLD_ENABLE);
	write_32(SDHC_CONTROL3,CON3_TX_FEEDBACK_CLK_DELAY2 | CON3_RX_FEEDBACK_CLK_DELAY4 );

	// Div : 0x40 for division 256 instead we shall try about 0x2 -> divide by 4
	// (base clock maybe 48M ?
	// enable internal clock
    write_16(SDHC_CLK_CTRL,div|CLKCON_INTCLK_EN);

	// wait to stable the internal clock
	sd_start_timer(10);
	while (!(( val=read_16(SDHC_CLK_CTRL)) & CLKCON_INTCLK_STABLE )) {
		if(SDR_OK!=sd_check_timer()){
					TRSP_MSG( "a : error in clock stabilization: %08x\n", val);
					return;
					}
	}

	// enable external clock again
	write_16(SDHC_CLK_CTRL,div|CLKCON_INTCLK_EN|CLKCON_SDCLK_EN);

	// check if the external clock is stable
	sd_start_timer(10);
	while (!(( val=read_16(SDHC_CLK_CTRL))& CLKCON_EXTCLK_STABLE )) {
			if(SDR_OK!=sd_check_timer()){
					TRSP_MSG( "a : error in clock stabilization: %08x\n", val);
					return;
			}
	}

	TRSP_MSG( "EXIT");
}
///===============================================================================================
void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
	TRSP_MSG( "ENTRY");      

	TRSP_MSG( "EXIT");
}
///===============================================================================================
int host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{
	volatile short reg = 0 ;
	volatile short reason = 0;
	TRSP_MSG( "ENTRY");    
	
	switch (irq_id) {
		case IRQ_ID_RESPONSE_COMPLETION:
			TRSP_MSG("IRQ_ID_RESPONSE_COMPLETION");
			reason |= NORINTSTA_CMD_COMPLETE;
			break;
		case IRQ_ID_RW_ACCESS_COMPLETION:
			reason |= NORINTSTA_TRANS_COMPLETE;
			break;
		case IRQ_ID_FIFO_READ_READY:
			TRSP_MSG("IRQ_ID_FIFO_READ_READY");
			reason |= NORINTSTA_BUF_READ_READY;
			break;
		case IRQ_ID_FIFO_WRITE_READY:
			TRSP_MSG("IRQ_ID_FIFO_WRITE_READY");
			reason |= NORINTSTA_BUF_WRITE_READY;
			break;
		case IRQ_ID_TARGET:
			reason |= NORINTSTA_GENERATE_CARD_INT;
			break;

		default:
			TRSP_ASSERT(0);
	}

	switch (irq_op) {
		case IRQ_OP_ENABLE:

			or_16(SDHC_NORMAL_INT_STAT_ENABLE,reason);
			TRSP_ASSERT(read_16(SDHC_NORMAL_INT_STAT_ENABLE) & reason);
			
			or_16(SDHC_NORMAL_INT_SIGNAL_ENABLE,reason);
			TRSP_ASSERT(read_16(SDHC_NORMAL_INT_SIGNAL_ENABLE) & reason);
			
			break;
		case IRQ_OP_DISABLE:
			
			and_16(SDHC_NORMAL_INT_SIGNAL_ENABLE,(~reason));
			TRSP_ASSERT((!(read_16(SDHC_NORMAL_INT_SIGNAL_ENABLE) & reason)));
			
			and_16(SDHC_NORMAL_INT_STAT_ENABLE,(~reason));
			TRSP_ASSERT((!(read_16(SDHC_NORMAL_INT_STAT_ENABLE) & reason)));

			break;
		case IRQ_OP_STATUS:			  
			TRSP_ASSERT((read_16(SDHC_NORMAL_INT_STAT) & reason));
			reg = read_16(SDHC_NORMAL_INT_STAT);
			reg &= reason;
			return reg;
		case IRQ_OP_ACKNOWLEDGE:
			break;
	}
	
	TRSP_MSG( "EXIT");
	return 0;
}
///===============================================================================================
int
host_fifo(void *data, unsigned long data_phys, uint16_t len, uint32_t flags)
{
  unsigned int *ptr = (unsigned int *) data;

  TRSP_MSG( "ENTER");

  if (flags & FIFO_FLAG_TO_HOST) {
    sd_sdio_read_from_fifo(ptr, len);
  } else {
    sd_sdio_write_to_fifo(ptr, len);
  }

  sdio_fifo_callback();
  TRSP_MSG( "EXIT");

  return 0;
}
///===============================================================================================
/// step:
///		write arg tou argument register
///		Check Command Inhibit (CMD)
/// 	check and set for CMD direction
///		write command to the register
///		wait for CMD complete 
///		clean the cmd complete flag(interrupt)
///
irq_status_t
host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
	 uint32_t flags, uint32_t arg, void *data)
{
	uint32_t cmd_val = 0;
	uint32_t mask __attribute__ ((unused));
	uint32_t mode __attribute__ ((unused));
	volatile unsigned long timeout = BUSY_WAIT_TIMEOUT_VALUE;
	long unsigned int present __attribute__ ((unused));
	uint32_t len =  arg & 0x000003FF ;

	KDEBUG(TRANSPORT, "ENTRY cmd: 0x%02x, arg: 0x%08x , len: %d", cmd, arg,len);

	TRSP_ASSERT(cmd_resp == CMD_RESP_R1 || cmd_resp == CMD_RESP_R5
		    || cmd_resp == CMD_RESP_R6);

	// write the arg 
	write_16(SDHC_ARG,arg);
	
	/// shift data to the cmd position
	cmd_val = (cmd << 8);
	/* replase:
	cmd_val |= (1 << 1);	// CMD Responses R1, R5, R6
	cmd_val |= (1 << 3); // CMD CRC check Enable
	cmd_val |= (1 << 4); // CMD INDEX check Enable
	with:
	*/
	cmd_val |= CMD_RESP_48B_NOBUSY_R1R5R6R7;

	if (flags & CMD_FLAG_DATA_EXISTS) {
#if 1
	/// check for th dat line
	   do {change clk src
		if(!timeout) {
			dump_regs(__FUNCTION__, __LINE__);
		  return IRQ_STATUS_FAIL;
		}
		timeout--;
	   } while (read_32(SDHC_PRESENT_STAT) & (PRESTATE_DATA_CMD_INHIBIT)) ;
#endif
		cmd_val |= CMD_DAT_PRESENT;
  }

	if (cmd == SDIO_RW_EXTENDED) {
		mode = 0;

		if (flags & CMD_FLAG_DIR_DATA_TO_HOST)
			write_16(CMD_DAT_PRESENT,TRANSMODE_DIRECTION_READ);
		else
			write_16(CMD_DAT_PRESENT,TRANSMODE_NO_CCS_CON);
	}
	
	write_16(SDHC_COMMAND,cmd_val);

#if 1
	/// check for cmd complete
    do {
     if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
      return IRQ_STATUS_FAIL;
     }
     timeout --;
    } while(!(read_16(SDHC_NORMAL_INT_STAT) & (NORINTSTA_CMD_COMPLETE))); 

	/// clean cmd complete interupt
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
    do {
       if(!timeout) {
	    dump_regs(__FUNCTION__, __LINE__);
        return IRQ_STATUS_FAIL;
       }
       timeout --;
	   write_16(SDHC_NORMAL_INT_STAT,NORINTSTA_CMD_COMPLETE);
    } while((read_16(SDHC_NORMAL_INT_STAT) & (1 << 0))); 
#endif


	//dump_regs(__FUNCTION__, __LINE__);
	KDEBUG(TRANSPORT, "EXIT");
	return IRQ_STATUS_SUCCESS;
}
///===============================================================================================
uint32_t
host_control(ctl_id_t ctl_id, uint32_t param)
{
	volatile uint16_t mask __attribute__ ((unused));
	KDEBUG(TRANSPORT, "ENTRY");
	// TODO : Host Ctrl Implementation.
	switch (ctl_id) {
	case CTL_ID_LENGTH:
		write_16(SDHC_BLK_SIZE,param);
		break;

	case CTL_ID_WAIT_CMD_READY:
#if 1
		while (read_32(SDHC_PRESENT_STAT) & PRESTATE_NONDATA_CMD_INHIBIT );
#endif
		break;

	case CTL_ID_RESPONSE:
		return (uint32_t) (read_32(SDHC_RSP01));
		break;

	default:
		TRSP_ASSERT(0);
	}
	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}
