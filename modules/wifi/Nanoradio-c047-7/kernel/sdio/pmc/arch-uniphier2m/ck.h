/******************************************************************************/
/*          :  (/)                      */
/*        :  ck.h                                                 */
/*        :  2005.03.28                                             */
/*            :                                                   */
/*          :                                                     */
/*                                                                            */
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
/***** DTV1  ****************************************************/
/*  No.                                       */
/*                                                                            */
/* <D1IO026>  DTV1 /  05.03.28        */
/* <D1IO027>      DTV1        05.03.28        */
/* <D1IO057>                       05.04.06        */
/* <D1IO096> real_pclkm_ctrl  /   05.06.02   koikes   */
/* <8601846>    Linux   05.06.30        */
/* <8601847>    Linux   05.06.30        */
/* <8601849>    Linux   05.06.30        */
/* <8602036>    SCHED     05.07.13        */
/* <8650104>    VIN/FDMA    05.08.26        */
/* <8602241>                      05.09.15        */
/* <8650145>            05.09.21        */
/* <8650216>    XIP           05.10.06        */
/* <8650258>    FIFO    05.10.21        */
/* <8650479>    VIN               06.01.23        */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _ASM_ARCH_UNIPHIER2M_CK_H_               /* _ASM_ARCH_UNIPHIER2M_CK_H_*/
#define _ASM_ARCH_UNIPHIER2M_CK_H_

/* <8601847>  START                           */
/*----------------------------------------------------------------------------*/
/*                                                      */
/*----------------------------------------------------------------------------*/
/* /ON/OFF */
#define    DEV_OFF               0               /* OFF               */
#define    DEV_ON                1               /* ON                */

/*  */
#define    DCLKM_OK              0               /* OK                        */
#define    DCLKM_NG             -1               /* NG                        */
#define    DCLKM_PLL2_TO_ERR    -2               /* PLL2TIME OUT  */
#define    DCLKM_CK_OK           1               /*               */
#define    DCLKM_CK_NG           0               /*               */

/* PLL2 ON/OF */
#define    DCLKM_PLL2_OFF        0               /* PLL2 OFF                  */
#define    DCLKM_PLL2_ON         1               /* PLL2 ON                   */

/* / */
#define    DCLKM_CMD_OFF         0               /*                 */
#define    DCLKM_CMD_ON          1               /*                 */

/*  */
#define DCLKM_MAJOR             123              /* MAJOR     */
/*<8650258>#define DCLKM_DEV    "/dev/clkm"    *//*       */
#define DCLKM_DEV               "clkmgr"         /*<8650258> Clock Mgr*/
#define DCLKM_CHG_GEAR           0               /*<8650258> /dev/clkm        */

/*  */                   /*<8650258>                  */
#define DCLKM_CTRL_ABTCYC        1               /*<8650258> /dev/cmdcyc      */
#define DCLKM_ABTCYCLE           0               /*<8650258>  */

/* ID */
#define DCLKM_EN_ATMR            0               /* ARMTIMER          */
#define DCLKM_EN_RTCT            1               /* RTCTIMER          */
#define DCLKM_EN_U1              2               /* UART1             */
#define DCLKM_EN_U2              3               /* UART2Felica   */
#define DCLKM_EN_SSI1            4               /* SSI1              */
#define DCLKM_EN_SSI2            5               /* SSI2              */
#define DCLKM_EN_I2C             6               /* I2C               */
#define DCLKM_EN_U5              7               /* UART5IrDA     */
#define DCLKM_EN_BSSI1           8               /* BSSI1             */
#define DCLKM_EN_BSSI2           9               /* BSSI2             */
#define DCLKM_EN_BSSI3          10               /* BSSI3             */
#define DCLKM_EN_BSSI4          11               /* BSSI4             */
#define DCLKM_EN_BSSI5          12               /* BSSI5             */
#define DCLKM_EN_ADMA           13               /* ADMA              */
#define DCLKM_EN_SDMA           14               /* SDMA              */
#define DCLKM_EN_VIN            15               /* VIN/FDMA          */
#define DCLKM_EN_USB            16               /* USB               */
#define DCLKM_EN_SCHD           17               /* Schedular         */
#define DCLKM_EN_FRAGO          18               /* FRAGO             */
#define DCLKM_EN_JPEG           19               /* JPEG              */
#define DCLKM_EN_SDIO1          20               /* SDIO              */
#define DCLKM_EN_TDDS           21               /* TDDS              */
#define DCLKM_EN_IPP            22               /* IPP               */
#define DCLKM_EN_MP             23               /* MP                */
#define DCLKM_EN_VPP            24               /* VPP               */
#define DCLKM_EN_GPIOD7         25               /* GPIOD7            */
#define DCLKM_EN_GPIOE7         26               /* GPIOE7            */
#define DCLKM_BLOCKID_MAX       27               /* ID          */
/* ID */
#define DCLKM_DRV_ABRIDGE        0               /* Abridge           */
#define DCLKM_DRV_UART           1               /* UART              */
#define DCLKM_DRV_FELICA         2               /* FELICA            */
#define DCLKM_DRV_TIMER          3               /*             */
#define DCLKM_DRV_IPP            4               /* IPP               */
#define DCLKM_DRV_JPEG           5               /* JPEG              */
#define DCLKM_DRV_ADMA           6               /* ADMA              */
#define DCLKM_DRV_TD             7               /* TD                */
#define DCLKM_DRV_OVERLAY        8               /*         */
#define DCLKM_DRV_ATV            9               /* ATV               */
#define DCLKM_DRV_CAMERA        10               /*             */
#define DCLKM_DRV_FDMA          11               /* FDMA              */
#define DCLKM_DRV_USB           12               /* USB               */
#define DCLKM_DRV_SD            13               /* SD                */
#define DCLKM_DRV_IRDA          14               /* IrDA              */
#define DCLKM_DRV_DTVTUNER      15               /* DTV       */
#define DCLKM_DRV_SSI           16               /* SSI           */
#define DCLKM_DRV_I2C           17               /* I2C           */
#define DCLKM_DRV_FB1           18               /* FB1               */
#define DCLKM_DRV_PATH          19               /*               */
#define DCLKM_DRV_IRRC          20               /* IrRC              */
#define DCLKM_DRV_DTVANT        21               /* DTV       */
#define DCLKM_DRV_SECURE        22               /*           */
#define DCLKM_DRV_SDMA          23               /* SDMA              */
#define DCLKM_DEVICEID_MAX      24               /* ID          */
#define DCLKM_DRV_CLKM_BSSI1    25       /* SDMAID(BSSI1) */
#define DCLKM_DRV_CLKM_BSSI2    26       /* SDMAID(BSSI2) */
#define DCLKM_DRV_CLKM_USB      27       /* SDMAID(USB)   */
#define DCLKM_DRV_CLKM_SD       28       /* SDMAID(SD)    */
#define DCLKM_DRV_CLKM_IPL      29               /* SCHDID(IPL)   */

/*  */
typedef enum{
    DCLKM_ARM_CK = 0x0000,                       /* ARM       */
    DCLKM_IPP_CK,                                /* IPP           */
    DCLKM_AHB1_CK,                               /* AHB           */
    DCLKM_APB_CK,                                /* APB           */
    DCLKM_SDMA_CK,                               /* SDMA          */
    DCLKM_SECURE_CK,                             /* SECURE        */
    DCLKM_USB_CK,                                /* USB           */
    DCLKM_SD_CK,                                 /* SD            */
    DCLKM_BSSI1_CK,                              /* BSSI1         */
    DCLKM_BSSI2_CK,                              /* BSSI2         */
    DCLKM_BSSI3_CK,                              /* BSSI3         */
    DCLKM_BSSI4_CK,                              /* BSSI4         */
    DCLKM_BSSI5_CK,                              /* BSSI5         */
    DCLKM_U1_CK,                                 /* UART1         */
    DCLKM_U2_CK,                                 /* UART2         */
    DCLKM_U5_CK ,                                /* UART5         */
    DCLKM_ARMVIC_CK,                             /* ARMVIC        */
    DCLKM_GPIO_CK,                               /* GPIO          */
    DCLKM_SSI1_CK,                               /* SSI1          */
    DCLKM_SSI2_CK,                               /* SSI2          */
    DCLKM_IIC_CK,                                /* I2C           */
    DCLKM_PLL2_CK,                               /* PLL2          */
    DCLKM_SRC_CK,                                /* SRC           */
    DCLKM_ARMTMR_CK,                             /* ARMTIMER      */
    DCLKM_IPPTMR_CK,                             /* IPPTIMER      */
    DCLKM_RTCTMR_CK,                             /* RTCTIMER      */
    DCLKM_U3_CK,                                 /* UART3         */
    DCLKM_U4_CK,                                 /* UART4         */
    DCLKM_KEY_CK                                 /* KEYSCAN       */
}ck_t;

/* CPU */
enum t_CLKM_FACTOR_HIGH                          /*                   */
{
    DCLKM_FACTOR_KERNEL_HIGH = 0x0200,           /*           */
    DCLKM_FACTOR_MAW_HIGH,                       /* MAW               */
    DCLKM_FACTOR_GEARLIB_HIGH,                   /* LIB   */
    DCLKM_FACTOR_KEY_HIGH,                       /*<8650145>  */
    DCLKM_FACTOR_OPNCLS_HIGH                     /*<8650145>  */
};

enum t_CLKM_FACTOR_MID                           /*                   */
{
    DCLKM_FACTOR_GEARLIB_MID = 0x0100            /* LIB   */
};

enum t_CLKM_FACTOR_LOW                           /*                   */
{
    DCLKM_FACTOR_POWONL = 0x0000                 /* ON            */
};

/* <8601847>   END                            */
/*----------------------------------------------------------------------------*/
/*                                                                  */
/*----------------------------------------------------------------------------*/
/* / */
typedef struct _tCLKM_INFO_TBL
{
    volatile unsigned long clkreg;               /*   */
    volatile unsigned long rstreg;               /*     */
    unsigned long clkbit;                        /*         */
    unsigned long rstbit;                        /*         */
    unsigned long wait;                          /*       */
    unsigned long access_wait;                   /*       */
}tclkm_info_tbl;

/*  */
typedef struct _tPM_STR_CLKREQ
{
    unsigned long reg_clk_en1;                   /* 1     */
    unsigned long reg_clk_en2;                   /* 2     */
    unsigned long reg_clk_en3;                   /* 3     */
    unsigned long reg_clk_en4;                   /* 4     */
    unsigned long reg_clk_en5;                   /* 5     */
}pm_str_clkreq;

/*----------------------------------------------------------------------------*/
/*                                                            */
/*----------------------------------------------------------------------------*/
/* / */
extern void init_ck( void );
#ifdef DCLKM_PARTIAL                             /* koikes <D1IO096>          */
extern signed int real_pclkm_ctrl( unsigned long block,
                                   unsigned long drv_id,
                                   unsigned char on_off );
#endif /* DCLKM_PARTIAL */                       /* koikes <D1IO096>          */
extern signed int pclkm_ctrl( unsigned long block,
                              unsigned long drv_id,
                              unsigned char on_off );
extern signed int pclkm_ctrl_pll2( unsigned long drv_id,
                                   unsigned char on_off,
                                   unsigned char src_ck );
extern void pclkm_req_xtal( unsigned char factor,
                            unsigned char ctrl );
extern void pclkm_get_clock_request( pm_str_clkreq *clkreq );
extern unsigned long pclkm_get_block_useinfo( void );
extern signed int ck_get_rate( ck_t ck );
extern signed int ck_valid_rate( int rate );

/*  */
extern signed int pclkm_chg_gear( unsigned char cmd,
                                  unsigned long factor);


/*----------------------------------------------------------------------------*/
/*                                                              */
/*----------------------------------------------------------------------------*/
#ifdef DCLKM_ES1
#define    DCLKM_IPP_CLOCK      0x4              /* IPP     */
#define    DCLKM_IPP_RST        0x4              /* IPP     */
#endif /* DCLKM_ES1 */

/*----------------------------------------------------------------------------*/
/* /                                        */
/*----------------------------------------------------------------------------*/
/* 0        */
#define    DCLKM_ZERO           0                /* 0                   */
/* PLL2 */
#define    DCLKM_PLL2_WAIT      62               /* 62usec                    */
/* PLL2 */
#define    DCLKM12M             1                /* 12MHz                 */
#define    DCLKM27M             0                /* 27MHz                 */
/* / */
#define    DCLKM_NOUSE          0                /*                     */
#define    DCLKM_NOCTRL         0xffffffff       /*                     */
/*  */
#define    DCLKM_288M           288              /* 288MHz                    */
#define    DCLKM_192M           192              /* 192MHZ                    */
#define    DCLKM_96M            96               /* 96MHz                     */
#define    DCLKM_48M            48               /* 48MHz                     */
#define    DCLKM_6M             6                /* 6MHz                      */
#define    DCLKM_32K            32               /* 32KHz                     */

/* <8601846> uniphier_bit.h                                 */

/* IPP */
#define    DCLKM_IPP_GEAR_WAIT  3                /* IPP  72MHz  */

/*  */
#define    DCLKM_BIT0           0x0001           /*  0                  */
#define    DCLKM_BIT1           0x0002           /*  1                  */
#define    DCLKM_BIT2           0x0004           /*  2                  */
#define    DCLKM_BIT3           0x0008           /*  3                  */
#define    DCLKM_BIT4           0x0010           /*  4                  */
#define    DCLKM_BIT5           0x0020           /*  5                  */
#define    DCLKM_BIT6           0x0040           /*  6                  */
#define    DCLKM_BIT7           0x0080           /*  7                  */
#define    DCLKM_BIT8           0x0100           /*  8                  */
#define    DCLKM_BIT9           0x0200           /*  9                  */
#define    DCLKM_BIT10          0x0400           /* 10                  */
#define    DCLKM_BIT11          0x0800           /* 11                  */
#define    DCLKM_BIT12          0x1000           /* 12                  */
#define    DCLKM_BIT13          0x2000           /* 13                  */
#define    DCLKM_BIT14          0x4000           /* 14                  */
#define    DCLKM_BIT15          0x8000           /* 15                  */

/*  */
#define    DCLKM_WAIT_ATMR      0                /* ARMTIMER          */
#define    DCLKM_WAIT_RTMR      0                /* RTCTIMER          */
#define    DCLKM_WAIT_U1        0                /* UART1             */
#define    DCLKM_WAIT_U2        0                /* UART2Felica   */
#define    DCLKM_WAIT_SSI1      0                /* SSI1              */
#define    DCLKM_WAIT_SSI2      0                /* SSI2              */
#define    DCLKM_WAIT_I2C       0                /* I2C               */
#define    DCLKM_WAIT_U5        0                /* UART5IrDA     */
#define    DCLKM_WAIT_BSSI1     0                /* BSSI1             */
#define    DCLKM_WAIT_BSSI2     0                /* BSSI2             */
#define    DCLKM_WAIT_BSSI3     0                /* BSSI3             */
#define    DCLKM_WAIT_BSSI4     0                /* BSSI4             */
#define    DCLKM_WAIT_BSSI5     0                /* BSSI5             */
#define    DCLKM_WAIT_ADMA      0                /* ADMA              */
#define    DCLKM_WAIT_SDMA      0                /* SDMA              */
#define    DCLKM_WAIT_VIN       2                /* VIN/FDMA          */
#define    DCLKM_WAIT_USB       1                /* USB               */
#define    DCLKM_WAIT_SCHD      0                /* Schedular         */
#define    DCLKM_WAIT_FRAGO     1                /* FRAGO             */
#define    DCLKM_WAIT_JPEG      1                /* JPEG              */
#define    DCLKM_WAIT_SDIO1     0                /* SDIO              */
#define    DCLKM_WAIT_TDDS      0                /* TDDS              */
#define    DCLKM_WAIT_IPP       3                /* IPP               */
#define    DCLKM_WAIT_MP        0                /* MP                */
#define    DCLKM_WAIT_VPP       0                /* VPP               */
#define    DCLKM_WAIT_GPIOD7    0                /* GPIOD7            */
#define    DCLKM_WAIT_GPIOE7    0                /* GPIOE7            */

/*  */
#define    DCLKM_ACCESS_WAIT_ATMR      0         /* ARMTIMER          */
#define    DCLKM_ACCESS_WAIT_RTMR      0         /* RTCTIMER          */
#define    DCLKM_ACCESS_WAIT_U1        0         /* UART1             */
#define    DCLKM_ACCESS_WAIT_U2        0         /* UART2Felica   */
#define    DCLKM_ACCESS_WAIT_SSI1      0         /* SSI1              */
#define    DCLKM_ACCESS_WAIT_SSI2      0         /* SSI2              */
#define    DCLKM_ACCESS_WAIT_I2C       0         /* I2C               */
#define    DCLKM_ACCESS_WAIT_U5        0         /* UART5IrDA     */
#define    DCLKM_ACCESS_WAIT_BSSI1     0         /* BSSI1             */
#define    DCLKM_ACCESS_WAIT_BSSI2     0         /* BSSI2             */
#define    DCLKM_ACCESS_WAIT_BSSI3     0         /* BSSI3             */
#define    DCLKM_ACCESS_WAIT_BSSI4     0         /* BSSI4             */
#define    DCLKM_ACCESS_WAIT_BSSI5     0         /* BSSI5             */
#define    DCLKM_ACCESS_WAIT_ADMA      0         /* ADMA              */
#define    DCLKM_ACCESS_WAIT_SDMA      0         /* SDMA              */
#define    DCLKM_ACCESS_WAIT_VIN       2         /* VIN/FDMA          */
#define    DCLKM_ACCESS_WAIT_USB       3         /* USB               */
#define    DCLKM_ACCESS_WAIT_SCHD      0         /* Schedular         */
#define    DCLKM_ACCESS_WAIT_FRAGO     1         /* FRAGO             */
#define    DCLKM_ACCESS_WAIT_JPEG      0         /* JPEG              */
#define    DCLKM_ACCESS_WAIT_SDIO1     0         /* SDIO              */
#define    DCLKM_ACCESS_WAIT_TDDS      0         /* TDDS              */
#define    DCLKM_ACCESS_WAIT_IPP       3         /* IPP               */
#define    DCLKM_ACCESS_WAIT_MP        0         /* MP                */
#define    DCLKM_ACCESS_WAIT_VPP       0         /* VPP               */
#define    DCLKM_ACCESS_WAIT_GPIOD7    0         /* GPIOD7            */
#define    DCLKM_ACCESS_WAIT_GPIOE7    0         /* GPIOE7            */

#define    D_LPM_FUNCNO_01      0x10300000       /*<8601849> PLL2T.O.   */

#define    DCLKM_AODIRB_WAIT    4                /*<8650479>  */

/*----------------------------------------------------------------------------*/
/* CPUÏÇ                                      */
/*----------------------------------------------------------------------------*/
/*  */
#define DCLKM_HIGH              0x0200           /*               */
#define DCLKM_MID               0x0100           /*               */
#define DCLKM_LOW               0x0000           /*               */

/* <8601846> uniphier_bit.h                                 */

/* / */
#define DCLKM_GEAR_MASK         0x0000ff00       /* MASK      */
#define DCLKM_FACTOR_MASK       0x000000ff       /* MASK              */
#define DCLKM_CLEAR             0                /*                   */
/* WAIT */
#define DCLKM_GEAR_WAIT         2                /* WAIT*/
/*  */
#define DCLKM_GEAR_FACTOR_MAX   32               /*  32bit        */
/* loops_per_jiffy */
#ifdef CONFIG_XIP_ROM                            /*<8650216>                  */
#define DCLKM_LPJ_HIGH_GEAR     718848           /*<8650216> LPJ 288MHz       */
#define DCLKM_LPJ_MID_GEAR      479232           /*<8650216> LPJ 192MHz       */
#define DCLKM_LPJ_LOW_GEAR      239104           /*<8650216> LPJ  96MHz       */
#else  /* CONFIG_XIP_ROM */                      /*<8650216>                  */
#define DCLKM_LPJ_HIGH_GEAR     958464           /* loops_per_jiffy 288MHz    */
#define DCLKM_LPJ_MID_GEAR      638976           /* loops_per_jiffy 192MHz    */
#define DCLKM_LPJ_LOW_GEAR      318464           /* loops_per_jiffy  96MHz    */
#endif /* CONFIG_XIP_ROM */                      /*<8650216>                  */

/*---------------------------------------------*//*<8650258>                  */
/* SDRAM ´Ø        *//*<8650258>                  */
/*---------------------------------------------*//*<8650258>                  */
#define DCLKM_ABT_RETRYCNT      5                /*<8650258>        */

#endif                                           /* _ASM_ARCH_UNIPHIER2M_CK_H_*/
/******************************************************************************/
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
