/**
*   $Id: host.c 12681 2009-09-08 10:07:54Z phth $
*
*/

#include "host.h"

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#if defined(CONFIG_MXC_MC13783_POWER)
#include <asm/arch/pmic_power.h>
#endif
#include <asm/arch/dma.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include "mxc_mmc.h"
#include "crm_regs.h"
#include "nanoutil.h"


#include <asm/io.h>
#include <asm/arch/gpio.h>
#include "iomux.h"

#define IMX31_MAGIC_VERSION "nanoradio wifi driver magic version 0.rc80"
#define BUSY_WAIT_TIMEOUT_VALUE 1000000

static void dump_status(const char *func, int sts);

#define IMX31_ASSERT(EXPR) do { if(!(EXPR)) { printk("%s : %d : ASSERTION FAILED\n", __FUNCTION__, __LINE__);\
dump_status(__FUNCTION__, imx31_sdhc_readl(MMC_STATUS));\
} } while(0);

static unsigned int is_4bit_sdio;

#define SDHC_DMA_ENABLE

#ifdef SDHC_DMA_ENABLE

mxc_dma_requestbuf_t *imx31_dma;

struct imx31_host {
 
	/*!
	 * DMA channel number.
	 */
	int dma;

	/*!
	 * Holds the number of bytes to transfer using DMA.
	 */
	unsigned int dma_size;

	/*!
	 * DMA address for scatter-gather transfers
	 */
	dma_addr_t buffer_phys;

	/*!
	 * Length of the scatter-gather list
	 */
	unsigned int dma_len;

	/*!
	 * Holds the direction of data transfer.
	 */
	unsigned int dma_dir;

	/*!
	 * Id for MMC block.
	 */
	unsigned int id;

	/*!
	 * Bus Width for MMC block.
	 */
	unsigned int bus_width;

} *imx31_sdio;

#endif



/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
void gpio_sdhc_active(void)
{
		mxc_request_iomux(MX31_PIN_SD1_CLK, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_CMD, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA0, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA1, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA2, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_SD1_DATA3, OUTPUTCONFIG_FUNC,
				  INPUTCONFIG_FUNC);

		mxc_iomux_set_pad(MX31_PIN_SD1_CLK,
				  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
		mxc_iomux_set_pad(MX31_PIN_SD1_CMD,
				  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
		mxc_iomux_set_pad(MX31_PIN_SD1_DATA0,
				  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
		mxc_iomux_set_pad(MX31_PIN_SD1_DATA1,
				  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
		mxc_iomux_set_pad(MX31_PIN_SD1_DATA2,
				  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));
		mxc_iomux_set_pad(MX31_PIN_SD1_DATA3,
				  (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

}


/*!
 * This function resets the SDHC host.
 *
 */
static void sdhc_softreset(void)
{
	/* reset sequence */
	imx31_sdhc_writel(0x8, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x9, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x1, MMC_STR_STP_CLK);
	imx31_sdhc_writel(0x3f, MMC_CLK_RATE);
}

static void imx31_stop_clock(bool wait)
{
  volatile unsigned long timeout;

	while (1) {
   imx31_sdhc_writel(STR_STP_CLK_IPG_CLK_GATE_DIS |
			     STR_STP_CLK_IPG_PERCLK_GATE_DIS |
			     STR_STP_CLK_STOP_CLK, MMC_STR_STP_CLK);

		if (!wait)
			break;

		timeout = BUSY_WAIT_TIMEOUT_VALUE;
		while (timeout--) {
			if (!(imx31_sdhc_readl(MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN))
				break;
		}

		if (!(imx31_sdhc_readl(MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN))
			break;
	}

}

static void imx31_start_clock(bool wait)
{
  volatile unsigned long timeout;

	while (1) {
		imx31_sdhc_writel(STR_STP_CLK_IPG_CLK_GATE_DIS |
			     STR_STP_CLK_IPG_PERCLK_GATE_DIS |
			     STR_STP_CLK_START_CLK, MMC_STR_STP_CLK);

		if (!wait)
			break;

		timeout = BUSY_WAIT_TIMEOUT_VALUE;

		while (timeout--) {
			if (imx31_sdhc_readl(MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) {
				break;
			}
		}

		if (imx31_sdhc_readl(MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) {
			break;
		}
	}

}

#ifdef SDHC_DMA_ENABLE
static void imx31_dma_callback(void *devid, int error, unsigned int cnt)
{
 struct imx31_host *imx31_sdio = (struct imx31_host *) devid;
 volatile unsigned int timeout = BUSY_WAIT_TIMEOUT_VALUE;
  volatile u32 status;

  mxc_dma_disable(imx31_sdio->dma);

  if(error) {
   printk("DMA error\n");
   dma_unmap_single(NULL, imx31_sdio->buffer_phys, imx31_sdio->dma_size,
                                     imx31_sdio->dma_dir);
   sdio_fifo_callback();
   return;
  }


  do {
   if(!timeout)
    break;
   timeout --;
  }while (!((imx31_sdhc_readl(MMC_STATUS)) & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)));
  IMX31_ASSERT(timeout);

	if ((imx31_sdhc_readl(MMC_STATUS)) & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)) {
		/* check for time out and CRC errors */
		status = imx31_sdhc_readl(MMC_STATUS);
		if (status & STATUS_READ_OP_DONE) {
			if (status & STATUS_TIME_OUT_READ) {
#if 0
				printk("%s: Read time out occurred\n",
					 DRIVER_NAME);
#endif
				imx31_sdhc_writel(STATUS_TIME_OUT_READ, MMC_STATUS);
			} else if (status & STATUS_READ_CRC_ERR) {
#if 0
				printk("%s: Read CRC error occurred\n",
					 DRIVER_NAME);
#endif
				imx31_sdhc_writel(STATUS_READ_CRC_ERR, MMC_STATUS);
			}
			imx31_sdhc_writel(STATUS_READ_OP_DONE, MMC_STATUS);
		}

		/* check for CRC errors */
		if (status & STATUS_WRITE_OP_DONE) {
			if (status & STATUS_WRITE_CRC_ERR) {
#if 0
				printk("%s: Write CRC error occurred\n",
					 DRIVER_NAME);
#endif
				imx31_sdhc_writel(STATUS_WRITE_CRC_ERR, MMC_STATUS);
			}
			imx31_sdhc_writel(STATUS_WRITE_OP_DONE, MMC_STATUS);
		}
	} else {
		printk("%s:%d: MXC MMC DMA transfer failed.\n", __FUNCTION__, __LINE__);
	}

dma_unmap_single(NULL, imx31_sdio->buffer_phys, imx31_sdio->dma_size,
                                     imx31_sdio->dma_dir);
sdio_fifo_callback();
}
#endif

void power_on(void)
{
#if defined(CONFIG_MXC_MC13783_POWER) 
    t_regulator_voltage voltage;
#endif

  voltage.vmmc1 = 7;
  pmic_power_regulator_set_voltage(REGU_VMMC1, voltage);
  pmic_power_regulator_set_lp_mode(REGU_VMMC1, LOW_POWER_DISABLED);
  pmic_power_regulator_on(REGU_VMMC1);

#ifdef USE_POWER_PRINTK
  printk("%s\n", __FUNCTION__);
#endif

}

void power_off(void)
{

  pmic_power_regulator_set_lp_mode(REGU_VMMC1, LOW_POWER_EN);
  pmic_power_regulator_off(REGU_VMMC1);

#ifdef USE_POWER_PRINTK
  printk("%s\n", __FUNCTION__);
#endif

}

int
host_init(transfer_mode_t transfer_mode)
{
  volatile u32 reg;
#if 0
  volatile unsigned long timeout;
  bool wait = false;
#endif
#if 0
#if defined(CONFIG_MXC_MC13783_POWER) 
    t_regulator_voltage voltage;
#endif
#endif

  printk("Driver's Magic Version : %s\n", IMX31_MAGIC_VERSION);
  
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

  reg = __raw_readl(MXC_CCM_CGR0);
  reg |= 3 << (MXC_CCM_CGR0_SD_MMC1_OFFSET);
  __raw_writel(reg, MXC_CCM_CGR0);
	//printk("%s:PARENT CLK_RATE: %d\n", DRIVER_NAME, __raw_readl(MXC_CCM_CGR0));

  mdelay(100);

  sdhc_softreset();

  if(imx31_sdhc_readl(MMC_REV_NO) != SDHC_REV_NO)
   printk("Error in Initialization\n");


  mdelay(500);

  // READ and RESPONSE timeout values are being Set to the MAX (0xFF, 0xFFFF)
  imx31_sdhc_writel(READ_TO_VALUE, MMC_READ_TO);
  imx31_sdhc_writel(RES_TO_VALUE, MMC_READ_TO);

#ifdef USE_IRQ_ID_RESPONSE_COMPLETION
  imx31_sdhc_writel(INT_CNTR_END_CMD_RES, MMC_INT_CNTR);
#endif

  reg = imx31_sdhc_readl(MMC_INT_CNTR);
  reg |= INT_CNTR_SDIO_INT_WKP_EN;
  imx31_sdhc_writel(reg, MMC_INT_CNTR);

	// Maybe we need it for 4-bit sdio ????
	if (is_4bit_sdio) {
		reg = imx31_sdhc_readl(MMC_INT_CNTR);
		reg |= INT_CNTR_DAT0_EN;
		imx31_sdhc_writel(reg, MMC_INT_CNTR);
	}

#if 1

  gpio_sdhc_active();
  mdelay(500);

#endif



#if 0
  printk("#############################################\n");
  printk("BASE ADDRESS FOR SDIO : 0x%x\n", (unsigned int)MMC_SDHC1_VIRT_ADDR);
  printk("#############################################\n");
#endif
 
#if 0
  voltage.vmmc1 = 7;
  pmic_power_regulator_set_voltage(REGU_VMMC1, voltage);
  pmic_power_regulator_set_lp_mode(REGU_VMMC1, LOW_POWER_DISABLED);
  pmic_power_regulator_on(REGU_VMMC1);

  mdelay(500);
#endif

#ifdef SDHC_DMA_ENABLE
   imx31_sdio = kmalloc(sizeof(struct imx31_host), GFP_KERNEL);
   if(imx31_sdio == NULL) {
    printk("Cannot allocate priv structure\n");
    return -ENODEV;
   }

	if (is_4bit_sdio) {
		imx31_sdio->id = MXC_DMA_MMC1_WIDTH_4;
		imx31_sdio->bus_width = MMC_BUS_WIDTH_4;
	}
	else {
		imx31_sdio->id = MXC_DMA_MMC1_WIDTH_1;
		imx31_sdio->bus_width = MMC_BUS_WIDTH_1;
	}

  imx31_sdio->dma = mxc_dma_request(imx31_sdio->id, "MXC MMC");
  if (imx31_sdio->dma < 0) {
	 printk("Cannot allocate MMC DMA channel\n");
  }

  imx31_dma = (mxc_dma_requestbuf_t *) kzalloc(sizeof(mxc_dma_requestbuf_t), GFP_KERNEL);
  if(imx31_dma == NULL) {
   TRSP_ASSERT(imx31_dma);
   return -ENOMEM;
  }

  mxc_dma_callback_set(imx31_sdio->dma, imx31_dma_callback, (void *)imx31_sdio);

#endif


	////KDEBUG(TRANSPORT, "EXIT");
	return 0;
}

int host_reset(void)
{
	return 0;
}

void
host_clock(clk_mode_t clk_mode)
{
  bool clk_on = false;
  int prescaler = 0;
  int divider = 0x2;

  switch(clk_mode) {
  case CLK_MODE_FAST:
		  prescaler = 0x000; /* 22 MHz */
          clk_on = true;
          break;

  case CLK_MODE_SLOW:
		  prescaler = 0x080; /* 85 kHz */
          clk_on = true;
          break;

  case CLK_MODE_OFF:
          clk_on = false;
          break;
  default: TRSP_ASSERT(0);
  }

  imx31_sdhc_writel(((prescaler << 4) | divider), MMC_CLK_RATE);

  if(clk_on)
    imx31_start_clock(true);
  else
    imx31_stop_clock(true);

	return;
}

static irq_status_t imx31_sdio_chk_resp_completion(uint32_t status_reg)
{
	if ((status_reg & STATUS_END_CMD_RESP) == 0)
		return IRQ_STATUS_INACTIVE;
	if (status_reg & STATUS_TIME_OUT_RESP) {
		TRSP_MSG("SDIO response timeout\n");
		return IRQ_STATUS_TIMEOUT;
	}
	if (status_reg & STATUS_RESP_CRC_ERR) {
		TRSP_MSG("SDIO response CRC error\n");
		return IRQ_STATUS_CRC;
	}
	return IRQ_STATUS_ACTIVE;
}

irq_status_t
host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{

	uint32_t i __attribute__ ((unused));
	volatile uint32_t dummy = 0;
  volatile uint32_t reg = 0 ;
#if 0
  volatile uint32_t timeout = BUSY_WAIT_TIMEOUT_VALUE;
#endif
  volatile uint32_t status = 0 ;
  volatile uint32_t reason = 0;

	
	switch (irq_id) {
	case IRQ_ID_RESPONSE_COMPLETION:
		reason |= INT_CNTR_END_CMD_RES;
    status |= STATUS_END_CMD_RESP;
		break;
	case IRQ_ID_RW_ACCESS_COMPLETION:
		break;
	case IRQ_ID_FIFO_READ_READY:
		reason |= INT_CNTR_BUF_READ_EN;
    status |= STATUS_BUF_READ_RDY;
		break;
	case IRQ_ID_FIFO_WRITE_READY:
		reason |= INT_CNTR_BUF_WRITE_EN;
    status |= STATUS_BUF_WRITE_RDY;
		break;
	case IRQ_ID_TARGET:
		reason |= INT_CNTR_SDIO_IRQ_EN;
    status |= STATUS_SDIO_INT_ACTIVE;
		break;
	default:
		TRSP_ASSERT(0);
		return IRQ_STATUS_FAIL;
	}

	switch (irq_op) {
	case IRQ_OP_ENABLE:
    if(irq_id == IRQ_ID_TARGET) {
		   dummy = imx31_sdhc_readl(MMC_STATUS);
       dummy |= status;
       imx31_sdhc_writel(dummy, MMC_STATUS);
    }
		dummy = imx31_sdhc_readl(MMC_INT_CNTR);
		dummy |= reason;
		imx31_sdhc_writel(dummy, MMC_INT_CNTR);
		break;
	case IRQ_OP_DISABLE:
		dummy = imx31_sdhc_readl(MMC_INT_CNTR);
		dummy &= ~reason;
		imx31_sdhc_writel(dummy, MMC_INT_CNTR);
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_INT_CNTR) & reason));
		break;
	case IRQ_OP_STATUS:
      reg = imx31_sdhc_readl(MMC_STATUS);
	  if (irq_id == IRQ_ID_RESPONSE_COMPLETION)
		return imx31_sdio_chk_resp_completion(reg);
	  else
		return (reg & status) ? IRQ_STATUS_ACTIVE : IRQ_STATUS_INACTIVE;
	case IRQ_OP_ACKNOWLEDGE:
      imx31_sdhc_writel(status | STATUS_ERR_MASK, MMC_STATUS);
#if 0
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & status));
#endif
	  break;
	default:
		TRSP_ASSERT(0);
		return IRQ_STATUS_FAIL;
	}

	return IRQ_STATUS_SUCCESS;
}

int
host_fifo(void *data, unsigned long data_phys, uint16_t len, uint32_t flags)
{
#ifdef SDHC_DMA_ENABLE
  volatile unsigned int dma = 1;
  volatile unsigned int err = 0;
#endif
	volatile u32 temp_data;
	unsigned int status;
	volatile unsigned long *buf;
	volatile u8 *buf8;
	volatile int no_of_bytes;
	volatile int no_of_words;
  volatile unsigned int timeout = BUSY_WAIT_TIMEOUT_VALUE;
  int i;


#if 0
  if((imx31_sdhc_readl(MMC_STATUS) & STATUS_TIME_OUT_RESP)) {
    printk("STATUS_TIME_OUT_RESP\n");
    imx31_sdhc_writel(STATUS_TIME_OUT_RESP, MMC_STATUS);
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_TIME_OUT_RESP));
    sdio_fifo_callback();
    return 0;
  } else if((imx31_sdhc_readl(MMC_STATUS) & STATUS_RESP_CRC_ERR)) {
    printk("STATUS_RESP_CRC_ERR\n");
    imx31_sdhc_writel(STATUS_RESP_CRC_ERR, MMC_STATUS);
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_RESP_CRC_ERR));
    sdio_fifo_callback();
    return 0;
  }
#endif

#ifdef SDHC_DMA_ENABLE
  if((len % 16) || (((uint32_t) data) % 4))
      dma = 0;
     
     imx31_sdio->buffer_phys = (dma_addr_t)data_phys;
     imx31_sdio->dma_size = len;

     if(dma && (len > (16 << imx31_sdio->bus_width))) {

        imx31_dma->num_of_bytes = len; /*!< the length of this transfer : bytes */

        //printk("dma packet - length : %d\n", len);

        if (flags & FIFO_FLAG_TO_HOST) {
          imx31_sdio->dma_dir = DMA_FROM_DEVICE;
          imx31_dma->src_addr = (dma_addr_t)(MMC_SDHC1_BASE_ADDR + MMC_BUFFER_ACCESS);  /*!< source address */
          imx31_dma->dst_addr = (dma_addr_t)data_phys;  /*!< destination address */

          err = mxc_dma_config(imx31_sdio->dma, imx31_dma, 1,
                                MXC_DMA_MODE_READ);
        } else {
          imx31_sdio->dma_dir = DMA_TO_DEVICE;
          /*!< source address */
          imx31_dma->src_addr = (dma_addr_t)data_phys;  
          /*!< destination address */
          imx31_dma->dst_addr = (dma_addr_t)(MMC_SDHC1_BASE_ADDR + MMC_BUFFER_ACCESS);  

          err = mxc_dma_config(imx31_sdio->dma, imx31_dma, 1,
                                MXC_DMA_MODE_WRITE);
        }

        if(err) {
         printk("Error in DMA tranfer : %d\n", err);
         sdio_fifo_callback();
         return 0;
        }

        err = mxc_dma_enable(imx31_sdio->dma);
        if(err) {
         printk("Error in DMA enable : %d\n", err);
         sdio_fifo_callback();
         return 0;
        }
        return 0;
     }
#endif

	buf = (volatile unsigned long *)(data);
	buf8 = (volatile u8 *) buf;

	/* calculate the number of bytes requested for transfer */
	no_of_bytes = len;
	no_of_words = (no_of_bytes + 3) / 4;

  if (flags & FIFO_FLAG_TO_HOST) {

	 // printk("%s : RX no_of_words=%d\n", DRIVER_NAME, no_of_words);

		for (i = 0; i < no_of_words; i++) {

			/* wait for buffers to be ready for read */
      do {
       if(!timeout)
         break;
       timeout --;
      }while (!(imx31_sdhc_readl(MMC_STATUS) & (STATUS_BUF_READ_RDY | STATUS_READ_OP_DONE))) ;

      IMX31_ASSERT(timeout);

			/* read 32 bit data */
			temp_data = imx31_sdhc_readl(MMC_BUFFER_ACCESS);
			if (no_of_bytes >= 4) {
				*buf++ = temp_data;
				no_of_bytes -= 4;
			} else {
				do {
					*buf8++ = temp_data;
					temp_data = temp_data >> 8;
				} while (--no_of_bytes);
			}
		}

    timeout = BUSY_WAIT_TIMEOUT_VALUE;
		/* wait for read operation completion bit */
    do {
     if(!timeout)
      break;
     timeout --;
    }while (!(imx31_sdhc_readl(MMC_STATUS) & STATUS_READ_OP_DONE)) ;
    IMX31_ASSERT(timeout);

		/* check for time out and CRC errors */
		status = imx31_sdhc_readl(MMC_STATUS);
		if (status & STATUS_TIME_OUT_READ) {
#if 0
			printk("%s: Read time out occurred\n", DRIVER_NAME);
#endif
			imx31_sdhc_writel(STATUS_TIME_OUT_READ, MMC_STATUS);
		} else if (status & STATUS_READ_CRC_ERR) {
#if 0
			printk("%s: Read CRC error occurred\n", DRIVER_NAME);
#endif
			imx31_sdhc_writel(STATUS_READ_CRC_ERR, MMC_STATUS);
		}
		imx31_sdhc_writel(STATUS_READ_OP_DONE, MMC_STATUS);
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & (STATUS_READ_OP_DONE)));
    

	//	printk("%s: Read %u words\n", DRIVER_NAME, i);

	} else {

	 // printk("%s : TX no_of_words=%d\n", DRIVER_NAME, no_of_words);
   timeout = BUSY_WAIT_TIMEOUT_VALUE;

		for (i = 0; i < no_of_words; i++) {

			/* wait for buffers to be ready for write */
      do {
       if(!timeout)
        break;
       timeout --;
      }while(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_BUF_WRITE_RDY)) ;
      IMX31_ASSERT(timeout);

			/* write 32 bit data */
			imx31_sdhc_writel(*buf++, MMC_BUFFER_ACCESS);
			if (imx31_sdhc_readl(MMC_STATUS) &
			    STATUS_WRITE_OP_DONE) {
				break;
			}
		}

    timeout = BUSY_WAIT_TIMEOUT_VALUE;
		/* wait for write operation completion bit */
      do {
       if(!timeout)
        break;
       timeout --;
      }while (!(imx31_sdhc_readl(MMC_STATUS) & STATUS_WRITE_OP_DONE));

      IMX31_ASSERT(timeout);

		/* check for CRC errors */
		status = imx31_sdhc_readl(MMC_STATUS);
		if (status & STATUS_WRITE_CRC_ERR) {
#if 0
			printk("%s: Write CRC error occurred : 0x%8x\n", DRIVER_NAME, imx31_sdhc_readl(MMC_STATUS));
#endif
			imx31_sdhc_writel(STATUS_WRITE_CRC_ERR, MMC_STATUS);
			imx31_sdhc_writel((0x3 << 9), MMC_STATUS);
#if 0
      IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_WRITE_CRC_ERR));
      IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & (0x3 << 9)));
#endif
		}
		imx31_sdhc_writel(STATUS_WRITE_OP_DONE, MMC_STATUS);
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & (STATUS_WRITE_OP_DONE)));
		//printk("%s: Written %u words\n", DRIVER_NAME, i);

	}

  sdio_fifo_callback();

  return 0;
}

irq_status_t
host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
	 uint32_t flags, uint32_t arg, void *data)
{
	uint32_t cmd_val = 0;
	uint32_t mask __attribute__ ((unused));
	uint32_t mode __attribute__ ((unused));
  //volatile unsigned long timeout = BUSY_WAIT_TIMEOUT_VALUE;
	long unsigned int present __attribute__ ((unused));

	//KDEBUG(TRANSPORT, "ENTRY cmd: 0x%02x, arg: 0x%08x", cmd, arg);


	TRSP_ASSERT(cmd_resp == CMD_RESP_R1 || cmd_resp == CMD_RESP_R5
		    || cmd_resp == CMD_RESP_R6);

  cmd_val |= CMD_DAT_CONT_RESPONSE_FORMAT_R1;

	if (flags & CMD_FLAG_DATA_EXISTS) {
    IMX31_ASSERT(!((imx31_sdhc_readl(MMC_STATUS) & STATUS_XBUF_FULL) &&
                   (imx31_sdhc_readl(MMC_STATUS) & STATUS_YBUF_FULL)));
		cmd_val |= CMD_DAT_CONT_DATA_ENABLE;
  }

	if (cmd == SDIO_RW_EXTENDED) {

		if (!(flags & CMD_FLAG_DIR_DATA_TO_HOST)) {
			cmd_val |= CMD_DAT_CONT_WRITE;
    }

	}

	if (is_4bit_sdio)
		cmd_val |= CMD_DAT_CONT_BUS_WIDTH_4;

  imx31_sdhc_writel(cmd, MMC_CMD);
  imx31_sdhc_writel(arg, MMC_ARG);
  imx31_sdhc_writel(cmd_val, MMC_CMD_DAT_CONT);

#ifndef USE_IRQ_ID_RESPONSE_COMPLETION
  // Busy Wait for END_CMD_RESP
  while(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_END_CMD_RESP));

  do {

     imx31_sdhc_writel(STATUS_END_CMD_RESP, MMC_STATUS);

  } while((imx31_sdhc_readl(MMC_STATUS) & STATUS_END_CMD_RESP));

  if((imx31_sdhc_readl(MMC_STATUS) & STATUS_TIME_OUT_RESP)) {
#if 0
    printk("STATUS_TIME_OUT_RESP\n");
#endif
    imx31_sdhc_writel(STATUS_TIME_OUT_RESP, MMC_STATUS);
#if 0
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_TIME_OUT_RESP));
#endif
  } else if((imx31_sdhc_readl(MMC_STATUS) & STATUS_RESP_CRC_ERR)) {
#if 0
    printk("STATUS_RESP_CRC_ERR\n");
#endif
    imx31_sdhc_writel(STATUS_RESP_CRC_ERR, MMC_STATUS);
#if 0
    IMX31_ASSERT(!(imx31_sdhc_readl(MMC_STATUS) & STATUS_RESP_CRC_ERR));
#endif
  }
#endif
//  printk("CMD : %d - STATUS : 0x%8x\n", cmd, imx31_sdhc_readl(MMC_STATUS));
	//KDEBUG(TRANSPORT, "EXIT");
	return IRQ_STATUS_SUCCESS;
}

uint32_t
host_control(ctl_id_t ctl_id, uint32_t param)
{
	volatile uint16_t mask __attribute__ ((unused));
  u32 a, b, c;
  volatile u32 resp;
	////KDEBUG(TRANSPORT, "ENTRY");

	switch (ctl_id) {
	case CTL_ID_LENGTH:
    imx31_sdhc_writel(0x1, MMC_NOB);
    imx31_sdhc_writel(param, MMC_BLK_LEN);
		break;
	case CTL_ID_WAIT_CMD_READY:
		break;
	case CTL_ID_RESPONSE:
    a = imx31_sdhc_readl(MMC_RES_FIFO) & 0xffff;
    b = imx31_sdhc_readl(MMC_RES_FIFO) & 0xffff;
    c = imx31_sdhc_readl(MMC_RES_FIFO) & 0xffff;
    resp = a << 24 | b << 8 | c >> 8;
		return resp;
	default:
		TRSP_ASSERT(0);
	}

	////KDEBUG(TRANSPORT, "EXIT");
	return 0;

}

static atomic_t power_dis = ATOMIC_INIT(0);
void
host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{

	int port __attribute__ ((unused));
	int mask __attribute__ ((unused));
	int err __attribute__ ((unused));

	// TODO : Host GPIO Implementation.
#ifdef BB_PM
   if ((gpio_op == GPIO_OP_HIGH) && (gpio_id == GPIO_ID_POWER)) { 
    power_on();
    //default -> mdelay(500);
    mdelay(500);
    power_off();
    //default -> mdelay(500);
    mdelay(500);
    power_on();
    //default -> mdelay(1500);
    mdelay(1500);
   } else if ((gpio_op == GPIO_OP_LOW) && (gpio_id == GPIO_ID_POWER)) { 
    if(!atomic_read(&power_dis)) {
       power_off();
       mdelay(500);
       atomic_set(&power_dis, 1);
    } else {
       atomic_set(&power_dis, 0);
    }
   }
#endif
	return;
}

void
host_exit(void)
{

#ifdef SDHC_DMA_ENABLE
   kfree(imx31_dma);
   mxc_dma_free(imx31_sdio->dma);
   kfree(imx31_sdio);
#endif

#ifdef BB_PM 
  pmic_power_regulator_set_lp_mode(REGU_VMMC1, LOW_POWER_EN);
  pmic_power_regulator_off(REGU_VMMC1);
  printk("powering off\n");
  mdelay(500);
#endif
	return;
}

static void dump_status(const char *func, int sts)
{
	unsigned int bitset;
	printk("%s:status: ", func);
	while (sts) {
		/* Find the next bit set */
		bitset = sts & ~(sts - 1);
		switch (bitset) {
		case STATUS_CARD_INSERTION:
			printk("CARD_INSERTION|");
			break;
		case STATUS_CARD_REMOVAL:
			printk("CARD_REMOVAL |");
			break;
		case STATUS_YBUF_EMPTY:
			printk("YBUF_EMPTY |");
			break;
		case STATUS_XBUF_EMPTY:
			printk("XBUF_EMPTY |");
			break;
		case STATUS_YBUF_FULL:
			printk("YBUF_FULL |");
			break;
		case STATUS_XBUF_FULL:
			printk("XBUF_FULL |");
			break;
		case STATUS_BUF_UND_RUN:
			printk("BUF_UND_RUN |");
			break;
		case STATUS_BUF_OVFL:
			printk("BUF_OVFL |");
			break;
		case STATUS_READ_OP_DONE:
			printk("READ_OP_DONE |");
			break;
		case STATUS_WR_CRC_ERROR_CODE_MASK:
			printk("WR_CRC_ERROR_CODE |");
			break;
		case STATUS_READ_CRC_ERR:
			printk("READ_CRC_ERR |");
			break;
		case STATUS_WRITE_CRC_ERR:
			printk("WRITE_CRC_ERR |");
			break;
		case STATUS_SDIO_INT_ACTIVE:
			printk("SDIO_INT_ACTIVE |");
			break;
		case STATUS_END_CMD_RESP:
			printk("END_CMD_RESP |");
			break;
		case STATUS_WRITE_OP_DONE:
			printk("WRITE_OP_DONE |");
			break;
		case STATUS_CARD_BUS_CLK_RUN:
			printk("CARD_BUS_CLK_RUN |");
			break;
		case STATUS_BUF_READ_RDY:
			printk("BUF_READ_RDY |");
			break;
		case STATUS_BUF_WRITE_RDY:
			printk("BUF_WRITE_RDY |");
			break;
		case STATUS_RESP_CRC_ERR:
			printk("RESP_CRC_ERR |");
			break;
		case STATUS_TIME_OUT_RESP:
			printk("TIME_OUT_RESP |");
			break;
		case STATUS_TIME_OUT_READ:
			printk("TIME_OUT_READ |");
			break;
		default:
			printk("Invalid Status Register value0x%x\n",
			       bitset);
			break;
		}
		sts &= ~bitset;
	}
	printk("\n");
}
