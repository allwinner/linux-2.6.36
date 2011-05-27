/******************************************************************************/
/*          :                                     */
/*        :  uniphier_bit.h                                         */
/*        :  2005.01.06                                             */
/*            :                                                 */
/*          :                                                     */
/*                                                                            */
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
/*****  ****************************************************/
/*  No.                                       */
/*                                                                            */
/*                                05.01.06 H.Nakajima */
/*          GPIO          GPIOC2,GPIOF2                   05.04.01     */
/*          SDIO          SDIO                            05.04.05 GT Kouno   */
/*<6600190> GPIOF2        GPIOF2/     05.06.10     */
/*<8601809> FDMAC         FDMA      05.06.24    */
/*<8601846> CLKM                          05.06.30 R.Shinohara*/
/*<8601826> ARMDMAC       Linux                         */
/*                        HW    */
/*                              05.06.29    */
/*<8601821> StreamDMAC    Linux                         */
/*                        HW    */
/*                              05.06.29    */
/*<8650258> CLKM                                05.10.21 R.Shinohara*/
/*<8650479> CLKM          VIN   06.01.23 R.Shinohara*/
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _UNIPHIER_BIT_H_                         /* _UNIPHIER_BIT_H_          */
#define _UNIPHIER_BIT_H_

/*============================================================================*/
/* Uniphier Block   0xD0000000                                              */
/*============================================================================*/

/*------- IPP            0xD0000000 ----------------------------------------*/
/*------- IPPTIMER       0xD0800000 ----------------------------------------*/
/*------- IPPINTC        0xD0810000 ----------------------------------------*/
/*------- RSTCNT         0xD0820000 ----------------------------------------*/
/*------- IXM            0xD1000000 ----------------------------------------*/
/*------- ABC            0xD1800000 ----------------------------------------*/
/*------- IPP-BCU        0xD1C00000 ----------------------------------------*/
/*------- VPP            0xD2040000 ----------------------------------------*/
/*------- PDMAC          0xD2050000 ----------------------------------------*/
/*------- MP-DBC         0xD2060000 ----------------------------------------*/
/*------- MPEG-4(IF)     0xD2200000 ----------------------------------------*/


/*============================================================================*/
/* Micom-0  Block   0xD5000000                                              */
/*============================================================================*/

/*------- ARMTIMER       0xD5000000 ----------------------------------------*/
/*------- RTCTIMER       0xD5010000 ----------------------------------------*/
/*------- ARMVIC         0xD5040000 ----------------------------------------*/
/*------- KeyScan        0xD5050000 ----------------------------------------*/
/*------- Protect Module 0xD5080000 ----------------------------------------*/
/*------- CLKGEN         0xD5100000 ----------------------------------------*/
/*  PMICOM0_IPPGEARREG  16bit R/W                                             */

#define     D_IPPGEARREG_CIPPGEA_72MHZ  0x0002      /* R/W CIPPGEA = 72Mhz    */

#define     D_IPPGEARREG_CIPPGEA_96MHZ  0x0001      /* R/W CIPPGEA = 96Mhz    */

#define     D_IPPGEARREG_CIPPGEA_144MHZ 0x0000      /* R/W CIPPGEA = 144Mhz   */

/*  PMICOM0_THIPPSTOP   16bit R/W                                             */

#define     D_THIPPSTOP_HIPPSTOP        0x0001      /* R/W HIPPSTOP           */

/*  PMICOM0_IPPMREG     16bit R/W                                             */

#define     D_IPPMREG_IPPSPRQ_BIT       0x0004      /* R/W IPPSPRQ            */

#define     D_IPPMREG_IPPSPAK_BIT       0x0002      /* R   IPPSPAK            */

/*  PMICOM0_CLKENREG3   16bit R/W                                             */

#define     D_CLKENREG3_CEN_IPP_BIT     0x0010      /* R/W CEN_IPP            */

#define     D_CLKENREG3_CEN_MP_BIT      0x0008      /* R/W CEN_MP             */

#define     D_CLKENREG3_CEN_VPP_BIT     0x0004      /* R/W CEN_VPP            */

/* <8601846>                                                    */
/*  */
#define     DCLKM_HIGH_GEAR             0x00000000  /* (288MHz) */
#define     DCLKM_MID_GEAR              0x00000001  /* (192MHz) */
#define     DCLKM_LOW_GEAR              0x00000002  /* (96MHz)  */

/*------- RESET/Power    0xD5110000 ----------------------------------------*/
/*  PMICOM0_IPPRSTCTR   16bit R/W                                             */

#define     D_IPPRSTCTR_IPPCHIRST       0x0001      /* R/W IPPCHIRST          */


/*------- UART1          0xD5130000 ----------------------------------------*/
/*------- UART2          0xD5140000 ----------------------------------------*/
/*------- GPIO           0xD5150000 ----------------------------------------*/
#define     GPIOC2_ICBIT           0x01          /* GPIOC2_IC BIT             */
#define     GPIOC2_IE_ARMBIT       0x80          /* GPIOC2_IE_ARM BIT         */
#define     D_OPNCLS_GPIOF2_DIR    0x10          /*<6600190>GPIOF2_DIR BIT    */
/*<6600190>#define D_OPNCLS_GPIOF2_DATA   0x40 *//* GPIOF2_DATA BIT           */
#define     D_OPNCLS_GPIOF2_DATA   0x20          /*<6600190>BIT */
                                                 /* GPIOF2_DATA BIT           */
/* <8601846>                                                    */
/* GPIO */
#define     DCLKM_GPIO_ENABLE      0x00000008    /* GPIO              */
#define     DCLKM_GPIO_DISABLE     0x00000010    /* GPIO          */
/* GPIO */
#define     DPM_GPIO_MASK          0x00000080    /* bit7    */

/*------- SSI1(2ch)      0xD5180000 ----------------------------------------*/
/*------- Backup Mem/Reg 0xD5190000 ----------------------------------------*/
#define     DCLKM_AODIRB_BIT       0x00000002    /*<8650479>CAM */

/*============================================================================*/
/* Micom-1  Block   0xD6000000                                              */
/*============================================================================*/

/*------- ARMDMAC        0xD6000000 ----------------------------------------*/
/*<8601826> ADMA DMA CTR -----------------------------------------------------*/
#define D_ADMA_INIT_CTR_VALUE         0x40000000  /*<8601826> */
/*<8601826> --                                                  */
#define D_ADMA_SHIFT_CTR_END          31          /*<8601826>     */
#define D_ADMA_BITSET_CTR_END         ( 1<<D_ADMA_SHIFT_CTR_END )  /*<8601826>*/
                                                  /*<8601826> 1:      */
                                                  /*<8601826> 0:or*/
/*<8601826> --                                        */
#define D_ADMA_SHIFT_CTR_AIEN         30          /*<8601826>     */
#define D_ADMA_BITSET_CTR_AIEN        ( 1<<D_ADMA_SHIFT_CTR_AIEN ) /*<8601826>*/
                                                  /*<8601826> 1:      */
                                                  /*<8601826> 0:      */
/*<8601826> --                                            */
#define D_ADMA_SHIFT_CTR_DE           28          /*<8601826>     */
/*<8601826> --                                            */
#define D_ADMA_SHIFT_CTR_SE           26          /*<8601826>     */
/*<8601826> --                                                    */
#define D_ADMA_SHIFT_CTR_ERF          25          /*<8601826>     */
#define D_ADMA_BITSET_CTR_ERF         ( 1<<D_ADMA_SHIFT_CTR_ERF )  /*<8601826>*/
                                                  /*<8601826> 1:    */
                                                  /*<8601826> 0: or */
/*<8601826> --                                                  */
#define D_ADMA_SHIFT_CTR_RQF          24          /*<8601826>     */
#define D_ADMA_BITSET_CTR_RQF         ( 1<<D_ADMA_SHIFT_CTR_RQF )  /*<8601826>*/
                                                  /*<8601826> 1:  */
                                                  /*<8601826> 0:  */
                                                  /*<8601826>       or  */
/*<8601826> --                                                        */
#define D_ADMA_SHIFT_CTR_TW           22          /*<8601826>     */
/*<8601826> --                                                    */
#define D_ADMA_SHIFT_CTR_TU           19          /*<8601826>     */
/*<8601826> --DMA                                               */
#define D_ADMA_SHIFT_CTR_TEN          16          /*<8601826>     */
#define D_ADMA_BITSET_CTR_TEN         ( 1<<D_ADMA_SHIFT_CTR_TEN ) /*<8601826>*/
                                                  /*<8601826> 1:      */
                                                  /*<8601826> 0:      */
/*<8601826> --                                      */
#define D_ADMA_SHIFT_CTR_DAM          12          /*<8601826>     */
/*<8601826> --                                      */
#define D_ADMA_SHIFT_CTR_SAM          8           /*<8601826>     */

/*<8601826> ADMA DMA MCTR --------------------------------------------------- */
/*<8601826> --                                                */
#define D_ADMA_SHIFT_MCTR_PRI         1           /*<8601826>     */
#define D_ADMA_BITSET_MCTR_PRI        ( 1<<D_ADMA_SHIFT_MCTR_PRI ) /*<8601826>*/
                                                  /*<8601826> 1:RR, 0:    */
/*<8601826> --DMA                                     */
#define D_ADMA_SHIFT_MCTR_DEN         0           /*<8601826>     */
#define D_ADMA_BITSET_MCTR_DEN        ( 1<<D_ADMA_SHIFT_MCTR_DEN ) /*<8601826>*/
                                                  /*<8601826> 1:      */
                                                  /*<8601826> 0:      */
/*<8601826> ADMA DMA SQN, SQA ----------------------------------------------- */
#define D_ADMA_SQN0_INIT_VALUE        0x00000001  /*<8601826> 0   */
                                                  /*<8601826>   */
#define D_ADMA_SQA0_INIT_VALUE        0x0000000E  /*<8601826> 0   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN1_INIT_VALUE        0x00000002  /*<8601826> 1   */
                                                  /*<8601826>   */
#define D_ADMA_SQA1_INIT_VALUE        0x00000004  /*<8601826> 1   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN2_INIT_VALUE        0x00000003  /*<8601826> 2   */
                                                  /*<8601826>   */
#define D_ADMA_SQA2_INIT_VALUE        0x0000001A  /*<8601826> 2   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN3_INIT_VALUE        0x00000006  /*<8601826> 3   */
                                                  /*<8601826>   */
#define D_ADMA_SQA3_INIT_VALUE        0x00000014  /*<8601826> 3   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN4_INIT_VALUE        0x00000001  /*<8601826> 4   */
                                                  /*<8601826>   */
#define D_ADMA_SQA4_INIT_VALUE        0x0000000E  /*<8601826> 4   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN5_INIT_VALUE        0x00000002  /*<8601826> 5   */
                                                  /*<8601826>   */
#define D_ADMA_SQA5_INIT_VALUE        0x00000004  /*<8601826> 5   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN6_INIT_VALUE        0x00000003  /*<8601826> 6   */
                                                  /*<8601826>   */
#define D_ADMA_SQA6_INIT_VALUE        0x0000001A  /*<8601826> 6   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN7_INIT_VALUE        0x00000006  /*<8601826> 7   */
                                                  /*<8601826>   */
#define D_ADMA_SQA7_INIT_VALUE        0x00000014  /*<8601826> 7   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN8_INIT_VALUE        0x00000001  /*<8601826> 8   */
                                                  /*<8601826>   */
#define D_ADMA_SQA8_INIT_VALUE        0x0000000E  /*<8601826> 8   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN9_INIT_VALUE        0x00000002  /*<8601826> 9   */
                                                  /*<8601826>   */
#define D_ADMA_SQA9_INIT_VALUE        0x00000004  /*<8601826> 9   */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN10_INIT_VALUE       0x00000003  /*<8601826> 10  */
                                                  /*<8601826>   */
#define D_ADMA_SQA10_INIT_VALUE       0x0000001A  /*<8601826> 10  */
                                                  /*<8601826> ADDR  */
#define D_ADMA_SQN11_INIT_VALUE       0x00000006  /*<8601826> 11  */
                                                  /*<8601826>   */
#define D_ADMA_SQA11_INIT_VALUE       0x00000014  /*<8601826> 11  */
                                                  /*<8601826> ADDR  */

/*------- IrDA-UART      0xD6010000 ----------------------------------------*/
/*------- I2C            0xD6020000 ----------------------------------------*/
/*------- BCU (Register) 0xD6100000 ----------------------------------------*/
/*------- UART3          0xD6130000 ----------------------------------------*/
/*------- UART4          0xD6140000 ----------------------------------------*/
/*------- SSI2(6ch)      0xD6180000 ----------------------------------------*/
/*------- Scheduler      0xD6190000 ----------------------------------------*/
#define DCLKM_ABT_TBL_ON         0x00000001       /*<8650258> TBL ON    */
#define DCLKM_ABT_TLB_OFF        0x00000000       /*<8650258> TBL ONOFF */
#define DCLKM_ABT_LOWLEV_MIN     0x0000000b       /*<8650258> LOLEV */
#define DCLKM_ABT_LOWLEV_MAX     0x0000000f       /*<8650258> LOLEV */
#define DCLKM_ATB_TBLON_MASK     0x00000001       /*<8650258> TBLONMASK */

/*------- TDDS           0xD61A0000 ----------------------------------------*/
/*------- PWM            0xD61B0000 ----------------------------------------*/

/*============================================================================*/
/* AVIO-0   Block   0xD8000000                                              */
/*============================================================================*/

/*------- ReCS           0xD8000000 ----------------------------------------*/
/*------- JPEG           0xD8100000 ----------------------------------------*/
/*------- AVIO-Bridge    0xD8200000 ----------------------------------------*/

/*============================================================================*/
/* AVIO-1   Block   0xD9000000                                              */
/*============================================================================*/

/*------- Frago          0xD9000000 ----------------------------------------*/
/*------- VIN            0xD9020000 ----------------------------------------*/
/*------- VOUT           0xD9030000 ----------------------------------------*/
/*------- FDMAC          0xD9080000 ----------------------------------------*/
#define D_FDMA_FDMACEXE_ON      0x0001            /*<8601809>FDMA         */
#define D_FDMA_FDMACEXE_OFF     0x0000            /*<8601809>FDMA         */
#define D_FDMA_FDPMCKEN_ON      0x0001            /*<8601809>     */
#define D_FDMA_FDPMCKEN_OFF     0x0000            /*<8601809>     */


/*============================================================================*/
/* AVIO-2   Block   0xDA000000                                              */
/*============================================================================*/

/*------- BSSI3          0xDA000000 ----------------------------------------*/
/*------- BSSI4          0xDA010000 ----------------------------------------*/
/*------- BSSI5          0xDA020000 ----------------------------------------*/


/*============================================================================*/
/* Stream   Block   0xDC000000                                              */
/*============================================================================*/

/*------- StreamDMAC     0xDC000000 ----------------------------------------*/
/*<8601821> SDMA DMA CLK_CTRL ----------------------------------------------- */
/*<8601821> --IF                                  */
#define D_SDMA_SHIFT_CLK_MEMEN_MCB  31            /*<8601821>     */
#define D_SDMA_BITSET_CLK_MEMEN_MCB ( 1<<D_SDMA_SHIFT_CLK_MEMEN_MCB )/*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> --                    */
#define D_SDMA_SHIFT_CLK_BUFEN      30            /*<8601821>     */
#define D_SDMA_BITSET_CLK_BUFEN     ( 1<<D_SDMA_SHIFT_CLK_BUFEN )  /*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> --                            */
#define D_SDMA_SHIFT_CLK_REGEN      29            /*<8601821>     */
#define D_SDMA_BITSET_CLK_REGEN     ( 1<<D_SDMA_SHIFT_CLK_REGEN )  /*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> --()                            */
#define D_SDMA_SHIFT_CLK_RESERVE28  28            /*<8601821>     */
#define D_SDMA_BITSET_CLK_RESERVE28 ( 1<<D_SDMA_SHIFT_CLK_RESERVE28 )/*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> --MCB                             */
#define D_SDMA_SHIFT_CLK_MEM        15            /*<8601821>     */
#define D_SDMA_BITSET_CLK_MEM       ( 1<<D_SDMA_SHIFT_CLK_MEM )    /*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> -- ON                               */
#define D_SDMA_SHIFT_CLK_BUF        14            /*<8601821>     */
#define D_SDMA_BITSET_CLK_BUF       ( 1<<D_SDMA_SHIFT_CLK_BUF )    /*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> -- ON()                                     */
#define D_SDMA_SHIFT_CLK_RESERVE13  13            /*<8601821>     */
#define D_SDMA_BITSET_CLK_RESERVE13 ( 1<<D_SDMA_SHIFT_CLK_RESERVE13 )/*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> -- ON()                                     */
#define D_SDMA_SHIFT_CLK_RESERVE12  12            /*<8601821>     */
#define D_SDMA_BITSET_CLK_RESERVE12 ( 1<<D_SDMA_SHIFT_CLK_RESERVE12 )/*<8601821>*/
                                                  /*<8601821> 1:ON     0:OFF  */
/*<8601821> -- OFF                                        */
#define D_SDMA_BITSET_CLK_OFF       0x00000000    /*<8601821>     */
                                                  /*<8601821> */

/*<8601821> SDMA DMA CLK_CTRL2 ---------------------------------------------- */
#ifndef DTV1_ES1_ERRATA                           /*<8601821>                 */
#define D_SDMA_BITSET_CLK2          0xFFFFFFFF    /*<8601821> CH        */
                                                  /*<8601821> GATEDCLOCK  */
#else /*<8601821> DTV1_ES1_ERRATA */
#define D_SDMA_BITSET_CLK2          0x0000FFFF    /*<8601821> CH        */
                                                  /*<8601821> GATEDCLOCK  */
#endif /*<8601821> DTV1_ES1_ERRATA */
#define D_SDMA_BITSET_CLK2_OFF      0x00000000    /*<8601821>     */
                                                  /*<8601821> */
                                                  
/*<8601821> SDMA DMA DMA_SIZE ----------------------------------------------- */
/*<8601821> --                                                      */
#define D_SDMA_SHIFT_ENDLESS        24            /*<8601821>     */
#define D_SDMA_BITSET_ENDLESS       ( 1<<D_SDMA_SHIFT_ENDLESS )    /*<8601821>*/
                                                  /*<8601821> 1:*/
                                                  /*<8601821> 0:*/

/*------- BSSI1          0xDC100000 ----------------------------------------*/
/*------- BSSI2          0xDC110000 ----------------------------------------*/
/*------- SDIO           0xDC120000 ----------------------------------------*/
#define D_SDIO_SD_STOP_SEC           0x0100  /* R/W                                     */
#define D_SDIO_SD_STOP_STP           0x0001  /* R/W                                         */
#define D_SDIO_SD_INFO1_INFO10       0x0400  /* R   SDDATA3                                         */
#define D_SDIO_SD_INFO1_INFO9        0x0200  /* R/W SDDATA3                                         */
#define D_SDIO_SD_INFO1_INFO8        0x0100  /* R/W SDDATA3                                       */
#define D_SDIO_SD_INFO1_INFO7        0x0080  /* R                                             */
#define D_SDIO_SD_INFO1_INFO5        0x0020  /* R                                                   */
#define D_SDIO_SD_INFO1_INFO4        0x0010  /* R/W SDCD                                            */
#define D_SDIO_SD_INFO1_INFO3        0x0008  /* R/W SDCD                                          */
#define D_SDIO_SD_INFO1_INFO2        0x0004  /* R/W R/W                                           */
#define D_SDIO_SD_INFO1_INFO0        0x0001  /* R/W                                             */
#define D_SDIO_SD_INFO2_ILA          0x8000  /* R/W                                   */
#define D_SDIO_SD_INFO2_CBSY         0x4000  /* R                                       */
#define D_SDIO_SD_INFO2_BWE          0x0200  /* R/W SDMemoryCard                          */
#define D_SDIO_SD_INFO2_BRE          0x0100  /* R/W SDMemoryCard                          */
#define D_SDIO_SD_INFO2_DAT0         0x0080  /* R   SDDATA0                                               */
#define D_SDIO_SD_INFO2_ERR6         0x0040  /* R/W                                     */
#define D_SDIO_SD_INFO2_ERR5         0x0020  /* R/W                                   */
#define D_SDIO_SD_INFO2_ERR4         0x0010  /* R/W                                   */
#define D_SDIO_SD_INFO2_ERR3         0x0008  /* R/W                                               */
#define D_SDIO_SD_INFO2_ERR2         0x0004  /* R/W                                           */
#define D_SDIO_SD_INFO2_ERR1         0x0002  /* R/W CRC                                                 */
#define D_SDIO_SD_INFO2_ERR0         0x0001  /* R/W                                             */
#define D_SDIO_SD_INFO1_MASK_IMASK9  0x0200  /* R/W SDDATA3                               */
#define D_SDIO_SD_INFO1_MASK_IMASK8  0x0100  /* R/W SDDATA3                             */
#define D_SDIO_SD_INFO1_MASK_IMASK4  0x0010  /* R/W SDCD                                  */
#define D_SDIO_SD_INFO1_MASK_IMASK3  0x0008  /* R/W SDCD                                */
#define D_SDIO_SD_INFO1_MASK_IMASK2  0x0004  /* R/W R/W                                 */
#define D_SDIO_SD_INFO1_MASK_IMASK0  0x0001  /* R/W                                   */
#define D_SDIO_SD_INFO2_MASK_IMASK   0x8000  /* R/W ILA                                           */
#define D_SDIO_SD_INFO2_MASK_BMSK1   0x0200  /* R/W BWE                                           */
#define D_SDIO_SD_INFO2_MASK_BMSK0   0x0100  /* R/W BRE                                           */
#define D_SDIO_SD_INFO2_MASK_EMASK6  0x0040  /* R/W                         */
#define D_SDIO_SD_INFO2_MASK_EMASK5  0x0020  /* R/W                       */
#define D_SDIO_SD_INFO2_MASK_EMASK4  0x0010  /* R/W                       */
#define D_SDIO_SD_INFO2_MASK_EMASK3  0x0008  /* R/W                                   */
#define D_SDIO_SD_INFO2_MASK_EMASK2  0x0004  /* R/W                               */
#define D_SDIO_SD_INFO2_MASK_EMASK1  0x0002  /* R/W CRC                                     */
#define D_SDIO_SD_INFO2_MASK_EMASK0  0x0001  /* R/W                                 */
#define D_SDIO_SD_CLK_CTRL_SCKLEN    0x0100  /* R/W SDCLK                                           */
#define D_SDIO_SD_CLK_CTRL_DIV_MASK  0x00FF  /* R/W                                     */
#define D_SDIO_SD_OPTION_WIDTH       0x8000  /* R/W                                               */
#define D_SDIO_SD_OPTION_TOP_MASK    0x00F0  /* R/W                       */
#define D_SDIO_SD_OPTION_CTOP_MASK   0x000F  /* R/W                           */
#define D_SDIO_SD_ERR_STS1_E11       0x0800  /* R   CRC                                 */
#define D_SDIO_SD_ERR_STS1_E10       0x0400  /* R   CRC                                     */
#define D_SDIO_SD_ERR_STS1_E9        0x0200  /* R   SD_STOPCRC                                */
#define D_SDIO_SD_ERR_STS1_E8        0x0100  /* R   CRC                               */
#define D_SDIO_SD_ERR_STS1_E5        0x0020  /* R   CRC                   */
#define D_SDIO_SD_ERR_STS1_E4        0x0010  /* R                               */
#define D_SDIO_SD_ERR_STS1_E3        0x0008  /* R   SD_STOP                       */
#define D_SDIO_SD_ERR_STS1_E2        0x0004  /* R                         */
#define D_SDIO_SD_ERR_STS1_E1        0x0002  /* R   SD_STOP             */
#define D_SDIO_SD_ERR_STS1_E0        0x0001  /* R               */
#define D_SDIO_SD_ERR_STS2_E6        0x0040  /* R   CRCNbusy        */
#define D_SDIO_SD_ERR_STS2_E5        0x0020  /* R   NCRC  */
#define D_SDIO_SD_ERR_STS2_E4        0x0010  /* R   N */
#define D_SDIO_SD_ERR_STS2_E3        0x0008  /* R   SD_STOPNbusy                      */
#define D_SDIO_SD_ERR_STS2_E2        0x0004  /* R   R1bNbusy                */
#define D_SDIO_SD_ERR_STS2_E1        0x0002  /* R   SDCLK640SD_STOP     */
#define D_SDIO_SD_ERR_STS2_E0        0x0001  /* R   SDCLK640    */
#define D_SDIO_CC_EXT_MODE_DMASDRW   0x0002  /* R/W SDR/WDMA                                */
#define D_SDIO_SOFT_RST_ORST         0x0002  /* R/W SDIO SD Memory Card   */
#define D_SDIO_SOFT_RST_SDRST        0x0001  /* R/W SD Memory Card                  */

/*------- USB            0xDC140000 ----------------------------------------*/


#endif                                           /* _UNIPHIER_BIT_H_          */
/******************************************************************************/
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
