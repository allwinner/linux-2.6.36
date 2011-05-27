#include <host.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>

#include "nanoutil.h"

#ifdef USE_DMA
#include <asm/arch/dma.h>
#endif 

#include "omap_def.h"


#define CLK_DIVIDER 0x3

static void
omap_dma_rx_callback(void *data, int size)
{
    KDEBUG(TRANSPORT, "ENTRY\n");

    TRSP_ASSERT(*MMC_STAT & STAT_BRS);
    *MMC_STAT = STAT_BRS;

    sdio_fifo_callback();

    KDEBUG(TRANSPORT, "EXIT\n");
}

static void
omap_dma_tx_callback(void *data, int size)
{
    KDEBUG(TRANSPORT, "ENTRY\n");
 
    sdio_fifo_callback();
    
    while(!(*MMC_STAT & STAT_BRS));
    *MMC_STAT = STAT_BRS;
}


static void omap_sdio_fifo_read(void *data, uint16_t len)
{
	uint16_t i;
	u16* ptr = (u16*) data;

        for(i = 0; i < len / 2; i++) {
                while(!(*MMC_STAT & STAT_AF));
                *MMC_STAT = STAT_AF;
                ptr[i] = *MMC_DATA;   
        }
        omap_dma_rx_callback(0, 0);
}


static void omap_sdio_fifo_write(void *data, uint16_t len)
{
	uint16_t i;
	u16* ptr = (u16*) data;
        for(i = 0; i < len / 2; i++) {
                while(!(*MMC_STAT & STAT_AE));
                *MMC_STAT = STAT_AE;
                *MMC_DATA = *ptr++;
        }
        omap_dma_tx_callback(0, 0);
}

#ifdef USE_DMA
dma_regs_t *tx_dma_regs;
dma_regs_t *rx_dma_regs;

static void omap_sdio_dma_write(dma_addr_t dma_addr, int size)

{
        int status;
        KDEBUG(TRANSPORT, "ENTRY\n size=%d",size);

        tx_dma_regs->csdp = ((1 << 10) | (1 << 9) | (2 << 7) | (1 << 6) | 1);
        tx_dma_regs->ccr =  (eMMCTx | (1 << 12) | (1 << 10) | (1 << 5) |
                             (1 << 6));
        tx_dma_regs->cicr = (1 << 5) | (1 << 1) | 1;
        status = tx_dma_regs->csr;     
        tx_dma_regs->cdsa_l = ((u32)MMC_DATA) & 0xffff;
        tx_dma_regs->cdsa_u = ((u32)MMC_DATA) >> 16;
        tx_dma_regs->cssa_l = dma_addr & 0xffff;
        tx_dma_regs->cssa_u = dma_addr >> 16;

        tx_dma_regs->cen = 8; //elements within a frame (AEL) (16 bytes)
        tx_dma_regs->cfn = size >> 4;;//number of frames
        tx_dma_regs->ccr2 = 0;
        tx_dma_regs->lch_ctrl = 2;
    
        /* Start DMA */
        tx_dma_regs->ccr |= (1 << 7);

        KDEBUG(TRANSPORT, "EXIT\n");
	
}

static void omap_sdio_dma_read(dma_addr_t dma_addr, int size)
{
        int status;
        KDEBUG(TRANSPORT, "ENTRY\n");

        /* Configure DMA */
        rx_dma_regs->csdp = (1 << 3) | (1 << 2) | 1;
        rx_dma_regs->ccr = (eMMCRx | (1 << 14) | (1 << 10) | (1 << 5) |
                            (1 << 6));
        rx_dma_regs->cicr = (1 << 5) | (1 << 1) | 1;
        status = rx_dma_regs->csr;	// read status to clear it.
        rx_dma_regs->cssa_l = ((u32)MMC_DATA) & 0xffff;
        rx_dma_regs->cssa_u = ((u32)MMC_DATA) >> 16;
        rx_dma_regs->cdsa_l = dma_addr & 0xffff;
        rx_dma_regs->cdsa_u = dma_addr >> 16;

        rx_dma_regs->cen = 8;//elements within a frame (AFL) (16 bytes)
        rx_dma_regs->cfn = size >> 4;//number of frames
        rx_dma_regs->ccr2 = 0;
        rx_dma_regs->lch_ctrl = 2;

        /* Start DMA */
        rx_dma_regs->ccr |= (1 << 7);

        KDEBUG(TRANSPORT, "EXIT\n");
}

#endif

void dump_registers(void)
{
        printk("MMC_STAT=0x%04x\n", *MMC_STAT);
        printk("MMC_IE=0x%04x\n", *MMC_IE);
        printk("MMC_SDIO=0x%04x\n", *MMC_SDIO);
        printk("MMC_SYSS=0x%04x\n", *MMC_SYSS);
        printk("MMC_CON=0x%04x\n", *MMC_CON);
        printk("MMC_SYSC=0x%04x\n", *MMC_SYSC);
        printk("MMC_CTO=0x%04x\n", *MMC_CTO);
        printk("MMC_DTO=0x%04x\n", *MMC_DTO);
        printk("MMC_ARGL=0x%04x\n", *MMC_ARGL);
        printk("MMC_ARGH=0x%04x\n", *MMC_ARGH);
        printk("MMC_CMD=0x%04x\n", *MMC_CMD);
        printk("MMC_RSP0=0x%04x\n", *MMC_RSP0);
        printk("MMC_RSP1=0x%04x\n", *MMC_RSP1);
        printk("MMC_RSP2=0x%04x\n", *MMC_RSP2);
        printk("MMC_RSP3=0x%04x\n", *MMC_RSP3);
        printk("MMC_RSP4=0x%04x\n", *MMC_RSP4);
        printk("MMC_RSP5=0x%04x\n", *MMC_RSP5);
        printk("MMC_RSP6=0x%04x\n", *MMC_RSP6);
        printk("MMC_RSP7=0x%04x\n", *MMC_RSP7);
        printk("MMC_BUF=0x%04x\n", *MMC_BUF);
        printk(". . . . . . . . . . . . .\n");
}


int host_init(transfer_mode_t transfer_mode)
{
        int status;
        KDEBUG(TRANSPORT, "ENTRY\n");
		
		if (transfer_mode != TRANSFER_MODE_1BIT_SDIO) {
			TRSP_MSG("Only 1-bit SDIO is supported.\n");
			return -EINVAL;
		}

	/* Enable MMC/SD clock and disable HI-Z on the MMC.DAT2 pin */
	outl(inl(MOD_CONF_CTRL_0) | (1 << 23) | (1 << 21), MOD_CONF_CTRL_0);

	/* Configure MMC.DAT3, MMC.CLK, MMC.DAT0, MMC.DAT2, MMC.DAT1,
         * MMC.DAT0, MMC.CMD pins.
         */
	outl(inl(FUNC_MUX_CTRL_D) & ~((1 << 14) | (1 << 13) | (1 << 12)),
	     FUNC_MUX_CTRL_D);
	outl(inl(FUNC_MUX_CTRL_A) & ~((1 << 23) | (1 << 22) | (1 << 21)),
	     FUNC_MUX_CTRL_A);
	outl(inl(FUNC_MUX_CTRL_B) & ~((1 << 2) | (1 << 1) | (1 << 0)),
	     FUNC_MUX_CTRL_B);
	outl(inl(FUNC_MUX_CTRL_A) & ~((1 << 20) | (1 << 19) | (1 << 18)),
	     FUNC_MUX_CTRL_A);
	outl(inl(FUNC_MUX_CTRL_A) & ~((1 << 26) | (1 << 25) | (1 << 24)),
	     FUNC_MUX_CTRL_A);
	outl(inl(FUNC_MUX_CTRL_A) & ~((1 << 29) | (1 << 28) | (1 << 27)),
	     FUNC_MUX_CTRL_A);

        /* Pull ups */
        outl(inl(PULL_DWN_CTRL_2) & ~((1 << 12) | (1 << 13) | (1 << 14) |
                                      (1 << 15) | (1 << 16)), PULL_DWN_CTRL_2);
        outl(inl(PULL_DWN_CTRL_3) & ~((1 << 8)), PULL_DWN_CTRL_3);
        outl(inl(PU_PD_SEL_2) | ((1 << 12) | (1 << 13) | (1 << 14) |
                                 (1 << 15) | (1 << 16)), PU_PD_SEL_2);
        outl(inl(PU_PD_SEL_3) | ((1 << 8)), PU_PD_SEL_3);


        /* Confirm pin reconfiguration */
        outl(0xEAEF, COMP_MODE_CTRL_0);

        /* Reset; we have to make sure the clock is enabled before setting
         * the POW bit in MMC_CON, otherwise MMC_SYSS[0] will never be set.
         */
        *MMC_SYSC = SYSC_SRST;
        *MMC_CON |= CLK_DIVIDER;
        *MMC_CON |= CON_POWER_UP;

        /* Wait for reset to complete */
        while(!(*MMC_SYSS & SYSS_RSTD));

        /* Disable SDIO block mode */
        *MMC_NBLK = 0;

        /* Use SDIO irq */
        *MMC_SDIO |= SDIO_IRQE;

	/* Disable DMA and set default AEL and AFL (2bytes) */
	*MMC_BUF = 0;

#ifdef USE_DMA
	status = omap_request_dma (eMMCRx, "NANOSDIO RX DMA",
                                   (dma_callback_t) omap_dma_rx_callback,
                                   NULL, &(rx_dma_regs));
        TRSP_ASSERT(status == 0);
	
	status = omap_request_dma (eMMCTx, "NANOSDIO TX DMA",
                                   (dma_callback_t) omap_dma_tx_callback,
                                   NULL, &(tx_dma_regs));
        TRSP_ASSERT(status == 0);
#endif

        
	return 0;
}

int host_reset(void)
{
        /* No reset required after firmware download */
        return 0;
}

void host_clock(clk_mode_t clk_mode)
{
        /* Clock toggling done in hardware */
        return;
}

int host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{
        uint16_t mask = 0;

        KDEBUG(TRANSPORT, "ENTRY\n");
        
        switch(irq_id) {
	case IRQ_ID_RESPONSE_COMPLETION:  mask = STAT_EOC; break;
	case IRQ_ID_TARGET:               mask = STAT_CIRQ; break;
	case IRQ_ID_RW_ACCESS_COMPLETION: mask = STAT_BRS; break; 
	default: TRSP_ASSERT(0);
	}
        
        switch(irq_op) {
        case IRQ_OP_ENABLE: *MMC_IE |= mask; break;
        case IRQ_OP_DISABLE: *MMC_IE &= ~mask; break;
        case IRQ_OP_ACKNOWLEDGE: *MMC_STAT = mask; break;
        case IRQ_OP_STATUS: return *MMC_STAT & mask;
        default: TRSP_ASSERT(0);
        }

        /* HW bug workaround */
        if(irq_id == IRQ_ID_RESPONSE_COMPLETION &&
           irq_op == IRQ_OP_ACKNOWLEDGE)
                *MMC_STAT = STAT_CIRQ;
        
        
	return 0;
}

int host_fifo(void *data, uint16_t len, uint32_t flags)
{

        uint8_t no_dma = 0;
	if(len % 16 || ((uint32_t) data) % 4) no_dma = 1;

        KDEBUG(TRANSPORT, "ENTER: len=%d MMC_STAT=0x%04x\n", len, *MMC_STAT);

#ifdef USE_DMA
        if(flags & FIFO_FLAG_TO_HOST) {
                if(no_dma) omap_sdio_fifo_read(data, len);
                else omap_sdio_dma_read((u32)virt_to_bus(data),len);
        }
        else {
                if(no_dma) omap_sdio_fifo_write(data, len);
                else omap_sdio_dma_write((u32)virt_to_bus(data),len);
        }
#else
        
        if(flags & FIFO_FLAG_TO_HOST) omap_sdio_fifo_read(data, len);
        else omap_sdio_fifo_write(data, len);
#endif
        return 0;

}

irq_status_t host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
					  uint32_t flags, uint32_t arg, void* data)
{
        
        uint16_t cmd_val = cmd;
        uint8_t no_dma = 0;
        uint16_t len = arg & 0xFFFF; /* ugly */
	if(len % 16 || ((uint32_t) data) % 4) no_dma = 1;

        KDEBUG(TRANSPORT, "ENTRY cmd=%d\n", cmd);
        KDEBUG(TRANSPORT, "MMC_STAT: 0x%04x\n", *MMC_STAT);

        /* ARG */
        *MMC_ARGL = arg & 0xFFFF;
        *MMC_ARGH = (arg >> 16) & 0xFFFF;

        
        /* Respose */
        switch(cmd_resp) {
        case CMD_RESP_R1: cmd_val |= CMD_RSP0; break;
        case CMD_RESP_R5: cmd_val |= (CMD_RSP2 | CMD_RSP0); break;
        case CMD_RESP_R6: cmd_val |= (CMD_RSP2 | CMD_RSP1); break;
        default: TRSP_ASSERT(0); /* Not implemented */
        }

        if(flags & CMD_FLAG_BUSY) cmd_val |= CMD_BUSY;

        /* Type */
        switch(cmd_type) {
        case CMD_TYPE_BCR: cmd_val |= CMD_TYPE0; break;
        case CMD_TYPE_AC: cmd_val |= CMD_TYPE1; break;
        case CMD_TYPE_ADTC: cmd_val |= (CMD_TYPE1 | CMD_TYPE0); break;
        default: TRSP_ASSERT(0);
        }

#ifdef USE_DMA
        /* Disable DMA and set default AEL and AFL (2bytes) */
	if(no_dma) {
                *MMC_BUF = 0;
        }
	else {
		//Enable TX DMA on SD HOST
		*MMC_BUF |= BUF_TXDE;
                //AEL 16 bytes
		*MMC_BUF |= (BUF_AEL0 | BUF_AEL1 | BUF_AEL2);
		//Enable RX DMA on SD HOST
		*MMC_BUF |= BUF_RXDE;
		//AFL 16 bytes
		*MMC_BUF |= (BUF_AFL0 | BUF_AFL1 | BUF_AFL2);
        }
#endif
        
	if((flags & CMD_FLAG_DATA_EXISTS) &&
           (flags & CMD_FLAG_DIR_DATA_TO_HOST))
                cmd_val |= CMD_DDIR;
        
        *MMC_CMD = cmd_val;
		return IRQ_STATUS_SUCCESS;
}

uint32_t host_control(ctl_id_t ctl_id, uint32_t param)
{
        KDEBUG(TRANSPORT, "ENTRY ctl_id=%d, param=%d\n", ctl_id, param);
	switch(ctl_id) {

        case CTL_ID_LENGTH:
                *MMC_BLEN = param - 1;
                break;

	case CTL_ID_WAIT_CMD_READY:
                break;

        case CTL_ID_RESPONSE:
                KDEBUG(TRANSPORT, "RSP7: 0x%04x\n", *MMC_RSP7);
                return *MMC_RSP7 << 16;

        default: break;
	}
	return 0;
}


void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
        if(gpio_id != GPIO_ID_RESET) return;

        switch(gpio_op) {
        case GPIO_OP_ENABLE:
                outl(inl(GPIO2_DIRECTION_REG) & ~(1 << 0),
                     GPIO2_DIRECTION_REG);
                break;
                
        case GPIO_OP_HIGH:
                outl(inl(GPIO2_DATAOUT_REG) | (1 << 0), GPIO2_DATAOUT_REG);
                break;
                
        case GPIO_OP_LOW:
                outl(inl(GPIO2_DATAOUT_REG) & ~(1 << 0), GPIO2_DATAOUT_REG);
                break;

        default: break;
        }
}

void host_exit(void)
{
#ifdef USE_DMA
    	omap_free_dma(tx_dma_regs);
	omap_free_dma(rx_dma_regs);
#endif
        return;
}
