/******************************************************************************/
/*          :                                     */
/*        :  port_rw.h                                              */
/*        :  2004.04.05                                             */
/*            :                                                 */
/*          :                                                     */
/*                                                                            */
/*         Unpublished Copyright(c)  2004             */
/******************************************************************************/
/*****  ****************************************************/
/*  No.                                       */
/*                                                                            */
/*                                04.04.05 H.Nakajima */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _PORT_RW_H_                              /* _PORT_RW_H_               */
#define _PORT_RW_H_

/*----------------------------------------------------------------------------*/
/*                                                    */
/*----------------------------------------------------------------------------*/
#define    PORTINB( addr ) ( *(( volatile unsigned char *  )( addr )))
#define    PORTINW( addr ) ( *(( volatile unsigned short * )( addr )))
#define    PORTINL( addr ) ( *(( volatile unsigned long *  )( addr )))

/*----------------------------------------------------------------------------*/
/*                                                    */
/*----------------------------------------------------------------------------*/
#define    PORTOUTB( addr,data ) \
           ( *(( volatile unsigned char *  )( addr )) = ( data ))
#define    PORTOUTW( addr,data ) \
           ( *(( volatile unsigned short * )( addr )) = ( data ))
#define    PORTOUTL( addr,data ) \
           ( *(( volatile unsigned long *  )( addr )) = ( data ))

/*----------------------------------------------------------------------------*/
/*                                              */
/*----------------------------------------------------------------------------*/
#define    PORTBSETB( addr,data ) \
           ( *(( volatile unsigned char *  )( addr )) |= ( data ))
#define    PORTBSETW( addr,data ) \
           ( *(( volatile unsigned short * )( addr )) |= ( data ))
#define    PORTBSETL( addr,data ) \
           ( *(( volatile unsigned long *  )( addr )) |= ( data ))

/*----------------------------------------------------------------------------*/
/*                                              */
/*----------------------------------------------------------------------------*/
#define    PORTBCLRB( addr,data ) \
           ( *(( volatile unsigned char *  )( addr )) &= ~( data ))
#define    PORTBCLRW( addr,data ) \
           ( *(( volatile unsigned short * )( addr )) &= ~( data ))
#define    PORTBCLRL( addr,data ) \
           ( *(( volatile unsigned long *  )( addr )) &= ~( data ))


#endif                                           /* _PORT_RW_H_               */
/******************************************************************************/
/*         Unpublished Copyright(c)  2004             */
/******************************************************************************/
