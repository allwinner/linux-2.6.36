/******************************************************************************/
/*          :                                     */
/*        :  uniphier_addr.h                                        */
/*        :  2005.03.28                                             */
/*            :                                                 */
/*          :                                                     */
/*                                                                            */
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
/*****  ****************************************************/
/*  No.                                       */
/*                                                                            */
/*                                05.03.28    */
/* <D1IO070> define   FDMA            05.05.30    */
/* <D1VD401> define   FRAGO           05.06.08  */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _UNIPHIER_ADDR_H_                        /* _UNIPHIER_ADDR_H_         */
#define _UNIPHIER_ADDR_H_

#include <asm/arch/hardware.h>                   /* IO_BASE                   */

#define PUNIPHIER_BASE      IO_BASE              /*                           */

/*============================================================================*/
/* Uniphier Block   0xD0000000                                              */
/*============================================================================*/

/*------- IPP            0xD0000000 ----------------------------------------*/
/*------- IPPTIMER       0xD0800000 ----------------------------------------*/
#define IPPTIMER_BASE       ( PUNIPHIER_BASE + 0x00800000 ) /* IPPTIMER       */

#define PUNIPHIER_CPUINT    ( IPPTIMER_BASE + 0x00001000 )  /* CPUINT         */
#define PUNIPHIER_CPUINTSET ( IPPTIMER_BASE + 0x00001004 )  /* CPUINTSET      */
#define PUNIPHIER_CPUINTCLR ( IPPTIMER_BASE + 0x00001008 )  /* CPUINTCLR      */
#define PUNIPHIER_IPPINT    ( IPPTIMER_BASE + 0x00001010 )  /* IPPINT         */
#define PUNIPHIER_IPPINTSET ( IPPTIMER_BASE + 0x00001014 )  /* IPPINTSET      */
#define PUNIPHIER_IPPINTCLR ( IPPTIMER_BASE + 0x00001018 )  /* IPPINTCLR      */
#define PUNIPHIER_TM0MD     ( IPPTIMER_BASE + 0x00002000 )  /* TM0MD          */
#define PUNIPHIER_TM1MD     ( IPPTIMER_BASE + 0x00002001 )  /* TM1MD          */
#define PUNIPHIER_TM2MD     ( IPPTIMER_BASE + 0x00002002 )  /* TM2MD          */
#define PUNIPHIER_TM3MD     ( IPPTIMER_BASE + 0x00002003 )  /* TM3MD          */
#define PUNIPHIER_TM4MD     ( IPPTIMER_BASE + 0x00002004 )  /* TM4MD          */
#define PUNIPHIER_TM5MD     ( IPPTIMER_BASE + 0x00002005 )  /* TM5MD          */
#define PUNIPHIER_TM6MD     ( IPPTIMER_BASE + 0x00002006 )  /* TM6MD          */
#define PUNIPHIER_TM7MD     ( IPPTIMER_BASE + 0x00002007 )  /* TM7MD          */
#define PUNIPHIER_TM0BR     ( IPPTIMER_BASE + 0x00002010 )  /* TM0BR          */
#define PUNIPHIER_TM1BR     ( IPPTIMER_BASE + 0x00002011 )  /* TM1BR          */
#define PUNIPHIER_TM2BR     ( IPPTIMER_BASE + 0x00002012 )  /* TM2BR          */
#define PUNIPHIER_TM3BR     ( IPPTIMER_BASE + 0x00002013 )  /* TM3BR          */
#define PUNIPHIER_TM4BR     ( IPPTIMER_BASE + 0x00002014 )  /* TM4BR          */
#define PUNIPHIER_TM5BR     ( IPPTIMER_BASE + 0x00002015 )  /* TM5BR          */
#define PUNIPHIER_TM6BR     ( IPPTIMER_BASE + 0x00002016 )  /* TM6BR          */
#define PUNIPHIER_TM7BR     ( IPPTIMER_BASE + 0x00002017 )  /* TM7BR          */
#define PUNIPHIER_TM0BC     ( IPPTIMER_BASE + 0x00002020 )  /* TM0BC          */
#define PUNIPHIER_TM1BC     ( IPPTIMER_BASE + 0x00002021 )  /* TM1BC          */
#define PUNIPHIER_TM2BC     ( IPPTIMER_BASE + 0x00002022 )  /* TM2BC          */
#define PUNIPHIER_TM3BC     ( IPPTIMER_BASE + 0x00002023 )  /* TM3BC          */
#define PUNIPHIER_TM4BC     ( IPPTIMER_BASE + 0x00002024 )  /* TM4BC          */
#define PUNIPHIER_TM5BC     ( IPPTIMER_BASE + 0x00002025 )  /* TM5BC          */
#define PUNIPHIER_TM6BC     ( IPPTIMER_BASE + 0x00002026 )  /* TM6BC          */
#define PUNIPHIER_TM7BC     ( IPPTIMER_BASE + 0x00002027 )  /* TM7BC          */
#define PUNIPHIER_TMPSCNTA  ( IPPTIMER_BASE + 0x00002070 )  /* TMPSCNTA       */
#define PUNIPHIER_IPPTMREGA ( IPPTIMER_BASE + 0x00002080 )  /* IPPTMREGA      */
#define PUNIPHIER_TM8MD     ( IPPTIMER_BASE + 0x00003000 )  /* TM8MD          */
#define PUNIPHIER_TM9MD     ( IPPTIMER_BASE + 0x00003001 )  /* TM9MD          */
#define PUNIPHIER_TM10MD    ( IPPTIMER_BASE + 0x00003002 )  /* TM10MD         */
#define PUNIPHIER_TM11MD    ( IPPTIMER_BASE + 0x00003003 )  /* TM11MD         */
#define PUNIPHIER_TM12MD    ( IPPTIMER_BASE + 0x00003004 )  /* TM12MD         */
#define PUNIPHIER_TM13MD    ( IPPTIMER_BASE + 0x00003005 )  /* TM13MD         */
#define PUNIPHIER_TM14MD    ( IPPTIMER_BASE + 0x00003006 )  /* TM14MD         */
#define PUNIPHIER_TM15MD    ( IPPTIMER_BASE + 0x00003007 )  /* TM15MD         */
#define PUNIPHIER_TM8BR     ( IPPTIMER_BASE + 0x00003010 )  /* TM8BR          */
#define PUNIPHIER_TM9BR     ( IPPTIMER_BASE + 0x00003011 )  /* TM9BR          */
#define PUNIPHIER_TM10BR    ( IPPTIMER_BASE + 0x00003012 )  /* TM10BR         */
#define PUNIPHIER_TM11BR    ( IPPTIMER_BASE + 0x00003013 )  /* TM11BR         */
#define PUNIPHIER_TM12BR    ( IPPTIMER_BASE + 0x00003014 )  /* TM12BR         */
#define PUNIPHIER_TM13BR    ( IPPTIMER_BASE + 0x00003015 )  /* TM13BR         */
#define PUNIPHIER_TM14BR    ( IPPTIMER_BASE + 0x00003016 )  /* TM14BR         */
#define PUNIPHIER_TM15BR    ( IPPTIMER_BASE + 0x00003017 )  /* TM15BR         */
#define PUNIPHIER_TM8BC     ( IPPTIMER_BASE + 0x00003020 )  /* TM8BC          */
#define PUNIPHIER_TM9BC     ( IPPTIMER_BASE + 0x00003021 )  /* TM9BC          */
#define PUNIPHIER_TM10BC    ( IPPTIMER_BASE + 0x00003022 )  /* TM10BC         */
#define PUNIPHIER_TM11BC    ( IPPTIMER_BASE + 0x00003023 )  /* TM11BC         */
#define PUNIPHIER_TM12BC    ( IPPTIMER_BASE + 0x00003024 )  /* TM12BC         */
#define PUNIPHIER_TM13BC    ( IPPTIMER_BASE + 0x00003025 )  /* TM13BC         */
#define PUNIPHIER_TM14BC    ( IPPTIMER_BASE + 0x00003026 )  /* TM14BC         */
#define PUNIPHIER_TM15BC    ( IPPTIMER_BASE + 0x00003027 )  /* TM15BC         */
#define PUNIPHIER_TMPSCNTB  ( IPPTIMER_BASE + 0x00003070 )  /* TMPSCNTB       */
#define PUNIPHIER_IPPTMREGB ( IPPTIMER_BASE + 0x00003080 )  /* IPPTMREGB      */

/*------- IPPINTC        0xD0810000 ----------------------------------------*/
#define IPPINTC_BASE        ( PUNIPHIER_BASE + 0x00810000 ) /* IPPINTC        */

#define PUNIPHIER_G0ICR     ( IPPINTC_BASE + 0x00000000 )   /* G0ICR          */
#define PUNIPHIER_G1ICR     ( IPPINTC_BASE + 0x00000004 )   /* G1ICR          */
#define PUNIPHIER_G2ICR     ( IPPINTC_BASE + 0x00000008 )   /* G2ICR          */
#define PUNIPHIER_G3ICR     ( IPPINTC_BASE + 0x0000000C )   /* G3ICR          */
#define PUNIPHIER_G4ICR     ( IPPINTC_BASE + 0x00000010 )   /* G4ICR          */
#define PUNIPHIER_G5ICR     ( IPPINTC_BASE + 0x00000014 )   /* G5ICR          */
#define PUNIPHIER_G6ICR     ( IPPINTC_BASE + 0x00000018 )   /* G6ICR          */
#define PUNIPHIER_G7ICR     ( IPPINTC_BASE + 0x0000001C )   /* G7ICR          */
#define PUNIPHIER_G9ICR     ( IPPINTC_BASE + 0x00000024 )   /* G9ICR          */
#define PUNIPHIER_G10ICR    ( IPPINTC_BASE + 0x00000028 )   /* G10ICR         */
#define PUNIPHIER_G11ICR    ( IPPINTC_BASE + 0x0000002C )   /* G11ICR         */
#define PUNIPHIER_G13ICR    ( IPPINTC_BASE + 0x00000034 )   /* G13ICR         */
#define PUNIPHIER_G14ICR    ( IPPINTC_BASE + 0x00000038 )   /* G14ICR         */
#define PUNIPHIER_G15ICR    ( IPPINTC_BASE + 0x0000003C )   /* G15ICR         */
#define PUNIPHIER_G16ICR    ( IPPINTC_BASE + 0x00000040 )   /* G16ICR         */
#define PUNIPHIER_G17ICR    ( IPPINTC_BASE + 0x00000044 )   /* G17ICR         */
#define PUNIPHIER_G18ICR    ( IPPINTC_BASE + 0x00000048 )   /* G18ICR         */
#define PUNIPHIER_G19ICR    ( IPPINTC_BASE + 0x0000004C )   /* G19ICR         */
#define PUNIPHIER_G20ICR    ( IPPINTC_BASE + 0x00000050 )   /* G20ICR         */
#define PUNIPHIER_G21ICR    ( IPPINTC_BASE + 0x00000054 )   /* G21ICR         */
#define PUNIPHIER_G22ICR    ( IPPINTC_BASE + 0x00000058 )   /* G22ICR         */
#define PUNIPHIER_G24ICR    ( IPPINTC_BASE + 0x00000060 )   /* G24ICR         */
#define PUNIPHIER_G25ICR    ( IPPINTC_BASE + 0x00000064 )   /* G25ICR         */
#define PUNIPHIER_G26ICR    ( IPPINTC_BASE + 0x00000068 )   /* G26ICR         */
#define PUNIPHIER_G27ICR    ( IPPINTC_BASE + 0x0000006C )   /* G27ICR         */
#define PUNIPHIER_G28ICR    ( IPPINTC_BASE + 0x00000070 )   /* G28ICR         */
#define PUNIPHIER_G29ICR    ( IPPINTC_BASE + 0x00000074 )   /* G29ICR         */
#define PUNIPHIER_G30ICR    ( IPPINTC_BASE + 0x00000078 )   /* G30ICR         */
#define PUNIPHIER_G31ICR    ( IPPINTC_BASE + 0x0000007C )   /* G31ICR         */
#define PUNIPHIER_G32ICR    ( IPPINTC_BASE + 0x00000080 )   /* G32ICR         */
#define PUNIPHIER_G33ICR    ( IPPINTC_BASE + 0x00000084 )   /* G33ICR         */
#define PUNIPHIER_G35ICR    ( IPPINTC_BASE + 0x0000008C )   /* G35ICR         */
#define PUNIPHIER_G36ICR    ( IPPINTC_BASE + 0x00000090 )   /* G36ICR         */
#define PUNIPHIER_G37ICR    ( IPPINTC_BASE + 0x00000094 )   /* G37ICR         */
#define PUNIPHIER_IAGR      ( IPPINTC_BASE + 0x00000100 )   /* IAGR           */

/*------- RSTCNT         0xD0820000 ----------------------------------------*/
#define RSTCNT_BASE         ( PUNIPHIER_BASE + 0x00820000 ) /* RSTCNT         */

#define PUNIPHIER_AMMPRSTC  ( IPPBCU_BASE + 0x00004000 )    /* AMMPRSTC       */

/*------- IXM            0xD1000000 ----------------------------------------*/
#define IXM_BASE            ( PUNIPHIER_BASE + 0x01000000 ) /* IXM            */

#define PUNIPHIER_DMCCCTR   ( IXM_BASE + 0x00000000 )       /* DMCCCTR        */
#define PUNIPHIER_DMCRCTR   ( IXM_BASE + 0x00000004 )       /* DMCRCTR        */
#define PUNIPHIER_DMCMDRR   ( IXM_BASE + 0x00000010 )       /* DMCMDRR        */
#define PUNIPHIER_DMCDIER   ( IXM_BASE + 0x00000100 )       /* DMCDIER        */
#define PUNIPHIER_DMCBIER   ( IXM_BASE + 0x00000108 )       /* DMCBIER        */
#define PUNIPHIER_DMCIIER   ( IXM_BASE + 0x00000110 )       /* DMCIIER        */
#define PUNIPHIER_DMCMIER   ( IXM_BASE + 0x00000118 )       /* DMCMIER        */
#define PUNIPHIER_DMCDISR   ( IXM_BASE + 0x00000200 )       /* DMCDISR        */
#define PUNIPHIER_DMCBISR   ( IXM_BASE + 0x00000208 )       /* DMCBISR        */
#define PUNIPHIER_DMCIISR   ( IXM_BASE + 0x00000210 )       /* DMCIISR        */
#define PUNIPHIER_DMCMISR   ( IXM_BASE + 0x00000218 )       /* DMCMISR        */
#define PUNIPHIER_DMCLDSR   ( IXM_BASE + 0x00000300 )       /* DMCLDSR        */
#define PUNIPHIER_DMCPDSR   ( IXM_BASE + 0x00000308 )       /* DMCPDSR        */
#define PUNIPHIER_DMCMCBR   ( IXM_BASE + 0x00000400 )       /* DMCMCBR        */
#define PUNIPHIER_DMCIOBR   ( IXM_BASE + 0x00000500 )       /* DMCIOBR        */
#define PUNIPHIER_DMCABSR   ( IXM_BASE + 0x00000600 )       /* DMCABSR        */
#define PUNIPHIER_DMCBSSR   ( IXM_BASE + 0x00000700 )       /* DMCBSSR        */
#define PUNIPHIER_DMC00CTR  ( IXM_BASE + 0x00001000 )       /* DMC00CTR       */
#define PUNIPHIER_DMC00SCR  ( IXM_BASE + 0x00001004 )       /* DMC00SCR       */
#define PUNIPHIER_DMC00DCR  ( IXM_BASE + 0x00001008 )       /* DMC00DCR       */
#define PUNIPHIER_DMC00WCR  ( IXM_BASE + 0x0000100c )       /* DMC00WCR       */
#define PUNIPHIER_DMC00BCR  ( IXM_BASE + 0x00001010 )       /* DMC00BCR       */
#define PUNIPHIER_DMC00LCR  ( IXM_BASE + 0x00001014 )       /* DMC00LCR       */
#define PUNIPHIER_DMC01CTR  ( IXM_BASE + 0x00001020 )       /* DMC01CTR       */
#define PUNIPHIER_DMC01SCR  ( IXM_BASE + 0x00001024 )       /* DMC01SCR       */
#define PUNIPHIER_DMC01DCR  ( IXM_BASE + 0x00001028 )       /* DMC01DCR       */
#define PUNIPHIER_DMC01WCR  ( IXM_BASE + 0x0000102c )       /* DMC01WCR       */
#define PUNIPHIER_DMC01BCR  ( IXM_BASE + 0x00001030 )       /* DMC01BCR       */
#define PUNIPHIER_DMC01LCR  ( IXM_BASE + 0x00001034 )       /* DMC01LCR       */
#define PUNIPHIER_DMC02CTR  ( IXM_BASE + 0x00001040 )       /* DMC02CTR       */
#define PUNIPHIER_DMC02SCR  ( IXM_BASE + 0x00001044 )       /* DMC02SCR       */
#define PUNIPHIER_DMC02DCR  ( IXM_BASE + 0x00001048 )       /* DMC02DCR       */
#define PUNIPHIER_DMC02WCR  ( IXM_BASE + 0x0000104c )       /* DMC02WCR       */
#define PUNIPHIER_DMC02BCR  ( IXM_BASE + 0x00001050 )       /* DMC02BCR       */
#define PUNIPHIER_DMC02LCR  ( IXM_BASE + 0x00001054 )       /* DMC02LCR       */
#define PUNIPHIER_DMC03CTR  ( IXM_BASE + 0x00001060 )       /* DMC03CTR       */
#define PUNIPHIER_DMC03SCR  ( IXM_BASE + 0x00001064 )       /* DMC03SCR       */
#define PUNIPHIER_DMC03DCR  ( IXM_BASE + 0x00001068 )       /* DMC03DCR       */
#define PUNIPHIER_DMC03WCR  ( IXM_BASE + 0x0000106c )       /* DMC03WCR       */
#define PUNIPHIER_DMC03BCR  ( IXM_BASE + 0x00001070 )       /* DMC03BCR       */
#define PUNIPHIER_DMC03LCR  ( IXM_BASE + 0x00001074 )       /* DMC03LCR       */
#define PUNIPHIER_DMC04CTR  ( IXM_BASE + 0x00001080 )       /* DMC04CTR       */
#define PUNIPHIER_DMC04SCR  ( IXM_BASE + 0x00001084 )       /* DMC04SCR       */
#define PUNIPHIER_DMC04DCR  ( IXM_BASE + 0x00001088 )       /* DMC04DCR       */
#define PUNIPHIER_DMC04WCR  ( IXM_BASE + 0x0000108c )       /* DMC04WCR       */
#define PUNIPHIER_DMC04BCR  ( IXM_BASE + 0x00001090 )       /* DMC04BCR       */
#define PUNIPHIER_DMC04LCR  ( IXM_BASE + 0x00001094 )       /* DMC04LCR       */
#define PUNIPHIER_DMC05CTR  ( IXM_BASE + 0x000010a0 )       /* DMC05CTR       */
#define PUNIPHIER_DMC05SCR  ( IXM_BASE + 0x000010a4 )       /* DMC05SCR       */
#define PUNIPHIER_DMC05DCR  ( IXM_BASE + 0x000010a8 )       /* DMC05DCR       */
#define PUNIPHIER_DMC05WCR  ( IXM_BASE + 0x000010ac )       /* DMC05WCR       */
#define PUNIPHIER_DMC05BCR  ( IXM_BASE + 0x000010b0 )       /* DMC05BCR       */
#define PUNIPHIER_DMC05LCR  ( IXM_BASE + 0x000010b4 )       /* DMC05LCR       */
#define PUNIPHIER_DMC06CTR  ( IXM_BASE + 0x000010c0 )       /* DMC06CTR       */
#define PUNIPHIER_DMC06SCR  ( IXM_BASE + 0x000010c4 )       /* DMC06SCR       */
#define PUNIPHIER_DMC06DCR  ( IXM_BASE + 0x000010c8 )       /* DMC06DCR       */
#define PUNIPHIER_DMC06WCR  ( IXM_BASE + 0x000010cc )       /* DMC06WCR       */
#define PUNIPHIER_DMC06BCR  ( IXM_BASE + 0x000010d0 )       /* DMC06BCR       */
#define PUNIPHIER_DMC06LCR  ( IXM_BASE + 0x000010d4 )       /* DMC06LCR       */
#define PUNIPHIER_DMC07CTR  ( IXM_BASE + 0x000010e0 )       /* DMC07CTR       */
#define PUNIPHIER_DMC07SCR  ( IXM_BASE + 0x000010e4 )       /* DMC07SCR       */
#define PUNIPHIER_DMC07DCR  ( IXM_BASE + 0x000010e8 )       /* DMC07DCR       */
#define PUNIPHIER_DMC07WCR  ( IXM_BASE + 0x000010ec )       /* DMC07WCR       */
#define PUNIPHIER_DMC07BCR  ( IXM_BASE + 0x000010f0 )       /* DMC07BCR       */
#define PUNIPHIER_DMC07LCR  ( IXM_BASE + 0x000010f4 )       /* DMC07LCR       */
#define PUNIPHIER_DMC08CTR  ( IXM_BASE + 0x00001100 )       /* DMC08CTR       */
#define PUNIPHIER_DMC08SCR  ( IXM_BASE + 0x00001104 )       /* DMC08SCR       */
#define PUNIPHIER_DMC08DCR  ( IXM_BASE + 0x00001108 )       /* DMC08DCR       */
#define PUNIPHIER_DMC08WCR  ( IXM_BASE + 0x0000110c )       /* DMC08WCR       */
#define PUNIPHIER_DMC08BCR  ( IXM_BASE + 0x00001110 )       /* DMC08BCR       */
#define PUNIPHIER_DMC08LCR  ( IXM_BASE + 0x00001114 )       /* DMC08LCR       */
#define PUNIPHIER_DMC09CTR  ( IXM_BASE + 0x00001120 )       /* DMC09CTR       */
#define PUNIPHIER_DMC09SCR  ( IXM_BASE + 0x00001124 )       /* DMC09SCR       */
#define PUNIPHIER_DMC09DCR  ( IXM_BASE + 0x00001128 )       /* DMC09DCR       */
#define PUNIPHIER_DMC09WCR  ( IXM_BASE + 0x0000112c )       /* DMC09WCR       */
#define PUNIPHIER_DMC09BCR  ( IXM_BASE + 0x00001130 )       /* DMC09BCR       */
#define PUNIPHIER_DMC09LCR  ( IXM_BASE + 0x00001134 )       /* DMC09LCR       */
#define PUNIPHIER_DMC10CTR  ( IXM_BASE + 0x00001140 )       /* DMC10CTR       */
#define PUNIPHIER_DMC10SCR  ( IXM_BASE + 0x00001144 )       /* DMC10SCR       */
#define PUNIPHIER_DMC10DCR  ( IXM_BASE + 0x00001148 )       /* DMC10DCR       */
#define PUNIPHIER_DMC10WCR  ( IXM_BASE + 0x0000114c )       /* DMC10WCR       */
#define PUNIPHIER_DMC10BCR  ( IXM_BASE + 0x00001150 )       /* DMC10BCR       */
#define PUNIPHIER_DMC10LCR  ( IXM_BASE + 0x00001154 )       /* DMC10LCR       */
#define PUNIPHIER_DMC11CTR  ( IXM_BASE + 0x00001160 )       /* DMC11CTR       */
#define PUNIPHIER_DMC11SCR  ( IXM_BASE + 0x00001164 )       /* DMC11SCR       */
#define PUNIPHIER_DMC11DCR  ( IXM_BASE + 0x00001168 )       /* DMC11DCR       */
#define PUNIPHIER_DMC11WCR  ( IXM_BASE + 0x0000116c )       /* DMC11WCR       */
#define PUNIPHIER_DMC11BCR  ( IXM_BASE + 0x00001170 )       /* DMC11BCR       */
#define PUNIPHIER_DMC11LCR  ( IXM_BASE + 0x00001174 )       /* DMC11LCR       */
#define PUNIPHIER_DMC12CTR  ( IXM_BASE + 0x00001180 )       /* DMC12CTR       */
#define PUNIPHIER_DMC12SCR  ( IXM_BASE + 0x00001184 )       /* DMC12SCR       */
#define PUNIPHIER_DMC12DCR  ( IXM_BASE + 0x00001188 )       /* DMC12DCR       */
#define PUNIPHIER_DMC12WCR  ( IXM_BASE + 0x0000118c )       /* DMC12WCR       */
#define PUNIPHIER_DMC12BCR  ( IXM_BASE + 0x00001190 )       /* DMC12BCR       */
#define PUNIPHIER_DMC12LCR  ( IXM_BASE + 0x00001194 )       /* DMC12LCR       */
#define PUNIPHIER_DMC13CTR  ( IXM_BASE + 0x000011a0 )       /* DMC13CTR       */
#define PUNIPHIER_DMC13SCR  ( IXM_BASE + 0x000011a4 )       /* DMC13SCR       */
#define PUNIPHIER_DMC13DCR  ( IXM_BASE + 0x000011a8 )       /* DMC13DCR       */
#define PUNIPHIER_DMC13WCR  ( IXM_BASE + 0x000011ac )       /* DMC13WCR       */
#define PUNIPHIER_DMC13BCR  ( IXM_BASE + 0x000011b0 )       /* DMC13BCR       */
#define PUNIPHIER_DMC13LCR  ( IXM_BASE + 0x000011b4 )       /* DMC13LCR       */
#define PUNIPHIER_DMC14CTR  ( IXM_BASE + 0x000011c0 )       /* DMC14CTR       */
#define PUNIPHIER_DMC14SCR  ( IXM_BASE + 0x000011c4 )       /* DMC14SCR       */
#define PUNIPHIER_DMC14DCR  ( IXM_BASE + 0x000011c8 )       /* DMC14DCR       */
#define PUNIPHIER_DMC14WCR  ( IXM_BASE + 0x000011cc )       /* DMC14WCR       */
#define PUNIPHIER_DMC14BCR  ( IXM_BASE + 0x000011d0 )       /* DMC14BCR       */
#define PUNIPHIER_DMC14LCR  ( IXM_BASE + 0x000011d4 )       /* DMC14LCR       */
#define PUNIPHIER_DMC15CTR  ( IXM_BASE + 0x000011e0 )       /* DMC15CTR       */
#define PUNIPHIER_DMC15SCR  ( IXM_BASE + 0x000011e4 )       /* DMC15SCR       */
#define PUNIPHIER_DMC15DCR  ( IXM_BASE + 0x000011e8 )       /* DMC15DCR       */
#define PUNIPHIER_DMC15WCR  ( IXM_BASE + 0x000011ec )       /* DMC15WCR       */
#define PUNIPHIER_DMC15BCR  ( IXM_BASE + 0x000011f0 )       /* DMC15BCR       */
#define PUNIPHIER_DMC15LCR  ( IXM_BASE + 0x000011f4 )       /* DMC15LCR       */
#define PUNIPHIER_DMC00SSA  ( IXM_BASE + 0x00001800 )       /* DMC00SSA       */
#define PUNIPHIER_DMC00SEA  ( IXM_BASE + 0x00001804 )       /* DMC00SEA       */
#define PUNIPHIER_DMC00SBA  ( IXM_BASE + 0x00001808 )       /* DMC00SBA       */
#define PUNIPHIER_DMC00SCA  ( IXM_BASE + 0x0000180c )       /* DMC00SCA       */
#define PUNIPHIER_DMC00DSA  ( IXM_BASE + 0x00001810 )       /* DMC00DSA       */
#define PUNIPHIER_DMC00DEA  ( IXM_BASE + 0x00001814 )       /* DMC00DEA       */
#define PUNIPHIER_DMC00DBA  ( IXM_BASE + 0x00001818 )       /* DMC00DBA       */
#define PUNIPHIER_DMC00DCA  ( IXM_BASE + 0x0000181c )       /* DMC00DCA       */
#define PUNIPHIER_DMC00SS0  ( IXM_BASE + 0x00001820 )       /* DMC00SS0       */
#define PUNIPHIER_DMC00DS0  ( IXM_BASE + 0x00001824 )       /* DMC00DS0       */
#define PUNIPHIER_DMC00WD0  ( IXM_BASE + 0x00001828 )       /* DMC00WD0       */
#define PUNIPHIER_DMC00CP0  ( IXM_BASE + 0x0000182c )       /* DMC00CP0       */
#define PUNIPHIER_DMC00SS1  ( IXM_BASE + 0x00001830 )       /* DMC00SS1       */
#define PUNIPHIER_DMC00DS1  ( IXM_BASE + 0x00001834 )       /* DMC00DS1       */
#define PUNIPHIER_DMC00WD1  ( IXM_BASE + 0x00001838 )       /* DMC00WD1       */
#define PUNIPHIER_DMC00CP1  ( IXM_BASE + 0x0000183c )       /* DMC00CP1       */
#define PUNIPHIER_DMC00SS2  ( IXM_BASE + 0x00001840 )       /* DMC00SS2       */
#define PUNIPHIER_DMC00DS2  ( IXM_BASE + 0x00001844 )       /* DMC00DS2       */
#define PUNIPHIER_DMC00WD2  ( IXM_BASE + 0x00001848 )       /* DMC00WD2       */
#define PUNIPHIER_DMC00CP2  ( IXM_BASE + 0x0000184c )       /* DMC00CP2       */
#define PUNIPHIER_DMC00SS3  ( IXM_BASE + 0x00001850 )       /* DMC00SS3       */
#define PUNIPHIER_DMC00DS3  ( IXM_BASE + 0x00001854 )       /* DMC00DS3       */
#define PUNIPHIER_DMC00WD3  ( IXM_BASE + 0x00001858 )       /* DMC00WD3       */
#define PUNIPHIER_DMC00CP3  ( IXM_BASE + 0x0000185c )       /* DMC00CP3       */
#define PUNIPHIER_DMC01SSA  ( IXM_BASE + 0x00001880 )       /* DMC01SSA       */
#define PUNIPHIER_DMC01SEA  ( IXM_BASE + 0x00001884 )       /* DMC01SEA       */
#define PUNIPHIER_DMC01SBA  ( IXM_BASE + 0x00001888 )       /* DMC01SBA       */
#define PUNIPHIER_DMC01SCA  ( IXM_BASE + 0x0000188c )       /* DMC01SCA       */
#define PUNIPHIER_DMC01DSA  ( IXM_BASE + 0x00001890 )       /* DMC01DSA       */
#define PUNIPHIER_DMC01DEA  ( IXM_BASE + 0x00001894 )       /* DMC01DEA       */
#define PUNIPHIER_DMC01DBA  ( IXM_BASE + 0x00001898 )       /* DMC01DBA       */
#define PUNIPHIER_DMC01DCA  ( IXM_BASE + 0x0000189c )       /* DMC01DCA       */
#define PUNIPHIER_DMC01SS0  ( IXM_BASE + 0x000018a0 )       /* DMC01SS0       */
#define PUNIPHIER_DMC01DS0  ( IXM_BASE + 0x000018a4 )       /* DMC01DS0       */
#define PUNIPHIER_DMC01WD0  ( IXM_BASE + 0x000018a8 )       /* DMC01WD0       */
#define PUNIPHIER_DMC01CP0  ( IXM_BASE + 0x000018ac )       /* DMC01CP0       */
#define PUNIPHIER_DMC01SS1  ( IXM_BASE + 0x000018b0 )       /* DMC01SS1       */
#define PUNIPHIER_DMC01DS1  ( IXM_BASE + 0x000018b4 )       /* DMC01DS1       */
#define PUNIPHIER_DMC01WD1  ( IXM_BASE + 0x000018b8 )       /* DMC01WD1       */
#define PUNIPHIER_DMC01CP1  ( IXM_BASE + 0x000018bc )       /* DMC01CP1       */
#define PUNIPHIER_DMC01SS2  ( IXM_BASE + 0x000018c0 )       /* DMC01SS2       */
#define PUNIPHIER_DMC01DS2  ( IXM_BASE + 0x000018c4 )       /* DMC01DS2       */
#define PUNIPHIER_DMC01WD2  ( IXM_BASE + 0x000018c8 )       /* DMC01WD2       */
#define PUNIPHIER_DMC01CP2  ( IXM_BASE + 0x000018cc )       /* DMC01CP2       */
#define PUNIPHIER_DMC01SS3  ( IXM_BASE + 0x000018d0 )       /* DMC01SS3       */
#define PUNIPHIER_DMC01DS3  ( IXM_BASE + 0x000018d4 )       /* DMC01DS3       */
#define PUNIPHIER_DMC01WD3  ( IXM_BASE + 0x000018d8 )       /* DMC01WD3       */
#define PUNIPHIER_DMC01CP3  ( IXM_BASE + 0x000018dc )       /* DMC01CP3       */
#define PUNIPHIER_DMC02SSA  ( IXM_BASE + 0x00001900 )       /* DMC02SSA       */
#define PUNIPHIER_DMC02SEA  ( IXM_BASE + 0x00001904 )       /* DMC02SEA       */
#define PUNIPHIER_DMC02SBA  ( IXM_BASE + 0x00001908 )       /* DMC02SBA       */
#define PUNIPHIER_DMC02SCA  ( IXM_BASE + 0x0000190c )       /* DMC02SCA       */
#define PUNIPHIER_DMC02DSA  ( IXM_BASE + 0x00001910 )       /* DMC02DSA       */
#define PUNIPHIER_DMC02DEA  ( IXM_BASE + 0x00001914 )       /* DMC02DEA       */
#define PUNIPHIER_DMC02DBA  ( IXM_BASE + 0x00001918 )       /* DMC02DBA       */
#define PUNIPHIER_DMC02DCA  ( IXM_BASE + 0x0000191c )       /* DMC02DCA       */
#define PUNIPHIER_DMC02SS0  ( IXM_BASE + 0x00001920 )       /* DMC02SS0       */
#define PUNIPHIER_DMC02DS0  ( IXM_BASE + 0x00001924 )       /* DMC02DS0       */
#define PUNIPHIER_DMC02WD0  ( IXM_BASE + 0x00001928 )       /* DMC02WD0       */
#define PUNIPHIER_DMC02CP0  ( IXM_BASE + 0x0000192c )       /* DMC02CP0       */
#define PUNIPHIER_DMC02SS1  ( IXM_BASE + 0x00001930 )       /* DMC02SS1       */
#define PUNIPHIER_DMC02DS1  ( IXM_BASE + 0x00001934 )       /* DMC02DS1       */
#define PUNIPHIER_DMC02WD1  ( IXM_BASE + 0x00001938 )       /* DMC02WD1       */
#define PUNIPHIER_DMC02CP1  ( IXM_BASE + 0x0000193c )       /* DMC02CP1       */
#define PUNIPHIER_DMC02SS2  ( IXM_BASE + 0x00001940 )       /* DMC02SS2       */
#define PUNIPHIER_DMC02DS2  ( IXM_BASE + 0x00001944 )       /* DMC02DS2       */
#define PUNIPHIER_DMC02WD2  ( IXM_BASE + 0x00001948 )       /* DMC02WD2       */
#define PUNIPHIER_DMC02CP2  ( IXM_BASE + 0x0000194c )       /* DMC02CP2       */
#define PUNIPHIER_DMC02SS3  ( IXM_BASE + 0x00001950 )       /* DMC02SS3       */
#define PUNIPHIER_DMC02DS3  ( IXM_BASE + 0x00001954 )       /* DMC02DS3       */
#define PUNIPHIER_DMC02WD3  ( IXM_BASE + 0x00001958 )       /* DMC02WD3       */
#define PUNIPHIER_DMC02CP3  ( IXM_BASE + 0x0000195c )       /* DMC02CP3       */
#define PUNIPHIER_DMC03SSA  ( IXM_BASE + 0x00001980 )       /* DMC03SSA       */
#define PUNIPHIER_DMC03SEA  ( IXM_BASE + 0x00001984 )       /* DMC03SEA       */
#define PUNIPHIER_DMC03SBA  ( IXM_BASE + 0x00001988 )       /* DMC03SBA       */
#define PUNIPHIER_DMC03SCA  ( IXM_BASE + 0x0000198c )       /* DMC03SCA       */
#define PUNIPHIER_DMC03DSA  ( IXM_BASE + 0x00001990 )       /* DMC03DSA       */
#define PUNIPHIER_DMC03DEA  ( IXM_BASE + 0x00001994 )       /* DMC03DEA       */
#define PUNIPHIER_DMC03DBA  ( IXM_BASE + 0x00001998 )       /* DMC03DBA       */
#define PUNIPHIER_DMC03DCA  ( IXM_BASE + 0x0000199c )       /* DMC03DCA       */
#define PUNIPHIER_DMC03SS0  ( IXM_BASE + 0x000019a0 )       /* DMC03SS0       */
#define PUNIPHIER_DMC03DS0  ( IXM_BASE + 0x000019a4 )       /* DMC03DS0       */
#define PUNIPHIER_DMC03WD0  ( IXM_BASE + 0x000019a8 )       /* DMC03WD0       */
#define PUNIPHIER_DMC03CP0  ( IXM_BASE + 0x000019ac )       /* DMC03CP0       */
#define PUNIPHIER_DMC03SS1  ( IXM_BASE + 0x000019b0 )       /* DMC03SS1       */
#define PUNIPHIER_DMC03DS1  ( IXM_BASE + 0x000019b4 )       /* DMC03DS1       */
#define PUNIPHIER_DMC03WD1  ( IXM_BASE + 0x000019b8 )       /* DMC03WD1       */
#define PUNIPHIER_DMC03CP1  ( IXM_BASE + 0x000019bc )       /* DMC03CP1       */
#define PUNIPHIER_DMC03SS2  ( IXM_BASE + 0x000019c0 )       /* DMC03SS2       */
#define PUNIPHIER_DMC03DS2  ( IXM_BASE + 0x000019c4 )       /* DMC03DS2       */
#define PUNIPHIER_DMC03WD2  ( IXM_BASE + 0x000019c8 )       /* DMC03WD2       */
#define PUNIPHIER_DMC03CP2  ( IXM_BASE + 0x000019cc )       /* DMC03CP2       */
#define PUNIPHIER_DMC03SS3  ( IXM_BASE + 0x000019d0 )       /* DMC03SS3       */
#define PUNIPHIER_DMC03DS3  ( IXM_BASE + 0x000019d4 )       /* DMC03DS3       */
#define PUNIPHIER_DMC03WD3  ( IXM_BASE + 0x000019d8 )       /* DMC03WD3       */
#define PUNIPHIER_DMC03CP3  ( IXM_BASE + 0x000019dc )       /* DMC03CP3       */
#define PUNIPHIER_DMC04SSA  ( IXM_BASE + 0x00001a00 )       /* DMC04SSA       */
#define PUNIPHIER_DMC04SEA  ( IXM_BASE + 0x00001a04 )       /* DMC04SEA       */
#define PUNIPHIER_DMC04SBA  ( IXM_BASE + 0x00001a08 )       /* DMC04SBA       */
#define PUNIPHIER_DMC04SCA  ( IXM_BASE + 0x00001a0c )       /* DMC04SCA       */
#define PUNIPHIER_DMC04DSA  ( IXM_BASE + 0x00001a10 )       /* DMC04DSA       */
#define PUNIPHIER_DMC04DEA  ( IXM_BASE + 0x00001a14 )       /* DMC04DEA       */
#define PUNIPHIER_DMC04DBA  ( IXM_BASE + 0x00001a18 )       /* DMC04DBA       */
#define PUNIPHIER_DMC04DCA  ( IXM_BASE + 0x00001a1c )       /* DMC04DCA       */
#define PUNIPHIER_DMC04SS0  ( IXM_BASE + 0x00001a20 )       /* DMC04SS0       */
#define PUNIPHIER_DMC04DS0  ( IXM_BASE + 0x00001a24 )       /* DMC04DS0       */
#define PUNIPHIER_DMC04WD0  ( IXM_BASE + 0x00001a28 )       /* DMC04WD0       */
#define PUNIPHIER_DMC04CP0  ( IXM_BASE + 0x00001a2c )       /* DMC04CP0       */
#define PUNIPHIER_DMC04SS1  ( IXM_BASE + 0x00001a30 )       /* DMC04SS1       */
#define PUNIPHIER_DMC04DS1  ( IXM_BASE + 0x00001a34 )       /* DMC04DS1       */
#define PUNIPHIER_DMC04WD1  ( IXM_BASE + 0x00001a38 )       /* DMC04WD1       */
#define PUNIPHIER_DMC04CP1  ( IXM_BASE + 0x00001a3c )       /* DMC04CP1       */
#define PUNIPHIER_DMC04SS2  ( IXM_BASE + 0x00001a40 )       /* DMC04SS2       */
#define PUNIPHIER_DMC04DS2  ( IXM_BASE + 0x00001a44 )       /* DMC04DS2       */
#define PUNIPHIER_DMC04WD2  ( IXM_BASE + 0x00001a48 )       /* DMC04WD2       */
#define PUNIPHIER_DMC04CP2  ( IXM_BASE + 0x00001a4c )       /* DMC04CP2       */
#define PUNIPHIER_DMC04SS3  ( IXM_BASE + 0x00001a50 )       /* DMC04SS3       */
#define PUNIPHIER_DMC04DS3  ( IXM_BASE + 0x00001a54 )       /* DMC04DS3       */
#define PUNIPHIER_DMC04WD3  ( IXM_BASE + 0x00001a58 )       /* DMC04WD3       */
#define PUNIPHIER_DMC04CP3  ( IXM_BASE + 0x00001a5c )       /* DMC04CP3       */
#define PUNIPHIER_DMC05SSA  ( IXM_BASE + 0x00001a80 )       /* DMC05SSA       */
#define PUNIPHIER_DMC05SEA  ( IXM_BASE + 0x00001a84 )       /* DMC05SEA       */
#define PUNIPHIER_DMC05SBA  ( IXM_BASE + 0x00001a88 )       /* DMC05SBA       */
#define PUNIPHIER_DMC05SCA  ( IXM_BASE + 0x00001a8c )       /* DMC05SCA       */
#define PUNIPHIER_DMC05DSA  ( IXM_BASE + 0x00001a90 )       /* DMC05DSA       */
#define PUNIPHIER_DMC05DEA  ( IXM_BASE + 0x00001a94 )       /* DMC05DEA       */
#define PUNIPHIER_DMC05DBA  ( IXM_BASE + 0x00001a98 )       /* DMC05DBA       */
#define PUNIPHIER_DMC05DCA  ( IXM_BASE + 0x00001a9c )       /* DMC05DCA       */
#define PUNIPHIER_DMC05SS0  ( IXM_BASE + 0x00001aa0 )       /* DMC05SS0       */
#define PUNIPHIER_DMC05DS0  ( IXM_BASE + 0x00001aa4 )       /* DMC05DS0       */
#define PUNIPHIER_DMC05WD0  ( IXM_BASE + 0x00001aa8 )       /* DMC05WD0       */
#define PUNIPHIER_DMC05CP0  ( IXM_BASE + 0x00001aac )       /* DMC05CP0       */
#define PUNIPHIER_DMC05SS1  ( IXM_BASE + 0x00001ab0 )       /* DMC05SS1       */
#define PUNIPHIER_DMC05DS1  ( IXM_BASE + 0x00001ab4 )       /* DMC05DS1       */
#define PUNIPHIER_DMC05WD1  ( IXM_BASE + 0x00001ab8 )       /* DMC05WD1       */
#define PUNIPHIER_DMC05CP1  ( IXM_BASE + 0x00001abc )       /* DMC05CP1       */
#define PUNIPHIER_DMC05SS2  ( IXM_BASE + 0x00001ac0 )       /* DMC05SS2       */
#define PUNIPHIER_DMC05DS2  ( IXM_BASE + 0x00001ac4 )       /* DMC05DS2       */
#define PUNIPHIER_DMC05WD2  ( IXM_BASE + 0x00001ac8 )       /* DMC05WD2       */
#define PUNIPHIER_DMC05CP2  ( IXM_BASE + 0x00001acc )       /* DMC05CP2       */
#define PUNIPHIER_DMC05SS3  ( IXM_BASE + 0x00001ad0 )       /* DMC05SS3       */
#define PUNIPHIER_DMC05DS3  ( IXM_BASE + 0x00001ad4 )       /* DMC05DS3       */
#define PUNIPHIER_DMC05WD3  ( IXM_BASE + 0x00001ad8 )       /* DMC05WD3       */
#define PUNIPHIER_DMC05CP3  ( IXM_BASE + 0x00001adc )       /* DMC05CP3       */
#define PUNIPHIER_DMC06SSA  ( IXM_BASE + 0x00001b00 )       /* DMC06SSA       */
#define PUNIPHIER_DMC06SEA  ( IXM_BASE + 0x00001b04 )       /* DMC06SEA       */
#define PUNIPHIER_DMC06SBA  ( IXM_BASE + 0x00001b08 )       /* DMC06SBA       */
#define PUNIPHIER_DMC06SCA  ( IXM_BASE + 0x00001b0c )       /* DMC06SCA       */
#define PUNIPHIER_DMC06DSA  ( IXM_BASE + 0x00001b10 )       /* DMC06DSA       */
#define PUNIPHIER_DMC06DEA  ( IXM_BASE + 0x00001b14 )       /* DMC06DEA       */
#define PUNIPHIER_DMC06DBA  ( IXM_BASE + 0x00001b18 )       /* DMC06DBA       */
#define PUNIPHIER_DMC06DCA  ( IXM_BASE + 0x00001b1c )       /* DMC06DCA       */
#define PUNIPHIER_DMC06SS0  ( IXM_BASE + 0x00001b20 )       /* DMC06SS0       */
#define PUNIPHIER_DMC06DS0  ( IXM_BASE + 0x00001b24 )       /* DMC06DS0       */
#define PUNIPHIER_DMC06WD0  ( IXM_BASE + 0x00001b28 )       /* DMC06WD0       */
#define PUNIPHIER_DMC06CP0  ( IXM_BASE + 0x00001b2c )       /* DMC06CP0       */
#define PUNIPHIER_DMC06SS1  ( IXM_BASE + 0x00001b30 )       /* DMC06SS1       */
#define PUNIPHIER_DMC06DS1  ( IXM_BASE + 0x00001b34 )       /* DMC06DS1       */
#define PUNIPHIER_DMC06WD1  ( IXM_BASE + 0x00001b38 )       /* DMC06WD1       */
#define PUNIPHIER_DMC06CP1  ( IXM_BASE + 0x00001b3c )       /* DMC06CP1       */
#define PUNIPHIER_DMC06SS2  ( IXM_BASE + 0x00001b40 )       /* DMC06SS2       */
#define PUNIPHIER_DMC06DS2  ( IXM_BASE + 0x00001b44 )       /* DMC06DS2       */
#define PUNIPHIER_DMC06WD2  ( IXM_BASE + 0x00001b48 )       /* DMC06WD2       */
#define PUNIPHIER_DMC06CP2  ( IXM_BASE + 0x00001b4c )       /* DMC06CP2       */
#define PUNIPHIER_DMC06SS3  ( IXM_BASE + 0x00001b50 )       /* DMC06SS3       */
#define PUNIPHIER_DMC06DS3  ( IXM_BASE + 0x00001b54 )       /* DMC06DS3       */
#define PUNIPHIER_DMC06WD3  ( IXM_BASE + 0x00001b58 )       /* DMC06WD3       */
#define PUNIPHIER_DMC06CP3  ( IXM_BASE + 0x00001b5c )       /* DMC06CP3       */
#define PUNIPHIER_DMC07SSA  ( IXM_BASE + 0x00001b80 )       /* DMC07SSA       */
#define PUNIPHIER_DMC07SEA  ( IXM_BASE + 0x00001b84 )       /* DMC07SEA       */
#define PUNIPHIER_DMC07SBA  ( IXM_BASE + 0x00001b88 )       /* DMC07SBA       */
#define PUNIPHIER_DMC07SCA  ( IXM_BASE + 0x00001b8c )       /* DMC07SCA       */
#define PUNIPHIER_DMC07DSA  ( IXM_BASE + 0x00001b90 )       /* DMC07DSA       */
#define PUNIPHIER_DMC07DEA  ( IXM_BASE + 0x00001b94 )       /* DMC07DEA       */
#define PUNIPHIER_DMC07DBA  ( IXM_BASE + 0x00001b98 )       /* DMC07DBA       */
#define PUNIPHIER_DMC07DCA  ( IXM_BASE + 0x00001b9c )       /* DMC07DCA       */
#define PUNIPHIER_DMC07SS0  ( IXM_BASE + 0x00001ba0 )       /* DMC07SS0       */
#define PUNIPHIER_DMC07DS0  ( IXM_BASE + 0x00001ba4 )       /* DMC07DS0       */
#define PUNIPHIER_DMC07WD0  ( IXM_BASE + 0x00001ba8 )       /* DMC07WD0       */
#define PUNIPHIER_DMC07CP0  ( IXM_BASE + 0x00001bac )       /* DMC07CP0       */
#define PUNIPHIER_DMC07SS1  ( IXM_BASE + 0x00001bb0 )       /* DMC07SS1       */
#define PUNIPHIER_DMC07DS1  ( IXM_BASE + 0x00001bb4 )       /* DMC07DS1       */
#define PUNIPHIER_DMC07WD1  ( IXM_BASE + 0x00001bb8 )       /* DMC07WD1       */
#define PUNIPHIER_DMC07CP1  ( IXM_BASE + 0x00001bbc )       /* DMC07CP1       */
#define PUNIPHIER_DMC07SS2  ( IXM_BASE + 0x00001bc0 )       /* DMC07SS2       */
#define PUNIPHIER_DMC07DS2  ( IXM_BASE + 0x00001bc4 )       /* DMC07DS2       */
#define PUNIPHIER_DMC07WD2  ( IXM_BASE + 0x00001bc8 )       /* DMC07WD2       */
#define PUNIPHIER_DMC07CP2  ( IXM_BASE + 0x00001bcc )       /* DMC07CP2       */
#define PUNIPHIER_DMC07SS3  ( IXM_BASE + 0x00001bd0 )       /* DMC07SS3       */
#define PUNIPHIER_DMC07DS3  ( IXM_BASE + 0x00001bd4 )       /* DMC07DS3       */
#define PUNIPHIER_DMC07WD3  ( IXM_BASE + 0x00001bd8 )       /* DMC07WD3       */
#define PUNIPHIER_DMC07CP3  ( IXM_BASE + 0x00001bdc )       /* DMC07CP3       */
#define PUNIPHIER_DMC08SSA  ( IXM_BASE + 0x00001c00 )       /* DMC08SSA       */
#define PUNIPHIER_DMC08SEA  ( IXM_BASE + 0x00001c04 )       /* DMC08SEA       */
#define PUNIPHIER_DMC08SBA  ( IXM_BASE + 0x00001c08 )       /* DMC08SBA       */
#define PUNIPHIER_DMC08SCA  ( IXM_BASE + 0x00001c0c )       /* DMC08SCA       */
#define PUNIPHIER_DMC08DSA  ( IXM_BASE + 0x00001c10 )       /* DMC08DSA       */
#define PUNIPHIER_DMC08DEA  ( IXM_BASE + 0x00001c14 )       /* DMC08DEA       */
#define PUNIPHIER_DMC08DBA  ( IXM_BASE + 0x00001c18 )       /* DMC08DBA       */
#define PUNIPHIER_DMC08DCA  ( IXM_BASE + 0x00001c1c )       /* DMC08DCA       */
#define PUNIPHIER_DMC08SS0  ( IXM_BASE + 0x00001c20 )       /* DMC08SS0       */
#define PUNIPHIER_DMC08DS0  ( IXM_BASE + 0x00001c24 )       /* DMC08DS0       */
#define PUNIPHIER_DMC08WD0  ( IXM_BASE + 0x00001c28 )       /* DMC08WD0       */
#define PUNIPHIER_DMC08CP0  ( IXM_BASE + 0x00001c2c )       /* DMC08CP0       */
#define PUNIPHIER_DMC08SS1  ( IXM_BASE + 0x00001c30 )       /* DMC08SS1       */
#define PUNIPHIER_DMC08DS1  ( IXM_BASE + 0x00001c34 )       /* DMC08DS1       */
#define PUNIPHIER_DMC08WD1  ( IXM_BASE + 0x00001c38 )       /* DMC08WD1       */
#define PUNIPHIER_DMC08CP1  ( IXM_BASE + 0x00001c3c )       /* DMC08CP1       */
#define PUNIPHIER_DMC08SS2  ( IXM_BASE + 0x00001c40 )       /* DMC08SS2       */
#define PUNIPHIER_DMC08DS2  ( IXM_BASE + 0x00001c44 )       /* DMC08DS2       */
#define PUNIPHIER_DMC08WD2  ( IXM_BASE + 0x00001c48 )       /* DMC08WD2       */
#define PUNIPHIER_DMC08CP2  ( IXM_BASE + 0x00001c4c )       /* DMC08CP2       */
#define PUNIPHIER_DMC08SS3  ( IXM_BASE + 0x00001c50 )       /* DMC08SS3       */
#define PUNIPHIER_DMC08DS3  ( IXM_BASE + 0x00001c54 )       /* DMC08DS3       */
#define PUNIPHIER_DMC08WD3  ( IXM_BASE + 0x00001c58 )       /* DMC08WD3       */
#define PUNIPHIER_DMC08CP3  ( IXM_BASE + 0x00001c5c )       /* DMC08CP3       */
#define PUNIPHIER_DMC09SSA  ( IXM_BASE + 0x00001c80 )       /* DMC09SSA       */
#define PUNIPHIER_DMC09SEA  ( IXM_BASE + 0x00001c84 )       /* DMC09SEA       */
#define PUNIPHIER_DMC09SBA  ( IXM_BASE + 0x00001c88 )       /* DMC09SBA       */
#define PUNIPHIER_DMC09SCA  ( IXM_BASE + 0x00001c8c )       /* DMC09SCA       */
#define PUNIPHIER_DMC09DSA  ( IXM_BASE + 0x00001c90 )       /* DMC09DSA       */
#define PUNIPHIER_DMC09DEA  ( IXM_BASE + 0x00001c94 )       /* DMC09DEA       */
#define PUNIPHIER_DMC09DBA  ( IXM_BASE + 0x00001c98 )       /* DMC09DBA       */
#define PUNIPHIER_DMC09DCA  ( IXM_BASE + 0x00001c9c )       /* DMC09DCA       */
#define PUNIPHIER_DMC09SS0  ( IXM_BASE + 0x00001ca0 )       /* DMC09SS0       */
#define PUNIPHIER_DMC09DS0  ( IXM_BASE + 0x00001ca4 )       /* DMC09DS0       */
#define PUNIPHIER_DMC09WD0  ( IXM_BASE + 0x00001ca8 )       /* DMC09WD0       */
#define PUNIPHIER_DMC09CP0  ( IXM_BASE + 0x00001cac )       /* DMC09CP0       */
#define PUNIPHIER_DMC09SS1  ( IXM_BASE + 0x00001cb0 )       /* DMC09SS1       */
#define PUNIPHIER_DMC09DS1  ( IXM_BASE + 0x00001cb4 )       /* DMC09DS1       */
#define PUNIPHIER_DMC09WD1  ( IXM_BASE + 0x00001cb8 )       /* DMC09WD1       */
#define PUNIPHIER_DMC09CP1  ( IXM_BASE + 0x00001cbc )       /* DMC09CP1       */
#define PUNIPHIER_DMC09SS2  ( IXM_BASE + 0x00001cc0 )       /* DMC09SS2       */
#define PUNIPHIER_DMC09DS2  ( IXM_BASE + 0x00001cc4 )       /* DMC09DS2       */
#define PUNIPHIER_DMC09WD2  ( IXM_BASE + 0x00001cc8 )       /* DMC09WD2       */
#define PUNIPHIER_DMC09CP2  ( IXM_BASE + 0x00001ccc )       /* DMC09CP2       */
#define PUNIPHIER_DMC09SS3  ( IXM_BASE + 0x00001cd0 )       /* DMC09SS3       */
#define PUNIPHIER_DMC09DS3  ( IXM_BASE + 0x00001cd4 )       /* DMC09DS3       */
#define PUNIPHIER_DMC09WD3  ( IXM_BASE + 0x00001cd8 )       /* DMC09WD3       */
#define PUNIPHIER_DMC09CP3  ( IXM_BASE + 0x00001cdc )       /* DMC09CP3       */
#define PUNIPHIER_DMC10SSA  ( IXM_BASE + 0x00001d00 )       /* DMC10SSA       */
#define PUNIPHIER_DMC10SEA  ( IXM_BASE + 0x00001d04 )       /* DMC10SEA       */
#define PUNIPHIER_DMC10SBA  ( IXM_BASE + 0x00001d08 )       /* DMC10SBA       */
#define PUNIPHIER_DMC10SCA  ( IXM_BASE + 0x00001d0c )       /* DMC10SCA       */
#define PUNIPHIER_DMC10DSA  ( IXM_BASE + 0x00001d10 )       /* DMC10DSA       */
#define PUNIPHIER_DMC10DEA  ( IXM_BASE + 0x00001d14 )       /* DMC10DEA       */
#define PUNIPHIER_DMC10DBA  ( IXM_BASE + 0x00001d18 )       /* DMC10DBA       */
#define PUNIPHIER_DMC10DCA  ( IXM_BASE + 0x00001d1c )       /* DMC10DCA       */
#define PUNIPHIER_DMC10SS0  ( IXM_BASE + 0x00001d20 )       /* DMC10SS0       */
#define PUNIPHIER_DMC10DS0  ( IXM_BASE + 0x00001d24 )       /* DMC10DS0       */
#define PUNIPHIER_DMC10WD0  ( IXM_BASE + 0x00001d28 )       /* DMC10WD0       */
#define PUNIPHIER_DMC10CP0  ( IXM_BASE + 0x00001d2c )       /* DMC10CP0       */
#define PUNIPHIER_DMC10SS1  ( IXM_BASE + 0x00001d30 )       /* DMC10SS1       */
#define PUNIPHIER_DMC10DS1  ( IXM_BASE + 0x00001d34 )       /* DMC10DS1       */
#define PUNIPHIER_DMC10WD1  ( IXM_BASE + 0x00001d38 )       /* DMC10WD1       */
#define PUNIPHIER_DMC10CP1  ( IXM_BASE + 0x00001d3c )       /* DMC10CP1       */
#define PUNIPHIER_DMC10SS2  ( IXM_BASE + 0x00001d40 )       /* DMC10SS2       */
#define PUNIPHIER_DMC10DS2  ( IXM_BASE + 0x00001d44 )       /* DMC10DS2       */
#define PUNIPHIER_DMC10WD2  ( IXM_BASE + 0x00001d48 )       /* DMC10WD2       */
#define PUNIPHIER_DMC10CP2  ( IXM_BASE + 0x00001d4c )       /* DMC10CP2       */
#define PUNIPHIER_DMC10SS3  ( IXM_BASE + 0x00001d50 )       /* DMC10SS3       */
#define PUNIPHIER_DMC10DS3  ( IXM_BASE + 0x00001d54 )       /* DMC10DS3       */
#define PUNIPHIER_DMC10WD3  ( IXM_BASE + 0x00001d58 )       /* DMC10WD3       */
#define PUNIPHIER_DMC10CP3  ( IXM_BASE + 0x00001d5c )       /* DMC10CP3       */
#define PUNIPHIER_DMC11SSA  ( IXM_BASE + 0x00001d80 )       /* DMC11SSA       */
#define PUNIPHIER_DMC11SEA  ( IXM_BASE + 0x00001d84 )       /* DMC11SEA       */
#define PUNIPHIER_DMC11SBA  ( IXM_BASE + 0x00001d88 )       /* DMC11SBA       */
#define PUNIPHIER_DMC11SCA  ( IXM_BASE + 0x00001d8c )       /* DMC11SCA       */
#define PUNIPHIER_DMC11DSA  ( IXM_BASE + 0x00001d90 )       /* DMC11DSA       */
#define PUNIPHIER_DMC11DEA  ( IXM_BASE + 0x00001d94 )       /* DMC11DEA       */
#define PUNIPHIER_DMC11DBA  ( IXM_BASE + 0x00001d98 )       /* DMC11DBA       */
#define PUNIPHIER_DMC11DCA  ( IXM_BASE + 0x00001d9c )       /* DMC11DCA       */
#define PUNIPHIER_DMC11SS0  ( IXM_BASE + 0x00001da0 )       /* DMC11SS0       */
#define PUNIPHIER_DMC11DS0  ( IXM_BASE + 0x00001da4 )       /* DMC11DS0       */
#define PUNIPHIER_DMC11WD0  ( IXM_BASE + 0x00001da8 )       /* DMC11WD0       */
#define PUNIPHIER_DMC11CP0  ( IXM_BASE + 0x00001dac )       /* DMC11CP0       */
#define PUNIPHIER_DMC11SS1  ( IXM_BASE + 0x00001db0 )       /* DMC11SS1       */
#define PUNIPHIER_DMC11DS1  ( IXM_BASE + 0x00001db4 )       /* DMC11DS1       */
#define PUNIPHIER_DMC11WD1  ( IXM_BASE + 0x00001db8 )       /* DMC11WD1       */
#define PUNIPHIER_DMC11CP1  ( IXM_BASE + 0x00001dbc )       /* DMC11CP1       */
#define PUNIPHIER_DMC11SS2  ( IXM_BASE + 0x00001dc0 )       /* DMC11SS2       */
#define PUNIPHIER_DMC11DS2  ( IXM_BASE + 0x00001dc4 )       /* DMC11DS2       */
#define PUNIPHIER_DMC11WD2  ( IXM_BASE + 0x00001dc8 )       /* DMC11WD2       */
#define PUNIPHIER_DMC11CP2  ( IXM_BASE + 0x00001dcc )       /* DMC11CP2       */
#define PUNIPHIER_DMC11SS3  ( IXM_BASE + 0x00001dd0 )       /* DMC11SS3       */
#define PUNIPHIER_DMC11DS3  ( IXM_BASE + 0x00001dd4 )       /* DMC11DS3       */
#define PUNIPHIER_DMC11WD3  ( IXM_BASE + 0x00001dd8 )       /* DMC11WD3       */
#define PUNIPHIER_DMC11CP3  ( IXM_BASE + 0x00001ddc )       /* DMC11CP3       */
#define PUNIPHIER_DMC12SSA  ( IXM_BASE + 0x00001e00 )       /* DMC12SSA       */
#define PUNIPHIER_DMC12SEA  ( IXM_BASE + 0x00001e04 )       /* DMC12SEA       */
#define PUNIPHIER_DMC12SBA  ( IXM_BASE + 0x00001e08 )       /* DMC12SBA       */
#define PUNIPHIER_DMC12SCA  ( IXM_BASE + 0x00001e0c )       /* DMC12SCA       */
#define PUNIPHIER_DMC12DSA  ( IXM_BASE + 0x00001e10 )       /* DMC12DSA       */
#define PUNIPHIER_DMC12DEA  ( IXM_BASE + 0x00001e14 )       /* DMC12DEA       */
#define PUNIPHIER_DMC12DBA  ( IXM_BASE + 0x00001e18 )       /* DMC12DBA       */
#define PUNIPHIER_DMC12DCA  ( IXM_BASE + 0x00001e1c )       /* DMC12DCA       */
#define PUNIPHIER_DMC12SS0  ( IXM_BASE + 0x00001e20 )       /* DMC12SS0       */
#define PUNIPHIER_DMC12DS0  ( IXM_BASE + 0x00001e24 )       /* DMC12DS0       */
#define PUNIPHIER_DMC12WD0  ( IXM_BASE + 0x00001e28 )       /* DMC12WD0       */
#define PUNIPHIER_DMC12CP0  ( IXM_BASE + 0x00001e2c )       /* DMC12CP0       */
#define PUNIPHIER_DMC12SS1  ( IXM_BASE + 0x00001e30 )       /* DMC12SS1       */
#define PUNIPHIER_DMC12DS1  ( IXM_BASE + 0x00001e34 )       /* DMC12DS1       */
#define PUNIPHIER_DMC12WD1  ( IXM_BASE + 0x00001e38 )       /* DMC12WD1       */
#define PUNIPHIER_DMC12CP1  ( IXM_BASE + 0x00001e3c )       /* DMC12CP1       */
#define PUNIPHIER_DMC12SS2  ( IXM_BASE + 0x00001e40 )       /* DMC12SS2       */
#define PUNIPHIER_DMC12DS2  ( IXM_BASE + 0x00001e44 )       /* DMC12DS2       */
#define PUNIPHIER_DMC12WD2  ( IXM_BASE + 0x00001e48 )       /* DMC12WD2       */
#define PUNIPHIER_DMC12CP2  ( IXM_BASE + 0x00001e4c )       /* DMC12CP2       */
#define PUNIPHIER_DMC12SS3  ( IXM_BASE + 0x00001e50 )       /* DMC12SS3       */
#define PUNIPHIER_DMC12DS3  ( IXM_BASE + 0x00001e54 )       /* DMC12DS3       */
#define PUNIPHIER_DMC12WD3  ( IXM_BASE + 0x00001e58 )       /* DMC12WD3       */
#define PUNIPHIER_DMC12CP3  ( IXM_BASE + 0x00001e5c )       /* DMC12CP3       */
#define PUNIPHIER_DMC13SSA  ( IXM_BASE + 0x00001e80 )       /* DMC13SSA       */
#define PUNIPHIER_DMC13SEA  ( IXM_BASE + 0x00001e84 )       /* DMC13SEA       */
#define PUNIPHIER_DMC13SBA  ( IXM_BASE + 0x00001e88 )       /* DMC13SBA       */
#define PUNIPHIER_DMC13SCA  ( IXM_BASE + 0x00001e8c )       /* DMC13SCA       */
#define PUNIPHIER_DMC13DSA  ( IXM_BASE + 0x00001e90 )       /* DMC13DSA       */
#define PUNIPHIER_DMC13DEA  ( IXM_BASE + 0x00001e94 )       /* DMC13DEA       */
#define PUNIPHIER_DMC13DBA  ( IXM_BASE + 0x00001e98 )       /* DMC13DBA       */
#define PUNIPHIER_DMC13DCA  ( IXM_BASE + 0x00001e9c )       /* DMC13DCA       */
#define PUNIPHIER_DMC13SS0  ( IXM_BASE + 0x00001ea0 )       /* DMC13SS0       */
#define PUNIPHIER_DMC13DS0  ( IXM_BASE + 0x00001ea4 )       /* DMC13DS0       */
#define PUNIPHIER_DMC13WD0  ( IXM_BASE + 0x00001ea8 )       /* DMC13WD0       */
#define PUNIPHIER_DMC13CP0  ( IXM_BASE + 0x00001eac )       /* DMC13CP0       */
#define PUNIPHIER_DMC13SS1  ( IXM_BASE + 0x00001eb0 )       /* DMC13SS1       */
#define PUNIPHIER_DMC13DS1  ( IXM_BASE + 0x00001eb4 )       /* DMC13DS1       */
#define PUNIPHIER_DMC13WD1  ( IXM_BASE + 0x00001eb8 )       /* DMC13WD1       */
#define PUNIPHIER_DMC13CP1  ( IXM_BASE + 0x00001ebc )       /* DMC13CP1       */
#define PUNIPHIER_DMC13SS2  ( IXM_BASE + 0x00001ec0 )       /* DMC13SS2       */
#define PUNIPHIER_DMC13DS2  ( IXM_BASE + 0x00001ec4 )       /* DMC13DS2       */
#define PUNIPHIER_DMC13WD2  ( IXM_BASE + 0x00001ec8 )       /* DMC13WD2       */
#define PUNIPHIER_DMC13CP2  ( IXM_BASE + 0x00001ecc )       /* DMC13CP2       */
#define PUNIPHIER_DMC13SS3  ( IXM_BASE + 0x00001ed0 )       /* DMC13SS3       */
#define PUNIPHIER_DMC13DS3  ( IXM_BASE + 0x00001ed4 )       /* DMC13DS3       */
#define PUNIPHIER_DMC13WD3  ( IXM_BASE + 0x00001ed8 )       /* DMC13WD3       */
#define PUNIPHIER_DMC13CP3  ( IXM_BASE + 0x00001edc )       /* DMC13CP3       */
#define PUNIPHIER_DMC14SSA  ( IXM_BASE + 0x00001f00 )       /* DMC14SSA       */
#define PUNIPHIER_DMC14SEA  ( IXM_BASE + 0x00001f04 )       /* DMC14SEA       */
#define PUNIPHIER_DMC14SBA  ( IXM_BASE + 0x00001f08 )       /* DMC14SBA       */
#define PUNIPHIER_DMC14SCA  ( IXM_BASE + 0x00001f0c )       /* DMC14SCA       */
#define PUNIPHIER_DMC14DSA  ( IXM_BASE + 0x00001f10 )       /* DMC14DSA       */
#define PUNIPHIER_DMC14DEA  ( IXM_BASE + 0x00001f14 )       /* DMC14DEA       */
#define PUNIPHIER_DMC14DBA  ( IXM_BASE + 0x00001f18 )       /* DMC14DBA       */
#define PUNIPHIER_DMC14DCA  ( IXM_BASE + 0x00001f1c )       /* DMC14DCA       */
#define PUNIPHIER_DMC14SS0  ( IXM_BASE + 0x00001f20 )       /* DMC14SS0       */
#define PUNIPHIER_DMC14DS0  ( IXM_BASE + 0x00001f24 )       /* DMC14DS0       */
#define PUNIPHIER_DMC14WD0  ( IXM_BASE + 0x00001f28 )       /* DMC14WD0       */
#define PUNIPHIER_DMC14CP0  ( IXM_BASE + 0x00001f2c )       /* DMC14CP0       */
#define PUNIPHIER_DMC14SS1  ( IXM_BASE + 0x00001f30 )       /* DMC14SS1       */
#define PUNIPHIER_DMC14DS1  ( IXM_BASE + 0x00001f34 )       /* DMC14DS1       */
#define PUNIPHIER_DMC14WD1  ( IXM_BASE + 0x00001f38 )       /* DMC14WD1       */
#define PUNIPHIER_DMC14CP1  ( IXM_BASE + 0x00001f3c )       /* DMC14CP1       */
#define PUNIPHIER_DMC14SS2  ( IXM_BASE + 0x00001f40 )       /* DMC14SS2       */
#define PUNIPHIER_DMC14DS2  ( IXM_BASE + 0x00001f44 )       /* DMC14DS2       */
#define PUNIPHIER_DMC14WD2  ( IXM_BASE + 0x00001f48 )       /* DMC14WD2       */
#define PUNIPHIER_DMC14CP2  ( IXM_BASE + 0x00001f4c )       /* DMC14CP2       */
#define PUNIPHIER_DMC14SS3  ( IXM_BASE + 0x00001f50 )       /* DMC14SS3       */
#define PUNIPHIER_DMC14DS3  ( IXM_BASE + 0x00001f54 )       /* DMC14DS3       */
#define PUNIPHIER_DMC14WD3  ( IXM_BASE + 0x00001f58 )       /* DMC14WD3       */
#define PUNIPHIER_DMC14CP3  ( IXM_BASE + 0x00001f5c )       /* DMC14CP3       */
#define PUNIPHIER_DMC15SSA  ( IXM_BASE + 0x00001f80 )       /* DMC15SSA       */
#define PUNIPHIER_DMC15SEA  ( IXM_BASE + 0x00001f84 )       /* DMC15SEA       */
#define PUNIPHIER_DMC15SBA  ( IXM_BASE + 0x00001f88 )       /* DMC15SBA       */
#define PUNIPHIER_DMC15SCA  ( IXM_BASE + 0x00001f8c )       /* DMC15SCA       */
#define PUNIPHIER_DMC15DSA  ( IXM_BASE + 0x00001f90 )       /* DMC15DSA       */
#define PUNIPHIER_DMC15DEA  ( IXM_BASE + 0x00001f94 )       /* DMC15DEA       */
#define PUNIPHIER_DMC15DBA  ( IXM_BASE + 0x00001f98 )       /* DMC15DBA       */
#define PUNIPHIER_DMC15DCA  ( IXM_BASE + 0x00001f9c )       /* DMC15DCA       */
#define PUNIPHIER_DMC15SS0  ( IXM_BASE + 0x00001fa0 )       /* DMC15SS0       */
#define PUNIPHIER_DMC15DS0  ( IXM_BASE + 0x00001fa4 )       /* DMC15DS0       */
#define PUNIPHIER_DMC15WD0  ( IXM_BASE + 0x00001fa8 )       /* DMC15WD0       */
#define PUNIPHIER_DMC15CP0  ( IXM_BASE + 0x00001fac )       /* DMC15CP0       */
#define PUNIPHIER_DMC15SS1  ( IXM_BASE + 0x00001fb0 )       /* DMC15SS1       */
#define PUNIPHIER_DMC15DS1  ( IXM_BASE + 0x00001fb4 )       /* DMC15DS1       */
#define PUNIPHIER_DMC15WD1  ( IXM_BASE + 0x00001fb8 )       /* DMC15WD1       */
#define PUNIPHIER_DMC15CP1  ( IXM_BASE + 0x00001fbc )       /* DMC15CP1       */
#define PUNIPHIER_DMC15SS2  ( IXM_BASE + 0x00001fc0 )       /* DMC15SS2       */
#define PUNIPHIER_DMC15DS2  ( IXM_BASE + 0x00001fc4 )       /* DMC15DS2       */
#define PUNIPHIER_DMC15WD2  ( IXM_BASE + 0x00001fc8 )       /* DMC15WD2       */
#define PUNIPHIER_DMC15CP2  ( IXM_BASE + 0x00001fcc )       /* DMC15CP2       */
#define PUNIPHIER_DMC15SS3  ( IXM_BASE + 0x00001fd0 )       /* DMC15SS3       */
#define PUNIPHIER_DMC15DS3  ( IXM_BASE + 0x00001fd4 )       /* DMC15DS3       */
#define PUNIPHIER_DMC15WD3  ( IXM_BASE + 0x00001fd8 )       /* DMC15WD3       */
#define PUNIPHIER_DMC15CP3  ( IXM_BASE + 0x00001fdc )       /* DMC15CP3       */
#define PUNIPHIER_IXMCLKR   ( IXM_BASE + 0x00600000 )       /* IXMCLKR        */

/*------- ABC            0xD1800000 ----------------------------------------*/
#define ABC_BASE            ( PUNIPHIER_BASE + 0x01800000 ) /* ABC            */

#define PUNIPHIER_MSTBER    ( IPPBCU_BASE + 0x00000000 )    /* MSTBER         */
#define PUNIPHIER_ERRADR    ( IPPBCU_BASE + 0x00000004 )    /* ERRADR         */

/*------- IPP-BCU        0xD1C00000 ----------------------------------------*/
#define IPPBCU_BASE         ( PUNIPHIER_BASE + 0x01C00000 ) /* IPP-BCU        */

#define PUNIPHIER_IPPBOOTADRS ( IPPBCU_BASE + 0x00000000 )  /* IPPBOOTADRS    */
#define PUNIPHIER_IPPBCUCR  ( IPPBCU_BASE + 0x00000010 )    /* IPPBCUCR       */
#define PUNIPHIER_IPPBCUSR  ( IPPBCU_BASE + 0x00000014 )    /* IPPBCUSR       */
#define PUNIPHIER_IPPBCUECR ( IPPBCU_BASE + 0x00000020 )    /* IPPBCUECR      */

/*------- VPP            0xD2040000 ----------------------------------------*/
#define VPP_BASE            ( PUNIPHIER_BASE + 0x02040000 ) /* VPP            */

#define PUNIPHIER_VPPON     ( VPP_BASE + 0x00000000 )       /* VPPON          */
#define PUNIPHIER_VPPRST    ( VPP_BASE + 0x00000004 )       /* VPPRST         */
#define PUNIPHIER_VPPRGCLK  ( VPP_BASE + 0x00000008 )       /* VPPRGCLK       */
#define PUNIPHIER_VPPIMGWD  ( VPP_BASE + 0x00000010 )       /* VPPIMGWD       */
#define PUNIPHIER_VPPIMGHT  ( VPP_BASE + 0x00000014 )       /* VPPIMGHT       */
#define PUNIPHIER_VPPAPPENG ( VPP_BASE + 0x0000001C )       /* VPPAPPENG      */
#define PUNIPHIER_DBLKEN    ( VPP_BASE + 0x00000020 )       /* DBLKEN         */
#define PUNIPHIER_DBLKTH1   ( VPP_BASE + 0x00000024 )       /* DBLKTH1        */
#define PUNIPHIER_DBLKTH2   ( VPP_BASE + 0x00000028 )       /* DBLKTH2        */
#define PUNIPHIER_DBLKQP    ( VPP_BASE + 0x0000002C )       /* DBLKQP         */
#define PUNIPHIER_DMSQTEN   ( VPP_BASE + 0x00000030 )       /* DMSQTEN        */
#define PUNIPHIER_DMSQTTH   ( VPP_BASE + 0x00000034 )       /* DMSQTTH        */

/*------- PDMAC          0xD2050000 ----------------------------------------*/
#define PDMAC_BASE          ( PUNIPHIER_BASE + 0x02050000 ) /* PDMAC          */

#define PUNIPHIER_PDMACEXE  ( PDMAC_BASE + 0x00001000   )   /* PDMACEXE       */
#define PUNIPHIER_PDMACST   ( PDMAC_BASE + 0x00001004   )   /* PDMACST        */
#define PUNIPHIER_PDPMCKEN  ( PDMAC_BASE + 0x00001008   )   /* PDPMCKEN       */
#define PUNIPHIER_PDTESTSEL ( PDMAC_BASE + 0x00001010   )   /* PDTESTSEL      */
#define PUNIPHIER_VPPRASA   ( PDMAC_BASE + 0x00001100   )   /* VPPRASA        */
#define PUNIPHIER_VPPRHSIZE ( PDMAC_BASE + 0x00001104   )   /* VPPRHSIZE      */
#define PUNIPHIER_VPPWASA   ( PDMAC_BASE + 0x00001108   )   /* VPPWASA        */
#define PUNIPHIER_VPPWHSIZE ( PDMAC_BASE + 0x0000110C   )   /* VPPWHSIZE      */

/*------- MP-DBC         0xD2060000 ----------------------------------------*/
/*------- MPEG-4(IF)     0xD2200000 ----------------------------------------*/
#define MPEG4IF_BASE        ( PUNIPHIER_BASE + 0x02200000 ) /* MPEG4IF        */

#define PUNIPHIER_SEMA0     ( MPEG4IF_BASE + 0x00001000 )   /* SEMA0          */
#define PUNIPHIER_SEMA1     ( MPEG4IF_BASE + 0x00001004 )   /* SEMA1          */
#define PUNIPHIER_SEMA2     ( MPEG4IF_BASE + 0x00001008 )   /* SEMA2          */
#define PUNIPHIER_SEMA3     ( MPEG4IF_BASE + 0x0000100C )   /* SEMA3          */
#define PUNIPHIER_SEMAST    ( MPEG4IF_BASE + 0x00001010 )   /* SEMAST         */
#define PUNIPHIER_SEMACLR   ( MPEG4IF_BASE + 0x00001014 )   /* SEMACLR        */
#define PUNIPHIER_HIFGPR0   ( MPEG4IF_BASE + 0x00001020 )   /* HIFGPR0        */
#define PUNIPHIER_HIFGPR1   ( MPEG4IF_BASE + 0x00001024 )   /* HIFGPR1        */
#define PUNIPHIER_HMAFG     ( MPEG4IF_BASE + 0x00001028 )   /* HMAFG          */
#define PUNIPHIER_HMBFG     ( MPEG4IF_BASE + 0x0000102C )   /* HMBFG          */
#define PUNIPHIER_HMCFG     ( MPEG4IF_BASE + 0x00001030 )   /* HMCFG          */
#define PUNIPHIER_HMDFG     ( MPEG4IF_BASE + 0x00001034 )   /* HMDFG          */
#define PUNIPHIER_HMFG      ( MPEG4IF_BASE + 0x0000103C )   /* HMFG           */
#define PUNIPHIER_HMERR     ( MPEG4IF_BASE + 0x00001040 )   /* HMERR          */
#define PUNIPHIER_HMFERR    ( MPEG4IF_BASE + 0x00001044 )   /* HMFERR         */
#define PUNIPHIER_IRMPR     ( MPEG4IF_BASE + 0x00150048 )   /* IRMPR          */
#define PUNIPHIER_IRMPC     ( MPEG4IF_BASE + 0x00150050 )   /* IRMPC          */
#define PUNIPHIER_VHOLD     ( MPEG4IF_BASE + 0x001500C8 )   /* VHOLD          */
#define PUNIPHIER_VST       ( MPEG4IF_BASE + 0x001500CC )   /* VST            */
#define PUNIPHIER_DLCFG     ( MPEG4IF_BASE + 0x001500D0 )   /* DLCFG          */
#define PUNIPHIER_DLADR     ( MPEG4IF_BASE + 0x001500D4 )   /* DLADR          */
#define PUNIPHIER_DLDAT     ( MPEG4IF_BASE + 0x001500D8 )   /* DLDAT          */
#define PUNIPHIER_IRAMPC    ( MPEG4IF_BASE + 0x00150168 )   /* IRAMPC         */
#define PUNIPHIER_IRAMPR    ( MPEG4IF_BASE + 0x0015016C )   /* IRAMPR         */
#define PUNIPHIER_GCCTL     ( MPEG4IF_BASE + 0x00150184 )   /* GCCTL          */

/*============================================================================*/
/* Micom-0  Block   0xD5000000                                              */
/*============================================================================*/

/*------- ARMTIMER       0xD5000000 ----------------------------------------*/
#define ARMTIMER_BASE       ( PUNIPHIER_BASE + 0x05000000 ) /* ARMTIMER       */

#define PMICOM0_ARMPS0MD    ( ARMTIMER_BASE + 0x00000000 )  /* ARMPS0MD       */
#define PMICOM0_ARMPS0BR    ( ARMTIMER_BASE + 0x00000004 )  /* ARMPS0BR       */
#define PMICOM0_ARMPS0BC    ( ARMTIMER_BASE + 0x00000008 )  /* ARMPS0BC       */
#define PMICOM0_ARMPS1MD    ( ARMTIMER_BASE + 0x00000010 )  /* ARMPS1MD       */
#define PMICOM0_ARMPS1BR    ( ARMTIMER_BASE + 0x00000014 )  /* ARMPS1BR       */
#define PMICOM0_ARMPS1BC    ( ARMTIMER_BASE + 0x00000018 )  /* ARMPS1BC       */
#define PMICOM0_ARMTM0MD    ( ARMTIMER_BASE + 0x00000020 )  /* ARMTM0MD       */
#define PMICOM0_ARMTM0BR    ( ARMTIMER_BASE + 0x00000024 )  /* ARMTM0BR       */
#define PMICOM0_ARMTM0BC    ( ARMTIMER_BASE + 0x00000028 )  /* ARMTM0BC       */
#define PMICOM0_ARMTM1MD    ( ARMTIMER_BASE + 0x00000030 )  /* ARMTM1MD       */
#define PMICOM0_ARMTM1BR    ( ARMTIMER_BASE + 0x00000034 )  /* ARMTM1BR       */
#define PMICOM0_ARMTM1BC    ( ARMTIMER_BASE + 0x00000038 )  /* ARMTM1BC       */
#define PMICOM0_ARMTM2MD    ( ARMTIMER_BASE + 0x00000040 )  /* ARMTM2MD       */
#define PMICOM0_ARMTM2BR    ( ARMTIMER_BASE + 0x00000044 )  /* ARMTM2BR       */
#define PMICOM0_ARMTM2BC    ( ARMTIMER_BASE + 0x00000048 )  /* ARMTM2BC       */
#define PMICOM0_ARMTM3MD    ( ARMTIMER_BASE + 0x00000050 )  /* ARMTM3MD       */
#define PMICOM0_ARMTM3BR    ( ARMTIMER_BASE + 0x00000054 )  /* ARMTM3BR       */
#define PMICOM0_ARMTM3BC    ( ARMTIMER_BASE + 0x00000058 )  /* ARMTM3BC       */
#define PMICOM0_ARMPS2MD    ( ARMTIMER_BASE + 0x00000080 )  /* ARMPS2MD       */
#define PMICOM0_ARMPS2BR    ( ARMTIMER_BASE + 0x00000084 )  /* ARMPS2BR       */
#define PMICOM0_ARMPS2BC    ( ARMTIMER_BASE + 0x00000088 )  /* ARMPS2BC       */
#define PMICOM0_ARMPS3MD    ( ARMTIMER_BASE + 0x00000090 )  /* ARMPS3MD       */
#define PMICOM0_ARMPS3BR    ( ARMTIMER_BASE + 0x00000094 )  /* ARMPS3BR       */
#define PMICOM0_ARMPS3BC    ( ARMTIMER_BASE + 0x00000098 )  /* ARMPS3BC       */
#define PMICOM0_ARMTM4MD    ( ARMTIMER_BASE + 0x000000A0 )  /* ARMTM4MD       */
#define PMICOM0_ARMTM4BR    ( ARMTIMER_BASE + 0x000000A4 )  /* ARMTM4BR       */
#define PMICOM0_ARMTM4BC    ( ARMTIMER_BASE + 0x000000A8 )  /* ARMTM4BC       */
#define PMICOM0_ARMTM5MD    ( ARMTIMER_BASE + 0x000000B0 )  /* ARMTM5MD       */
#define PMICOM0_ARMTM5BR    ( ARMTIMER_BASE + 0x000000B4 )  /* ARMTM5BR       */
#define PMICOM0_ARMTM5BC    ( ARMTIMER_BASE + 0x000000B8 )  /* ARMTM5BC       */
#define PMICOM0_ARMTM6MD    ( ARMTIMER_BASE + 0x000000C0 )  /* ARMTM6MD       */
#define PMICOM0_ARMTM6BR    ( ARMTIMER_BASE + 0x000000C4 )  /* ARMTM6BR       */
#define PMICOM0_ARMTM6BC    ( ARMTIMER_BASE + 0x000000C8 )  /* ARMTM6BC       */
#define PMICOM0_ARMTM7MD    ( ARMTIMER_BASE + 0x000000D0 )  /* ARMTM7MD       */
#define PMICOM0_ARMTM7BR    ( ARMTIMER_BASE + 0x000000D4 )  /* ARMTM7BR       */
#define PMICOM0_ARMTM7BC    ( ARMTIMER_BASE + 0x000000D8 )  /* ARMTM7BC       */
#define PMICOM0_ARMPS4MD    ( ARMTIMER_BASE + 0x00000100 )  /* ARMPS4MD       */
#define PMICOM0_ARMPS4BR    ( ARMTIMER_BASE + 0x00000104 )  /* ARMPS4BR       */
#define PMICOM0_ARMPS4BC    ( ARMTIMER_BASE + 0x00000108 )  /* ARMPS4BC       */
#define PMICOM0_ARMPS5MD    ( ARMTIMER_BASE + 0x00000110 )  /* ARMPS5MD       */
#define PMICOM0_ARMPS5BR    ( ARMTIMER_BASE + 0x00000114 )  /* ARMPS5BR       */
#define PMICOM0_ARMPS5BC    ( ARMTIMER_BASE + 0x00000118 )  /* ARMPS5BC       */
#define PMICOM0_ARMTM8MD    ( ARMTIMER_BASE + 0x00000120 )  /* ARMTM8MD       */
#define PMICOM0_ARMTM8BR    ( ARMTIMER_BASE + 0x00000124 )  /* ARMTM8BR       */
#define PMICOM0_ARMTM8BC    ( ARMTIMER_BASE + 0x00000128 )  /* ARMTM8BC       */
#define PMICOM0_ARMTM9MD    ( ARMTIMER_BASE + 0x00000130 )  /* ARMTM9MD       */
#define PMICOM0_ARMTM9BR    ( ARMTIMER_BASE + 0x00000134 )  /* ARMTM9BR       */
#define PMICOM0_ARMTM9BC    ( ARMTIMER_BASE + 0x00000138 )  /* ARMTM9BC       */
#define PMICOM0_ARMTM10MD   ( ARMTIMER_BASE + 0x00000140 )  /* ARMTM10MD      */
#define PMICOM0_ARMTM10BR   ( ARMTIMER_BASE + 0x00000144 )  /* ARMTM10BR      */
#define PMICOM0_ARMTM10BC   ( ARMTIMER_BASE + 0x00000148 )  /* ARMTM10BC      */
#define PMICOM0_ARMTM11MD   ( ARMTIMER_BASE + 0x00000150 )  /* ARMTM11MD      */
#define PMICOM0_ARMTM11BR   ( ARMTIMER_BASE + 0x00000154 )  /* ARMTM11BR      */
#define PMICOM0_ARMTM11BC   ( ARMTIMER_BASE + 0x00000158 )  /* ARMTM11BC      */


/*------- RTCTIMER       0xD5010000 ----------------------------------------*/
#define RTCTIMER_BASE       ( PUNIPHIER_BASE + 0x05010000 ) /* RTCTIMER       */

#define PMICOM0_ARMPSRTCMD  ( RTCTIMER_BASE + 0x00000000 )  /* ARMPSRTCMD     */
#define PMICOM0_ARMPSRTCBR  ( RTCTIMER_BASE + 0x00000004 )  /* ARMPSRTCBR     */
#define PMICOM0_ARMPSRTCBC  ( RTCTIMER_BASE + 0x00000008 )  /* ARMPSRTCBC     */
#define PMICOM0_ARMTMRTC0MD ( RTCTIMER_BASE + 0x00000010 )  /* ARMTMRTC0MD    */
#define PMICOM0_ARMTMRTC0BR ( RTCTIMER_BASE + 0x00000014 )  /* ARMTMRTC0BR    */
#define PMICOM0_ARMTMRTC0BC ( RTCTIMER_BASE + 0x00000018 )  /* ARMTMRTC0BC    */
#define PMICOM0_ARMTMRTC1MD ( RTCTIMER_BASE + 0x00000020 )  /* ARMTMRTC1MD    */
#define PMICOM0_ARMTMRTC1BR ( RTCTIMER_BASE + 0x00000024 )  /* ARMTMRTC1MD    */
#define PMICOM0_ARMTMRTC1BC ( RTCTIMER_BASE + 0x00000028 )  /* ARMTMRTC1MD    */


/*------- ARMVIC         0xD5040000 ----------------------------------------*/
#define ARMVIC_BASE         ( PUNIPHIER_BASE + 0x05040000 ) /* ARMVIC         */

#define PMICOM0_FIQ1ENR     ( ARMVIC_BASE + 0x00000000 )    /* FIQ1ENR        */
#define PMICOM0_FIQ1CLR     ( ARMVIC_BASE + 0x00000004 )    /* FIQ1CLR        */
#define PMICOM0_IRQMLVR     ( ARMVIC_BASE + 0x00000010 )    /* IRQMLVR        */
#define PMICOM0_IRQMLVCLR   ( ARMVIC_BASE + 0x00000014 )    /* IRQMLVCLR      */
#define PMICOM0_IRQCFR      ( ARMVIC_BASE + 0x00000020 )    /* IRQCFR         */
#define PMICOM0_IRQBADDR    ( ARMVIC_BASE + 0x00000050 )    /* IRQBADDR       */
#define PMICOM0_IRQADDR1    ( ARMVIC_BASE + 0x00000100 )    /* IRQADDR1       */
#define PMICOM0_IRQADDR2    ( ARMVIC_BASE + 0x00000104 )    /* IRQADDR2       */
#define PMICOM0_IRQADDR3    ( ARMVIC_BASE + 0x00000108 )    /* IRQADDR3       */
#define PMICOM0_IRQADDR4    ( ARMVIC_BASE + 0x0000010C )    /* IRQADDR4       */
#define PMICOM0_IRQADDR5    ( ARMVIC_BASE + 0x00000110 )    /* IRQADDR5       */
#define PMICOM0_IRQADDR6    ( ARMVIC_BASE + 0x00000114 )    /* IRQADDR6       */
#define PMICOM0_IRQADDR7    ( ARMVIC_BASE + 0x00000118 )    /* IRQADDR7       */
#define PMICOM0_IRQADDR8    ( ARMVIC_BASE + 0x0000011C )    /* IRQADDR8       */
#define PMICOM0_IRQADDR9    ( ARMVIC_BASE + 0x00000120 )    /* IRQADDR9       */
#define PMICOM0_IRQADDR10   ( ARMVIC_BASE + 0x00000124 )    /* IRQADDR10      */
#define PMICOM0_IRQADDR11   ( ARMVIC_BASE + 0x00000128 )    /* IRQADDR11      */
#define PMICOM0_IRQADDR12   ( ARMVIC_BASE + 0x0000012C )    /* IRQADDR12      */
#define PMICOM0_IRQADDR13   ( ARMVIC_BASE + 0x00000130 )    /* IRQADDR13      */
#define PMICOM0_IRQADDR14   ( ARMVIC_BASE + 0x00000134 )    /* IRQADDR14      */
#define PMICOM0_IRQADDR15   ( ARMVIC_BASE + 0x00000138 )    /* IRQADDR15      */
#define PMICOM0_IRQADDR16   ( ARMVIC_BASE + 0x0000013C )    /* IRQADDR16      */
#define PMICOM0_IRQADDR17   ( ARMVIC_BASE + 0x00000140 )    /* IRQADDR17      */
#define PMICOM0_IRQADDR18   ( ARMVIC_BASE + 0x00000144 )    /* IRQADDR18      */
#define PMICOM0_IRQADDR19   ( ARMVIC_BASE + 0x00000148 )    /* IRQADDR19      */
#define PMICOM0_IRQADDR20   ( ARMVIC_BASE + 0x0000014C )    /* IRQADDR20      */
#define PMICOM0_IRQADDR21   ( ARMVIC_BASE + 0x00000150 )    /* IRQADDR21      */
#define PMICOM0_IRQADDR22   ( ARMVIC_BASE + 0x00000154 )    /* IRQADDR22      */
#define PMICOM0_IRQADDR23   ( ARMVIC_BASE + 0x00000158 )    /* IRQADDR23      */
#define PMICOM0_IRQADDR24   ( ARMVIC_BASE + 0x0000015C )    /* IRQADDR24      */
#define PMICOM0_IRQADDR25   ( ARMVIC_BASE + 0x00000160 )    /* IRQADDR25      */
#define PMICOM0_IRQADDR26   ( ARMVIC_BASE + 0x00000164 )    /* IRQADDR26      */
#define PMICOM0_IRQADDR27   ( ARMVIC_BASE + 0x00000168 )    /* IRQADDR27      */
#define PMICOM0_IRQADDR28   ( ARMVIC_BASE + 0x0000016C )    /* IRQADDR28      */
#define PMICOM0_IRQADDR29   ( ARMVIC_BASE + 0x00000170 )    /* IRQADDR29      */
#define PMICOM0_IRQADDR30   ( ARMVIC_BASE + 0x00000174 )    /* IRQADDR30      */
#define PMICOM0_IRQADDR31   ( ARMVIC_BASE + 0x00000178 )    /* IRQADDR31      */
#define PMICOM0_IRQADDR32   ( ARMVIC_BASE + 0x0000017C )    /* IRQADDR32      */
#define PMICOM0_IRQADDR33   ( ARMVIC_BASE + 0x00000180 )    /* IRQADDR33      */
#define PMICOM0_IRQADDR34   ( ARMVIC_BASE + 0x00000184 )    /* IRQADDR34      */
#define PMICOM0_IRQADDR35   ( ARMVIC_BASE + 0x00000188 )    /* IRQADDR35      */
#define PMICOM0_IRQADDR36   ( ARMVIC_BASE + 0x0000018C )    /* IRQADDR36      */
#define PMICOM0_IRQADDR37   ( ARMVIC_BASE + 0x00000190 )    /* IRQADDR37      */
#define PMICOM0_IRQADDR38   ( ARMVIC_BASE + 0x00000194 )    /* IRQADDR38      */
#define PMICOM0_IRQADDR39   ( ARMVIC_BASE + 0x00000198 )    /* IRQADDR39      */
#define PMICOM0_IRQADDR40   ( ARMVIC_BASE + 0x0000019C )    /* IRQADDR40      */
#define PMICOM0_IRQADDR41   ( ARMVIC_BASE + 0x000001A0 )    /* IRQADDR41      */
#define PMICOM0_IRQADDR42   ( ARMVIC_BASE + 0x000001A4 )    /* IRQADDR42      */
#define PMICOM0_IRQADDR43   ( ARMVIC_BASE + 0x000001A8 )    /* IRQADDR43      */
#define PMICOM0_IRQADDR44   ( ARMVIC_BASE + 0x000001AC )    /* IRQADDR44      */
#define PMICOM0_IRQADDR45   ( ARMVIC_BASE + 0x000001B0 )    /* IRQADDR45      */
#define PMICOM0_IRQADDR46   ( ARMVIC_BASE + 0x000001B4 )    /* IRQADDR46      */
#define PMICOM0_IRQADDR47   ( ARMVIC_BASE + 0x000001B8 )    /* IRQADDR47      */
#define PMICOM0_IRQADDR48   ( ARMVIC_BASE + 0x000001BC )    /* IRQADDR48      */
#define PMICOM0_IRQADDR49   ( ARMVIC_BASE + 0x000001C0 )    /* IRQADDR49      */
#define PMICOM0_IRQADDR50   ( ARMVIC_BASE + 0x000001C4 )    /* IRQADDR50      */
#define PMICOM0_IRQADDR51   ( ARMVIC_BASE + 0x000001C8 )    /* IRQADDR51      */
#define PMICOM0_IRQADDR52   ( ARMVIC_BASE + 0x000001CC )    /* IRQADDR52      */
#define PMICOM0_IRQADDR53   ( ARMVIC_BASE + 0x000001D0 )    /* IRQADDR53      */
#define PMICOM0_IRQADDR54   ( ARMVIC_BASE + 0x000001D4 )    /* IRQADDR54      */
#define PMICOM0_IRQADDR55   ( ARMVIC_BASE + 0x000001D8 )    /* IRQADDR55      */
#define PMICOM0_IRQADDR56   ( ARMVIC_BASE + 0x000001DC )    /* IRQADDR56      */
#define PMICOM0_IRQADDR57   ( ARMVIC_BASE + 0x000001E0 )    /* IRQADDR57      */
#define PMICOM0_IRQADDR58   ( ARMVIC_BASE + 0x000001E4 )    /* IRQADDR58      */
#define PMICOM0_IRQADDR59   ( ARMVIC_BASE + 0x000001E8 )    /* IRQADDR59      */
#define PMICOM0_IRQADDR60   ( ARMVIC_BASE + 0x000001EC )    /* IRQADDR60      */
#define PMICOM0_IRQADDR61   ( ARMVIC_BASE + 0x000001F0 )    /* IRQADDR61      */
#define PMICOM0_IRQADDR62   ( ARMVIC_BASE + 0x000001F4 )    /* IRQADDR62      */
#define PMICOM0_IRQADDR63   ( ARMVIC_BASE + 0x000001F8 )    /* IRQADDR63      */
#define PMICOM0_IRQADDR64   ( ARMVIC_BASE + 0x000001FC )    /* IRQADDR64      */
#define PMICOM0_IRQADDR65   ( ARMVIC_BASE + 0x00000200 )    /* IRQADDR65      */
#define PMICOM0_IRQADDR66   ( ARMVIC_BASE + 0x00000204 )    /* IRQADDR66      */
#define PMICOM0_IRQADDR67   ( ARMVIC_BASE + 0x00000208 )    /* IRQADDR67      */
#define PMICOM0_IRQADDR68   ( ARMVIC_BASE + 0x0000020C )    /* IRQADDR68      */
#define PMICOM0_IRQADDR69   ( ARMVIC_BASE + 0x00000210 )    /* IRQADDR69      */
#define PMICOM0_IRQADDR70   ( ARMVIC_BASE + 0x00000214 )    /* IRQADDR70      */
#define PMICOM0_IRQADDR71   ( ARMVIC_BASE + 0x00000218 )    /* IRQADDR71      */
#define PMICOM0_IRQADDR72   ( ARMVIC_BASE + 0x0000021C )    /* IRQADDR72      */
#define PMICOM0_IRQADDR73   ( ARMVIC_BASE + 0x00000220 )    /* IRQADDR73      */
#define PMICOM0_IRQADDR74   ( ARMVIC_BASE + 0x00000224 )    /* IRQADDR74      */
#define PMICOM0_IRQADDR75   ( ARMVIC_BASE + 0x00000228 )    /* IRQADDR75      */
#define PMICOM0_IRQADDR76   ( ARMVIC_BASE + 0x0000022C )    /* IRQADDR76      */
#define PMICOM0_IRQADDR77   ( ARMVIC_BASE + 0x00000230 )    /* IRQADDR77      */
#define PMICOM0_IRQADDR78   ( ARMVIC_BASE + 0x00000234 )    /* IRQADDR78      */
#define PMICOM0_IRQADDR79   ( ARMVIC_BASE + 0x00000238 )    /* IRQADDR79      */
#define PMICOM0_IRQADDR80   ( ARMVIC_BASE + 0x0000023C )    /* IRQADDR80      */
#define PMICOM0_IRQADDR81   ( ARMVIC_BASE + 0x00000240 )    /* IRQADDR81      */
#define PMICOM0_IRQADDR82   ( ARMVIC_BASE + 0x00000244 )    /* IRQADDR82      */
#define PMICOM0_IRQADDR83   ( ARMVIC_BASE + 0x00000248 )    /* IRQADDR83      */
#define PMICOM0_IRQADDR84   ( ARMVIC_BASE + 0x0000024C )    /* IRQADDR84      */
#define PMICOM0_IRQADDR85   ( ARMVIC_BASE + 0x00000250 )    /* IRQADDR85      */
#define PMICOM0_IRQCR1      ( ARMVIC_BASE + 0x00000300 )    /* IRQCR1         */
#define PMICOM0_IRQCR2      ( ARMVIC_BASE + 0x00000304 )    /* IRQCR2         */
#define PMICOM0_IRQCR3      ( ARMVIC_BASE + 0x00000308 )    /* IRQCR3         */
#define PMICOM0_IRQCR4      ( ARMVIC_BASE + 0x0000030C )    /* IRQCR4         */
#define PMICOM0_IRQCR5      ( ARMVIC_BASE + 0x00000310 )    /* IRQCR5         */
#define PMICOM0_IRQCR6      ( ARMVIC_BASE + 0x00000314 )    /* IRQCR6         */
#define PMICOM0_IRQCR7      ( ARMVIC_BASE + 0x00000318 )    /* IRQCR7         */
#define PMICOM0_IRQCR8      ( ARMVIC_BASE + 0x0000031C )    /* IRQCR8         */
#define PMICOM0_IRQCR9      ( ARMVIC_BASE + 0x00000320 )    /* IRQCR9         */
#define PMICOM0_IRQCR10     ( ARMVIC_BASE + 0x00000324 )    /* IRQCR10        */
#define PMICOM0_IRQCR11     ( ARMVIC_BASE + 0x00000328 )    /* IRQCR11        */
#define PMICOM0_IRQCR12     ( ARMVIC_BASE + 0x0000032C )    /* IRQCR12        */
#define PMICOM0_IRQCR13     ( ARMVIC_BASE + 0x00000330 )    /* IRQCR13        */
#define PMICOM0_IRQCR14     ( ARMVIC_BASE + 0x00000334 )    /* IRQCR14        */
#define PMICOM0_IRQCR15     ( ARMVIC_BASE + 0x00000338 )    /* IRQCR15        */
#define PMICOM0_IRQCR16     ( ARMVIC_BASE + 0x0000033C )    /* IRQCR16        */
#define PMICOM0_IRQCR17     ( ARMVIC_BASE + 0x00000340 )    /* IRQCR17        */
#define PMICOM0_IRQCR18     ( ARMVIC_BASE + 0x00000344 )    /* IRQCR18        */
#define PMICOM0_IRQCR19     ( ARMVIC_BASE + 0x00000348 )    /* IRQCR19        */
#define PMICOM0_IRQCR20     ( ARMVIC_BASE + 0x0000034C )    /* IRQCR20        */
#define PMICOM0_IRQCR21     ( ARMVIC_BASE + 0x00000350 )    /* IRQCR21        */
#define PMICOM0_IRQCR22     ( ARMVIC_BASE + 0x00000354 )    /* IRQCR22        */
#define PMICOM0_IRQCR23     ( ARMVIC_BASE + 0x00000358 )    /* IRQCR23        */
#define PMICOM0_IRQCR24     ( ARMVIC_BASE + 0x0000035C )    /* IRQCR24        */
#define PMICOM0_IRQCR25     ( ARMVIC_BASE + 0x00000360 )    /* IRQCR25        */
#define PMICOM0_IRQCR26     ( ARMVIC_BASE + 0x00000364 )    /* IRQCR26        */
#define PMICOM0_IRQCR27     ( ARMVIC_BASE + 0x00000368 )    /* IRQCR27        */
#define PMICOM0_IRQCR28     ( ARMVIC_BASE + 0x0000036C )    /* IRQCR28        */
#define PMICOM0_IRQCR29     ( ARMVIC_BASE + 0x00000370 )    /* IRQCR29        */
#define PMICOM0_IRQCR30     ( ARMVIC_BASE + 0x00000374 )    /* IRQCR30        */
#define PMICOM0_IRQCR31     ( ARMVIC_BASE + 0x00000378 )    /* IRQCR31        */
#define PMICOM0_IRQCR32     ( ARMVIC_BASE + 0x0000037C )    /* IRQCR32        */
#define PMICOM0_IRQCR33     ( ARMVIC_BASE + 0x00000380 )    /* IRQCR33        */
#define PMICOM0_IRQCR34     ( ARMVIC_BASE + 0x00000384 )    /* IRQCR34        */
#define PMICOM0_IRQCR35     ( ARMVIC_BASE + 0x00000388 )    /* IRQCR35        */
#define PMICOM0_IRQCR36     ( ARMVIC_BASE + 0x0000038C )    /* IRQCR36        */
#define PMICOM0_IRQCR37     ( ARMVIC_BASE + 0x00000390 )    /* IRQCR37        */
#define PMICOM0_IRQCR38     ( ARMVIC_BASE + 0x00000394 )    /* IRQCR38        */
#define PMICOM0_IRQCR39     ( ARMVIC_BASE + 0x00000398 )    /* IRQCR39        */
#define PMICOM0_IRQCR40     ( ARMVIC_BASE + 0x0000039C )    /* IRQCR40        */
#define PMICOM0_IRQCR41     ( ARMVIC_BASE + 0x000003A0 )    /* IRQCR41        */
#define PMICOM0_IRQCR42     ( ARMVIC_BASE + 0x000003A4 )    /* IRQCR42        */
#define PMICOM0_IRQCR43     ( ARMVIC_BASE + 0x000003A8 )    /* IRQCR43        */
#define PMICOM0_IRQCR44     ( ARMVIC_BASE + 0x000003AC )    /* IRQCR44        */
#define PMICOM0_IRQCR45     ( ARMVIC_BASE + 0x000003B0 )    /* IRQCR45        */
#define PMICOM0_IRQCR46     ( ARMVIC_BASE + 0x000003B4 )    /* IRQCR46        */
#define PMICOM0_IRQCR47     ( ARMVIC_BASE + 0x000003B8 )    /* IRQCR47        */
#define PMICOM0_IRQCR48     ( ARMVIC_BASE + 0x000003BC )    /* IRQCR48        */
#define PMICOM0_IRQCR49     ( ARMVIC_BASE + 0x000003C0 )    /* IRQCR49        */
#define PMICOM0_IRQCR50     ( ARMVIC_BASE + 0x000003C4 )    /* IRQCR50        */
#define PMICOM0_IRQCR51     ( ARMVIC_BASE + 0x000003C8 )    /* IRQCR51        */
#define PMICOM0_IRQCR52     ( ARMVIC_BASE + 0x000003CC )    /* IRQCR52        */
#define PMICOM0_IRQCR53     ( ARMVIC_BASE + 0x000003D0 )    /* IRQCR53        */
#define PMICOM0_IRQCR54     ( ARMVIC_BASE + 0x000003D4 )    /* IRQCR54        */
#define PMICOM0_IRQCR55     ( ARMVIC_BASE + 0x000003D8 )    /* IRQCR55        */
#define PMICOM0_IRQCR56     ( ARMVIC_BASE + 0x000003DC )    /* IRQCR56        */
#define PMICOM0_IRQCR57     ( ARMVIC_BASE + 0x000003E0 )    /* IRQCR57        */
#define PMICOM0_IRQCR58     ( ARMVIC_BASE + 0x000003E4 )    /* IRQCR58        */
#define PMICOM0_IRQCR59     ( ARMVIC_BASE + 0x000003E8 )    /* IRQCR59        */
#define PMICOM0_IRQCR60     ( ARMVIC_BASE + 0x000003EC )    /* IRQCR60        */
#define PMICOM0_IRQCR61     ( ARMVIC_BASE + 0x000003F0 )    /* IRQCR61        */
#define PMICOM0_IRQCR62     ( ARMVIC_BASE + 0x000003F4 )    /* IRQCR62        */
#define PMICOM0_IRQCR63     ( ARMVIC_BASE + 0x000003F8 )    /* IRQCR63        */
#define PMICOM0_IRQCR64     ( ARMVIC_BASE + 0x000003FC )    /* IRQCR64        */
#define PMICOM0_IRQCR65     ( ARMVIC_BASE + 0x00000400 )    /* IRQCR65        */
#define PMICOM0_IRQCR66     ( ARMVIC_BASE + 0x00000404 )    /* IRQCR66        */
#define PMICOM0_IRQCR67     ( ARMVIC_BASE + 0x00000408 )    /* IRQCR67        */
#define PMICOM0_IRQCR68     ( ARMVIC_BASE + 0x0000040C )    /* IRQCR68        */
#define PMICOM0_IRQCR69     ( ARMVIC_BASE + 0x00000410 )    /* IRQCR69        */
#define PMICOM0_IRQCR70     ( ARMVIC_BASE + 0x00000414 )    /* IRQCR70        */
#define PMICOM0_IRQCR71     ( ARMVIC_BASE + 0x00000418 )    /* IRQCR71        */
#define PMICOM0_IRQCR72     ( ARMVIC_BASE + 0x0000041C )    /* IRQCR72        */
#define PMICOM0_IRQCR73     ( ARMVIC_BASE + 0x00000420 )    /* IRQCR73        */
#define PMICOM0_IRQCR74     ( ARMVIC_BASE + 0x00000424 )    /* IRQCR74        */
#define PMICOM0_IRQCR75     ( ARMVIC_BASE + 0x00000428 )    /* IRQCR75        */
#define PMICOM0_IRQCR76     ( ARMVIC_BASE + 0x0000042C )    /* IRQCR76        */
#define PMICOM0_IRQCR77     ( ARMVIC_BASE + 0x00000430 )    /* IRQCR77        */
#define PMICOM0_IRQCR78     ( ARMVIC_BASE + 0x00000434 )    /* IRQCR78        */
#define PMICOM0_IRQCR79     ( ARMVIC_BASE + 0x00000438 )    /* IRQCR79        */
#define PMICOM0_IRQCR80     ( ARMVIC_BASE + 0x0000043C )    /* IRQCR80        */
#define PMICOM0_IRQCR81     ( ARMVIC_BASE + 0x00000440 )    /* IRQCR81        */
#define PMICOM0_IRQCR82     ( ARMVIC_BASE + 0x00000444 )    /* IRQCR82        */
#define PMICOM0_IRQCR83     ( ARMVIC_BASE + 0x00000448 )    /* IRQCR83        */
#define PMICOM0_IRQCR84     ( ARMVIC_BASE + 0x0000044C )    /* IRQCR84        */
#define PMICOM0_IRQCR85     ( ARMVIC_BASE + 0x00000450 )    /* IRQCR85        */


/*------- KeyScan        0xD5050000 ----------------------------------------*/
#define KEYSCAN_BASE        ( PUNIPHIER_BASE + 0x05050000 ) /* KeyScan        */

#define PMICOM0_KDNC        ( KEYSCAN_BASE + 0x00000000 )   /* KDNC           */
#define PMICOM0_KDTB        ( KEYSCAN_BASE + 0x00000004 )   /* KDTB           */
#define PMICOM0_KAUC        ( KEYSCAN_BASE + 0x00000008 )   /* KAUC           */
#define PMICOM0_KATB        ( KEYSCAN_BASE + 0x0000000C )   /* KATB           */
#define PMICOM0_KREQ        ( KEYSCAN_BASE + 0x00000010 )   /* KREQ           */
#define PMICOM0_KMSK        ( KEYSCAN_BASE + 0x00000014 )   /* KMSK           */
#define PMICOM0_KINT        ( KEYSCAN_BASE + 0x00000018 )   /* KINT           */
#define PMICOM0_KFLG        ( KEYSCAN_BASE + 0x0000001C )   /* KFLG           */
#define PMICOM0_KSCA        ( KEYSCAN_BASE + 0x00000020 )   /* KSCA           */
#define PMICOM0_KSE0        ( KEYSCAN_BASE + 0x00000024 )   /* KSE0           */
#define PMICOM0_KSE1        ( KEYSCAN_BASE + 0x00000028 )   /* KSE1           */
#define PMICOM0_KSE2        ( KEYSCAN_BASE + 0x0000002C )   /* KSE2           */
#define PMICOM0_KSE3        ( KEYSCAN_BASE + 0x00000030 )   /* KSE3           */
#define PMICOM0_KSE4        ( KEYSCAN_BASE + 0x00000034 )   /* KSE4           */
#define PMICOM0_KSE5        ( KEYSCAN_BASE + 0x00000038 )   /* KSE5           */


/*------- Protect Module 0xD5080000 ----------------------------------------*/


/*------- CLKGEN         0xD5100000 ----------------------------------------*/
#define CLKGEN_BASE         ( PUNIPHIER_BASE + 0x05100000 ) /* CLKGEN         */

#define PMICOM0_CLKMDNLREG    ( CLKGEN_BASE + 0x00000000 )  /* CLKMDNLREG     */
#define PMICOM0_IPPGEARREG    ( CLKGEN_BASE + 0x00000004 )  /* IPPGEARREG     */
#define PMICOM0_BAUDREG       ( CLKGEN_BASE + 0x0000000C )  /* BAUDREG        */
#define PMICOM0_BSSI1CLKREG1  ( CLKGEN_BASE + 0x00000010 )  /* BSSI1CLKREG1   */
#define PMICOM0_BSSI1CLKREG2  ( CLKGEN_BASE + 0x00000014 )  /* BSSI1CLKREG2   */
#define PMICOM0_BSSI2CLKREG1  ( CLKGEN_BASE + 0x00000018 )  /* BSSI2CLKREG1   */
#define PMICOM0_BSSI2CLKREG2  ( CLKGEN_BASE + 0x0000001C )  /* BSSI2CLKREG2   */
#define PMICOM0_BSSI3CLKREG   ( CLKGEN_BASE + 0x00000020 )  /* BSSI3CLKREG    */
#define PMICOM0_BSSI4CLKREG   ( CLKGEN_BASE + 0x00000024 )  /* BSSI4CLKREG    */
#define PMICOM0_BSSI5CLKREG   ( CLKGEN_BASE + 0x00000028 )  /* BSSI5CLKREG    */
#define PMICOM0_CAMMCLKREG    ( CLKGEN_BASE + 0x0000002C )  /* CAMMCLKREG     */
#define PMICOM0_LCDCLKREG     ( CLKGEN_BASE + 0x00000030 )  /* LCDCLKREG      */
#define PMICOM0_SLWCLKREG     ( CLKGEN_BASE + 0x00000034 )  /* SLWCLKREG      */
#define PMICOM0_THIPPSTOP     ( CLKGEN_BASE + 0x00000038 )  /* THIPPSTOP      */
#define PMICOM0_IPPMREG       ( CLKGEN_BASE + 0x0000003C )  /* IPPMREG        */
#define PMICOM0_CLKENREG1     ( CLKGEN_BASE + 0x00000040 )  /* CLKENREG1      */
#define PMICOM0_CLKENREG2     ( CLKGEN_BASE + 0x00000044 )  /* CLKENREG2      */
#define PMICOM0_CLKENREG3     ( CLKGEN_BASE + 0x00000048 )  /* CLKENREG3      */
#define PMICOM0_CLKENREG4     ( CLKGEN_BASE + 0x0000004C )  /* CLKENREG4      */
#define PMICOM0_CLKENREG5     ( CLKGEN_BASE + 0x00000050 )  /* CLKENREG5      */
#define PMICOM0_CLKENREG6     ( CLKGEN_BASE + 0x00000054 )  /* CLKENREG6      */
#define PMICOM0_CLKENREG7     ( CLKGEN_BASE + 0x00000058 )  /* CLKENREG7      */
#define PMICOM0_CLKENREG8     ( CLKGEN_BASE + 0x0000005C )  /* CLKENREG8      */
#define PMICOM0_CLKENREG9     ( CLKGEN_BASE + 0x00000060 )  /* CLKENREG9      */
#define PMICOM0_OSCCNTREG     ( CLKGEN_BASE + 0x00000064 )  /* OSCCNTREG      */
#define PMICOM0_BOOTCLKREG    ( CLKGEN_BASE + 0x00000068 )  /* BOOTCLKREG     */
#define PMICOM0_CLK12O2REG    ( CLKGEN_BASE + 0x0000006C )  /* CLK12O2REG     */


/*------- RESET/Power    0xD5110000 ----------------------------------------*/
#define RESET_BASE          ( PUNIPHIER_BASE + 0x05110000 ) /* RESET/Power    */

#define PMICOM0_RSTCTL      ( RESET_BASE + 0x00001000 )     /* RSTCTL         */
#define PMICOM0_RSTWMOD     ( RESET_BASE + 0x00001004 )     /* RSTWMOD        */
#define PMICOM0_RSTNINITREQ ( RESET_BASE + 0x00001008 )     /* RSTNINITREQ    */
#define PMICOM0_SLFRST      ( RESET_BASE + 0x0000100C )     /* SLFRST         */
#define PMICOM0_RSTCWDT     ( RESET_BASE + 0x00001010 )     /* RSTCWDT        */
#define PMICOM0_RSTCSLF     ( RESET_BASE + 0x00001014 )     /* RSTCSLF        */
#define PMICOM0_RSTWDTREQ   ( RESET_BASE + 0x00001018 )     /* RSTWDTREQ      */
#define PMICOM0_NMITIMCTL   ( RESET_BASE + 0x0000101C )     /* NMITIMCTL      */
#define PMICOM0_RSTTIMCTL1  ( RESET_BASE + 0x00001020 )     /* RSTTIMCTL1     */
#define PMICOM0_RSTTIMCTL2  ( RESET_BASE + 0x00001024 )     /* RSTTIMCTL2     */
#define PMICOM0_RSTCTL21    ( RESET_BASE + 0x0000102C )     /* RSTCTL21       */
#define PMICOM0_RSTCTL22    ( RESET_BASE + 0x00001030 )     /* RSTCTL22       */
#define PMICOM0_RSTCTL31    ( RESET_BASE + 0x00001034 )     /* RSTCTL31       */
#define PMICOM0_RSTCTL32    ( RESET_BASE + 0x00001038 )     /* RSTCTL32       */
#define PMICOM0_RSTCTL41    ( RESET_BASE + 0x0000103C )     /* RSTCTL41       */
#define PMICOM0_RSTCTL42    ( RESET_BASE + 0x00001040 )     /* RSTCTL42       */
#define PMICOM0_RSTCTL51    ( RESET_BASE + 0x00001044 )     /* RSTCTL51       */
#define PMICOM0_RSTCTL52    ( RESET_BASE + 0x00001048 )     /* RSTCTL52       */
#define PMICOM0_RSTCTL61    ( RESET_BASE + 0x0000104C )     /* RSTCTL61       */
#define PMICOM0_RSTCTL62    ( RESET_BASE + 0x00001050 )     /* RSTCTL62       */
#define PMICOM0_RSTCTL7     ( RESET_BASE + 0x00001054 )     /* RSTCTL7        */
#define PMICOM0_PLL2TIMCTL  ( RESET_BASE + 0x00001058 )     /* PLL2TIMCTL     */
#define PMICOM0_IPPRSTCTR   ( RESET_BASE + 0x0000105C )     /* IPPRSTCTR      */
#define PMICOM0_RSTFMOD     ( RESET_BASE + 0x00002000 )     /* RSTFMOD        */
#define PMICOM0_PPCNTREG1   ( RESET_BASE + 0x00002008 )     /* PPCNTREG1      */
#define PMICOM0_PONTIMCTL   ( RESET_BASE + 0x00002014 )     /* PONTIMCTL      */
#define PMICOM0_PONIRQREG   ( RESET_BASE + 0x0000201C )     /* PONIRQREG      */
#define PMICOM0_PONTMRREG   ( RESET_BASE + 0x00002020 )     /* PONTMRREG      */
#define PMICOM0_IFENREG     ( RESET_BASE + 0x00002024 )     /* IFENREG        */
#define PMICOM0_INTHOLD     ( RESET_BASE + 0x00002028 )     /* INTHOLD        */


/*------- UART1          0xD5130000 ----------------------------------------*/
#define UART1_BASE          ( PUNIPHIER_BASE + 0x05130000 ) /* UART1          */

#define PMICOM0_UA1_RDR     ( UART1_BASE + 0x00000000 )     /* UA1_RDR        */
#define PMICOM0_UA1_TDR     ( UART1_BASE + 0x00000000 )     /* UA1_TDR        */
#define PMICOM0_UA1_IER     ( UART1_BASE + 0x00000004 )     /* UA1_IER        */
#define PMICOM0_UA1_IIR     ( UART1_BASE + 0x00000008 )     /* UA1_IIR        */
#define PMICOM0_UA1_CHAR    ( UART1_BASE + 0x0000000C )     /* UA1_CHAR       */
#define PMICOM0_UA1_FCR     ( UART1_BASE + 0x0000000C )     /* UA1_FCR        */
#define PMICOM0_UA1_LCR     ( UART1_BASE + 0x00000010 )     /* UA1_LCR        */
#define PMICOM0_UA1_MCR     ( UART1_BASE + 0x00000010 )     /* UA1_MCR        */
#define PMICOM0_UA1_LSR     ( UART1_BASE + 0x00000014 )     /* UA1_LSR        */
#define PMICOM0_UA1_MSR     ( UART1_BASE + 0x00000018 )     /* UA1_MSR        */
#define PMICOM0_UA1_DLR     ( UART1_BASE + 0x00000024 )     /* UA1_DLR        */


/*------- UART2          0xD5140000 ----------------------------------------*/
#define UART2_BASE          ( PUNIPHIER_BASE + 0x05140000 ) /* UART2          */

#define PMICOM0_UA2_RDR     ( UART2_BASE + 0x00000000 )     /* UA2_RDR        */
#define PMICOM0_UA2_TDR     ( UART2_BASE + 0x00000000 )     /* UA2_TDR        */
#define PMICOM0_UA2_IER     ( UART2_BASE + 0x00000004 )     /* UA2_IER        */
#define PMICOM0_UA2_IIR     ( UART2_BASE + 0x00000008 )     /* UA2_IIR        */
#define PMICOM0_UA2_CHAR    ( UART2_BASE + 0x0000000C )     /* UA2_CHAR       */
#define PMICOM0_UA2_FCR     ( UART2_BASE + 0x0000000C )     /* UA2_FCR        */
#define PMICOM0_UA2_LCR     ( UART2_BASE + 0x00000010 )     /* UA2_LCR        */
#define PMICOM0_UA2_MCR     ( UART2_BASE + 0x00000010 )     /* UA2_MCR        */
#define PMICOM0_UA2_LSR     ( UART2_BASE + 0x00000014 )     /* UA2_LSR        */
#define PMICOM0_UA2_MSR     ( UART2_BASE + 0x00000018 )     /* UA2_MSR        */
#define PMICOM0_UA2_DLR     ( UART2_BASE + 0x00000024 )     /* UA2_DLR        */

/*------- GPIO           0xD5150000 ----------------------------------------*/
#define GPIO_BASE           ( PUNIPHIER_BASE + 0x05150000 ) /* GPIO           */

#define PMICOM0_GPIOA0CTRL       ( GPIO_BASE + 0x00000000 ) /* GPIOA0CTRL     */
#define PMICOM0_GPIOA1CTRL       ( GPIO_BASE + 0x00000004 ) /* GPIOA1CTRL     */
#define PMICOM0_GPIOA2CTRL       ( GPIO_BASE + 0x00000008 ) /* GPIOA2CTRL     */
#define PMICOM0_GPIOA3CTRL       ( GPIO_BASE + 0x0000000C ) /* GPIOA3CTRL     */
#define PMICOM0_GPIOA4CTRL       ( GPIO_BASE + 0x00000010 ) /* GPIOA4CTRL     */
#define PMICOM0_GPIOA5CTRL       ( GPIO_BASE + 0x00000014 ) /* GPIOA5CTRL     */
#define PMICOM0_GPIOA6CTRL       ( GPIO_BASE + 0x00000018 ) /* GPIOA6CTRL     */
#define PMICOM0_GPIOA7CTRL       ( GPIO_BASE + 0x0000001C ) /* GPIOA7CTRL     */
#define PMICOM0_GPIOARIS         ( GPIO_BASE + 0x00000100 ) /* GPIOARIS       */
#define PMICOM0_GPIOAMIS_ARM     ( GPIO_BASE + 0x00000200 ) /* GPIOAMIS_ARM   */
#define PMICOM0_GPIOAMIS_IPP     ( GPIO_BASE + 0x00000300 ) /* GPIOAMIS_IPP   */
#define PMICOM0_GPIOA0IC         ( GPIO_BASE + 0x00000400 ) /* GPIOA0IC       */
#define PMICOM0_GPIOA1IC         ( GPIO_BASE + 0x00000404 ) /* GPIOA1IC       */
#define PMICOM0_GPIOA2IC         ( GPIO_BASE + 0x00000408 ) /* GPIOA2IC       */
#define PMICOM0_GPIOA3IC         ( GPIO_BASE + 0x0000040C ) /* GPIOA3IC       */
#define PMICOM0_GPIOA4IC         ( GPIO_BASE + 0x00000410 ) /* GPIOA4IC       */
#define PMICOM0_GPIOA5IC         ( GPIO_BASE + 0x00000414 ) /* GPIOA5IC       */
#define PMICOM0_GPIOA6IC         ( GPIO_BASE + 0x00000418 ) /* GPIOA6IC       */
#define PMICOM0_GPIOA7IC         ( GPIO_BASE + 0x0000041C ) /* GPIOA7IC       */
#define PMICOM0_GPIOA_BSSICINFO  ( GPIO_BASE + 0x00000500 ) /* GPIOA_BSSICINFO*/
#define PMICOM0_GPIOB0CTRL       ( GPIO_BASE + 0x00001000 ) /* GPIOB0CTRL     */
#define PMICOM0_GPIOB1CTRL       ( GPIO_BASE + 0x00001004 ) /* GPIOB1CTRL     */
#define PMICOM0_GPIOB2CTRL       ( GPIO_BASE + 0x00001008 ) /* GPIOB2CTRL     */
#define PMICOM0_GPIOB3CTRL       ( GPIO_BASE + 0x0000100C ) /* GPIOB3CTRL     */
#define PMICOM0_GPIOB4CTRL       ( GPIO_BASE + 0x00001010 ) /* GPIOB4CTRL     */
#define PMICOM0_GPIOB5CTRL       ( GPIO_BASE + 0x00001014 ) /* GPIOB5CTRL     */
#define PMICOM0_GPIOB6CTRL       ( GPIO_BASE + 0x00001018 ) /* GPIOB6CTRL     */
#define PMICOM0_GPIOB7CTRL       ( GPIO_BASE + 0x0000101C ) /* GPIOB7CTRL     */
#define PMICOM0_GPIOBRIS         ( GPIO_BASE + 0x00001100 ) /* GPIOBRIS       */
#define PMICOM0_GPIOBMIS_ARM     ( GPIO_BASE + 0x00001200 ) /* GPIOBMIS_ARM   */
#define PMICOM0_GPIOBMIS_IPP     ( GPIO_BASE + 0x00001300 ) /* GPIOBMIS_IPP   */
#define PMICOM0_GPIOB0IC         ( GPIO_BASE + 0x00001400 ) /* GPIOB0IC       */
#define PMICOM0_GPIOB1IC         ( GPIO_BASE + 0x00001404 ) /* GPIOB1IC       */
#define PMICOM0_GPIOB2IC         ( GPIO_BASE + 0x00001408 ) /* GPIOB2IC       */
#define PMICOM0_GPIOB3IC         ( GPIO_BASE + 0x0000140C ) /* GPIOB3IC       */
#define PMICOM0_GPIOB4IC         ( GPIO_BASE + 0x00001410 ) /* GPIOB4IC       */
#define PMICOM0_GPIOB5IC         ( GPIO_BASE + 0x00001414 ) /* GPIOB5IC       */
#define PMICOM0_GPIOB6IC         ( GPIO_BASE + 0x00001418 ) /* GPIOB6IC       */
#define PMICOM0_GPIOB7IC         ( GPIO_BASE + 0x0000141C ) /* GPIOB7IC       */
#define PMICOM0_GPIOC0CTRL       ( GPIO_BASE + 0x00002000 ) /* GPIOC0CTRL     */
#define PMICOM0_GPIOC1CTRL       ( GPIO_BASE + 0x00002004 ) /* GPIOC1CTRL     */
#define PMICOM0_GPIOC2CTRL       ( GPIO_BASE + 0x00002008 ) /* GPIOC2CTRL     */
#define PMICOM0_GPIOC3CTRL       ( GPIO_BASE + 0x0000200C ) /* GPIOC3CTRL     */
#define PMICOM0_GPIOC4CTRL       ( GPIO_BASE + 0x00002010 ) /* GPIOC4CTRL     */
#define PMICOM0_GPIOC5CTRL       ( GPIO_BASE + 0x00002014 ) /* GPIOC5CTRL     */
#define PMICOM0_GPIOC6CTRL       ( GPIO_BASE + 0x00002018 ) /* GPIOC6CTRL     */
#define PMICOM0_GPIOC7CTRL       ( GPIO_BASE + 0x0000201C ) /* GPIOC7CTRL     */
#define PMICOM0_GPIOCRIS         ( GPIO_BASE + 0x00002100 ) /* GPIOCRIS       */
#define PMICOM0_GPIOCMIS_ARM     ( GPIO_BASE + 0x00002200 ) /* GPIOCMIS_ARM   */
#define PMICOM0_GPIOCMIS_IPP     ( GPIO_BASE + 0x00002300 ) /* GPIOCMIS_IPP   */
#define PMICOM0_GPIOC0IC         ( GPIO_BASE + 0x00002400 ) /* GPIOC0IC       */
#define PMICOM0_GPIOC1IC         ( GPIO_BASE + 0x00002404 ) /* GPIOC1IC       */
#define PMICOM0_GPIOC2IC         ( GPIO_BASE + 0x00002408 ) /* GPIOC2IC       */
#define PMICOM0_GPIOC3IC         ( GPIO_BASE + 0x0000240C ) /* GPIOC3IC       */
#define PMICOM0_GPIOC4IC         ( GPIO_BASE + 0x00002410 ) /* GPIOC4IC       */
#define PMICOM0_GPIOC5IC         ( GPIO_BASE + 0x00002414 ) /* GPIOC5IC       */
#define PMICOM0_GPIOC6IC         ( GPIO_BASE + 0x00002418 ) /* GPIOC6IC       */
#define PMICOM0_GPIOC7IC         ( GPIO_BASE + 0x0000241C ) /* GPIOC7IC       */
#define PMICOM0_GPIOD0CTRL       ( GPIO_BASE + 0x00003000 ) /* GPIOD0CTRL     */
#define PMICOM0_GPIOD1CTRL       ( GPIO_BASE + 0x00003004 ) /* GPIOD1CTRL     */
#define PMICOM0_GPIOD2CTRL       ( GPIO_BASE + 0x00003008 ) /* GPIOD2CTRL     */
#define PMICOM0_GPIOD3CTRL       ( GPIO_BASE + 0x0000300C ) /* GPIOD3CTRL     */
#define PMICOM0_GPIOD4CTRL       ( GPIO_BASE + 0x00003010 ) /* GPIOD4CTRL     */
#define PMICOM0_GPIOD5CTRL       ( GPIO_BASE + 0x00003014 ) /* GPIOD5CTRL     */
#define PMICOM0_GPIOD6CTRL       ( GPIO_BASE + 0x00003018 ) /* GPIOD6CTRL     */
#define PMICOM0_GPIOD7CTRL       ( GPIO_BASE + 0x0000301C ) /* GPIOD7CTRL     */
#define PMICOM0_GPIOD0IC         ( GPIO_BASE + 0x00003400 ) /* GPIOD0IC       */
#define PMICOM0_GPIOD1IC         ( GPIO_BASE + 0x00003404 ) /* GPIOD1IC       */
#define PMICOM0_GPIOD2IC         ( GPIO_BASE + 0x00003408 ) /* GPIOD2IC       */
#define PMICOM0_GPIOD3IC         ( GPIO_BASE + 0x0000340C ) /* GPIOD3IC       */
#define PMICOM0_GPIOD4IC         ( GPIO_BASE + 0x00003410 ) /* GPIOD4IC       */
#define PMICOM0_GPIOD5IC         ( GPIO_BASE + 0x00003414 ) /* GPIOD5IC       */
#define PMICOM0_GPIOD6IC         ( GPIO_BASE + 0x00003418 ) /* GPIOD6IC       */
#define PMICOM0_GPIOD7IC         ( GPIO_BASE + 0x0000341C ) /* GPIOD7IC       */
#define PMICOM0_GPIOE0CTRL       ( GPIO_BASE + 0x00004000 ) /* GPIOE0CTRL     */
#define PMICOM0_GPIOE1CTRL       ( GPIO_BASE + 0x00004004 ) /* GPIOE1CTRL     */
#define PMICOM0_GPIOE2CTRL       ( GPIO_BASE + 0x00004008 ) /* GPIOE2CTRL     */
#define PMICOM0_GPIOE3CTRL       ( GPIO_BASE + 0x0000400C ) /* GPIOE3CTRL     */
#define PMICOM0_GPIOE4CTRL       ( GPIO_BASE + 0x00004010 ) /* GPIOE4CTRL     */
#define PMICOM0_GPIOE5CTRL       ( GPIO_BASE + 0x00004014 ) /* GPIOE5CTRL     */
#define PMICOM0_GPIOE6CTRL       ( GPIO_BASE + 0x00004018 ) /* GPIOE6CTRL     */
#define PMICOM0_GPIOE7CTRL       ( GPIO_BASE + 0x0000401C ) /* GPIOE7CTRL     */
#define PMICOM0_GPIOF0CTRL       ( GPIO_BASE + 0x00005000 ) /* GPIOF0CTRL     */
#define PMICOM0_GPIOF1CTRL       ( GPIO_BASE + 0x00005004 ) /* GPIOF1CTRL     */
#define PMICOM0_GPIOF2CTRL       ( GPIO_BASE + 0x00005008 ) /* GPIOF2CTRL     */
#define PMICOM0_GPIOF3CTRL       ( GPIO_BASE + 0x0000500C ) /* GPIOF3CTRL     */
#define PMICOM0_GPIOF4CTRL       ( GPIO_BASE + 0x00005010 ) /* GPIOF4CTRL     */
#define PMICOM0_GPIOF5CTRL       ( GPIO_BASE + 0x00005014 ) /* GPIOF5CTRL     */
#define PMICOM0_GPIOF6CTRL       ( GPIO_BASE + 0x00005018 ) /* GPIOF6CTRL     */
#define PMICOM0_GPIOF7CTRL       ( GPIO_BASE + 0x0000501C ) /* GPIOF7CTRL     */


/*------- SSI1(2ch)      0xD5180000 ----------------------------------------*/
#define SSI1_BASE           ( PUNIPHIER_BASE + 0x05180000 ) /* SSI1           */

#define PMICOM0_SSIACTL     ( SSI1_BASE + 0x00000000 )      /* SSIACTL        */
#define PMICOM0_SSIACKS     ( SSI1_BASE + 0x00000004 )      /* SSIACKS        */
#define PMICOM0_SSIATXWDS   ( SSI1_BASE + 0x00000008 )      /* SSIATXWDS      */
#define PMICOM0_SSIARXWDS   ( SSI1_BASE + 0x0000000C )      /* SSIARXWDS      */
#define PMICOM0_SSIAFPS     ( SSI1_BASE + 0x00000010 )      /* SSIAFPS        */
#define PMICOM0_SSIAIE      ( SSI1_BASE + 0x00000014 )      /* SSIAIE         */
#define PMICOM0_SSIASR      ( SSI1_BASE + 0x00000018 )      /* SSIASR         */
#define PMICOM0_SSIAICR     ( SSI1_BASE + 0x00000018 )      /* SSIAICR        */
#define PMICOM0_SSIAPCTL    ( SSI1_BASE + 0x0000001C )      /* SSIAPCTL       */
#define PMICOM0_SSIATXDR    ( SSI1_BASE + 0x00000024 )      /* SSIATXDR       */
#define PMICOM0_SSIARXDR    ( SSI1_BASE + 0x00000024 )      /* SSIARXDR       */
#define PMICOM0_SSIBCTL     ( SSI1_BASE + 0x00000100 )      /* SSIBCTL        */
#define PMICOM0_SSIBCKS     ( SSI1_BASE + 0x00000104 )      /* SSIBCKS        */
#define PMICOM0_SSIBTXWDS   ( SSI1_BASE + 0x00000108 )      /* SSIBTXWDS      */
#define PMICOM0_SSIBRXWDS   ( SSI1_BASE + 0x0000010C )      /* SSIBRXWDS      */
#define PMICOM0_SSIBFPS     ( SSI1_BASE + 0x00000110 )      /* SSIBFPS        */
#define PMICOM0_SSIBIE      ( SSI1_BASE + 0x00000114 )      /* SSIBIE         */
#define PMICOM0_SSIBSR      ( SSI1_BASE + 0x00000118 )      /* SSIBSR         */
#define PMICOM0_SSIBICR     ( SSI1_BASE + 0x00000118 )      /* SSIBICR        */
#define PMICOM0_SSIBPCTL    ( SSI1_BASE + 0x0000011C )      /* SSIBPCTL       */
#define PMICOM0_SSIBEXR     ( SSI1_BASE + 0x00000120 )      /* SSIBEXR        */
#define PMICOM0_SSIBTXDR    ( SSI1_BASE + 0x00000124 )      /* SSIBTXDR       */
#define PMICOM0_SSIBRXDR    ( SSI1_BASE + 0x00000124 )      /* SSIBRXDR       */
#define PMICOM0_SSI1WTR     ( SSI1_BASE + 0x00001000 )      /* SSI1WTR        */


/*------- Backup Mem/Reg 0xD5190000 ----------------------------------------*/
#define BACKUP_BASE         ( PUNIPHIER_BASE + 0x05190000 ) /* BACKUP         */

#define PMICOM0_MBC0R       ( BACKUP_BASE + 0x00001020 )    /* MBC0R          */
#define PMICOM0_MBC1R       ( BACKUP_BASE + 0x00001024 )    /* MBC1R          */
#define PMICOM0_MBC2R       ( BACKUP_BASE + 0x00001028 )    /* MBC2R          */
#define PMICOM0_MBC3R       ( BACKUP_BASE + 0x0000102C )    /* MBC3R          */
#define PMICOM0_MBC4R       ( BACKUP_BASE + 0x00001030 )    /* MBC4R          */
#define PMICOM0_MBC5R       ( BACKUP_BASE + 0x00001034 )    /* MBC5R          */
#define PMICOM0_EMC0R       ( BACKUP_BASE + 0x00001038 )    /* EMC0R          */
#define PMICOM0_EMP0R       ( BACKUP_BASE + 0x0000103C )    /* EMP0R          */
#define PMICOM0_CSA1R       ( BACKUP_BASE + 0x00001040 )    /* CSA1R          */
#define PMICOM0_CSA2R       ( BACKUP_BASE + 0x00001044 )    /* CSA2R          */
#define PMICOM0_CSA3R       ( BACKUP_BASE + 0x00001048 )    /* CSA3R          */
#define PMICOM0_CSA4R       ( BACKUP_BASE + 0x0000104C )    /* CSA4R          */
#define PMICOM0_CSA5R       ( BACKUP_BASE + 0x00001050 )    /* CSA5R          */
#define PMICOM0_RSTAR0R     ( BACKUP_BASE + 0x00001054 )    /* RSTAR0R        */
#define PMICOM0_WSTAR0R     ( BACKUP_BASE + 0x0000105C )    /* WSTAR0R        */
#define PMICOM0_RDTAR0R     ( BACKUP_BASE + 0x00001064 )    /* RDTAR0R        */
#define PMICOM0_WDTAR0R     ( BACKUP_BASE + 0x0000106C )    /* WDTAR0R        */
#define PMICOM0_RAST0R      ( BACKUP_BASE + 0x00001074 )    /* RAST0R         */
#define PMICOM0_WAST0R      ( BACKUP_BASE + 0x00001078 )    /* WAST0R         */
#define PMICOM0_PAST0R      ( BACKUP_BASE + 0x0000107C )    /* PAST0R         */
#define PMICOM0_RSET0R      ( BACKUP_BASE + 0x00001080 )    /* RSET0R         */
#define PMICOM0_WSET0R      ( BACKUP_BASE + 0x00001084 )    /* WSET0R         */
#define PMICOM0_RHLD0R      ( BACKUP_BASE + 0x00001088 )    /* RHLD0R         */
#define PMICOM0_WHLD0R      ( BACKUP_BASE + 0x0000108C )    /* WHLD0R         */
#define PMICOM0_AAST0R      ( BACKUP_BASE + 0x00001090 )    /* AAST0R         */
#define PMICOM0_ASET0R      ( BACKUP_BASE + 0x00001094 )    /* ASET0R         */
#define PMICOM0_AHLD0R      ( BACKUP_BASE + 0x00001098 )    /* AHLD0R         */
#define PMICOM0_RWA0R       ( BACKUP_BASE + 0x000010A4 )    /* RWA0R          */
#define PMICOM0_WWA0R       ( BACKUP_BASE + 0x000010A8 )    /* WWA0R          */
#define PMICOM0_SDMSLF0     ( BACKUP_BASE + 0x00001100 )    /* SDMSLF0        */
#define PMICOM0_SDMSLF1     ( BACKUP_BASE + 0x00001104 )    /* SDMSLF1        */
#define PMICOM0_TDDSSTDBY   ( BACKUP_BASE + 0x00001200 )    /* TDDSSTDBY      */
#define PMICOM0_USB_SET     ( BACKUP_BASE + 0x00001300 )    /* USB_SET        */
#define PMICOM0_PRBSL       ( BACKUP_BASE + 0x00001400 )    /* PRBSL          */
#define PMICOM0_DBGPDCTL    ( BACKUP_BASE + 0x00001404 )    /* DBGPDCTL       */
#define PMICOM0_I2CIOSL     ( BACKUP_BASE + 0x00001600 )    /* I2CIOSL        */
#define PMICOM0_IOAFSELREG  ( BACKUP_BASE + 0x00002000 )    /* IOAFSELREG     */
#define PMICOM0_IOIDMSKREG  ( BACKUP_BASE + 0x00002010 )    /* IOIDMSKREG     */
#define PMICOM0_IODRVREG0   ( BACKUP_BASE + 0x00002100 )    /* IODRVREG0      */
#define PMICOM0_IODRVREG1   ( BACKUP_BASE + 0x00002104 )    /* IODRVREG1      */
#define PMICOM0_IODRVREG2   ( BACKUP_BASE + 0x00002108 )    /* IODRVREG2      */
#define PMICOM0_IOPUPDREG0  ( BACKUP_BASE + 0x00002200 )    /* IOPUPDREG0     */
#define PMICOM0_IOPUPDREG1  ( BACKUP_BASE + 0x00002204 )    /* IOPUPDREG1     */
#define PMICOM0_IOPUPDREG2  ( BACKUP_BASE + 0x00002208 )    /* IOPUPDREG2     */
#define PMICOM0_BLPUCNTR    ( BACKUP_BASE + 0x00002210 )    /* BLPUCNTR       */
#define PMICOM0_BLPUSETR    ( BACKUP_BASE + 0x00002214 )    /* BLPUSETR       */
#define PMICOM0_SDRPUCNTR   ( BACKUP_BASE + 0x00002220 )    /* SDRPUCNTR      */
#define PMICOM0_SDRPUSETR   ( BACKUP_BASE + 0x00002224 )    /* SDRPUSETR      */
#define PMICOM0_IOAOOSELREG ( BACKUP_BASE + 0x00002300 )    /* IOAOOSELREG    */
#define PMICOM0_IOAODIRREG  ( BACKUP_BASE + 0x00002310 )    /* IOAODIRREG     */
#define PMICOM0_IOAOOREG    ( BACKUP_BASE + 0x00002320 )    /* IOAOOREG       */
#define PMICOM0_IOCODIRREG0 ( BACKUP_BASE + 0x00002400 )    /* IOCODIRREG0    */
#define PMICOM0_IOCODIRREG1 ( BACKUP_BASE + 0x00002404 )    /* IOCODIRREG1    */
#define PMICOM0_IOCOOREG0   ( BACKUP_BASE + 0x00002410 )    /* IOCOOREG0      */
#define PMICOM0_IOCOOREG1   ( BACKUP_BASE + 0x00002414 )    /* IOCOOREG1      */
#define PMICOM0_IOIECTLREG  ( BACKUP_BASE + 0x00002500 )    /* IOIECTLREG     */


/*============================================================================*/
/* Micom-1  Block   0xD6000000                                              */
/*============================================================================*/

/*------- ARMDMAC        0xD6000000 ----------------------------------------*/
#define ARMDMAC_BASE        ( PUNIPHIER_BASE + 0x06000000 ) /* ARMDMAC        */

#define PMICOM1_ADMA0CTR    ( ARMDMAC_BASE + 0x00000000 )   /* ADMA0CTR       */
#define PMICOM1_ADMA0SRC    ( ARMDMAC_BASE + 0x00000004 )   /* ADMA0SRC       */
#define PMICOM1_ADMA0DST    ( ARMDMAC_BASE + 0x00000008 )   /* ADMA0DST       */
#define PMICOM1_ADMA0NUM    ( ARMDMAC_BASE + 0x0000000C )   /* ADMA0NUM       */
#define PMICOM1_ADMA0CNT    ( ARMDMAC_BASE + 0x00000010 )   /* ADMA0CNT       */
#define PMICOM1_ADMA0CYC    ( ARMDMAC_BASE + 0x00000014 )   /* ADMA0CYC       */
#define PMICOM1_ADMA1CTR    ( ARMDMAC_BASE + 0x00000020 )   /* ADMA1CTR       */
#define PMICOM1_ADMA1SRC    ( ARMDMAC_BASE + 0x00000024 )   /* ADMA1SRC       */
#define PMICOM1_ADMA1DST    ( ARMDMAC_BASE + 0x00000028 )   /* ADMA1DST       */
#define PMICOM1_ADMA1NUM    ( ARMDMAC_BASE + 0x0000002C )   /* ADMA1NUM       */
#define PMICOM1_ADMA1CNT    ( ARMDMAC_BASE + 0x00000030 )   /* ADMA1CNT       */
#define PMICOM1_ADMA1CYC    ( ARMDMAC_BASE + 0x00000034 )   /* ADMA1CYC       */
#define PMICOM1_ADMA2CTR    ( ARMDMAC_BASE + 0x00000040 )   /* ADMA2CTR       */
#define PMICOM1_ADMA2SRC    ( ARMDMAC_BASE + 0x00000044 )   /* ADMA2SRC       */
#define PMICOM1_ADMA2DST    ( ARMDMAC_BASE + 0x00000048 )   /* ADMA2DST       */
#define PMICOM1_ADMA2NUM    ( ARMDMAC_BASE + 0x0000004C )   /* ADMA2NUM       */
#define PMICOM1_ADMA2CNT    ( ARMDMAC_BASE + 0x00000050 )   /* ADMA2CNT       */
#define PMICOM1_ADMA2CYC    ( ARMDMAC_BASE + 0x00000054 )   /* ADMA2CYC       */
#define PMICOM1_ADMA3CTR    ( ARMDMAC_BASE + 0x00000060 )   /* ADMA3CTR       */
#define PMICOM1_ADMA3SRC    ( ARMDMAC_BASE + 0x00000064 )   /* ADMA3SRC       */
#define PMICOM1_ADMA3DST    ( ARMDMAC_BASE + 0x00000068 )   /* ADMA3DST       */
#define PMICOM1_ADMA3NUM    ( ARMDMAC_BASE + 0x0000006C )   /* ADMA3NUM       */
#define PMICOM1_ADMA3CNT    ( ARMDMAC_BASE + 0x00000070 )   /* ADMA3CNT       */
#define PMICOM1_ADMA3CYC    ( ARMDMAC_BASE + 0x00000074 )   /* ADMA3CYC       */
#define PMICOM1_ADMA4CTR    ( ARMDMAC_BASE + 0x00000080 )   /* ADMA4CTR       */
#define PMICOM1_ADMA4SRC    ( ARMDMAC_BASE + 0x00000084 )   /* ADMA4SRC       */
#define PMICOM1_ADMA4DST    ( ARMDMAC_BASE + 0x00000088 )   /* ADMA4DST       */
#define PMICOM1_ADMA4NUM    ( ARMDMAC_BASE + 0x0000008C )   /* ADMA4NUM       */
#define PMICOM1_ADMA4CNT    ( ARMDMAC_BASE + 0x00000090 )   /* ADMA4CNT       */
#define PMICOM1_ADMA4CYC    ( ARMDMAC_BASE + 0x00000094 )   /* ADMA4CYC       */
#define PMICOM1_ADMA5CTR    ( ARMDMAC_BASE + 0x000000A0 )   /* ADMA5CTR       */
#define PMICOM1_ADMA5SRC    ( ARMDMAC_BASE + 0x000000A4 )   /* ADMA5SRC       */
#define PMICOM1_ADMA5DST    ( ARMDMAC_BASE + 0x000000A8 )   /* ADMA5DST       */
#define PMICOM1_ADMA5NUM    ( ARMDMAC_BASE + 0x000000AC )   /* ADMA5NUM       */
#define PMICOM1_ADMA5CNT    ( ARMDMAC_BASE + 0x000000B0 )   /* ADMA5CNT       */
#define PMICOM1_ADMA5CYC    ( ARMDMAC_BASE + 0x000000B4 )   /* ADMA5CYC       */
#define PMICOM1_ADMA6CTR    ( ARMDMAC_BASE + 0x000000C0 )   /* ADMA6CTR       */
#define PMICOM1_ADMA6SRC    ( ARMDMAC_BASE + 0x000000C4 )   /* ADMA6SRC       */
#define PMICOM1_ADMA6DST    ( ARMDMAC_BASE + 0x000000C8 )   /* ADMA6DST       */
#define PMICOM1_ADMA6NUM    ( ARMDMAC_BASE + 0x000000CC )   /* ADMA6NUM       */
#define PMICOM1_ADMA6CNT    ( ARMDMAC_BASE + 0x000000D0 )   /* ADMA6CNT       */
#define PMICOM1_ADMA6CYC    ( ARMDMAC_BASE + 0x000000D4 )   /* ADMA6CYC       */
#define PMICOM1_ADMA7CTR    ( ARMDMAC_BASE + 0x000000E0 )   /* ADMA7CTR       */
#define PMICOM1_ADMA7SRC    ( ARMDMAC_BASE + 0x000000E4 )   /* ADMA7SRC       */
#define PMICOM1_ADMA7DST    ( ARMDMAC_BASE + 0x000000E8 )   /* ADMA7DST       */
#define PMICOM1_ADMA7NUM    ( ARMDMAC_BASE + 0x000000EC )   /* ADMA7NUM       */
#define PMICOM1_ADMA7CNT    ( ARMDMAC_BASE + 0x000000F0 )   /* ADMA7CNT       */
#define PMICOM1_ADMA7CYC    ( ARMDMAC_BASE + 0x000000F4 )   /* ADMA7CYC       */
#define PMICOM1_ADMA8CTR    ( ARMDMAC_BASE + 0x00000100 )   /* ADMA8CTR       */
#define PMICOM1_ADMA8SRC    ( ARMDMAC_BASE + 0x00000104 )   /* ADMA8SRC       */
#define PMICOM1_ADMA8DST    ( ARMDMAC_BASE + 0x00000108 )   /* ADMA8DST       */
#define PMICOM1_ADMA8NUM    ( ARMDMAC_BASE + 0x0000010C )   /* ADMA8NUM       */
#define PMICOM1_ADMA8CNT    ( ARMDMAC_BASE + 0x00000110 )   /* ADMA8CNT       */
#define PMICOM1_ADMA8CYC    ( ARMDMAC_BASE + 0x00000114 )   /* ADMA8CYC       */
#define PMICOM1_ADMA9CTR    ( ARMDMAC_BASE + 0x00000120 )   /* ADMA9CTR       */
#define PMICOM1_ADMA9SRC    ( ARMDMAC_BASE + 0x00000124 )   /* ADMA9SRC       */
#define PMICOM1_ADMA9DST    ( ARMDMAC_BASE + 0x00000128 )   /* ADMA9DST       */
#define PMICOM1_ADMA9NUM    ( ARMDMAC_BASE + 0x0000012C )   /* ADMA9NUM       */
#define PMICOM1_ADMA9CNT    ( ARMDMAC_BASE + 0x00000130 )   /* ADMA9CNT       */
#define PMICOM1_ADMA9CYC    ( ARMDMAC_BASE + 0x00000134 )   /* ADMA9CYC       */
#define PMICOM1_ADMA10CTR   ( ARMDMAC_BASE + 0x00000140 )   /* ADMA10CTR      */
#define PMICOM1_ADMA10SRC   ( ARMDMAC_BASE + 0x00000144 )   /* ADMA10SRC      */
#define PMICOM1_ADMA10DST   ( ARMDMAC_BASE + 0x00000148 )   /* ADMA10DST      */
#define PMICOM1_ADMA10NUM   ( ARMDMAC_BASE + 0x0000014C )   /* ADMA10NUM      */
#define PMICOM1_ADMA10CNT   ( ARMDMAC_BASE + 0x00000150 )   /* ADMA10CNT      */
#define PMICOM1_ADMA10CYC   ( ARMDMAC_BASE + 0x00000154 )   /* ADMA10CYC      */
#define PMICOM1_ADMA11CTR   ( ARMDMAC_BASE + 0x00000160 )   /* ADMA11CTR      */
#define PMICOM1_ADMA11SRC   ( ARMDMAC_BASE + 0x00000164 )   /* ADMA11SRC      */
#define PMICOM1_ADMA11DST   ( ARMDMAC_BASE + 0x00000168 )   /* ADMA11DST      */
#define PMICOM1_ADMA11NUM   ( ARMDMAC_BASE + 0x0000016C )   /* ADMA11NUM      */
#define PMICOM1_ADMA11CNT   ( ARMDMAC_BASE + 0x00000170 )   /* ADMA11CNT      */
#define PMICOM1_ADMA11CYC   ( ARMDMAC_BASE + 0x00000174 )   /* ADMA11CYC      */
#define PMICOM1_ARMASQN0    ( ARMDMAC_BASE + 0x00000400 )   /* ARMASQN0       */
#define PMICOM1_ARMASQA0    ( ARMDMAC_BASE + 0x00000404 )   /* ARMASQA0       */
#define PMICOM1_ARMASQN1    ( ARMDMAC_BASE + 0x00000410 )   /* ARMASQN1       */
#define PMICOM1_ARMASQA1    ( ARMDMAC_BASE + 0x00000414 )   /* ARMASQA1       */
#define PMICOM1_ARMASQN2    ( ARMDMAC_BASE + 0x00000420 )   /* ARMASQN2       */
#define PMICOM1_ARMASQA2    ( ARMDMAC_BASE + 0x00000424 )   /* ARMASQA2       */
#define PMICOM1_ARMASQN3    ( ARMDMAC_BASE + 0x00000430 )   /* ARMASQN3       */
#define PMICOM1_ARMASQA3    ( ARMDMAC_BASE + 0x00000434 )   /* ARMASQA3       */
#define PMICOM1_ARMASQN4    ( ARMDMAC_BASE + 0x00000440 )   /* ARMASQN4       */
#define PMICOM1_ARMASQA4    ( ARMDMAC_BASE + 0x00000444 )   /* ARMASQA4       */
#define PMICOM1_ARMASQN5    ( ARMDMAC_BASE + 0x00000450 )   /* ARMASQN5       */
#define PMICOM1_ARMASQA5    ( ARMDMAC_BASE + 0x00000454 )   /* ARMASQA5       */
#define PMICOM1_ARMASQN6    ( ARMDMAC_BASE + 0x00000460 )   /* ARMASQN6       */
#define PMICOM1_ARMASQA6    ( ARMDMAC_BASE + 0x00000464 )   /* ARMASQA6       */
#define PMICOM1_ARMASQN7    ( ARMDMAC_BASE + 0x00000470 )   /* ARMASQN7       */
#define PMICOM1_ARMASQA7    ( ARMDMAC_BASE + 0x00000474 )   /* ARMASQA7       */
#define PMICOM1_ARMASQN8    ( ARMDMAC_BASE + 0x00000480 )   /* ARMASQN8       */
#define PMICOM1_ARMASQA8    ( ARMDMAC_BASE + 0x00000484 )   /* ARMASQA8       */
#define PMICOM1_ARMASQN9    ( ARMDMAC_BASE + 0x00000490 )   /* ARMASQN9       */
#define PMICOM1_ARMASQA9    ( ARMDMAC_BASE + 0x00000494 )   /* ARMASQA9       */
#define PMICOM1_ARMASQN10   ( ARMDMAC_BASE + 0x000004A0 )   /* ARMASQN10      */
#define PMICOM1_ARMASQA10   ( ARMDMAC_BASE + 0x000004A4 )   /* ARMASQA10      */
#define PMICOM1_ARMASQN11   ( ARMDMAC_BASE + 0x000004B0 )   /* ARMASQN11      */
#define PMICOM1_ARMASQA11   ( ARMDMAC_BASE + 0x000004B4 )   /* ARMASQA11      */
#define PMICOM1_ADMAMCTR    ( ARMDMAC_BASE + 0x00000500 )   /* ADMAMCTR       */
#define PMICOM1_ADMAENDI    ( ARMDMAC_BASE + 0x00000504 )   /* ADMAENDI       */
#define PMICOM1_ADMAERRI    ( ARMDMAC_BASE + 0x00000508 )   /* ADMAERRI       */
#define PMICOM1_ADMASTA     ( ARMDMAC_BASE + 0x0000050C )   /* ADMASTA        */


/*------- IrDA-UART      0xD6010000 ----------------------------------------*/
#define IRDAUART_BASE       ( PUNIPHIER_BASE + 0x06010000 ) /* IrDA-UART      */

#define PMICOM1_UA5_RDR     ( IRDAUART_BASE + 0x00000000 )  /* UA5_RDR        */
#define PMICOM1_UA5_TDR     ( IRDAUART_BASE + 0x00000000 )  /* UA5_TDR        */
#define PMICOM1_UA5_IER     ( IRDAUART_BASE + 0x00000004 )  /* UA5_IER        */
#define PMICOM1_UA5_IIR     ( IRDAUART_BASE + 0x00000008 )  /* UA5_IIR        */
#define PMICOM1_UA5_CHAR    ( IRDAUART_BASE + 0x0000000C )  /* UA5_CHAR       */
#define PMICOM1_UA5_FCR     ( IRDAUART_BASE + 0x0000000C )  /* UA5_FCR        */
#define PMICOM1_UA5_LCR     ( IRDAUART_BASE + 0x00000010 )  /* UA5_LCR        */
#define PMICOM1_UA5_MCR     ( IRDAUART_BASE + 0x00000010 )  /* UA5_MCR        */
#define PMICOM1_UA5_LSR     ( IRDAUART_BASE + 0x00000014 )  /* UA5_LSR        */
#define PMICOM1_UA5_MSR     ( IRDAUART_BASE + 0x00000018 )  /* UA5_MSR        */
#define PMICOM1_UA5_DLR     ( IRDAUART_BASE + 0x00000024 )  /* UA5_DLR        */
#define PMICOM1_UA5_IRDA    ( IRDAUART_BASE + 0x00000040 )  /* UA5_IRDA       */


/*------- I2C            0xD6020000 ----------------------------------------*/
#define I2C_BASE            ( PUNIPHIER_BASE + 0x06020000 ) /* I2C            */

#define PMICOM1_I2CCR       ( I2C_BASE + 0x00000000 )       /* I2CCR          */
#define PMICOM1_I2CDTTX     ( I2C_BASE + 0x00000004 )       /* I2CDTTX        */
#define PMICOM1_I2CDTRX     ( I2C_BASE + 0x00000004 )       /* I2CDTRX        */
#define PMICOM1_I2CBC       ( I2C_BASE + 0x00000008 )       /* I2CBC          */
#define PMICOM1_I2CSLAD     ( I2C_BASE + 0x0000000C )       /* I2CSLAD        */
#define PMICOM1_I2CCYC      ( I2C_BASE + 0x00000010 )       /* I2CCYC         */
#define PMICOM1_I2CLCTL     ( I2C_BASE + 0x00000014 )       /* I2CLCTL        */
#define PMICOM1_I2CSSUT     ( I2C_BASE + 0x00000018 )       /* I2CSSUT        */
#define PMICOM1_I2CDSUT     ( I2C_BASE + 0x0000001C )       /* I2CDSUT        */
#define PMICOM1_I2CINT      ( I2C_BASE + 0x00000020 )       /* I2CINT         */
#define PMICOM1_I2CIE       ( I2C_BASE + 0x00000024 )       /* I2CIE          */
#define PMICOM1_I2CIC       ( I2C_BASE + 0x00000028 )       /* I2CIC          */
#define PMICOM1_I2CSR       ( I2C_BASE + 0x0000002C )       /* I2CSR          */
#define PMICOM1_I2CBCMON    ( I2C_BASE + 0x00000030 )       /* I2CBCMON       */
#define PMICOM1_I2CRST      ( I2C_BASE + 0x00000034 )       /* I2CRST         */
#define PMICOM1_I2CBM       ( I2C_BASE + 0x00000038 )       /* I2CBM          */


/*------- BCU (Register) 0xD6100000 ----------------------------------------*/
#define BCU_BASE            ( PUNIPHIER_BASE + 0x06100000 ) /* BCU            */

#define PMICOM1_EMC1R       ( BCU_BASE + 0x00000104 )       /* EMC1R          */
#define PMICOM1_EMC2R       ( BCU_BASE + 0x00000108 )       /* EMC2R          */
#define PMICOM1_EMC3R       ( BCU_BASE + 0x0000010C )       /* EMC3R          */
#define PMICOM1_EMC4R       ( BCU_BASE + 0x00000110 )       /* EMC4R          */
#define PMICOM1_EMC5R       ( BCU_BASE + 0x00000114 )       /* EMC5R          */
#define PMICOM1_EMP1R       ( BCU_BASE + 0x00000204 )       /* EMP1R          */
#define PMICOM1_EMP2R       ( BCU_BASE + 0x00000208 )       /* EMP2R          */
#define PMICOM1_EMP3R       ( BCU_BASE + 0x0000020C )       /* EMP3R          */
#define PMICOM1_EMP4R       ( BCU_BASE + 0x00000210 )       /* EMP4R          */
#define PMICOM1_EMP5R       ( BCU_BASE + 0x00000214 )       /* EMP5R          */
#define PMICOM1_RSTAR1R     ( BCU_BASE + 0x00000404 )       /* RSTAR1R        */
#define PMICOM1_RSTAR2R     ( BCU_BASE + 0x00000408 )       /* RSTAR2R        */
#define PMICOM1_RSTAR3R     ( BCU_BASE + 0x0000040C )       /* RSTAR3R        */
#define PMICOM1_RSTAR4R     ( BCU_BASE + 0x00000410 )       /* RSTAR4R        */
#define PMICOM1_RSTAR5R     ( BCU_BASE + 0x00000414 )       /* RSTAR5R        */
#define PMICOM1_WSTAR1R     ( BCU_BASE + 0x00000604 )       /* WSTAR1R        */
#define PMICOM1_WSTAR2R     ( BCU_BASE + 0x00000608 )       /* WSTAR2R        */
#define PMICOM1_WSTAR3R     ( BCU_BASE + 0x0000060C )       /* WSTAR3R        */
#define PMICOM1_WSTAR4R     ( BCU_BASE + 0x00000610 )       /* WSTAR4R        */
#define PMICOM1_WSTAR5R     ( BCU_BASE + 0x00000614 )       /* WSTAR5R        */
#define PMICOM1_RDTAR1R     ( BCU_BASE + 0x00000804 )       /* RDTAR1R        */
#define PMICOM1_RDTAR2R     ( BCU_BASE + 0x00000808 )       /* RDTAR2R        */
#define PMICOM1_RDTAR3R     ( BCU_BASE + 0x0000080C )       /* RDTAR3R        */
#define PMICOM1_RDTAR4R     ( BCU_BASE + 0x00000810 )       /* RDTAR4R        */
#define PMICOM1_RDTAR5R     ( BCU_BASE + 0x00000814 )       /* RDTAR5R        */
#define PMICOM1_WDTAR1R     ( BCU_BASE + 0x00000A04 )       /* WDTAR1R        */
#define PMICOM1_WDTAR2R     ( BCU_BASE + 0x00000A08 )       /* WDTAR2R        */
#define PMICOM1_WDTAR3R     ( BCU_BASE + 0x00000A0C )       /* WDTAR3R        */
#define PMICOM1_WDTAR4R     ( BCU_BASE + 0x00000A10 )       /* WDTAR4R        */
#define PMICOM1_WDTAR5R     ( BCU_BASE + 0x00000A14 )       /* WDTAR5R        */
#define PMICOM1_RAST1R      ( BCU_BASE + 0x00001004 )       /* RAST1R         */
#define PMICOM1_RAST2R      ( BCU_BASE + 0x00001008 )       /* RAST2R         */
#define PMICOM1_RAST3R      ( BCU_BASE + 0x0000100C )       /* RAST3R         */
#define PMICOM1_RAST4R      ( BCU_BASE + 0x00001010 )       /* RAST4R         */
#define PMICOM1_RAST5R      ( BCU_BASE + 0x00001014 )       /* RAST5R         */
#define PMICOM1_WAST1R      ( BCU_BASE + 0x00001104 )       /* WAST1R         */
#define PMICOM1_WAST2R      ( BCU_BASE + 0x00001108 )       /* WAST2R         */
#define PMICOM1_WAST3R      ( BCU_BASE + 0x0000110C )       /* WAST3R         */
#define PMICOM1_WAST4R      ( BCU_BASE + 0x00001110 )       /* WAST4R         */
#define PMICOM1_WAST5R      ( BCU_BASE + 0x00001114 )       /* WAST5R         */
#define PMICOM1_PAST1R      ( BCU_BASE + 0x00001204 )       /* PAST1R         */
#define PMICOM1_PAST2R      ( BCU_BASE + 0x00001208 )       /* PAST2R         */
#define PMICOM1_PAST3R      ( BCU_BASE + 0x0000120C )       /* PAST3R         */
#define PMICOM1_PAST4R      ( BCU_BASE + 0x00001210 )       /* PAST4R         */
#define PMICOM1_PAST5R      ( BCU_BASE + 0x00001214 )       /* PAST5R         */
#define PMICOM1_RSET1R      ( BCU_BASE + 0x00001304 )       /* RSET1R         */
#define PMICOM1_RSET2R      ( BCU_BASE + 0x00001308 )       /* RSET2R         */
#define PMICOM1_RSET3R      ( BCU_BASE + 0x0000130C )       /* RSET3R         */
#define PMICOM1_RSET4R      ( BCU_BASE + 0x00001310 )       /* RSET4R         */
#define PMICOM1_RSET5R      ( BCU_BASE + 0x00001314 )       /* RSET5R         */
#define PMICOM1_WSET1R      ( BCU_BASE + 0x00001404 )       /* WSET1R         */
#define PMICOM1_WSET2R      ( BCU_BASE + 0x00001408 )       /* WSET2R         */
#define PMICOM1_WSET3R      ( BCU_BASE + 0x0000140C )       /* WSET3R         */
#define PMICOM1_WSET4R      ( BCU_BASE + 0x00001410 )       /* WSET4R         */
#define PMICOM1_WSET5R      ( BCU_BASE + 0x00001414 )       /* WSET5R         */
#define PMICOM1_RHLD1R      ( BCU_BASE + 0x00001504 )       /* RHLD1R         */
#define PMICOM1_RHLD2R      ( BCU_BASE + 0x00001508 )       /* RHLD2R         */
#define PMICOM1_RHLD3R      ( BCU_BASE + 0x0000150C )       /* RHLD3R         */
#define PMICOM1_RHLD4R      ( BCU_BASE + 0x00001510 )       /* RHLD4R         */
#define PMICOM1_RHLD5R      ( BCU_BASE + 0x00001514 )       /* RHLD5R         */
#define PMICOM1_WHLD1R      ( BCU_BASE + 0x00001604 )       /* WHLD1R         */
#define PMICOM1_WHLD2R      ( BCU_BASE + 0x00001608 )       /* WHLD2R         */
#define PMICOM1_WHLD3R      ( BCU_BASE + 0x0000160C )       /* WHLD3R         */
#define PMICOM1_WHLD4R      ( BCU_BASE + 0x00001610 )       /* WHLD4R         */
#define PMICOM1_WHLD5R      ( BCU_BASE + 0x00001614 )       /* WHLD5R         */
#define PMICOM1_AAST1R      ( BCU_BASE + 0x00002004 )       /* AAST1R         */
#define PMICOM1_AAST2R      ( BCU_BASE + 0x00002008 )       /* AAST2R         */
#define PMICOM1_AAST3R      ( BCU_BASE + 0x0000200C )       /* AAST3R         */
#define PMICOM1_AAST4R      ( BCU_BASE + 0x00002010 )       /* AAST4R         */
#define PMICOM1_AAST5R      ( BCU_BASE + 0x00002014 )       /* AAST5R         */
#define PMICOM1_ASET1R      ( BCU_BASE + 0x00002104 )       /* ASET1R         */
#define PMICOM1_ASET2R      ( BCU_BASE + 0x00002108 )       /* ASET2R         */
#define PMICOM1_ASET3R      ( BCU_BASE + 0x0000210C )       /* ASET3R         */
#define PMICOM1_ASET4R      ( BCU_BASE + 0x00002110 )       /* ASET4R         */
#define PMICOM1_ASET5R      ( BCU_BASE + 0x00002114 )       /* ASET5R         */
#define PMICOM1_AHLD1R      ( BCU_BASE + 0x00002204 )       /* AHLD1R         */
#define PMICOM1_AHLD2R      ( BCU_BASE + 0x00002208 )       /* AHLD2R         */
#define PMICOM1_AHLD3R      ( BCU_BASE + 0x0000220C )       /* AHLD3R         */
#define PMICOM1_AHLD4R      ( BCU_BASE + 0x00002210 )       /* AHLD4R         */
#define PMICOM1_AHLD5R      ( BCU_BASE + 0x00002214 )       /* AHLD5R         */
#define PMICOM1_RWA1R       ( BCU_BASE + 0x00003004 )       /* RWA1R          */
#define PMICOM1_RWA2R       ( BCU_BASE + 0x00003008 )       /* RWA2R          */
#define PMICOM1_RWA3R       ( BCU_BASE + 0x0000300C )       /* RWA3R          */
#define PMICOM1_RWA4R       ( BCU_BASE + 0x00003010 )       /* RWA4R          */
#define PMICOM1_RWA5R       ( BCU_BASE + 0x00003014 )       /* RWA5R          */
#define PMICOM1_WWA1R       ( BCU_BASE + 0x00003104 )       /* WWA1R          */
#define PMICOM1_WWA2R       ( BCU_BASE + 0x00003108 )       /* WWA2R          */
#define PMICOM1_WWA3R       ( BCU_BASE + 0x0000310C )       /* WWA3R          */
#define PMICOM1_WWA4R       ( BCU_BASE + 0x00003110 )       /* WWA4R          */
#define PMICOM1_WWA5R       ( BCU_BASE + 0x00003114 )       /* WWA5R          */
#define PMICOM1_EWSR        ( BCU_BASE + 0x00003200 )       /* EWSR           */
#define PMICOM1_CWTMR       ( BCU_BASE + 0x00003300 )       /* CWTMR          */
#define PMICOM1_MBST0R      ( BCU_BASE + 0x00004000 )       /* MBST0R         */
#define PMICOM1_MBST1R      ( BCU_BASE + 0x00004004 )       /* MBST1R         */
#define PMICOM1_MBST2R      ( BCU_BASE + 0x00004008 )       /* MBST2R         */
#define PMICOM1_MBST3R      ( BCU_BASE + 0x0000400C )       /* MBST3R         */
#define PMICOM1_MBST4R      ( BCU_BASE + 0x00004010 )       /* MBST4R         */
#define PMICOM1_MBST5R      ( BCU_BASE + 0x00004014 )       /* MBST5R         */
#define PMICOM1_INTSTR      ( BCU_BASE + 0x00004100 )       /* INTSTR         */
#define PMICOM1_INTENR      ( BCU_BASE + 0x00004200 )       /* INTENR         */
#define PMICOM1_LWTEAR      ( BCU_BASE + 0x00004300 )       /* LWTEAR         */
#define PMICOM1_UWTEAR      ( BCU_BASE + 0x00004400 )       /* UWTEAR         */


/*------- UART3          0xD6130000 ----------------------------------------*/
#define UART3_BASE          ( PUNIPHIER_BASE + 0x06130000 ) /* UART3          */

#define PMICOM1_UA3_RDR     ( UART3_BASE + 0x00000000 )     /* UA3_RDR        */
#define PMICOM1_UA3_TDR     ( UART3_BASE + 0x00000000 )     /* UA3_TDR        */
#define PMICOM1_UA3_IER     ( UART3_BASE + 0x00000004 )     /* UA3_IER        */
#define PMICOM1_UA3_IIR     ( UART3_BASE + 0x00000008 )     /* UA3_IIR        */
#define PMICOM1_UA3_CHAR    ( UART3_BASE + 0x0000000C )     /* UA3_CHAR       */
#define PMICOM1_UA3_FCR     ( UART3_BASE + 0x0000000C )     /* UA3_FCR        */
#define PMICOM1_UA3_LCR     ( UART3_BASE + 0x00000010 )     /* UA3_LCR        */
#define PMICOM1_UA3_MCR     ( UART3_BASE + 0x00000010 )     /* UA3_MCR        */
#define PMICOM1_UA3_LSR     ( UART3_BASE + 0x00000014 )     /* UA3_LSR        */
#define PMICOM1_UA3_MSR     ( UART3_BASE + 0x00000018 )     /* UA3_MSR        */
#define PMICOM1_UA3_DLR     ( UART3_BASE + 0x00000024 )     /* UA3_DLR        */


/*------- UART4          0xD6140000 ----------------------------------------*/
#define UART4_BASE          ( PUNIPHIER_BASE + 0x06140000 ) /* UART4          */

#define PMICOM1_UA4_RDR     ( UART4_BASE + 0x00000000 )     /* UA4_RDR        */
#define PMICOM1_UA4_TDR     ( UART4_BASE + 0x00000000 )     /* UA4_TDR        */
#define PMICOM1_UA4_IER     ( UART4_BASE + 0x00000004 )     /* UA4_IER        */
#define PMICOM1_UA4_IIR     ( UART4_BASE + 0x00000008 )     /* UA4_IIR        */
#define PMICOM1_UA4_CHAR    ( UART4_BASE + 0x0000000C )     /* UA4_CHAR       */
#define PMICOM1_UA4_FCR     ( UART4_BASE + 0x0000000C )     /* UA4_FCR        */
#define PMICOM1_UA4_LCR     ( UART4_BASE + 0x00000010 )     /* UA4_LCR        */
#define PMICOM1_UA4_MCR     ( UART4_BASE + 0x00000010 )     /* UA4_MCR        */
#define PMICOM1_UA4_LSR     ( UART4_BASE + 0x00000014 )     /* UA4_LSR        */
#define PMICOM1_UA4_MSR     ( UART4_BASE + 0x00000018 )     /* UA4_MSR        */
#define PMICOM1_UA4_DLR     ( UART4_BASE + 0x00000024 )     /* UA4_DLR        */


/*------- SSI2(6ch)      0xD6180000 ----------------------------------------*/
#define SSI2_BASE           ( PUNIPHIER_BASE + 0x06180000 ) /* SSI2           */

#define PMICOM1_SSICCTL     ( SSI2_BASE + 0x00000000 )      /* SSICCTL        */
#define PMICOM1_SSICCKS     ( SSI2_BASE + 0x00000004 )      /* SSICCKS        */
#define PMICOM1_SSICTXWDS   ( SSI2_BASE + 0x00000008 )      /* SSICTXWDS      */
#define PMICOM1_SSICRXWDS   ( SSI2_BASE + 0x0000000C )      /* SSICRXWDS      */
#define PMICOM1_SSICFPS     ( SSI2_BASE + 0x00000010 )      /* SSICFPS        */
#define PMICOM1_SSICIE      ( SSI2_BASE + 0x00000014 )      /* SSICIE         */
#define PMICOM1_SSICSR      ( SSI2_BASE + 0x00000018 )      /* SSICSR         */
#define PMICOM1_SSICICR     ( SSI2_BASE + 0x00000018 )      /* SSICICR        */
#define PMICOM1_SSICPCTL    ( SSI2_BASE + 0x0000001C )      /* SSICPCTL       */
#define PMICOM1_SSICTXDR    ( SSI2_BASE + 0x00000024 )      /* SSICTXDR       */
#define PMICOM1_SSICRXDR    ( SSI2_BASE + 0x00000024 )      /* SSICRXDR       */
#define PMICOM1_SSIDCTL     ( SSI2_BASE + 0x00000100 )      /* SSIDCTL        */
#define PMICOM1_SSIDCKS     ( SSI2_BASE + 0x00000104 )      /* SSIDCKS        */
#define PMICOM1_SSIDTXWDS   ( SSI2_BASE + 0x00000108 )      /* SSIDTXWDS      */
#define PMICOM1_SSIDRXWDS   ( SSI2_BASE + 0x0000010C )      /* SSIDRXWDS      */
#define PMICOM1_SSIDFPS     ( SSI2_BASE + 0x00000110 )      /* SSIDFPS        */
#define PMICOM1_SSIDIE      ( SSI2_BASE + 0x00000114 )      /* SSIDIE         */
#define PMICOM1_SSIDSR      ( SSI2_BASE + 0x00000118 )      /* SSIDSR         */
#define PMICOM1_SSIDICR     ( SSI2_BASE + 0x00000118 )      /* SSIDICR        */
#define PMICOM1_SSIDPCTL    ( SSI2_BASE + 0x0000011C )      /* SSIDPCTL       */
#define PMICOM1_SSIDTXDR    ( SSI2_BASE + 0x00000124 )      /* SSIDTXDR       */
#define PMICOM1_SSIDRXDR    ( SSI2_BASE + 0x00000124 )      /* SSIDRXDR       */
#define PMICOM1_SSIECTL     ( SSI2_BASE + 0x00000200 )      /* SSIECTL        */
#define PMICOM1_SSIECKS     ( SSI2_BASE + 0x00000204 )      /* SSIECKS        */
#define PMICOM1_SSIETXWDS   ( SSI2_BASE + 0x00000208 )      /* SSIETXWDS      */
#define PMICOM1_SSIERXWDS   ( SSI2_BASE + 0x0000020C )      /* SSIERXWDS      */
#define PMICOM1_SSIEFPS     ( SSI2_BASE + 0x00000210 )      /* SSIEFPS        */
#define PMICOM1_SSIEIE      ( SSI2_BASE + 0x00000214 )      /* SSIEIE         */
#define PMICOM1_SSIESR      ( SSI2_BASE + 0x00000218 )      /* SSIESR         */
#define PMICOM1_SSIEICR     ( SSI2_BASE + 0x00000218 )      /* SSIEICR        */
#define PMICOM1_SSIETXDR    ( SSI2_BASE + 0x00000224 )      /* SSIETXDR       */
#define PMICOM1_SSIERXDR    ( SSI2_BASE + 0x00000224 )      /* SSIERXDR       */
#define PMICOM1_SSIFCTL     ( SSI2_BASE + 0x00000300 )      /* SSIFCTL        */
#define PMICOM1_SSIFCKS     ( SSI2_BASE + 0x00000304 )      /* SSIFCKS        */
#define PMICOM1_SSIFTXWDS   ( SSI2_BASE + 0x00000308 )      /* SSIFTXWDS      */
#define PMICOM1_SSIFRXWDS   ( SSI2_BASE + 0x0000030C )      /* SSIFRXWDS      */
#define PMICOM1_SSIFFPS     ( SSI2_BASE + 0x00000310 )      /* SSIFFPS        */
#define PMICOM1_SSIFIE      ( SSI2_BASE + 0x00000314 )      /* SSIFIE         */
#define PMICOM1_SSIFSR      ( SSI2_BASE + 0x00000318 )      /* SSIFSR         */
#define PMICOM1_SSIFICR     ( SSI2_BASE + 0x00000318 )      /* SSIFICR        */
#define PMICOM1_SSIFTXDR    ( SSI2_BASE + 0x00000324 )      /* SSIFTXDR       */
#define PMICOM1_SSIFRXDR    ( SSI2_BASE + 0x00000324 )      /* SSIFRXDR       */
#define PMICOM1_SSIGCTL     ( SSI2_BASE + 0x00000400 )      /* SSIGCTL        */
#define PMICOM1_SSIGCKS     ( SSI2_BASE + 0x00000404 )      /* SSIGCKS        */
#define PMICOM1_SSIGTXWDS   ( SSI2_BASE + 0x00000408 )      /* SSIGTXWDS      */
#define PMICOM1_SSIGRXWDS   ( SSI2_BASE + 0x0000040C )      /* SSIGRXWDS      */
#define PMICOM1_SSIGFPS     ( SSI2_BASE + 0x00000410 )      /* SSIGFPS        */
#define PMICOM1_SSIGIE      ( SSI2_BASE + 0x00000414 )      /* SSIGIE         */
#define PMICOM1_SSIGSR      ( SSI2_BASE + 0x00000418 )      /* SSIGSR         */
#define PMICOM1_SSIGICR     ( SSI2_BASE + 0x00000418 )      /* SSIGICR        */
#define PMICOM1_SSIGTXDR    ( SSI2_BASE + 0x00000424 )      /* SSIGTXDR       */
#define PMICOM1_SSIGRXDR    ( SSI2_BASE + 0x00000424 )      /* SSIGRXDR       */
#define PMICOM1_SSIHCTL     ( SSI2_BASE + 0x00000500 )      /* SSIHCTL        */
#define PMICOM1_SSIHCKS     ( SSI2_BASE + 0x00000504 )      /* SSIHCKS        */
#define PMICOM1_SSIHTXWDS   ( SSI2_BASE + 0x00000508 )      /* SSIHTXWDS      */
#define PMICOM1_SSIHRXWDS   ( SSI2_BASE + 0x0000050C )      /* SSIHRXWDS      */
#define PMICOM1_SSIHFPS     ( SSI2_BASE + 0x00000510 )      /* SSIHFPS        */
#define PMICOM1_SSIHIE      ( SSI2_BASE + 0x00000514 )      /* SSIHIE         */
#define PMICOM1_SSIHSR      ( SSI2_BASE + 0x00000518 )      /* SSIHSR         */
#define PMICOM1_SSIHICR     ( SSI2_BASE + 0x00000518 )      /* SSIHICR        */
#define PMICOM1_SSIHTXDR    ( SSI2_BASE + 0x00000524 )      /* SSIHTXDR       */
#define PMICOM1_SSIHRXDR    ( SSI2_BASE + 0x00000524 )      /* SSIHRXDR       */
#define PMICOM1_SSI2WTR     ( SSI2_BASE + 0x00001000 )      /* SSI2WTR        */


/*------- Scheduler      0xD6190000 ----------------------------------------*/
#define SCHEDULER_BASE      ( PUNIPHIER_BASE + 0x06190000 ) /* Scheduler      */

#define PMICOM1_ABTSLT0     ( SCHEDULER_BASE + 0x00000000 ) /* ABTSLT0        */
#define PMICOM1_ABTSLT1     ( SCHEDULER_BASE + 0x00000004 ) /* ABTSLT1        */
#define PMICOM1_ABTSLT2     ( SCHEDULER_BASE + 0x00000008 ) /* ABTSLT2        */
#define PMICOM1_ABTSLT3     ( SCHEDULER_BASE + 0x0000000C ) /* ABTSLT3        */
#define PMICOM1_ABTSLT4     ( SCHEDULER_BASE + 0x00000010 ) /* ABTSLT4        */
#define PMICOM1_ABTSLT5     ( SCHEDULER_BASE + 0x00000014 ) /* ABTSLT5        */
#define PMICOM1_ABTSLT6     ( SCHEDULER_BASE + 0x00000018 ) /* ABTSLT6        */
#define PMICOM1_ABTSLT7     ( SCHEDULER_BASE + 0x0000001C ) /* ABTSLT7        */
#define PMICOM1_ABTSLT8     ( SCHEDULER_BASE + 0x00000020 ) /* ABTSLT8        */
#define PMICOM1_ABTSLT9     ( SCHEDULER_BASE + 0x00000024 ) /* ABTSLT9        */
#define PMICOM1_ABTSLT10    ( SCHEDULER_BASE + 0x00000028 ) /* ABTSLT10       */
#define PMICOM1_ABTSLT11    ( SCHEDULER_BASE + 0x0000002C ) /* ABTSLT11       */
#define PMICOM1_ABTSLT12    ( SCHEDULER_BASE + 0x00000030 ) /* ABTSLT12       */
#define PMICOM1_ABTSLT13    ( SCHEDULER_BASE + 0x00000034 ) /* ABTSLT13       */
#define PMICOM1_ABTSLT14    ( SCHEDULER_BASE + 0x00000038 ) /* ABTSLT14       */
#define PMICOM1_ABTSLT15    ( SCHEDULER_BASE + 0x0000003C ) /* ABTSLT15       */
#define PMICOM1_ABTSLT16    ( SCHEDULER_BASE + 0x00000040 ) /* ABTSLT16       */
#define PMICOM1_ABTSLT17    ( SCHEDULER_BASE + 0x00000044 ) /* ABTSLT17       */
#define PMICOM1_ABTSLT18    ( SCHEDULER_BASE + 0x00000048 ) /* ABTSLT18       */
#define PMICOM1_ABTSLT19    ( SCHEDULER_BASE + 0x0000004C ) /* ABTSLT19       */
#define PMICOM1_ABTSLT20    ( SCHEDULER_BASE + 0x00000050 ) /* ABTSLT20       */
#define PMICOM1_ABTSLT21    ( SCHEDULER_BASE + 0x00000054 ) /* ABTSLT21       */
#define PMICOM1_ABTSLT22    ( SCHEDULER_BASE + 0x00000058 ) /* ABTSLT22       */
#define PMICOM1_ABTSLT23    ( SCHEDULER_BASE + 0x0000005C ) /* ABTSLT23       */
#define PMICOM1_ABTSLT24    ( SCHEDULER_BASE + 0x00000060 ) /* ABTSLT24       */
#define PMICOM1_ABTSLT25    ( SCHEDULER_BASE + 0x00000064 ) /* ABTSLT25       */
#define PMICOM1_ABTSLT26    ( SCHEDULER_BASE + 0x00000068 ) /* ABTSLT26       */
#define PMICOM1_ABTSLT27    ( SCHEDULER_BASE + 0x0000006C ) /* ABTSLT27       */
#define PMICOM1_ABTSLT28    ( SCHEDULER_BASE + 0x00000070 ) /* ABTSLT28       */
#define PMICOM1_ABTSLT29    ( SCHEDULER_BASE + 0x00000074 ) /* ABTSLT29       */
#define PMICOM1_ABTSLT30    ( SCHEDULER_BASE + 0x00000078 ) /* ABTSLT30       */
#define PMICOM1_ABTSLT31    ( SCHEDULER_BASE + 0x0000007C ) /* ABTSLT31       */
#define PMICOM1_ABTCNT      ( SCHEDULER_BASE + 0x00000080 ) /* ABTCNT         */
#define PMICOM1_SDMCNF      ( SCHEDULER_BASE + 0x00000084 ) /* SDMCNF         */
#define PMICOM1_SDMMREF     ( SCHEDULER_BASE + 0x00000088 ) /* SDMMREF        */
#define PMICOM1_SDMAREF0    ( SCHEDULER_BASE + 0x0000008C ) /* SDMAREF0       */
#define PMICOM1_SDMAREF1    ( SCHEDULER_BASE + 0x00000090 ) /* SDMAREF1       */
#define PMICOM1_ABTMON0     ( SCHEDULER_BASE + 0x00000094 ) /* ABTMON0        */
#define PMICOM1_ABTMON1     ( SCHEDULER_BASE + 0x00000098 ) /* ABTMON1        */
#define PMICOM1_ABTMON2     ( SCHEDULER_BASE + 0x0000009C ) /* ABTMON2        */
#define PMICOM1_ABTMON3     ( SCHEDULER_BASE + 0x000000A0 ) /* ABTMON3        */
#define PMICOM1_ABTMON4     ( SCHEDULER_BASE + 0x000000A4 ) /* ABTMON4        */
#define PMICOM1_ABTMON5     ( SCHEDULER_BASE + 0x000000A8 ) /* ABTMON5        */
#define PMICOM1_ABTMON6     ( SCHEDULER_BASE + 0x000000AC ) /* ABTMON6        */
#define PMICOM1_ABTMON7     ( SCHEDULER_BASE + 0x000000B0 ) /* ABTMON7        */
#define PMICOM1_ABTMON8     ( SCHEDULER_BASE + 0x000000B4 ) /* ABTMON8        */
#define PMICOM1_ABTMON9     ( SCHEDULER_BASE + 0x000000B8 ) /* ABTMON9        */
#define PMICOM1_ABTMON10    ( SCHEDULER_BASE + 0x000000BC ) /* ABTMON10       */
#define PMICOM1_ABTMON11    ( SCHEDULER_BASE + 0x000000C0 ) /* ABTMON11       */
#define PMICOM1_ABTMON12    ( SCHEDULER_BASE + 0x000000C4 ) /* ABTMON12       */
#define PMICOM1_ABTMON13    ( SCHEDULER_BASE + 0x000000C8 ) /* ABTMON13       */
#define PMICOM1_ABTMON14    ( SCHEDULER_BASE + 0x000000CC ) /* ABTMON14       */
#define PMICOM1_ABTMON15    ( SCHEDULER_BASE + 0x000000D0 ) /* ABTMON15       */
#define PMICOM1_ABTRMON0    ( SCHEDULER_BASE + 0x000000D4 ) /* ABTRMON0       */
#define PMICOM1_ABTRMON1    ( SCHEDULER_BASE + 0x000000D8 ) /* ABTRMON1       */
#define PMICOM1_ABTRMON2    ( SCHEDULER_BASE + 0x000000DC ) /* ABTRMON2       */
#define PMICOM1_ABTRMON3    ( SCHEDULER_BASE + 0x000000E0 ) /* ABTRMON3       */
#define PMICOM1_ABTRMON4    ( SCHEDULER_BASE + 0x000000E4 ) /* ABTRMON4       */
#define PMICOM1_ABTRMON5    ( SCHEDULER_BASE + 0x000000E8 ) /* ABTRMON5       */
#define PMICOM1_ABTRMON6    ( SCHEDULER_BASE + 0x000000EC ) /* ABTRMON6       */
#define PMICOM1_ABTRMON7    ( SCHEDULER_BASE + 0x000000F0 ) /* ABTRMON7       */
#define PMICOM1_ABTRMON8    ( SCHEDULER_BASE + 0x000000F4 ) /* ABTRMON8       */
#define PMICOM1_ABTRMON9    ( SCHEDULER_BASE + 0x000000F8 ) /* ABTRMON9       */
#define PMICOM1_ABTRMON10   ( SCHEDULER_BASE + 0x000000FC ) /* ABTRMON10      */
#define PMICOM1_ABTRMON11   ( SCHEDULER_BASE + 0x00000100 ) /* ABTRMON11      */
#define PMICOM1_ABTRMON12   ( SCHEDULER_BASE + 0x00000104 ) /* ABTRMON12      */
#define PMICOM1_ABTRMON13   ( SCHEDULER_BASE + 0x00000108 ) /* ABTRMON13      */
#define PMICOM1_ABTRMON14   ( SCHEDULER_BASE + 0x0000010C ) /* ABTRMON14      */
#define PMICOM1_ABTRMON15   ( SCHEDULER_BASE + 0x00000110 ) /* ABTRMON15      */
#define PMICOM1_ABTSET0     ( SCHEDULER_BASE + 0x00000114 ) /* ABTSET0        */
#define PMICOM1_ABTSET1     ( SCHEDULER_BASE + 0x00000118 ) /* ABTSET1        */
#define PMICOM1_ABTSET2     ( SCHEDULER_BASE + 0x0000011C ) /* ABTSET2        */
#define PMICOM1_ABTSET3     ( SCHEDULER_BASE + 0x00000120 ) /* ABTSET3        */
#define PMICOM1_ABTSET4     ( SCHEDULER_BASE + 0x00000124 ) /* ABTSET4        */
#define PMICOM1_ABTSET5     ( SCHEDULER_BASE + 0x00000128 ) /* ABTSET5        */
#define PMICOM1_ABTSET6     ( SCHEDULER_BASE + 0x0000012C ) /* ABTSET6        */
#define PMICOM1_ABTSET7     ( SCHEDULER_BASE + 0x00000130 ) /* ABTSET7        */
#define PMICOM1_ABTSET8     ( SCHEDULER_BASE + 0x00000134 ) /* ABTSET8        */
#define PMICOM1_ABTSET9     ( SCHEDULER_BASE + 0x00000138 ) /* ABTSET9        */
#define PMICOM1_ABTSET10    ( SCHEDULER_BASE + 0x0000013C ) /* ABTSET10       */
#define PMICOM1_ABTSET11    ( SCHEDULER_BASE + 0x00000140 ) /* ABTSET11       */
#define PMICOM1_ABTSET12    ( SCHEDULER_BASE + 0x00000144 ) /* ABTSET12       */
#define PMICOM1_LATLAYER0   ( SCHEDULER_BASE + 0x00000168 ) /* LATLAYER0      */
#define PMICOM1_LATSLOT0    ( SCHEDULER_BASE + 0x0000016C ) /* LATSLOT0       */
#define PMICOM1_LATLAYER1   ( SCHEDULER_BASE + 0x00000170 ) /* LATLAYER1      */
#define PMICOM1_LATSLOT1    ( SCHEDULER_BASE + 0x00000174 ) /* LATSLOT1       */
#define PMICOM1_LATLAYER2   ( SCHEDULER_BASE + 0x00000178 ) /* LATLAYER2      */
#define PMICOM1_LATSLOT2    ( SCHEDULER_BASE + 0x0000017C ) /* LATSLOT2       */
#define PMICOM1_CMDHILEV    ( SCHEDULER_BASE + 0x00000180 ) /* CMDHILEV       */
#define PMICOM1_CMDLOLEV    ( SCHEDULER_BASE + 0x00000184 ) /* CMDLOLEV       */
#define PMICOM1_CMDHICYC    ( SCHEDULER_BASE + 0x00000188 ) /* CMDHICYC       */
#define PMICOM1_CMDLOCYC    ( SCHEDULER_BASE + 0x0000018C ) /* CMDLOCYC       */
#define PMICOM1_SDMMODE     ( SCHEDULER_BASE + 0x00000190 ) /* SDMMODE        */
#define PMICOM1_SDMEXM0     ( SCHEDULER_BASE + 0x00000194 ) /* SDMEXM0        */
#define PMICOM1_SDMEXM1     ( SCHEDULER_BASE + 0x00000198 ) /* SDMEXM1        */
#define PMICOM1_SMDEC       ( SCHEDULER_BASE + 0x0000019C ) /* SMDEC          */
#define PMICOM1_SMDEUA      ( SCHEDULER_BASE + 0x000001A0 ) /* SMDEUA         */
#define PMICOM1_SMDELA      ( SCHEDULER_BASE + 0x000001A4 ) /* SMDELA         */
#define PMICOM1_SDMINT      ( SCHEDULER_BASE + 0x000001A8 ) /* SDMINT         */
#define PMICOM1_SDMRNT      ( SCHEDULER_BASE + 0x000001AC ) /* SDMRNT         */
#define PMICOM1_SDMCKE      ( SCHEDULER_BASE + 0x000001B0 ) /* SDMCKE         */
#define PMICOM1_SMDAC       ( SCHEDULER_BASE + 0x000001B4 ) /* SMDAC          */
#define PMICOM1_SMDAUA      ( SCHEDULER_BASE + 0x000001B8 ) /* SMDAUA         */
#define PMICOM1_SMDALA      ( SCHEDULER_BASE + 0x000001BC ) /* SMDALA         */
#define PMICOM1_BMSLCT      ( SCHEDULER_BASE + 0x000001C0 ) /* BMSLCT         */
#define PMICOM1_SDMAPE      ( SCHEDULER_BASE + 0x000001C4 ) /* SDMAPE         */
#define PMICOM1_SDMDLY      ( SCHEDULER_BASE + 0x000001C8 ) /* SDMDLY         */
#define PMICOM1_SDMMONEN    ( SCHEDULER_BASE + 0x000001CC ) /* SDMMONEN       */
#define PMICOM1_DQSCAL      ( SCHEDULER_BASE + 0x000001D0 ) /* DQSCAL         */
#define PMICOM1_MCBCANC     ( SCHEDULER_BASE + 0x000001D4 ) /* MCBCANC        */
#define PMICOM1_CALEXP      ( SCHEDULER_BASE + 0x000001D8 ) /* CALEXP         */
#define PMICOM1_DBCNT       ( SCHEDULER_BASE + 0x00000200 ) /* DBCNT          */


/*------- TDDS           0xD61A0000 ----------------------------------------*/
#define TDDS_BASE           ( PUNIPHIER_BASE + 0x061A0000 ) /* TDDS           */

#define PMICOM1_TD_SYSC     ( TDDS_BASE + 0x00000000 )      /* TD_SYSC        */
#define PMICOM1_TD_BUFC     ( TDDS_BASE + 0x00000004 )      /* TD_BUFC        */
#define PMICOM1_TD_SYSSTAT  ( TDDS_BASE + 0x00000008 )      /* TD_SYSSTAT     */
#define PMICOM1_TD_IPPIM0   ( TDDS_BASE + 0x00000010 )      /* TD_IPPIM0      */
#define PMICOM1_TD_IPPIM1   ( TDDS_BASE + 0x00000014 )      /* TD_IPPIM1      */
#define PMICOM1_TD_IPPIS0   ( TDDS_BASE + 0x00000018 )      /* TD_IPPIS0      */
#define PMICOM1_TD_IPPIS1   ( TDDS_BASE + 0x0000001C )      /* TD_IPPIS1      */
#define PMICOM1_TD_ARMIM0   ( TDDS_BASE + 0x00000020 )      /* TD_ARMIM0      */
#define PMICOM1_TD_ARMIM1   ( TDDS_BASE + 0x00000024 )      /* TD_ARMIM1      */
#define PMICOM1_TD_ARMIS0   ( TDDS_BASE + 0x00000028 )      /* TD_ARMIS0      */
#define PMICOM1_TD_ARMIS1   ( TDDS_BASE + 0x0000002C )      /* TD_ARMIS1      */
#define PMICOM1_TD_SYNCS    ( TDDS_BASE + 0x00000030 )      /* TD_SYNCS       */
#define PMICOM1_TD_DCPIDM   ( TDDS_BASE + 0x00000038 )      /* TD_DCPIDM      */
#define PMICOM1_TD_SCBED    ( TDDS_BASE + 0x0000003C )      /* TD_SCBED       */
#define PMICOM1_TD_DSC      ( TDDS_BASE + 0x00000040 )      /* TD_DSC         */
#define PMICOM1_TD_MDSKEY0  ( TDDS_BASE + 0x00000044 )      /* TD_MDSKEY0     */
#define PMICOM1_TD_MDSKEY1  ( TDDS_BASE + 0x00000048 )      /* TD_MDSKEY1     */
#define PMICOM1_TD_KYWR     ( TDDS_BASE + 0x0000004C )      /* TD_KYWR        */
#define PMICOM1_TD_CKC      ( TDDS_BASE + 0x00000060 )      /* TD_CKC         */
#define PMICOM1_TD_PWMOS    ( TDDS_BASE + 0x00000064 )      /* TD_PWMOS       */
#define PMICOM1_TD_PCRM0    ( TDDS_BASE + 0x00000068 )      /* TD_PCRM0       */
#define PMICOM1_TD_PCRM1    ( TDDS_BASE + 0x0000006C )      /* TD_PCRM1       */
#define PMICOM1_TD_PCRM2    ( TDDS_BASE + 0x00000070 )      /* TD_PCRM2       */
#define PMICOM1_TD_STCM0    ( TDDS_BASE + 0x00000074 )      /* TD_STCM0       */
#define PMICOM1_TD_STCM1    ( TDDS_BASE + 0x00000078 )      /* TD_STCM1       */
#define PMICOM1_TD_STCM2    ( TDDS_BASE + 0x0000007C )      /* TD_STCM2       */
#define PMICOM1_TD_CSTC0    ( TDDS_BASE + 0x00000080 )      /* TD_CSTC0       */
#define PMICOM1_TD_CSTC1    ( TDDS_BASE + 0x00000084 )      /* TD_CSTC1       */
#define PMICOM1_TD_CSTC2    ( TDDS_BASE + 0x00000088 )      /* TD_CSTC2       */
#define PMICOM1_TD_TIMNS0   ( TDDS_BASE + 0x0000008C )      /* TD_TIMNS0      */
#define PMICOM1_TD_TIMNS1   ( TDDS_BASE + 0x00000090 )      /* TD_TIMNS1      */
#define PMICOM1_TD_TIMNS2   ( TDDS_BASE + 0x00000094 )      /* TD_TIMNS2      */
#define PMICOM1_TD_RMSA     ( TDDS_BASE + 0x000000A0 )      /* TD_RMSA        */
#define PMICOM1_TD_RMLS     ( TDDS_BASE + 0x000000A4 )      /* TD_RMLS        */
#define PMICOM1_TD_RMRD     ( TDDS_BASE + 0x000000A8 )      /* TD_RMRD        */
#define PMICOM1_TD_SBSA     ( TDDS_BASE + 0x000000C0 )      /* TD_SBSA        */
#define PMICOM1_TD_SBLS     ( TDDS_BASE + 0x000000C4 )      /* TD_SBLS        */
#define PMICOM1_TD_SBRD     ( TDDS_BASE + 0x000000C8 )      /* TD_SBRD        */
#define PMICOM1_TD_SBRC0    ( TDDS_BASE + 0x000000CC )      /* TD_SBRC0       */
#define PMICOM1_TD_SBRC1    ( TDDS_BASE + 0x000000D0 )      /* TD_SBRC1       */
#define PMICOM1_TD_SBINTS   ( TDDS_BASE + 0x000000D4 )      /* TD_SBINTS      */
#define PMICOM1_TD_DSS0_3   ( TDDS_BASE + 0x000005D0 )      /* TD_DSS0_3      */
#define PMICOM1_TD_DSS4_7   ( TDDS_BASE + 0x000005D4 )      /* TD_DSS4_7      */
#define PMICOM1_TD_DSS8_11  ( TDDS_BASE + 0x000005D8 )      /* TD_DSS8_11     */
#define PMICOM1_TD_DSS12_15 ( TDDS_BASE + 0x000005DC )      /* TD_DSS12_15    */
#define PMICOM1_TD_DSS16_19 ( TDDS_BASE + 0x000005E0 )      /* TD_DSS16_19    */
#define PMICOM1_TD_DSS20_23 ( TDDS_BASE + 0x000005E4 )      /* TD_DSS20_23    */
#define PMICOM1_TD_DSS24_27 ( TDDS_BASE + 0x000005E8 )      /* TD_DSS24_27    */
#define PMICOM1_TD_DSS28    ( TDDS_BASE + 0x000005EC )      /* TD_DSS28       */
#define PMICOM1_TD_SPPID0   ( TDDS_BASE + 0x00000600 )      /* TD_SPPID0      */
#define PMICOM1_TD_SPPID1   ( TDDS_BASE + 0x00000604 )      /* TD_SPPID1      */
#define PMICOM1_TD_SPPID2   ( TDDS_BASE + 0x00000608 )      /* TD_SPPID2      */
#define PMICOM1_TD_SPPID3   ( TDDS_BASE + 0x0000060C )      /* TD_SPPID3      */
#define PMICOM1_TD_SPPID4   ( TDDS_BASE + 0x00000610 )      /* TD_SPPID4      */
#define PMICOM1_TD_SPPID5   ( TDDS_BASE + 0x00000614 )      /* TD_SPPID5      */
#define PMICOM1_TD_SPPID6   ( TDDS_BASE + 0x00000618 )      /* TD_SPPID6      */
#define PMICOM1_TD_SPPID7   ( TDDS_BASE + 0x0000061C )      /* TD_SPPID7      */
#define PMICOM1_TD_SPPID8   ( TDDS_BASE + 0x00000620 )      /* TD_SPPID8      */
#define PMICOM1_TD_SPPID9   ( TDDS_BASE + 0x00000624 )      /* TD_SPPID9      */
#define PMICOM1_TD_SPPID10  ( TDDS_BASE + 0x00000628 )      /* TD_SPPID10     */
#define PMICOM1_TD_SPPID11  ( TDDS_BASE + 0x0000062C )      /* TD_SPPID11     */
#define PMICOM1_TD_SPPID12  ( TDDS_BASE + 0x00000630 )      /* TD_SPPID12     */
#define PMICOM1_TD_SPPID13  ( TDDS_BASE + 0x00000634 )      /* TD_SPPID13     */
#define PMICOM1_TD_SPPID14  ( TDDS_BASE + 0x00000638 )      /* TD_SPPID14     */
#define PMICOM1_TD_SPPID15  ( TDDS_BASE + 0x0000063C )      /* TD_SPPID15     */
#define PMICOM1_TD_SPPID16  ( TDDS_BASE + 0x00000640 )      /* TD_SPPID16     */
#define PMICOM1_TD_SPPID17  ( TDDS_BASE + 0x00000644 )      /* TD_SPPID17     */
#define PMICOM1_TD_SPPID18  ( TDDS_BASE + 0x00000648 )      /* TD_SPPID18     */
#define PMICOM1_TD_SPPID19  ( TDDS_BASE + 0x0000064C )      /* TD_SPPID19     */
#define PMICOM1_TD_SPPID20  ( TDDS_BASE + 0x00000650 )      /* TD_SPPID20     */
#define PMICOM1_TD_SPPID21  ( TDDS_BASE + 0x00000654 )      /* TD_SPPID21     */
#define PMICOM1_TD_SPPID22  ( TDDS_BASE + 0x00000658 )      /* TD_SPPID22     */
#define PMICOM1_TD_SPPID23  ( TDDS_BASE + 0x0000065C )      /* TD_SPPID23     */
#define PMICOM1_TD_SPPID24  ( TDDS_BASE + 0x00000660 )      /* TD_SPPID24     */
#define PMICOM1_TD_SPPID25  ( TDDS_BASE + 0x00000664 )      /* TD_SPPID25     */
#define PMICOM1_TD_SPPID26  ( TDDS_BASE + 0x00000668 )      /* TD_SPPID26     */
#define PMICOM1_TD_SPPID27  ( TDDS_BASE + 0x0000066C )      /* TD_SPPID27     */
#define PMICOM1_TD_SPPID28  ( TDDS_BASE + 0x00000670 )      /* TD_SPPID28     */
#define PMICOM1_TD_PCRPIDS  ( TDDS_BASE + 0x0000067C )      /* TD_PCRPIDS     */



/*------- PWM            0xD61B0000 ----------------------------------------*/

/*============================================================================*/
/* AVIO-0   Block   0xD8000000                                              */
/*============================================================================*/

/*------- ReCS           0xD8000000 ----------------------------------------*/

#define PAVIO0_XXXX                0xD8000000    /*                           */


/*------- JPEG           0xD8100000 ----------------------------------------*/
/*------- AVIO-Bridge    0xD8200000 ----------------------------------------*/
#define BRIDGE_BASE  ( PUNIPHIER_BASE + 0x08200000 )  /*<D1IO070> AVIO-Bridge */

#define PAVIO0_VINIRR       ( BRIDGE_BASE + 0x00000000 ) /*<D1IO070> VINIRR   */
#define PAVIO0_VINIRE       ( BRIDGE_BASE + 0x00000004 ) /*<D1IO070> VINIRE   */
#define PAVIO0_VINIRD       ( BRIDGE_BASE + 0x00000008 ) /*<D1IO070> VINIRD   */
#define PAVIO0_VOFIRR       ( BRIDGE_BASE + 0x00000010 ) /*<D1IO070> VOFIRR   */
#define PAVIO0_VOFIRE       ( BRIDGE_BASE + 0x00000014 ) /*<D1IO070> VOFIRE   */
#define PAVIO0_VOFIRD       ( BRIDGE_BASE + 0x00000018 ) /*<D1IO070> VOFIRD   */
#define PAVIO0_FDMIRR       ( BRIDGE_BASE + 0x00000020 ) /*<D1IO070> FDMIRR   */
#define PAVIO0_FDMIRE       ( BRIDGE_BASE + 0x00000024 ) /*<D1IO070> FDMIRE   */
#define PAVIO0_FDMIRD       ( BRIDGE_BASE + 0x00000028 ) /*<D1IO070> FDMIRD   */


/*============================================================================*/
/* AVIO-1   Block   0xD9000000                                              */
/*============================================================================*/

/*------- Frago          0xD9000000 ----------------------------------------*/

#define PAVIO1_XXXX                0xD9000000    /*                           */

/* 2005.05.31 S.Kojima  *//*<D1VD401>*/
#define FRAGO_BASE       ( PUNIPHIER_BASE + 0x09000000 ) /* Frago             */


/*------- VIN            0xD9020000 ----------------------------------------*/
#define VIN_BASE    ( PUNIPHIER_BASE + 0x09020000 )   /*<D1IO070> VIN         */

#define PAVIO1_VI_SET       ( VIN_BASE + 0x00000000 ) /*<D1IO070> VI_SET      */
#define PAVIO1_VI_CAPTURE   ( VIN_BASE + 0x00000004 ) /*<D1IO070> VI_CAPTURE  */
#define PAVIO1_VI_REFLECT   ( VIN_BASE + 0x00000008 ) /*<D1IO070> VI_REFLECT  */
#define PAVIO1_VI_POL       ( VIN_BASE + 0x00000010 ) /*<D1IO070> VI_POL      */
#define PAVIO1_VI_CAMPAT    ( VIN_BASE + 0x00000014 ) /*<D1IO070> VI_CAMPA T  */
#define PAVIO1_VI_IRQFS     ( VIN_BASE + 0x00000018 ) /*<D1IO070> VI_IRQFS    */
#define PAVIO1_VI_IRQFE     ( VIN_BASE + 0x0000001C ) /*<D1IO070> VI_IRQFE    */
#define PAVIO1_VI_CAMFORM   ( VIN_BASE + 0x00000020 ) /*<D1IO070> VI_CAMFORM  */
#define PAVIO1_VI_RQHME     ( VIN_BASE + 0x00000024 ) /*<D1IO070> VI_RQHME    */
#define PAVIO1_VI_IRQMODE   ( VIN_BASE + 0x00000028 ) /*<D1IO070> VI_IRQMODE  */
#define PAVIO1_VI_IRQWIDTH  ( VIN_BASE + 0x0000002C ) /*<D1IO070> VI_IRQWIDTH */
#define PAVIO1_VI_AREA      ( VIN_BASE + 0x00000030 ) /*<D1IO070> VI_AREA     */
#define PAVIO1_VI_HSTART    ( VIN_BASE + 0x00000034 ) /*<D1IO070> VI_HSTART   */
#define PAVIO1_VI_VSTART    ( VIN_BASE + 0x00000038 ) /*<D1IO070> VIVSTART    */
#define PAVIO1_VI_HVALID    ( VIN_BASE + 0x0000003C ) /*<D1IO070> VI_HVALID   */
#define PAVIO1_VI_VVALID    ( VIN_BASE + 0x00000040 ) /*<D1IO070> VI_VVALID   */
#define PAVIO1_VI_REDUCE    ( VIN_BASE + 0x00000050 ) /*<D1IO070> VI_REDUCE   */
#define PAVIO1_VI_HREDUCE   ( VIN_BASE + 0x00000054 ) /*<D1IO070> VI_HREDUCE  */
#define PAVIO1_VI_VREDUCE   ( VIN_BASE + 0x00000058 ) /*<D1IO070> VI_VREDUCE  */
#define PAVIO1_VI_HROFFSET  ( VIN_BASE + 0x0000005C ) /*<D1IO070> VI_HROFFSET */
#define PAVIO1_VI_VROFFSET  ( VIN_BASE + 0x00000060 ) /*<D1IO070> VI_VROFFSET */
#define PAVIO1_VI_HOUT      ( VIN_BASE + 0x00000064 ) /*<D1IO070> VI_HOUT     */
#define PAVIO1_VI_VOUT      ( VIN_BASE + 0x00000068 ) /*<D1IO070> VI_VOUT     */
#define PAVIO1_VI_C420      ( VIN_BASE + 0x00000070 ) /*<D1IO070> VI_C420     */
#define PAVIO1_VI_CLIP      ( VIN_BASE + 0x00000074 ) /*<D1IO070> VI_CLIP     */
#define PAVIO1_VI_PACKMD    ( VIN_BASE + 0x00000080 ) /*<D1IO070> VI_PACKMD   */
#define PAVIO1_VI_ENDIAN    ( VIN_BASE + 0x00000084 ) /*<D1IO070> VI_ENDIAN   */
#define PAVIO1_VI_HREV      ( VIN_BASE + 0x00000088 ) /*<D1IO070> VI_HREV     */
#define PAVIO1_VI_DMADEF    ( VIN_BASE + 0x0000008C ) /*<D1IO070> VI_DMADEF   */
#define PAVIO1_VI_DMALEN    ( VIN_BASE + 0x00000090 ) /*<D1IO070> VI_DMALEN   */
#define PAVIO1_VI_CAMCKC    ( VIN_BASE + 0x000000A0 ) /*<D1IO070> VI_CAMCKC   */
#define PAVIO1_VI_PALLS     ( VIN_BASE + 0x00000100 ) /*<D1IO070> VI_PALLS    */
#define PAVIO1_VI_LALLS     ( VIN_BASE + 0x00000104 ) /*<D1IO070> VI_LALLS    */
#define PAVIO1_VI_STATUS    ( VIN_BASE + 0x00000108 ) /*<D1IO070> VI_STATUS   */


/*------- VOUT           0xD9030000 ----------------------------------------*/
/* 2005.05.31 S.Kojima  *//*<D1VD401>*/
#define VOUT_BASE         ( PUNIPHIER_BASE + 0x09030000 ) /* VOUT             */


/*------- FDMAC          0xD9080000 ----------------------------------------*/
#define FDMAC_BASE  ( PUNIPHIER_BASE + 0x09082000 )    /*<D1IO070>FDMAC       */

#define PAVIO1_FDMACEXE     ( FDMAC_BASE + 0x00000000 )/*<D1IO070>FDMACEXE    */
#define PAVIO1_FDMACST      ( FDMAC_BASE + 0x00000004 )/*<D1IO070>FDMACST     */
#define PAVIO1_FDPMCKEN     ( FDMAC_BASE + 0x00000008 )/*<D1IO070>FDPMCKEN    */
#define PAVIO1_FDTESTSEL    ( FDMAC_BASE + 0x00000020 )/*<D1IO070>FDTESTSEL   */
#define PAVIO1_RPSAVINY     ( FDMAC_BASE + 0x00000100 )/*<D1IO070>RPSAVINY    */
#define PAVIO1_RPEAVINY     ( FDMAC_BASE + 0x00000104 )/*<D1IO070>RPEAVINY    */
#define PAVIO1_RPCNTVINY    ( FDMAC_BASE + 0x00000108 )/*<D1IO070>RPCNTVINY   */
#define PAVIO1_RPCADRVINY   ( FDMAC_BASE + 0x0000010C )/*<D1IO070>RPCADRVINY  */
#define PAVIO1_RPDF1VINY    ( FDMAC_BASE + 0x00000110 )/*<D1IO070>RPDF1VINY   */
#define PAVIO1_RPDF2VINY    ( FDMAC_BASE + 0x00000114 )/*<D1IO070>RPDF2VINY   */
#define PAVIO1_RPTHADRVINY  ( FDMAC_BASE + 0x00000118 )/*<D1IO070>RPTHADRVINY */
#define PAVIO1_RPSAVINCB    ( FDMAC_BASE + 0x00000120 )/*<D1IO070>RPSAVINCB   */
#define PAVIO1_RPEAVINCB    ( FDMAC_BASE + 0x00000124 )/*<D1IO070>RPEAVINCB   */
#define PAVIO1_RPCNTVINCB   ( FDMAC_BASE + 0x00000128 )/*<D1IO070>RPCNTVINCB  */
#define PAVIO1_RPCADRVINCB  ( FDMAC_BASE + 0x0000012C )/*<D1IO070>RPCADRVINCB */
#define PAVIO1_RPDF1VINCB   ( FDMAC_BASE + 0x00000130 )/*<D1IO070>RPDF1VINCB  */
#define PAVIO1_RPDF2VINCB   ( FDMAC_BASE + 0x00000134 )/*<D1IO070>RPDF2VINCB  */
#define PAVIO1_RPTHADRVINCB ( FDMAC_BASE + 0x00000138 )/*<D1IO070>RPTHADRVINCB*/
#define PAVIO1_RPSAVINCR    ( FDMAC_BASE + 0x00000140 )/*<D1IO070>RPSAVINCR   */
#define PAVIO1_RPEAVINCR    ( FDMAC_BASE + 0x00000144 )/*<D1IO070>RPEAVINCR   */
#define PAVIO1_RPCNTVINCR   ( FDMAC_BASE + 0x00000148 )/*<D1IO070>RPCNTVINCR  */
#define PAVIO1_RPCADRVINCR  ( FDMAC_BASE + 0x0000014C )/*<D1IO070>RPCADRVINCR */
#define PAVIO1_RPDF1VINCR   ( FDMAC_BASE + 0x00000150 )/*<D1IO070>RPDF1VINCR  */
#define PAVIO1_RPDF2VINCR   ( FDMAC_BASE + 0x00000154 )/*<D1IO070>RPDF2VINCR  */
#define PAVIO1_RPTHADRVINCR ( FDMAC_BASE + 0x00000158 )/*<D1IO070>RPTHADRVINCR*/


/*============================================================================*/
/* AVIO-2   Block   0xDA000000                                              */
/*============================================================================*/

/*------- BSSI3          0xDA000000 ----------------------------------------*/
#define BSSI3_BASE          ( PUNIPHIER_BASE + 0x0A000000 ) /* BSSI3          */

#define PAVIO2_BSSI3TXCTL   ( BSSI3_BASE + 0x00000000 )     /* BSSI3TXCTL     */
#define PAVIO2_BSSI3TXFCTL  ( BSSI3_BASE + 0x00000004 )     /* BSSI3TXFCTL    */
#define PAVIO2_BSSI3TXCKS   ( BSSI3_BASE + 0x00000008 )     /* BSSI3TXCKS     */
#define PAVIO2_BSSI3TXWDS   ( BSSI3_BASE + 0x0000000C )     /* BSSI3TXWDS     */
#define PAVIO2_BSSI3TXFPS   ( BSSI3_BASE + 0x00000014 )     /* BSSI3TXFPS     */
#define PAVIO2_BSSI3TXFS    ( BSSI3_BASE + 0x00000018 )     /* BSSI3TXFS      */
#define PAVIO2_BSSI3TXTOE   ( BSSI3_BASE + 0x00000020 )     /* BSSI3TXTOE     */
#define PAVIO2_BSSI3TXIE    ( BSSI3_BASE + 0x00000024 )     /* BSSI3TXIE      */
#define PAVIO2_BSSI3TXSR    ( BSSI3_BASE + 0x00000028 )     /* BSSI3TXSR      */
#define PAVIO2_BSSI3TXICR   ( BSSI3_BASE + 0x00000028 )     /* BSSI3TXICR     */
#define PAVIO2_BSSI3TXDR    ( BSSI3_BASE + 0x0000002C )     /* BSSI3TXDR      */
#define PAVIO2_BSSI3TXCNT   ( BSSI3_BASE + 0x00000030 )     /* BSSI3TXCNT     */
#define PAVIO2_BSSI3RXCTL   ( BSSI3_BASE + 0x00000100 )     /* BSSI3RXCTL     */
#define PAVIO2_BSSI3RXFCTL  ( BSSI3_BASE + 0x00000104 )     /* BSSI3RXFCTL    */
#define PAVIO2_BSSI3RXCKS   ( BSSI3_BASE + 0x00000108 )     /* BSSI3RXCKS     */
#define PAVIO2_BSSI3RXWDS   ( BSSI3_BASE + 0x0000010C )     /* BSSI3RXWDS     */
#define PAVIO2_BSSI3RXFPS   ( BSSI3_BASE + 0x00000114 )     /* BSSI3RXFPS     */
#define PAVIO2_BSSI3RXFS    ( BSSI3_BASE + 0x00000118 )     /* BSSI3RXFS      */
#define PAVIO2_BSSI3RXTOE   ( BSSI3_BASE + 0x00000120 )     /* BSSI3RXTOE     */
#define PAVIO2_BSSI3RXCNT   ( BSSI3_BASE + 0x00000124 )     /* BSSI3RXCNT     */
#define PAVIO2_BSSI3NRXCNT  ( BSSI3_BASE + 0x00000128 )     /* BSSI3NRXCNT    */
#define PAVIO2_BSSI3RXIE    ( BSSI3_BASE + 0x0000012C )     /* BSSI3RXIE      */
#define PAVIO2_BSSI3RXSR    ( BSSI3_BASE + 0x00000130 )     /* BSSI3RXSR      */
#define PAVIO2_BSSI3RXICR   ( BSSI3_BASE + 0x00000130 )     /* BSSI3RXICR     */
#define PAVIO2_BSSI3RXDR    ( BSSI3_BASE + 0x00000134 )     /* BSSI3RXDR      */


/*------- BSSI4          0xDA010000 ----------------------------------------*/
#define BSSI4_BASE          ( PUNIPHIER_BASE + 0x0A010000 ) /* BSSI4          */

#define PAVIO2_BSSI4TXCTL   ( BSSI4_BASE + 0x00000000 )     /* BSSI4TXCTL     */
#define PAVIO2_BSSI4TXFCTL  ( BSSI4_BASE + 0x00000004 )     /* BSSI4TXFCTL    */
#define PAVIO2_BSSI4TXCKS   ( BSSI4_BASE + 0x00000008 )     /* BSSI4TXCKS     */
#define PAVIO2_BSSI4TXWDS   ( BSSI4_BASE + 0x0000000C )     /* BSSI4TXWDS     */
#define PAVIO2_BSSI4TXFPS   ( BSSI4_BASE + 0x00000014 )     /* BSSI4TXFPS     */
#define PAVIO2_BSSI4TXFS    ( BSSI4_BASE + 0x00000018 )     /* BSSI4TXFS      */
#define PAVIO2_BSSI4TXTOE   ( BSSI4_BASE + 0x00000020 )     /* BSSI4TXTOE     */
#define PAVIO2_BSSI4TXIE    ( BSSI4_BASE + 0x00000024 )     /* BSSI4TXIE      */
#define PAVIO2_BSSI4TXSR    ( BSSI4_BASE + 0x00000028 )     /* BSSI4TXSR      */
#define PAVIO2_BSSI4TXICR   ( BSSI4_BASE + 0x00000028 )     /* BSSI4TXICR     */
#define PAVIO2_BSSI4TXDR    ( BSSI4_BASE + 0x0000002C )     /* BSSI4TXDR      */
#define PAVIO2_BSSI4TXCNT   ( BSSI4_BASE + 0x00000030 )     /* BSSI4TXCNT     */
#define PAVIO2_BSSI4RXCTL   ( BSSI4_BASE + 0x00000100 )     /* BSSI4RXCTL     */
#define PAVIO2_BSSI4RXFCTL  ( BSSI4_BASE + 0x00000104 )     /* BSSI4RXFCTL    */
#define PAVIO2_BSSI4RXCKS   ( BSSI4_BASE + 0x00000108 )     /* BSSI4RXCKS     */
#define PAVIO2_BSSI4RXWDS   ( BSSI4_BASE + 0x0000010C )     /* BSSI4RXWDS     */
#define PAVIO2_BSSI4RXFPS   ( BSSI4_BASE + 0x00000114 )     /* BSSI4RXFPS     */
#define PAVIO2_BSSI4RXFS    ( BSSI4_BASE + 0x00000118 )     /* BSSI4RXFS      */
#define PAVIO2_BSSI4RXTOE   ( BSSI4_BASE + 0x00000120 )     /* BSSI4RXTOE     */
#define PAVIO2_BSSI4RXCNT   ( BSSI4_BASE + 0x00000124 )     /* BSSI4RXCNT     */
#define PAVIO2_BSSI4NRXCNT  ( BSSI4_BASE + 0x00000128 )     /* BSSI4NRXCNT    */
#define PAVIO2_BSSI4RXIE    ( BSSI4_BASE + 0x0000012C )     /* BSSI4RXIE      */
#define PAVIO2_BSSI4RXSR    ( BSSI4_BASE + 0x00000130 )     /* BSSI4RXSR      */
#define PAVIO2_BSSI4RXICR   ( BSSI4_BASE + 0x00000130 )     /* BSSI4RXICR     */
#define PAVIO2_BSSI4RXDR    ( BSSI4_BASE + 0x00000134 )     /* BSSI4RXDR      */


/*------- BSSI5          0xDA020000 ----------------------------------------*/
#define BSSI5_BASE          ( PUNIPHIER_BASE + 0x0A020000 ) /* BSSI5          */

#define PAVIO2_BSSI5TXCTL   ( BSSI5_BASE + 0x00000000 )     /* BSSI5TXCTL     */
#define PAVIO2_BSSI5TXFCTL  ( BSSI5_BASE + 0x00000004 )     /* BSSI5TXFCTL    */
#define PAVIO2_BSSI5TXCKS   ( BSSI5_BASE + 0x00000008 )     /* BSSI5TXCKS     */
#define PAVIO2_BSSI5TXWDS   ( BSSI5_BASE + 0x0000000C )     /* BSSI5TXWDS     */
#define PAVIO2_BSSI5TXFPS   ( BSSI5_BASE + 0x00000014 )     /* BSSI5TXFPS     */
#define PAVIO2_BSSI5TXFS    ( BSSI5_BASE + 0x00000018 )     /* BSSI5TXFS      */
#define PAVIO2_BSSI5TXTOE   ( BSSI5_BASE + 0x00000020 )     /* BSSI5TXTOE     */
#define PAVIO2_BSSI5TXIE    ( BSSI5_BASE + 0x00000024 )     /* BSSI5TXIE      */
#define PAVIO2_BSSI5TXSR    ( BSSI5_BASE + 0x00000028 )     /* BSSI5TXSR      */
#define PAVIO2_BSSI5TXICR   ( BSSI5_BASE + 0x00000028 )     /* BSSI5TXICR     */
#define PAVIO2_BSSI5TXDR    ( BSSI5_BASE + 0x0000002C )     /* BSSI5TXDR      */
#define PAVIO2_BSSI5TXCNT   ( BSSI5_BASE + 0x00000030 )     /* BSSI5TXCNT     */
#define PAVIO2_BSSI5RXCTL   ( BSSI5_BASE + 0x00000100 )     /* BSSI5RXCTL     */
#define PAVIO2_BSSI5RXFCTL  ( BSSI5_BASE + 0x00000104 )     /* BSSI5RXFCTL    */
#define PAVIO2_BSSI5RXCKS   ( BSSI5_BASE + 0x00000108 )     /* BSSI5RXCKS     */
#define PAVIO2_BSSI5RXWDS   ( BSSI5_BASE + 0x0000010C )     /* BSSI5RXWDS     */
#define PAVIO2_BSSI5RXFPS   ( BSSI5_BASE + 0x00000114 )     /* BSSI5RXFPS     */
#define PAVIO2_BSSI5RXFS    ( BSSI5_BASE + 0x00000118 )     /* BSSI5RXFS      */
#define PAVIO2_BSSI5RXTOE   ( BSSI5_BASE + 0x00000120 )     /* BSSI5RXTOE     */
#define PAVIO2_BSSI5RXCNT   ( BSSI5_BASE + 0x00000124 )     /* BSSI5RXCNT     */
#define PAVIO2_BSSI5NRXCNT  ( BSSI5_BASE + 0x00000128 )     /* BSSI5NRXCNT    */
#define PAVIO2_BSSI5RXIE    ( BSSI5_BASE + 0x0000012C )     /* BSSI5RXIE      */
#define PAVIO2_BSSI5RXSR    ( BSSI5_BASE + 0x00000130 )     /* BSSI5RXSR      */
#define PAVIO2_BSSI5RXICR   ( BSSI5_BASE + 0x00000130 )     /* BSSI5RXICR     */
#define PAVIO2_BSSI5RXDR    ( BSSI5_BASE + 0x00000134 )     /* BSSI5RXDR      */


/*============================================================================*/
/* Stream   Block   0xDC000000                                              */
/*============================================================================*/

/*------- StreamDMAC     0xDC000000 ----------------------------------------*/
#define SDMAC_BASE          ( PUNIPHIER_BASE + 0x0C000000 ) /* StreamDMAC     */

#define PSTREAM_SBUF_STADR0     ( SDMAC_BASE + 0x00000000 ) /* SBUF_STADR0    */
#define PSTREAM_SBUF_WIDTH0     ( SDMAC_BASE + 0x00000004 ) /* SBUF_WIDTH0    */
#define PSTREAM_SBUF_IN0        ( SDMAC_BASE + 0x00000008 ) /* SBUF_IN0       */
#define PSTREAM_SBUF_OUT0       ( SDMAC_BASE + 0x0000000C ) /* SBUF_OUT0      */
#define PSTREAM_SBUF_SH0        ( SDMAC_BASE + 0x00000010 ) /* SBUF_SH0       */
#define PSTREAM_SBUF_WPTR0      ( SDMAC_BASE + 0x00000014 ) /* SBUF_WPTR0     */
#define PSTREAM_SBUF_RPTR0      ( SDMAC_BASE + 0x00000018 ) /* SBUF_RPTR0     */
#define PSTREAM_SBUF_STADR1     ( SDMAC_BASE + 0x00000020 ) /* SBUF_STADR1    */
#define PSTREAM_SBUF_WIDTH1     ( SDMAC_BASE + 0x00000024 ) /* SBUF_WIDTH1    */
#define PSTREAM_SBUF_IN1        ( SDMAC_BASE + 0x00000028 ) /* SBUF_IN1       */
#define PSTREAM_SBUF_OUT1       ( SDMAC_BASE + 0x0000002C ) /* SBUF_OUT1      */
#define PSTREAM_SBUF_SH1        ( SDMAC_BASE + 0x00000030 ) /* SBUF_SH1       */
#define PSTREAM_SBUF_WPTR1      ( SDMAC_BASE + 0x00000034 ) /* SBUF_WPTR1     */
#define PSTREAM_SBUF_RPTR1      ( SDMAC_BASE + 0x00000038 ) /* SBUF_RPTR1     */
#define PSTREAM_SBUF_STADR2     ( SDMAC_BASE + 0x00000040 ) /* SBUF_STADR2    */
#define PSTREAM_SBUF_WIDTH2     ( SDMAC_BASE + 0x00000044 ) /* SBUF_WIDTH2    */
#define PSTREAM_SBUF_IN2        ( SDMAC_BASE + 0x00000048 ) /* SBUF_IN2       */
#define PSTREAM_SBUF_OUT2       ( SDMAC_BASE + 0x0000004C ) /* SBUF_OUT2      */
#define PSTREAM_SBUF_SH2        ( SDMAC_BASE + 0x00000050 ) /* SBUF_SH2       */
#define PSTREAM_SBUF_WPTR2      ( SDMAC_BASE + 0x00000054 ) /* SBUF_WPTR2     */
#define PSTREAM_SBUF_RPTR2      ( SDMAC_BASE + 0x00000058 ) /* SBUF_RPTR2     */
#define PSTREAM_SBUF_STADR3     ( SDMAC_BASE + 0x00000060 ) /* SBUF_STADR3    */
#define PSTREAM_SBUF_WIDTH3     ( SDMAC_BASE + 0x00000064 ) /* SBUF_WIDTH3    */
#define PSTREAM_SBUF_IN3        ( SDMAC_BASE + 0x00000068 ) /* SBUF_IN3       */
#define PSTREAM_SBUF_OUT3       ( SDMAC_BASE + 0x0000006C ) /* SBUF_OUT3      */
#define PSTREAM_SBUF_SH3        ( SDMAC_BASE + 0x00000070 ) /* SBUF_SH3       */
#define PSTREAM_SBUF_WPTR3      ( SDMAC_BASE + 0x00000074 ) /* SBUF_WPTR3     */
#define PSTREAM_SBUF_RPTR3      ( SDMAC_BASE + 0x00000078 ) /* SBUF_RPTR3     */
#define PSTREAM_SBUF_STADR4     ( SDMAC_BASE + 0x00000080 ) /* SBUF_STADR4    */
#define PSTREAM_SBUF_WIDTH4     ( SDMAC_BASE + 0x00000084 ) /* SBUF_WIDTH4    */
#define PSTREAM_SBUF_IN4        ( SDMAC_BASE + 0x00000088 ) /* SBUF_IN4       */
#define PSTREAM_SBUF_OUT4       ( SDMAC_BASE + 0x0000008C ) /* SBUF_OUT4      */
#define PSTREAM_SBUF_SH4        ( SDMAC_BASE + 0x00000090 ) /* SBUF_SH4       */
#define PSTREAM_SBUF_WPTR4      ( SDMAC_BASE + 0x00000094 ) /* SBUF_WPTR4     */
#define PSTREAM_SBUF_RPTR4      ( SDMAC_BASE + 0x00000098 ) /* SBUF_RPTR4     */
#define PSTREAM_SBUF_STADR5     ( SDMAC_BASE + 0x000000A0 ) /* SBUF_STADR5    */
#define PSTREAM_SBUF_WIDTH5     ( SDMAC_BASE + 0x000000A4 ) /* SBUF_WIDTH5    */
#define PSTREAM_SBUF_IN5        ( SDMAC_BASE + 0x000000A8 ) /* SBUF_IN5       */
#define PSTREAM_SBUF_OUT5       ( SDMAC_BASE + 0x000000AC ) /* SBUF_OUT5      */
#define PSTREAM_SBUF_SH5        ( SDMAC_BASE + 0x000000B0 ) /* SBUF_SH5       */
#define PSTREAM_SBUF_WPTR5      ( SDMAC_BASE + 0x000000B4 ) /* SBUF_WPTR5     */
#define PSTREAM_SBUF_RPTR5      ( SDMAC_BASE + 0x000000B8 ) /* SBUF_RPTR5     */
#define PSTREAM_SBUF_STADR6     ( SDMAC_BASE + 0x000000C0 ) /* SBUF_STADR6    */
#define PSTREAM_SBUF_WIDTH6     ( SDMAC_BASE + 0x000000C4 ) /* SBUF_WIDTH6    */
#define PSTREAM_SBUF_IN6        ( SDMAC_BASE + 0x000000C8 ) /* SBUF_IN6       */
#define PSTREAM_SBUF_OUT6       ( SDMAC_BASE + 0x000000CC ) /* SBUF_OUT6      */
#define PSTREAM_SBUF_SH6        ( SDMAC_BASE + 0x000000D0 ) /* SBUF_SH6       */
#define PSTREAM_SBUF_WPTR6      ( SDMAC_BASE + 0x000000D4 ) /* SBUF_WPTR6     */
#define PSTREAM_SBUF_RPTR6      ( SDMAC_BASE + 0x000000D8 ) /* SBUF_RPTR6     */
#define PSTREAM_SBUF_STADR7     ( SDMAC_BASE + 0x000000E0 ) /* SBUF_STADR7    */
#define PSTREAM_SBUF_WIDTH7     ( SDMAC_BASE + 0x000000E4 ) /* SBUF_WIDTH7    */
#define PSTREAM_SBUF_IN7        ( SDMAC_BASE + 0x000000E8 ) /* SBUF_IN7       */
#define PSTREAM_SBUF_OUT7       ( SDMAC_BASE + 0x000000EC ) /* SBUF_OUT7      */
#define PSTREAM_SBUF_SH7        ( SDMAC_BASE + 0x000000F0 ) /* SBUF_SH7       */
#define PSTREAM_SBUF_WPTR7      ( SDMAC_BASE + 0x000000F4 ) /* SBUF_WPTR7     */
#define PSTREAM_SBUF_RPTR7      ( SDMAC_BASE + 0x000000F8 ) /* SBUF_RPTR7     */
#define PSTREAM_SBUF_STADR8     ( SDMAC_BASE + 0x00000100 ) /* SBUF_STADR8    */
#define PSTREAM_SBUF_WIDTH8     ( SDMAC_BASE + 0x00000104 ) /* SBUF_WIDTH8    */
#define PSTREAM_SBUF_IN8        ( SDMAC_BASE + 0x00000108 ) /* SBUF_IN8       */
#define PSTREAM_SBUF_OUT8       ( SDMAC_BASE + 0x0000010C ) /* SBUF_OUT8      */
#define PSTREAM_SBUF_SH8        ( SDMAC_BASE + 0x00000110 ) /* SBUF_SH8       */
#define PSTREAM_SBUF_WPTR8      ( SDMAC_BASE + 0x00000114 ) /* SBUF_WPTR8     */
#define PSTREAM_SBUF_RPTR8      ( SDMAC_BASE + 0x00000118 ) /* SBUF_RPTR8     */
#define PSTREAM_SBUF_STADR9     ( SDMAC_BASE + 0x00000120 ) /* SBUF_STADR9    */
#define PSTREAM_SBUF_WIDTH9     ( SDMAC_BASE + 0x00000124 ) /* SBUF_WIDTH9    */
#define PSTREAM_SBUF_IN9        ( SDMAC_BASE + 0x00000128 ) /* SBUF_IN9       */
#define PSTREAM_SBUF_OUT9       ( SDMAC_BASE + 0x0000012C ) /* SBUF_OUT9      */
#define PSTREAM_SBUF_SH9        ( SDMAC_BASE + 0x00000130 ) /* SBUF_SH9       */
#define PSTREAM_SBUF_WPTR9      ( SDMAC_BASE + 0x00000134 ) /* SBUF_WPTR9     */
#define PSTREAM_SBUF_RPTR9      ( SDMAC_BASE + 0x00000138 ) /* SBUF_RPTR9     */
#define PSTREAM_SBUF_STADR10    ( SDMAC_BASE + 0x00000140 ) /* SBUF_STADR10   */
#define PSTREAM_SBUF_WIDTH10    ( SDMAC_BASE + 0x00000144 ) /* SBUF_WIDTH10   */
#define PSTREAM_SBUF_IN10       ( SDMAC_BASE + 0x00000148 ) /* SBUF_IN10      */
#define PSTREAM_SBUF_OUT10      ( SDMAC_BASE + 0x0000014C ) /* SBUF_OUT10     */
#define PSTREAM_SBUF_SH10       ( SDMAC_BASE + 0x00000150 ) /* SBUF_SH10      */
#define PSTREAM_SBUF_WPTR10     ( SDMAC_BASE + 0x00000154 ) /* SBUF_WPTR10    */
#define PSTREAM_SBUF_RPTR10     ( SDMAC_BASE + 0x00000158 ) /* SBUF_RPTR10    */
#define PSTREAM_SBUF_STADR11    ( SDMAC_BASE + 0x00000160 ) /* SBUF_STADR11   */
#define PSTREAM_SBUF_WIDTH11    ( SDMAC_BASE + 0x00000164 ) /* SBUF_WIDTH11   */
#define PSTREAM_SBUF_IN11       ( SDMAC_BASE + 0x00000168 ) /* SBUF_IN11      */
#define PSTREAM_SBUF_OUT11      ( SDMAC_BASE + 0x0000016C ) /* SBUF_OUT11     */
#define PSTREAM_SBUF_SH11       ( SDMAC_BASE + 0x00000170 ) /* SBUF_SH11      */
#define PSTREAM_SBUF_WPTR11     ( SDMAC_BASE + 0x00000174 ) /* SBUF_WPTR11    */
#define PSTREAM_SBUF_RPTR11     ( SDMAC_BASE + 0x00000178 ) /* SBUF_RPTR11    */
#define PSTREAM_SBUF_STADR12    ( SDMAC_BASE + 0x00000180 ) /* SBUF_STADR12   */
#define PSTREAM_SBUF_WIDTH12    ( SDMAC_BASE + 0x00000184 ) /* SBUF_WIDTH12   */
#define PSTREAM_SBUF_IN12       ( SDMAC_BASE + 0x00000188 ) /* SBUF_IN12      */
#define PSTREAM_SBUF_OUT12      ( SDMAC_BASE + 0x0000018C ) /* SBUF_OUT12     */
#define PSTREAM_SBUF_SH12       ( SDMAC_BASE + 0x00000190 ) /* SBUF_SH12      */
#define PSTREAM_SBUF_WPTR12     ( SDMAC_BASE + 0x00000194 ) /* SBUF_WPTR12    */
#define PSTREAM_SBUF_RPTR12     ( SDMAC_BASE + 0x00000198 ) /* SBUF_RPTR12    */
#define PSTREAM_SBUF_STADR13    ( SDMAC_BASE + 0x000001A0 ) /* SBUF_STADR13   */
#define PSTREAM_SBUF_WIDTH13    ( SDMAC_BASE + 0x000001A4 ) /* SBUF_WIDTH13   */
#define PSTREAM_SBUF_IN13       ( SDMAC_BASE + 0x000001A8 ) /* SBUF_IN13      */
#define PSTREAM_SBUF_OUT13      ( SDMAC_BASE + 0x000001AC ) /* SBUF_OUT13     */
#define PSTREAM_SBUF_SH13       ( SDMAC_BASE + 0x000001B0 ) /* SBUF_SH13      */
#define PSTREAM_SBUF_WPTR13     ( SDMAC_BASE + 0x000001B4 ) /* SBUF_WPTR13    */
#define PSTREAM_SBUF_RPTR13     ( SDMAC_BASE + 0x000001B8 ) /* SBUF_RPTR13    */
#define PSTREAM_SBUF_DATA       ( SDMAC_BASE + 0x00000220 ) /* SBUF_DATA      */
#define PSTREAM_SBUF_SEL        ( SDMAC_BASE + 0x00000224 ) /* SBUF_SEL       */
#define PSTREAM_SBUF_CLR        ( SDMAC_BASE + 0x00000228 ) /* SBUF_CLR       */
#define PSTREAM_ABUF_STADR0     ( SDMAC_BASE + 0x00000240 ) /* ABUF_STADR0    */
#define PSTREAM_ABUF_WIDTH0     ( SDMAC_BASE + 0x00000244 ) /* ABUF_WIDTH0    */
#define PSTREAM_ABUF_IN0        ( SDMAC_BASE + 0x00000248 ) /* ABUF_IN0       */
#define PSTREAM_ABUF_OUT0       ( SDMAC_BASE + 0x0000024C ) /* ABUF_OUT0      */
#define PSTREAM_ABUF_SH0        ( SDMAC_BASE + 0x00000250 ) /* ABUF_SH0       */
#define PSTREAM_ABUF_WPTR0      ( SDMAC_BASE + 0x00000254 ) /* ABUF_WPTR0     */
#define PSTREAM_ABUF_RPTR0      ( SDMAC_BASE + 0x00000258 ) /* ABUF_RPTR0     */
#define PSTREAM_ABUF_STADR1     ( SDMAC_BASE + 0x00000260 ) /* ABUF_STADR1    */
#define PSTREAM_ABUF_WIDTH1     ( SDMAC_BASE + 0x00000264 ) /* ABUF_WIDTH1    */
#define PSTREAM_ABUF_IN1        ( SDMAC_BASE + 0x00000268 ) /* ABUF_IN1       */
#define PSTREAM_ABUF_OUT1       ( SDMAC_BASE + 0x0000026C ) /* ABUF_OUT1      */
#define PSTREAM_ABUF_SH1        ( SDMAC_BASE + 0x00000270 ) /* ABUF_SH1       */
#define PSTREAM_ABUF_WPTR1      ( SDMAC_BASE + 0x00000274 ) /* ABUF_WPTR1     */
#define PSTREAM_ABUF_RPTR1      ( SDMAC_BASE + 0x00000278 ) /* ABUF_RPTR1     */
#define PSTREAM_ABUF_STADR2     ( SDMAC_BASE + 0x00000280 ) /* ABUF_STADR2    */
#define PSTREAM_ABUF_WIDTH2     ( SDMAC_BASE + 0x00000284 ) /* ABUF_WIDTH2    */
#define PSTREAM_ABUF_IN2        ( SDMAC_BASE + 0x00000288 ) /* ABUF_IN2       */
#define PSTREAM_ABUF_OUT2       ( SDMAC_BASE + 0x0000028C ) /* ABUF_OUT2      */
#define PSTREAM_ABUF_SH2        ( SDMAC_BASE + 0x00000290 ) /* ABUF_SH2       */
#define PSTREAM_ABUF_WPTR2      ( SDMAC_BASE + 0x00000294 ) /* ABUF_WPTR2     */
#define PSTREAM_ABUF_RPTR2      ( SDMAC_BASE + 0x00000298 ) /* ABUF_RPTR2     */
#define PSTREAM_ABUF_STADR3     ( SDMAC_BASE + 0x000002A0 ) /* ABUF_STADR3    */
#define PSTREAM_ABUF_WIDTH3     ( SDMAC_BASE + 0x000002A4 ) /* ABUF_WIDTH3    */
#define PSTREAM_ABUF_IN3        ( SDMAC_BASE + 0x000002A8 ) /* ABUF_IN3       */
#define PSTREAM_ABUF_OUT3       ( SDMAC_BASE + 0x000002AC ) /* ABUF_OUT3      */
#define PSTREAM_ABUF_SH3        ( SDMAC_BASE + 0x000002B0 ) /* ABUF_SH3       */
#define PSTREAM_ABUF_WPTR3      ( SDMAC_BASE + 0x000002B4 ) /* ABUF_WPTR3     */
#define PSTREAM_ABUF_RPTR3      ( SDMAC_BASE + 0x000002B8 ) /* ABUF_RPTR3     */
#define PSTREAM_ABUF_DATA       ( SDMAC_BASE + 0x000002C0 ) /* ABUF_DATA      */
#define PSTREAM_ABUF_SEL        ( SDMAC_BASE + 0x000002C4 ) /* ABUF_SEL       */
#define PSTREAM_ABUF_CLR        ( SDMAC_BASE + 0x000002C8 ) /* ABUF_CLR       */
#define PSTREAM_ABUF_OPT        ( SDMAC_BASE + 0x000002CC ) /* ABUF_OPT       */
#define PSTREAM_DMA_CNT0        ( SDMAC_BASE + 0x00000400 ) /* DMA_CNT0       */
#define PSTREAM_DMA_SIZE0       ( SDMAC_BASE + 0x00000404 ) /* DMA_SIZE0      */
#define PSTREAM_DMA_TRANS0      ( SDMAC_BASE + 0x00000408 ) /* DMA_TRANS0     */
#define PSTREAM_DMA_INTCNT0     ( SDMAC_BASE + 0x0000040C ) /* DMA_INTCNT0    */
#define PSTREAM_DMA_MRSTRT0     ( SDMAC_BASE + 0x00000410 ) /* DMA_MRSTRT0    */
#define PSTREAM_DMA_MRSIZE0     ( SDMAC_BASE + 0x00000414 ) /* DMA_MRSIZE0    */
#define PSTREAM_DMA_MWSTRT0     ( SDMAC_BASE + 0x00000418 ) /* DMA_MWSTRT0    */
#define PSTREAM_DMA_MWSIZE0     ( SDMAC_BASE + 0x0000041C ) /* DMA_MWSIZE0    */
#define PSTREAM_DMA_CNT1        ( SDMAC_BASE + 0x00000420 ) /* DMA_CNT1       */
#define PSTREAM_DMA_SIZE1       ( SDMAC_BASE + 0x00000424 ) /* DMA_SIZE1      */
#define PSTREAM_DMA_TRANS1      ( SDMAC_BASE + 0x00000428 ) /* DMA_TRANS1     */
#define PSTREAM_DMA_INTCNT1     ( SDMAC_BASE + 0x0000042C ) /* DMA_INTCNT1    */
#define PSTREAM_DMA_MRSTRT1     ( SDMAC_BASE + 0x00000430 ) /* DMA_MRSTRT1    */
#define PSTREAM_DMA_MRSIZE1     ( SDMAC_BASE + 0x00000434 ) /* DMA_MRSIZE1    */
#define PSTREAM_DMA_MWSTRT1     ( SDMAC_BASE + 0x00000438 ) /* DMA_MWSTRT1    */
#define PSTREAM_DMA_MWSIZE1     ( SDMAC_BASE + 0x0000043C ) /* DMA_MWSIZE1    */
#define PSTREAM_DMA_CNT2        ( SDMAC_BASE + 0x00000440 ) /* DMA_CNT2       */
#define PSTREAM_DMA_SIZE2       ( SDMAC_BASE + 0x00000444 ) /* DMA_SIZE2      */
#define PSTREAM_DMA_TRANS2      ( SDMAC_BASE + 0x00000448 ) /* DMA_TRANS2     */
#define PSTREAM_DMA_INTCNT2     ( SDMAC_BASE + 0x0000044C ) /* DMA_INTCNT2    */
#define PSTREAM_DMA_MRSTRT2     ( SDMAC_BASE + 0x00000450 ) /* DMA_MRSTRT2    */
#define PSTREAM_DMA_MRSIZE2     ( SDMAC_BASE + 0x00000454 ) /* DMA_MRSIZE2    */
#define PSTREAM_DMA_MWSTRT2     ( SDMAC_BASE + 0x00000458 ) /* DMA_MWSTRT2    */
#define PSTREAM_DMA_MWSIZE2     ( SDMAC_BASE + 0x0000045C ) /* DMA_MWSIZE2    */
#define PSTREAM_DMA_CNT3        ( SDMAC_BASE + 0x00000460 ) /* DMA_CNT3       */
#define PSTREAM_DMA_SIZE3       ( SDMAC_BASE + 0x00000464 ) /* DMA_SIZE3      */
#define PSTREAM_DMA_TRANS3      ( SDMAC_BASE + 0x00000468 ) /* DMA_TRANS3     */
#define PSTREAM_DMA_INTCNT3     ( SDMAC_BASE + 0x0000046C ) /* DMA_INTCNT3    */
#define PSTREAM_DMA_MRSTRT3     ( SDMAC_BASE + 0x00000470 ) /* DMA_MRSTRT3    */
#define PSTREAM_DMA_MRSIZE3     ( SDMAC_BASE + 0x00000474 ) /* DMA_MRSIZE3    */
#define PSTREAM_DMA_MWSTRT3     ( SDMAC_BASE + 0x00000478 ) /* DMA_MWSTRT3    */
#define PSTREAM_DMA_MWSIZE3     ( SDMAC_BASE + 0x0000047C ) /* DMA_MWSIZE3    */
#define PSTREAM_DMA_CNT4        ( SDMAC_BASE + 0x00000480 ) /* DMA_CNT4       */
#define PSTREAM_DMA_SIZE4       ( SDMAC_BASE + 0x00000484 ) /* DMA_SIZE4      */
#define PSTREAM_DMA_TRANS4      ( SDMAC_BASE + 0x00000488 ) /* DMA_TRANS4     */
#define PSTREAM_DMA_INTCNT4     ( SDMAC_BASE + 0x0000048C ) /* DMA_INTCNT4    */
#define PSTREAM_DMA_MRSTRT4     ( SDMAC_BASE + 0x00000490 ) /* DMA_MRSTRT4    */
#define PSTREAM_DMA_MRSIZE4     ( SDMAC_BASE + 0x00000494 ) /* DMA_MRSIZE4    */
#define PSTREAM_DMA_MWSTRT4     ( SDMAC_BASE + 0x00000498 ) /* DMA_MWSTRT4    */
#define PSTREAM_DMA_MWSIZE4     ( SDMAC_BASE + 0x0000049C ) /* DMA_MWSIZE4    */
#define PSTREAM_DMA_CNT5        ( SDMAC_BASE + 0x000004A0 ) /* DMA_CNT5       */
#define PSTREAM_DMA_SIZE5       ( SDMAC_BASE + 0x000004A4 ) /* DMA_SIZE5      */
#define PSTREAM_DMA_TRANS5      ( SDMAC_BASE + 0x000004A8 ) /* DMA_TRANS5     */
#define PSTREAM_DMA_INTCNT5     ( SDMAC_BASE + 0x000004AC ) /* DMA_INTCNT5    */
#define PSTREAM_DMA_MRSTRT5     ( SDMAC_BASE + 0x000004B0 ) /* DMA_MRSTRT5    */
#define PSTREAM_DMA_MRSIZE5     ( SDMAC_BASE + 0x000004B4 ) /* DMA_MRSIZE5    */
#define PSTREAM_DMA_MWSTRT5     ( SDMAC_BASE + 0x000004B8 ) /* DMA_MWSTRT5    */
#define PSTREAM_DMA_MWSIZE5     ( SDMAC_BASE + 0x000004BC ) /* DMA_MWSIZE5    */
#define PSTREAM_DMA_CNT6        ( SDMAC_BASE + 0x000004C0 ) /* DMA_CNT6       */
#define PSTREAM_DMA_SIZE6       ( SDMAC_BASE + 0x000004C4 ) /* DMA_SIZE6      */
#define PSTREAM_DMA_TRANS6      ( SDMAC_BASE + 0x000004C8 ) /* DMA_TRANS6     */
#define PSTREAM_DMA_INTCNT6     ( SDMAC_BASE + 0x000004CC ) /* DMA_INTCNT6    */
#define PSTREAM_DMA_MRSTRT6     ( SDMAC_BASE + 0x000004D0 ) /* DMA_MRSTRT6    */
#define PSTREAM_DMA_MRSIZE6     ( SDMAC_BASE + 0x000004D4 ) /* DMA_MRSIZE6    */
#define PSTREAM_DMA_MWSTRT6     ( SDMAC_BASE + 0x000004D8 ) /* DMA_MWSTRT6    */
#define PSTREAM_DMA_MWSIZE6     ( SDMAC_BASE + 0x000004DC ) /* DMA_MWSIZE6    */
#define PSTREAM_DMA_CNT7        ( SDMAC_BASE + 0x000004E0 ) /* DMA_CNT7       */
#define PSTREAM_DMA_SIZE7       ( SDMAC_BASE + 0x000004E4 ) /* DMA_SIZE7      */
#define PSTREAM_DMA_TRANS7      ( SDMAC_BASE + 0x000004E8 ) /* DMA_TRANS7     */
#define PSTREAM_DMA_INTCNT7     ( SDMAC_BASE + 0x000004EC ) /* DMA_INTCNT7    */
#define PSTREAM_DMA_MRSTRT7     ( SDMAC_BASE + 0x000004F0 ) /* DMA_MRSTRT7    */
#define PSTREAM_DMA_MRSIZE7     ( SDMAC_BASE + 0x000004F4 ) /* DMA_MRSIZE7    */
#define PSTREAM_DMA_MWSTRT7     ( SDMAC_BASE + 0x000004F8 ) /* DMA_MWSTRT7    */
#define PSTREAM_DMA_MWSIZE7     ( SDMAC_BASE + 0x000004FC ) /* DMA_MWSIZE7    */
#define PSTREAM_DMA_CNT8        ( SDMAC_BASE + 0x00000500 ) /* DMA_CNT8       */
#define PSTREAM_DMA_SIZE8       ( SDMAC_BASE + 0x00000504 ) /* DMA_SIZE8      */
#define PSTREAM_DMA_TRANS8      ( SDMAC_BASE + 0x00000508 ) /* DMA_TRANS8     */
#define PSTREAM_DMA_INTCNT8     ( SDMAC_BASE + 0x0000050C ) /* DMA_INTCNT8    */
#define PSTREAM_DMA_MRSTRT8     ( SDMAC_BASE + 0x00000510 ) /* DMA_MRSTRT8    */
#define PSTREAM_DMA_MRSIZE8     ( SDMAC_BASE + 0x00000514 ) /* DMA_MRSIZE8    */
#define PSTREAM_DMA_MWSTRT8     ( SDMAC_BASE + 0x00000518 ) /* DMA_MWSTRT8    */
#define PSTREAM_DMA_MWSIZE8     ( SDMAC_BASE + 0x0000051C ) /* DMA_MWSIZE8    */
#define PSTREAM_DMA_CNT9        ( SDMAC_BASE + 0x00000520 ) /* DMA_CNT9       */
#define PSTREAM_DMA_SIZE9       ( SDMAC_BASE + 0x00000524 ) /* DMA_SIZE9      */
#define PSTREAM_DMA_TRANS9      ( SDMAC_BASE + 0x00000528 ) /* DMA_TRANS9     */
#define PSTREAM_DMA_INTCNT9     ( SDMAC_BASE + 0x0000052C ) /* DMA_INTCNT9    */
#define PSTREAM_DMA_MRSTRT9     ( SDMAC_BASE + 0x00000530 ) /* DMA_MRSTRT9    */
#define PSTREAM_DMA_MRSIZE9     ( SDMAC_BASE + 0x00000534 ) /* DMA_MRSIZE9    */
#define PSTREAM_DMA_MWSTRT9     ( SDMAC_BASE + 0x00000538 ) /* DMA_MWSTRT9    */
#define PSTREAM_DMA_MWSIZE9     ( SDMAC_BASE + 0x0000053C ) /* DMA_MWSIZE9    */
#define PSTREAM_DMA_CNT10       ( SDMAC_BASE + 0x00000540 ) /* DMA_CNT10      */
#define PSTREAM_DMA_SIZE10      ( SDMAC_BASE + 0x00000544 ) /* DMA_SIZE10     */
#define PSTREAM_DMA_TRANS10     ( SDMAC_BASE + 0x00000548 ) /* DMA_TRANS10    */
#define PSTREAM_DMA_INTCNT10    ( SDMAC_BASE + 0x0000054C ) /* DMA_INTCNT10   */
#define PSTREAM_DMA_MRSTRT10    ( SDMAC_BASE + 0x00000550 ) /* DMA_MRSTRT10   */
#define PSTREAM_DMA_MRSIZE10    ( SDMAC_BASE + 0x00000554 ) /* DMA_MRSIZE10   */
#define PSTREAM_DMA_MWSTRT10    ( SDMAC_BASE + 0x00000558 ) /* DMA_MWSTRT10   */
#define PSTREAM_DMA_MWSIZE10    ( SDMAC_BASE + 0x0000055C ) /* DMA_MWSIZE10   */
#define PSTREAM_DMA_CNT11       ( SDMAC_BASE + 0x00000560 ) /* DMA_CNT11      */
#define PSTREAM_DMA_SIZE11      ( SDMAC_BASE + 0x00000564 ) /* DMA_SIZE11     */
#define PSTREAM_DMA_TRANS11     ( SDMAC_BASE + 0x00000568 ) /* DMA_TRANS11    */
#define PSTREAM_DMA_INTCNT11    ( SDMAC_BASE + 0x0000056C ) /* DMA_INTCNT11   */
#define PSTREAM_DMA_MRSTRT11    ( SDMAC_BASE + 0x00000570 ) /* DMA_MRSTRT11   */
#define PSTREAM_DMA_MRSIZE11    ( SDMAC_BASE + 0x00000574 ) /* DMA_MRSIZE11   */
#define PSTREAM_DMA_MWSTRT11    ( SDMAC_BASE + 0x00000578 ) /* DMA_MWSTRT11   */
#define PSTREAM_DMA_MWSIZE11    ( SDMAC_BASE + 0x0000057C ) /* DMA_MWSIZE11   */
#define PSTREAM_DMA_CNT12       ( SDMAC_BASE + 0x00000580 ) /* DMA_CNT12      */
#define PSTREAM_DMA_SIZE12      ( SDMAC_BASE + 0x00000584 ) /* DMA_SIZE12     */
#define PSTREAM_DMA_TRANS12     ( SDMAC_BASE + 0x00000588 ) /* DMA_TRANS12    */
#define PSTREAM_DMA_INTCNT12    ( SDMAC_BASE + 0x0000058C ) /* DMA_INTCNT12   */
#define PSTREAM_DMA_MRSTRT12    ( SDMAC_BASE + 0x00000590 ) /* DMA_MRSTRT12   */
#define PSTREAM_DMA_MRSIZE12    ( SDMAC_BASE + 0x00000594 ) /* DMA_MRSIZE12   */
#define PSTREAM_DMA_MWSTRT12    ( SDMAC_BASE + 0x00000598 ) /* DMA_MWSTRT12   */
#define PSTREAM_DMA_MWSIZE12    ( SDMAC_BASE + 0x0000059C ) /* DMA_MWSIZE12   */
#define PSTREAM_DMA_CNT13       ( SDMAC_BASE + 0x000005A0 ) /* DMA_CNT13      */
#define PSTREAM_DMA_SIZE13      ( SDMAC_BASE + 0x000005A4 ) /* DMA_SIZE13     */
#define PSTREAM_DMA_TRANS13     ( SDMAC_BASE + 0x000005A8 ) /* DMA_TRANS13    */
#define PSTREAM_DMA_INTCNT13    ( SDMAC_BASE + 0x000005AC ) /* DMA_INTCNT13   */
#define PSTREAM_DMA_MRSTRT13    ( SDMAC_BASE + 0x000005B0 ) /* DMA_MRSTRT13   */
#define PSTREAM_DMA_MRSIZE13    ( SDMAC_BASE + 0x000005B4 ) /* DMA_MRSIZE13   */
#define PSTREAM_DMA_MWSTRT13    ( SDMAC_BASE + 0x000005B8 ) /* DMA_MWSTRT13   */
#define PSTREAM_DMA_MWSIZE13    ( SDMAC_BASE + 0x000005BC ) /* DMA_MWSIZE13   */
#define PSTREAM_DMA_STS         ( SDMAC_BASE + 0x00000640 ) /* DMA_STS        */
#define PSTREAM_SCB_OPT         ( SDMAC_BASE + 0x00000E00 ) /* SCB_OPT        */
#define PSTREAM_CIPDM_OPT       ( SDMAC_BASE + 0x00000F00 ) /* CIPDM_OPT      */
#define PSTREAM_CIP_HEAD_LEN    ( SDMAC_BASE + 0x00000F04 ) /* CIP_HEAD_LEN   */
#define PSTREAM_CIP_FRM_LEN     ( SDMAC_BASE + 0x00000F08 ) /* CIP_FRM_LEN    */
#define PSTREAM_CIPDM_OPT2      ( SDMAC_BASE + 0x00000F40 ) /* CIPDM_OPT2     */
#define PSTREAM_CIP_HEAD_LEN2   ( SDMAC_BASE + 0x00000F44 ) /* CIP_HEAD_LEN2  */
#define PSTREAM_CIP_FRM_LEN2    ( SDMAC_BASE + 0x00000F48 ) /* CIP_FRM_LEN2   */
#define PSTREAM_SOFT_RST2       ( SDMAC_BASE + 0x00001004 ) /* SOFT_RST2      */
#define PSTREAM_CLK_CTRL        ( SDMAC_BASE + 0x00001008 ) /* CLK_CTRL       */
#define PSTREAM_CLK_CTRL2       ( SDMAC_BASE + 0x0000100C ) /* CLK_CTRL2      */
#define PSTREAM_CH_EVENT0       ( SDMAC_BASE + 0x00001100 ) /* CH_EVENT0      */
#define PSTREAM_CH_EVENT1       ( SDMAC_BASE + 0x00001104 ) /* CH_EVENT1      */
#define PSTREAM_CH_EVENT2       ( SDMAC_BASE + 0x00001108 ) /* CH_EVENT2      */
#define PSTREAM_CH_EVENT3       ( SDMAC_BASE + 0x0000110C ) /* CH_EVENT3      */
#define PSTREAM_CH_EVENT4       ( SDMAC_BASE + 0x00001110 ) /* CH_EVENT4      */
#define PSTREAM_CH_EVENT5       ( SDMAC_BASE + 0x00001114 ) /* CH_EVENT5      */
#define PSTREAM_CH_EVENT6       ( SDMAC_BASE + 0x00001118 ) /* CH_EVENT6      */
#define PSTREAM_CH_EVENT7       ( SDMAC_BASE + 0x0000111C ) /* CH_EVENT7      */
#define PSTREAM_CH_EVENT8       ( SDMAC_BASE + 0x00001120 ) /* CH_EVENT8      */
#define PSTREAM_CH_EVENT9       ( SDMAC_BASE + 0x00001124 ) /* CH_EVENT9      */
#define PSTREAM_CH_EVENT10      ( SDMAC_BASE + 0x00001128 ) /* CH_EVENT10     */
#define PSTREAM_CH_EVENT11      ( SDMAC_BASE + 0x0000112C ) /* CH_EVENT11     */
#define PSTREAM_CH_EVENT12      ( SDMAC_BASE + 0x00001130 ) /* CH_EVENT12     */
#define PSTREAM_CH_EVENT13      ( SDMAC_BASE + 0x00001134 ) /* CH_EVENT13     */


/*------- BSSI1          0xDC100000 ----------------------------------------*/
#define BSSI1_BASE          ( PUNIPHIER_BASE + 0x0C100000 ) /* BSSI1          */

#define PSTREAM_BSSI1TXCTL  ( BSSI1_BASE + 0x00000000 )     /* BSSI1TXCTL     */
#define PSTREAM_BSSI1TXFCTL ( BSSI1_BASE + 0x00000004 )     /* BSSI1TXFCTL    */
#define PSTREAM_BSSI1TXCKS  ( BSSI1_BASE + 0x00000008 )     /* BSSI1TXCKS     */
#define PSTREAM_BSSI1TXWDS  ( BSSI1_BASE + 0x0000000C )     /* BSSI1TXWDS     */
#define PSTREAM_BSSI1TXFPS  ( BSSI1_BASE + 0x00000014 )     /* BSSI1TXFPS     */
#define PSTREAM_BSSI1TXFS   ( BSSI1_BASE + 0x00000018 )     /* BSSI1TXFS      */
#define PSTREAM_BSSI1TXTOE  ( BSSI1_BASE + 0x00000020 )     /* BSSI1TXTOE     */
#define PSTREAM_BSSI1TXIE   ( BSSI1_BASE + 0x00000024 )     /* BSSI1TXIE      */
#define PSTREAM_BSSI1TXSR   ( BSSI1_BASE + 0x00000028 )     /* BSSI1TXSR      */
#define PSTREAM_BSSI1TXICR  ( BSSI1_BASE + 0x00000028 )     /* BSSI1TXICR     */
#define PSTREAM_BSSI1TXDR   ( BSSI1_BASE + 0x0000002C )     /* BSSI1TXDR      */
#define PSTREAM_BSSI1TXCNT  ( BSSI1_BASE + 0x00000030 )     /* BSSI1TXCNT     */
#define PSTREAM_BSSI1RXCTL  ( BSSI1_BASE + 0x00000100 )     /* BSSI1RXCTL     */
#define PSTREAM_BSSI1RXFCTL ( BSSI1_BASE + 0x00000104 )     /* BSSI1RXFCTL    */
#define PSTREAM_BSSI1RXCKS  ( BSSI1_BASE + 0x00000108 )     /* BSSI1RXCKS     */
#define PSTREAM_BSSI1RXWDS  ( BSSI1_BASE + 0x0000010C )     /* BSSI1RXWDS     */
#define PSTREAM_BSSI1RXFPS  ( BSSI1_BASE + 0x00000114 )     /* BSSI1RXFPS     */
#define PSTREAM_BSSI1RXFS   ( BSSI1_BASE + 0x00000118 )     /* BSSI1RXFS      */
#define PSTREAM_BSSI1RXTOE  ( BSSI1_BASE + 0x00000120 )     /* BSSI1RXTOE     */
#define PSTREAM_BSSI1RXCNT  ( BSSI1_BASE + 0x00000124 )     /* BSSI1RXCNT     */
#define PSTREAM_BSSI1NRXCNT ( BSSI1_BASE + 0x00000128 )     /* BSSI1NRXCNT    */
#define PSTREAM_BSSI1RXIE   ( BSSI1_BASE + 0x0000012C )     /* BSSI1RXIE      */
#define PSTREAM_BSSI1RXSR   ( BSSI1_BASE + 0x00000130 )     /* BSSI1RXSR      */
#define PSTREAM_BSSI1RXICR  ( BSSI1_BASE + 0x00000130 )     /* BSSI1RXICR     */
#define PSTREAM_BSSI1RXDR   ( BSSI1_BASE + 0x00000134 )     /* BSSI1RXDR      */


/*------- BSSI2          0xDC110000 ----------------------------------------*/
#define BSSI2_BASE          ( PUNIPHIER_BASE + 0x0C110000 ) /* BSSI2          */

#define PSTREAM_BSSI2TXCTL  ( BSSI2_BASE + 0x00000000 )     /* BSSI2TXCTL     */
#define PSTREAM_BSSI2TXFCTL ( BSSI2_BASE + 0x00000004 )     /* BSSI2TXFCTL    */
#define PSTREAM_BSSI2TXCKS  ( BSSI2_BASE + 0x00000008 )     /* BSSI2TXCKS     */
#define PSTREAM_BSSI2TXWDS  ( BSSI2_BASE + 0x0000000C )     /* BSSI2TXWDS     */
#define PSTREAM_BSSI2TXFPS  ( BSSI2_BASE + 0x00000014 )     /* BSSI2TXFPS     */
#define PSTREAM_BSSI2TXFS   ( BSSI2_BASE + 0x00000018 )     /* BSSI2TXFS      */
#define PSTREAM_BSSI2TXTOE  ( BSSI2_BASE + 0x00000020 )     /* BSSI2TXTOE     */
#define PSTREAM_BSSI2TXIE   ( BSSI2_BASE + 0x00000024 )     /* BSSI2TXIE      */
#define PSTREAM_BSSI2TXSR   ( BSSI2_BASE + 0x00000028 )     /* BSSI2TXSR      */
#define PSTREAM_BSSI2TXICR  ( BSSI2_BASE + 0x00000028 )     /* BSSI2TXICR     */
#define PSTREAM_BSSI2TXDR   ( BSSI2_BASE + 0x0000002C )     /* BSSI2TXDR      */
#define PSTREAM_BSSI2TXCNT  ( BSSI2_BASE + 0x00000030 )     /* BSSI2TXCNT     */
#define PSTREAM_BSSI2RXCTL  ( BSSI2_BASE + 0x00000100 )     /* BSSI2RXCTL     */
#define PSTREAM_BSSI2RXFCTL ( BSSI2_BASE + 0x00000104 )     /* BSSI2RXFCTL    */
#define PSTREAM_BSSI2RXCKS  ( BSSI2_BASE + 0x00000108 )     /* BSSI2RXCKS     */
#define PSTREAM_BSSI2RXWDS  ( BSSI2_BASE + 0x0000010C )     /* BSSI2RXWDS     */
#define PSTREAM_BSSI2RXFPS  ( BSSI2_BASE + 0x00000114 )     /* BSSI2RXFPS     */
#define PSTREAM_BSSI2RXFS   ( BSSI2_BASE + 0x00000118 )     /* BSSI2RXFS      */
#define PSTREAM_BSSI2RXTOE  ( BSSI2_BASE + 0x00000120 )     /* BSSI2RXTOE     */
#define PSTREAM_BSSI2RXCNT  ( BSSI2_BASE + 0x00000124 )     /* BSSI2RXCNT     */
#define PSTREAM_BSSI2NRXCNT ( BSSI2_BASE + 0x00000128 )     /* BSSI2NRXCNT    */
#define PSTREAM_BSSI2RXIE   ( BSSI2_BASE + 0x0000012C )     /* BSSI2RXIE      */
#define PSTREAM_BSSI2RXSR   ( BSSI2_BASE + 0x00000130 )     /* BSSI2RXSR      */
#define PSTREAM_BSSI2RXICR  ( BSSI2_BASE + 0x00000130 )     /* BSSI2RXICR     */
#define PSTREAM_BSSI2RXDR   ( BSSI2_BASE + 0x00000134 )     /* BSSI2RXDR      */


/*------- SDIO           0xDC120000 ----------------------------------------*/
#define SDIO_BASE           ( PUNIPHIER_BASE + 0x0C120000 ) /* SDIO           */

#define PSTREAM_SD_CMD           ( SDIO_BASE + 0x00000000 ) /* SD_CMD         */
#define PSTREAM_SD_PORTSEL       ( SDIO_BASE + 0x00000002 ) /* SD_PORTSEL     */
#define PSTREAM_SD_ARG0          ( SDIO_BASE + 0x00000004 ) /* SD_ARG0        */
#define PSTREAM_SD_ARG1          ( SDIO_BASE + 0x00000006 ) /* SD_ARG1        */
#define PSTREAM_SD_STOP          ( SDIO_BASE + 0x00000008 ) /* SD_STOP        */
#define PSTREAM_SD_SECCNT        ( SDIO_BASE + 0x0000000A ) /* SD_SECCNT      */
#define PSTREAM_SD_RSP0          ( SDIO_BASE + 0x0000000C ) /* SD_RSP0        */
#define PSTREAM_SD_RSP1          ( SDIO_BASE + 0x0000000E ) /* SD_RSP1        */
#define PSTREAM_SD_RSP2          ( SDIO_BASE + 0x00000010 ) /* SD_RSP2        */
#define PSTREAM_SD_RSP3          ( SDIO_BASE + 0x00000012 ) /* SD_RSP3        */
#define PSTREAM_SD_RSP4          ( SDIO_BASE + 0x00000014 ) /* SD_RSP4        */
#define PSTREAM_SD_RSP5          ( SDIO_BASE + 0x00000016 ) /* SD_RSP5        */
#define PSTREAM_SD_RSP6          ( SDIO_BASE + 0x00000018 ) /* SD_RSP6        */
#define PSTREAM_SD_RSP7          ( SDIO_BASE + 0x0000001A ) /* SD_RSP7        */
#define PSTREAM_SD_INFO1         ( SDIO_BASE + 0x0000001C ) /* SD_INFO1       */
#define PSTREAM_SD_INFO2         ( SDIO_BASE + 0x0000001E ) /* SD_INFO2       */
#define PSTREAM_SD_INFO1_MASK    ( SDIO_BASE + 0x00000020 ) /* SD_INFO1_MASK  */
#define PSTREAM_SD_INFO2_MASK    ( SDIO_BASE + 0x00000022 ) /* SD_INFO2_MASK  */
#define PSTREAM_SD_CLK_CTRL      ( SDIO_BASE + 0x00000024 ) /* SD_CLK_CTRL    */
#define PSTREAM_SD_SIZE          ( SDIO_BASE + 0x00000026 ) /* SD_SIZE        */
#define PSTREAM_SD_OPTION        ( SDIO_BASE + 0x00000028 ) /* SD_OPTION      */
#define PSTREAM_SD_ERR_STS1      ( SDIO_BASE + 0x0000002C ) /* SD_ERR_STS1    */
#define PSTREAM_SD_ERR_STS2      ( SDIO_BASE + 0x0000002E ) /* SD_ERR_STS2    */
#define PSTREAM_SD_BUF0          ( SDIO_BASE + 0x00000030 ) /* SD_BUF0        */
#define PSTREAM_SDIO_MODE        ( SDIO_BASE + 0x00000034 ) /* SDIO_MODE      */
#define PSTREAM_SDIO_INFO1       ( SDIO_BASE + 0x00000036 ) /* SDIO_INFO1     */
#define PSTREAM_SDIO_INFO1_MASK  ( SDIO_BASE + 0x00000038 ) /* SDIO_INFO1_MASK*/
#define PSTREAM_CC_EXT_MODE      ( SDIO_BASE + 0x000000D8 ) /* CC_EXT_MODE    */
#define PSTREAM_SOFT_RST         ( SDIO_BASE + 0x000000E0 ) /* SOFT_RST       */
#define PSTREAM_VERSION          ( SDIO_BASE + 0x000000E2 ) /* VERSION        */
#define PSTREAM_SD_PWR           ( SDIO_BASE + 0x000000F2 ) /* SD_PWR         */
#define PSTREAM_EXT_SDIO         ( SDIO_BASE + 0x000000F4 ) /* EXT_SDIO       */
#define PSTREAM_EXT_WP           ( SDIO_BASE + 0x000000F6 ) /* EXT_WP         */
#define PSTREAM_EXT_CD           ( SDIO_BASE + 0x000000F8 ) /* EXT_CD         */
#define PSTREAM_EXT_CD_DAT3      ( SDIO_BASE + 0x000000FA ) /* EXT_CD_DAT3    */
#define PSTREAM_EXT_CD_MASK      ( SDIO_BASE + 0x000000FC ) /* EXT_CD_MASK    */
#define PSTREAM_EXT_CD_DAT3_MASK ( SDIO_BASE + 0x000000FE ) /* EXT_CD_DAT3_MAS*/


/*------- USB            0xDC140000 ----------------------------------------*/
#define USB_BASE            ( PUNIPHIER_BASE + 0x0C140000 ) /* USB            */

#define PSTREAM_USBD_FUNCTION_ADDRESS  ( USB_BASE + 0x00000000 ) /* USBD_FUNCTION_ADDRESS  */
#define PSTREAM_USBD_POWER_MANAGEMENT  ( USB_BASE + 0x00000004 ) /* USBD_POWER_MANAGEMENT  */
#define PSTREAM_USBD_GEN_INT           ( USB_BASE + 0x00000008 ) /* USBD_GEN_INT           */
#define PSTREAM_USBD_DMA_INT           ( USB_BASE + 0x0000000C ) /* USBD_DMA_INT           */
#define PSTREAM_USBD_MSOF_STS          ( USB_BASE + 0x00000010 ) /* USBD_MSOF_STS          */
#define PSTREAM_USBD_POWER_INT         ( USB_BASE + 0x00000014 ) /* USBD_POWER_INT         */
#define PSTREAM_USBD_GEN_INT_EN        ( USB_BASE + 0x00000018 ) /* USBD_GEN_INT_EN        */
#define PSTREAM_USBD_DMA_INT_EN        ( USB_BASE + 0x0000001C ) /* USBD_DMA_INT_EN        */
#define PSTREAM_USBD_MSOF_STS_EN       ( USB_BASE + 0x00000020 ) /* USBD_MSOF_STS_EN       */
#define PSTREAM_USBD_POWER_INT_EN      ( USB_BASE + 0x00000024 ) /* USBD_POWER_INT_EN      */
#define PSTREAM_USBD_FRAME_NUMBER      ( USB_BASE + 0x00000028 ) /* USBD_FRAME_NUMBER      */
#define PSTREAM_USBD_INDEX             ( USB_BASE + 0x0000002C ) /* USBD_INDEX             */
#define PSTREAM_USBD_DMA_START         ( USB_BASE + 0x00000030 ) /* USBD_DMA_START         */
#define PSTREAM_USBD_MAXP              ( USB_BASE + 0x00000034 ) /* USBD_MAXP              */
#define PSTREAM_USBD_BUF_BLK           ( USB_BASE + 0x00000038 ) /* USBD_BUF_BLK           */
#define PSTREAM_USBD_IN_CSR1           ( USB_BASE + 0x0000003C ) /* USBD_IN_CSR1           */
#define PSTREAM_USBD_IN_CSR2           ( USB_BASE + 0x00000040 ) /* USBD_IN_CSR2           */
#define PSTREAM_USBD_IN_CSR3           ( USB_BASE + 0x00000044 ) /* USBD_IN_CSR3           */
#define PSTREAM_USBD_OUT_CSR1          ( USB_BASE + 0x00000048 ) /* USBD_OUT_CSR1          */
#define PSTREAM_USBD_OUT_CSR2          ( USB_BASE + 0x0000004C ) /* USBD_OUT_CSR2          */
#define PSTREAM_USBD_OUT_CSR3          ( USB_BASE + 0x00000050 ) /* USBD_OUT_CSR3          */
#define PSTREAM_USBD_OUT_WRITE_COUNT   ( USB_BASE + 0x00000054 ) /* USBD_OUT_WRITE_COUNT   */
#define PSTREAM_USBD_EP0_FIFO          ( USB_BASE + 0x00000060 ) /* USBD_EP0_FIFO          */
#define PSTREAM_USBD_EP1_FIFO          ( USB_BASE + 0x00000064 ) /* USBD_EP1_FIFO          */
#define PSTREAM_USBD_EP2_FIFO          ( USB_BASE + 0x00000068 ) /* USBD_EP2_FIFO          */
#define PSTREAM_USBD_EP3_FIFO          ( USB_BASE + 0x0000006C ) /* USBD_EP3_FIFO          */
#define PSTREAM_USBD_EP4_FIFO          ( USB_BASE + 0x00000070 ) /* USBD_EP4_FIFO          */
#define PSTREAM_USBD_EP5_FIFO          ( USB_BASE + 0x00000074 ) /* USBD_EP5_FIFO          */
#define PSTREAM_USBD_EP6_FIFO          ( USB_BASE + 0x00000078 ) /* USBD_EP6_FIFO          */
#define PSTREAM_USBD_EP7_FIFO          ( USB_BASE + 0x0000007C ) /* USBD_EP7_FIFO          */
#define PSTREAM_USBD_EP8_FIFO          ( USB_BASE + 0x00000080 ) /* USBD_EP8_FIFO          */
#define PSTREAM_USBD_EP9_FIFO          ( USB_BASE + 0x00000084 ) /* USBD_EP9_FIFO          */
#define PSTREAM_USBD_EP10_FIFO         ( USB_BASE + 0x00000088 ) /* USBD_EP10_FIFO         */
#define PSTREAM_USBD_EP11_FIFO         ( USB_BASE + 0x0000008C ) /* USBD_EP11_FIFO         */
#define PSTREAM_USBD_EP12_FIFO         ( USB_BASE + 0x00000090 ) /* USBD_EP12_FIFO         */
#define PSTREAM_USBD_EP13_FIFO         ( USB_BASE + 0x00000094 ) /* USBD_EP13_FIFO         */
#define PSTREAM_USBD_EP14_FIFO         ( USB_BASE + 0x00000098 ) /* USBD_EP14_FIFO         */
#define PSTREAM_USBD_EP15_FIFO         ( USB_BASE + 0x0000009C ) /* USBD_EP15_FIFO         */
#define PSTREAM_USBD_ISO_LEN1_EP       ( USB_BASE + 0x000000A0 ) /* USBD_ISO_LEN1_EP       */
#define PSTREAM_USBD_ISO_LEN1_LENGTH   ( USB_BASE + 0x000000A4 ) /* USBD_ISO_LEN1_LENGTH   */
#define PSTREAM_USBD_ISO_LEN2_EP       ( USB_BASE + 0x000000A8 ) /* USBD_ISO_LEN2_EP       */
#define PSTREAM_USBD_ISO_LEN2_LENGTH   ( USB_BASE + 0x000000AC ) /* USBD_ISO_LEN2_LENGTH   */
#define PSTREAM_USBD_ISO_LEN3_EP       ( USB_BASE + 0x000000B0 ) /* USBD_ISO_LEN3_EP       */
#define PSTREAM_USBD_ISO_LEN3_LENGTH   ( USB_BASE + 0x000000B4 ) /* USBD_ISO_LEN3_LENGTH   */
#define PSTREAM_USBD_ISO_LEN4_EP       ( USB_BASE + 0x000000B8 ) /* USBD_ISO_LEN4_EP       */
#define PSTREAM_USBD_ISO_LEN4_LENGTH   ( USB_BASE + 0x000000BC ) /* USBD_ISO_LEN4_LENGTH   */
#define PSTREAM_USBD_ISO_LEN5_EP       ( USB_BASE + 0x000000C0 ) /* USBD_ISO_LEN5_EP       */
#define PSTREAM_USBD_ISO_LEN5_LENGTH   ( USB_BASE + 0x000000C4 ) /* USBD_ISO_LEN5_LENGTH   */
#define PSTREAM_USBD_ISO_LEN6_EP       ( USB_BASE + 0x000000C8 ) /* USBD_ISO_LEN6_EP       */
#define PSTREAM_USBD_ISO_LEN6_LENGTH   ( USB_BASE + 0x000000CC ) /* USBD_ISO_LEN6_LENGTH   */
#define PSTREAM_USBD_ISO_LEN7_EP       ( USB_BASE + 0x000000D0 ) /* USBD_ISO_LEN7_EP       */
#define PSTREAM_USBD_ISO_LEN7_LENGTH   ( USB_BASE + 0x000000D4 ) /* USBD_ISO_LEN7_LENGTH   */
#define PSTREAM_USBD_ISO_LEN8_EP       ( USB_BASE + 0x000000D8 ) /* USBD_ISO_LEN8_EP       */
#define PSTREAM_USBD_ISO_LEN8_LENGTH   ( USB_BASE + 0x000000DC ) /* USBD_ISO_LEN8_LENGTH   */
#define PSTREAM_USBD_ISO_LEN9_EP       ( USB_BASE + 0x000000E0 ) /* USBD_ISO_LEN9_EP       */
#define PSTREAM_USBD_ISO_LEN9_LENGTH   ( USB_BASE + 0x000000E4 ) /* USBD_ISO_LEN9_LENGTH   */
#define PSTREAM_USBD_ISO_LEN10_EP      ( USB_BASE + 0x000000E8 ) /* USBD_ISO_LEN10_EP      */
#define PSTREAM_USBD_ISO_LEN10_LENGTH  ( USB_BASE + 0x000000EC ) /* USBD_ISO_LEN10_LENGTH  */
#define PSTREAM_USBD_ISO_LEN11_EP      ( USB_BASE + 0x000000F0 ) /* USBD_ISO_LEN11_EP      */
#define PSTREAM_USBD_ISO_LEN11_LENGTH  ( USB_BASE + 0x000000F4 ) /* USBD_ISO_LEN11_LENGTH  */
#define PSTREAM_USBD_ISO_LEN12_EP      ( USB_BASE + 0x000000F8 ) /* USBD_ISO_LEN12_EP      */
#define PSTREAM_USBD_ISO_LEN12_LENGTH  ( USB_BASE + 0x000000FC ) /* USBD_ISO_LEN12_LENGTH  */
#define PSTREAM_USBD_ISO_LEN13_EP      ( USB_BASE + 0x00000100 ) /* USBD_ISO_LEN13_EP      */
#define PSTREAM_USBD_ISO_LEN13_LENGTH  ( USB_BASE + 0x00000104 ) /* USBD_ISO_LEN13_LENGTH  */
#define PSTREAM_USBD_ISO_LEN14_EP      ( USB_BASE + 0x00000108 ) /* USBD_ISO_LEN14_EP      */
#define PSTREAM_USBD_ISO_LEN14_LENGTH  ( USB_BASE + 0x0000010C ) /* USBD_ISO_LEN14_LENGTH  */
#define PSTREAM_USBD_ISO_LEN15_EP      ( USB_BASE + 0x00000110 ) /* USBD_ISO_LEN15_EP      */
#define PSTREAM_USBD_ISO_LEN15_LENGTH  ( USB_BASE + 0x00000114 ) /* USBD_ISO_LEN15_LENGTH  */
#define PSTREAM_USBD_TEST_MODE1        ( USB_BASE + 0x00000118 ) /* USBD_TEST_MODE1        */
#define PSTREAM_USBD_TEST_MODE2        ( USB_BASE + 0x0000011C ) /* USBD_TEST_MODE2        */
#define PSTREAM_USBD_UPDET_CONTROL     ( USB_BASE + 0x00000120 ) /* USBD_UPDET_CONTROL     */
#define PSTREAM_USBD_NUMBER_OF_SOF     ( USB_BASE + 0x00000128 ) /* USBD_NUMBER_OF_SOF     */
#define PSTREAM_USBD_MAX_NUMBER_OF_SOF ( USB_BASE + 0x0000012C ) /* USBD_MAX_NUMBER_OF_SOF */
#define PSTREAM_USBD_MIN_NUMBER_OF_SOF ( USB_BASE + 0x00000130 ) /* USBD_MIN_NUMBER_OF_SOF */
#define PSTREAM_USBD_INTERVAL_M        ( USB_BASE + 0x00000134 ) /* USBD_INTERVAL_M        */
#define PSTREAM_USBD_FIFO_TEST1        ( USB_BASE + 0x00000138 ) /* USBD_FIFO_TEST1        */
#define PSTREAM_USBD_FIFO_TEST2        ( USB_BASE + 0x0000013C ) /* USBD_FIFO_TEST2        */
#define PSTREAM_USBD_FIFO_TEST3        ( USB_BASE + 0x00000140 ) /* USBD_FIFO_TEST3        */
#define PSTREAM_USBD_HVER_USB          ( USB_BASE + 0x00000200 ) /* USBD_HVER USB          */
#define PSTREAM_USBD_HARD_OPTION1      ( USB_BASE + 0x00000204 ) /* USBD_HARD_OPTION1      */
#define PSTREAM_USBD_HARD_OPTION2      ( USB_BASE + 0x00000208 ) /* USBD_HARD_OPTION2      */
#define PSTREAM_USB_CFG_CH0            ( USB_BASE + 0x0000020C ) /* USB_CFG_CH0            */
#define PSTREAM_USB_CFG_CH1            ( USB_BASE + 0x00000210 ) /* USB_CFG_CH1            */
#define PSTREAM_USB_CLK_GEN_CTL        ( USB_BASE + 0x00000220 ) /* USB_CLK_GEN_CTL        */
#define PSTREAM_USB_SEL_TEST           ( USB_BASE + 0x00000224 ) /* USB_SEL_TEST           */
#define PSTREAM_USB_VERSION            ( USB_BASE + 0x00000228 ) /* USB_Version            */


#endif                                           /* _UNIPHIER_ADDR_H_         */
/******************************************************************************/
/*         Unpublished Copyright(c)  2005             */
/******************************************************************************/
