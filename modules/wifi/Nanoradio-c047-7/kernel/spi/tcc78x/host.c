#if 1
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
//#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/proc_fs.h>


#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#endif
#include <asm/arch/tcc78x.h>


#include "tcc78x_gpsb.h"
#include "tcc78x_ioport.h"
#include "tcc78x_clk.h"
#include "tcc78x_intr.h"



#include "host.h"

#include "nanoutil.h"

#define TCC78X_MAGIC_VERSION "nanoradio spi wifi driver magic version 1.spi.rc1"
#define SPI_BIT_WIDTH 8
#define TCC78X_SPI_BASE_ADDRESS 0xF0057800
#define TCC78X_SPI_REG_SIZE     sizeof(TCC78X_GPSB_REG)

#define TCC78X_IOPORT_ADDRESS      0xF005A000
#define TCC78X_IOPORT_REG_SIZE     sizeof(TCC78X_IOPORT_REG)
#define TCC78X_CLK_ADDRESS      0xF3000000
#define TCC78X_CLK_REG_SIZE     sizeof(TCC78X_CLK_REG)
#define TCC78X_INT_ADDRESS      0xF3001000
#define TCC78X_INT_REG_SIZE     sizeof(TCC78X_INTR_REG)

static void         gpio_initialize_target_reset_pin(void);
static void         gpio_initialize_spi_clock_pin(void);
static void         gpio_initialize_spi_cs_pin(void);
static void         gpio_initialize_interrupt_pin(void);
static void         gpio_initialize_spi_rx_pin(void);
static void         gpio_initialize_spi_tx_pin(void);
static void         gpio_initialize_target_shutdown_pin(void);
#if 0
static void tcc78x_wifi_startup_sequence(void);
#endif
void                gpio_turn_on_target_power(void);
#if 0
static void         gpio_initialize_power_crl_pin(void);
static void         gpio_uninitialize_target_reset_pin(void);
static void         gpio_uninitialize_spi_clock_pin(void);
static void         gpio_uninitialize_spi_cs_pin(void);
static void         gpio_uninitialize_interrupt_pin(void);
static void         gpio_uninitialize_spi_rx_pin(void);
static void         gpio_uninitialize_spi_tx_pin(void);
static void         gpio_uninitialize_power_crl_pin(void);
static void         gpio_uninitialize_target_shutdown_pin(void);
#endif
void                gpio_set_analog_reset_pinHigh(void);
void                gpio_set_digital_reset_pinHigh(void);
unsigned int        gpio_initialize(void);


typedef enum { false, true } __attribute__ ((packed)) boolean;

volatile TCC78X_GPSB_REG *s_pGpsbRegs = NULL;
volatile TCC78X_CLK_REG *s_pClkRegs = NULL;
volatile TCC78X_IOPORT_REG *g_pGPIORegs = NULL;
static volatile TCC78X_INTR_REG *s_pIntrRegs = NULL;

volatile int g_IsReady = 0;
volatile unsigned int    CS_Forced_HIGH;


int
host_init(void)
{
  volatile unsigned long ioaddr1;
  volatile unsigned long ioaddr2;
  volatile unsigned long ioaddr3;
  volatile unsigned long ioaddr4;
	////KDEBUG(TRANSPORT, "ENTRY");       

 printk("%s\n", TCC78X_MAGIC_VERSION);
 ioaddr1 = (volatile unsigned long)ioremap(TCC78X_SPI_BASE_ADDRESS, TCC78X_SPI_REG_SIZE);
 if(!ioaddr1) {
  printk("ioremap failed for SPI SLOT 1\n");
  return -1;
 }

 //printk("spi ioaddr : 0x%x\n", (unsigned int)ioaddr1);
 s_pGpsbRegs = (volatile TCC78X_GPSB_REG *)ioaddr1;

 ioaddr2 = (volatile unsigned long)ioremap(TCC78X_CLK_ADDRESS, TCC78X_CLK_REG_SIZE);
 if(!ioaddr2) {
  printk("ioremap failed for SPI SLOT 1\n");
  return -1;
 }

 //printk("clk ioaddr : 0x%x\n", (unsigned int)ioaddr2);
 s_pClkRegs = (volatile TCC78X_CLK_REG *)ioaddr2;

 ioaddr3 = (volatile unsigned long)ioremap(TCC78X_IOPORT_ADDRESS, TCC78X_IOPORT_REG_SIZE);
 if(!ioaddr3) {
  printk("ioremap failed for SPI SLOT 1\n");
  return -1;
 }

 //printk("ioport ioaddr : 0x%x\n", (unsigned int)ioaddr3);
 g_pGPIORegs = (volatile TCC78X_IOPORT_REG *)ioaddr3;

 ioaddr4 = (volatile unsigned long)ioremap(TCC78X_INT_ADDRESS, TCC78X_INT_REG_SIZE);
 if(!ioaddr4) {
  printk("ioremap failed for SPI SLOT 1\n");
  return -1;
 }

// printk("intr ioaddr : 0x%x\n", (unsigned int)ioaddr4);
 s_pIntrRegs = (volatile TCC78X_INTR_REG *)ioaddr4;

#if 0
 printk("PORTCFG3 : 0x%x\n",  g_pGPIORegs->PORTCFG3);
 printk("SPIS : 0x%x\n",  s_pClkRegs->PCK_SPIS);
#endif

#if 0
 sema_init(&host_mutex, 1);
#endif
 // GPIO INIT BLOCK ===>>>
 
 irq_initialize();

 // GPIO INIT BLOCK ===>>>
 
 mdelay(500);

 gpio_initialize();

 mdelay(500);
 // SPI INIT BLOCK ===>>>
 //Turns clock signal for SPI port on
 s_pClkRegs->SWRESET |= Hw15;
 mdelay(100);
 s_pClkRegs->BCLKCTR |= Hw15;
 mdelay(100);

 // Enable GPSB clock
 s_pClkRegs->PCK_SPIS = 0;
 mdelay(100);
 s_pClkRegs->PCK_SPIS = 0x10000003;  // PLL0 (196.8MHz) : 98.4MHz % 4 = 24.6MHz
 //s_pClkRegs->PCK_SPIS = 0x10000007;  // PLL0 (196.8MHz) : 98.4MHz % 8 = 12.6MHz
 //s_pClkRegs->PCK_SPIS = 0x14000000;  // 12MHz Direct : 6MHz % 2 = 3MHz
 //s_pClkRegs->PCK_SPIS = 0x10000008;  // PLL0 (196.8MHz) : 98.4MHz % 9 = 10.93MHz
 //s_pClkRegs->PCK_SPIS = 0x10000006;  // PLL0 (196.8MHz) : 98.4MHz % 7 = 14.08MHz
 //s_pClkRegs->PCK_SPIS = 0x10000004;  // PLL0 (196.8MHz) : 98.4MHz % 5 = 19.68MHz
 mdelay(100);


 s_pClkRegs->SWRESET &= ~Hw15;
 
 mdelay(500);

 mdelay(100);
 // SPI INIT BLOCK ===>>>
 // SPI CLOCK BLOCK ===>>>
 s_pGpsbRegs->MODE &= ~Hw3;
 g_pGPIORegs->GPEDAT |= Hw20; //Chip select high
 // SPI CLOCK BLOCK ===>>>
    s_pGpsbRegs->MODE = ( HwGPSB_CH1MODE_MD_SPI
                        | HwGPSB_CH1MODE_CTF
                        | ((SPI_BIT_WIDTH-1) <<8 )  // Bit Width Select 16bit=0xF+1
                        | HwGPSB_CH1MODE_CWF        // 1 : Clear TX FIFO Counter
                        | HwGPSB_CH1MODE_CRF        // 1 : Clear RX FIFO Counter
                        | (0x00<<24)                // clk : F_GCLK/{(n+1)*2}
                        );

 s_pGpsbRegs->MODE &= ~( HwGPSB_CH1MODE_CWF | HwGPSB_CH1MODE_CRF );
 mdelay(500);


s_pGpsbRegs->STAT = 0;
s_pGpsbRegs->INTEN = 0;
  mdelay(500);

  transport_target_reset();

	////KDEBUG(TRANSPORT, "EXIT");
	return 0;
}

int
host_interrupt(irq_op_t irq_op)
{
  int status = 0;
  switch (irq_op) {
  case IRQ_OP_ENABLE :
  gpio_enable_interrupt();
   break;
  case IRQ_OP_DISABLE :
  gpio_disable_interrupt();
   break;
	case IRQ_OP_ACKNOWLEDGE:
  irq_clear_interrupt();
   break;
	case IRQ_OP_STATUS:
   status = gpio_get_interrupt_pinState();
   return status;
  }
	return 0;
}

int host_read_cmd53(unsigned char* data, unsigned int size)
{
    uint16_t remaining_size = len;
    uint16_t current_byte = 0;
    uint16_t max_size = 511;
    static unsigned char cmd53_read[] = {
        0xff,
        0xff,
        0xff,
        0x75,
        0x10,
        0x00,
        0x00,   // size msb
        0x00,   // size lsb
        0x01,
        0xff,
        0xff, 0xff,
        0xff, 0xff  // preamble
    };
    static unsigned char cmd53_rpost[] =
    {
        0xff, 0xff,  // CRC
        0xff, 0xff,
    };


       // A message to/from the Nanoradio Chip can be up to 1600 bytes but SDIO can only handle max 512 byte at a time
       // Therefore is it nececary to split the message into a number of SDIO reads/writes
       while(remaining_size)
       {
           uint16_t size_to_read = remaining_size;
           if(size_to_read > max_size) size_to_read = max_size;

           //Write the size to the CMD53 header
           cmd53_read[6] = (unsigned char) ((size_to_read & 511)>>8);
           cmd53_read[7] = (unsigned char) size_to_read & 0xFF;


           spi_start();

           //writes the SDIO header
           spi_senddata_nodma(cmd53_read, sizeof(cmd53_read));
           //reads the data
           spi_readdata_nodma(&data[current_byte], size_to_read);
           //writes the SDIO CRC
           spi_senddata_nodma(cmd53_rpost, sizeof(cmd53_rpost));

           spi_stop();

           remaining_size -= size_to_read;
           current_byte += size_to_read;
       }
  return 0;
}

int host_send_cmd53(unsigned char* data, unsigned int size)
{
    uint16_t remaining_size = len;
    uint16_t current_byte = 0;
    uint16_t max_size = 511;
    static unsigned char cmd53_write[] =
    {
        0xff,
        0xff,
        0xff,
        0x75,
        0x90,
        0x00,
        0x00,   // size msb
        0x00,   // size lsb
        0x01,
        0xff,
        0xff, 0xff,
        0xff, 0xfe  // preamble
    };
    static unsigned char cmd53_tpost[] =
    {
        0x00, 0x00,  // CRC
        0xff, 0xff,
    };

       while(remaining_size)
       {
           uint16_t size_to_send = remaining_size;
           if(size_to_send > max_size) size_to_send = max_size;

           //Write the size to the CMD53 header
           cmd53_write[6] = (unsigned char) ((size_to_send & 511)>>8);
           cmd53_write[7] = (unsigned char) size_to_send & 0xFF;


           spi_start();

           spi_senddata_nodma(cmd53_write, sizeof(cmd53_write));
           spi_senddata_nodma(&data[current_byte], size_to_send);
           spi_senddata_nodma(cmd53_tpost, sizeof(cmd53_tpost));
           
           spi_stop();

           remaining_size -= size_to_send;
           current_byte += size_to_send;
       }
  return 0;
}

/* phth: Obsolete GPIO_ID_RESET_D and GPIO_ID_RESET_A removed.
 * nrx710C has only one reset input, represented as GPIO_ID_RESET
 */
#ifdef LGIT_MODULE
void
host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
#ifndef JTAG_CONNECTION
    //GPIO_A18 -WIFI_RSTD 
	//GPIO_A19 -WIFI_RSTA
	//GPIO_A20 -WIFI_1.2V_ON 
	//GPIO_A21 -WIFI_3.3V_ON 
    switch(gpio_op) {
    case GPIO_OP_DISABLE:
	       break; 
    case GPIO_OP_ENABLE:
	       break;
    case GPIO_OP_HIGH:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPAEN |= (1 << 18);
              g_pGPIORegs->GPADAT |= (1 << 18);
              g_pGPIORegs->GPASET |= (1 << 18);
         } else if(gpio_id == GPIO_ID_RESET_A) {
              g_pGPIORegs->GPAEN |= (1 << 19);
              g_pGPIORegs->GPADAT |= (1 << 19);
              g_pGPIORegs->GPASET |= (1 << 19);
         } else if(gpio_id == GPIO_ID_POWER) {
              g_pGPIORegs->GPAEN |= (1 << 21);
              g_pGPIORegs->GPAEN |= (1 << 20);
              g_pGPIORegs->GPADAT |= (1 << 21);
              g_pGPIORegs->GPADAT |= (1 << 20);
              g_pGPIORegs->GPASET |= (1 << 21);
              g_pGPIORegs->GPASET |= (1 << 20);
         }
	  break;
    case GPIO_OP_LOW:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPADAT &= ~(1 << 18);
              mdelay(10);
              g_pGPIORegs->GPAEN &= ~(1 << 18);
              mdelay(10);
         } else if(gpio_id == GPIO_ID_RESET_A) {
              g_pGPIORegs->GPADAT &= ~(1 << 19);
              mdelay(10);
              g_pGPIORegs->GPAEN &= ~(1 << 19);
              mdelay(10);
         } else if(gpio_id == GPIO_ID_POWER) {
              g_pGPIORegs->GPADAT &= ~(Hw21 | Hw20);
              mdelay(10);
              g_pGPIORegs->GPADAT &= ~(Hw21 | Hw20);
              mdelay(10);
         }
	  break;
    default: HOST_ASSERT(0);

    }


#endif
    return;
}
#else
void
host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
#ifndef JTAG_CONNECTION
    //GPIO_A18 -WIFI_RSTD 
	//GPIO_A19 -WIFI_RSTA
	//GPIO_A20 -WIFI_1.2V_ON 
	//GPIO_A21 -WIFI_3.3V_ON 
    switch(gpio_op) {
    case GPIO_OP_DISABLE:
         if(gpio_id == GPIO_ID_RESET_D) {
           g_pGPIORegs->GPACLR = Hw18;
         } else if(gpio_id == GPIO_ID_RESET_A) {
           g_pGPIORegs->GPACLR = Hw19;
         } else if(gpio_id == GPIO_ID_POWER) {
           g_pGPIORegs->GPACLR = Hw21;
           g_pGPIORegs->GPACLR = Hw20;
         }

	       break; 
    case GPIO_OP_ENABLE:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPAEN |= Hw18;
         } else if(gpio_id == GPIO_ID_RESET_A) {
              g_pGPIORegs->GPAEN |= Hw19;
         } else if(gpio_id == GPIO_ID_POWER){
              g_pGPIORegs->GPAEN |= (Hw21 | Hw20);
         }

	       break;
    case GPIO_OP_HIGH:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPADAT |= Hw18;
         } else if(gpio_id == GPIO_ID_RESET_A) {
              g_pGPIORegs->GPADAT |= Hw19;
         } else if(gpio_id == GPIO_ID_POWER) {
              g_pGPIORegs->GPADAT |= Hw21;
              g_pGPIORegs->GPADAT |= Hw20;
         }
	  break;
    case GPIO_OP_LOW:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPADAT &= ~Hw18;
         } else if(gpio_id == GPIO_ID_RESET_A) {
              g_pGPIORegs->GPADAT &= ~Hw19;
         } else if(gpio_id == GPIO_ID_POWER) {
              g_pGPIORegs->GPADAT &= ~(Hw21 | Hw20);
         }
	  break;
    default: HOST_ASSERT(0);

    }

   // mdelay(10);

#endif
    return;
}
#endif

#if 0
void
host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
#ifndef JTAG_CONNECTION
    //GPIO_A18 -WIFI_RSTD 
	//GPIO_A19 -WIFI_RSTA
	//GPIO_A20 -WIFI_1.2V_ON 
	//GPIO_A21 -WIFI_3.3V_ON 
    switch(gpio_op) {
    case GPIO_OP_DISABLE:
         if(gpio_id == GPIO_ID_RESET_D) {
          g_pGPIORegs->GPAEN &= ~Hw18;
          g_pGPIORegs->PORTCFG1 &= ~Hw18;
         } else if(gpio_id == GPIO_ID_RESET_A) {
          g_pGPIORegs->GPAEN &= ~Hw19;
          g_pGPIORegs->PORTCFG1 &= ~Hw19;
         } else if(gpio_id == GPIO_ID_POWER) {
          g_pGPIORegs->PORTCFG1 &= ~(Hw21 | Hw20);
         }

	       break; 
    case GPIO_OP_ENABLE:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->PORTCFG1 |= Hw18;
              g_pGPIORegs->GPAEN |= Hw18;
         } else if(gpio_id == GPIO_ID_RESET_A) {
              g_pGPIORegs->PORTCFG1 |= Hw19;
         } else if(gpio_id == GPIO_ID_POWER){
              g_pGPIORegs->PORTCFG1 |= (Hw21 | Hw20);
              g_pGPIORegs->GPAEN |= (Hw21 | Hw20);
         }

	       break;
    case GPIO_OP_HIGH:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPADAT |= Hw18;
         } else if(gpio_id == GPIO_ID_POWER) {
              g_pGPIORegs->GPADAT |= (Hw21 | Hw20);
         }
	  break;
    case GPIO_OP_LOW:
         if(gpio_id == GPIO_ID_RESET_D) {
              g_pGPIORegs->GPADAT &= ~Hw18;
         } else if(gpio_id == GPIO_ID_POWER) {
              g_pGPIORegs->GPADAT &= ~(Hw21 | Hw20);
         }
	  break;
    default: HOST_ASSERT(0);

    }

    mdelay(100);
#endif
    return;
}
#endif
void
host_exit(void)
{
   KDEBUG(TRANSPORT, "ENTRY");
	 KDEBUG(TRANSPORT, "EXIT");
	return;
}

unsigned int gpio_initialize(void)
{
    CS_Forced_HIGH = false;
    
        g_pGPIORegs->PORTCFG2 &= ~( Hw14 | Hw15 );

        //gpio_initialize_target_shutdown_pin();
        //gpio_initialize_target_reset_pin();
        gpio_initialize_spi_cs_pin();
        gpio_initialize_spi_clock_pin();        
        gpio_initialize_interrupt_pin();
        gpio_initialize_spi_rx_pin();
        gpio_initialize_spi_tx_pin();
 return true;
}

void gpio_set_digital_reset_pinHigh(void)
{
    g_pGPIORegs->GPCSET = Hw26;
}

void gpio_set_digital_reset_pinLow(void)
{
    g_pGPIORegs->GPCCLR = Hw26;
}

void gpio_force_spi_cs_pinHigh(void)
{
    //This function is used for sending SPI-clock-pulses without CS
    //After calling this you must not lower the pin untill 
    // GPIO_SPIcsRestore is called

    //Setting the pin high
    g_pGPIORegs->GPEDAT |= Hw20;
    
    CS_Forced_HIGH = true;
}

void gpio_spi_cs_restore(void)
{
    CS_Forced_HIGH = false;
}

void gpio_turn_on_target_power(void)
{
 g_pGPIORegs->PORTCFG1 |= Hw21;
 g_pGPIORegs->PORTCFG1 &= ~Hw20;
 mdelay(10);
 g_pGPIORegs->PORTCFG1 |= (Hw19 | Hw18);
 mdelay(10);
 g_pGPIORegs->GPADAT |= (Hw21 | Hw20);
 g_pGPIORegs->GPAEN |= (Hw21 | Hw20);
 mdelay(10);
 g_pGPIORegs->GPASET = Hw21;
 g_pGPIORegs->GPASET = Hw20;
 mdelay(500);
}

void gpio_turn_off_target_power(void)
{
}

void gpio_set_analog_reset_pinHigh(void)
{
    g_pGPIORegs->GPADAT |= Hw27;
}

void gpio_set_analog_reset_pinLow(void)
{
    g_pGPIORegs->GPADAT &= ~Hw27;
}

void gpio_enable_interrupt(void)
{
    //Do whatever is needed to enable interrupt (and is that is not done by the OS)
    //irq_clear_interrupt();
    irq_enable_interrupt();
}

void gpio_disable_interrupt(void)
{
    irq_disable_interrupt();
    //irq_clear_interrupt();
}

void gpio_set_cs_pinLow(void)
{
    if(CS_Forced_HIGH) return;
    g_pGPIORegs->GPEDAT &= ~Hw20; //Chip select low
}

void gpio_set_cs_pinHigh(void)
{
    g_pGPIORegs->GPEDAT |= Hw20; //Chip select high
}

unsigned int gpio_get_interrupt_pinState(void)
{
    return  0 != (g_pGPIORegs->GPBDAT & Hw27);
}

static void gpio_initialize_target_reset_pin(void)
{
    //After calling this function _TARGET_ reset-pin should be low.
    
    gpio_set_digital_reset_pinLow();
    mdelay(1);

    //Setting GPIO to out
    g_pGPIORegs->GPCEN |= Hw26;
}

static void gpio_initialize_target_shutdown_pin(void)
{
    gpio_set_analog_reset_pinLow();
    g_pGPIORegs->GPAEN |= Hw27;
}

static void gpio_initialize_spi_clock_pin(void)
{
    g_pGPIORegs->PORTCFG3 &= ~HwPORTCFG3_SCK1_3;
    g_pGPIORegs->PORTCFG3 |= HwPORTCFG3_SCK1_3;
}

static void gpio_initialize_spi_cs_pin(void)
{
    // Initializing CS
    g_pGPIORegs->PORTCFG3 &= ~HwPORTCFG3_FRM1_3;
    g_pGPIORegs->PORTCFG3 |= HwPORTCFG3_FRM1_3;

    g_pGPIORegs->GPESET = Hw20;
    g_pGPIORegs->GPEEN |= Hw20;
    g_pGPIORegs->GPESET = Hw20;
}

static void gpio_initialize_interrupt_pin(void)
{
    //Do whatever is needed to prepare the Interupt PIN

    //Selects function 0=GPIO
//    g_pGPIORegs->GAFR1_L &= ~(3 << 10);    

    //Setting GPIO to in
//    g_pGPIORegs->GPDR1 &= ~(1 << 5 );
}

static void gpio_initialize_spi_rx_pin(void)
{
    g_pGPIORegs->PORTCFG3 &= ~HwPORTCFG3_SDI1_3;
    g_pGPIORegs->PORTCFG3 |= HwPORTCFG3_SDI1_3;
}

static void gpio_initialize_spi_tx_pin()
{
    g_pGPIORegs->PORTCFG3 &= ~HwPORTCFG3_SDO1_3;
    g_pGPIORegs->PORTCFG3 |= HwPORTCFG3_SDO1_3;
}

#if 0
static void gpio_initialize_power_crl_pin(void)
{
    //After calling this function the Power to _TARGET_ should be off
    gpio_turn_off_target_power();

    //g_pGPIORegs->GPEEN &= ~Hw8;  //EXT_PWR_ENABLE as input
    g_pGPIORegs->GPAEN &= ~Hw8;  //EXT_PWR_ENABLE as input
    g_pGPIORegs->PORTCFG3 |= (Hw12 | Hw13); //Set
    g_pGPIORegs->GPACLR = Hw8;  //EXT_PWR_ENABLE low
    //g_pGPIORegs->GPECLR = Hw8;  //EXT_PWR_ENABLE low

    g_pGPIORegs->PORTCFG3 |= (Hw18 | Hw19); //Set pin as GPIO E11
    
    //g_pGPIORegs->GPEEN |= Hw11;
    g_pGPIORegs->GPAEN |= Hw21;
    g_pGPIORegs->GPACLR = Hw21; //Turn off VDD_3.3A
    
    g_pGPIORegs->GPCEN |= Hw20;
    g_pGPIORegs->GPCCLR = Hw20; //Turn off VDD_1.2

    
    g_pGPIORegs->GPAEN |= Hw4;  //WIFI_PWR_ENABLE (WIFI_3.3B) as output
    g_pGPIORegs->GPACLR = Hw4;  //Turn off VDD_3.3B
    
    gpio_turn_off_target_power();
}

#endif
#if 0
static void gpio_uninitialize_target_reset_pin(void)
{
    //Do whatever you need in order to save HOST-power

    if(g_pGPIORegs) {
        //Setting GPIO to in
        g_pGPIORegs->GPCEN &= ~Hw26;
    }
}

static void gpio_uninitialize_spi_clock_pin(void)
{
    //Do whatever you need in order to save HOST-power
    if(g_pGPIORegs) {
        //Setting GPIO to default value
        g_pGPIORegs->PORTCFG3 &= ~HwPORTCFG3_SCK1_3;
    }
}

static void gpio_uninitialize_spi_cs_pin(void)
{
    //Do whatever you need in order to save HOST-power
    if(g_pGPIORegs) {
        //Setting GPIO to in
        g_pGPIORegs->GPEEN &= ~Hw20;
    }
}

static void gpio_uninitialize_interrupt_pin(void)
{
    if(g_pGPIORegs) {
        //Setting GPIO to in
//        g_pGPIORegs->GPDR1 &= ~(1 << 5 );
    }
}

static void gpio_uninitialize_spi_rx_pin(void)
{
    //Do whatever you need in order to save HOST-power
    if(g_pGPIORegs) {
        //Setting GPIO to in
//        g_pGPIORegs->GPDR0 &= ~(1 << 26);
    }
}

static void gpio_uninitialize_spi_tx_pin(void)
{
    //Do whatever you need in order to save HOST-power
    if(g_pGPIORegs) {
        //Setting GPIO to in
        g_pGPIORegs->GPDEN &= ~(1 << 25);
    }
}

static void gpio_uninitialize_power_crl_pin(void)
{
    //Do whatever you need in order to save HOST-power
    if(g_pGPIORegs) {
        g_pGPIORegs->GPAEN &= ~Hw4;
        g_pGPIORegs->GPCEN &= ~Hw30;
        g_pGPIORegs->GPEEN &= ~Hw11;
    }
}

static void gpio_uninitialize_target_shutdown_pin(void)
{
    //Do whatever you need in order to save HOST-power
    if(g_pGPIORegs) {
        g_pGPIORegs->GPAEN &= ~Hw27;
    }
}
#endif

unsigned int irq_initialize()
{


        g_pGPIORegs->PORTCFG0 &= ~(Hw17 | Hw16);

        g_pGPIORegs->GPBEN &= ~Hw27;

        irq_disable_interrupt();

        // Interrupt Select Reg : Set EI1 as IRQ
        s_pIntrRegs->SEL |= HwSEL_EI1;

        s_pIntrRegs->IRQ |= HwSEL_EI1;

        s_pIntrRegs->INTMSK |= HwSEL_EI1;

        s_pIntrRegs->POL &= ~HwSEL_EI1;


        // Intr Mode Register : Set EI1 as edge-trigger
        s_pIntrRegs->MODE &= ~HwMODE_EI1;

        // Intr Mode Register : Set EI1 as both edges
        s_pIntrRegs->MODEA |= HwMODEA_EI1;

        irq_clear_interrupt();


 return true;
}

void irq_disable_interrupt()
{
    s_pIntrRegs->IEN &= ~HwIEN_EI1;
    s_pIntrRegs->INTMSK &= ~HwIRQMSK_EI1;
}

void irq_enable_interrupt()
{
    s_pIntrRegs->IEN |= HwIEN_EI1;
    s_pIntrRegs->INTMSK |= HwIRQMSK_EI1;
}

void irq_clear_interrupt()
{
    s_pIntrRegs->CLR |= HwIEN_EI1;
}

int transport_target_reset(void)
{
    char ch=0xFF;

    g_IsReady=false;

    //sends a reset pulse
    gpio_set_digital_reset_pinLow();
    mdelay(20);
    gpio_set_digital_reset_pinHigh();
    
    mdelay(50);


    //Sends some SPI-clocks without CS low
    //This is (was) needed on some old Baseband
    gpio_force_spi_cs_pinHigh();
    spi_start();
    spi_senddata_nodma(&ch,1);
    spi_stop();
    gpio_spi_cs_restore();

    mdelay(100);

    return true;
}

void spi_stop()
{
    //Do whatever is needed after a SPI transfer
    //If Chip-Select is under sowtware control should it be put high now
    s_pGpsbRegs->MODE &= ~Hw3;
    gpio_set_cs_pinHigh();
}

void spi_start()
{
    //Do whatever is needed to prepare for a SPI transfer
    //If Chip-Select is under sowtware control should it be put low now
    gpio_set_cs_pinLow();
    s_pGpsbRegs->MODE |= Hw3;
}

unsigned int  spi_senddata_nodma(unsigned char* data, unsigned int size)
{
    unsigned int uTxFIFOValidCnt, uTxNumBytes=0;
    volatile unsigned char* pBuf;
    //volatile unsigned int i = 10000000;

    pBuf = (volatile unsigned char *)data; 

    s_pGpsbRegs->MODE |= ( HwGPSB_CH1MODE_CWF | HwGPSB_CH1MODE_CRF | HwGPSB_CH1MODE_LB);
    s_pGpsbRegs->MODE &= ~( HwGPSB_CH1MODE_CWF | HwGPSB_CH1MODE_CRF );

    do {
        uTxFIFOValidCnt   = ( s_pGpsbRegs->STAT & (Hw28-Hw24)) >> 24;
        if (uTxFIFOValidCnt < 8 /* fifo depth */) {
            s_pGpsbRegs->PORT = *(volatile unsigned char *)pBuf;                     
            pBuf++;
            uTxNumBytes++;
        }
        else {

        }
    } while ( (uTxNumBytes < size));   // FRM1 as GPE20

    
    //while( i-- && !(s_pGpsbRegs->STAT & HwGPSB_CH1STAT_WE) ); // Tx Empty Check
    while( !(s_pGpsbRegs->STAT & HwGPSB_CH1STAT_WE) ); // Tx Empty Check
 

    return 0;
    //return i;
}

unsigned int spi_readdata_nodma(unsigned char* data, unsigned int size)
{
    unsigned int uTxFIFOValidCnt, uTxNumBytes=0, uRxNumBytes=0 ;
    volatile unsigned char* pBuf;
    volatile unsigned char uDummy;
    //volatile unsigned int i = 10000000;

    pBuf = (volatile unsigned char *)data; //s_NonDmaRxBuf;

    s_pGpsbRegs->MODE |= ( HwGPSB_CH1MODE_CWF | HwGPSB_CH1MODE_CRF );
    s_pGpsbRegs->MODE &= ~( HwGPSB_CH1MODE_CWF | HwGPSB_CH1MODE_CRF | HwGPSB_CH1MODE_LB);

    do
    {

        if ( s_pGpsbRegs->STAT & HwGPSB_CH1STAT_RNE )
        {
            uDummy = s_pGpsbRegs->PORT; 

            if (uRxNumBytes < size)
            {
                *pBuf= uDummy;
                pBuf++;
                uRxNumBytes++;
            }
        }

        uTxFIFOValidCnt   = ( s_pGpsbRegs->STAT & (Hw28-Hw24)) >> 24;
        if ((uTxFIFOValidCnt < 8) && (uTxNumBytes < size ))
        {
            if( uTxNumBytes < uRxNumBytes + 4 )
            {
                s_pGpsbRegs->PORT = 0xFF;
                uTxNumBytes++;
            }
        }
       

    } while ((uRxNumBytes < size));


    HOST_ASSERT(!(s_pGpsbRegs->STAT & HwGPSB_CH1STAT_RNE));

    //while(i--)
    while(1)
    {
        if( ( s_pGpsbRegs->STAT & HwGPSB_CH1STAT_WE  ) &&
            !( s_pGpsbRegs->STAT & HwGPSB_CH1STAT_RNE ) )
        {
            break;
        }
    }

    HOST_ASSERT(( ( s_pGpsbRegs->STAT & HwGPSB_CH1STAT_WE  ) 
                  && !( s_pGpsbRegs->STAT & HwGPSB_CH1STAT_RNE ) ));
    return 0;
    //return i;
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

    spi_start();

    spi_senddata_nodma(zeros,sizeof(zeros));
    spi_senddata_nodma(cmd0,sizeof(cmd0));
    spi_readdata_nodma(&input,1);

    spi_stop();

    // Last byte should be 1 on success
    result = (input == 1);

    if(!result)
    {
        printk("%s failed\n", __FUNCTION__);
    }


    return result ? 0 : -EIO;
}

int host_send_cmd52(uint32_t data, int use_slow_clock)
{
    unsigned char cmd52[] = 
    {
        0xff,
        0xff, 
        0xff,
        0x74, // Start = 0, Dir = 1, cmd index = 0b110100
        0x00,
        0x00,
        0x00,
        0x00,
        0xff    // crc=0b1111111, EndBit = 1
    };


    cmd52[4] = (unsigned char) (data>>24)& 0xFF;
    cmd52[5] = (unsigned char) (data>>16)& 0xFF;
    cmd52[6] = (unsigned char) (data>>8)& 0xFF;
    cmd52[7] = (unsigned char) data & 0xFF;

    spi_start();

    spi_senddata_nodma(cmd52,sizeof(cmd52));

    spi_stop();



    return 0;
}

#if 0
static void tcc78x_wifi_startup_sequence(void)
{
    //GPIO_A18 -WIFI_RSTD 
	//GPIO_A19 -WIFI_RSTA
	//GPIO_A20 -WIFI_1.2V_ON 
	//GPIO_A21 -WIFI_3.3V_ON 


	//HwPORTCFG1 : set GPIO_A18-21 as GPIO
	//HP0->2
	//HP0->3
	g_pGPIORegs->PORTCFG1 &= ~0x003C0000;
	g_pGPIORegs->PORTCFG1 |= (HwPORTCFG1_HP1_2 | HwPORTCFG1_HP0_3);

	
	//GPIO_A18,GPIO_A20,GPIO_A21 : output
	//GPIO_A19 : input

	//GPIO_A19 -WIFI_RSTA : input
	g_pGPIORegs->GPAEN |= (1<<19); 

	//GPIO_A18 -WIFI_RSTD :output
	g_pGPIORegs->GPAEN |= (1<<18); 
	g_pGPIORegs->GPADAT |= (1<<18);
	mdelay(15);

	//GPIO_A18 -WIFI_RSTD 
	g_pGPIORegs->GPADAT &= ~(1<<18) ;//0
	mdelay(15);


	//GPIO_A21 -WIFI_3.3V_ON :output
	g_pGPIORegs->GPAEN |= (1<<21); 
	g_pGPIORegs->GPADAT |= (1<<21) ; //1
	mdelay(4);

	
	//GPIO_A20 -WIFI_1.2V_ON : output
	g_pGPIORegs->GPAEN |= (1<<20); 
	g_pGPIORegs->GPADAT |= (1<<20) ; //1
	mdelay(15);

	//GPIO_A18 -WIFI_RSTD 
	g_pGPIORegs->GPADAT |= (1<<18) ; //1
	mdelay(15);

}
#endif
