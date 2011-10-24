
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <asm/io.h>
#include <mach/dma.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>
#include <asm/cacheflush.h>

#include "spi_private.h"

#if 0
static int hex_dump(char* str, void __iomem* buf, u32 len, u32 mode/*0-8bit, 1-16bit, 2-32bit*/)
{
    u32 i;
    u32 step[3] = {1, 2, 4};
    char* mod[3] = {"%02x ", "%04x ", "%08x "};
    
    printk("\nDump %s:", str);
    for (i=0; i<len; i+=step[mode])             
    {                                       
        if (!(i&0xf))
            printk("\n0x%p : ", buf + i);
        printk(mod[mode], readl(buf + i));  
    }                                       
    printk("\n");
    
    return 0;
}
#endif

#define SYS_SPI_PIN
#ifndef SYS_SPI_PIN
static void* __iomem gpio_addr = NULL;
// gpio base address
#define _PIO_BASE_ADDRESS    (0x01c20800)
//gpio  spi1
#define _Pn_CFG1(n) ( (n)*0x24 + 0x04 + gpio_addr )
#define _Pn_DRV1(n) ( (n)*0x24 + 0x14 + gpio_addr )
#define _Pn_PUL1(n) ( (n)*0x24 + 0x1C + gpio_addr )
#endif

//#define AW1623_FPGA


struct aw16xx_spi {
    struct platform_device *pdev;
	struct spi_master *master;// kzalloc
	
	void __iomem *base_addr; // register 
	struct clk *hclk;  // ahb spi gating bit
	struct clk *mclk;  // ahb spi gating bit
	unsigned long gpio_hdle;

	enum sw_dma_ch dma_id;
	enum sw_dmadir dma_dir;	
	int dma_hdle;	
	
	unsigned int irq; // irq NO.
	
	int busy;
#define SPI_FREE   (1<<0)
#define SPI_SUSPND (1<<1)
#define SPI_BUSY   (1<<2)
	
	int result; // 0: succeed -1:fail

	struct workqueue_struct *workqueue;
	struct work_struct work;

	struct list_head queue; // spi messages
	spinlock_t lock;

	struct completion done;  // wakup another spi transfer
	
	//keep select during one message
	void (*cs_control)(struct spi_device *spi, bool on);

/* 
* 
*   (1) enable cs1,    cs_bitmap = SPI_CHIP_SELECT_CS1;
*   (2) enable cs0&cs1,cs_bitmap = SPI_CHIP_SELECT_CS0|SPI_CHIP_SELECT_CS1; 
*
*/
#define SPI_CHIP_SELECT_CS0 (0x01)
#define SPI_CHIP_SELECT_CS1 (0x02)

	int cs_bitmap;// cs0- 0x1; cs1-0x2, cs0&cs1-0x3.
	
};

/* spi master board para */
struct aw16xx_spi_info {
    int cs_bitmap; // cs0-0x1,cs1-0x2,cs0&cs1-0x3
    int num_cs;   // number of cs
    const char *clk_name; // ahb clk name
};


/* spi device controller state, alloc */
struct aw16xx_spi_config {
	int bits_per_word; // 8bit 
	int max_speed_hz;  // 20MHz
	int mode; // pha,pol,LSB,etc..
};

static int aw16xx_get_cfg_csbitmap(int bus_num);


// flush d-cache
static void aw16xx_spi_cleanflush_dcache_region(void *addr, __u32 len)
{
	__cpuc_flush_dcache_area(addr, len + (1 << 5) * 2 - 2);
}

//------------------------------- dma operation start-----------------------------
//  dma client name
static struct sw_dma_client spi_dma_client[] = {
	[0] = {
	    .name		= "aw16xx-spi0",
	},
	[1] = {
	    .name		= "aw16xx-spi1",
	},
	[2] = {
	    .name		= "aw16xx-spi2",
	},
	[3] = {
	    .name		= "aw16xx-spi3",
	}
};

/*
*    rx dma callback,  disable the 1/4 fifo rx drq.
*    tx dma callback,  disable the tx empty drq.
*   it can do this in the isr,too.
* @ buf: client's id
* @ size:
* @ result: 
*/
static void aw16xx_spi_dma_cb(struct sw_dma_chan *dma_ch, void *buf, int size, enum sw_dma_buffresult result)
{
	struct aw16xx_spi *aw_spi = (struct aw16xx_spi *)buf;
	unsigned long flags;
    
//    spi_msg("aw16xx_spi_dma_cb\n");
	spin_lock_irqsave(&aw_spi->lock, flags);

	if (result != SW_RES_OK) {
	    spi_wrn("aw16xx spi dma callback: fail NO. = %d\n", result);
	    spin_unlock_irqrestore(&aw_spi->lock, flags);
		return;
	}
	if(aw_spi->dma_dir == SW_DMA_RDEV) {
//	    spi_msg("spi dma rx -read data\n");
        aw_spi_disable_dma_irq(SPI_DRQEN_RHF, aw_spi->base_addr);
    }
    else if(aw_spi->dma_dir == SW_DMA_WDEV) {
//        spi_msg("spi dma tx -write data\n");
        aw_spi_disable_dma_irq(SPI_DRQEN_THE, aw_spi->base_addr);
    }
    else {
        spi_wrn("unknow dma direction = %d \n",aw_spi->dma_dir);
    }
    
    aw_spi_sel_dma_type(0, aw_spi->base_addr);
    
	spin_unlock_irqrestore(&aw_spi->lock, flags);
}


/*  config dma src and dst address, 
*   io or linear address, 
*   drq type, 
*   then enqueue 
*   but not trigger dma start
*/
static int aw16xx_spi_config_dma(struct aw16xx_spi *aw_spi, void *buf, unsigned int len)
{     
    int ret = 0;
    int bus_num = aw_spi->master->bus_num;
    unsigned char spi_drq[] = {DRQ_TYPE_SPI0, DRQ_TYPE_SPI1, DRQ_TYPE_SPI2, DRQ_TYPE_SPI3};
    unsigned long spi_phyaddr[] = {SPI0_BASE_ADDR, SPI1_BASE_ADDR, SPI2_BASE_ADDR, SPI3_BASE_ADDR}; // physical address
    struct dma_hw_conf spi_hw_conf = {0};
    
    
	#ifdef CONFIG_SUN4I_SPI_NDMA
    //write
    if (aw_spi->dma_dir == SW_DMA_WDEV) {
	    spi_hw_conf.drqsrc_type  = N_DRQDST_SDRAM;               // must be sdram,or sdram
	    spi_hw_conf.drqdst_type  = spi_drq[bus_num];             // spi drq type
	    spi_hw_conf.dir          = SW_DMA_WDEV;                  //transmit data to device
	    spi_hw_conf.address_type = DMAADDRT_D_IO_S_LN;           //dest io, src linear
        spi_hw_conf.to           = spi_phyaddr[bus_num] + SPI_TXDATA_REG; // physical address, destination address
	}//read 
	else if (aw_spi->dma_dir == SW_DMA_RDEV) {    
	    spi_hw_conf.drqsrc_type  = spi_drq[bus_num];    // spi drq type
	    spi_hw_conf.drqdst_type  = N_DRQSRC_SDRAM;      // must be sdram ?? what about sram ?
	    spi_hw_conf.dir          = SW_DMA_RDEV;         // receive data from device
	    spi_hw_conf.address_type = DMAADDRT_D_LN_S_IO; // dest linear, src io
	    spi_hw_conf.from         = spi_phyaddr[bus_num] + SPI_RXDATA_REG; // physical address, source address
	}
	else {
	    spi_wrn("[aw16xx spi-%d]: unknow dma direction = %d \n",
	                    bus_num, aw_spi->dma_dir);
	    return -1;
	}
	
    #else   //#ifdef CONFIG_SUN4I_SPI_NDMA
    
    //write
    if (aw_spi->dma_dir == SW_DMA_WDEV) {
	    spi_hw_conf.drqsrc_type  = D_DRQSRC_SDRAM;               // must be sdram,or sdram
	    spi_hw_conf.drqdst_type  = spi_drq[bus_num];             // spi drq type
	    spi_hw_conf.dir          = SW_DMA_WDEV;                  //transmit data to device
	    spi_hw_conf.address_type = DMAADDRT_D_IO_S_LN;           //dest io, src linear
        spi_hw_conf.to           = spi_phyaddr[bus_num] + SPI_TXDATA_REG; // physical address, destination address
	}//read 
	else if (aw_spi->dma_dir == SW_DMA_RDEV) {    
	    spi_hw_conf.drqsrc_type  = spi_drq[bus_num];    // spi drq type
	    spi_hw_conf.drqdst_type  = D_DRQDST_SDRAM;      // must be sdram ?? what about sram ?
	    spi_hw_conf.dir          = SW_DMA_RDEV;         // receive data from device
	    spi_hw_conf.address_type = DMAADDRT_D_LN_S_IO; // dest linear, src io
	    spi_hw_conf.from         = spi_phyaddr[bus_num] + SPI_RXDATA_REG; // physical address, source address
	}
	else {
	    spi_wrn("[aw16xx spi-%d]: unknow dma direction = %d \n",
	                    bus_num, aw_spi->dma_dir);
	    return -1;
	}
    
    spi_hw_conf.cmbk        = 0x07070707;
    #endif //#ifdef CONFIG_SUN4I_SPI_NDMA
    
    spi_hw_conf.xfer_type   = DMAXFER_D_SWORD_S_SWORD;	
    spi_hw_conf.hf_irq      = SW_DMA_IRQ_FULL;
    
    // set src,dst, drq type,configuration
    ret = sw_dma_config(aw_spi->dma_id, &spi_hw_conf);
    
    // flush the transfer queue
    ret += sw_dma_ctrl(aw_spi->dma_id, SW_DMAOP_FLUSH);
    
    // 1. flush d-cache
    aw16xx_spi_cleanflush_dcache_region((void *)buf, len);
    
    // 2. enqueue dma transfer, --FIXME--: buf: virtual address, not physical address.    
    ret += sw_dma_enqueue(aw_spi->dma_id, (void *)aw_spi, (dma_addr_t)buf, len);
    
    return ret;
}

  
/*
*   set dma start flag, if queue, it will auto restart to transfer next queue
*/
static int aw16xx_spi_start_dma(struct aw16xx_spi *aw_spi)
{
    int ret = 0;
    /*  change the state of the dma channel, dma start */
    ret = sw_dma_ctrl(aw_spi->dma_id, SW_DMAOP_START);
    /*  set the channel's flags to a given state */
    ret += sw_dma_setflags(aw_spi->dma_id, SW_DMAF_AUTOSTART);  

    return ret;
}

/*
*   request dma channel and set callback function
*/
static int aw16xx_spi_prepare_dma(struct aw16xx_spi *aw_spi)
{
    int ret = 0;
    int bus_num   = aw_spi->master->bus_num;
    
    aw_spi->dma_hdle = sw_dma_request(aw_spi->dma_id, &spi_dma_client[bus_num], NULL);
    if(aw_spi->dma_hdle < 0) {
        spi_wrn("aw16xx spi request dma failed!\n");
        return aw_spi->dma_hdle;
    }
    
//    spi_msg("[spi-%d] request dma handle = %d, dmaid %d\n", bus_num, aw_spi->dma_hdle, aw_spi->dma_id);
    /* 
      * no need to distinguish rx and tx callback
      */
	if(aw_spi->dma_dir == SW_DMA_RDEV || aw_spi->dma_dir == SW_DMA_WDEV) {	
    	ret = sw_dma_set_buffdone_fn(aw_spi->dma_hdle, aw16xx_spi_dma_cb);
	}
	else {
		spi_wrn("[spi-%d]: unknow dma direction = %d \n", bus_num, aw_spi->dma_dir);
	    return -1;
	}
    // make sure the queue safe
	ret += sw_dma_setflags(aw_spi->dma_id, 0); // set flag ??? dma run status, mainly usd

    return ret;
}


/* 
*   release dma channel, and set queue status to idle.
*/
static int aw16xx_spi_release_dma(struct aw16xx_spi *aw_spi)
{
    int ret = 0;
    ret  = sw_dma_ctrl(aw_spi->dma_id, SW_DMAOP_STOP); /* first stop */
    ret += sw_dma_setflags(aw_spi->dma_id, 0);
    ret += sw_dma_free(aw_spi->dma_id, &spi_dma_client[aw_spi->master->bus_num]);    

//    spi_msg("[spi-%d] release dma,ret = %d \n", aw_spi->master->bus_num, ret);

    aw_spi->dma_hdle = -1;
    aw_spi->dma_dir  = SW_DMA_RWNULL;

    return ret;
}
// ------------------------------dma operation end-----------------------------



/*
*  check the valid of cs id
*/
static int aw16xx_spi_check_cs(int cs_id, struct aw16xx_spi *aw_spi)
{
    int ret = AW_SPI_FAIL;
    
    switch(cs_id)
    {
        case 0:
            ret = (aw_spi->cs_bitmap & SPI_CHIP_SELECT_CS0) ? AW_SPI_OK : AW_SPI_FAIL;            
            break;
            
        case 1:
            ret = (aw_spi->cs_bitmap & SPI_CHIP_SELECT_CS1) ? AW_SPI_OK : AW_SPI_FAIL;
            break;
            
        default:
            spi_wrn("chip select not support! cs = %d \n", cs_id);
            break;
    }
    return ret;
}

/*
*   spi device on or off control
*/
static void aw16xx_spi_cs_control(struct spi_device *spi, bool on)
{
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(spi->master);
	unsigned int cs = 0;
	
	if (aw_spi->cs_control) {
		if(on) {					
			// set active
			cs = (spi->mode & SPI_CS_HIGH) ? 1 : 0;			
		}	
		else {	
			//set inactive					
			cs = (spi->mode & SPI_CS_HIGH) ? 0 : 1;	
		}		
		aw_spi_ss_level(aw_spi->base_addr, cs);
	}		
}

/* 
*   change the properties of spi device with spi transfer.
*   every spi transfer must call this interface to update the master to the excute transfer
*   set clock frequecy, bits per word, mode etc...
*   return:  >= 0 : succeed;    < 0: failed.
*/
static int aw16xx_spi_xfer_setup(struct spi_device *spi, struct spi_transfer *t)
{
	// get at the setup function, the properties of spi device
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(spi->master);
	struct aw16xx_spi_config *config = spi->controller_data; //allocate in the setup,and free in the cleanup
    void *__iomem base_addr = aw_spi->base_addr;
    
//    spi_msg("aw16xx_spi_xfer_setup\n");
    
	config->max_speed_hz  = (t && t->speed_hz) ? t->speed_hz : spi->max_speed_hz;
	config->bits_per_word = (t && t->bits_per_word) ? t->bits_per_word : spi->bits_per_word; 	
	config->bits_per_word = ((config->bits_per_word + 7) / 8) * 8;

	if(config->bits_per_word != 8) {
	    spi_wrn("aw16xx-spi just support 8bits per word... \n");
	    return -EINVAL;
	}

	if(spi->chip_select >= spi->master->num_chipselect) {
	    spi_wrn("[aw16xx-spi%d]: spi device's chip select = %d exceeds the master supported cs_num[%d] \n",
	                    spi->master->bus_num, spi->chip_select, spi->master->num_chipselect);
	    return -EINVAL;
	}	
	/* check again board info */
	if( AW_SPI_OK != aw16xx_spi_check_cs(spi->chip_select, aw_spi) ) {
	    spi_wrn("aw16xx_spi_check_cs failed! spi_device cs =%d ...\n", spi->master->num_chipselect);
	    return -EINVAL;
	}
	/* set cs */
	aw_spi_set_cs(spi->chip_select, base_addr);
    /* 
     *  master: set spi module clock; 
     *  set the default frequency	10MHz
     */
    aw_spi_set_master(base_addr); 

   	if(config->max_speed_hz > SPI_MAX_FREQUENCY) {
	    return -EINVAL;
	} 
    aw_spi_set_clk(config->max_speed_hz, clk_get_rate(aw_spi->mclk), base_addr);
    
    /* 
     *  master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     *  set bit width-default: 8 bits
     */
    aw_spi_config(1, spi->mode, base_addr);  
	
	return 0;	
}


/* > 64 : cpu ;  =< 64 : dma
*  wait for done completion in this function, wakup in the irq hanlder
*/
static int aw16xx_spi_xfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(spi->master);
	void __iomem* base_addr = aw_spi->base_addr; 
	unsigned long flags = 0;
	unsigned tx_len = t->len;	/* number of bytes receieved */
	unsigned rx_len = t->len;	/* number of bytes sent */
	unsigned char *rx_buf = (unsigned char *)t->rx_buf;
	unsigned char *tx_buf = (unsigned char *)t->tx_buf;
	int ret = 0;
    
//    spi_msg("Begin transfer, txbuf %p, rxbuf %p, len %d\n", t->tx_buf, t->rx_buf, t->len);
    
	if (!t->tx_buf && !t->rx_buf && t->len)
		return -EINVAL;
    // check in the __spi_async if use half duplex
	if(t->tx_buf && t->rx_buf)
	    return -EINVAL;
	
//	hex_dump("spi_regs:", base_addr, 0x60, 2);
	
    // write 1 to clear 0
    aw_spi_clr_irq_pending(SPI_STAT_MASK, base_addr);
    //disable all DRQ
    aw_spi_disable_dma_irq(SPI_DRQEN_MASK, base_addr);
    aw_spi_sel_dma_type(0, base_addr);
    //reset tx/rx fifo
    aw_spi_reset_fifo(base_addr);    
   
    //set the Burst Counter and Write Transmit Counter,auto put dummy data into the txFIFO!
    if(tx_buf){
        aw_spi_set_bc_wtc(tx_len, 0, base_addr);
    }
    else if(rx_buf) {
        aw_spi_set_bc_wtc(0, rx_len, base_addr);
    }
    // 1. Tx/Rx error irq,process in IRQ; 
    // 2. Transfer Complete Interrupt Enable
    aw_spi_enable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);	
	/* read */
	if(rx_buf) {
		if(rx_len > BULK_DATA_BOUNDARY) { /* dma */
//		    spi_msg(" rx -> by dma\n");
            #ifdef CONFIG_SUN4I_SPI_NDMA
            aw_spi_sel_dma_type(0, base_addr);
            #else
            aw_spi_sel_dma_type(1, base_addr);
            #endif
            
    		 /* rxFIFO 1/4 full dma request enable,when 16 or more than 16 bytes */
            aw_spi_enable_dma_irq(SPI_DRQEN_RHF, base_addr);
    		aw_spi->dma_dir = SW_DMA_RDEV;
    		
    		ret = aw16xx_spi_prepare_dma(aw_spi);
    		if(ret < 0) {
    			aw_spi_disable_dma_irq(SPI_DRQEN_RHF, base_addr);
    			aw_spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
    			return -EINVAL;
    		}
    		aw16xx_spi_config_dma(aw_spi, (void *)rx_buf, rx_len);
    		aw16xx_spi_start_dma(aw_spi);
//    		hex_dump("spi_regs + 0x8:", base_addr+8, 0x58, 2);
//			hex_dump("dma regs:", (void __iomem*)SW_VA_DMAC_IO_BASE, 0x400, 2);				
		}
		else{
		    unsigned int poll_time = 0x7ffff;  
//		    spi_msg(" rx -> by ahb\n");
			//SMC=1,XCH trigger the transfer
//	        hex_dump("spi_regs + 0x8:", base_addr+8, 0x58, 2);
		    aw_spi_start_xfer(base_addr);			  		
    	    while(rx_len && (--poll_time >0)) {
    	    	//rxFIFO counter    	        
    	        if(aw_spi_query_rxfifo(base_addr)){
    	            *rx_buf++ =  readb(base_addr + SPI_RXDATA_REG);//fetch data
    	            --rx_len;
    	            poll_time = 0xffff;
    	        }
    	    }
    	    if(poll_time <= 0) {
    	        spi_wrn("cpu receive data timeout!\n");
    	    }
    	    goto out; 
		}			
	}
	
//	hex_dump("spi_regs + 0x8:", base_addr+8, 0x58, 2);
	/* SMC=1,XCH trigger the transfer */
    aw_spi_start_xfer(base_addr);	
	/* write */
	if(tx_buf){
		if(t->len <= BULK_DATA_BOUNDARY) {
		  unsigned int poll_time = 0xfffff;			
//		  spi_msg(" tx -> by ahb\n");		
          spin_lock_irqsave(&aw_spi->lock, flags);
          for(; tx_len > 0; --tx_len) {
             writeb(*tx_buf++, base_addr + SPI_TXDATA_REG);
          }            
          spin_unlock_irqrestore(&aw_spi->lock, flags);
          while(aw_spi_query_txfifo(base_addr)&&(--poll_time > 0) );/* txFIFO counter */
          if(poll_time <= 0) {
               spi_wrn("cpu tx data time out!\n");
          }             
		}
		else {
//		    spi_msg(" tx -> by dma\n");	
            #ifdef CONFIG_SUN4I_SPI_NDMA
            aw_spi_sel_dma_type(0, base_addr);
            #else
            aw_spi_sel_dma_type(1, base_addr);
            #endif	
			/* txFIFO 1/4 empty dma request enable,when 16 or less than 16 bytes. */
            aw_spi_enable_dma_irq(SPI_DRQEN_THE, base_addr);
			aw_spi->dma_dir = SW_DMA_WDEV;
			ret = aw16xx_spi_prepare_dma(aw_spi);			
			if(ret < 0) {
			    aw_spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
			    aw_spi_disable_dma_irq(SPI_DRQEN_THE, base_addr);
				return -EINVAL;
			}
			aw16xx_spi_config_dma(aw_spi, (void *)tx_buf, t->len);
			aw16xx_spi_start_dma(aw_spi);
//			hex_dump("spi_regs + 0x8:", base_addr+8, 0x58, 2);
//			hex_dump("dma regs:", (void __iomem*)SW_VA_DMAC_IO_BASE, 0x400, 2);
		}		
	}	
			
out:
	/*  wait for xfer complete in the isr.	*/
	wait_for_completion(&aw_spi->done);	
    /* get the isr return code	*/
    if(aw_spi->result != 0) {
        spi_wrn("aw16xx spi xfer failed... \n");
        ret = -1;
    }
    // release dma resource if neccessary
    if(aw_spi->dma_dir != SW_DMA_RWNULL) {
        aw16xx_spi_release_dma(aw_spi);
    }
    
	return ret;	
}

/*
*   spi core xfer process
*/
static void aw16xx_spi_work(struct work_struct *work)
{
	struct aw16xx_spi *aw_spi = container_of(work, struct aw16xx_spi, work);
	spin_lock_irq(&aw_spi->lock);
	aw_spi->busy = SPI_BUSY;
	
	/* get from messages queue, and then do with them,
	 *  if message queue is empty ,then return and set status to free,
	 *  otherwise process them.
	 */
	while (!list_empty(&aw_spi->queue)) {
		struct spi_message *msg = NULL;
		struct spi_device  *spi = NULL;
		struct spi_transfer *t  = NULL;
		unsigned int cs_change = 0;
		int status;
		/* get message from message queue in aw16xx_spi. */
		msg = container_of(aw_spi->queue.next, struct spi_message, queue);
		/* then delete from the message queue,now it is alone.*/
		list_del_init(&msg->queue);
		spin_unlock_irq(&aw_spi->lock);
		/* get spi device from this message */
		spi = msg->spi;
		/* set default value,no need to change cs,keep select until spi transfer require to change cs. */
		cs_change = 1;
		/* set message status to succeed. */
		status = 0;
		/* search the spi transfer in this message, deal with it alone. */
		list_for_each_entry (t, &msg->transfers, transfer_list) {
			if (t->bits_per_word || t->speed_hz) { /* if spi transfer is zero,use spi device value. */
				status = aw16xx_spi_xfer_setup(spi, t);/* set the value every spi transfer */
				spi_msg(" xfer setup \n");
				if (status < 0)
					break;/* fail, quit */
			}
			/* first active the cs */
			if (cs_change) {
				aw_spi->cs_control(spi, 1);
			}
			/* update the new cs value */
			cs_change = t->cs_change;
			/* do transfer
			* > 64 : cpu ;  =< 64 : dma
			* wait for done completion in this function, wakup in the irq hanlder
			*/
			status = aw16xx_spi_xfer(spi, t);
			if (status)
				break;/* fail quit, zero means succeed */
			/* accmulate the value in the message */
			msg->actual_length += t->len;
			/* may be need to delay */
			if (t->delay_usecs)
				udelay(t->delay_usecs);
			/* if zero ,keep active,otherwise deactived. */
			if (cs_change) {
				aw_spi->cs_control(spi, 0);
			}			
		}
		/* 
		 * spi message complete,succeed or failed 	
		 * return value 
		 */
		msg->status = status;
		/* wakup the uplayer caller,complete one message */
		msg->complete(msg->context);
		// fail or need to change cs
		if (status || !cs_change) {
			aw_spi->cs_control(spi, 0);
		}
	  /* restore default value. */
		aw16xx_spi_xfer_setup(spi, NULL);
		spin_lock_irq(&aw_spi->lock);
	}
	/* set spi to free */
	aw_spi->busy = SPI_FREE;
	spin_unlock_irq(&aw_spi->lock);
	return;	
}


/*
*  wake up the sleep thread, and give the result code
*/
static irqreturn_t aw16xx_spi_isr(int irq, void *dev_id)
{
	struct aw16xx_spi *aw_spi = (struct aw16xx_spi *)dev_id;
	void *base_addr = aw_spi->base_addr;
    unsigned int status = aw_spi_qry_irq_pending(base_addr);
    aw_spi_clr_irq_pending(status, base_addr);//write 1 to clear 0.
        
//    spi_msg("status = %x \n", status);

    aw_spi->result = 0; // assume succeed
    
    //master mode, Transfer Complete Interrupt
    if( status & SPI_STAT_TC ) {        
        aw_spi_disable_irq(SPI_STAT_TC|SPI_STAT_ERR, base_addr);    
        /*
          * just check dma+callback receive,skip other condition.
          * dma+callback receive: when TC comes,dma may be still not complete fetch data from rxFIFO.
          * other receive: cpu or dma+poll,just skip this.
          */          
        if(aw_spi->dma_dir == SW_DMA_RDEV) {
            unsigned int poll_time = 0xffff;
            //during poll,dma maybe complete rx,rx_dma_used is 0. then return.
            while(aw_spi_query_rxfifo(base_addr)&&(--poll_time > 0));                
            if(poll_time <= 0) {
                spi_wrn("dma callback method,rx data time out in irq !\n");
                aw_spi->result = -1;// failed
                complete(&aw_spi->done);
                return AW_SPI_FAIL;
            }
            else {
//                spi_msg("rx: irq comes first,dma last. wait = 0x%x\n", poll_time);
            }
        }         
//        spi_msg("SPI TC comes\n");
        complete(&aw_spi->done);/*wakup uplayer, by the sem */
        return IRQ_HANDLED;            
    }/* master mode:err */
    else if( status & SPI_STAT_ERR ) {
        // error process        
        // release dma in the workqueue,should not be here
        aw_spi_disable_irq(SPI_STAT_TC|SPI_STAT_ERR, base_addr);        
        aw_spi_restore_state(1, base_addr);
        aw_spi->result = -1;
        complete(&aw_spi->done);
//        spi_msg("SPI ERR comes\n");
        spi_wrn("master mode error: txFIFO overflow/rxFIFO underrun or overflow\n");
        return IRQ_HANDLED;
    }    
//    spi_msg("SPI NONE comes\n");

	return IRQ_NONE;
}

// interface 1
static int aw16xx_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(spi->master);
	unsigned long flags;

	msg->actual_length = 0;
	msg->status = -EINPROGRESS;

	spin_lock_irqsave(&aw_spi->lock, flags);
	list_add_tail(&msg->queue, &aw_spi->queue); // add msg to the aw16xx_spi queue
	queue_work(aw_spi->workqueue, &aw_spi->work); // add work to the workqueue,schedule the cpu.
	spin_unlock_irqrestore(&aw_spi->lock, flags);

	return 0; // return immediately and wait for completion in the uplayer caller.
}

// interface 2, setup the frequency and default status
static int aw16xx_spi_setup(struct spi_device *spi)
{
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(spi->master);
	struct aw16xx_spi_config *config = spi->controller_data;// general is null.
	unsigned long flags;

    // just support 8 bits per word
	if (spi->bits_per_word != 8)
		return -EINVAL;

    // first check its valid,then set it as default select,finally set its
    if(AW_SPI_FAIL == aw16xx_spi_check_cs(spi->chip_select, aw_spi)) {
        spi_wrn("[aw16xx-spi%d] not support cs-%d \n", aw_spi->master->bus_num, spi->chip_select);
        return -EINVAL;
    }	    
   	if(spi->max_speed_hz > SPI_MAX_FREQUENCY) {
	    return -EINVAL;
	} 
    // check the functionality
/*	if(spi->mode & ~(spi->master->mode_bits)) {
	
	}
*/	
	if (!config) {
		config = kzalloc(sizeof *config, GFP_KERNEL);
		if (!config)
			return -ENOMEM;
		spi->controller_data = config;
	}
    /*
     *  set the default vaule with spi device
     *  can change by every spi transfer
     */
	config->bits_per_word = spi->bits_per_word;
	config->max_speed_hz  = spi->max_speed_hz;
	config->mode		  = spi->mode;

	spin_lock_irqsave(&aw_spi->lock, flags);

	// if aw16xx spi is free, then deactived the spi device
	if (aw_spi->busy & SPI_FREE) {		
		// set chip select number
	    aw_spi_set_cs(spi->chip_select, aw_spi->base_addr);	
	    //deactivate chip select
		aw_spi->cs_control(spi, 0);
	}	
	spin_unlock_irqrestore(&aw_spi->lock, flags);
	
	return 0;
}

// interface 3
static void aw16xx_spi_cleanup(struct spi_device *spi)
{
    if(spi->controller_data) {  
        kfree(spi->controller_data);
        spi->controller_data = NULL;
    }
}

static int aw16xx_spi_set_gpio(struct aw16xx_spi *aw_spi, bool on)
{
    if(on) {
        if(aw_spi->master->bus_num == 0) {
            aw_spi->gpio_hdle = gpio_request_ex("spi0_para", NULL);
            if(!aw_spi->gpio_hdle) {
                spi_wrn("spi0 request gpio fail!\n");
                return -1;
            }
            
//            hex_dump("gpio regs:", (void __iomem*)SW_VA_PORTC_IO_BASE, 0x200, 2);     
        }
        else if(aw_spi->master->bus_num == 1) {

			/*
			*   PI8		SPI1_CS0
			*   PI9		SPI1_CS1
			*   PI10    	SPI1_CLK
			*   PI11    	SPI1_MOSI
			*   PI12	    SPI1_MISO
			*/       
            #ifndef SYS_SPI_PIN      
		    unsigned int  reg_val = readl(_Pn_CFG1(8));		    
			// set spi function
		    reg_val &= ~0x77777;
		    reg_val |=  0x22222;
		    writel(reg_val, _Pn_CFG1(8));		               
            // set pull up
    		reg_val = readl(_Pn_PUL1(8));
    		reg_val &= ~(0x3ff<<16);
    		reg_val |=  (0x155<<16);
    		writel(reg_val, _Pn_PUL1(8));    		
    		// no need to set driver,default is driver 1.
    		#else
            aw_spi->gpio_hdle = gpio_request_ex("spi1_para", NULL);
            if(!aw_spi->gpio_hdle) {
                spi_wrn("spi1 request gpio fail!\n");
                return -1;
            }      		
    		#endif
        }
        else if(aw_spi->master->bus_num == 2) {
            aw_spi->gpio_hdle = gpio_request_ex("spi2_para", NULL);
            if(!aw_spi->gpio_hdle) {
                spi_wrn("spi2 request gpio fail!\n");
                return -1;
            }          
        }
        
        #ifdef AW1623_FPGA
        {
            #include <mach/platform.h>
            void __iomem* pi_cfg0 = (void __iomem*)(SW_VA_PORTC_IO_BASE+0x48);
            u32 rval = readl(pi_cfg0) & (~(0x70777));
            writel(rval|(0x30333), pi_cfg0);
        }
        #endif
    }
    else {
        if(aw_spi->master->bus_num == 0) {
            gpio_release(aw_spi->gpio_hdle, 0);
        }
        else if(aw_spi->master->bus_num == 1) {    
        	#ifndef SYS_SPI_PIN
		    unsigned int  reg_val = readl(_Pn_CFG1(8));		    
			// set default
		    reg_val &= ~0x77777;
		    writel(reg_val, _Pn_CFG1(8)); 	
            #else            
		    gpio_release(aw_spi->gpio_hdle, 0);
		    #endif
        }
        else if(aw_spi->master->bus_num == 2) {
            gpio_release(aw_spi->gpio_hdle, 0);        
        }
    }
    
	return 0;
}

static int aw16xx_spi_set_mclk(struct aw16xx_spi *aw_spi, u32 mod_clk)
{
    struct clk *source_clock = NULL;
    char* name = NULL;
    u32 source = 1;
    int ret = 0;
    
    switch (source)
    {
        case 0:
            source_clock = clk_get(NULL, "hosc");
            name = "hosc";
            break;
        case 1:
            source_clock = clk_get(NULL, "sdram_pll_p");
            name = "sdram_pll_p";
            break;
        case 2:
            source_clock = clk_get(NULL, "sata_pll");
            name = "sata_pll";
            break;
        default:
            return -1;
    }
    
    if (IS_ERR(source_clock)) 
	{
		ret = PTR_ERR(source_clock);
		spi_wrn("Unable to get spi source clock resource\n");
		return -1;
	}
    
    if (clk_set_parent(aw_spi->mclk, source_clock))
    {
        spi_wrn("clk_set_parent failed\n");
        ret = -1;
        goto out;
    }
    
    if (clk_set_rate(aw_spi->mclk, mod_clk))
    {
        spi_wrn("clk_set_rate failed\n");
        ret = -1;
        goto out;
    }
    
    spi_msg("source = %s, src_clk = %u, mclk %u\n", name, (unsigned)clk_get_rate(source_clock), (unsigned)clk_get_rate(aw_spi->mclk));
    
	if (clk_enable(aw_spi->mclk)) {
		spi_wrn("Couldn't enable module clock 'spi'\n");
		ret = -EBUSY;
		goto out;
	}
	
	ret = 0;
	
out:
    clk_put(source_clock);	
    
    return ret;
}

static int aw16xx_spi_hw_init(struct aw16xx_spi *aw_spi)
{	
	void *base_addr = aw_spi->base_addr;
	unsigned long sclk_freq = 0;
	char* mclk_name[] = {"spi0", "spi1", "spi2", "spi3"};
    
    aw_spi->mclk = clk_get(&aw_spi->pdev->dev, mclk_name[aw_spi->pdev->id]);
	if (IS_ERR(aw_spi->mclk)) {
		spi_wrn("Unable to acquire module clock 'spi'\n");
		return -1;
	}
	
    if (aw16xx_spi_set_mclk(aw_spi, 100000000))
    {
        spi_wrn("aw16xx_spi_set_mclk 'spi'\n");
		clk_put(aw_spi->mclk);
		return -1;
    }
    
//    hex_dump("ccmu regs:", (void __iomem*)SW_VA_CCM_IO_BASE, 0x200, 2);    
     
	// 1. enable the spi module
	aw_spi_enable_bus(base_addr);

	// 2. set the default chip select
	if(AW_SPI_OK == aw16xx_spi_check_cs(0, aw_spi)) {
	    aw_spi_set_cs(0, base_addr);
	}
	else{
        aw_spi_set_cs(1, base_addr);
	}		
    /* 
     * 3. master: set spi module clock; 
     * 4. set the default frequency	10MHz
     */
    aw_spi_set_master(base_addr); 
    sclk_freq  = clk_get_rate(aw_spi->mclk);
    aw_spi_set_clk(10000000, sclk_freq, base_addr);
    /* 
     * 5. master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     * 6. set bit width-default: 8 bits
     */
    aw_spi_config(1, SPI_MODE_3, base_addr); 	
	// 7. manual control the chip select
	aw_spi_ss_ctrl(base_addr, 1);
	
	return 0;
}

static int aw16xx_spi_hw_exit(struct aw16xx_spi *aw_spi)
{
	// disable the spi controller 
    aw_spi_disable_bus(aw_spi->base_addr);
	
	//disable module clock
    clk_disable(aw_spi->mclk);
    clk_put(aw_spi->mclk);
	
	return 0;
}


static int __init aw16xx_spi_probe(struct platform_device *pdev)
{
	struct resource	*mem_res, *dma_res;
	struct aw16xx_spi *aw_spi;
	struct aw16xx_spi_info *pdata;
	struct spi_master *master;
	int ret = 0, err = 0, irq;
	int cs_bitmap = 0;

	if (pdev->id < 0) {
		spi_wrn("Invalid platform device id-%d\n", pdev->id);
		return -ENODEV;
	}

	if (pdev->dev.platform_data == NULL) {
		spi_wrn("platform_data missing!\n");
		return -ENODEV;
	}

	pdata = pdev->dev.platform_data;
	if (!pdata->clk_name) {
		spi_wrn("platform data must initial! \n");
		return -EINVAL;
	}

	/* Check for availability of necessary resource */
    #ifdef CONFIG_SUN4I_SPI_NDMA   //normal dma
	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (dma_res == NULL) {
		spi_wrn("Unable to get spi DMA resource\n");
		return -ENXIO;
	}
    #else
	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (dma_res == NULL) {
		spi_wrn("Unable to get spi DMA resource\n");
		return -ENXIO;
	}
    #endif
    
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (mem_res == NULL) {
        	spi_wrn("Unable to get spi MEM resource\n");
        	return -ENXIO;
        }

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		spi_wrn("No spi IRQ specified\n");
		return -ENXIO;
	}
	
    /* create spi master */
	master = spi_alloc_master(&pdev->dev, sizeof(struct aw16xx_spi));
	if (master == NULL) {
		spi_wrn("Unable to allocate SPI Master\n");
		return -ENOMEM;
	}
    
	platform_set_drvdata(pdev, master);
	aw_spi = spi_master_get_devdata(master);	
    memset(aw_spi, 0, sizeof(struct aw16xx_spi));
	
	aw_spi->master     = master;
	aw_spi->irq        = irq;
	aw_spi->dma_id     = dma_res->start;
    aw_spi->dma_hdle   = -1;
    aw_spi->dma_dir    = SW_DMA_RWNULL;
	aw_spi->cs_control = aw16xx_spi_cs_control;
	aw_spi->cs_bitmap  = pdata->cs_bitmap; // cs0-0x1; cs1-0x2; cs0&cs1-0x3.
	aw_spi->busy       = SPI_FREE;

	master->bus_num         = pdev->id;
	master->setup           = aw16xx_spi_setup;
	master->cleanup         = aw16xx_spi_cleanup;
	master->transfer        = aw16xx_spi_transfer;
	master->num_chipselect  = pdata->num_cs;
//	master->dma_alignment   = 8; //  should be set to 32  ??
//  master->flags           = SPI_MASTER_HALF_DUPLEX; // temporay not support duplex
	/* the spi->mode bits understood by this driver: */
	master->mode_bits       = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH| SPI_LSB_FIRST;

    /* update the cs bitmap */
    cs_bitmap = aw16xx_get_cfg_csbitmap(pdev->id);
	if(cs_bitmap & 0x3){
	    aw_spi->cs_bitmap  = cs_bitmap&0x3;
//	    spi_msg("cs_bitmap = 0x%x \n", cs_bitmap);
	}
	
	err = request_irq(aw_spi->irq, aw16xx_spi_isr, IRQF_DISABLED, pdev->name, aw_spi);
	if (err) {
		spi_wrn("Cannot claim IRQ\n");
		goto err0;
	}

	if (request_mem_region(mem_res->start,
			resource_size(mem_res), pdev->name) == NULL) {
		spi_wrn("Req mem region failed\n");
		ret = -ENXIO;
		goto err1;
	}

	aw_spi->base_addr = ioremap(mem_res->start, resource_size(mem_res));
	if (aw_spi->base_addr == NULL) {
		spi_wrn("Unable to remap IO\n");
		ret = -ENXIO;
		goto err2;
	}

	/* Setup clocks */
	aw_spi->hclk = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(aw_spi->hclk)) {
		spi_wrn("Unable to acquire clock 'spi'\n");
		ret = PTR_ERR(aw_spi->hclk);
		goto err3;
	}

	if (clk_enable(aw_spi->hclk)) {
		spi_wrn("Couldn't enable clock 'spi'\n");
		ret = -EBUSY;
		goto err4;
	}
	
	aw_spi->workqueue = create_singlethread_workqueue(dev_name(master->dev.parent));
	if (aw_spi->workqueue == NULL) {
		spi_wrn("Unable to create workqueue\n");
		ret = -ENOMEM;
		goto err5;
	}
	
    aw_spi->pdev = pdev;
    
	/* Setup Deufult Mode */	
	aw16xx_spi_hw_init(aw_spi);
#ifndef SYS_SPI_PIN	
	/* set gpio */
	gpio_addr = ioremap(_PIO_BASE_ADDRESS, 0x1000);
#endif	
	aw16xx_spi_set_gpio(aw_spi, 1);

	spin_lock_init(&aw_spi->lock);
	init_completion(&aw_spi->done);
	INIT_WORK(&aw_spi->work, aw16xx_spi_work);// banding the process handler
	INIT_LIST_HEAD(&aw_spi->queue);

	if (spi_register_master(master)) {
		spi_wrn("cannot register SPI master\n");
		ret = -EBUSY;
		goto err6;
	}

	spi_msg("allwinners SoC SPI Driver loaded for Bus SPI-%d with %d Slaves attached\n", pdev->id, master->num_chipselect);
	//spi_msg("\tIOmem=[0x%x-0x%x]\tDMA=[%d]\n", mem_res->end, mem_res->start, aw_spi->dma_id);
	spi_msg("spi-%d driver probe succeed, base %p, irq %d, dma_id %d!\n", master->bus_num, aw_spi->base_addr, aw_spi->irq, aw_spi->dma_id);				

	return 0;

err6:
	destroy_workqueue(aw_spi->workqueue);
err5:
	clk_disable(aw_spi->hclk);
err4:
	clk_put(aw_spi->hclk);
err3:
	iounmap((void *)aw_spi->base_addr);
err2:
	release_mem_region(mem_res->start, resource_size(mem_res));
err1:
	free_irq(aw_spi->irq, aw_spi);
err0:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);
	
	return ret;
}


static int aw16xx_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(master);
	struct resource	*mem_res;
	unsigned long flags;

	spin_lock_irqsave(&aw_spi->lock, flags);
	aw_spi->busy |= SPI_FREE;
	spin_unlock_irqrestore(&aw_spi->lock, flags);

	while (aw_spi->busy & SPI_BUSY)
		msleep(10);

	aw16xx_spi_hw_exit(aw_spi);

	spi_unregister_master(master);

	destroy_workqueue(aw_spi->workqueue);

	clk_disable(aw_spi->hclk);
	clk_put(aw_spi->hclk);
	
	iounmap((void *) aw_spi->base_addr);

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res != NULL)
		release_mem_region(mem_res->start, resource_size(mem_res));

	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return 0;
}



#ifdef CONFIG_PM
static int ax16xx_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct aw16xx_spi *aw_spi = spi_master_get_devdata(master);
	unsigned long flags;

	printk("[SPI%d] suspend okay.. \n", master->bus_num);

	spin_lock_irqsave(&aw_spi->lock, flags);
	aw_spi->busy |= SPI_SUSPND;
	spin_unlock_irqrestore(&aw_spi->lock, flags);

	while (aw_spi->busy & SPI_BUSY)
		msleep(10);

	/* Disable the clock */
	clk_disable(aw_spi->hclk);

	/* not need to release pin */
	
	return 0;
}

static int ax16xx_spi_resume(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct aw16xx_spi  *aw_spi = spi_master_get_devdata(master);
	unsigned long flags;

    printk("[SPI%d] resume okay.. \n", master->bus_num);
    
	/* no need to set gpio	*/
    
	/* Enable the clock */
	clk_enable(aw_spi->hclk);

	aw16xx_spi_hw_init(aw_spi);

	spin_lock_irqsave(&aw_spi->lock, flags);
	aw_spi->busy = SPI_FREE;
	spin_unlock_irqrestore(&aw_spi->lock, flags);

	return 0;
}
#else
#define ax16xx_spi_suspend	NULL
#define ax16xx_spi_resume	NULL
#endif /* CONFIG_PM */

static struct platform_driver aw16xx_spi_driver = {
	.driver = {
		.name	= "aw16xx-spi",
		.owner = THIS_MODULE,
	},
	.probe   = aw16xx_spi_probe,
	.remove  = aw16xx_spi_remove,
	.suspend = ax16xx_spi_suspend,
	.resume  = ax16xx_spi_resume,
};
MODULE_ALIAS("platform:aw16xx-spi");


//---------------- spi resouce and platform data start ---------------

struct aw16xx_spi_info aw16xx_spi0_pdata = {
	.cs_bitmap  = 0x1,
	.num_cs		= 1,
	.clk_name = "ahb_spi0",
};
static struct resource aw16xx_spi0_resources[] = {
	[0] = {
		.start	= SPI0_BASE_ADDR,
		.end	= SPI0_BASE_ADDR + 1024,
		.flags	= IORESOURCE_MEM,
	},
	#ifdef CONFIG_SUN4I_SPI_NDMA
	[1] = {
		.start	= DMACH_NSPI0,
		.end	= DMACH_NSPI0,
		.flags	= IORESOURCE_DMA,
	},
	#else
	[1] = {
		.start	= DMACH_DSPI0,
		.end	= DMACH_DSPI0,
		.flags	= IORESOURCE_DMA,
	},
	#endif
	[2] = {
		.start	= SW_INT_IRQNO_SPI00,
		.end	= SW_INT_IRQNO_SPI00,
		.flags	= IORESOURCE_IRQ,
	},
};
static struct platform_device aw_spi0 = {
	.name		= "aw16xx-spi",
	.id			= 0,
	.num_resources	= ARRAY_SIZE(aw16xx_spi0_resources),
	.resource	= aw16xx_spi0_resources,
	.dev		= {
		.platform_data = &aw16xx_spi0_pdata,
	},	
};

struct aw16xx_spi_info aw16xx_spi1_pdata = {
	.cs_bitmap	= 0x3,
	.num_cs		= 2,
	.clk_name = "ahb_spi1",
};
static struct resource aw16xx_spi1_resources[] = {
	[0] = {
		.start	= SPI1_BASE_ADDR,
		.end	= SPI1_BASE_ADDR + 1024,
		.flags	= IORESOURCE_MEM,
	},
	#ifdef CONFIG_SUN4I_SPI_NDMA
	[1] = {
		.start	= DMACH_NSPI1,
		.end	= DMACH_NSPI1,
		.flags	= IORESOURCE_DMA,
	},
	#else
	[1] = {
		.start	= DMACH_DSPI1,
		.end	= DMACH_DSPI1,
		.flags	= IORESOURCE_DMA,
	},
	#endif
	[3] = {
		.start	= SW_INT_IRQNO_SPI01,
		.end	= SW_INT_IRQNO_SPI01,
		.flags	= IORESOURCE_IRQ,
	},
};
static struct platform_device aw_spi1 = {
	.name		= "aw16xx-spi",
	.id			= 1,
	.num_resources	= ARRAY_SIZE(aw16xx_spi1_resources),
	.resource	= aw16xx_spi1_resources,
	.dev		= {
		.platform_data = &aw16xx_spi1_pdata,
	},	
};

static struct resource aw16xx_spi2_resources[] = {
	[0] = {
		.start	= SPI2_BASE_ADDR,
		.end	= SPI2_BASE_ADDR + 1024,
		.flags	= IORESOURCE_MEM,
	},
	#ifdef CONFIG_SUN4I_SPI_NDMA
	[1] = {
		.start	= DMACH_NSPI2,
		.end	= DMACH_NSPI2,
		.flags	= IORESOURCE_DMA,
	},
	#else
	[1] = {
		.start	= DMACH_DSPI2,
		.end	= DMACH_DSPI2,
		.flags	= IORESOURCE_DMA,
	},
	#endif
	[3] = {
		.start	= SW_INT_IRQNO_SPI02,
		.end	= SW_INT_IRQNO_SPI02,
		.flags	= IORESOURCE_IRQ,
	},
};
struct aw16xx_spi_info aw16xx_spi2_pdata = {
	.cs_bitmap	= 0x1,
	.num_cs		= 1,
	.clk_name = "ahb_spi2",
};
static struct platform_device aw_spi2 = {
	.name		= "aw16xx-spi",
	.id			= 2,
	.num_resources	= ARRAY_SIZE(aw16xx_spi2_resources),
	.resource	= aw16xx_spi2_resources,
	.dev		= {
		.platform_data = &aw16xx_spi2_pdata,
	},	
};

static struct resource aw16xx_spi3_resources[] = {
	[0] = {
		.start	= SPI3_BASE_ADDR,
		.end	= SPI3_BASE_ADDR + 1024,
		.flags	= IORESOURCE_MEM,
	},
	#ifdef CONFIG_SUN4I_SPI_NDMA
	[1] = {
		.start	= DMACH_NSPI3,
		.end	= DMACH_NSPI3,
		.flags	= IORESOURCE_DMA,
	},
	#else
	[1] = {
		.start	= DMACH_DSPI3,
		.end	= DMACH_DSPI3,
		.flags	= IORESOURCE_DMA,
	},
	#endif
	[3] = {
		.start	= SW_INT_IRQNO_SPI02,
		.end	= SW_INT_IRQNO_SPI02,
		.flags	= IORESOURCE_IRQ,
	},
};
struct aw16xx_spi_info aw16xx_spi3_pdata = {
	.cs_bitmap	= 0x1,
	.num_cs		= 1,
	.clk_name = "ahb_spi3",
};

static struct platform_device aw_spi3 = {
	.name		= "aw16xx-spi",
	.id			= 3,
	.num_resources	= ARRAY_SIZE(aw16xx_spi3_resources),
	.resource	= aw16xx_spi3_resources,
	.dev		= {
		.platform_data = &aw16xx_spi3_pdata,
	},	
};

//---------------- spi resource and platform data end -----------------------
static struct spi_board_info *spi_boards = NULL;
int sw_register_spi_dev(void)
{
    int spi_dev_num = 0;
    int ret = 0;
    int i = 0;
    char spi_board_name[32] = {0};
    struct spi_board_info* board;
    
    ret = script_parser_fetch("spi_devices", "spi_dev_num", &spi_dev_num, sizeof(int));
    if(ret != SCRIPT_PARSER_OK){
        spi_msg("Get spi devices number failed\n");
        return -1;
    }
    spi_msg("Found %d spi devices in config files\n", spi_dev_num);
    /* alloc spidev board information structure */
    spi_boards = (struct spi_board_info*)kzalloc(sizeof(struct spi_board_info) * spi_dev_num, GFP_KERNEL);
    if (spi_boards == NULL)
    {
        spi_msg("Alloc spi board information failed \n");
        return -1;
    }
    
    spi_msg("%-10s %-16s %-16s %-8s %-4s %-4s\n", "boards num", "modalias", "max_spd_hz", "bus_num", "cs", "mode");
    for (i=0; i<spi_dev_num; i++)
    {
        board = &spi_boards[i];
        sprintf(spi_board_name, "spi_board%d", i);
        ret = script_parser_fetch(spi_board_name, "modalias", (void*)board->modalias, sizeof(char*));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices modalias failed\n");
            goto fail;
        }
        ret = script_parser_fetch(spi_board_name, "max_speed_hz", (void*)&board->max_speed_hz, sizeof(int));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices max_speed_hz failed\n");
            goto fail;
        }
        ret = script_parser_fetch(spi_board_name, "bus_num", (void*)&board->bus_num, sizeof(u16));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices bus_num failed\n");
            goto fail;
        }
        ret = script_parser_fetch(spi_board_name, "chip_select", (void*)&board->chip_select, sizeof(u16));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices chip_select failed\n");
            goto fail;
        }
        ret = script_parser_fetch(spi_board_name, "mode", (void*)&board->mode, sizeof(u8));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices mode failed\n");
            goto fail;
        }
        /*
        ret = script_parser_fetch(spi_board_name, "full_duplex", &board->full_duplex, sizeof(int));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices full_duplex failed\n");
            goto fail;
        }
        ret = script_parser_fetch(spi_board_name, "manual_cs", &board->manual_cs, sizeof(int));
        if(ret != SCRIPT_PARSER_OK) {
            spi_msg("Get spi devices manual_cs failed\n");
            goto fail;
        }
        */
        spi_msg("%-10d %-16s %-16d %-8d %-4d 0x%-4d\n", i, board->modalias, board->max_speed_hz, board->bus_num, board->chip_select, board->mode);
        
    }
    
    /* register boards */
    ret = spi_register_board_info(spi_boards, spi_dev_num);
    if (ret)
    {
        spi_msg("Register board information failed\n");
        goto fail;
    }
    
    return 0;
fail:
    if (spi_boards)
    {
        kfree(spi_boards);
        spi_boards = NULL;
    }
    return -1;
}

static int aw16xx_get_cfg_csbitmap(int bus_num)
{
    int value = 0;
    int ret   = 0;
    char *main_name[] = {"spi0_para", "spi1_para", "spi2_para"};
    char *sub_name = "spi_cs_bitmap";
    
    ret = script_parser_fetch(main_name[bus_num], sub_name, &value, sizeof(int));
    if(ret != SCRIPT_PARSER_OK){
        spi_wrn("get spi %d para failed, err code = %d \n", bus_num, ret);
        return 0;
    }
    spi_msg("bus num = %d, spi used = %d \n", bus_num, value);
    return value;
}

/* get configuration in the script */
#define SPI0_USED_MASK 0x1
#define SPI1_USED_MASK 0x2
#define SPI2_USED_MASK 0x4
#define SPI3_USED_MASK 0x8
static int spi_used = 0;
static int __init aw16xx_spi_init(void)
{
    int used = 0;
    int i = 0;
    int ret = 0;
    char spi_para[16] = {0};
    
    spi_msg("sw spi init !!\n");
    spi_used = 0;
    for (i=0; i<4; i++)
    {
        used = 0;
        sprintf(spi_para, "spi%d_para", i);
        ret = script_parser_fetch(spi_para, "spi_used", &used, sizeof(int));
        if (ret)
        {
            spi_msg("sw spi init fetch spi%d uning configuration failed\n", i);
            continue;
        }
        if (used)
            spi_used |= 1 << i;
    }
    
    ret = sw_register_spi_dev();
    if (ret)
    {
        spi_msg("register spi devices board info failed \n");
    }
    
    if (spi_used & SPI0_USED_MASK)
        platform_device_register(&aw_spi0);
    if (spi_used & SPI1_USED_MASK)
        platform_device_register(&aw_spi1);
    if (spi_used & SPI2_USED_MASK)
        platform_device_register(&aw_spi2);
    if (spi_used & SPI3_USED_MASK)
        platform_device_register(&aw_spi3);
    
    if (spi_used)
    {
        return platform_driver_register(&aw16xx_spi_driver);	
    }
    else
    {
        pr_warning("spi: cannot find any using configuration for \
                    all 4 spi controllers, return directly!\n");
        return 0;
    }
}
module_init(aw16xx_spi_init);


static void __exit aw16xx_spi_exit(void)
{
    if (spi_used)
	    platform_driver_unregister(&aw16xx_spi_driver);
}
module_exit(aw16xx_spi_exit);


MODULE_AUTHOR("Victor.Wei @allwinner");
MODULE_DESCRIPTION("aw16xx spi controller driver");
MODULE_LICENSE("GPL");


