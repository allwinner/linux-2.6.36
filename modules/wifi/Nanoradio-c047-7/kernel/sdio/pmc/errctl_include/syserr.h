/******************************************************************************/
/*          :  RESET                                                  */
/*        :  syserr.h                                               */
/*        :  2004.02.05                                             */
/*            :                                                */
/*          :                                                     */
/*                                                                            */
/*         Unpublished Copyright(c)  2004             */
/******************************************************************************/
/*****  ****************************************************/
/*  No.                                       */
/*                                                                            */
/*                                04.02.05   */
/*         & Coding          04.04.08 GT    */
/*        /               */
/*                                                        04.07.09 GT    */
/*      SYSMAX  412             04.08.23 GT    */
/*      syserr_TimeStamp                                                  */
/*                        staticEXPORT_SYMBOL   04.09.02 GT    */
/*      ResetMask     RestMaskI/F               04.09.03 GT    */
/*      srus_sw_reset srus_sw_resetI/F          04.09.28 GT    */
/*<6105553> SYSERR_RESET_WAIT Define              04.10.19 GT    */
/*<6106457> SYSERR_ROM_WR_TIMER Define                                */
/*          ms_alarm_rom_write_proc     04.11.15 GT    */
/******************************************************************************/
/***** FOMA DTV1 ************************************************/
/*  No.                                       */
/*                                                                            */
/*<6600845>      srus_read_reset_mask [P901i]                             */
/*                                  05.07.28     */
/*<8602169>      ms_alarm_rom_write_proc                                      */
/*                                        05.09.01     */
/*<8602253>      warm_reset_proc/ErrCtl_Vaddr                                 */
/*                                        05.09.16     */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _LINUX_SYSERR_H_
#define _LINUX_SYSERR_H_

#define CSYSERR_ALM_RANK_A      0x01        /* A                */
#define CSYSERR_ALM_RANK_B      0x02        /* B                */

#define CSYSERR_SYSINF_LEN      32          /*          */
#define CSYSERR_LOGINF_LEN      32          /*                */
#define CSYSERR_SYSINF_MAX      12          /* SYSMAX           */

#define SYSERR_RST_MASK_ON      0x01        /* Reset Mask                 */
#define SYSERR_RST_MASK_OFF     0x00        /* Reset Mask                 */

#define SYSERR_ROM_WR_TIMER     15*(HZ/10)  /* Rom Write(1.5)   */

/* ResetDefine */
/*  04/10/19 */
#define SYSERR_RESET_WAIT    /* <6105553> add */

/*----------------------------------------------------------------------------*/
/*                                                      */
/*----------------------------------------------------------------------------*/
typedef struct system_log_type {            /* TAG            */
    unsigned char  timestamp[16];           /*                  */
    unsigned short major;                   /*                      */
    unsigned short minor;                   /* No                     */
    void           *pc;                     /*      */
    void           *cause_addr;             /*              */
    void           *next_summary;           /* Next Summary Pointer           */
} SYSTEM_LOG_TYPE;

typedef struct alarm_log_type {             /* TAG        */
    unsigned char  ubkind;                  /*                  */
    unsigned char  ubreserved[3];           /* ()             */
    unsigned long  uifunc_no;               /*                        */
    SYSTEM_LOG_TYPE sys_dat;                /*          */
    unsigned char  log_dat[CSYSERR_LOGINF_LEN]; /*      */
} ALARM_LOG_TYPE;                           /*                                */

typedef struct syserr_log_type {            /* SYS            */
    unsigned char  ubindex;                 /*SYSINDEX*/
    unsigned char  ubreserved[3];           /* ()             */
    ALARM_LOG_TYPE abuf[CSYSERR_SYSINF_MAX];/* SYS          */
} SYSERR_LOG_TYPE;                          /*                                */

/*----------------------------------------------------------------------------*/
/*  write                       */
/*----------------------------------------------------------------------------*/
typedef struct write_log_type {             /* TAG        */
    unsigned char  ubkind;                  /*                  */
    unsigned char  ubreserved[3];           /* ()             */
    unsigned long  uifunc_no;               /*                        */
    unsigned char  log_dat[CSYSERR_LOGINF_LEN]; /*      */
} WRITE_LOG_TYPE;                           /*                                */

/********************************************************************<8602253>*/
/*                                                       <8602253>*/
/********************************************************************<8602253>*/
extern unsigned char   *ErrCtl_Vaddr;            /*<8602253>SDRAM */

/******************************************************************************/
/*                                                            */
/******************************************************************************/
void system_reset( int );
void warm_reset( void );
void ms_alarm_proc( unsigned char,
                    unsigned long,
                    void *,
                    size_t,
                    void *,
                    size_t );
void syserr_TimeStamp( unsigned long *,
                       unsigned long *,
                       time_t *,
                       time_t *,
                       time_t *,
                       time_t * );
void srus_set_reset_mask( unsigned char );
/*<6600845>void srus_read_reset_mask( unsigned char *);                       */
void srus_sw_reset( void );
/*<8602169>void ms_alarm_rom_write_proc( void );                              */

#endif/* _LINUX_SYSERR_H_ */
/******************************************************************************/
/*         Unpublished Copyright(c)  2004             */
/******************************************************************************/
