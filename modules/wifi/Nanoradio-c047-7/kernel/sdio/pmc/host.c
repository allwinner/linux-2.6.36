
/* Use DMA for data transfers between RAM and fifo */
#define USE_DMA


#include "host.h"
#include "pmc_registers.h"
#include <linux/delay.h>
#ifdef USE_DMA
#include <sdma.h>
#endif

#ifdef USE_DMA
struct T_tagSDMA_REQ sdma_req = { 0 };
#endif


#define TESTBIT(reg,bit) ((reg) & (1 << (bit)))
#define SETBIT(val,bit) (val) |= (1 << (bit))
#define CLRBIT(val,bit) (val) &= ~(1 << (bit))

#define WAITBIT_CLR(reg, bit) while(TESTBIT(reg, bit)) udelay(10);
#define WAITBIT_SET(reg, bit) while(!TESTBIT(reg, bit)) udelay(10);

static unsigned int is_4bit_sdio;


static void dma_callback(unsigned char ch, short sh)
{
        sdio_fifo_callback();
}

int host_init(transfer_mode_t transfer_mode)
{
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
		
#ifdef USE_DMA
        sdma_req.ch = D_SDMA_CH7;
        sdma_req.sleep_flg = D_SDMA_SLEEP_OFF;
        sdma_req.callback = dma_callback;
        sdma_req.intena = (D_SDMA_INTENA_ILA | D_SDMA_INTENA_DDONE);
        sdma_req.bswp = D_SDMA_ENDIAN_LITTLE;
        sdma_req.type = D_SDMA_FWD_LIMITED;
        sdma_req.sbuf_uint = D_SDMA_SBUF_UNIT_4BYTE;
        sdma_req.sbuf_stadr = (0x1000 >> 2); 
        sdma_req.sbuf_in = D_SDMA_SBUF_OFFSET_NOUSE;
        sdma_req.sbuf_out = D_SDMA_SBUF_OFFSET_NOUSE;
        sdma_req.sbuf_sh = 1;
        sdma_req.mwstart = 0;
        sdma_req.mwsize = 0;
        sdma_req.mrstart = 0;
        sdma_req.mrsize = 0;
#endif

        return 0;
}

int host_reset(void)
{
        /* Enable clock supply for SDIO 1 */
        SETBIT (*reg_clkenreg7, BIT_CEN_SDIO1);

        /* Enable clock supply for STREAM DMA */
        SETBIT (*reg_clkenreg7, BIT_CEN_SDMA);

        /* Release reset release for SDIO controller (held at startup) */
        SETBIT (*reg_rstctl61, BIT_nSDIOCTL); 

        /* Release reset release for STREAM DMA controller (held at startup) */
        SETBIT (*reg_rstctl61, BIT_nSTRMCTL); 

        /* Soft reset */
        *reg_soft_rst |= 0x3; /* Set bit 0 and 1 */

        /* Connect internal pull-up/pull-down registers for SDIO port */
        SETBIT (*reg_iopupdreg0, BIT_IOREG_AC);

        /* Enable SDIO port*/
        SETBIT (*reg_ioaodirreg, BIT_AODIR_M);

        /* Drive capability setup for CMD, DAT0-3 (1.5mA) */
        CLRBIT (*reg_iodrvreg1, BIT_IODRV_AJ);

        /* SD card port selection */
        *reg_sd_portsel &= ~0x0003; // Clear bit 0 and 1

        /* Clear lower 8 bits */
        *reg_sd_clk_ctrl &= 0xff00; // Clear lower 8 bits

#define D_SDIO_SD_OPTION_TOP_MASK 0x00F0
        *reg_sd_option = (*reg_sd_option & ~D_SDIO_SD_OPTION_TOP_MASK)|(0xE<<4);

        /* Transfer width should be changed to "1bit" ? */
        if(!is_4bit_sdio)
                SETBIT (*reg_sd_option, 15);
        
        /* SDIO Interruption setup */
        SETBIT (*reg_sdio_mode, 0);
  
        /* Setup interrupt masks */
        *reg_sd_info1_mask = 0x0315; /* Disable all irqs */
        *reg_sd_info2_mask = 0x837f; /* Disable all irqs */

#ifdef USE_DMA
        SETBIT(*reg_cc_ext_mode, 1);
#else
        SETBIT(*reg_cc_ext_mode, 5);
#endif

        return 0;
}

void host_clock(clk_mode_t clk_mode)
{
        switch(clk_mode) {

        case CLK_MODE_FAST: 
                CLRBIT(*reg_sd_clk_ctrl,5); 
                SETBIT(*reg_sd_clk_ctrl,8);
                break;

        case CLK_MODE_SLOW: 
                SETBIT(*reg_sd_clk_ctrl,5);
                SETBIT(*reg_sd_clk_ctrl,8);
                break;

        case CLK_MODE_OFF: 
                CLRBIT(*reg_sd_clk_ctrl,8); 
                break;

        default: TRSP_ASSERT(0);
        }
}

int host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{
        volatile uint16_t* status;
        volatile uint16_t* mask;
        uint16_t bit;

        switch(irq_id) {
        case IRQ_ID_RESPONSE_COMPLETION: 
                status = reg_sd_info1; mask = reg_sd_info1_mask; bit = 0;
                break;

        case IRQ_ID_FIFO_READ_READY: 
                status = reg_sd_info2; mask = reg_sd_info2_mask; bit = BIT_BRE;
                break;

        case IRQ_ID_FIFO_WRITE_READY: 
                status = reg_sd_info2; mask = reg_sd_info2_mask; bit = BIT_BWE;
                break;

        case IRQ_ID_TARGET: 
                status = reg_sdio_info1; mask = reg_sdio_info1_mask; bit = 0; 
                break;

        case IRQ_ID_RW_ACCESS_COMPLETION: 
                status = reg_sd_info1; mask = reg_sd_info1_mask; bit = 2; 
                break;

        default: TRSP_ASSERT(0); return 0;
        }

        switch(irq_op) {
        case IRQ_OP_ENABLE: CLRBIT(*mask, bit); break;
        case IRQ_OP_DISABLE: SETBIT(*mask, bit); break;
        case IRQ_OP_STATUS: return TESTBIT(*status, bit) ? IRQ_STATUS_ACTIVE : IRQ_STATUS_INACTIVE;
        case IRQ_OP_ACKNOWLEDGE: CLRBIT(*status, bit); break;
        default: TRSP_ASSERT(0); break;
        }

        return IRQ_STATUS_SUCCESS;
}

int host_fifo(void *data, unsigned long data_phys, uint16_t len, uint32_t flags)
{
        int status;
        uint16_t i;
        uint8_t no_dma = 0;
        
        u16* ptr = (u16*) data;
        if(len % 4 || ((uint32_t) data) % 4) no_dma = 1;
        

        if(no_dma) {
                
                /* This will only happen during core dump and last packet of
                 * firmware download.
                 */
                
                if(flags & FIFO_FLAG_TO_HOST) {
#ifndef USE_IRQ_ID_FIFO_READ_READY                      
                        /* Wait for READ_READY in the FIFO case only since
                         * it's a performance bottleneck in the DMA case.
                         */
                        WAITBIT_SET(*reg_sd_info2, BIT_BRE); 
                        CLRBIT(*reg_sd_info2, BIT_BRE);
#endif
                        for(i = 0; i < len / 2; i++) 
                                ptr[i] = *reg_sd_buf0;
                } 
                else {
#ifndef USE_IRQ_ID_FIFO_WRITE_READY                      
                        /* Wait for WRITE_READY in the FIFO case only since
                         * it's a performance bottleneck in the DMA case.
                         */
                        WAITBIT_SET(*reg_sd_info2, BIT_BWE); 
                        CLRBIT(*reg_sd_info2, BIT_BWE);
#endif
                        for(i = 0; i < len / 2; i++) 
                                *reg_sd_buf0 = *ptr++;
                }
                
                dma_callback(0, 0);
                return 0;
        }

#ifdef USE_DMA
        sdma_req.mwsize = 0; 
        sdma_req.mwstart = 0;
        sdma_req.mrsize = 0; 
        sdma_req.mrstart = 0;
        sdma_req.u_sdma_size.f_size = len;
        sdma_req.sbuf_width = len;
        
        if(flags & FIFO_FLAG_TO_HOST) {
                sdma_req.src_id = D_SDMA_FWD_ID_SD;
                sdma_req.dst_id = D_SDMA_FWD_ID_SDRAM;
                sdma_req.mwsize = len >> 2;
                sdma_req.mwstart = ((virt_to_phys(data) - 0x90000000) >> 2);
        }
        else {
                sdma_req.src_id = D_SDMA_FWD_ID_SDRAM;
                sdma_req.dst_id = D_SDMA_FWD_ID_SD;
                sdma_req.mrsize = len >> 2;
                sdma_req.mrstart = ((virt_to_phys(data) - 0x90000000) >> 2);
        }
        
        status = p_sdma_start(&sdma_req);      
        TRSP_ASSERT(status == 0);
        
#else

        if(flags & FIFO_FLAG_TO_HOST) {
#ifndef USE_IRQ_ID_FIFO_READ_READY                     
                /* Wait for READ_READY in the FIFO case only since
                 * it's a performance bottleneck in the DMA case.
                 */
                WAITBIT_SET(*reg_sd_info2, BIT_BRE); 
                CLRBIT(*reg_sd_info2, BIT_BRE);
#endif
                for(i = 0; i < len / 2; i++) 
                        ptr[i] = *reg_sd_buf0;
        }                
        else {
                for(i = 0; i < len / 2; i++) 
                        *reg_sd_buf0 = *ptr++;
        }
   
        dma_callback(0, 0);
#endif

        return 0;
}

irq_status_t host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp, 
	      			  uint32_t flags, uint32_t arg, void* data)
{
        uint16_t temp_cmd_reg = cmd;
        
        switch(cmd_resp) {
        case CMD_RESP_NONE: temp_cmd_reg |= (0x3 << 8); break;
        case CMD_RESP_R5:
        case CMD_RESP_R6:
        case CMD_RESP_R1: temp_cmd_reg |= (0x4 << 8); break;

        case CMD_RESP_R2: temp_cmd_reg |= (0x6 << 8); break;
        case CMD_RESP_R3:
        case CMD_RESP_R4: temp_cmd_reg |= (0x7 << 8); break;
        default: TRSP_ASSERT(0);
        }

        if(flags & CMD_FLAG_BUSY) temp_cmd_reg |= (0x1 << 8);
        if(flags & CMD_FLAG_SECURITY) SETBIT(temp_cmd_reg, 14);
        if(flags & CMD_FLAG_MULTI_BLOCK_TRANSFER) SETBIT(temp_cmd_reg, 13);
        if(flags & CMD_FLAG_DATA_EXISTS) {
                SETBIT(temp_cmd_reg, 11);
                if(flags & CMD_FLAG_DIR_DATA_TO_HOST)
                        SETBIT(temp_cmd_reg, 12);     
        }
        *reg_sd_arg = arg;
        *reg_sd_cmd = temp_cmd_reg;
		return IRQ_STATUS_SUCCESS;
}

uint32_t host_control(ctl_id_t ctl_id, uint32_t param)
{
        switch(ctl_id) {

        case CTL_ID_LENGTH:
                *reg_sd_size = param; break;

        case CTL_ID_WAIT_CMD_READY:
                WAITBIT_CLR(*reg_sd_info2, BIT_CBSY); break;

        case CTL_ID_RESPONSE:
                return *reg_sd_rsp1 << 16;

        default: break;
        }
        return 0;
}


void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
        volatile uint32_t* reg;

        switch(gpio_id) {
        case GPIO_ID_CPU_LOAD:
                reg = reg_gpio7; break;
                
        case GPIO_ID_RESET_D:
                reg = reg_gpio6; break;
                
        default: return;
        }

        switch(gpio_op) {
        case GPIO_OP_ENABLE:
                *reg &= ~0x38; CLRBIT(*reg, 3); SETBIT(*reg, 4); break;

        case GPIO_OP_HIGH:
                SETBIT(*reg, 5); break;
                
        case GPIO_OP_LOW:
                CLRBIT(*reg, 5); break;
                
        default: return;
        }
                

}

void host_if_power_on(void)
{
        printk(KERN_INFO "Calling PM ON");
}

void host_if_power_off(void)
{
        printk(KERN_INFO "Calling PM OFF");
}


void host_exit(void)
{
        return;
}
