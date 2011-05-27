#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "crm_regs.h"
#include "mxc_spi.h"
#include "host.h"
#include "iomux.h"

#include "nanoutil.h"
#include "macro.h"

#define TARGET_IRQ_PIN_NUM	(IOMUX_TO_GPIO(TARGET_IRQ_PIN))
#define GPIO_IRQ_MASK		(1 << TARGET_IRQ_PIN_NUM)
#define GPIO_IRQ_CFG_MASK			(0x3 << (2 * TARGET_IRQ_PIN_NUM))
#define GPIO_IRQ_CFG_LOW_LEVEL		(0x0 << (2 * TARGET_IRQ_PIN_NUM))
#define GPIO_IRQ_CFG_HIGH_LEVEL		(0x1 << (2 * TARGET_IRQ_PIN_NUM))
#define GPIO_IRQ_CFG_RISING_EDGE	(0x2 << (2 * TARGET_IRQ_PIN_NUM))
#define GPIO_IRQ_CFG_FALLING_EDGE	(0x3 << (2 * TARGET_IRQ_PIN_NUM))

#define BUSY_WAIT_TIMEOUT_VALUE 10000

volatile unsigned long IMX31_GPIO_BASE;


/*****************************************/
/* Hardware management utility functions */
/*****************************************/
static void imx31_spi_clk_ctrl(int enable)
{
	unsigned long reg;

	KDEBUG(TRANSPORT, "ENTRY");
	reg = __raw_readl(MXC_CCM_CGR2);
	if (enable)
		reg |= MXC_CCM_CGR2_CSPI1_MASK;
	else
		reg &= ~MXC_CCM_CGR2_CSPI1_MASK;		
	__raw_writel(reg, MXC_CCM_CGR2);
	KDEBUG(TRANSPORT, "EXIT");
}

typedef struct pin_list_entry_t
{
	iomux_pin_name_t id;
	iomux_pin_ocfg_t out_cfg;
	iomux_pin_icfg_t in_cfg;
	char		     name[30];
} pin_list_entry_t;

#define PIN_LST_ENT(id, out_cfg, in_cfg) { id, out_cfg, in_cfg, #id }

static const pin_list_entry_t spi_pin_list[] = {
	PIN_LST_ENT(MX31_PIN_CSPI1_MISO,	OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	PIN_LST_ENT(MX31_PIN_CSPI1_MOSI,	OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	PIN_LST_ENT(MX31_PIN_CSPI1_SCLK,	OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	PIN_LST_ENT(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	PIN_LST_ENT(MX31_PIN_CSPI1_MOSI,	OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	PIN_LST_ENT(MX31_PIN_CSPI1_SS0,		OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	PIN_LST_ENT(TARGET_IRQ_PIN, 		OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO)
};

static int imx31_spi_alloc_pins(void)
{
	int status = 0;
	int count = sizeof(spi_pin_list) / sizeof(spi_pin_list[0]);
	const pin_list_entry_t *p_entry = spi_pin_list;

	KDEBUG(TRANSPORT, "ENTRY");
	while (count--) {
		status = mxc_request_iomux(p_entry->id, p_entry->out_cfg,
												p_entry->in_cfg);
		if (status) {
			TRSP_MSG("mxc_request_iomux() for %s failed (status: %d)",
					 p_entry->name, status);
			break;
		}
		p_entry++;
	}

	KDEBUG(TRANSPORT, "EXIT");
	return status;
}

static void imx31_spi_release_pins(void)
{
	int count = sizeof(spi_pin_list) / sizeof(spi_pin_list[0]);
	const pin_list_entry_t *p_entry = spi_pin_list;

	KDEBUG(TRANSPORT, "ENTRY");
	while (count--) {
		mxc_free_iomux(p_entry->id, p_entry->out_cfg,
									p_entry->in_cfg);
		p_entry++;
	}
	KDEBUG(TRANSPORT, "EXIT");
}

static void imx31_prog_gpio_irq(void)
{
	uint32_t icr1 = read_gpio_32(GPIO_ICR1) & ~GPIO_IRQ_CFG_MASK;
	uint32_t psr = read_gpio_32(GPIO_PSR);

	/* An IRQ is triggered on both edges of the GPIO pin,
	 * so set IRQ edge polarity to anticipate the next transition.
	 */
	if (psr & GPIO_MASK)
		icr1 |= GPIO_IRQ_CFG_FALLING_EDGE;
	else
		icr1 |= GPIO_IRQ_CFG_RISING_EDGE;

	write_gpio_32(GPIO_ICR1, icr1);
}


/*****************/
/* Data transfer */
/*****************/
unsigned int  spi_senddata_nodma(unsigned char* data, unsigned int size)
{
    volatile unsigned int uTxNumBytes = 0;
    volatile unsigned char fifo_cnt = 0;
    volatile unsigned char dummy = 0xFF;
	volatile unsigned long timeout = BUSY_WAIT_TIMEOUT_VALUE;
	KDEBUG(TRANSPORT, "ENTRY");

	do {
		//// FILL TXFIFO
		do{
			 write_32(MXC_CSPITXDATA,*(volatile unsigned char *)data); /// write the data to fifo
			 data++;
			 uTxNumBytes++;
			 fifo_cnt = read_32(MXC_CSPITEST) & (0x0000000F); /// read TXFIFO counter
		} while( (fifo_cnt <7) & ( uTxNumBytes<size ) ); /// stop if we fill the fifo or we don't have any other data

		/// start the transfer
		or_32(MXC_CSPICTRL,MXC_CSPICTRL_XCH); 

		timeout = BUSY_WAIT_TIMEOUT_VALUE;
		/// Transfer Complete Check
		do {
		 if(!timeout) {
			TRSP_ASSERT(0); 
		  return 0;
		 }
		 timeout --;
		} while(!(read_32(MXC_CSPISTAT) & (MXC_CSPISTAT_TC_0_4))); 

		write_32(MXC_CSPISTAT,MXC_CSPISTAT_TC_0_4); /// clean the tranfer complete

		/// check if we have read data
		while(read_32(MXC_CSPISTAT) & MXC_CSPISTAT_RR) {
		 dummy = read_32(MXC_CSPIRXDATA); // Dummy read of any 0xFF to empty
		}

		fifo_cnt = 0;

	} while (uTxNumBytes < size);
	
	fifo_cnt =read_32(MXC_CSPITEST) & (0x0000000F);  /// read TXFIFO counter
    TRSP_ASSERT(fifo_cnt == 0);

	 KDEBUG(TRANSPORT, "EXIT");
    return 0;
}

unsigned int spi_readdata_nodma(unsigned char* data, unsigned int size)
{
    volatile unsigned int numberofLoops = 0;
    volatile unsigned int bytes_to_read = 0;
    volatile unsigned int remaining_bytes = 0;
    unsigned int i,j;
    volatile unsigned char fifo_cnt = 0;
    const unsigned char nop = 0xFF;
    KDEBUG(TRANSPORT, "ENTRY");

    numberofLoops = size/8;

    if(size%8)
		numberofLoops ++;

    remaining_bytes = size;

    for(i = 0 ; i < numberofLoops; i++) {
       if(remaining_bytes > 8)
        bytes_to_read = 8;
       else
        bytes_to_read = remaining_bytes;

		for(j=0;j<bytes_to_read;j++){
			write_32(MXC_CSPITXDATA,nop); /// write the nop data to fifo
        } 
		
		/// start the transfer
		or_32(MXC_CSPICTRL,MXC_CSPICTRL_XCH); 

		/// Transfer Complete Check
		while(!(read_32(MXC_CSPISTAT) & (MXC_CSPISTAT_TC_0_4))); 

		/// clean the tranfer complete
		write_32(MXC_CSPISTAT,MXC_CSPISTAT_TC_0_4);

        do {
			 fifo_cnt = ((read_32(MXC_CSPITEST) & (0x000000F0)) >> 4); // read the # of RXFIFO 

			 if(fifo_cnt > 0) {
				*data = read_32(MXC_CSPIRXDATA) & (0x000000FF);
				data++;
				remaining_bytes--;
				bytes_to_read--;
			 }
        }while(bytes_to_read);
    }

    fifo_cnt = ((read_32(MXC_CSPITEST) & (0x000000F0)) >> 4); // read the # of RXFIFO 

    TRSP_ASSERT(fifo_cnt == 0);

	KDEBUG(TRANSPORT, "EXIT");
    return 0;
}


/************************************/
/*  Implementation of API in host.h */
/************************************/
int host_init(void)
{
	int status;
	u32 ctrl_reg;

	KDEBUG(TRANSPORT, "ENTRY");
	
	if (TARGET_IRQ_PIN_NUM > 15) {
		TRSP_MSG("TARGET_IRQ_PIN must be in the range of "
				 "MX31_PIN_GPIO1_0 to MX31_PIN_GPIO1_15");
		return -EINVAL;
	}

	IMX31_GPIO_BASE = (volatile unsigned long) ioremap(0x53FCC000, 0x18);
	if(!IMX31_GPIO_BASE) {
		TRSP_MSG("ioremap() for IMX31_GPIO_BASE failed");
	 return -EINVAL;
	}

	/* Keep host SPI hardware reset */
	ctrl_reg = read_32(MXC_CSPICTRL);
	write_32(MXC_CSPICTRL, ctrl_reg & ~MXC_CSPICTRL_ENABLE);

	status = imx31_spi_alloc_pins();
	if (status) {
		TRSP_MSG("imx31_spi_pin_alloc() failed");
		return -EIO;
	}

	imx31_spi_clk_ctrl(1);
	mdelay(1);

	/* Program and start host SPI hardware */
	ctrl_reg &= ~ ((0x3 << MXC_CSPICTRL_CSSHIFT_0_4)
				 | (0x3 << MXC_CSPICTRL_DRCTRLSHIFT_0_4)
				 | (0x7 << MXC_CSPICTRL_DATASHIFT)
				 | (0x1f << MXC_CSPICTRL_BCSHIFT_0_4));
	ctrl_reg |= (0x0 << MXC_CSPICTRL_CSSHIFT_0_4);		/* use ~SS0 as Chip Select */ 
	ctrl_reg |= (0x0 << MXC_CSPICTRL_DRCTRLSHIFT_0_4);	/* don't care about ~SPI_RDY */
	ctrl_reg |= (0x1 << MXC_CSPICTRL_DATASHIFT);		/* divide by 8 to generate SCLK at 8MHz */
	ctrl_reg |= (0x7 << MXC_CSPICTRL_BCSHIFT_0_4);		/* set bit count to 8 */
	ctrl_reg |= MXC_CSPICTRL_LOWSSPOL;	/* Chip Select (SS) is active low */
	ctrl_reg &= ~MXC_CSPICTRL_SSCTL;	/* leave Chip Select asserted between bursts */
	ctrl_reg |= MXC_CSPICTRL_NOPHA;		/* SCLK phase 0 */
	ctrl_reg |= MXC_CSPICTRL_HIGHPOL;	/* SCLK is low when the bus is idle */
	ctrl_reg &= ~MXC_CSPICTRL_SMC;		/* start bursts manually (when MXC_CSPICTRL_XCH is set) */
	ctrl_reg |= MXC_CSPICTRL_MASTER;	/* operate as a master */
	ctrl_reg |= MXC_CSPICTRL_ENABLE; 	/* exit reset state */
	write_32(MXC_CSPICTRL, ctrl_reg);

	/* Disable interrupt from target and prepare to receive IRQ */
	and_gpio_32(GPIO_IMR,~GPIO_MASK);
	imx31_prog_gpio_irq();

	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}


int host_interrupt(irq_op_t irq_op)
{
	uint32_t isr;

	KDEBUG(TRANSPORT, "ENTRY");

	switch (irq_op) {
		case IRQ_OP_ENABLE:			
			or_gpio_32(GPIO_IMR, GPIO_MASK);
			break;

		case IRQ_OP_DISABLE:
			and_gpio_32(GPIO_IMR, ~GPIO_MASK);
			imx31_prog_gpio_irq(); /* Prepare for the next IRQ */
			break;

		case IRQ_OP_STATUS:
			isr = read_gpio_32(GPIO_ISR) ;
			return (isr & GPIO_MASK);

		case IRQ_OP_ACKNOWLEDGE:
			or_gpio_32(GPIO_ISR, GPIO_MASK);
			break;

		default:
			BUG();
	}
	
	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}


int host_send_cmd0(void)
{
    unsigned char zeros[] = {   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    unsigned char cmd0[] = {	0xff,0xff,0x40,0x0,0x0,0x0,0x0,0x01,0xff};
    unsigned char response = 0xff;

	KDEBUG(TRANSPORT, "ENTRY");

    spi_senddata_nodma(zeros, sizeof(zeros));
    spi_senddata_nodma(cmd0, sizeof(cmd0));
    spi_readdata_nodma(&response, 1);

	KDEBUG(TRANSPORT, "Response to CMD0: 0x%02x\n", response);
	return (response == 0x01) ? 0 : -EIO;
}


int host_send_cmd52(uint32_t data, int use_slow_clock)
{
	static unsigned char cmd52[] =
	{
		0xff,	// ** command preamble
		0x74,	// StartBit = 0, Dir = 1, cmd index = 0b110100
		0x00,
		0x00,
		0x00,
		0x00,
		0xff,	// CRC = 0b1111111, EndBit = 1
		0xff,	// ** response preamble
		0xff, 0xff	// receive R5 response (16 bits)
	};

    KDEBUG(TRANSPORT, "ENTRY");

    cmd52[2] = (unsigned char) (data >> 24) & 0xff;
    cmd52[3] = (unsigned char) (data >> 16) & 0xff;
    cmd52[4] = (unsigned char) (data >>  8) & 0xff;
    cmd52[5] = (unsigned char) data & 0xff;

#if 1
    spi_senddata_nodma(cmd52, sizeof(cmd52));
#else
	{
		unsigned char resp[2];
		spi_senddata_nodma(cmd52, sizeof(cmd52) - 2);
		spi_readdata_nodma(resp, 2);
		TRSP_MSG("CMD52 response: 0x%02x 0x%02x\n", resp[0], resp[1]);
	}
#endif

	KDEBUG(TRANSPORT, "EXIT");
    return 0;
}


int host_read_cmd53(unsigned char* data, unsigned int size)
{
	static unsigned char cmd53_read[] =
	{
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
	};

    static unsigned char cmd53_post[] =
    {
        0xff, 0xff
    };

    KDEBUG(TRANSPORT, "ENTRY");

	/* Write the size to the CMD53 header */
	cmd53_read[4] = (unsigned char) ((size & 511) >> 8);
	cmd53_read[5] = (unsigned char) (size & 0xff);

	spi_senddata_nodma(cmd53_read, sizeof(cmd53_read));
	spi_readdata_nodma(data, size);
	spi_senddata_nodma(cmd53_post, sizeof(cmd53_post));

    KDEBUG(TRANSPORT, "EXIT");
    return 0;
}


int host_send_cmd53(unsigned char* data, unsigned int size)
{
	static unsigned char cmd53_write[] = 
	{
		0xff,	// ** command preamble
		0x75,	// StartBit = 0, Dir = 1, cmd index = 0b110101
		0x90,	// R/W = 1, Func# = 1, BlockMode = 0, OpCode = 0
		0x00,
		0x00,   // size msb
		0x00,   // size lsb
		0x01,	// CRC = 0b0000000, EndBit = 1
		0xff,	// ** response preamble
		0xff, 0xfe	// receive R5 response (16 bits), also send StartBit = 0
	};

	static unsigned char cmd53_post[] =
	{
		0xff, 0xff,
	};

    KDEBUG(TRANSPORT, "ENTRY");

	/* Write the size to the CMD53 header */
	cmd53_write[4] = (unsigned char) ((size & 511) >> 8);
	cmd53_write[5] = (unsigned char) (size & 0xff);

	spi_senddata_nodma(cmd53_write, sizeof(cmd53_write));
	spi_senddata_nodma(data, size);
	spi_senddata_nodma(cmd53_post, sizeof(cmd53_post));

    KDEBUG(TRANSPORT, "EXIT");
	return 0;
}


void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
}


void host_exit(void)
{
	imx31_spi_release_pins();
	imx31_spi_clk_ctrl(0);
}
