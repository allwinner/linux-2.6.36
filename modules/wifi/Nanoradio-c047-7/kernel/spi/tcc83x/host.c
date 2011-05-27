#if 1
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


//#include "tcc83x_gpsb.h"
#include "tcc83x_virt.h"
#include "spi_io.h"



#include "host.h"

#include "nanoutil.h"

#define TCC83X_MAGIC_VERSION "nanoradio spi wifi driver magic version 0.spi.rc27"
#define SPI_BIT_WIDTH 8
#define TCC83X_SPI_BASE_ADDRESS 0x9000E000
#define TCC83X_SPI_REG_SIZE     sizeof(TCC83X_GPSB_REG)
#define CKC_EnableBUS_SPI1()   (BITSET(HwBCLKCTR, HwBCLKCTR_SPIMS1_ON))

#define SPI0      0
#define SPI1      1

#define TCC83X_ASSERT(EXPR, REG_A, REG_B) do { if(!(EXPR)) {\
                                     debug_gpio_set();\
                                     debug_gpio_clr();\
                                     debug_gpio_set();\
                                     debug_gpio_clr();\
                                     debug_gpio_set();\
                                     debug_gpio_clr();\
                                     debug_gpio_set();\
                                     debug_gpio_clr();\
                                     debug_gpio_set();\
                                     debug_gpio_clr();\
                               } } while(0);
#if 0
#define TCC83X_IOPORT_ADDRESS      0xF005A000
#define TCC83X_IOPORT_REG_SIZE     sizeof(TCC83X_IOPORT_REG)
#define TCC83X_CLK_ADDRESS      0xF3000000
#define TCC83X_CLK_REG_SIZE     sizeof(TCC83X_CLK_REG)
#define TCC83X_INT_ADDRESS      0xF3001000
#define TCC83X_INT_REG_SIZE     sizeof(TCC83X_INTR_REG)
#endif



//typedef enum { false, true } __attribute__ ((packed)) boolean;

#if 0
volatile TCC83X_CLK_REG *s_pClkRegs = NULL;
volatile TCC83X_IOPORT_REG *g_pGPIORegs = NULL;
static volatile TCC83X_INTR_REG *s_pIntrRegs = NULL;
#endif

volatile int g_IsReady = 0;
volatile unsigned int    CS_Forced_HIGH;

#define _IsRXEMPTY(x)     (((x) & HwSPSTS1_REMP) == HwSPSTS1_REMP)
#define _IsTXFULL(x)      (((x) & HwSPSTS1_TFUL) == HwSPSTS1_TFUL)


#define _SPI_WaitRX(Tout, cnt)    Tout = 300000;                        \
                      while (_IsRXEMPTY((IO_SPI_Status = HwSPSTS1)) && (Tout --));    \
                      cnt;    \

#define _SPI_WaitTX(Tout, cnt)    Tout = 300000;                        \
                      while (_IsTXFULL((IO_SPI_Status = HwSPSTS1)) && (Tout --));   \
                      cnt;    \


volatile unsigned long IO_SPI_Status = 0;

#define SPI1_FRM_HEAD 0
#define SPI1_FRM_TAIL 0
#define SPI1_FRM_LEN  0
#define SPI1_WAIT_LEN 0

/**********************************************************
 *
 *	FUNCTION
 *      IO_CKC_SetSPIClock
 *
 *	DESCRIPTION
 *		Set SPI Source Clock
 *
 *	INPUTS
 *		uCh			Select SPI channel
 *
 *	OUTPUTS
 *		None
 *
 **********************************************************/
#define SETUP_BOTH
void IO_CKC_SetSPIClock(unsigned uCH)
{
	unsigned pclk_cfg;

#ifdef SETUP_BOTH
	/*
	 * SPI0 Clock Setting
	 */
	BITSET(HwPCLKCFG2, HwPCLKCFG2_EN1_OFF);
	pclk_cfg = HwPCLKCFG2_SEL1_DIVPLL0 | (0x3 << 16);	// SEL1:PLL0(204Mhz), P1:0, DIV1:3
	HwPCLKCFG2 = pclk_cfg;

	/* 
	 * SPI1 Clock Setting
	 */
	BITSET(HwPCLKCFG2, HwPCLKCFG2_EN0_OFF);
	pclk_cfg = HwPCLKCFG2_SEL0_DIVPLL0 | (0x1);	// SEL0:PLL0(204Mhz), P0:0, DIV0:3
	HwPCLKCFG2 = pclk_cfg;
#endif

	switch (uCH) {
	case 0:
#ifndef SETUP_BOTH
		BITSET(HwPCLKCFG2, HwPCLKCFG2_EN1_OFF);
		// SEL1:PLL0(204Mhz), P1:0, DIV1:3
		pclk_cfg = HwPCLKCFG2_EN1_ON
		    | HwPCLKCFG2_SEL1_DIVPLL0 | (0x3 << 16);
		HwPCLKCFG2 = pclk_cfg;
#else
		HwPCLKCFG2 |= HwPCLKCFG2_EN1_ON;
#endif
		break;
	case 1:
#ifndef SETUP_BOTH
		BITSET(HwPCLKCFG2, HwPCLKCFG2_EN0_OFF);
		// SEL0:PLL0(204Mhz), P0:0, DIV0:3
		pclk_cfg = HwPCLKCFG2_EN0_ON | HwPCLKCFG2_SEL0_DIVPLL0 | (0x1);
		HwPCLKCFG2 = pclk_cfg;
#else
		HwPCLKCFG2 |= HwPCLKCFG2_EN0_ON;
#endif
		break;
	default:
		break;
	}
}

void configure_debug_gpio (void)
{
#if 0
  // Setting for DEBUG purposes SPI0_CS
  BITCLR(HwNGSEL_B, (Hw2));
	BITCLR(HwGSEL_B,  Hw12);					
	BITCLR(HwGSEL_B,  Hw14);					
  BITSET(HwGIOCON_B, Hw2);
#endif
}

void debug_gpio_set (void)
{
#if 0
  BITSET(HwGDATA_B, Hw2);
#endif
}

void debug_gpio_clr (void)
{
#if 0
	BITCLR(HwGDATA_B,  Hw2);					
#endif
}

#if 0
void IO_CKC_SetSPIClock(void)
{
	/* 
	 * SPI1 Clock Setting
	 */
	HwPCLKCFG2 |= (HwPCLKCFG2_SEL0_PLL0 | (0x3)); 	// SEL0:Direct PLL0, P0:0, DIV0:3

	HwPCLKCFG2 |= HwPCLKCFG2_EN0_ON;
}
#endif

void IO_CKC_EnableBUS_SPI(unsigned uCH)
{
	switch (uCH) {
		case 0:
			IO_CKC_EnableBUS_SPI0();
			break;
		case 1:
			IO_CKC_EnableBUS_SPI1();
			break;
		default:
			break;
	}
}

void spi_cfg_mode(volatile unsigned int SCR)
{
   unsigned uBCLKCTR = HwBCLKCTR;

      BITSET(HwBCLKCTR, HwBCLKCTR_SPIMS1_ON);
			HwSPCR1 = SCR;
	    HwBCLKCTR = uBCLKCTR;
}


void spi_clr_txfifo(void)
{
	unsigned uBCLKCTR = HwBCLKCTR;
	
	CKC_EnableBUS_SPI1();
	
		HwSPCR1 |= HwSPCR1_CTF_EMPTY;

	HwBCLKCTR = uBCLKCTR;
}


/**********************************************************
 *
 *	FUNCTION
 *		IO_SPI_ClearRxFIFO
 *
 *	DESCRIPTION
 *		Clear SPI RX FIFO
 *
 *	INPUTS
 *		uCh			Select SPI channel
 *
 *	OUTPUTS
 *		None
 *
 **********************************************************/
void spi_clr_rxfifo(void)
{
	unsigned uBCLKCTR = HwBCLKCTR;
		
	CKC_EnableBUS_SPI1();
	
		HwSPCR1 |= HwSPCR1_CRF_EMPTY;

	HwBCLKCTR = uBCLKCTR;
}

unsigned int irq_initialize(void)
{

	/* WIFI IRQ */
	HwGSEL_F |= Hw9;		// GPIO_F[11] be used for EINT_7
	HwGIOCON_F &= ~(Hw11);	// GPIO_F[11] Input mode
	HwGPIOF_PE |= Hw11;		// GPIO_F[11] enable pull up/down
	HwGPIOF_PS &= ~Hw11;	// GPIO_F[11] pull up mode

  irq_disable_interrupt();
	/* EINT7 init */
	HwSEL_B |= Hw7;
	HwMODE_B &= ~(Hw7);
	//HwCLR_B |= Hw7;
  irq_clear_interrupt();
	// DO NOT enable here!!!
	HwICFG |= (0x6 << 29);	// Both edge trigger
  printk("Controller : %s\n", __FUNCTION__);
	
  mdelay(100);
 return 0;
}

void irq_disable_interrupt()
{
    HwIEN_B &= ~Hw7;
  //  HwCLR_B |= Hw7;
}

void irq_enable_interrupt()
{
    HwIEN_B |= Hw7;
}

void irq_clear_interrupt()
{
    HwCLR_B |= Hw7;
}

int transport_target_reset(void)
{
    char ch=0xFF;

    spi_senddata_nodma(&ch,1);

    mdelay(100);

    return true;
}

int
host_init(void)
{
  printk("Driver's Magic Version : %s\n", TCC83X_MAGIC_VERSION);


	BITSET(HwBCLKCTR, HwBCLKCTR_SPIMS1_ON);		// SPI1 Master0 Clock Enable
	BITSET(HwSWRESET, HwSWRESET_SPIMS1_ON);		// SPI1 Master0 Reset Enable
	BITCLR(HwSWRESET, HwSWRESET_SPIMS1_ON);		// SPI1 Master0 Reset Disable

	BITCLR(HwNGSEL_B, (Hw15|Hw14|Hw13|Hw12));	// GPIO_B[15:12] is not general I/O
	BITCLR(HwGSEL_B,  Hw13);					// GPIO_B[15:12] is not use UART1
	BITSET(HwGSEL_B,  Hw15);					// GPIO_B[15:12] is SPI1


 IO_CKC_SetSPIClock(SPI1);


	HwSBCR1   = 0x0;	// Base Clock = SPI clock / (DIV+1)*2 ~= 17 mhz
	HwSPCTRL1 =0; 
	HwSPCTRL1 = 
      (0 << 13)|
    	Hw12 |		// [12] Rx FIFO Count Full Interrupt Enable
			Hw11 |		// [11] Rx FIFO Empty Interrupt Enable
			Hw10 |		// [10] Rx FIFO Full Interrupt Enable
			Hw9  |		// [9] Rx FIFO Overrun Error Interrupt Enable
			Hw8  |		// [8] Rx FIFO Underrun Error Interrupt Enable
      (0 << 5)|
     	Hw4  |		// [4] Tx FIFO Count Empty Interrupt Enable
			Hw3  |		// [3] Tx FIFO Empty Interrupt Enable
			Hw2  |		// [2] Tx FIFO Full Interrupt Enable
			Hw1  |		// [1] Tx FIFO Overrun Error Interrupt Enable
			Hw0  |		// [0] Tx FIFO Underrun Error Interrupt Enable
			0;

  spi_cfg_mode(
//				Hw31 |		// [31] SPI Enable
				Hw30 |		// [30] Select Master/Slave 
				Hw29 |		// [29] SCK Polarity
//				Hw28 |		// [28] SCK Phase
				Hw27 |		// [27] MSB/LSB
//				Hw26 |		// [26] TDO Enable
				Hw25 |		// [25] Select Endian
//				Hw24 |		// [24] Continuous Transfer Mode
				(0 << 16) |	// [23:16] Frame Length 
//				Hw15 |		// [15] Rx Mode Enable
				Hw14 |		// [14] Tx Mode Enable 
//				Hw13 |		// [13] External Frame Trigger Enable
//				Hw12 |		// [12] Frame Pulse Polarity
//				Hw11 |		// [11] Frame Signal Extension
//				Hw10 |		// [10] Clear Rx FIFO
//				Hw9  |		// [9] Clear Tx FIFO
//				Hw8	 |		// [8] Trigger Position
//				Hw7  |		// [7] Captured SDI
				0    |		// [6:0] Wait Count 
				0);

	BITSET(HwSPCR1, HwSPCR1_CRF_EMPTY);	// Clear RX FIFO
	BITSET(HwSPCR1, HwSPCR1_CTF_EMPTY);	// Clear TX FIFO
	BITSET(HwSPCR1, HwSPCR1_EN_ON);		// Enable SPI0
	BITSET(HwSPCR1, HwSPCR1_TOE_EN);	// Enable TX Data Out

	printk("Controller spi1: open\n");
	printk("[SBCR1    ] %08X\n", (unsigned int)HwSBCR1);
	printk("[SPCR1    ] %08X\n", (unsigned int)HwSPCR1);
	printk("[SPCTRL1  ] %08X\n", (unsigned int)HwSPCTRL1);
	printk("[SPFRMPOS1] %08X\n", (unsigned int)HwSPFRMPOS1);
	printk("[SPSTS1   ] %08X\n", (unsigned int)HwSPSTS1);

  mdelay(100);
  irq_initialize();

	////KDEBUG(TRANSPORT, "EXIT");
	return 0;
}

void gpio_enable_interrupt(void)
{
    //Do whatever is needed to enable interrupt (and is that is not done by the OS)
    irq_enable_interrupt();
}

void gpio_disable_interrupt(void)
{
    irq_disable_interrupt();
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



           //writes the SDIO header
           spi_senddata_nodma(cmd53_read, sizeof(cmd53_read));
           //reads the data
           spi_readdata_nodma(&data[current_byte], size_to_read);
           //writes the SDIO CRC
           spi_senddata_nodma(cmd53_rpost, sizeof(cmd53_rpost));


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



           spi_senddata_nodma(cmd53_write, sizeof(cmd53_write));
           spi_senddata_nodma(&data[current_byte], size_to_send);
           spi_senddata_nodma(cmd53_tpost, sizeof(cmd53_tpost));
           

           remaining_size -= size_to_send;
           current_byte += size_to_send;
       }
  return 0;
}

/* phth: Obsolete GPIO_ID_RESET_D and GPIO_ID_RESET_A removed.
 * nrx710C has only one reset input, represented as GPIO_ID_RESET
 */
void
host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
#ifndef JTAG_CONNECTION
    //GPIO_A18 -WIFI_RSTD 
	//GPIO_A19 -WIFI_RSTA
	//GPIO_A20 -WIFI_1.2V_ON 
	//GPIO_A21 -WIFI_3.3V_ON 
  //SPI 1 TCC83X
    //GPIO_C8 -WIFI_RSTD 
	//GPIO_C9 -WIFI_RSTA
	//GPIO_C10 -WIFI_1.2V_ON 
	//GPIO_C11 -WIFI_1.2V_ON 
    switch(gpio_op) {
    case GPIO_OP_DISABLE:
         if(gpio_id == GPIO_ID_RESET_D) {
           BITSET(HwGIOCON_C, Hw8);
           BITCLR(HwGDATA_C, Hw8);
           BITCLR(HwNGSEL_C, Hw8);
           
         } else if(gpio_id == GPIO_ID_RESET_A) {
           BITSET(HwGIOCON_C, Hw9);
           BITCLR(HwGDATA_C, Hw9);
           BITCLR(HwNGSEL_C, Hw9);
         } else if(gpio_id == GPIO_ID_POWER) {
           BITSET(HwGIOCON_C, (Hw10 | Hw11));
           BITCLR(HwGDATA_C, (Hw10 | Hw11));
           BITCLR(HwNGSEL_C, (Hw10 | Hw11));
         }

	       break; 
    case GPIO_OP_ENABLE:
         if(gpio_id == GPIO_ID_RESET_D) {
           BITSET(HwGIOCON_C, Hw8);
           BITCLR(HwGSEL_C, Hw8);
           BITSET(HwNGSEL_C, Hw8);
           BITSET(HwGDATA_C, Hw8);
         } else if(gpio_id == GPIO_ID_RESET_A) {
           BITSET(HwGIOCON_C, Hw9);
           BITCLR(HwGSEL_C, Hw9);
           BITSET(HwNGSEL_C, Hw9);
           BITSET(HwGDATA_C, Hw9);
         } else if(gpio_id == GPIO_ID_POWER){
           BITSET(HwGIOCON_C, (Hw10 | Hw11));
           BITCLR(HwGSEL_C, (Hw10 | Hw11));
           BITSET(HwNGSEL_C, (Hw10 | Hw11));
           BITSET(HwGDATA_C, (Hw10 | Hw11));
         }

	       break;
    case GPIO_OP_HIGH:
         if(gpio_id == GPIO_ID_RESET_D) {
           BITSET(HwGDATA_C, Hw8);
         } else if(gpio_id == GPIO_ID_RESET_A) {
           BITSET(HwGDATA_C, Hw9);
         } else if(gpio_id == GPIO_ID_POWER) {
           BITSET(HwGDATA_C, (Hw10 | Hw11));
         }
	  break;
    case GPIO_OP_LOW:
         if(gpio_id == GPIO_ID_RESET_D) {
           BITCLR(HwGDATA_C, Hw8);
         } else if(gpio_id == GPIO_ID_RESET_A) {
           BITCLR(HwGDATA_C, Hw9);
         } else if(gpio_id == GPIO_ID_POWER) {
           BITCLR(HwGDATA_C, (Hw10 | Hw11));
         }
	  break;
    default: HOST_ASSERT(0);

    }

   // mdelay(10);

#endif
    return;
}

void
host_exit(void)
{
   KDEBUG(TRANSPORT, "ENTRY");
	 KDEBUG(TRANSPORT, "EXIT");
	return;
}


unsigned int  spi_senddata_nodma(unsigned char* data, unsigned int size)
{
    volatile unsigned int uTxFIFOWords=0;
    volatile unsigned int modulo = 0;
    volatile unsigned int LoopCnt = 0;
    unsigned long dummy=0;
    volatile unsigned long tmp = 0;
    volatile unsigned int size_to_send = 0;
    volatile unsigned char completion_flag = 0;


   modulo = size % 32; 

   if((modulo != size)) {

    LoopCnt = (size/32);

     size_to_send = 32;
      
   } else {

    LoopCnt = 1;
    size_to_send = size;

   }

   //printk("LoopCnt : %d\n", LoopCnt);

   while(LoopCnt) {

      HwSPCR1 &= ~(Hw31 | Hw26);

      tmp = HwSPSTS1;
      while (!(HwSPSTS1 & HwSPSTS1_REMP))
        tmp = HwSDI1;

      while(!(HwSPSTS1 & HwSPSTS1_TEMP))
       HwSPCR1 |= (HwSPCR1_CTF_EMPTY);


      uTxFIFOWords = (size_to_send + sizeof(unsigned int) - 1) / sizeof(unsigned int);

      

      // Set FrameLength
      HwSPCR1 &= 0xFF00FFFF;
      HwSPCR1 |= (((size_to_send - 1)) << 16);


       while (uTxFIFOWords) {
         HwSDO1 = *((volatile unsigned long *)data);					
         data += 4;
         uTxFIFOWords--;
       }

       HwSPCR1 |= (Hw31 | Hw26);

       uTxFIFOWords = (size_to_send + sizeof(unsigned int) - 1) / sizeof(unsigned int);
       while(uTxFIFOWords) {
        
         while ((HwSPSTS1 & HwSPSTS1_REMP));

         dummy = HwSDI1;
         uTxFIFOWords--;
       }

       if((LoopCnt == 1) && (modulo !=size) && (modulo)) {
          if(!completion_flag) {
           size_to_send = modulo;
           LoopCnt++;
           completion_flag = 1;
          }
       }

    LoopCnt--;

   }
   
    return 0;
}

unsigned int spi_readdata_nodma(unsigned char* data, unsigned int size)
{
    volatile unsigned int uRxNumWords=0;
    volatile unsigned long dummy = 0xffffffff;
    volatile unsigned int modulo = 0;
    volatile unsigned int LoopCnt = 0;
    volatile unsigned long tmp = 0;
    volatile unsigned int size_to_read = 0;
    volatile unsigned char completion_flag = 0;

   modulo = (size % 32); 

   if((modulo != size)) {

    LoopCnt = (size/32);

     size_to_read = 32;
      
   } else {

    LoopCnt = 1;
    size_to_read = size;

   }

   while (LoopCnt) {

        HwSPCR1 &= ~(Hw31 | Hw26);

       while (!(HwSPSTS1 & HwSPSTS1_REMP))
        HwSPCR1 |= (HwSPCR1_CRF_EMPTY);

       while(!(HwSPSTS1 & HwSPSTS1_TEMP))
        HwSPCR1 |= (HwSPCR1_CTF_EMPTY);



       uRxNumWords = (size_to_read + sizeof(unsigned int) - 1) / sizeof(unsigned int);

       // Set FrameLength
       HwSPCR1 &= 0xFF00FFFF;
       HwSPCR1 |= ((size_to_read - 1)) << 16;




       while (uRxNumWords) {
         HwSDO1 = dummy;

         uRxNumWords--;
       }

       HwSPCR1 |= (Hw31 | Hw26);


       uRxNumWords = (size_to_read + sizeof(unsigned int) - 1) / sizeof(unsigned int);

       while (uRxNumWords) {

         while ((HwSPSTS1 & HwSPSTS1_REMP));

         *((volatile unsigned long *)data) = *(volatile unsigned long *)(&HwSDI1);
         data += 4;

         uRxNumWords--;
       }

       if((LoopCnt == 1) && (modulo !=size) && (modulo)) {

          if(!completion_flag) {
             size_to_read = modulo;
             LoopCnt++;
             completion_flag = 1;
          }

       }

   LoopCnt--;    
   }


    return 0;
}


int host_send_cmd0(void)
{
#if 1
    //After restart TARGET interface will be in SDIO-mode
    //To put the interface in SPI-mode we need to send a Command-zero with CS low

      // Init string to set SDIO controller in SPI mode
#if 0
    unsigned char zeros[] = {   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
#endif
    unsigned char cmd0[] = {0xff,0x40,0x0,0x0,0x0,0x0,0x95,0xff};
    unsigned char input=0xFF;
    int result = 0;

  //  spi_start();

    spi_senddata_nodma(cmd0,sizeof(cmd0));
    spi_readdata_nodma(&input,1);

   // spi_stop();

    // Last byte should be 1 on success
    result = (input == 1);

    if(!result)
    {
        printk("%s failed\n", __FUNCTION__);
    }

#endif
#if 0
    printk("******************************************\n");
    printk("CMD0's result : %d\n", result);  
    printk("******************************************\n");
#endif
    

    return result ? 0 : -EIO;
}

int host_send_cmd52(uint32_t data, int use_slow_clock)
{
#if 0
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



#endif
    return 0;
}

#if 0
static void tcc83x_wifi_startup_sequence(void)
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
#ifdef SPI_IF_TEST
int spi_test(unsigned char  *data, uint16_t len, uint32_t flags)
{
    uint16_t remaining_size = len;
    uint16_t current_byte = 0;
    uint16_t max_size = 255;
    unsigned char response[2];
    const unsigned char nop = 0xFF;
    const unsigned char start_bit = 0xFE;
    int i;

    static unsigned char cmd53_read[] = {
        0xff,
        0x75,
        0x10,
        0x00,
        0x00,   // size msb
        0x00,   // size lsb
        0x01,
        0xff
    };
    static unsigned char cmd53_write[] =
    {
        0xff,
        0x75,
        0x90,
        0x00,
        0x00,   // size msb
        0x00,   // size lsb
        0x01,
        0xff
    };

    if (flags & FIFO_FLAG_TO_HOST) {
       // A message to/from the Nanoradio Chip can be up to 1600 bytes but
       // SDIO can only handle max +512 byte at a time, therefore is it
       // necessary to split the message into a number of SDIO reads/writes
       while(remaining_size)
       {
           uint16_t size_to_read = remaining_size;
           if(size_to_read > max_size) size_to_read = max_size;

           //Write the size to the CMD53 header
           cmd53_read[4] = (unsigned char) ((size_to_read & 255)>>8);
           cmd53_read[5] = (unsigned char) size_to_read & 0xFF;



           //write the CMD53 header
           spi_senddata_nodma(cmd53_read, sizeof(cmd53_read));
           //read the CMD53 response (2 bytes)
           spi_readdata_nodma(response, 2);
//           printk("CMD53 Read response: 0x%02x 0x%02x\n", response[0], response[1]);
           //read the start bit
           spi_readdata_nodma(response, 2);
	   // read actual data
           spi_readdata_nodma(&data[current_byte], size_to_read);
 //          printk("CMD53 Read Data: ");
#if 0
           for (i=0; i<size_to_read; i++) {
             printk("0x%02x ", data[current_byte + i]);
           }
#endif
           printk("\n");
	   // read CRC
           spi_readdata_nodma(response, 2);
           //printk("CMD53 Read CRC: 0x%02x 0x%02x\n", response[0], response[1]);

           remaining_size -= size_to_read;
           current_byte += size_to_read;
       }

    } else {
       while(remaining_size)
       {
           uint16_t size_to_send = remaining_size;
           if(size_to_send > max_size) size_to_send = max_size;

           //Write the size to the CMD53 header
           cmd53_write[4] = (unsigned char) ((size_to_send & 255)>>8);
           cmd53_write[5] = (unsigned char) size_to_send & 0xFF;



           //write the CMD53 header
           spi_senddata_nodma(cmd53_write, sizeof(cmd53_write));
           //read the CMD53 response (2 bytes)
           spi_readdata_nodma(response, 2);
    //       printk("CMD53 Write response: 0x%02x 0x%02x\n", response[0], response[1]);
#if 0
           spi_senddata_nodma((unsigned char *)&nop, sizeof(nop)); // Not really needed,
	                                         // but just to be on the safe side
#endif
           //write the start bit
           spi_senddata_nodma((unsigned char *)&start_bit, 1);
	   // Write actual data
           spi_senddata_nodma(&data[current_byte], size_to_send);
	   // Write CRC
           spi_senddata_nodma(response, 2);
           // read data response (1 byte)
	   // In SPI, we should always get 0xE5 as data response
           spi_readdata_nodma(response, 2);
     //      printk("CMD53 Data response: 0x%02x 0x%02x\n", response[0], response[1]);

           remaining_size -= size_to_send;
           current_byte += size_to_send;
       }

    }

  return 0;
}
#endif

