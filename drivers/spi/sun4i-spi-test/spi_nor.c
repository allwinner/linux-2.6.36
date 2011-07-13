
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/cache.h>
#include <linux/slab.h>


#include "spi_nor_common.h"
#include "spi_nor.h"







static u32 txnum;
static u32 rxnum;


#define SYSTEM_PAGE_SIZE     (512)
#define SPI_NOR_PAGE_SIZE    (256)
#define SPI_NOR_SECTOR_SIZE  (64*1024)


#define EPDK_OK 0
#define EPDK_FAIL -1



#define NPAGE_IN_1SYSPAGE    (SYSTEM_PAGE_SIZE/SPI_NOR_PAGE_SIZE)

/*  local function  */
static s32 spi_nor_wren(void)
{
    u8 sdata = SPI_NOR_WREN;
    s32 ret = EPDK_FAIL;
    
	txnum = 1;
    rxnum = 0;
	pr_debug("spi_nor_wren\n");
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)0);
	
	return ret;
}


static s32 spi_nor_wrdi(void)
{
	u8 sdata = SPI_NOR_WRDI;
    s32 ret = EPDK_FAIL;
    
	txnum = 1;
	rxnum = 0;
	pr_debug("spi_nor_wrdi\n");
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)0);
	
	return ret;
}


static s32 spi_nor_rdsr(u8* reg)
{
    s32 ret = EPDK_FAIL;
    u8 sdata = SPI_NOR_RDSR;
    rxnum = 1;
	txnum = 1;
	
//	pr_debug("spi_nor_rdsr\n");
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)reg);
//	printk("spi nor get status [%p]:%02x\n", reg, *reg);
	return ret;
}

static s32  spi_nor_wrsr(u8 reg)
{
	u8 sdata[2] = {0};
    s32 ret = EPDK_FAIL;
    u8  status = 0;
    u32 i = 0;
    
	ret = spi_nor_wren();
	if (ret==EPDK_FAIL)
	    goto err_out_;
	    
	txnum = 2;
	rxnum = 0;
	
	sdata[0] = SPI_NOR_WRSR;
	sdata[1] = reg;
	pr_debug("spi_nor_wrsr\n");
	ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0);
	if (ret==EPDK_FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status);
	    if (ret==EPDK_FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = EPDK_OK;
	
err_out_:
    
    return ret;
}

static s32 spi_nor_pp(u32 page_addr, void* buf, u32 len)
{
    u8 sdata[260];
    u8  status = 0;
    u32 i = 0;
    s32 ret = EPDK_FAIL;
    
    if (len>256)
    {
        ret = EPDK_FAIL;
        goto err_out_;
    }
    
	ret = spi_nor_wren();
	if (ret==EPDK_FAIL)
	    goto err_out_;
	    
	txnum = len+4;
	rxnum = 0;
	
	memset((void*)sdata, 0xff, 260);
	sdata[0] = SPI_NOR_PP;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
	memcpy((void*)(sdata+4), buf, len);
	pr_debug("spi_nor_pp\n");
	ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0);
	if (ret==EPDK_FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status);
	    if (ret==EPDK_FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = EPDK_OK;
	
err_out_:
    
    return ret;
}


static s32 sst_nor_aai_program(u32 page_addr, void* buf, u32 len)
{
	u32 i = 0;
	u32 j = 0;
    u8  sdata[6] = {0};
    u8* pbuf = buf;
    u8  status = 0;
    s32 ret = EPDK_FAIL;
    
	ret = spi_nor_wren();
	if (ret==EPDK_FAIL)
	    goto err_out_;
	
	rxnum = 0;
	for (j=0; j<len; j+=2)
	{
		if (j==0)
		{
			txnum = 6;
        	sdata[0] = SST_AAI_PRG;
        	sdata[1] = (page_addr>>16)&0xff;
        	sdata[2] = (page_addr>>8)&0xff;
        	sdata[3] = page_addr&0xff;
        	sdata[4] = pbuf[j];
        	sdata[5] = pbuf[j+1];
        }
        else
		{
			txnum = 3;
        	sdata[0] = SST_AAI_PRG;
        	sdata[1] = pbuf[j];
        	sdata[2] = pbuf[j+1];
        }
        ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0);
    	if (ret==EPDK_FAIL)
            goto err_out_;
        
    	do {
    	    ret = spi_nor_rdsr(&status);
    	    if (ret==EPDK_FAIL)
    	        goto err_out_;
    	        
    	    for(i=0; i<100; i++);
    	} while(status&0x01);
	}
	
	ret = spi_nor_wrdi();
	if (ret==EPDK_FAIL)
	    goto err_out_;
        
	do {
	    ret = spi_nor_rdsr(&status);
	    if (ret==EPDK_FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);    
    
	ret = EPDK_OK;
	
err_out_:
    
    return ret;
}


/*  Global Function  */
s32 spi_nor_rdid(u32* id)
{
    s32 ret = EPDK_FAIL;
    u8 sdata = SPI_NOR_RDID;
    txnum = 1;
	rxnum = 3;
	    
    ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)id);

	return ret;
}


s32 spi_nor_read(u32 page_num, u32 page_cnt, void* buf)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u32 rbyte_cnt = page_cnt * SYSTEM_PAGE_SIZE;
    u8  sdata[4] = {0};
    s32 ret = EPDK_FAIL;
    
    txnum = 4;
	rxnum = rbyte_cnt;
    
	sdata[0] = SPI_NOR_READ;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
    
    ret = spic_rw(txnum, (void*)sdata, rxnum, buf);
    
    return ret;
}

s32 spi_nor_fast_read(u32 page_num, u32 page_cnt, void* buf)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u32 rbyte_cnt = page_cnt * SYSTEM_PAGE_SIZE;
    u8  sdata[5] = {0};
    s32 ret = EPDK_FAIL;
    
    txnum = 5;
	rxnum = rbyte_cnt;
    
	sdata[0] = SPI_NOR_FREAD;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
	sdata[4] = 0x55;
    
    ret = spic_rw(txnum, (void*)sdata, rxnum, buf);
    
    return ret;
}

s32 spi_nor_write(u32 page_num, u32 page_cnt, void* buf)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u32 nor_page_cnt = page_cnt * (NPAGE_IN_1SYSPAGE);
    u32 i = 0;
    s32 ret = EPDK_FAIL;
    
    for (i=0; i<nor_page_cnt; i++)
    {
        ret = spi_nor_pp(page_addr+SPI_NOR_PAGE_SIZE*i, (void*)((u32)buf+SPI_NOR_PAGE_SIZE*i), SPI_NOR_PAGE_SIZE);
        if (ret==EPDK_FAIL)
        goto err_out_;
    }
    
    ret = EPDK_OK;
    
err_out_:
    
    return ret;
}

s32 spi_nor_sector_erase(u32 page_num)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u8 sdata[4] = {0};
    s32 ret = EPDK_FAIL;
    u8  status = 0;
    u32 i = 0;
    
	ret = spi_nor_wren();
	if (ret==EPDK_FAIL)
	    goto err_out_;
	    
	txnum = 4;
	rxnum = 0;
	
	sdata[0] = SPI_NOR_SE;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
	
	ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0);
	if (ret==EPDK_FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status);
	    if (ret==EPDK_FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = EPDK_OK;
	
err_out_:
    
    return ret;
}

s32 spi_nor_bulk_erase(void)
{
    u8 sdata = 0;
    s32 ret = EPDK_FAIL;
    u8  status = 0;
    u32 i = 0;
//    u32 busy = 0x55;

	ret = spi_nor_wren();
	if (ret==EPDK_FAIL)
	    goto err_out_;
	    
	txnum = 1;
	rxnum = 0;
	
	sdata = SPI_NOR_BE;
	
	//while(busy);
	
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)0);
	if (ret==EPDK_FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status);
	    if (ret==EPDK_FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = EPDK_OK;
	
err_out_:    
    return ret;
}



/*  spi nor test  */
s32 spi_nor_rw_test(void)
{
    u32 test_len = 512;
    u8* wbuf = (u8*)kmalloc(test_len*sizeof(u8), GFP_KERNEL);
    u8* rbuf = (u8*)kmalloc(test_len*sizeof(u8) , GFP_KERNEL);
    u8* buf =(u8*)kmalloc(65536*sizeof(u8), GFP_KERNEL);
    u32 i,j, temp  = 0;
    u32 sector_num = 0;       
    
    memset(buf, 0, 65536*sizeof(u8));
    
    for (i=0; i<test_len; i++)
    {
        wbuf[i] = ~i;
    }
    
   printk("\n*******test 1--erase all*********\n\n");
    
//    spi_nor_bulk_erase();
   
//   return 0;
   printk("\n*******test 2--erase 64kbytes*********\n\n");
    //擦除0扇区，64KB
   spi_nor_sector_erase(sector_num*128);
    
    printk("\n*******test 3--write 64kbytes*********\n\n");
    //写64KB数据,
    for (i=0; i<128; i++)
    {
        //写1KB数据
        spi_nor_write(sector_num*128+i, 1, wbuf);
        memset(rbuf, 0, 512*sizeof(u8));
        spi_nor_read(sector_num*128+i, 1, rbuf);
        
        if (memcmp(wbuf, rbuf, 512))
        {
            printk("\n spi nor page %d test failed!---\n\n", i);
        }
        else
        {   // return 0 when identical
            printk("\n spi nor page %d test okay!---\n\n", i);
        }
    }
        
    printk("\n*******test 4--read 64kbytes*********\n\n");    
    spi_nor_read(sector_num*128, 128, buf);
    
    for(j=0;j<65536;j++)
    {
        pr_debug("buf[%d] = %d \n", j, buf[j]);
    }    

    printk("\n*******test 5--read id*********\n\n"); 
    spi_nor_rdid(&temp);
    printk("id = %x\n", temp);
    
    kfree(wbuf);
    kfree(rbuf);
    kfree(buf);   
    
    return EPDK_OK;
}

