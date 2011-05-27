/******************************************************************************/
/*        SDMA                                                      */
/*      sdma.h                                                    */
/*                                                                    */
/*      2005.03.24                                                */
/*                                                            */
/*                                                                  */
/*                                                                            */
/*   Unpublished Copyright(c)  2005                   */
/******************************************************************************/
/***** FOMA DTV1  ***********************************************/
/*  No.                                       */
/*              DTV1            05.03.24      */
/* <D1IO013>                    UniPhier2m ES1      05.04.13      */
/* <7600274>        CHTBL               */
/*                                                      05.06.01      */
/* <D1IO075>        SDMA IDBSSI2                      */
/*                                                      05.06.06      */
/* <7600374>        DMAreturn */
/*                                                      05.06.08      */
/* <6700339>        SD          */
/*                                                      05.06.14      */
/* <D1IO082>        StreamBuffer*/
/*                              define        05.06.21      */
/* <8601822>        Linux                   */
/*                                      */
/*                                            */
/*                                                      05.06.29      */
/* <8601820>        Linux                   */
/*                              #if 0                     */
/*                                                      05.06.29      */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/

#ifndef SDMA_H
#define SDMA_H


#include <linux/module.h>                         /* loadable                 */
#include <linux/timer.h>                          /* timer                    */
#include <linux/sched.h>                          /*                          */
/*<8601822>#include <asm/arch/port_rw.h>        *//* PORT_RW              */
/*<8601822>#include <asm/arch/uniphier2m_addr.h>*//* UNIPHIER2M_ADDR      */


/* ************************************************************************** *
 * 
 * ************************************************************************** */

/* SDMA   *************************************************** */

/* SDMA ------------------------------------------------------- */
typedef struct T_tagSDMA_REQ {
    unsigned short       intena;                  /*          */
    unsigned char        sleep_flg;               /*            */
    unsigned char        ch;                      /*              */
    unsigned char        src_id;                  /* ID                 */
    unsigned char        dst_id;                  /* ID                 */
/*<8601820>#if 0     *//* DTV1  */
#ifdef SDMA_DTV1_SUPPORT                          /*<8601820> DTV1      */
    unsigned char        aux;                     /* AUX  */
    unsigned char        sid;                     /* ID     */
/*<8601820>#endif    *//* DTV1  */
#endif                                            /*<8601820>SDMA_DTV1_SUPPORT*/
    unsigned char        bswp;                    /*        */
    unsigned char        type;                    /* Type                 */
    union U_tagSDMA_SIZE
    {
      unsigned long      f_size;                  /*                */
      unsigned long      i_size;                  /*    */
    }u_sdma_size;
    unsigned long        mrstart;                 /* read   */
    unsigned long        mrsize;                  /* read     */
    unsigned long        mwstart;                 /* write  */
    unsigned long        mwsize;                  /* write    */
    unsigned char        sbuf_uint;               /* Sbuf . */
    unsigned char        dummy;                   /*                    */
    unsigned short       sbuf_stadr;              /* Sbuf         */
    unsigned short       sbuf_width;              /* Sbuf           */
    unsigned char        dummy2[2];               /*                    */
    unsigned long        sbuf_in;                 /* Sbuf       */
    unsigned long        sbuf_out;                /* Sbuf       */
    unsigned short       sbuf_sh;                 /* Sbuf                 */
/*<8601820>#if 0     *//* DTV1  */
#ifdef SDMA_DTV1_SUPPORT                          /*<8601820> DTV1      */
    unsigned char        aux_uint;                /* AUX  . */
    unsigned short       aux_stadr;               /* AUX          */
    unsigned short       aux_width;               /* AUX            */
    unsigned short       aux_in;                  /* AUX        */
    unsigned short       aux_out;                 /* AUX        */
    unsigned short       aux_sh;                  /* AUX                  */
/*<8601820>#endif    *//* DTV1  */
#endif                                            /*<8601820>SDMA_DTV1_SUPPORT*/
    unsigned char        dummy3[2];               /*                    */
    void                 (*callback)(unsigned char,short);
                                                  /* CallBack             */
} t_sdma_req;


/* SDMA   ***************************************************** */

/* SDMA TBL ------------------------------------------------------------- */
/*<8601822>typedef struct T_tagSDMA_INFO {                                    */
/*<7600274>unsigned char        use_state;      *//* SDMA             */
/*<8601822>unsigned short       use_state;      *//*<7600274> SDMA    */
/*<8601822>} t_sdma_info;                                                     */


/* SDMA TBL --------------------------------------------------- */
/*<8601822>typedef struct T_tagSDMA_CH_INFO {                                 */
/*<8601822>unsigned char        ch_state;       *//*          */
/*<8601822>unsigned char        ch_state2;      *//*          */
/*<8601822>unsigned char        sync;           *//*  /             */
/*<8601822>unsigned char        dummy;          *//*                    */
/*<8601822>unsigned short       sbuf_addr;      *//* Sbuf         */
/*<8601822>unsigned short       sbuf_size;      *//* Sbuf           */
/*<8601822>unsigned short       intena;         *//*          */
/*<8601822>unsigned char        dummy2[2];      *//*                    */
/*<8601822>wait_queue_head_t*   wait_queue;     *//*    */
/*<8601822>pid_t                proc_id;        *//* ID   */
/*<8601822>void (*callback)( unsigned char, short );                          */
                                                  /*      */
/*<8601822>struct timer_list    timer_st;       *//*  Timer       */
/*<8601822>short                error_flg;      *//*              */
/*<8601822>unsigned char        event_flg;      *//*<7600374>   */
/*<8601822>} t_sdma_ch_info;                                                  */


/* Other ******************************************************************** */

/* SDMA TBL ----------------------------------------- */
/*<8601822>typedef struct T_tagREQIRQ_INIT_SDMA{                              */
/*<8601822>unsigned int   irq_num;                                            */
/*<8601822>void           (*callback)(int, void *, struct pt_regs *);         */
/*<8601822>char  *dev_name;                                                   */
/*<8601822>} t_reqirq_init_sdma;                                              */



/* ************************************************************************** *
 * Define
 * ************************************************************************** */
/* ************************************************************************** *
 * 
 * ************************************************************************** */
/*<8601822>#define D_SDMA_OFF              0    *//*OFF                   */
/*<8601822>#define D_SDMA_ON               1    *//*ON                    */

/*<8601822>#define D_SDMA_NULL             0    *//*NULL                  */
/*<8601822>#define D_SDMA_NO00             0x00 *//*                          */
/*<8601822>#define D_SDMA_NO01             0x01 *//*                          */
/*<8601822>#define D_SDMA_NO02             0x02 *//*                          */
/*<8601822>#define D_SDMA_NO03             0x03 *//*                          */
/*<8601822>#define D_SDMA_NO04             0x04 *//*                          */
/*<8601822>#define D_SDMA_NO05             0x05 *//*                          */
/*<8601822>#define D_SDMA_NO06             0x06 *//*                          */
/*<8601822>#define D_SDMA_NO07             0x07 *//*                          */
/*<8601822>#define D_SDMA_NO08             0x08 *//*                          */
/*<8601822>#define D_SDMA_NO09             0x09 *//*                          */

/*<D1IO001> Bit Mask */
/*<8601822>#define D_SDMA_BITMASK_LOW      0x0000FFFF *//*                    */
/*<8601822>#define D_SDMA_BITMASK_HIGH     0xFFFF0000 *//*                    */
/*<8601822>#define D_SDMA_BITMASK_LOWEST8  0x000000FF *//*                    */
/*<8601822>#define D_SDMA_BITMASK_LOWER8   0x0000FF00 *//*                    */
/*<8601822>#define D_SDMA_BITMASK_HIGHER8  0x00FF0000 *//*                    */
/*<8601822>#define D_SDMA_BITMASK_HIGHEST8 0xFF000000 *//*                    */
/*<8601822>#define D_SDMA_BITMASK_LOWER24  0x00FFFFFF *//*                    */
/*<8601822>#define D_SDMA_BITMASK_LOWEST1  0x00000001 *//*                    */
/*<8601822>#define D_SDMA_BITMASK_HIGHEST1 0x80000000 *//*                    */

/* Bit Shift ---------------------------------------------------------------- */
/*<8601822>#define D_SDMA_SHIFT2           2    *//* (2bit)       */
/*<8601822>#define D_SDMA_SHIFT8           8    *//* (8bit)       */
/*<8601822>#define D_SDMA_SHIFT16          16   *//* (16bit)      */
/*<8601822>#define D_SDMA_SHIFT24          24   *//* (24bit)      */
/*<8601822>#define D_SDMA_SHIFT29          29   *//* (29bit)      */
/*<8601822>#define D_SDMA_SHIFT31          31   *//* (31bit)      */

/* SDMA  Define ************************************************* */
/* DMA -------------------------------------------------------------- */
#define D_SDMA_CH0                 0              /* DMA0             */
#define D_SDMA_CH1                 1              /* DMA1             */
#define D_SDMA_CH2                 2              /* DMA2             */
#define D_SDMA_CH3                 3              /* DMA3             */
#define D_SDMA_CH4                 4              /* DMA4             */
#define D_SDMA_CH5                 5              /* DMA5             */
#define D_SDMA_CH6                 6              /* DMA6             */
#define D_SDMA_CH7                 7              /* DMA7             */
#define D_SDMA_CH8                 8              /* DMA8             */
#define D_SDMA_CH9                 9              /* DMA9             */
#define D_SDMA_CH10                10             /* DMA10            */
#define D_SDMA_CH11                11             /* DMA11            */
#define D_SDMA_CH12                12             /* DMA12            */
#define D_SDMA_CH13                13             /* DMA13            */

/* ID ------------------------------------------------------------------- */
#define D_SDMA_FWD_ID_SDRAM         0x00          /* SDRAM/OCM(MCB)           */
#define D_SDMA_FWD_ID_SD            0x02          /* SDIP(SD)     */
/*<8601820>#if 0     *//* DTV1  */
#ifdef SDMA_DTV1_SUPPORT                          /*<8601820> DTV1      */
#define D_SDMA_FWD_ID_BSSI1         0x04          /* BSSI1                    */
/*<8601820>#endif    *//*<D1IO075> DTV1  */
#endif                                            /*<8601820>SDMA_DTV1_SUPPORT*/
#define D_SDMA_FWD_ID_BSSI2         0x05          /* BSSI2                    */
/*<8601820>#if 0     *//*<D1IO075> DTV1  */
#ifdef SDMA_DTV1_SUPPORT                          /*<8601820> DTV1      */
#define D_SDMA_FWD_ID_SCB           0x0A          /*            */
#define D_SDMA_FWD_ID_SB            0x0F          /*        */
#define D_SDMA_FWD_ID_USB1          0x11          /* USB EP1                  */
#define D_SDMA_FWD_ID_USB2          0x12          /* USB EP2                  */
#define D_SDMA_FWD_ID_USB3          0x13          /* USB EP3                  */
#define D_SDMA_FWD_ID_USB4          0x14          /* USB EP4                  */
#define D_SDMA_FWD_ID_USB5          0x15          /* USB EP5                  */
#define D_SDMA_FWD_ID_USB6          0x16          /* USB EP6                  */
#define D_SDMA_FWD_ID_USB7          0x17          /* USB EP7                  */
#define D_SDMA_FWD_ID_USB8          0x18          /* USB EP8                  */
#define D_SDMA_FWD_ID_USB9          0x19          /* USB EP9                  */
#define D_SDMA_FWD_ID_USB10         0x1A          /* USB EP10                 */
#define D_SDMA_FWD_ID_USB11         0x1B          /* USB EP11                 */
#define D_SDMA_FWD_ID_USB12         0x1C          /* USB EP12                 */
#define D_SDMA_FWD_ID_USB13         0x1D          /* USB EP13                 */
#define D_SDMA_FWD_ID_USB14         0x1E          /* USB EP14                 */
#define D_SDMA_FWD_ID_USB15         0x1F          /* USB EP15                 */
/*<8601820>#endif    *//* DTV1  */
#endif                                            /*<8601820>SDMA_DTV1_SUPPORT*/

/*  ------------------------------------------------------------- */
#define D_SDMA_ENDIAN_LITTLE        0             /* Little-Endian            */
                                                  /* D3D2D1D0       */
#define D_SDMA_ENDIAN_16BIG         1             /* 16b Big-Endian           */
                                                  /* D2D3D0D1       */
#define D_SDMA_ENDIAN_32HALFBIG     2             /* 32b HalfWord Big-Endian  */
                                                  /* D1D0D3D2       */
#define D_SDMA_ENDIAN_32BIG         3             /* 32b Big-Endian           */
                                                  /* D0D1D2D3       */
/* Sbuf . ------------------------------------------------- */
#define D_SDMA_SBUF_UNIT_4BYTE      0x00          /*              */
#define D_SDMA_SBUF_UNIT_16BYTE     0x01          /*            */
#define D_SDMA_SBUF_UNIT_512BYTE    0x07          /*          */

/* Type ----------------------------------------------------------------- */
#define D_SDMA_FWD_LIMITED          0x00          /*                */
#define D_SDMA_FWD_UNLIMITED        0x01          /*                */

/*  ------------------------------------------------------------- */
#define D_SDMA_INTFCT_SDONE         0x01          /* src -> StreamBuffer  */
                                                  /*          */
#define D_SDMA_INTFCT_DDONE         0x02          /* StreamBuffer -> dst  */
                                                  /*          */
#define D_SDMA_INTFCT_SH            0x04          /* StreamBuffer       */
                                                  /*          */
#define D_SDMA_INTFCT_ILA           0x08          /*          */
#define D_SDMA_INTFCT_ICNT          0x10          /*  */
                                                  /*        */

/*  --------------------------------------------------------- */
#define D_SDMA_INTENA_SDONE         0x01          /* src -> StreamBuffer  */
                                                  /*      */
#define D_SDMA_INTENA_DDONE         0x02          /* StreamBuffer -> dst  */
                                                  /*      */
#define D_SDMA_INTENA_SH            0x04          /* StreamBuffer       */
                                                  /*      */
#define D_SDMA_INTENA_ILA           0x08          /*      */
#define D_SDMA_INTENA_ICNT          0x10          /*  */
                                                  /*    */

/*  ----------------------------------------------------------- */
#define D_SDMA_SLEEP_OFF            0             /*              */
#define D_SDMA_SLEEP_ON             1             /*              */

/*<D1IO082> StreamBuffer Office --------------------------------------------- */
#define D_SDMA_SBUF_OFFSET_NOUSE    0x80000000    /*<D1IO082>                 */

/*  --------------------------------------------------------------- */
#define D_SDMA_OK                    0
#define D_SDMA_NG                   -1
#define D_SDMA_ERROR_CH             -1
#define D_SDMA_ERROR_FWDID          -2
#define D_SDMA_ERROR_ENDIAN         -3
#define D_SDMA_ERROR_FWDTYPE        -4
#define D_SDMA_ERROR_FWDSIZE        -5
#define D_SDMA_ERROR_MWR_ADDR       -6
#define D_SDMA_ERROR_MWR_SIZE       -7
#define D_SDMA_ERROR_SBUF_UNIT      -8
#define D_SDMA_ERROR_SBUF_ADDR      -9
#define D_SDMA_ERROR_SBUF_SIZE      -10
#define D_SDMA_ERROR_SBUF_OFFSET    -11
#define D_SDMA_ERROR_SBUF_SH        -12
#define D_SDMA_ERROR_INTENA         -13
#define D_SDMA_ERROR_SLEEP_FLG      -14
#define D_SDMA_ERROR_ADDR           -15
#define D_SDMA_ERROR_CH_IN_USE      -31
#define D_SDMA_ERROR_SB             -32
#define D_SDMA_STOP                 -33
#define D_SDMA_ERROR                -34
#define D_SDMA_ERROR_ALREADY_STOPED -35


/* SDMA  Define *************************************************** */

/* Other -------------------------------------------------------------------- */
/* --                                                     */
/*<8601822>#define D_SDMA_TIMEOUT_VALUE        5*//* 5s Sleep                 */

/* --                                                             */
/*<8601822>#define D_SDMA_IRQ_NUM_0            35                             */
/*<8601822>#define D_SDMA_IRQ_NUM_1            36                             */
/*<8601822>#define D_SDMA_IRQ_NUM_2            37                             */
/*<8601822>#define D_SDMA_IRQ_NUM_3            38                             */
/*<8601822>#define D_SDMA_IRQ_NUM_4            39                             */
/*<8601822>#define D_SDMA_IRQ_NUM_5            40                             */
/*<8601822>#define D_SDMA_IRQ_NUM_6            41                             */
/*<8601822>#define D_SDMA_IRQ_NUM_7            42                             */
/*<8601822>#define D_SDMA_IRQ_NUM_8            43                             */
/*<8601822>#define D_SDMA_IRQ_NUM_9            44                             */
/*<8601822>#define D_SDMA_IRQ_NUM_10           45                             */
/*<8601822>#define D_SDMA_IRQ_NUM_11           46                             */
/*<8601822>#define D_SDMA_IRQ_NUM_12           47                             */
/*<8601822>#define D_SDMA_IRQ_NUM_13           48                             */

/* --DMA                                    */
/*<8601822>#define D_SDMA_STSPOL             500*//* 500                    */

/*  Define ---------------------------------------------------- */
/* --SDMA                                                             */
/*<8601822>#define D_SDMA_INIT_USESTATE     0   *//* ADMA       */
/* --                                                         */
/*<8601822>#define D_SDMA_CH_USE            0x01*//*            */
/*<8601822>#define D_SDMA_CH_FREE           0x00*//*            */
/* --                                                         */
/*<8601822>#define D_SDMA_CH_RUNNING        0x01*//*    */
/*<8601822>#define D_SDMA_CH_STOPPED        0x00*//*            */

/* --                                                         */
/*<8601822>#define D_SDMA_CH_SYNC           0x01*//*                  */
/*<8601822>#define D_SDMA_CH_ASYNC          0x00*//*                */
/* --                                                             */
/*<8601822>#define D_SDMA_ERR_NON           0x00*//*                */
/* --                                                           */
/*<8601822>#define D_SDMA_EVENT_ON          0x01                              */
/*<8601822>#define D_SDMA_EVENT_OFF         0x00                              */

/* SDMA   Define                                    */
/* SDMA DMA CLK_CTRL -------------------------------------------------------- */
/* --IF                                           */
/*<8601822>#define D_SDMA_SHIFT_CLK_MEMEN_MCB 31*//*             */
/*<8601822>#define D_SDMA_BITSET_CLK_MEMEN_MCB ( 1<<D_SDMA_SHIFT_CLK_MEMEN_MCB )*/
                                                  /* 1:ON     0:OFF           */
/* --                             */
/*<8601822>#define D_SDMA_SHIFT_CLK_BUFEN     30*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_BUFEN    ( 1<<D_SDMA_SHIFT_CLK_BUFEN )   */
                                                  /* 1:ON     0:OFF           */
/* --                                     */
/*<8601822>#define D_SDMA_SHIFT_CLK_REGEN     29*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_REGEN    ( 1<<D_SDMA_SHIFT_CLK_REGEN )   */
                                                  /* 1:ON     0:OFF           */
/* --()                                     */
/*<8601822>#define D_SDMA_SHIFT_CLK_RESERVE28 28*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_RESERVE28 ( 1<<D_SDMA_SHIFT_CLK_RESERVE28 )*/
                                                  /* 1:ON     0:OFF           */
/* --MCB                                      */
/*<8601822>#define D_SDMA_SHIFT_CLK_MEM       15*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_MEM      ( 1<<D_SDMA_SHIFT_CLK_MEM )     */
                                                  /* 1:ON     0:OFF           */
/* -- ON                                        */
/*<8601822>#define D_SDMA_SHIFT_CLK_BUF       14*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_BUF      ( 1<<D_SDMA_SHIFT_CLK_BUF )     */
                                                  /* 1:ON     0:OFF           */
/* -- ON()                                              */
/*<8601822>#define D_SDMA_SHIFT_CLK_RESERVE13 13*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_RESERVE13 ( 1<<D_SDMA_SHIFT_CLK_RESERVE13 )*/
                                                  /* 1:ON     0:OFF           */
/* -- ON()                                              */
/*<8601822>#define D_SDMA_SHIFT_CLK_RESERVE12 12*//*              */
/*<8601822>#define D_SDMA_BITSET_CLK_RESERVE12 ( 1<<D_SDMA_SHIFT_CLK_RESERVE12 )*/
                                                  /* 1:ON     0:OFF           */
/* -- OFF                                                 */
/*<8601822>#define D_SDMA_BITSET_CLK_OFF 0x00000000 *//*          */
                                                  /*          */

/* SDMA DMA CLK_CTRL2 ------------------------------------------------------- */
/*<8601822>#ifndef DTV1_ES1_ERRATA              *//*<D1IO013>                 */
/*<8601822>#define D_SDMA_BITSET_CLK2 0xFFFFFFFF*//* CH     */
                                                  /*              */
                                                  /* CH         */
/*<8601822>#else                                *//*<D1IO013> DTV1_ES1_ERRATA */
/*<8601822>#define D_SDMA_BITSET_CLK2 0x0000FFFF*//* CH     */
                                                  /*              */
                                                  /* CH         */
/*<8601822>#endif                               *//*<D1IO013> DTV1_ES1_ERRATA */
/*<8601822>#define D_SDMA_BITSET_CLK2_OFF 0x00000000*//*          */
                                                  /*          */
                                                  
/* SDMA DMA DMA_SIZE -------------------------------------------------------- */
/* --                                                               */
/*<8601822>#define D_SDMA_SHIFT_ENDLESS       24*//*              */
/*<8601822>#define D_SDMA_BITSET_ENDLESS      ( 1<<D_SDMA_SHIFT_ENDLESS )     */
                                                  /* 1:         */
                                                  /* 0:         */

/* ARMSTREAM-IO ------------------------------------------------- */
/*<8601822>#define D_CLKGEN_SDMA_MSK 0x0020                                   */
                                                  /* Clk Generaer ON          */

/* ARMVDDI3 ----------------------------------------------------- */
/*<8601822>#define D_RSTCTL_SDMA_ON  0x0001                                   */
                                                  /* Reset off                */


/*  Define -------------------------------------------------- */
/*<8601822>#define D_SDMA_CH_MIN        D_SDMA_CH0                            */
/*<8601822>#define D_SDMA_CH_MAX        D_SDMA_CH13                           */
/*<8601822>#define D_SDMA_CH_NUM        D_SDMA_CH_MAX+1                       */
/*<8601822>#define D_SDMA_FSIZE_MIN     0                                     */
/*<8601822>#define D_SDMA_FSIZE_MAX     0x00FFFFFF                            */
/*<8601822>#define D_SDMA_ISIZE_MIN     0                                     */
/*<8601822>#define D_SDMA_ISIZE_MAX     0x0000FFFF                            */
/*<8601822>#define D_SDMA_MSTART_MIN    0                                     */
/*<6700339>#define D_SDMA_MSTART_MAX    0x00FFFFFF                            */
/*<8601822>#define D_SDMA_MSTART_MAX  0x03FFFFFF*//*<6700339>ES2.0 SDRAM 256MB*/
/*<8601822>#define D_SDMA_MSIZE_MIN     0                                     */
/*<8601822>#define D_SDMA_MSIZE_MAX     0x003FFFFF                            */
/*<8601822>#define D_SDMA_SBUFSIZE_MIN  0                                     */
/*<8601822>#define D_SDMA_SBUFSIZE_MAX  0x07FF                                */
/*<8601822>#define D_SDMA_ENDIAN_MIN    D_SDMA_ENDIAN_LITTLE                  */
/*<8601822>#define D_SDMA_ENDIAN_MAX    D_SDMA_ENDIAN_32BIG                   */

/*<7600374>  */
/*<8601822>#define D_SDMA_EVENT_ON      0x01    *//*<7600374>*/
/*<8601822>#define D_SDMA_EVENT_OFF     0x00    *//*<7600374>*/

/*  */
extern short p_sdma_start( t_sdma_req *parameter );
extern short p_sdma_stop( unsigned char ch );
extern void  p_sdma_local_timeout_handler( unsigned long ch );
extern void p_sdma_local_int_handler_0( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_1( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_2( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_3( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_4( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_5( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_6( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_7( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_8( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_9( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_10( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_11( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_12( int irq, void *dev_id, struct pt_regs *regs );
extern void p_sdma_local_int_handler_13( int irq, void *dev_id, struct pt_regs *regs );

/*  */
/*<8601822>#define DMA_CALLBACK_NON (void *)0xFFFFFFFF *//*  */



#endif                                           /*SDMA_H              */
