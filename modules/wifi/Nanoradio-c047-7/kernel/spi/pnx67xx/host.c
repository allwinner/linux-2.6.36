#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/clk.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#include "spi_pnx67xx.h"
#include "host.h"
#include "nanoutil.h"
#include "macro.h"
	
#include "spi_config.c"
#define IMX31_SPI_MAGIC_VERSION "nanoradio wifi driver for spi linux"


///////////////////////////////////////////////////////////////

// [samsung] You must change this function to setup your gpios
int spi_gpio_setup(void)
{
	/* MISO */
	if(pnx_request_gpio(GPIO_A31)!=0)
	{
		TRSP_MSG("[WLAN DEBUG] spi_gpio_setup MISO requesting failed\n");
		return -1;
	}
	pnx_set_gpio_mode(GPIO_A31,GPIO_MODE_MUX2);
	pnx_set_gpio_direction(GPIO_A31,GPIO_DIR_INPUT);
	
	TRSP_MSG("[WLAN DEBUG] spi_gpio_setup MISO setting OK\n");
		

	/* MOSI */
	if(pnx_request_gpio(GPIO_A28)!=0)
	{
		TRSP_MSG("[WLAN DEBUG] spi_gpio_setup MOSI requesting failed\n");
		return -1;
	}
	pnx_set_gpio_mode(GPIO_A28,GPIO_MODE_MUX1);
	pnx_set_gpio_direction(GPIO_A28,GPIO_DIR_OUTPUT);

	TRSP_MSG("[WLAN DEBUG] spi_gpio_setup MOSI setting OK\n");

	/* CLK */
	if(pnx_request_gpio(GPIO_A27)!=0)
	{
		TRSP_MSG("[WLAN DEBUG] spi_gpio_setup CLK requesting failed\n");
		return -1;
	}
	pnx_set_gpio_mode(GPIO_A27,GPIO_MODE_MUX1);
	pnx_set_gpio_direction(GPIO_A27,GPIO_DIR_OUTPUT);

	TRSP_MSG("[WLAN DEBUG] spi_gpio_setup CLK setting OK\n");

	/* Reset */
	if(pnx_request_gpio(GPIO_D16)!=0)
	{
		TRSP_MSG("[WLAN DEBUG] spi_gpio_setup Reset requesting failed\n");
		return -1;
	}

	pnx_set_gpio_mode(GPIO_D16,GPIO_MODE_MUX1);
	pnx_set_gpio_direction(GPIO_D16,GPIO_DIR_OUTPUT);

	TRSP_MSG("[WLAN DEBUG] spi_gpio_setup Reset setting OK\n");

	/* WLAN_INT */
	if(pnx_request_gpio(EXTINT_TO_GPIO(IRQ_EXTINT(14))!=0))
	{
		TRSP_MSG("[WLAN DEBUG] spi_gpio_setup WLAN_INT requesting failed\n");
		return -1;
	}
	pnx_set_gpio_direction(EXTINT_TO_GPIO(IRQ_EXTINT(14)),GPIO_DIR_INPUT);
	set_irq_type(IRQ_EXTINT(14), IRQ_TYPE_EDGE_BOTH);

	TRSP_MSG("[WLAN DEBUG] spi_gpio_setup WLAN_INT setting OK\n");
	
	/* CS */
	if( pnx_request_gpio( SPI_CHIP_SELECT ) != 0 )
	{
		TRSP_MSG("[WLAN DEBUG] spi_gpio_setup CS requesting failed\n");
		return -1;
	}

	/* GPIO_Ax have MUX0, others have MUX1 */
	if( SPI_CHIP_SELECT >= GPIO_B0 )
	{
		pnx_set_gpio_mode( SPI_CHIP_SELECT, GPIO_MODE_MUX1 );
	}
	else
	{
		pnx_set_gpio_mode( SPI_CHIP_SELECT, GPIO_MODE_MUX0 );
	}
	pnx_set_gpio_direction( SPI_CHIP_SELECT, GPIO_DIR_OUTPUT );
	/* set the cs to 0 only for the start */
	pnx_write_gpio_pin(SPI_CHIP_SELECT, 0);

	TRSP_MSG("[WLAN DEBUG] spi_gpio_setup CS setting OK\n");
	
	return 0;
}

void spi_gpio_cleanup(void)
{
	pnx_free_gpio(GPIO_A31);
	pnx_set_gpio_direction(GPIO_A28, GPIO_DIR_INPUT);
	pnx_free_gpio(GPIO_A28);
	pnx_set_gpio_direction(GPIO_A27, GPIO_DIR_INPUT);
	pnx_free_gpio(GPIO_A27);
	pnx_set_gpio_direction(GPIO_D16, GPIO_DIR_INPUT);
	pnx_free_gpio(GPIO_D16);
	pnx_free_gpio(EXTINT_TO_GPIO(IRQ_EXTINT(14)));
	pnx_set_gpio_direction(SPI_CHIP_SELECT, GPIO_DIR_INPUT);
	pnx_free_gpio(SPI_CHIP_SELECT);
}

int
host_init(void)
{

	KDEBUG(TRANSPORT, "ENTRY");
	TRSP_MSG("Driver's Magic Version : %s\n", IMX31_SPI_MAGIC_VERSION);
	
	spi_base_regs=ioremap(SPI_BASE_ADDR, 0x0FFF + 0x0001);
	
	spi_gpio_setup(); /// [Samsung] You must change this function for your gpio.

	TRSP_MSG("FX==> SPI1_BASE=%x\n",SPI1_BASE);
	
	spi_nrx_setup();
	
	host_interrupt(IRQ_OP_ACKNOWLEDGE);
	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}

//JuHyoung patch for porting to PNX platform
//volatile unsigned int masked; // [Samsung] You can delete after implementing your own host_interrupt()

// [Samsung] Must implement your own host_interrupt(), see host.h about its behaviour
int
host_interrupt(irq_op_t irq_op)
{
//++JuHyoung patch for porting to PNX platform
switch (irq_op) {
	case IRQ_OP_ENABLE :
	{
		/***
			Enable EXTINT14
		***/
		enable_irq(IRQ_EXTINT(14));

		TRSP_MSG("[WLAN DEBUG] host_interrupt EXTINT pin enabled\n");
	}
	break;

	case IRQ_OP_DISABLE :
	{
		/***
			Disable EXTINT14
		***/
		disable_irq(IRQ_EXTINT(14));
		TRSP_MSG("[WLAN DEBUG] host_interrupt EXTINT pin disabled\n");
		#if 0
		reg = __raw_readl(EXTINT_ENABLE1_REG);
		reg &= ~(1 << 14);
		__raw_writel(reg, EXTINT_ENABLE1_REG);
		#endif
	}
	break;

	case IRQ_OP_STATUS :
	{	
		/***
			Get interrupt status
		***/
	
		unsigned int reg = irq_desc[IRQ_EXTINT(14)].status;

		TRSP_MSG("[WLAN DEBUG] host_interrupt : EXTINT status[%x]\n", reg);

		if(reg & IRQ_DISABLED)
		{
			TRSP_MSG("[WLAN DEBUG] host_interrupt EXTINT pin is disabled\n");
			return 0; /* IRQ is disabled */
		}
		else
		{
			TRSP_MSG("[WLAN DEBUG] host_interrupt EXTINT pin is enabled\n");
			return 1; /* IRQ is enabled */
		}
		
		#if 0
		reg = __raw_readl(EXTINT_STATUS_REG);
		reg &= (1 << 14); /*target interrupt mask*/
		if(reg)
			return 1;
		#endif
	}
	break;

	case IRQ_OP_ACKNOWLEDGE :
	{
		/***
			Clear interrupt
		***/
		#if 1
		struct irq_chip * chip_info = get_irq_chip(IRQ_EXTINT(14));
		chip_info->ack(IRQ_EXTINT(14));
		TRSP_MSG("[WLAN DEBUG] host_interrupt EXTINT pin acked\n");
		#endif
		
	}
	break;

	default:
		BUG();
}

return 0;
//--JuHyoung patch for porting to PNX platform
}

void
host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
//++JuHyoung patch for porting to PNX platform
	switch(gpio_id)
	{
		case GPIO_ID_RESET:
		{
			switch(gpio_op)
			{
				case GPIO_OP_ENABLE:
				{
					#if 0
					/* This pin already requested by SPI core */
					if(pnx_request_gpio(GPIO_D16)!=0)
					{
						TRSP_MSG("[WLAN DEBUG] host_gpio : reset pin enabling failed\n");
						return -1;
					}
					#endif
				
					pnx_set_gpio_mode(GPIO_D16,GPIO_MODE_MUX1);
					pnx_set_gpio_direction(GPIO_D16,GPIO_DIR_OUTPUT);
					TRSP_MSG("[WLAN DEBUG] host_gpio : reset pin enabled\n");
					
				}break;
				case GPIO_OP_DISABLE:
				{
					pnx_free_gpio(GPIO_D16);
					TRSP_MSG("[WLAN DEBUG] host_gpio : reset pin disabled\n");
				}break;
				case GPIO_OP_HIGH:
				{
					pnx_write_gpio_pin(GPIO_D16, 1);
					TRSP_MSG("[WLAN DEBUG] host_gpio : reset pin set to HIGH\n");

				}break;
				case GPIO_OP_LOW:
				{
					pnx_write_gpio_pin(GPIO_D16, 0);
					TRSP_MSG("[WLAN DEBUG] host_gpio : reset pin set to LOW\n");
				}break;
				default:
				break;
			}
		}break;

		case GPIO_ID_POWER:
		{
			switch(gpio_op)
			{
				case GPIO_OP_ENABLE:
				{
					if(pnx_request_gpio(GPIO_A13)!=0)
					{
						TRSP_MSG("[WLAN DEBUG] host_gpio : POWER pin enabling failed \n");
						return;
					}
					pnx_set_gpio_mode(GPIO_A13,GPIO_MODE_MUX0);
					pnx_set_gpio_direction(GPIO_A13,GPIO_DIR_OUTPUT);
					TRSP_MSG("[WLAN DEBUG] host_gpio : POWER pin enabled \n");

				}break;
				case GPIO_OP_DISABLE:
				{
					pnx_free_gpio(GPIO_A13);
					TRSP_MSG("[WLAN DEBUG] host_gpio : POWER pin disabled \n");
				}break;
				case GPIO_OP_HIGH:
				{
					pnx_write_gpio_pin(GPIO_A13, 1);
					TRSP_MSG("[WLAN DEBUG] host_gpio : POWER pin set to HIGH \n");
				}break;
				case GPIO_OP_LOW:
				{
					pnx_write_gpio_pin(GPIO_A13, 0);
					TRSP_MSG("[WLAN DEBUG] host_gpio : POWER pin set to LOW \n");
				}break;
				default:
				break;
			}
		}break;

		default:
		break;
	}
//--JuHyoung patch for porting to PNX platform
 
    return;
}

void
host_exit(void)
{
   //spi_unregister_driver(&spi_nrx_driver);
	spi_nrx_cleanup();
    spi_gpio_cleanup();
	return;
}

int host_send_cmd0(void)
{
	//After restart TARGET interface will be in SDIO-mode
    //To put the interface in SPI-mode we need to send a Command-zero with CS low

    // Init string to set SDIO controller in SPI mode
    unsigned char zeros[] = {   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    unsigned char cmd0[] = {0xff,0xff,0x40,0x0,0x0,0x0,0x0,0x01,0xff};

    unsigned char input=0xFF;

    int result = false;
   KDEBUG(TRANSPORT, "ENTRY");

  ///  TRSP_MSG("[WLAN DEBUG] send_cmd0 : send zeros\n");

    spi_nrx_write(zeros,sizeof(zeros));

    spi_nrx_write(zeros,sizeof(zeros));

    spi_nrx_write(cmd0,sizeof(cmd0));

  //  TRSP_MSG("[WLAN DEBUG] send_cmd0 : spi_nrx_read\n");

    spi_nrx_read(&input,1);

 //   TRSP_MSG("[WLAN DEBUG] input[%d]\n",input);

	KDEBUG(TRANSPORT, "input=%d",input);
    // Last byte should be 1 on success
    result = (input == 1);

    if(!result)
    {
        KDEBUG(TRANSPORT, "%s failed", __FUNCTION__);
    }

KDEBUG(TRANSPORT, "EXIT");
    return result ? 0 : -EIO;
}


int host_send_cmd52(uint32_t content, int use_slow_clock)
{
	KDEBUG(TRANSPORT, "ENTRY");

    cmd52[2] = (unsigned char) (content >> 24) & 0xFF;
    cmd52[3] = (unsigned char) (content >> 16) & 0xFF;
    cmd52[4] = (unsigned char) (content >> 8) & 0xFF;
    cmd52[5] = (unsigned char) content & 0xFF;

    spi_nrx_write(cmd52, sizeof(cmd52_temp));
    //spi_readdata_nodma(response,2);
    //TRSP_MSG("CMD52 Read response: 0x%02x 0x%02x\n", response[0], response[1]);
	KDEBUG(TRANSPORT, "EXIT");
    return 0;
}


int host_read_cmd53(unsigned char* data, unsigned int size)
{
        //Write the size to the CMD53 header
        cmd53_read[6] = (unsigned char) ((size & 511)>>8);
        cmd53_read[7] = (unsigned char) size & 0xFF;

	//TRSP_MSG("[WLAN DEBUG] read_cmd53 : send cmd53_read\n");

        //writed the SDIO header
        spi_nrx_write(cmd53_read, sizeof(cmd53_read_temp));
        //reads the data
        spi_nrx_read(data, size);

	//TRSP_MSG("[WLAN DEBUG] read_cmd53 : send cmd53_post\n");
        //writes the SDIO CRC
        spi_nrx_write(cmd53_crc, sizeof(cmd53_crc_temp));
		
    return 0;
}

int host_send_cmd53(unsigned char* data, unsigned int size)
{
        //Write the size to the CMD53 header
        cmd53_write[6] = (unsigned char) ((size & 511)>>8);
        cmd53_write[7] = (unsigned char) size & 0xFF;

	//TRSP_MSG("[WLAN DEBUG] send_cmd53 : send cmd53_write\n");
        spi_nrx_write(cmd53_write, sizeof(cmd53_write_temp));
        spi_nrx_write(data, size);
	//TRSP_MSG("[WLAN DEBUG] send_cmd53 : send cmd53_post\n");
        spi_nrx_write(cmd53_crc, sizeof(cmd53_crc_temp));

    return 0;
}
