/**
*   $Id: host.c 9119 2008-06-09 11:50:52Z joda $
*
*/


#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "host.h"

#include <asm/arch/platform.h>
#include <asm/arch/mm6_reg.h>
#include <asm/arch/mm6p_gpio.h>
#include <asm/arch/mm6p_clock.h>
#include <asm/arch/irqs.h>
#include <asm/arch/dma.h>
#include "mm6_defs.h"

#include "nanoutil.h"

PL9200_SDIO_MODULE pSDIO;
//GPR regs
#if !defined(SDIO_SLOT_A) && !defined(SDIO_BUILTIN)
static PMM6_SYSTEM_MODULE  mm6_sys = (PMM6_SYSTEM_MODULE) (VIRT_STCL_BASE);
#endif

static irq_status_t mm6_sdio_chk_resp_completion(uint32_t SDIS);
static irq_status_t mm6_sdio_chk_rw_completion(uint32_t SDIS);
static void mm6_sdio_wait_idle(void);

static void mm6_sdio_read_from_fifo(unsigned int *buf, unsigned int total);
static void mm6_sdio_write_to_fifo(unsigned int *buf, unsigned int total);
static void mm6_sdio_dma_write(unsigned long data, unsigned int total);
static void mm6_sdio_dma_read(unsigned long data, unsigned int total);

#ifdef USE_DEBUG_PINS
static void mm6_sdio_debug_pins(gpio_op_t gpio_op, uint32_t mask);
#endif

static unsigned int gpio_dev_id;
static unsigned int is_4bit_sdio;


struct mm6_sdio_t {
        uint32_t dma_tx_channel;
        uint32_t dma_rx_channel;
} *mm6_sdio;

void mm6_dma_rx_callback(int error)
{
        KDEBUG(TRANSPORT, "ENTRY");
        sdio_fifo_callback();
        KDEBUG(TRANSPORT, "EXIT");
}

void mm6_dma_tx_callback(int error)
{
        KDEBUG(TRANSPORT, "ENTRY");
        sdio_fifo_callback();
        KDEBUG(TRANSPORT, "EXIT");
}

int host_init(transfer_mode_t transfer_mode)
{
        volatile unsigned long *pInt;
        int status;
        int magic = 36;
        
        KDEBUG(TRANSPORT, "ENTRY");       

        printk("Driver's Magic Number : %d\n", magic);
		
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
		
		pSDIO = (PL9200_SDIO_MODULE) (SLOT_ADDR);
        mm6_sdio = kmalloc(sizeof(struct mm6_sdio_t), GFP_KERNEL);
        if (mm6_sdio == NULL) return -1;

        pInt = (volatile unsigned long *)(VIRT_INTC_BASE + 0x14);
        *pInt = (1 << IRQ_NO);
    
        mm6_sdio->dma_rx_channel =
                mm6_dma_request(DMA_SDC_RX, (dma_callback_t)mm6_dma_rx_callback,
                                receive);

        status = mm6_dma_setup(DMA_SDC_RX, mm6_sdio->dma_rx_channel, 0);
        if(status != 0) return -1;
        
        mm6_sdio->dma_tx_channel =
                mm6_dma_request(DMA_SDC_TX, (dma_callback_t)mm6_dma_tx_callback,
                                transmit);
        
        status = mm6_dma_setup(DMA_SDC_TX, mm6_sdio->dma_tx_channel, 0);
        if (status != 0 ) return -1;
	
#ifdef SDIO_BUILTIN
        *SCTRL_GPR0 |= (0xef);
#endif

		KDEBUG(TRANSPORT, "EXIT");
        return 0;
}

int host_reset(void)
{
        KDEBUG(TRANSPORT, "ENTRY");

        /* enable SDIO controller */
        pSDIO->REG_SDCT = SDCT_ENABLE | SDCT_TCT;
        pSDIO->REG_SDCT = 0x00;
        pSDIO->REG_SDCT = SDCT_ENABLE;
    
        /* clear all interrupts and status bits */
        pSDIO->REG_SDIS = 0xffffffff;
        pSDIO->REG_SDIC = 0xf;
    
        /* Open drain pullup disabled */
        pSDIO->REG_SDCT &= ~SDCT_ROD;

        /* maximum data access time delay for SD cards */
        pSDIO->REG_SDTM = 0x01FFFFFF;
    
        /* Enable "command response received" and "transfer complete" interrupts */
        pSDIO->REG_SDIM = SDIM_CRRM | SDIM_DTCM;
        
        /* Enable SDIO interrupt (interrupt from the device) */
        pSDIO->REG_SDCT |= SDCT_SDIOINT;
		
		/* Stop CLK when SDIO bus is inactive */
		pSDIO->REG_SDCT &= ~SDCT_CSD;

		if (is_4bit_sdio)
			pSDIO->REG_SDCT |= SDCT_DPW;
    
        KDEBUG(TRANSPORT, "EXIT");
		return 0;
}

void host_clock(clk_mode_t clk_mode)
{
		int status;
		int freq;
        KDEBUG(TRANSPORT, "ENTRY");

		freq = 0;
		switch(clk_mode) {
        
		case CLK_MODE_FAST:
			freq = 24000; /* 24 MHz */
			break;
			
        case CLK_MODE_SLOW:
			freq = 300; /* 300 kHz */
			break;

        case CLK_MODE_OFF:
			/* nothing to do, the host controller stops SDIO CLK */
			break;

        default:
			TRSP_ASSERT(0);
        }

		if (freq > 0) {
			status = mm6plus_set_clock(CLK_SRC, freq);
			TRSP_ASSERT(status == 0);
		}
        KDEBUG(TRANSPORT, "EXIT");
}


/* Called by host_interrupt to process status bits of SDIS register
   when checking for IRQ_ID_RESPONSE_COMPLETION interrupt.
   Asserts hw consistent behaviour and returns apropriate irq_status_t value. */
static irq_status_t mm6_sdio_chk_resp_completion(uint32_t SDIS)
{
	irq_status_t irq_status;
	int is_bus_error;

	/* More severe error are checked with higher priority. */
	if (SDIS & SDIS_RTO)
		irq_status = IRQ_STATUS_TIMEOUT;
	else if (SDIS & SDIS_RCE)
		irq_status = IRQ_STATUS_CRC;
	else if (SDIS & SDIS_CRR)
		irq_status = IRQ_STATUS_ACTIVE;
	else
		irq_status = IRQ_STATUS_INACTIVE;
		
	/* Assert that ETO bit is set if and only if an error occured. */
	if (irq_status > IRQ_STATUS_ACTIVE) {
		TRSP_ASSERT((SDIS & SDIS_ETO) != 0);
		is_bus_error = 1;
	}
	else if (SDIS & SDIS_ETO) {
		TRSP_ASSERT(0);
		is_bus_error = 1;
	}
	else
		is_bus_error = 0;

	/* In case of bus error, print SDIS bitfield and pulse GPIO pin. */
	if (is_bus_error) {
		host_debug_pins(GPIO_OP_HIGH, 0x1);
		udelay(5);
		host_debug_pins(GPIO_OP_LOW, 0x1);
		TRSP_MSG("resp, SDIS = 0x%08x\n", SDIS);
	}

	return irq_status;
}


/* Called by host_interrupt to process status bits of SDIS register
   when checking for IRQ_ID_RW_ACCESS_COMPLETION interrupt.
   Asserts hw consistent behaviour and returns apropriate irq_status_t value.
*/
static irq_status_t mm6_sdio_chk_rw_completion(uint32_t SDIS)
{
	irq_status_t irq_status;
	int is_bus_error;

	/* More severe error are checked with higher priority. */
	if (SDIS & SDIS_DTO)
		irq_status = IRQ_STATUS_TIMEOUT;
	else if (SDIS & (SDIS_RDCE | SDIS_WDCE))
		irq_status = IRQ_STATUS_CRC;
	else if (SDIS & (SDIS_RDCE | SDIS_WDCE))
		irq_status = IRQ_STATUS_FIFOERR;
	else if (SDIS & SDIS_DTC)
		irq_status = IRQ_STATUS_ACTIVE;
	else
		irq_status = IRQ_STATUS_INACTIVE;
		
	/* Assert that ETO bit is set if and only if an error occured. */
	if (irq_status > IRQ_STATUS_ACTIVE) {
		TRSP_ASSERT((SDIS & SDIS_ETO) != 0);
		is_bus_error = 1;
	}
	else if (SDIS & SDIS_ETO) {
		TRSP_ASSERT(0);
		is_bus_error = 1;
	}
	else
		is_bus_error = 0;

	/* In case of bus error, print SDIS bitfield. */
	if (is_bus_error)
		TRSP_MSG("data, SDIS = 0x%08x\n", SDIS);

	return irq_status;
}


irq_status_t host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{

    switch(irq_id) {

	case IRQ_ID_TARGET:
                if(irq_op == IRQ_OP_ENABLE)
                        pSDIO->REG_SDIM |= SDIM_SDINTM;
                
                else if(irq_op == IRQ_OP_DISABLE)
                        pSDIO->REG_SDIM &= ~SDIM_SDINTM;
                
                else if(irq_op == IRQ_OP_STATUS) {
                        if ((pSDIO->REG_SDIM & SDIM_SDINTM) && (pSDIO->REG_SDIS & SDIS_SDINT))
                                return IRQ_STATUS_ACTIVE;
                        else
                                return IRQ_STATUS_INACTIVE;
                }

                else if(irq_op == IRQ_OP_ACKNOWLEDGE)
                	; /* No acknowledgement */

                else {
					TRSP_ASSERT(0);
					return IRQ_STATUS_FAIL;
				}
		break;

	case IRQ_ID_RESPONSE_COMPLETION: 
                if(irq_op == IRQ_OP_ENABLE)
                        pSDIO->REG_SDIM |= (SDIM_CRRM | SDIM_ETOM);
                
                else if(irq_op == IRQ_OP_DISABLE)
                        pSDIO->REG_SDIM &= ~(SDIM_CRRM | SDIM_ETOM);
                
                else if(irq_op == IRQ_OP_STATUS)
					return mm6_sdio_chk_resp_completion(pSDIO->REG_SDIS);

                else if(irq_op == IRQ_OP_ACKNOWLEDGE)
                        pSDIO->REG_SDIC = (SDIC_CRRFC | SDIC_ETOFC);

                else {
					TRSP_ASSERT(0);
					return IRQ_STATUS_FAIL;
				}
		break;


	case IRQ_ID_RW_ACCESS_COMPLETION:
                if(irq_op == IRQ_OP_ENABLE)
                        pSDIO->REG_SDIM |= (SDIM_DTCM | SDIM_ETOM);
                
                else if(irq_op == IRQ_OP_DISABLE)
                        pSDIO->REG_SDIM &= ~(SDIM_DTCM | SDIM_ETOM);
                
                else if(irq_op == IRQ_OP_STATUS)
                        return mm6_sdio_chk_rw_completion(pSDIO->REG_SDIS);

                else if(irq_op == IRQ_OP_ACKNOWLEDGE)
                        pSDIO->REG_SDIC = (SDIC_DTCFC | SDIC_ETOFC);
                
                else {
					TRSP_ASSERT(0);
					return IRQ_STATUS_FAIL;
				}
		   break;

/*  Not supported:
	case IRQ_ID_FIFO_READ_READY: 
	case IRQ_ID_FIFO_WRITE_READY:
*/
	default:
		TRSP_ASSERT(0);
		return IRQ_STATUS_FAIL;
	}
        
    return IRQ_STATUS_SUCCESS;
}


int host_fifo(void *data, unsigned long data_phys, uint16_t len, uint32_t flags)
{
        unsigned int *ptr = (unsigned int*)data;
        uint8_t no_dma = 0;
        
        KDEBUG(TRANSPORT, "ENTRY");

        if((len % 16) || (((uint32_t) data) % 4)) no_dma = 1;

        if (flags & FIFO_FLAG_TO_HOST) {
                if (no_dma) {
                        mm6_sdio_read_from_fifo(ptr, len);
                        mm6_dma_rx_callback(0);        
                }else{
                        while(!(pSDIO->REG_SDIS & SDIS_RFHF)); /* XXX */
                        mm6_sdio_dma_read(data_phys, len);
                }
        }else{
                if (no_dma) {
                        mm6_sdio_write_to_fifo(ptr, len);
                        mm6_dma_tx_callback(0);
                }else{
                        mm6_sdio_dma_write(data_phys, len);
                }
        }
        KDEBUG(TRANSPORT, "EXIT");
        return 0;
}

irq_status_t host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
					  uint32_t flags, uint32_t arg, void* data)
{
        KDEBUG(TRANSPORT, "ENTRY cmd: 0x%02x, arg: 0x%08x", cmd, arg);
        pSDIO->REG_SDCM0 = arg;
        pSDIO->REG_SDCM1 = cmd;
        pSDIO->REG_SDCT |= SDCT_TC;
		
        KDEBUG(TRANSPORT, "EXIT");
		return IRQ_STATUS_SUCCESS;
}


/* Called by host_interrupt to wait until response
   for the current SDIO command has been received.
*/
static void mm6_sdio_wait_idle(void)
{
	unsigned int attempts;
	unsigned int is_busy;

	do {
		for (attempts = BUSYWAIT_LOOP_MAX; attempts > 0; attempts--) {
			is_busy = pSDIO->REG_SDCT & SDCT_TC;
			if (!is_busy)
				break;
		}
		
		/* In case of timeout, we can not (and will not) give up waiting */
		TRSP_ASSERT(!is_busy);
	} while (is_busy);
}

uint32_t host_control(ctl_id_t ctl_id, uint32_t param)
{
		uint32_t ret_val;

        KDEBUG(TRANSPORT, "ENTRY");

		ret_val = 0;
        switch(ctl_id) {

			case CTL_ID_LENGTH:
				/* Nothing to be done */
				break;

			case CTL_ID_WAIT_CMD_READY:
				mm6_sdio_wait_idle();
				break;

			case CTL_ID_RESPONSE:
				mm6_sdio_wait_idle();
				ret_val = (uint32_t) ((pSDIO->REG_SDRS0 & 0xff000000) >> 8);
				break;

			default:
				TRSP_ASSERT(0); 
        }
        KDEBUG(TRANSPORT, "EXIT");
        return ret_val;
}

void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
   
    int port;
    int mask;
    int err;
    
    switch(gpio_id) {
    case GPIO_ID_RESET_D:
#ifdef  SDIO_SLOT_A
	port = 0x0;
	mask = BITMASK_0;
#elif defined (SDIO_BUILTIN)
	port = 0xd;
	mask = BITMASK_0;
#else
	port = 0xb;
	mask = BITMASK_6;
#endif
	break;
	
    case GPIO_ID_RESET_A:
#ifdef SDIO_SLOT_A
	port = 0x0;
	mask = BITMASK_7;
#elif defined (SDIO_BUILTIN)
	port = 0xd;
	mask = BITMASK_3;
#else
	return;
#endif
	break;
#ifndef SDIO_BUILTIN
    case GPIO_ID_CPU_LOAD:
    case GPIO_ID_POWER:
       return;
#else
    case GPIO_ID_CPU_LOAD:
       return;
    case GPIO_ID_POWER:
       port = 0xd;
       mask = BITMASK_1;
       break;
#endif
	
    default: 
       KDEBUG(TRANSPORT, "gpio_id = %u", gpio_id);
       TRSP_ASSERT(0); 
       return;
    }
    
    switch(gpio_op) {
    case GPIO_OP_DISABLE:
	//unregister gpio pin
	if((err = unregister_gpio(port, mask, &gpio_dev_id)) !=0){
	    printk(KERN_ERR"Error unregister_gpio\n");
	}
	break;

    case GPIO_OP_ENABLE:
	//enable GPIO-port 0
#ifdef SDIO_SLOT_A
	*SCTRL_SCPER_CTRL &= ~(BITMASK_29);
#elif defined (SDIO_BUILTIN)
	*SCTRL_SCPER_CTRL &= ~(BITMASK_22);
	*SCTRL_SCPER_CTRL |= (BITMASK_21);	
#else
	//enable GPIO pin 6 on port b (RESET)
	*SCTRL_SCPER_CTRL |= (BITMASK_26);	
	*SCTRL_SCPER_CTRL &= ~(BITMASK_4);
	//GPR6 6
	mm6_sys->REG_Sys_GPR6 &= ~(BITMASK_6);
#endif

	//configure gpio-pin
	if((err = set_gpioport_direction(port,mask,GPIO_Direction_Output)) !=0){
	    printk(KERN_ERR "Error on set_gpioport_direction\n");
	}
	//register read/write
	if((err = register_gpio_readwrite(port,mask,&gpio_dev_id)) !=0){
	    printk(KERN_ERR "Error on register_gpio_readwrite\n");
	}
	break;

    case GPIO_OP_HIGH:
	if((err = write_to_gpio_port(port,mask,mask)) !=0){
	    printk(KERN_ERR "Error on write_to_gpio_port: %d\n", err);
	}
	break;

    case GPIO_OP_LOW:
	if((err = write_to_gpio_port(port,mask,0)) !=0){
	    printk(KERN_ERR "Error on write_to_gpio_port: %d\n", err);
	}
	break;

    default: TRSP_ASSERT(0);

    }
    return;
}

void host_exit(void)
{
        volatile unsigned long *pInt;

        KDEBUG(TRANSPORT, "ENTRY");        

        pInt = (volatile unsigned long *) (VIRT_INTC_BASE + 0x14);
        *pInt = (1 << IRQ_NO);
    
        mm6_dma_free(DMA_SDC_RX, mm6_sdio->dma_rx_channel);
        mm6_dma_free(DMA_SDC_TX, mm6_sdio->dma_tx_channel);
        kfree(mm6_sdio);

        KDEBUG(TRANSPORT, "EXIT");
        return;
}


static void mm6_sdio_read_from_fifo(unsigned int *buf, unsigned int total)
{    
    unsigned int *ptr = buf;
    KDEBUG(TRANSPORT, "ENTRY");
    total = (total + sizeof(unsigned int) - 1) / sizeof(unsigned int);
    while(total)
    {
        if (!(pSDIO->REG_SDIS & SDIS_RFE))
        {
            *ptr = pSDIO->REG_SDDT;
            total--;
            ptr++;
        }
    }
    KDEBUG(TRANSPORT, "EXIT");
}

static void mm6_sdio_write_to_fifo(unsigned int *buf, unsigned int total)
{
   
    unsigned int *ptr = buf;
  
    KDEBUG(TRANSPORT, "ENTRY");

    //4 byte aligned size
    total = (total + sizeof(unsigned int) - 1) / sizeof(unsigned int);
  
    while (total) 
    {
        if (!(pSDIO->REG_SDIS & SDIS_TFF)) 
        {
            pSDIO->REG_SDDT =  *ptr;
            total--;
            ptr++;
        }
    }
    KDEBUG(TRANSPORT, "EXIT");
}

static void mm6_sdio_dma_write(unsigned long data, unsigned int total)
{
    int result = 0;
    KDEBUG(TRANSPORT, "ENTRY");
    pSDIO->REG_SDIM &= ~SDIM_RDM;
    pSDIO->REG_SDIM |= SDIM_TDM;

    result = mm6_dma_start(DMA_SDC_TX,			/* device id */
			   (PHYS_SDIO_BASE + 0x034),	/* destination addr */
			   data,		/* source addr */
			   total,			/* number of bytes */
			   mm6_sdio->dma_tx_channel);	/* channel to use */
	
    if (result < 0) { /* error in transfer */
	printk(KERN_ERR "Error in DMA WRITE start\n");
	return;
    }
    KDEBUG(TRANSPORT, "EXIT");
}

static void mm6_sdio_dma_read(unsigned long data, unsigned int total)
{	
    int result = 0;
    KDEBUG(TRANSPORT, "ENTRY");
    pSDIO->REG_SDIM &= ~SDIM_TDM;
    pSDIO->REG_SDIM |= SDIM_RDM;		
	
    result = mm6_dma_start(DMA_SDC_RX,			/* device id */
			   data,		/* destination addr */
			   (PHYS_SDIO_BASE + 0x034),	/* source addr */
			   total,			/* number of bytes */
			   mm6_sdio->dma_rx_channel);	/* channel to use */
	
    if (result < 0) { /* error in transfer */
	printk(KERN_ERR "Error in DMA READ start\n");
	return;
    }
    KDEBUG(TRANSPORT, "EXIT");
}

#ifdef USE_DEBUG_PINS
void host_debug_pins(gpio_op_t gpio_op, uint32_t mask)
{
	uint32_t single_bit_mask;

	/* Iteration is needed because mm6 gpio functions
	   may fail if more than one bits in mask are set! */
	for (single_bit_mask = 0x1; single_bit_mask <= 0x2; single_bit_mask <<= 1)
		if (mask & single_bit_mask)
			mm6_sdio_debug_pins(gpio_op, single_bit_mask);
}

void mm6_sdio_debug_pins(gpio_op_t gpio_op, uint32_t mask)
{
    int port = 0x1;
	int status;
	
	mask = (mask & 0x3) << 2;
	switch(gpio_op) {

    case GPIO_OP_DISABLE:
		unregister_gpio(port, mask, &gpio_dev_id);
		*SCTRL_SCPER_CTRL &= ~BITMASK_30;
		break;

    case GPIO_OP_ENABLE:
		*SCTRL_SCPER_CTRL |= BITMASK_30;
		status = set_gpioport_direction(port, mask, GPIO_Direction_Output);
		if (status) {
			TRSP_MSG("set_gpioport_direction returned %d\n", status);
			return;
		}
		status = register_gpio_readwrite(port, mask, &gpio_dev_id);
		if (status)
			TRSP_MSG("register_gpio_readwrite returned %d\n", status);
		break;
	
    case GPIO_OP_HIGH:
	case GPIO_OP_LOW:
		status = write_to_gpio_port(port, mask, (gpio_op == GPIO_OP_HIGH) ? mask : 0);
		if (status)
			TRSP_MSG("write_to_gpio_port returned %d\n", status);
		break;

	default:
		TRSP_ASSERT(0);
	}
}
#endif /* USE_DEBUG_PINS */
