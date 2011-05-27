#include <linux/spi/spi.h>

#include <mach/clock.h>
#include <asm/arch/dma.h>
#include <linux/dma-mapping.h>

//// global var
struct spi_device *spi_nrx_dev;


//// defines [Samsung]
#define SPI_BIT_WIDTH 	8
#define SPI_SPEED_HZ	26000000
#define SPI_CHIP_SELECT GPIO_A30
#define SPI_BUS_NUM 	1
#define SPI_BLOCK_CLOCK		104000000
#define SPI_MODE_NANO SPI_MODE_3
#define IRQ_SPI_CONTROLER IRQ_SPI1

#define NRX_SUCCESS 0
#define NRX_ERROR 1
///////////////////////////////////////////////////////////////
int dma_channel;

/* SPI clock */
struct clk	*spi_clk;

/* DMA callback */
struct semaphore dma_sem; 
int irq_cookie=0;
/** DMA transfer complete
 *
 * Called by Kernel when dma transfer is complete
 */
void spi_dma_callback(int channel, int cause, void* data)
{
	//KDEBUG(TRANSPORT, "ENTRY");
	up(&dma_sem);
	//KDEBUG(TRANSPORT, "EXIT");
}
///////////////////////////////////////////////////////////////
void set_interrupt(  int ie)
{
	volatile u32 stat;
	int timeout;
	stat = read_32(SPI_ADR_OFFSET_IER);
	stat &=~(SPI_IER_SPI_INTEOT | SPI_IER_SPI_INTTHR);
	stat |= (ie & (SPI_IER_SPI_INTEOT | SPI_IER_SPI_INTTHR));
	write_32(SPI_ADR_OFFSET_IER, stat);

   // wait for the interrupt enable to change
   timeout = 10;
	while (stat != read_32( SPI_ADR_OFFSET_IER)) {
		if (!timeout) {
			printk( "a : error in interrupt enable: %08x\n", stat);
			return;
		}
		timeout--;
		mdelay(1);
		write_32( SPI_ADR_OFFSET_IER, stat);
	}
}

irqreturn_t spi_interrupt(int irq,void *x)
{
	u16					stat;
	//KDEBUG(TRANSPORT, "ENTRY");
	/* Read interrupt source */
	stat = read_32(SPI_ADR_OFFSET_STAT);

	/* Acknowledge interrupt */
	write_32( SPI_ADR_OFFSET_STAT, (stat | SPI_STAT_SPI_INTCLR) & ~SPI_STAT_SPI_EOT);
	//KDEBUG(TRANSPORT, "EXIT");
	return IRQ_HANDLED;
}

///////////////////////////////////////////////////////////////////////////////////

struct pnx_dma_config cfg_read;
struct pnx_dma_ch_config ch_cfg_read;
struct pnx_dma_ch_ctrl ch_ctrl_read;
	
int spi_nrx_read( u8 *buf, size_t len)
{
	int err = 0;


	/// get physical address
	unsigned long buffer_phy = (unsigned long) dma_map_single(NULL, buf, len,
												DMA_FROM_DEVICE);
	
	KDEBUG(TRANSPORT, "ENTRY");

	/* enable SPI controler clock */
	clk_enable(spi_clk);
	
	/* SPI interrupt must be disabled now */
	disable_irq(IRQ_SPI_CONTROLER);
	
	/* write frame counter */
	write_32(SPI_ADR_OFFSET_FRM,len);
	
	/* We don't want SPI interrupt, so we'll mark it as done */
	set_interrupt(0);

	/* THR is set to low */
	and_32(SPI_ADR_OFFSET_CON,~SPI_CON_THR);

	/* disabled the spi */
	write_32(SPI_ADR_OFFSET_GLOBAL,0);
	
	/* set tranfer direction */
	and_32(SPI_ADR_OFFSET_CON,~SPI_CON_RxTx);
	
	/* enable again spi */
	write_32(SPI_ADR_OFFSET_GLOBAL,SPI_GLOBAL_SPI_ON);
	
	/* stop shift clock */
	or_32(SPI_ADR_OFFSET_CON,SPI_CON_SHIFT_OFF);
	
	/* Set up dma and start it */
	/* DMA setup */

	cfg_read.dest_addr = buffer_phy; /* buff->dma_buffer; */
	cfg_read.src_addr = (u32) (SPI_BASE_ADDR+SPI_ADR_OFFSET_DAT);
	ch_ctrl_read.tr_size = len;
	
	if (0 > (err = pnx_dma_pack_config(&ch_cfg_read, &cfg_read.ch_cfg))) {
		printk( "[%s][%d] We have errror in the DMA write\n",__func__,__LINE__);
		goto out;
	}
	if (0 > (err = pnx_dma_pack_control(&ch_ctrl_read, &cfg_read.ch_ctrl))) {
		if ( err == -E2BIG ) {
		} else {
			printk( "[%s][%d] We have errror in the DMA write\n",__func__,__LINE__);
			goto out;
		}
	}
	err = pnx_config_channel(dma_channel, &cfg_read);

	if(!err)
	{
		int ret;
	    	ret = pnx_dma_ch_enable( dma_channel );

    	if(ret)
    		printk("[%s][%d]Ret =%d on dma_start_channel\n",__func__,__LINE__,ret);
	}else
    	printk( "SETUP DMA FAILED\n");
		
	/*  DMA requests get generated only after dummy read/write */
	(void) read_32(SPI_ADR_OFFSET_DAT);
		
	/* SPI interrupt can be enabled now */
	enable_irq(IRQ_SPI_CONTROLER);
	
	/// wait the to dma complete
	down_interruptible(&dma_sem);

	/* stop shift clock */
	or_32(SPI_ADR_OFFSET_CON,SPI_CON_SHIFT_OFF);

	/* Unmap dma mapped transfers */
	dma_unmap_single(NULL, buffer_phy,len, DMA_FROM_DEVICE);

	/* report completed message */
	err = pnx_dma_ch_disable( dma_channel );
		
	KDEBUG(TRANSPORT, "EXIT");
	//nano_util_printbuf(buf,len, "rx");

	/* disable SPI controler clock */
	clk_disable(spi_clk);
	
	return 1;
out:
	/* disable SPI controler clock */
	clk_disable(spi_clk);

	return err;
}
///////////////////////////////////////////////////////////////
struct pnx_dma_config cfg_write;
struct pnx_dma_ch_config ch_cfg_write;
struct pnx_dma_ch_ctrl ch_ctrl_write;
	
int spi_nrx_write( u8 *buf, size_t len)
{
	int err = 0;


	/// get physical address
	unsigned long buffer_phy = (unsigned long) dma_map_single(NULL, buf, len,
												DMA_TO_DEVICE);
	
	KDEBUG(TRANSPORT, "ENTRY");

	/* enable SPI controler clock */
	clk_enable(spi_clk);
	
	/* SPI interrupt must be disabled now */
	disable_irq(IRQ_SPI_CONTROLER);
	
	/* write frame counter */
	write_32(SPI_ADR_OFFSET_FRM,len);
	
	set_interrupt(SPI_IER_SPI_INTEOT);
	
	/* THR is set to low */
	and_32(SPI_ADR_OFFSET_CON,~SPI_CON_THR);
	
	/* disabled the spi */
	//write_32(SPI_ADR_OFFSET_GLOBAL,0);
	
	/* set tranfer direction */
	or_32(SPI_ADR_OFFSET_CON,SPI_CON_RxTx);
	
	/* enable again spi */
	//write_32(SPI_ADR_OFFSET_GLOBAL,SPI_GLOBAL_SPI_ON);
	
	/* stop shift clock */
	or_32(SPI_ADR_OFFSET_CON,SPI_CON_SHIFT_OFF);
	
	/* Set up dma and start it */
	/* DMA setup */
	cfg_write.src_addr = buffer_phy; /* buff->dma_buffer; */
	cfg_write.dest_addr = (u32) (SPI_BASE_ADDR+SPI_ADR_OFFSET_DAT);
	ch_ctrl_write.tr_size = (len);
		
	if (0 > (err = pnx_dma_pack_config(&ch_cfg_write, &cfg_write.ch_cfg))) {
		printk( "[%s][%d] We have errror in the DMA write\n",__func__,__LINE__);
		goto out;
	}
	if (0 > (err = pnx_dma_pack_control(&ch_ctrl_write, &cfg_write.ch_ctrl))) {
		if ( err == -E2BIG ) {
		} else {
			printk( "[%s][%d] We have errror in the DMA write\n",__func__,__LINE__);
			goto out;
		}
	}
	err = pnx_config_channel(dma_channel, &cfg_write);

	if(!err)
	{
		int ret;
	    	ret = pnx_dma_ch_enable( dma_channel );

    	if(ret)
    		printk("[%s][%d]Ret =%d on dma_start_channel\n",__func__,__LINE__,ret);
	}else
    	printk( "SETUP DMA FAILED\n");
		
	/* SPI interrupt can be enabled now */
	enable_irq(IRQ_SPI_CONTROLER);
	
	/// wait the to dma complete
	down_interruptible(&dma_sem);

	/* stop shift clock */
	or_32(SPI_ADR_OFFSET_CON,SPI_CON_SHIFT_OFF);

	/* Unmap dma mapped transfers */
	dma_unmap_single(NULL, buffer_phy,len, DMA_TO_DEVICE);

	/* report completed message */
	err = pnx_dma_ch_disable( dma_channel );
		
	KDEBUG(TRANSPORT, "EXIT");
	/* disable SPI controler clock */
	clk_disable(spi_clk);
	
	//nano_util_printbuf(buf,len, "tx");
	return 1;
out:

	/* disable SPI controler clock */
	clk_disable(spi_clk);

	return err;
}

///////////////////////////////////////////////////////////////

/*!
 * SDIO is emulated over SPI.
 * So, we need SDIO command fields etc.
 * See SDIO specification.
 */
const unsigned char cmd53_write_temp[] = 
{
        0xFF,
        0xFF,
        0xFF,
        0x75,
        0x90,
        0x00,
        0x00,   // size msb
        0x00,   // size lsb
        0x01,
        0xff,
        0xff, 0xff,
        0xff, 0xfe  // preamble
/*
	0xff,	// ** command preamble
	0x75,	// StartBit = 0, Dir = 1, cmd index = 0b110101
	0x90,	// R/W = 1, Func# = 1, BlockMode = 0, OpCode = 0
	0x00,
	0x00,   // size msb
	0x00,   // size lsb
	0x01,	// CRC = 0b0000000, EndBit = 1
	0xff,	// ** response preamble
	0xff, 0xfe	// receive R5 response (16 bits), also send StartBit = 0
*/
};

const unsigned char cmd53_read_temp[] =
{
        0xFF,
        0xFF,
        0xFF,
        0x75,
        0x10,
        0x00,
        0x00,   // size msb
        0x00,   // size lsb
        0x01,
        0xff,
        0xff, 0xff,
        0xff, 0xff  // preamble 
/*
	0xff,	// ** command preamble
	0x75,	// StartBit = 0, Dir = 1, cmd index = 0b110101
	0x10,	// R/W = 0, Func# = 1, BlockMode = 0, OpCode = 0
	0x00,
	0x00,   // size msb
	0x00,   // size lsb
	0x01,	// CRC = 0b0000000, EndBit = 1
	0xff,	// ** response preamble
	0xff, 0xff,	// receive R5 response (16 bits)
	0xff, 0xff	// ** data phase preamble, receive StartBit
*/
};

const unsigned char cmd53_crc_temp[] =
{
	0xff, 0xff
};

const unsigned char cmd52_temp[] =
{
	0xff,	/*** command preamble */
	0x74,	/* StartBit = 0, Dir = 1, cmd index = 0b110100 */
	0x00,
	0x00,
	0x00,
	0x00,
	0xff,	/* CRC = 0b1111111, EndBit = 1 */
	0xff,	/*** response preamble */
	0xff, 0xff	/* receive R5 response (16 bits) */
};

char *cmd53_write;
char *cmd53_read;
char *cmd53_crc;
char *cmd52;
///////////////////////////////////////////////////////////////
int spi_alloc_buff(void)
{
	cmd53_write =(char *) kmalloc(sizeof(cmd53_write_temp)*sizeof(char),GFP_KERNEL);
	cmd53_read = (char *) kmalloc(sizeof(cmd53_read_temp)*sizeof(char),GFP_KERNEL);
	cmd53_crc = (char *) kmalloc(sizeof(cmd53_crc_temp)*sizeof(char),GFP_KERNEL);
	cmd52 = (char *) kmalloc(sizeof(cmd52_temp)*sizeof(char),GFP_KERNEL);

	/// copy buffers
	memcpy(cmd53_write, cmd53_write_temp, sizeof(cmd53_write_temp));
	memcpy(cmd53_read, cmd53_read_temp, sizeof(cmd53_read_temp));
	memcpy(cmd53_crc, cmd53_crc_temp, sizeof(cmd53_crc_temp));
	memcpy(cmd52, cmd52_temp, sizeof(cmd52_temp));

	return 0;
}

void spi_free_buff(void)
{
	kfree(cmd53_write);
	kfree(cmd53_read);
	kfree(cmd53_crc);
	kfree(cmd52);
}
///////////////////////////////////////////////////////////////
void spi_dma_struct_init(void)
{
	
	memset(&cfg_read, 0, sizeof(cfg_read));
	
	ch_cfg_read.flow_cntrl = FC_PER2MEM_DMA;
	ch_cfg_read.src_per = PER_SPI1;
	ch_cfg_read.dest_per = 0;
	ch_ctrl_read.di = 1;
	ch_ctrl_read.si = 0;
	ch_ctrl_read.dwidth = WIDTH_BYTE;
	ch_ctrl_read.swidth = WIDTH_BYTE;
	ch_ctrl_read.src_ahb1 = 0;
	ch_ctrl_read.dest_ahb1 = 1;

		
	
	ch_cfg_read.halt = 0;
	ch_cfg_read.active = 1;
	ch_cfg_read.lock = 0;
	ch_cfg_read.itc = 1;
	ch_cfg_read.ie = 1;
	ch_cfg_read.ie = 0;
	ch_ctrl_read.tc_mask = 1;
	ch_ctrl_read.cacheable = 0;
	ch_ctrl_read.bufferable = 0;
	ch_ctrl_read.priv_mode = 1;
	ch_ctrl_read.dbsize = 1;//8; /* 8 */
	ch_ctrl_read.sbsize = 1;//8; /* 8 */
	
	////////////////////////////////////////
	memset(&cfg_write, 0, sizeof(cfg_write));
		

	/* Must be _DMA in master mode - see 11.28.5.6 in PNX67xx manual */
	ch_cfg_write.flow_cntrl = FC_MEM2PER_DMA;
	ch_cfg_write.dest_per = PER_SPI1;
	ch_cfg_write.src_per = 0;
	ch_ctrl_write.di = 0;
	ch_ctrl_write.si = 1;
	ch_ctrl_write.dwidth = WIDTH_BYTE;
	ch_ctrl_write.swidth = WIDTH_BYTE;
	ch_ctrl_write.src_ahb1 = 1;
	ch_ctrl_write.dest_ahb1 = 0;

		
	ch_cfg_write.halt = 0;
	ch_cfg_write.active = 1;
	ch_cfg_write.lock = 0;
	ch_cfg_write.itc = 1;
	ch_cfg_write.ie = 1;
	ch_cfg_write.ie = 0;
	ch_ctrl_write.tc_mask = 1;
	ch_ctrl_write.cacheable = 0;
	ch_ctrl_write.bufferable = 0;
	ch_ctrl_write.priv_mode = 1;
	ch_ctrl_write.dbsize = 1;//8; /* 8 */
	ch_ctrl_write.sbsize = 1;//8; /* 8 */
	
}

int spi_nrx_setup(void)
{
	int ret;
	int factor;
	int timeout;
	volatile u32 control_val;
	printk("[WLAN DEBUG] spi_nrx_setup\n");
	KDEBUG(TRANSPORT, "ENTRY");
	
	sema_init(&dma_sem, 0); /// init the dma semaphore

	ret = request_irq(IRQ_SPI_CONTROLER, spi_interrupt, 0,
			"pnx67xx_spi", &irq_cookie);
	if (ret)
	{
		printk("[%s][%d] Request IRQ failed :%d\n",__func__,__LINE__,IRQ_SPI_CONTROLER);
	}
	
	/* get the clock for the SPI */
	spi_clk = clk_get(NULL, "SPI1");
	/* enable SPI clock */
	clk_enable(spi_clk);
	
	/// try to enable clock !!!
	factor = SPI_BLOCK_CLOCK /SPI_SPEED_HZ; /* rounded inverse of adap->clk/((2* factor)+2) */
	/* just avoiding negative results */
	if(factor>1)
	   factor=(factor-1)/2;
	
	/* limit slowest transfer */
	if (factor>SPI_CON_RATE_MASK)
		factor = SPI_CON_RATE_MASK;
		
	factor &= SPI_CON_RATE_MASK; /* change to allow dynamic clock control in system */

	/* Set SPI controller on and reset it for 1ms */
	write_32( SPI_ADR_OFFSET_GLOBAL, SPI_GLOBAL_BLRES_SPI + SPI_GLOBAL_SPI_ON );
	mdelay( 1 );    // delay 5 millisecond
	write_32( SPI_ADR_OFFSET_GLOBAL, SPI_GLOBAL_SPI_ON );
	
	
	printk(KERN_INFO "Control val from %x\n",(unsigned int) read_32( SPI_ADR_OFFSET_CON ));
	
	/* Set SPI controller to work as master */
	control_val = SPI_CON_MS  | SPI_CON_RATE_13 | SPI_CON_THR | SPI_CON_SPI_BIDIR ;
	write_32( SPI_ADR_OFFSET_CON, control_val );
	
	/* Change the clock */
	control_val = read_32( SPI_ADR_OFFSET_CON ) & ~ SPI_CON_RATE_MASK;
	control_val |= factor;
	write_32( SPI_ADR_OFFSET_CON, control_val );
	
	/* Change the spi mode */
	control_val = read_32( SPI_ADR_OFFSET_CON ) & ~SPI_CON_SPI_MODE_MASK;
	control_val |= ( SPI_MODE_NANO & 3 ) << SPI_CON_MODE_SHIFT;
	write_32( SPI_ADR_OFFSET_CON, control_val );
	
	/* set bitwidth */
	control_val = read_32( SPI_ADR_OFFSET_CON ) & ~SPI_CON_BITNUM_MASK;
	control_val |=(( SPI_BIT_WIDTH - 1 ) << SPI_CON_BITNUM_SHIFT ) & SPI_CON_BITNUM_MASK;
	write_32( SPI_ADR_OFFSET_CON, control_val );

	printk(KERN_INFO "Control val to %x\n",(unsigned int) read_32( SPI_ADR_OFFSET_CON ));
		
	/* change interrupt */
	control_val = read_32(SPI_ADR_OFFSET_IER);
	control_val &=~(SPI_IER_SPI_INTEOT | SPI_IER_SPI_INTTHR);
	control_val |= (1 & (SPI_IER_SPI_INTEOT | SPI_IER_SPI_INTTHR));
	write_32( SPI_ADR_OFFSET_IER, control_val);

	// wait for the interrupt enable to change
   timeout = 10;
	while (control_val != read_32( SPI_ADR_OFFSET_IER)) {
		if (!timeout) {
			KDEBUG(TRANSPORT, "a : error in interrupt enable: %08x\n", control_val);
			return 0;
		}
		timeout--;
		mdelay(10);
		write_32( SPI_ADR_OFFSET_IER, control_val);
	}
	//while (control_val != read_32( SPI_ADR_OFFSET_IER));

	/* change the timer ctrl */
	write_32( SPI_ADR_OFFSET_TIMER_CTRL_REG, SPI_TIMER_CTRL_REG_PIRQE );

	/* DMA Alloc */
	dma_channel = pnx_request_channel("spi_pnx", -1,spi_dma_callback, NULL);
	if((dma_channel == -EINVAL) || (dma_channel == -ENODEV))
	{
		printk(KERN_WARNING "DMA channel allocation failed\n");
		return dma_channel;
	}

	spi_alloc_buff();
	
	spi_dma_struct_init();
	
	/* set the cs to 0 */
	pnx_write_gpio_pin(SPI_CHIP_SELECT, 0);

	/* disable SPI controler clock */
	clk_disable(spi_clk);
	
	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}

void spi_nrx_cleanup(void)
{
	volatile u32 control_val;

	/* enable SPI clock */
	clk_enable(spi_clk);

	control_val = read_32(SPI_ADR_OFFSET_IER);
	control_val &=~(SPI_IER_SPI_INTEOT | SPI_IER_SPI_INTTHR);
	write_32( SPI_ADR_OFFSET_IER, control_val);
	
	pnx_free_channel(dma_channel);
	spi_free_buff();

	free_irq(IRQ_SPI_CONTROLER, &irq_cookie);

	/* disable SPI controler clock */
	clk_disable(spi_clk);
}
