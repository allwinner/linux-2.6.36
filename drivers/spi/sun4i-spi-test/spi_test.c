

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include <linux/platform_device.h> 
#include <linux/spi/spi.h> 

#include "spi_nor.h"


static struct spi_device *nor_device_spi = NULL;


s32 spic_rw(u32 tcnt, u8* txbuf,u32 rcnt, u8* rxbuf)
{   
    int ret = 0;

	struct spi_transfer t[2];
	struct spi_message m;
	
	spi_message_init(&m);
	memset(t, 0, (sizeof t));
	
	if(txbuf) {
		t[0].tx_buf = txbuf;
		t[0].len 	= tcnt;
		spi_message_add_tail(&t[0], &m);		
	}
	
	if(rxbuf) {
		t[1].rx_buf = rxbuf;
		t[1].len	= rcnt;
		spi_message_add_tail(&t[1], &m);	
	}
	
	spi_sync(nor_device_spi, &m);
	
	ret = m.actual_length;
	
	pr_debug("message xfer len = %d \n", ret);	
    
    return ret;
}




static int spi_test_probe(struct spi_device *spi) 
{ 
	printk("spi device name = %s \n", spi->modalias); 
	printk("spi device freq = %d \n", spi->max_speed_hz); 
	printk("spi device's bus_num = %d \n", spi->master->bus_num); 
	printk("spi device cs  = %d \n", spi->chip_select); 
	printk("spi device mode = %d \n\n", spi->mode); 

	nor_device_spi = spi;
	
	spi_nor_rw_test();
	
	
	return 0; 
} 

static int spi_test_remove(struct spi_device *spi) 
{ 
	return 0; 
} 

static struct spi_driver spi_test_driver = { 
	.probe = spi_test_probe, 
	.remove = __devexit_p(spi_test_remove), 
	.driver = { 
		.owner = THIS_MODULE,
		.name = "sp08a", 
	}, 
}; 

static int __init spi_test_init(void) 
{
    printk("spi nor test init\n");
    
	return spi_register_driver(&spi_test_driver); 
} 

static void __exit spi_test_exit(void) 
{ 
	spi_unregister_driver(&spi_test_driver); 
} 

module_init(spi_test_init); 
module_exit(spi_test_exit); 

MODULE_AUTHOR("victor.wei");
MODULE_DESCRIPTION("spansion 08a SPI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:sp08a");

