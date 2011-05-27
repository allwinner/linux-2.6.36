/******************************************************************************/
/*          :  (IDLE)                                   */
/*        :  pm.h                                                 */
/*        :  2005.03.28                                             */
/*            :                                                   */
/*          :                                                     */
/*                                                                            */
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
/***** DTV1  ****************************************************/
/*  No.                                       */
/*                                                                            */
/* <D1IO028>      DTV1 IDLE       05.03.28        */
/* <D1IO077>          SDRAM OFF               05.06.20        */
/* <D1IO083> dpm_idle_p       I/F       05.06.23        */
/* <8601846>    Linux   05.06.30        */
/* <8601847>    Linux   05.06.30        */
/* <D1IO084>    SDRAM SLR   05.06.30        */
/* <8602037>          05.07.24        */
/* <6600991>      Sysdes0.93            05.07.28        */
/* <8602241>                      05.09.15        */
/* <8650216>    XIP           05.10.06        */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _ASM_ARCH_UNIPHIER2M_PM_H_               /* _ASM_ARCH_UNIPHIER2M_PM_H_*/
#define _ASM_ARCH_UNIPHIER2M_PM_H_

#include <asm/arch/uniphier2m_addr.h>

/*----------------------------------------------------------------------------*/
/*                                                                  */
/*----------------------------------------------------------------------------*/
/*  */
/*#define    DPM_POWERON_MASK     0xffffffe0   *//* /       */
#ifndef DPM_DORMANT_UART
#define    DPM_POWERON_MASK     0x067DFFFC       /* /       */
#else  /* DPM_DORMANT_UART */
#define    DPM_POWERON_MASK     0x067DFFF8       /* /       */
#endif /* DPM_DORMANT_UART */
/*  */
#define    DPM_TIMER_THRESHOLD  20               /*<D1IO083> (ms)     */

/* <8601846> uniphier_bit.h                                 */

/* SIRIUSINT Bit */
#define    PM_B_SIRIUSINT_IOLINT_P2     0x04     /* P1(H/S)   */
#define    PM_B_SIRIUSINT_IOLINT_P6     0x40     /* P6(Ferica)*/
#define    PM_B_SIRIUSINT_IOLINT_P7     0x80     /* P7(SD mini)     */
#define    PM_B_SIRIUSINT_KEYMODE_KEYMSK 0x08    /* KEY                 */

/*  */
#define    DPM_MODE_USE        0x10              /*               */
#define    DPM_MODE_FIQ        0x11              /* FIQ                 */
#define    DPM_MODE_IRQ        0x12              /* IRQ                 */
#define    DPM_MODE_SVC        0x13              /*         */
#define    DPM_MODE_ABT        0x17              /*             */
#define    DPM_MODE_UND        0x1B              /*               */
#define    DPM_MODE_SYS        0x1F              /*             */

/* ARM */                    /*<8602037>                  */
                                                 /*<8602037>  */
#define    DPM_RESTART_ADDR          ( BACKUP_BASE + 0x00000030 )/*<8602037>  */
                                                 /*<8602037>IBIT*/
#define    DPM_ICACHE_MASTER_VALID   ( BACKUP_BASE + 0x00000034 )/*<8602037>  */
                                                 /*<8602037>DBIT*/
#define    DPM_DCACHE_MASTER_VALID   ( BACKUP_BASE + 0x00000044 )/*<8602037>  */
                                                 /*<8602037> MMU  */
#define    DPM_MMU_TBL               ( BACKUP_BASE + 0x00000054 )/*<8602037>  */
                                                 /*<8602037> TLB  */
#define    DPM_TLB_TBL               ( BACKUP_BASE + 0x00000058 )/*<8602037>  */
                                                 /*<8602037> ARM      */
#define    DPM_SAVE_CORE_INFO        ( BACKUP_BASE + 0x00000358 )/*<8602037>  */
                                                 /*<8602037> ARMEND   */
#define    DPM_SAVE_CORE_INFO_END    ( BACKUP_BASE + 0x00000458 )/*<8602037>  */
                                                 /*<D1IO077> SDRAM DPD  */
#define    DPM_SDRAM_DPD_FLG         ( BACKUP_BASE + 0x0000045C )/*<D1IO077>  */
                                                 /*<8602037> TLB()    */
#define    DPM_TLB_TBL_PHYS          ( DPM_TLB_TBL - BACKUP_BASE + 0xD5190000 )
                                                 /*<8602037> DPD()  */
#define    DPM_SDRAM_DPD_FLG_PHYS    ( DPM_SDRAM_DPD_FLG -                    \
                                       BACKUP_BASE       +                    \
                                       0xD5190000 )/*<8602037>                */

#define    DPM_VIRT_BACKUPMEM_OFFSET BACKUP_BASE /*<8602037> BACKUPoffset */
#define    DPM_PHYS_BACKUPMEM_OFFSET 0xD5190000  /*<8602037> BACKUPoffset */

#define    PMICOM1_SDMCKE_PHYS       0xD61901B0  /*<8602037> SDMCKE ()    */
#define    PMICOM0_SDMSLF0_PHYS      0xD5191100  /*<8602037> SDMSLF0()    */
#define    PMICOM0_SDMSLF1_PHYS      0xD5191104  /*<8602037> SDMSLF1()    */

#define    PMICOM0_BLPUSETR_PHYS     0xD5192214  /*<6600991> BLPUSETR ()  */
#define    PMICOM0_SDRPUCNTR_PHYS    0xD5192220  /*<6600991> SDRPUCNTR()  */
#define    PMICOM0_SDRPUSETR_PHYS    0xD5192224  /*<6600991> SDRPUSETR()  */
#define    PMICOM0_RSTFMOD_PHYS      0xD5112000  /*<6600991> RSTFMOD  ()  */

#define    DPM_VIRT_OFFSET           0xC0000000  /*<8602037>kerneloffset  */
#define    DPM_PHYS_OFFSET           0x90400000  /*<8602037>kerneloffset  */

#ifdef CONFIG_XIP_ROM                            /*<8650216>                  */
#define    DPM_VIRT_OFFSET_XIP_ROM   0xD8000000  /*<8650216>kerneloffset  */
#define    DPM_PHYS_OFFSET_XIP_ROM   0x0         /*<8650216>kerneloffset  */
#endif /* CONFIG_XIP_ROM */                      /*<8650216>                  */

/*<8602037>#define DPM_NOCACHE_AREA 0x90000000 *//*<D1IO084>  */

/* <8601847>                                  */
/*----------------------------------------------------------------------------*/
/*                                                      */
/*----------------------------------------------------------------------------*/
/*  */
#define     DPM_OK              0                /* OK                        */
#define     DPM_NG             -1                /* NG                        */

#ifndef __ASSEMBLER__
/*----------------------------------------------------------------------------*/
/*                                                                  */
/*----------------------------------------------------------------------------*/
/*  */
typedef struct _tPM_REG_SAVE{
    /* GPIO */
    unsigned long    gpioa2_irq;                 /* GPIOA2  (A_WAKEUP)        */
    unsigned long    gpioa3_irq;                 /* GPIOA3  (FRMINT)          */
    unsigned long    gpioa7_irq;                 /* GPIOA7  (SPINTB)          */
    unsigned long    gpiob0_irq;                 /* GPIOB0  (3d_int0)         */
    unsigned long    gpiob1_irq;                 /* GPIOB1  (3d_int1)         */
    unsigned long    gpiob2_irq;                 /* GPIOB2  (3d_int2)         */
    unsigned long    gpiob3_irq;                 /* GPIOB3  (3d_int3)         */
    unsigned long    gpiob4_irq;                 /* GPIOB4  (RSTOB)           */
    unsigned long    gpiob6_irq;                 /* GPIOB6  (REMO_INT)        */
    unsigned long    gpiob7_irq;                 /* GPIOB7  (MIDI_INT)        */
    /* SriusINT */
    unsigned char    sirius_keymsk;              /* KEYMSK(KEYINT)            */
    unsigned char    sirius_iolmsk;              /* IOMSK1(IOLINT)            */
    unsigned short   reserved;                   /* 8byte     */
}_tpm_reg_save;

/*----------------------------------------------------------------------------*/
/*                                                          */
/*----------------------------------------------------------------------------*/
extern unsigned long gdpm_nocache_area;          /*<D1IO084> Cache  */

/*----------------------------------------------------------------------------*/
/*                                                            */
/*----------------------------------------------------------------------------*/
extern void dpm_idle_p(void);

extern void ppm_idle1( void );
extern void ppm_idle2( void );
extern void ppm_idle3( void );

extern void puniphier2m_cpu_idle1( void );
extern void puniphier2m_cpu_idle2( void );
extern void puniphier2m_cpu_idle3( void );

extern void dormant_restart( void );
extern void off_restart( void );

extern signed int ppm_ipp_power_on( void );
extern signed int ppm_ipp_power_off( void );
#endif /* __ASSEMBLER__ */
#endif                                          /* _ASM_ARCH_UNIPHIER2M_PM_H_ */
/******************************************************************************/
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
