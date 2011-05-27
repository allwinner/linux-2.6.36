/******************************************************************************/
/*        SDMA                                                      */
/*      sdma_local.h                                              */
/*                                                                    */
/*      2005.03.24                                                */
/*                                                            */
/*        SDMA                              */
/*                  <8601822>sdma.h           */
/*                                                                            */
/*   Unpublished Copyright(c)  2005                   */
/******************************************************************************/
/***** FOMA DTV1  ***********************************************/
/*  No.                                       */
/*              DTV1            05.03.24      */
/* <D1IO013>                    UniPhier2m ES1      05.04.13      */
/* <7600274>        CHTBL               */
/*                                                      05.06.01      */
/* <7600374>        DMAreturn */
/*                                                      05.06.08      */
/* <6700339>        SD          */
/*                                                      05.06.14      */
/* <8601822>        Linux                   */
/*                                      */
/*                                            */
/*                                                      05.06.29      */
/* <8601823>        Linux                   */
/*                                                      */
/*                                                      05.06.29      */
/* <8601821>        Linux                   */
/*                              HW    */
/*                                            */
/*                                                      05.06.29      */
/*<8601825>                     Linux                   */
/*                                          */
/*                                                */
/*                                          05.06.29      */
/*<6600850>                                                             */
/*                      SDMA TBL        05.07.27      */
/*<6600849>               wait_queue  05.07.27      */
/*<8602185>      SDMA  Define                                       */
/*                          proc  05.08.31      */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/

#ifndef SDMA_LOCAL_H
#define SDMA_LOCAL_H

/* ************************************************************************** *
 * 
 * ************************************************************************** */

/* SDMA   ***************************************************** */

/* SDMA TBL ------------------------------------------------------------- */
/*<6600850>typedef struct T_tagSDMA_INFO {                                    */
/*<7600274>unsigned char        use_state;      *//* SDMA             */
/*<6600850>    unsigned short        use_state; *//*<7600274> SDMA    */
/*<6600850>} t_sdma_info;                                                     */


/* SDMA TBL --------------------------------------------------- */
typedef struct T_tagSDMA_CH_INFO {
    unsigned char        ch_state;                /*          */
    unsigned char        ch_state2;               /*          */
    unsigned char        sync;                    /*  /             */
    unsigned char        dummy;                   /*                    */
    unsigned short       sbuf_addr;               /* Sbuf         */
    unsigned short       sbuf_size;               /* Sbuf           */
    unsigned short       intena;                  /*          */
    unsigned char        dummy2[2];               /*                    */
/*<6600849>wait_queue_head_t*   wait_queue;     *//*    */
    pid_t                proc_id;                 /* ID   */
    void                 (*callback)( unsigned char, short );
                                                  /*      */
    struct timer_list    timer_st;                /*  Timer       */
    short                error_flg;               /*              */
    unsigned char        event_flg;               /*<7600374>   */
} t_sdma_ch_info;


/* Other ******************************************************************** */

/* SDMA TBL ----------------------------------------- */
typedef struct T_tagREQIRQ_INIT_SDMA{
    unsigned int   irq_num;
    void           (*callback)(int, void *, struct pt_regs *);
    char  *dev_name;
} t_reqirq_init_sdma;



/* ************************************************************************** *
 * Define
 * ************************************************************************** */
/* ************************************************************************** *
 * 
 * ************************************************************************** */
#define D_SDMA_OFF                  0             /*OFF                   */
#define D_SDMA_ON                   1             /*ON                    */

#define D_SDMA_NULL                 0             /*NULL                  */
/*<8601823>#define D_SDMA_NO00      0x00        *//*                          */
/*<8601823>#define D_SDMA_NO01      0x01        *//*                          */
/*<8601823>#define D_SDMA_NO02      0x02        *//*                          */
/*<8601823>#define D_SDMA_NO03      0x03        *//*                          */
/*<8601823>#define D_SDMA_NO04      0x04        *//*                          */
/*<8601823>#define D_SDMA_NO05      0x05        *//*                          */
/*<8601823>#define D_SDMA_NO06      0x06        *//*                          */
/*<8601823>#define D_SDMA_NO07      0x07        *//*                          */
/*<8601823>#define D_SDMA_NO08      0x08        *//*                          */
/*<8601823>#define D_SDMA_NO09      0x09        *//*                          */

/* Bit Mask */
#define D_SDMA_BITMASK_LOW          0x0000FFFF    /*                          */
/*<8601823>#define D_SDMA_BITMASK_HIGH     0xFFFF0000*//*                     */
#define D_SDMA_BITMASK_LOWEST8      0x000000FF    /*                          */
/*<8601823>#define D_SDMA_BITMASK_LOWER8   0x0000FF00*//*                     */
/*<8601823>#define D_SDMA_BITMASK_HIGHER8  0x00FF0000*//*                     */
/*<8601823>#define D_SDMA_BITMASK_HIGHEST8 0xFF000000*//*                     */
#define D_SDMA_BITMASK_LOWER24      0x00FFFFFF    /*                          */
#define D_SDMA_BITMASK_LOWEST1      0x00000001    /*                          */
/*<8601823>#define D_SDMA_BITMASK_HIGHEST1 0x80000000*//*                     */

/* Bit Shift ---------------------------------------------------------------- */
#define D_SDMA_SHIFT2               2             /* (2bit)       */
#define D_SDMA_SHIFT8               8             /* (8bit)       */
#define D_SDMA_SHIFT16              16            /* (16bit)      */
#define D_SDMA_SHIFT24              24            /* (24bit)      */
#define D_SDMA_SHIFT29              29            /* (29bit)      */
#define D_SDMA_SHIFT31              31            /* (31bit)      */

/* SDMA  Define *************************************************** */

/* Other -------------------------------------------------------------------- */
/* --                                                     */
#define D_SDMA_TIMEOUT_VALUE        5             /* 5s Sleep                 */

/* --                                                             */
#define D_SDMA_IRQ_NUM_0            35
#define D_SDMA_IRQ_NUM_1            36
#define D_SDMA_IRQ_NUM_2            37
#define D_SDMA_IRQ_NUM_3            38
#define D_SDMA_IRQ_NUM_4            39
#define D_SDMA_IRQ_NUM_5            40
#define D_SDMA_IRQ_NUM_6            41
#define D_SDMA_IRQ_NUM_7            42
#define D_SDMA_IRQ_NUM_8            43
#define D_SDMA_IRQ_NUM_9            44
#define D_SDMA_IRQ_NUM_10           45
#define D_SDMA_IRQ_NUM_11           46
#define D_SDMA_IRQ_NUM_12           47
#define D_SDMA_IRQ_NUM_13           48

/* --DMA                                    */
#define D_SDMA_STSPOL               500           /* 500                    */

/*  Define ---------------------------------------------------- */
/* --SDMA                                                             */
/*<6600850>#define D_SDMA_INIT_USESTATE        0*//* ADMA       */
/* --                                                         */
#define D_SDMA_CH_USE               0x01          /*            */
#define D_SDMA_CH_FREE              0x00          /*            */
/* --                                                         */
#define D_SDMA_CH_RUNNING           0x01          /*    */
#define D_SDMA_CH_STOPPED           0x00          /*            */

/* --                                                         */
#define D_SDMA_CH_SYNC              0x01          /*                  */
#define D_SDMA_CH_ASYNC             0x00          /*                */
/* --                                                             */
#define D_SDMA_ERR_NON              0x00          /*                */
/* --                                                           */
/*<8601823>#define D_SDMA_EVENT_ON  0x01                                      */
/*<8601823>#define D_SDMA_EVENT_OFF 0x00                                      */

/* SDMA   Define                                    */
/* SDMA DMA CLK_CTRL -------------------------------------------------------- */
/* --IF                                           */
/*<8601821>#define D_SDMA_SHIFT_CLK_MEMEN_MCB 31*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_MEMEN_MCB ( 1<<D_SDMA_SHIFT_CLK_MEMEN_MCB )*/
                                                  /* 1:ON     0:OFF           */
/* --                             */
/*<8601821>#define D_SDMA_SHIFT_CLK_BUFEN     30*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_BUFEN    ( 1<<D_SDMA_SHIFT_CLK_BUFEN )   */
                                                  /* 1:ON     0:OFF           */
/* --                                     */
/*<8601821>#define D_SDMA_SHIFT_CLK_REGEN     29*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_REGEN    ( 1<<D_SDMA_SHIFT_CLK_REGEN )   */
                                                  /* 1:ON     0:OFF           */
/* --()                                     */
/*<8601821>#define D_SDMA_SHIFT_CLK_RESERVE28 28*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_RESERVE28 ( 1<<D_SDMA_SHIFT_CLK_RESERVE28 )*/
                                                  /* 1:ON     0:OFF           */
/* --MCB                                      */
/*<8601821>#define D_SDMA_SHIFT_CLK_MEM       15*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_MEM      ( 1<<D_SDMA_SHIFT_CLK_MEM )     */
                                                  /* 1:ON     0:OFF           */
/* -- ON                                        */
/*<8601821>#define D_SDMA_SHIFT_CLK_BUF       14*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_BUF      ( 1<<D_SDMA_SHIFT_CLK_BUF )     */
                                                  /* 1:ON     0:OFF           */
/* -- ON()                                              */
/*<8601821>#define D_SDMA_SHIFT_CLK_RESERVE13 13*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_RESERVE13 ( 1<<D_SDMA_SHIFT_CLK_RESERVE13 )*/
                                                  /* 1:ON     0:OFF           */
/* -- ON()                                              */
/*<8601821>#define D_SDMA_SHIFT_CLK_RESERVE12 12*//*              */
/*<8601821>#define D_SDMA_BITSET_CLK_RESERVE12 ( 1<<D_SDMA_SHIFT_CLK_RESERVE12 )*/
                                                  /* 1:ON     0:OFF           */
/* -- OFF                                                 */
/*<8601821>#define D_SDMA_BITSET_CLK_OFF 0x00000000*//*           */
                                                  /*          */

/* SDMA DMA CLK_CTRL2 ------------------------------------------------------- */
/*<8601821>#ifndef DTV1_ES1_ERRATA              *//*<D1IO013>                 */
/*<8601821>#define D_SDMA_BITSET_CLK2 0xFFFFFFFF*//* CH     */
                                                  /*              */
                                                  /* CH         */
/*<8601821>#else *//*<D1IO013> DTV1_ES1_ERRATA */
/*<8601821>#define D_SDMA_BITSET_CLK2 0x0000FFFF*//* CH     */
                                                  /*              */
                                                  /* CH         */
/*<8601821>#endif *//*<D1IO013> DTV1_ES1_ERRATA */
/*<8601821>#define D_SDMA_BITSET_CLK2_OFF 0x00000000 *//*         */
                                                  /*          */
                                                  
/* SDMA DMA DMA_SIZE -------------------------------------------------------- */
/* --                                                               */
/*<8601821>#define D_SDMA_SHIFT_ENDLESS       24*//*              */
/*<8601821>#define D_SDMA_BITSET_ENDLESS      ( 1<<D_SDMA_SHIFT_ENDLESS )     */
                                                  /* 1:         */
                                                  /* 0:         */

/* ARMSTREAM-IO ------------------------------------------------- */
/*<8601823>#define D_CLKGEN_SDMA_MSK 0x0020                                   */
                                                  /* Clk Generaer ON          */

/* ARMVDDI3 ----------------------------------------------------- */
/*<8601823>#define D_RSTCTL_SDMA_ON  0x0001                                   */
                                                  /* Reset off                */


/*  Define -------------------------------------------------- */
#define D_SDMA_CH_MIN        D_SDMA_CH0
#define D_SDMA_CH_MAX        D_SDMA_CH13
#define D_SDMA_CH_NUM        D_SDMA_CH_MAX+1
/*<8601823>#define D_SDMA_FSIZE_MIN     0                                     */
#define D_SDMA_FSIZE_MAX     0x00FFFFFF
/*<8601823>#define D_SDMA_ISIZE_MIN     0                                     */
#define D_SDMA_ISIZE_MAX     0x0000FFFF
/*<8601823>#define D_SDMA_MSTART_MIN    0                                     */
/*<6700339>#define D_SDMA_MSTART_MAX    0x00FFFFFF                            */
#define D_SDMA_MSTART_MAX    0x03FFFFFF           /*<6700339>ES2.0 SDRAM 256MB*/
/*<8601823>#define D_SDMA_MSIZE_MIN     0                                     */
#define D_SDMA_MSIZE_MAX     0x003FFFFF
/*<8601823>#define D_SDMA_SBUFSIZE_MIN  0                                     */
#define D_SDMA_SBUFSIZE_MAX  0x07FF
/*<8601823>#define D_SDMA_ENDIAN_MIN    D_SDMA_ENDIAN_LITTLE                  */
#define D_SDMA_ENDIAN_MAX    D_SDMA_ENDIAN_32BIG

/*<8601825> SYS Define ---------------------------------- */
/*<8601825> --MajorNoMinorNo                                                */
#define D_SDMA_MAJOR         0xFF                 /*<8601825>MajorNo      */
#define D_SDMA_MINOR         0xFF                 /*<8601825>MinorNo      */
/*<8601825> --ID                                                          */
/*<8601825> --kernel SDMA ID : 0x01080000 - 0x0108001F                  */
#define D_SDMA_DEVICE_ERR    0x01080000           /*<8601825>StreamDMA Error  */
#define D_SDMA_TIMEOUT_ERR   0x01080001           /*<8601825>Timeout Error    */
/*<8601825> --ACPU PERIPHERAL ERROR COUNTER                                   */
#define D_SDMA_ERR_CNT_MIN   0x00                 /*<8601825>Counter    */
#define D_SDMA_ERR_CNT_MAX   0xFF                 /*<8601825>CounterMAX     */

/*<7600374>  */
#define D_SDMA_EVENT_ON              0x01/*<7600374>*/
#define D_SDMA_EVENT_OFF             0x00/*<7600374>*/

/*  */
/*<8601823>#define DMA_CALLBACK_NON (void *)0xFFFFFFFF*//**/

/* No.                             *//*<8602185>                */
#define D_SDMA_MODULE_NON                   0x0000 /*<8602185>          */
#define D_SDMA_P_SDMA_START                 0x0100 /*<8602185>        */
#define D_SDMA_P_SDMA_STOP                  0x0200 /*<8602185>        */
#define D_SDMA_P_SDMA_LOCAL_INPUT_DATA_CHK  0x0300 /*<8602185>*/
#define D_SDMA_P_SDMA_LOCAL_CH_INFO_SET     0x0400 /*<8602185>TBL */
#define D_SDMA_P_SDMA_LOCAL_CH_INFO_CLR     0x0500 /*<8602185>TBL     */
                                                   /*<8602185>        */
#define D_SDMA_P_SDMA_LOCAL_BOOT            0x0600 /*<8602185>Boot        */
#define D_SDMA_P_SDMA_LOCAL_SHUTDOWN        0x0700 /*<8602185>Shutdown    */
#define D_SDMA_P_SDMA_LOCAL_CH_RESET        0x0800 /*<8602185>ch reset    */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_0   0x0900 /*<8602185>Ch0 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_1   0x0A00 /*<8602185>Ch1 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_2   0x0B00 /*<8602185>Ch2 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_3   0x0C00 /*<8602185>Ch3 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_4   0x0D00 /*<8602185>Ch4 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_5   0x0E00 /*<8602185>Ch5 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_6   0x0F00 /*<8602185>Ch6 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_7   0x1000 /*<8602185>Ch7 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_8   0x1100 /*<8602185>Ch8 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_9   0x1200 /*<8602185>Ch9 */
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_10  0x1300 /*<8602185>Ch10*/
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_11  0x1400 /*<8602185>Ch11*/
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_12  0x1500 /*<8602185>Ch12*/
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_13  0x1600 /*<8602185>Ch13*/
#define D_SDMA_P_SDMA_LOCAL_INT_HANDLER_SUB 0x1700 /*<8602185>*/
#define D_SDMA_P_SDMA_LOCAL_ERROR_INT       0x1800 /*<8602185>  */
#define D_SDMA_P_SDMA_LOCAL_INT_UNLIMITED   0x1900 /*<8602185>    */
                                                   /*<8602185>    */
#define D_SDMA_P_SDMA_LOCAL_INT_LIMITED     0x2000 /*<8602185>    */
                                                   /*<8602185>        */
#define D_SDMA_P_SDMA_LOCAL_TIMEOUT_HANDLER 0x2100 /*<8602185>    */
                                                   /*<8602185>        */
#define D_SDMA_INIT_SDMA_MODULE             0x2200 /*<8602185>      */
                                                   /*<8602185>      */
#define D_SDMA_CLEANUP_SDMA_MODULE          0x2300 /*<8602185>      */
                                                   /*<8602185>        */
#define D_SDMA_P_SDMA_READ_PROC             0x2400 /*<8602185>read_proc   */

#endif                                          /*SDMA_LOCAL_H                */
/******************************************************************************/
/*   Unpublished Copyright(c)  2005                   */
/******************************************************************************/
