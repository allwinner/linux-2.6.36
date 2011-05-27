

/******************************************************************************
S I G M A T E L    S T M P 3 6 x x
******************************************************************************/


#include <host.h>
#include <stdbool.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <asm/arch/regsssp.h>
#include <asm/hardware.h>
#include <asm/arch/regsclkctrl.h>
#include "include/stmp36xx_gpio.h"

#include "nanoutil.h"

MODULE_LICENSE("GPL");

/* GPIO PINS */
#define STMP36XX_GPIO_GPMID11_BANK        0
#define STMP36XX_GPIO_GPMID11_BIT         11
#define STMP36XX_GPIO_GPMID11_MUX         0
#define STMP36XX_GPIO_GPMID11_FN          0

#define STMP36XX_GPIO_GPMID12_BANK        0
#define STMP36XX_GPIO_GPMID12_BIT         12
#define STMP36XX_GPIO_GPMID12_MUX         0
#define STMP36XX_GPIO_GPMID12_FN          0

#define STMP36XX_GPIO_GPMID13_BANK        0
#define STMP36XX_GPIO_GPMID13_BIT         13
#define STMP36XX_GPIO_GPMID13_MUX         0
#define STMP36XX_GPIO_GPMID13_FN          0

#define PBANKMASK(pin) ((uint32_t) 1 << STMP36XX_GPIO_##pin##_BIT)
#define PSELPOS(pin)   ((STMP36XX_GPIO_##pin##_BIT * 2) % 32)
#define PSELVAL(pin)   ((uint32_t) STMP36XX_GPIO_##pin##_FN << PSELPOS(pin))
#define PSELMASK(pin)  ((uint32_t) _STMP36XX_GPIO_FN << PSELPOS(pin))

/* Timer stuff */
#define US_TIME_1US   ((uint64_t) 1U)
#define US_TIME_1MS   ((uint64_t) 1000U)
#define US_TIME_1SEC  ((uint64_t) 1000000U)
#define US_TIME_1MIN  ((uint64_t) 60000000U)
#define US_TIME_1HR   ((uint64_t) 3600000000U)

typedef struct
{
	uint64_t  expiration;
	uint64_t  elapsed;
	reg32_t   prior;
	bool    expired;
} us_timer_t;

void     us_timer_start(volatile us_timer_t* timer, uint64_t expiration);
uint64_t us_timer_elapsed(volatile us_timer_t* timer);
bool     us_timer_expired(volatile us_timer_t* timer);
void     us_timer_delay(uint64_t duration);

void 	 print_ctrl0(void);
void     print_status(void);	

#define HOST_POLL(cond, duration)                \
{                                                \
        us_timer_t  timer;                       \
        us_timer_start(&timer, duration);        \
        while((cond) && !timer.expired)          \
               us_timer_expired(&timer);         \
        if (timer.expired) panic("timeout %s!\n", #cond);  \
}                                                


void us_timer_start(volatile us_timer_t* timer, uint64_t expiration)
{
	timer->prior = HW_DIGCTL_MICROSECONDS_RD();
	timer->elapsed = 0;
	timer->expiration = expiration;
	timer->expired = false;
}

uint64_t us_timer_elapsed(volatile us_timer_t* timer)
{
	uint32_t now = HW_DIGCTL_MICROSECONDS_RD();

	if (now < timer->prior)
	{
		timer->elapsed += ((0xFFFFFFFF - timer->prior + 1) + now);
	}
	else if (now >= timer->prior)
	{
		timer->elapsed += (now - timer->prior);
	}

	timer->prior = now;

	return timer->elapsed;
}

bool us_timer_expired(volatile us_timer_t* timer)
{
	if (us_timer_elapsed(timer) >= timer->expiration)
	{
		timer->expired = true;
	}

	return timer->expired;
}

void us_timer_delay(uint64_t duration)
{
	us_timer_t timer;

	// load expiration time 1 hour from now
	us_timer_start(&timer, duration);

	while (!us_timer_expired(&timer))
	{
		// wait for timer to expire
	}

}

void print_ctrl0(void)
{
	TRSP_MSG("HW_SSP_CTRL0.U           = 0x%x\n", HW_SSP_CTRL0.U);
	TRSP_MSG(" .B.XFER_COUNT           = %d\n", HW_SSP_CTRL0.B.XFER_COUNT);
	TRSP_MSG(" .B.ENABLE               = %d\n", HW_SSP_CTRL0.B.ENABLE);
	TRSP_MSG(" .B.GET_RESP             = %d\n", HW_SSP_CTRL0.B.GET_RESP);
	TRSP_MSG(" .B.CHECK_RESP           = %d\n", HW_SSP_CTRL0.B.CHECK_RESP);
	TRSP_MSG(" .B.LONG_RESP            = %d\n", HW_SSP_CTRL0.B.LONG_RESP);
	TRSP_MSG(" .B.WAIT_FOR_CMD         = %d\n", HW_SSP_CTRL0.B.WAIT_FOR_CMD);
	TRSP_MSG(" .B.WAIT_FOR_IRQ         = %d\n", HW_SSP_CTRL0.B.WAIT_FOR_IRQ);
	TRSP_MSG(" .B.BUS_WIDTH            = %d\n", HW_SSP_CTRL0.B.BUS_WIDTH);
	TRSP_MSG(" .B.SDIO_IRQ             = %d\n", HW_SSP_CTRL0.B.SDIO_IRQ);
	TRSP_MSG(" .B.DATA_XFER            = %d\n", HW_SSP_CTRL0.B.DATA_XFER);
	TRSP_MSG(" .B.READX                = %d\n", HW_SSP_CTRL0.B.READX);
	TRSP_MSG(" .B.IGNORE_CRC           = %d\n", HW_SSP_CTRL0.B.IGNORE_CRC);
	TRSP_MSG(" .B.LOCK_CS              = %d\n", HW_SSP_CTRL0.B.LOCK_CS);
	TRSP_MSG(" .B.HALF_DUPLEX          = %d\n", HW_SSP_CTRL0.B.HALF_DUPLEX);
	TRSP_MSG(" .B.RUN                  = %d\n", HW_SSP_CTRL0.B.RUN);
	TRSP_MSG(" .B.CLKGATE              = %d\n", HW_SSP_CTRL0.B.CLKGATE);
	TRSP_MSG(" .B.SFTRST               = %d\n", HW_SSP_CTRL0.B.SFTRST);
}

void print_status(void)
{
	TRSP_MSG("HW_SSP_STATUS.          = 0x%x\n", HW_SSP_STATUS.U);
	TRSP_MSG(" .B.BUSY                = %d\n", HW_SSP_STATUS.B.BUSY);
	TRSP_MSG(" .B.DATA_XFER           = %d\n", HW_SSP_STATUS.B.DATA_XFER);
	TRSP_MSG(" .B.DATA_BUSY           = %d\n", HW_SSP_STATUS.B.DATA_BUSY);
	TRSP_MSG(" .B.CMD_BUSY            = %d\n", HW_SSP_STATUS.B.CMD_BUSY);
	TRSP_MSG(" .B.XMIT_UNDRFLW        = %d\n", HW_SSP_STATUS.B.XMIT_UNDRFLW);
	TRSP_MSG(" .B.XMIT_EMPTY          = %d\n", HW_SSP_STATUS.B.XMIT_EMPTY);
	TRSP_MSG(" .B.XMIT_NOT_FULL       = %d\n", HW_SSP_STATUS.B.XMIT_NOT_FULL);
	TRSP_MSG(" .B.RECV_NOT_EMPTY      = %d\n", HW_SSP_STATUS.B.RECV_NOT_EMPTY);
	TRSP_MSG(" .B.RECV_FULL           = %d\n", HW_SSP_STATUS.B.RECV_FULL);
	TRSP_MSG(" .B.RECV_OVRFLW         = %d\n", HW_SSP_STATUS.B.RECV_OVRFLW);
	TRSP_MSG(" .B.RECV_DATA_STAT      = %d\n", HW_SSP_STATUS.B.RECV_DATA_STAT);
	TRSP_MSG(" .B.RECV_TIMEOUT_STAT   = %d\n", HW_SSP_STATUS.B.RECV_TIMEOUT_STAT);
	TRSP_MSG(" .B.TIMEOUT             = %d\n", HW_SSP_STATUS.B.TIMEOUT);
	TRSP_MSG(" .B.DATA_CRC_ERR        = %d\n", HW_SSP_STATUS.B.DATA_CRC_ERR);
	TRSP_MSG(" .B.RESP_TIMEOUT        = %d\n", HW_SSP_STATUS.B.RESP_TIMEOUT);
	TRSP_MSG(" .B.RESP_ERR            = %d\n", HW_SSP_STATUS.B.RESP_ERR);
	TRSP_MSG(" .B.RESP_CRC_ERR        = %d\n", HW_SSP_STATUS.B.RESP_CRC_ERR);
	TRSP_MSG(" .B.SDIO_IRQ            = %d\n", HW_SSP_STATUS.B.SDIO_IRQ);
	TRSP_MSG(" .B.DMAEND              = %d\n", HW_SSP_STATUS.B.DMAEND);
	TRSP_MSG(" .B.DMAREQ              = %d\n", HW_SSP_STATUS.B.DMAREQ);
	TRSP_MSG(" .B.XMIT_COUNT          = %d\n", HW_SSP_STATUS.B.XMIT_COUNT);
	TRSP_MSG(" .B.RECV_COUNT          = %d\n", HW_SSP_STATUS.B.RECV_COUNT);
	TRSP_MSG(" .B.CARD_DETECT         = %d\n", HW_SSP_STATUS.B.CARD_DETECT);
	TRSP_MSG(" .B.SD_PRESENT          = %d\n", HW_SSP_STATUS.B.SD_PRESENT);
	TRSP_MSG(" .B.MS_PRESENT          = %d\n", HW_SSP_STATUS.B.MS_PRESENT);
	TRSP_MSG(" .B.PRESENT             = %d\n", HW_SSP_STATUS.B.PRESENT);
}




// descriptor de dma
typedef union _dma_desc_t dma_desc_t;
union _dma_desc_t
{
	uint32_t U[4];
	struct 
	{
		dma_addr_t next;
		union {
			hw_apbh_chn_cmd_t cmd;
			hw_apbx_chn_cmd_t cmdx;
		};    
		dma_addr_t buffer;
		uint32_t   pio[3];
	};
};

dma_desc_t *dma_desc;
dma_desc_t *dma_desc_pa;
uint8_t *dma_buf;
dma_addr_t dma_buf_pa;
void *buffer_data;

wait_queue_head_t wq;
uint32_t prev_dma_req;

int sdio_dma = 0;
int sdio_dma_irq = 1;


irqreturn_t dma_isr(int irq_num, void* dev_idp)
{
	irqreturn_t ret = IRQ_HANDLED;

	KDEBUG(TRANSPORT, "irq: %d\n", irq_num);
	
	if (HW_APBH_CTRL1.B.CH1_CMDCMPLT_IRQ) {
		TRSP_MSG("[ch1_cmd_cmplt]\n");

		HW_APBH_CTRL1_CLR(BM_APBH_CTRL1_CH1_CMDCMPLT_IRQ);
                
                //sdio_fifo_callback();
		wake_up(&wq);
	}
	else {
		TRSP_MSG("warning: irq not signalled!\n");
		ret = IRQ_NONE;
	}

	return ret;
}

int host_init(transfer_mode_t transfer_mode)
{
	int ret, len;

	KDEBUG(TRANSPORT, "enter\n");
	
	if (transfer_mode != TRANSFER_MODE_1BIT_SDIO) {
	  TRSP_MSG("Only 1-bit SDIO is supported.\n");
	  return -EINVAL;
	}
  
	// Reservamos memoria de DMA
	len = 4096;
	dma_desc = (dma_desc_t *) dma_alloc_coherent(0, len, (dma_addr_t *)&dma_desc_pa, GFP_DMA);
	if (dma_desc == NULL) {
		TRSP_MSG("error dma_alloc_coherent(SSP device)!\n");
		return -1;
	}
	dma_buf    = ((uint8_t *)dma_desc) + len/2;
	dma_buf_pa = ((dma_addr_t) ((uint8_t *)dma_desc_pa) + len/2);

	// Resetea el canal SD/MMC y esperamos a que finalize el reset
	TRSP_MSG("SSP dma channel reset\n");
	BW_APBH_CTRL0_RESET_CHANNEL(BV_APBH_CTRL0_RESET_CHANNEL__SSP);
	HOST_POLL(HW_APBH_CTRL0.B.RESET_CHANNEL & BV_APBH_CTRL0_RESET_CHANNEL__SSP, US_TIME_1SEC);

	// registramos la interrupcion
	ret = request_irq(IRQ_SSP_DMA, 
			  dma_isr, 
			  SA_INTERRUPT,
			  "stmp36xx_ssp_dma", 
			  NULL);
	if (ret != 0) {
		TRSP_MSG("error %d at request_irq(IRQ_SSP_DMA)!", ret);
		return -1;
	}
	
	init_waitqueue_head(&wq);

	// Armamos la interrupcion
	HW_APBH_CTRL1_CLR(BM_APBH_CTRL1_CH1_CMDCMPLT_IRQ);
	HW_APBH_CTRL1_SET(BM_APBH_CTRL1_CH1_CMDCMPLT_IRQ_EN);
 
	// Ajustamos los MUXSEL
        HW_PINCTRL_MUXSEL1_CLR(0xFFFC0000);
        HW_PINCTRL_MUXSEL0_SET(0x0FFC0000);

	KDEBUG(TRANSPORT, "exit\n");

        return 0;
}


int host_reset(void)
{
	uint32_t ctrl;
	int div;

	KDEBUG(TRANSPORT, "enter\n");
   
	// Activamos el reset del SSP
        HW_SSP_CTRL0_SET(BM_SSP_CTRL0_SFTRST);
        
	// La frecuencia del SSP con la que trabajamos por defecto es 45Mhz
	div = HW_CLKCTRL_PLLCTRL0.B.FREQ / 40;

	// Activamos el reloj y ponemos el divisor del PLL
        HW_CLKCTRL_SSPCLKCTRL_WR(BF_CLKCTRL_SSPCLKCTRL_CLKGATE(0) |
                                 BF_CLKCTRL_SSPCLKCTRL_DIV(div));
        
	// Esperamos a que el reloj este estable
	HOST_POLL(HW_CLKCTRL_SSPCLKCTRL.B.BUSY, US_TIME_1SEC);

	// Sacamos el bloque de reset
        HW_SSP_CTRL0_CLR(BM_SSP_CTRL0_SFTRST | BM_SSP_CTRL0_CLKGATE);
        
	// Valores por defecto para los registros
	ctrl =  BV_FLD(SSP_CTRL1, WORD_LENGTH, EIGHT_BITS) |
		BF_SSP_CTRL1_DMA_ENABLE(1)                 |
		BV_FLD(SSP_CTRL1, SSP_MODE, SD_MMC)        |
		BF_SSP_CTRL1_POLARITY(1);

	HW_SSP_CTRL1_WR(ctrl);

	ctrl =  BF_SSP_CTRL0_BUS_WIDTH(0)    |
		BF_SSP_CTRL0_SDIO_IRQ(1)     |
		BF_SSP_CTRL0_WAIT_FOR_IRQ(1) |
		BF_SSP_CTRL0_IGNORE_CRC(0)   |
		BF_SSP_CTRL0_GET_RESP(1);

	HW_SSP_CTRL0_WR(ctrl);


        ctrl =  BF_SSP_TIMING_TIMEOUT(0xFFFF) |
		BF_SSP_TIMING_CLOCK_DIVIDE(2) |
		BF_SSP_TIMING_CLOCK_RATE(0);

	HW_SSP_TIMING_WR(ctrl);

	KDEBUG(TRANSPORT, "exit\n");
	return 0;
}


void host_clock(clk_mode_t clk_mode)
{
	uint32_t ctrl;
 
       	KDEBUG(TRANSPORT, "enter\n");

        switch(clk_mode) 
	  {
	  case CLK_MODE_FAST:
	    ctrl =  BF_SSP_TIMING_TIMEOUT(0xFFFF) |
	      BF_SSP_TIMING_CLOCK_DIVIDE(2) |
	      BF_SSP_TIMING_CLOCK_RATE(0);
	    
	    HW_SSP_TIMING_WR(ctrl);
	    
	    break;
	    
	  case CLK_MODE_SLOW:
	    ctrl =  BF_SSP_TIMING_TIMEOUT(0xFFFF) |
	      BF_SSP_TIMING_CLOCK_DIVIDE(128) |
	      BF_SSP_TIMING_CLOCK_RATE(0);
	    
	    HW_SSP_TIMING_WR(ctrl);
	    break;
	    
	  case CLK_MODE_OFF:
	    
	    /* Clock is disabled automatically whenever there's no data on the
	     * CMD or DAT line, therefore this function is not implemented. */

	    break;
	    
	  default: TRSP_ASSERT(0);
	  }
	KDEBUG(TRANSPORT, "exit\n");
	
        return;
}


int host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{
	int ret = 0;

	KDEBUG(TRANSPORT, "irq_op: %d, irq_id: %d\n", irq_op, irq_id);

        switch (irq_id) {
	case IRQ_ID_TARGET:
	{
		switch (irq_op) {
		case IRQ_OP_ENABLE:
                        BW_SSP_CTRL1_SDIO_IRQ_EN(1);
			break;

		case  IRQ_OP_DISABLE:
                        BW_SSP_CTRL1_SDIO_IRQ_EN(0);
			break;

		case IRQ_OP_STATUS:
			ret = HW_SSP_CTRL1.B.SDIO_IRQ;
			break;

		case IRQ_OP_ACKNOWLEDGE:
                        BW_SSP_CTRL1_SDIO_IRQ(0);
                        break;

		default:
			break;
		}
	}
	break;

	case IRQ_ID_RESPONSE_COMPLETION: 
	case IRQ_ID_FIFO_READ_READY:
	case IRQ_ID_FIFO_WRITE_READY:
	case IRQ_ID_RW_ACCESS_COMPLETION:
	default:
		break;
	}

	KDEBUG(TRANSPORT, "exit\n");

	return ret;
}


int host_fifo(void *data, unsigned long buffer_phys, uint16_t len, uint32_t flags)
{
	uint16_t i;
	uint32_t* ptr = (uint32_t*) data;
        uint32_t prev_dma_end;
	uint8_t error = 0;
	int ret;

        KDEBUG(TRANSPORT, "data: 0x%08X, len:%d, flags: 0x%08x\n", (uint32_t)data, len, flags);

	if (!sdio_dma) {        
		prev_dma_end = HW_SSP_STATUS.B.DMAEND;
		
		if(flags & FIFO_FLAG_TO_HOST) {
			
			for(i = 0; i < len / 4; i++) {
				HOST_POLL(HW_SSP_STATUS.B.DMAREQ == prev_dma_req,
					  US_TIME_1SEC);

				if (HW_SSP_STATUS.B.DMAREQ == prev_dma_req) {
					error = 1;
					break;
				}
		
				// If there's an error, what should we do here?
				prev_dma_req = HW_SSP_STATUS.B.DMAREQ;
				ptr[i] = HW_SSP_DATA_RD();

			}
		}
		else {
			dma_desc[0].cmd.U |= BV_FLD(APBH_CHn, CMD_COMMAND, DMA_READ);

			for(i = 0; i < len / 4; i++) {
				prev_dma_req = HW_SSP_STATUS.B.DMAREQ;
				HW_SSP_DATA_WR(ptr[i]);
				HOST_POLL(HW_SSP_STATUS.B.DMAREQ == prev_dma_req &&
					  HW_SSP_STATUS.B.DMAEND == prev_dma_end,
					  US_TIME_1SEC);

				if (HW_SSP_STATUS.B.DMAREQ == prev_dma_req &&
				    HW_SSP_STATUS.B.DMAEND == prev_dma_end) {
					error = 2;
					break;
				}
			}
		}
	}
	else {
		memcpy(dma_buf, data, len);

		dma_desc[0].buffer = (dma_addr_t) dma_buf_pa;
		dma_desc[0].next = 0;
		dma_desc[0].cmd.U =
			BF_APBH_CHn_CMD_XFER_COUNT(len)         |
			BF_APBH_CHn_CMD_CMDWORDS(0)             |
			BF_APBH_CHn_CMD_SEMAPHORE(1)            |
			BF_APBH_CHn_CMD_WAIT4ENDCMD(1)          |
			BF_APBH_CHn_CMD_CHAIN(0)                |
			BF_APBH_CHn_CMD_IRQONCMPLT(1);

		if(flags & FIFO_FLAG_TO_HOST)
			dma_desc[0].cmd.U |= BV_FLD(APBH_CHn, CMD_COMMAND, DMA_WRITE);
		else
			dma_desc[0].cmd.U |= BV_FLD(APBH_CHn, CMD_COMMAND, DMA_READ);

		mb(); HW_APBH_CHn_NXTCMDAR(1).B.CMD_ADDR = (dma_addr_t)&dma_desc_pa[0];
	
		mb(); BF_WRn(APBH_CHn_SEMA, 1, INCREMENT_SEMA, 1);

		mb();

		if (sdio_dma_irq) {
                   ret = wait_event_timeout(wq, !HW_APBH_CHn_SEMA(1).B.PHORE , HZ);
                   if (ret == 0)
                      {
                         if(flags & FIFO_FLAG_TO_HOST)
                            panic("timeout fifo read !\n");
                         else
                            panic("timeout fifo write!\n");
                      }
		}
		else {
			HOST_POLL(BF_RDn(APBH_CHn_SEMA, 1, PHORE), US_TIME_1SEC);
		}


		memcpy(data, dma_buf, len);
	}

	// If something went wrong, we recover here
	if (error) {
		TRSP_MSG("Error dealing with packet!\n");
		return -1;
	}

        sdio_fifo_callback();

	KDEBUG(TRANSPORT, "exit\n");

        return 0;
}

void host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
              uint32_t flags, uint32_t arg, void* data)

     //void host_cmd(cmd_t cmd, cmd_resp_t cmd_resp, uint32_t flags, uint32_t arg)
{
	int ret;
	uint32_t ctrl0;

        KDEBUG(TRANSPORT, "cmd: %d, cmd_resp: %d, flags: %u arg: %d\n",  cmd, cmd_resp, flags, arg);
	KDEBUG(TRANSPORT, "data_exists: %d, read: %d\n", flags & CMD_FLAG_DATA_EXISTS, flags & CMD_FLAG_DIR_DATA_TO_HOST);

        ctrl0 = HW_SSP_CTRL0_RD();
	
        if (flags & CMD_FLAG_DATA_EXISTS)
		ctrl0 |= BF_SSP_CTRL0_DATA_XFER(1);
        else
		ctrl0 &= ~BF_SSP_CTRL0_DATA_XFER(1);
        
        if (flags & CMD_FLAG_DIR_DATA_TO_HOST)
		ctrl0 |= BF_SSP_CTRL0_READ(1);
        else
		ctrl0 &= ~BF_SSP_CTRL0_READ(1);
         
	// esto es un envio de comando
        ctrl0 |= BF_SSP_CTRL0_ENABLE(1);

        //	if (!sdio_dma) {
        if (1) {
		if (cmd == SDIO_RW_EXTENDED)
			prev_dma_req = HW_SSP_STATUS.B.DMAREQ;

		ctrl0 |= BF_SSP_CTRL0_RUN(1);

		// escribimos el comando y el argumento
		HW_SSP_CMD0_WR(cmd); mb();
		HW_SSP_CMD1_WR(arg); mb();

		// escribimos el registro de control
		HW_SSP_CTRL0_WR(ctrl0); mb();
	}
	else {
		dma_desc[0].cmd.U =
			BF_APBH_CHn_CMD_XFER_COUNT(0)           |
			BF_APBH_CHn_CMD_CMDWORDS(3)             |
			BF_APBH_CHn_CMD_SEMAPHORE(1)            |
			BF_APBH_CHn_CMD_WAIT4ENDCMD(0)          |
			BF_APBH_CHn_CMD_CHAIN(0)                |
			BF_APBH_CHn_CMD_IRQONCMPLT(1)           |
			BV_FLD(APBH_CHn, CMD_COMMAND, NO_DMA_XFER);

		dma_desc[0].pio[0] = ctrl0;
		dma_desc[0].pio[1] = cmd;
		dma_desc[0].pio[2] = arg;

		mb(); HW_APBH_CHn_NXTCMDAR(1).B.CMD_ADDR = (dma_addr_t)&dma_desc_pa[0];
	
		mb(); BF_WRn(APBH_CHn_SEMA, 1, INCREMENT_SEMA, 1);
		
		mb();

		if (sdio_dma_irq) {
                   ret = wait_event_timeout(wq, !HW_APBH_CHn_SEMA(1).B.PHORE , HZ);
			if (ret == 0)
                        panic("timeout cmd!\n");
		}
		else {
			HOST_POLL(BF_RDn(APBH_CHn_SEMA, 1, PHORE), US_TIME_1SEC);
                        }
	}

	KDEBUG(TRANSPORT, "exit\n");
	return IRQ_STATUS_SUCCESS;
}


uint32_t host_control(ctl_id_t ctl_id, uint32_t param)
{
	uint32_t ret = 0;

	KDEBUG(TRANSPORT, "ctl_id: %d, param: %u\n", ctl_id, param);

	switch(ctl_id) {
        case CTL_ID_LENGTH:
                BW_SSP_CTRL0_XFER_COUNT(param); // !!
                break;
                
	case CTL_ID_WAIT_CMD_READY:
                HOST_POLL(HW_SSP_STATUS.B.CMD_BUSY, US_TIME_1SEC);
                break;
                
        case CTL_ID_RESPONSE:
                HOST_POLL(HW_SSP_STATUS.B.CMD_BUSY, US_TIME_1SEC);
                ret  = HW_SSP_SDRESP0_RD();
                TRSP_MSG("Response: 0x%08x\n", ret);

	default:
		break;
	}

	KDEBUG(TRANSPORT, "exit\n");

	return ret;
}

void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{

   KDEBUG(TRANSPORT, "ENTER: gpio_op=%d gpio_id=%d\n", gpio_op, gpio_id);
   switch(gpio_op) {
        case GPIO_OP_ENABLE:
           switch(gpio_id)
              {
              case GPIO_ID_POWER:
                 HW_PINCTRL_MUXSEL0_SET(PSELMASK(GPMID11));
                 HW_PINCTRL_DOE0_SET(PBANKMASK(GPMID11));
                 break;
              case GPIO_ID_RESET_A:
                 HW_PINCTRL_MUXSEL0_SET(PSELMASK(GPMID13));
                 HW_PINCTRL_DOE0_SET(PBANKMASK(GPMID13));
                 break;
              case GPIO_ID_RESET_D:
                 HW_PINCTRL_MUXSEL0_SET(PSELMASK(GPMID12));
                 HW_PINCTRL_DOE0_SET(PBANKMASK(GPMID12));
                 break;
              default: TRSP_ASSERT(0);
              }
           break;
           
        case GPIO_OP_HIGH:
           
           switch(gpio_id)
              {
              case GPIO_ID_POWER:
                 HW_PINCTRL_DOUT0_SET(PBANKMASK(GPMID11));
                 break;
              case GPIO_ID_RESET_A:
                 HW_PINCTRL_DOUT0_SET(PBANKMASK(GPMID13));
                 break;   
              case GPIO_ID_RESET_D:
                 HW_PINCTRL_DOUT0_SET(PBANKMASK(GPMID12));
                 break;
              default: TRSP_ASSERT(0);
              }
           break;
           
           
        case GPIO_OP_LOW:
           switch(gpio_id)
              {
              case GPIO_ID_POWER:
                 HW_PINCTRL_DOUT0_CLR(PBANKMASK(GPMID11));
                 break;
              case GPIO_ID_RESET_A:
                 HW_PINCTRL_DOUT0_CLR(PBANKMASK(GPMID13));
                 break;   
              case GPIO_ID_RESET_D:
                 HW_PINCTRL_DOUT0_CLR(PBANKMASK(GPMID12));
                 break;
              default: TRSP_ASSERT(0);
              }
           break;

        case GPIO_OP_DISABLE:
           break;

        default: TRSP_ASSERT(0);
        }
}

void host_exit(void)
{
	KDEBUG(TRANSPORT, "enter\n");
	
	// Libera el DMA
	dma_free_coherent(0, 0, dma_desc, 0);

	// Libera la interrupcion
	free_irq(IRQ_SSP_DMA, NULL);

	KDEBUG(TRANSPORT, "exit\n");

        return;
}
