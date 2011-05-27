#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "driverenv.h"

#include "crm_regs.h"
#include "mxc_spi_mx27.h"
#include "host.h"
#include "gpio_mux.h"


#include "nanoutil.h"
#include "macro.h"

//MX27 #define TARGET_IRQ_PIN_NUM			(IOMUX_TO_GPIO(TARGET_IRQ_PIN))

#define TARGET_IRQ_PIN_NUM			GPIO_TO_INDEX(IOMUX_TO_GPIO(TARGET_IRQ_PIN))
#define TARGET_IRQ_PORT_NUM			GPIO_TO_PORT(IOMUX_TO_GPIO(TARGET_IRQ_PIN))

// Not used #define GPIO_IRQ_MASK				(1 << TARGET_IRQ_PIN_NUM)

/* Definitions for GPIO_ICR1 and GPIO_ICR2 */
#define GPIO_IRQ_CFG_REG2			((TARGET_IRQ_PIN_NUM & 0x10) >> 4) /* If 1 use GPIO_ICR2 else GPIO_ICR1 */
#define GPIO_IRQ_CFG_MASK			(0x3 << (2 * (TARGET_IRQ_PIN_NUM & 0xF)))
#define GPIO_IRQ_CFG_LOW_LEVEL		(0x0 << (2 * (TARGET_IRQ_PIN_NUM & 0xF)))
#define GPIO_IRQ_CFG_HIGH_LEVEL		(0x1 << (2 * (TARGET_IRQ_PIN_NUM & 0xF)))
#define GPIO_IRQ_CFG_RISING_EDGE	(0x2 << (2 * (TARGET_IRQ_PIN_NUM & 0xF)))
#define GPIO_IRQ_CFG_FALLING_EDGE	(0x3 << (2 * (TARGET_IRQ_PIN_NUM & 0xF)))

/* Definitions for GPIO_PSR Pad Status */
#define	 GPIO_TARGET_IRQ_PSR_MASK				(1 << TARGET_IRQ_PIN_NUM)

/* Definitions for GPIO_IMR Interrupt Mask Register */
#define	 GPIO_TARGET_IRQ_IMR_MASK				(1 << TARGET_IRQ_PIN_NUM)

/* Definitions for GPIO_ISR Interrupt Status Register */
#define	 GPIO_TARGET_IRQ_ISR_MASK				(1 << TARGET_IRQ_PIN_NUM)

/* Converts the port number to an address offset */
#define GPIO_PORT_REG_OFFSET(port)				(port << 8)
#define GPIO_TARGET_IRQ_PORT_REG_OFFSET			GPIO_PORT_REG_OFFSET(TARGET_IRQ_PORT_NUM)

volatile unsigned long IMX27_GPIO_BASE;

/*
 * Stolen functions from kernel
 *
 *
 */

#include <asm/arch/gpio.h>


struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR),
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x100,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN,
	 },
	{
	 .num = 2,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x200,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 2,
	 },
	{
	 .num = 3,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x300,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 3,
	 },
	{
	 .num = 4,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x400,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 4,
	 },
	{
	 .num = 5,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x500,
	 .irq = INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 5,
	 },
};
/*!
 * This structure defines the offset of registers in gpio module.
 */
enum gpio_reg {
	GPIO_GIUS = 0x20,
	GPIO_GPR = 0x38,
	GPIO_PUEN = 0x40,
	GPIO_DDIR = 0x00,
	GPIO_OCR1 = 0x04,
	GPIO_OCR2 = 0x08,
	GPIO_ICONFA1 = 0x0C,
	GPIO_ICONFA2 = 0x10,
	GPIO_ICONFB1 = 0x14,
	GPIO_ICONFB2 = 0x18,
};
/*!
 * This enumeration data type defines the configuration for input mode.
 */
typedef enum {
	GPIO_INPUT_GPIO = 0x00,
	GPIO_INPUT_INTR = 0x01,
	GPIO_INPUT_LOW = 0x02,
	GPIO_INPUT_HIGH = 0x03
} gpio_input_cfg_t;
/*!
 * This enumeration data type defines the configuration for output mode.
 */
typedef enum {
	GPIO_OUTPUT_A = 0x00,
	GPIO_OUTPUT_B = 0x01,
	GPIO_OUTPUT_C = 0x02,
	GPIO_OUTPUT_DR = 0x03
} gpio_output_cfg_t;







/*!
 * This function set the input configuration A. 
 * @param port  	a pointer of gpio port
 * @param index 	the index of the  pin in the port
 * @param config	a mode as define in \b #gpio_input_cfg_t
 * @return		none
 */
static inline void _gpio_set_iconfa(struct mxc_gpio_port *port, u32 index,
				    gpio_input_cfg_t config)
{
	u32 reg, val;
	u32 mask;

	mask = 0x3 << ((index % 16) << 1);

	if (index >= 16) {
		reg = port->base + GPIO_ICONFA2;
		val = config << ((index - 16) * 2);
	} else {
		reg = port->base + GPIO_ICONFA1;
		val = config << (index * 2);
	}
	val |= __raw_readl(reg) & ~(mask);
	__raw_writel(val, reg);
}




/*!
 * This function set the input configuration B. 
 * @param port  	a pointer of gpio port
 * @param index 	the index of the  pin in the port
 * @param config	a mode as define in \b #gpio_input_cfg_t
 * @return		none
 */
static inline void _gpio_set_iconfb(struct mxc_gpio_port *port, u32 index,
				    gpio_input_cfg_t config)
{
	u32 reg, val;
	u32 mask;

	mask = 0x3 << ((index % 16) << 1);

	if (index >= 16) {
		reg = port->base + GPIO_ICONFB2;
		val = config << ((index - 16) * 2);
	} else {
		reg = port->base + GPIO_ICONFB1;
		val = config << (index * 2);
	}
	val |= __raw_readl(reg) & (~mask);
	__raw_writel(val, reg);
}

/*!
 * This function set the output configuration. 
 * @param port  	a pointer of gpio port
 * @param index 	the index of the  pin in the port
 * @param config	a mode as define in \b #gpio_output_cfg_t
 * @return		none
 */
static inline void _gpio_set_ocr(struct mxc_gpio_port *port, u32 index,
				 gpio_output_cfg_t config)
{
	u32 reg, val;
	u32 mask;

	mask = 0x3 << ((index % 16) << 1);
	if (index >= 16) {
		reg = port->base + GPIO_OCR2;
		val = config << ((index - 16) * 2);
	} else {
		reg = port->base + GPIO_OCR1;
		val = config << (index * 2);
	}
	val |= __raw_readl(reg) & (~mask);
	__raw_writel(val, reg);
}

extern struct mxc_gpio_port mxc_gpio_ports[];

/*!
 * defines a spinlock to protected the accessing to gpio pin.
 */
DEFINE_SPINLOCK(gpio_mux_lock);


/*!
 * This function is just used to request a pin and configure it.
 * @param pin	a pin number as defined in \b #iomux_pin_name_t
 * @param mode	a module as define in \b #gpio_mux_mode_t;
 * @return	0 if successful, Non-zero otherwise
 */
int gpio_request_mux(iomux_pin_name_t pin, gpio_mux_mode_t mode)
{
	int ret;
	ret = mxc_request_gpio(pin);
	if (ret == 0) {
		ret = gpio_config_mux(pin, mode);
		if (ret) {
			mxc_free_gpio(pin);
		}
	}
	return ret;
}


/*!
 * This function is just used to release a pin.
 * @param pin	a pin number as defined in \b #iomux_pin_name_t
 * @return	none	
 */
void gpio_free_mux(iomux_pin_name_t pin)
{
	mxc_free_gpio(pin);
}



/*!
 *@brief gpio_config_mux - just configure the mode of the gpio pin.
 *@param pin   a pin number as defined in \b #iomux_pin_name_t
 *@param mode  a module as define in \b #gpio_mux_mode_t;
 *	GPIO_MUX_PRIMARY set pin to work as primary function.
 *	GPIO_MUX_ALT set pin to work as alternate function.
 *	GPIO_MUX_GPIO set pin to work as output function based the data register
 *	GPIO_MUX_INPUT1 set pin to work as input function connected with  A_OUT
 *	GPIO_MUX_INPUT2 set pin to work as input function connected with B_OUT
 *	GPIO_MUX_OUTPUT1 set pin to work as output function connected with A_IN
 *	GPIO_MUX_OUTPUT2 set pin to work as output function connected with B_IN
 *	GPIO_MUX_OUTPUT3 set pin to work as output function connected with C_IN
 *@return      0 if successful, Non-zero otherwise
 */

int gpio_config_mux(iomux_pin_name_t pin, gpio_mux_mode_t mode)
{
	unsigned long lock_flags;
	u32 gius_reg, gpr_reg;
	struct mxc_gpio_port *port;
	u32 index, gpio = IOMUX_TO_GPIO(pin);

	port = &(mxc_gpio_ports[GPIO_TO_PORT(gpio)]);
	index = GPIO_TO_INDEX(gpio);

	pr_debug("%s: Configuring PORT %c, bit %d\n",
		 __FUNCTION__, port->num + 'A', index);

	spin_lock_irqsave(&gpio_mux_lock, lock_flags);

	gius_reg = __raw_readl(port->base + GPIO_GIUS);
	gpr_reg = __raw_readl(port->base + GPIO_GPR);

	switch (mode) {
	case GPIO_MUX_PRIMARY:
		gius_reg &= ~(1L << index);
		gpr_reg &= ~(1L << index);
		break;
	case GPIO_MUX_ALT:
		gius_reg &= ~(1L << index);
		gpr_reg |= (1L << index);
		break;
	case GPIO_MUX_GPIO:
		gius_reg |= (1L << index);
		_gpio_set_ocr(port, index, GPIO_OUTPUT_DR);
		break;
	case GPIO_MUX_INPUT1:
		gius_reg |= (1L << index);
		_gpio_set_iconfa(port, index, GPIO_INPUT_GPIO);
		break;
	case GPIO_MUX_INPUT2:
		gius_reg |= (1L << index);
		_gpio_set_iconfb(port, index, GPIO_INPUT_GPIO);
		break;
	case GPIO_MUX_OUTPUT1:
		gius_reg |= (1L << index);
		_gpio_set_ocr(port, index, GPIO_OUTPUT_A);
		break;
	case GPIO_MUX_OUTPUT2:
		gius_reg |= (1L << index);
		_gpio_set_ocr(port, index, GPIO_OUTPUT_B);
		break;
	case GPIO_MUX_OUTPUT3:
		gius_reg |= (1L << index);
		_gpio_set_ocr(port, index, GPIO_OUTPUT_C);
		break;
	default:
		spin_unlock_irqrestore(&gpio_mux_lock, lock_flags);
		return -1;
	}

	__raw_writel(gius_reg, port->base + GPIO_GIUS);
	__raw_writel(gpr_reg, port->base + GPIO_GPR);

	spin_unlock_irqrestore(&gpio_mux_lock, lock_flags);
	return 0;
}



/*
 * End of stolen functions from kernel
 *
 *
 */




/*****************************************/
/* Hardware management utility functions */
/*****************************************/

/* Enable/disable the clock to the SPI interface */

static void imx27_spi_clk_ctrl(int enable)
{
	unsigned long reg;

	KDEBUG(TRANSPORT, "ENTRY");
	reg = __raw_readl(CCM_PCCR0);
	if (enable)
		reg |= CCM_PCCR0_CSPI1_MASK;
	else
		reg &= ~CCM_PCCR0_CSPI1_MASK;
	__raw_writel(reg, CCM_PCCR0);
	KDEBUG(TRANSPORT, "EXIT");
}



static void imx27_spi_slow_clock(void)
{
  unsigned long ctrl_reg;

  /* Keep host SPI hardware reset */
  ctrl_reg = read_32(MXC_CSPICTRL);

  write_32(MXC_CSPICTRL, ctrl_reg & ~MXC_CSPICTRL_ENABLE);

  /* Program and start host SPI hardware */
  ctrl_reg &= ~ ((0x3 << MXC_CSPICTRL_CSSHIFT_0_0)	/* Chip select */
		 | (0x3 << MXC_CSPICTRL_DRCTRLSHIFT_0_0)/* Data ready ctl */
		 | (0x1f << MXC_CSPICTRL_DATASHIFT)		/* Data rate */
		 | (0x1f << MXC_CSPICTRL_BCSHIFT_0_0)); /* Bit count */
  ctrl_reg |= (0x0 << MXC_CSPICTRL_CSSHIFT_0_0);		/* use ~ as Chip Select */
  ctrl_reg |= (0x0 << MXC_CSPICTRL_DRCTRLSHIFT_0_0);	/* don't care about ~SPI_RDY */
  ctrl_reg |= (0xE << MXC_CSPICTRL_DATASHIFT);		/* divide clock by 256 (reg value= 0xe)  */
  ctrl_reg |= (0x7 << MXC_CSPICTRL_BCSHIFT_0_0);		/* set bit count to 8 */
  ctrl_reg |= MXC_CSPICTRL_LOWSSPOL;	/* Chip Select (SS) is active low */
  ctrl_reg &= ~MXC_CSPICTRL_SSCTL;	/* leave Chip Select asserted between bursts */
  ctrl_reg |= MXC_CSPICTRL_NOPHA;		/* SCLK phase 0 */
  ctrl_reg |= MXC_CSPICTRL_HIGHPOL;	/* SCLK is low when the bus is idle */
  ctrl_reg |= MXC_CSPICTRL_MASTER;	/* operate as a master */
  ctrl_reg |= MXC_CSPICTRL_ENABLE; 	/* Enable, exit reset state */
  ctrl_reg |= MXC_CSPICTRL_BURST;		/* Burst mode, idle time between data transfers */
  ctrl_reg |= MXC_CSPICTRL_NOSWAP;	/* Don't byte swap */
  
  
 
    
  write_32(MXC_CSPICTRL, ctrl_reg);
  //wait for clock to stabilize
  mdelay(10);
  KDEBUG(TRACE, "EXIT johe");
}

void imx27_spi_normal_clock(void)
{
  unsigned long ctrl_reg;

  /* Keep host SPI hardware reset */
  ctrl_reg = read_32(MXC_CSPICTRL);

 write_32(MXC_CSPICTRL, ctrl_reg & ~MXC_CSPICTRL_ENABLE);

  /* Program and start host SPI hardware */
  ctrl_reg &= ~ ((0x3 << MXC_CSPICTRL_CSSHIFT_0_0)	/* Chip select */
		 | (0x3 << MXC_CSPICTRL_DRCTRLSHIFT_0_0)/* Data ready ctl */
		 | (0x1f << MXC_CSPICTRL_DATASHIFT)		/* Data rate */
		 | (0x1f << MXC_CSPICTRL_BCSHIFT_0_0)); /* Bit count */
  ctrl_reg |= (0x0 << MXC_CSPICTRL_CSSHIFT_0_0);		/* use ~ as Chip Select */
  ctrl_reg |= (0x0 << MXC_CSPICTRL_DRCTRLSHIFT_0_0);	/* don't care about ~SPI_RDY */
  ctrl_reg |= (0x2 << MXC_CSPICTRL_DATASHIFT);		/* divide by 4 to generate SCLK at 6.25 MHz */
  ctrl_reg |= (0x7 << MXC_CSPICTRL_BCSHIFT_0_0);		/* set bit count to 8 */
  ctrl_reg |= MXC_CSPICTRL_LOWSSPOL;	/* Chip Select (SS) is active low */
  ctrl_reg &= ~MXC_CSPICTRL_SSCTL;	/* leave Chip Select asserted between bursts */
  ctrl_reg |= MXC_CSPICTRL_NOPHA;		/* SCLK phase 0 */
  ctrl_reg |= MXC_CSPICTRL_HIGHPOL;	/* SCLK is low when the bus is idle */
  ctrl_reg |= MXC_CSPICTRL_MASTER;	/* operate as a master */
  ctrl_reg |= MXC_CSPICTRL_ENABLE; 	/* Enable, exit reset state */
  ctrl_reg |= MXC_CSPICTRL_BURST;		/* Burst mode, idle time between data transfers */
  ctrl_reg |= MXC_CSPICTRL_NOSWAP;	/* Don't byte swap */
  
  
 
    
  write_32(MXC_CSPICTRL, ctrl_reg);

  //wait for clock to stabilize
  mdelay(10);

  KDEBUG(TRACE, "EXIT johe");
}

/*
 *	Allocate and de-allocate pins
 */



typedef struct pin_list_entry_t
{
	iomux_pin_name_t id;
	gpio_mux_mode_t pin_cfg;
	char		     name[30];
} pin_list_entry_t;

#define PIN_LST_ENT(id, pin_cfg) { id, pin_cfg, #id }

static const pin_list_entry_t spi_pin_list[] = {
	PIN_LST_ENT(MX27_PIN_CSPI1_MISO,	GPIO_MUX_PRIMARY),
	PIN_LST_ENT(MX27_PIN_CSPI1_MOSI,	GPIO_MUX_PRIMARY),
	PIN_LST_ENT(MX27_PIN_CSPI1_SCLK,	GPIO_MUX_PRIMARY),
	// Defined as TARGET_IRQ_PIN	PIN_LST_ENT(MX27_PIN_CSPI1_RDY,		OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC),
	// This IRQ is not directly related to the SPI transfers, instead it is
	// a communication from the chip to the driver.
	PIN_LST_ENT(TARGET_IRQ_PIN, 		GPIO_MUX_PRIMARY),
	PIN_LST_ENT(MX27_PIN_CSPI1_SS1, GPIO_MUX_GPIO), // WIFI_RESET_N
	
	//seems like this pin is already configured
	PIN_LST_ENT(MX27_PIN_CSPI1_SS0, GPIO_MUX_GPIO), // WIFI_SPI_CS configured as GPIO
	//PIN_LST_ENT(MX27_PIN_CSPI1_SS0,		GPIO_MUX_PRIMARY),
	
	PIN_LST_ENT(MX27_PIN_USBH2_NXT, GPIO_MUX_GPIO) //WIFI_SHUTDOWN_N/WIFI_WAKEUP
	

};

static int imx27_spi_alloc_pins(void)
{
	int status = 0;

	KDEBUG(TRANSPORT, "ENTRY");

	//#ifdef USE_PINLIST
	{
		int count = sizeof(spi_pin_list) / sizeof(spi_pin_list[0]);
		const pin_list_entry_t *p_entry = spi_pin_list;
		gpio_free_mux(MX27_PIN_CSPI1_MISO);
		gpio_free_mux(MX27_PIN_CSPI1_MOSI);
		gpio_free_mux(MX27_PIN_CSPI1_SCLK);

		while (count--) {
			status = gpio_request_mux(p_entry->id, p_entry->pin_cfg);
			if (status) {
				TRSP_MSG("mxc_request_mux() for %s failed (status: %d)\n",
						 p_entry->name, status);
				break;
			}
			p_entry++;
		}
	}

#if 0 
	// Use function in linux/arch/arm/mach-mx27/mx27ads_gpio.c
	gpio_spi_active(0); // Activate CSPI1 pins

#endif
	KDEBUG(TRANSPORT, "EXIT");
	return status;
}



static void imx27_spi_release_pins(void)
{
	KDEBUG(TRANSPORT, "ENTRY");

	//#ifdef USE_PINLIST
	  {
	  int count = sizeof(spi_pin_list) / sizeof(spi_pin_list[0]);
	  const pin_list_entry_t *p_entry = spi_pin_list;
	  
	  while (count--) {
	    gpio_free_mux(p_entry->id);
	    p_entry++;
	  }
	  }

#if 0
	// Use function in linux/arch/arm/mach-mx27/mx27ads_gpio.c
	gpio_spi_inactive(0); // Deactivate CSPI1 pins
#endif
	KDEBUG(TRANSPORT, "EXIT");
}


/*
 * Set up GPIO IRQ
 */

static void imx27_prog_gpio_irq(void)
{
	// FIXME Perhaps it would be appropriate to take a lock here
	// to protect the register values

	uint32_t icr = read_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET +
			(GPIO_IRQ_CFG_REG2 ? GPIO_ICR2 : GPIO_ICR1)) & ~GPIO_IRQ_CFG_MASK;
	uint32_t psr = read_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET + GPIO_SSR);

	/* An IRQ is triggered on both edges of the GPIO pin,
	 * so set IRQ edge polarity to anticipate the next transition.
	 */
	if (psr & GPIO_TARGET_IRQ_PSR_MASK){
		KDEBUG(TRANSPORT, "GPIO_IRQ_CFG_FALLING_EDGE");
		icr |= GPIO_IRQ_CFG_FALLING_EDGE;
	}else{
		KDEBUG(TRANSPORT, "GPIO_IRQ_CFG_RISING_EDGE");
		icr |= GPIO_IRQ_CFG_RISING_EDGE;
	}
	write_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET +
			(GPIO_IRQ_CFG_REG2 ? GPIO_ICR2 : GPIO_ICR1), icr);
}


/*************************************************************************/
/*                Data transfer                                          */
/*************************************************************************/

#ifdef WIFI_DEBUG_ON
static int spi_recursive_protect = 0;
#endif


//johe: increased the busy wait timeout from 50 to 5000  else it wont work with slow bitrates
#define BUSY_WAIT_TIMEOUT_VALUE 5000 /* Number of BUSY_DELAY */
#define BUSY_DELAY_US 1 /* 5uS  */

unsigned int  spi_senddata_nodma(unsigned char* data, unsigned int size)
{
    volatile unsigned int uTxNumBytes = 0;
    volatile unsigned char fifo_cnt = 0;
    volatile unsigned char dummy = 0xFF;
	volatile unsigned long timeout;

	KDEBUG(TRANSPORT, "ENTRY size=%d", size);


#ifdef WIFI_DEBUG_ON
	TRSP_ASSERT(spi_recursive_protect == 0);
	spi_recursive_protect++;

    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_TXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_TE) != 0);
    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_RR) == 0);
    TRSP_ASSERT((read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH) == 0);
#endif

	do {
		//// FILL TXFIFO
		do{
			 write_32(MXC_CSPITXDATA,*(volatile unsigned char *)data); /// write the data to fifo
			 data++;
			 uTxNumBytes++;
			 fifo_cnt = read_32(MXC_CSPITEST) & (MXC_CSPITEST_TXCNTMASK); /// read TXFIFO counter
		} while( (fifo_cnt < 7) & ( uTxNumBytes < size ) ); /// stop if we fill the fifo or we don't have any other data


		// Start the transfer
		TRSP_ASSERT((read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH ) == 0);
		or_32(MXC_CSPICTRL, MXC_CSPICTRL_XCH);
		read_32(MXC_CSPICTRL); /* Dummy read */

		// Wait for transfer to complete
		timeout = BUSY_WAIT_TIMEOUT_VALUE;
		while(read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH){
			 udelay(BUSY_DELAY_US);
			 if(!timeout) {
				 TRSP_ASSERT(0);
				 goto exit;
			 }
			 timeout --;
		}

		// Purge read data
		while(read_32(MXC_CSPIINT) & MXC_CSPIINT_RR) {
			dummy = read_32(MXC_CSPIRXDATA); // Dummy read of any 0xFF to empty
		}
	    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) == 0);

	} while (uTxNumBytes < size);


exit:
#ifdef WIFI_DEBUG_ON
    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_TXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_TE) != 0);
    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_RR) == 0);
    TRSP_ASSERT((read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH) == 0);

    spi_recursive_protect--;
	TRSP_ASSERT(spi_recursive_protect == 0);
#endif

	KDEBUG(TRANSPORT, "EXIT");

    return 0;
}

unsigned int spi_readdata_nodma(unsigned char* data, unsigned int size)
{
    volatile unsigned int numberofLoops = 0;
    volatile unsigned int bytes_to_read = 0;
    volatile unsigned int remaining_bytes = 0;
	volatile unsigned long timeout;
    unsigned int i,j;
    volatile unsigned char fifo_cnt = 0;
    const unsigned char nop = 0xFF;

    KDEBUG(TRANSPORT, "ENTRY size=%d", size);
 

#ifdef WIFI_DEBUG_ON

	TRSP_ASSERT(spi_recursive_protect == 0);
	spi_recursive_protect++;

    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_TXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_TE) != 0);
    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_RR) == 0);
    TRSP_ASSERT((read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH) == 0);
#endif

    numberofLoops = size/8;

	if(size%8)
		numberofLoops ++;

    remaining_bytes = size;

    for(i = 0 ; i < numberofLoops; i++) {
    	if(remaining_bytes > 8){
    		bytes_to_read = 8;
    	}else{
    		bytes_to_read = remaining_bytes;
    	}

    	// Write nop data to the fifo
		for(j=0;j<bytes_to_read;j++){
			write_32(MXC_CSPITXDATA,nop);
        } 
		

		// Start the transfer
		TRSP_ASSERT((read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH ) == 0);
		or_32(MXC_CSPICTRL, MXC_CSPICTRL_XCH);
		read_32(MXC_CSPICTRL); /* Dummy read */

		// Wait for transfer to complete
		timeout = BUSY_WAIT_TIMEOUT_VALUE;
		while(read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH){
			udelay(BUSY_DELAY_US);
			if(!timeout) {
				 TRSP_ASSERT(0);
				 goto exit;
			}
			timeout --;
		}

		// Read received data
        do {
			 fifo_cnt = ((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) >> 4); // read the # of RXFIFO

			 if(fifo_cnt > 0) {
				 *data = read_32(MXC_CSPIRXDATA) & MXC_CSPIRXDATA_MASK;
				 data++;
				 remaining_bytes--;
				 bytes_to_read--;
			 }
        }while(bytes_to_read);
		TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) == 0);
   }

exit:
#ifdef WIFI_DEBUG_ON
    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_TXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_TE) != 0);
    TRSP_ASSERT((read_32(MXC_CSPITEST) & MXC_CSPITEST_RXCNTMASK) == 0);
    TRSP_ASSERT((read_32(MXC_CSPIINT) & MXC_CSPIINT_RR) == 0);
    TRSP_ASSERT((read_32(MXC_CSPICTRL) & MXC_CSPICTRL_XCH) == 0);

    spi_recursive_protect--;
	TRSP_ASSERT(spi_recursive_protect == 0);
#endif

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

	/* Map physical I/O memory range into logical address space */
	IMX27_GPIO_BASE = (volatile unsigned long) ioremap(GPIO_BASE_ADDR, 0x18 /*24*/);

	if(!IMX27_GPIO_BASE) {
		TRSP_MSG("ioremap() for IMX27_GPIO_BASE failed\n");
	 return -EINVAL;
	}

	printk("GPIO_BASE_ADDR physical address = 0x%08x\n",  GPIO_BASE_ADDR);
	printk("IMX27_GPIO_BASE logical address = 0x%08lx\n",  IMX27_GPIO_BASE);
	printk("TARGET_IRQ_PIN_NUM = 0x%08x\n", TARGET_IRQ_PIN_NUM);
	printk("ICR address 0x%08x\n",GPIO_TARGET_IRQ_PORT_REG_OFFSET + (GPIO_IRQ_CFG_REG2 ? GPIO_ICR2 : GPIO_ICR1));
	printk("CSPI_VIRT_ADDR 0x%08x\n",CSPI_VIRT_ADDR);

	ctrl_reg = *(u32*)CCM_PCDR1;
	printk("CCM_PCDR1 logical address 0x%08x = 0x%08x\n", CCM_PCDR1, ctrl_reg);

	ctrl_reg = *(u32*)CCM_PCCR0;
	printk("CCM_PCCR0 logical address 0x%08x = 0x%08x\n", CCM_PCCR0, ctrl_reg);

	ctrl_reg = *(u32*)(0xd4013008);
	printk("SDHC CLK_RATE1 logical address 0x%08x = 0x%08x\n", 0xd4013008, ctrl_reg);
	ctrl_reg = *(u32*)(0xd4014008);
	printk("SDHC CLK_RATE2 logical address 0x%08x = 0x%08x\n", 0xd4014008, ctrl_reg);


	/* Keep host SPI hardware reset */
	ctrl_reg = read_32(MXC_CSPICTRL);
	write_32(MXC_CSPICTRL, ctrl_reg & ~MXC_CSPICTRL_ENABLE);

	/* Allocate pins for the CSPI1 interface */
	status = imx27_spi_alloc_pins();
	if (status) {
		TRSP_MSG("imx27_spi_pin_alloc() failed\n");
		return -EIO;
	}





	/* Enable clock to the CSPI1 interface */
	imx27_spi_clk_ctrl(1);
	mdelay(1);

	/* Program and start host SPI hardware */
	ctrl_reg &= ~ ((0x3 << MXC_CSPICTRL_CSSHIFT_0_0)	/* Chip select */
				 | (0x3 << MXC_CSPICTRL_DRCTRLSHIFT_0_0)/* Data ready ctl */
				 | (0x1f << MXC_CSPICTRL_DATASHIFT)		/* Data rate */
				 | (0x1f << MXC_CSPICTRL_BCSHIFT_0_0)); /* Bit count */
	ctrl_reg |= (0x0 << MXC_CSPICTRL_CSSHIFT_0_0);		/* use ~ as Chip Select */
	ctrl_reg |= (0x0 << MXC_CSPICTRL_DRCTRLSHIFT_0_0);	/* don't care about ~SPI_RDY */
	ctrl_reg |= (0x2 << MXC_CSPICTRL_DATASHIFT);		/* divide by 4 to generate SCLK at 6.25 MHz */
	ctrl_reg |= (0x7 << MXC_CSPICTRL_BCSHIFT_0_0);		/* set bit count to 8 */
	ctrl_reg |= MXC_CSPICTRL_LOWSSPOL;	/* Chip Select (SS) is active low */
	ctrl_reg &= ~MXC_CSPICTRL_SSCTL;	/* leave Chip Select asserted between bursts */
	ctrl_reg |= MXC_CSPICTRL_NOPHA;		/* SCLK phase 0 */
	ctrl_reg |= MXC_CSPICTRL_HIGHPOL;	/* SCLK is low when the bus is idle */
	ctrl_reg |= MXC_CSPICTRL_MASTER;	/* operate as a master */
	ctrl_reg |= MXC_CSPICTRL_ENABLE; 	/* Enable, exit reset state */
	ctrl_reg |= MXC_CSPICTRL_BURST;		/* Burst mode, idle time between data transfers */
	ctrl_reg |= MXC_CSPICTRL_NOSWAP;	/* Don't byte swap */
	TRSP_MSG("Setting up MXC_CSPICTRL = 0x%08x\n",  ctrl_reg);
	write_32(MXC_CSPICTRL, ctrl_reg);


	// assert that the signal to wifi reset is high since it is connected to a gpio
	// set to output

	or_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS1))) + GPIO_GDIR,
	1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS1)));

	// set reset low
	and_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS1))) + GPIO_DR,
	~(1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS1))));
	

	/* Disable interrupt from target and prepare to receive IRQ */
	//MX27 and_gpio_32(GPIO_IMR,~GPIO_MASK);
	and_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET + GPIO_IMR, ~GPIO_TARGET_IRQ_IMR_MASK);


	// Set up CS through GPIO


	or_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0))) + GPIO_GDIR,
	1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0)));

	and_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0))) + GPIO_DR,
		    ~(1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0))));

	//
	// Set up Wake Up through GPIO
	//



	// Set as output
	or_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_USBH2_NXT))) + GPIO_GDIR,
		   1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_USBH2_NXT)));
	
	// Set low
	and_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_USBH2_NXT))) + GPIO_DR,
		~(1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_USBH2_NXT))));
		mdelay(20);
	mdelay(50);
	or_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_USBH2_NXT))) + GPIO_DR,
		   (1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_USBH2_NXT)))
		   );
	mdelay(50);


	// set reset high
		or_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS1))) + GPIO_DR,
	(1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS1))));
	


	/* GPIO in use for IRQ pin */
	or_gpio_32(GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_RDY))) + GPIO_IUS,
	(1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_RDY))));

	{
		u32 x,y,z;
		x = GPIO_PORT_REG_OFFSET(GPIO_TO_PORT(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0))) + GPIO_DR;
		y = 1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0));
		z = ~(1 << GPIO_TO_INDEX(IOMUX_TO_GPIO(MX27_PIN_CSPI1_SS0)));
		TRSP_MSG("Setting up GPIO_GDIR to  0x%08x = 0x%08x DR &= 0x%08x\n", x, y, z);
	}

	//Dump GPIO in use reg
	{
		u32 port, reg;
		for(port = 0; port < 6; port++){
			reg = read_gpio_32(GPIO_PORT_REG_OFFSET(port) + GPIO_IUS);
			TRSP_MSG("GPIO in use %d: 0x%08x = 0x%08x\n",
					port, GPIO_PORT_REG_OFFSET(port) + GPIO_IUS, reg);
		}
	}

	//Dump GPIO DDIR reg
	{
		u32 port, reg;
		for(port = 0; port < 6; port++){
			reg = read_gpio_32(GPIO_PORT_REG_OFFSET(port) + GPIO_GDIR);
			TRSP_MSG("GPIO Data dir %d: 0x%08x = 0x%08x\n",
					port, GPIO_PORT_REG_OFFSET(port) + GPIO_GDIR, reg);
		}
	}
	//Dump GPIO Data reg
	{
		u32 port, reg;
		for(port = 0; port < 6; port++){
			reg = read_gpio_32(GPIO_PORT_REG_OFFSET(port) + GPIO_DR);
			TRSP_MSG("GPIO Data reg %d: 0x%08x = 0x%08x\n",
					port, GPIO_PORT_REG_OFFSET(port) + GPIO_DR, reg);
		}
	}

	imx27_prog_gpio_irq();

	KDEBUG(TRANSPORT, "EXIT");
	return 0;
}


int host_interrupt(irq_op_t irq_op)
{
	uint32_t isr;

	KDEBUG(TRANSPORT, "ENTRY");

	switch (irq_op) {
		case IRQ_OP_ENABLE:			
			or_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET + GPIO_IMR, GPIO_TARGET_IRQ_IMR_MASK);
			break;

		case IRQ_OP_DISABLE:
			and_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET + GPIO_IMR, ~GPIO_TARGET_IRQ_IMR_MASK);
			imx27_prog_gpio_irq(); /* Prepare for the next IRQ */
			break;

		case IRQ_OP_STATUS:
			isr = read_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET + GPIO_ISR) ;
			return (isr & GPIO_TARGET_IRQ_ISR_MASK);

		case IRQ_OP_ACKNOWLEDGE:
			or_gpio_32(GPIO_TARGET_IRQ_PORT_REG_OFFSET + GPIO_ISR, GPIO_TARGET_IRQ_ISR_MASK);
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

	KDEBUG(TRANSPORT, "EXIT, Response to CMD0: 0x%02x\n", response);
	//johe
	TRSP_MSG( "EXIT, Response to CMD0: 0x%02x\n", response);
	
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

      if (use_slow_clock != 0)
      {
	imx27_spi_slow_clock();
	spi_senddata_nodma(cmd52, sizeof(cmd52));
	imx27_spi_normal_clock();

      }
      else
      {
	spi_senddata_nodma(cmd52, sizeof(cmd52));
      }

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
 
	KDEBUG(TRANSPORT, "IGNORED");
}

void host_exit(void)
{
	imx27_spi_release_pins();
	imx27_spi_clk_ctrl(0);
}




