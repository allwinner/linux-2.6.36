/* $Id: transport_platform.h 9119 2008-06-09 11:50:52Z joda $ */

#ifndef _TRANSPORT_PLATFORM_H_
#define _TRANSPORT_PLATFORM_H_

#ifndef WIN32_LEAN_AND_MEAN
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
#endif

#include "transport.h"

//
// Insert Host-specific includes here
//
#include <tcc78x_intr.h>

//
// Insert Host-specific defines here
//
#define IRQ_NUMBER  IRQ_EINT3

#define TRANSPORT_MIN_PDU_SIZE 16
#define TRANSPORT_PDU_SIZE_ALIGNMENT 4
#define TRANSPORT_RX_HDR_SIZE 24
#define TRANSPORT_TX_HDR_SIZE 32
//IRQ-type 0 is "XP-IRQ"
//IRQ-type 1 is SDIO-standard
#define TRANSPORT_IRQ_TYPE 0

//--------------------Global variables--------------------

extern transport_free_send_buffer   g_FreeSendBuffer;
extern from_transport_receive_t     g_IndicateReceive;
extern int                          g_IsReady;
extern CRITICAL_SECTION             g_RxTxLock;
extern CRITICAL_SECTION             g_TxLock;



#endif /*_TRANSPORT_PLATFORM_H_*/
