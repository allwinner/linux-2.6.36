/* $Id: spi_control.h 9119 2008-06-09 11:50:52Z joda $ */
/*****************************************************************************

Copyright (c) 2006 by Nanoradio AB

This software is copyrighted by and is the sole property of Nanoradio AB.
All rights, title, ownership, or other interests in the
software remain the property of Nanoradio AB.  This software may
only be used in accordance with the corresponding license agreement.  Any
unauthorized use, duplication, transmission, distribution, or disclosure of
this software is expressly forbidden.

This Copyright notice may not be removed or modified without prior written
consent of Nanoradio AB.

Nanoradio AB reserves the right to modify this software without
notice.

Nanoradio AB
Torshamnsgatan 39                       info@nanoradio.se
164 40 Kista                            http://www.nanoradio.se
SWEDEN

Module Description :
==================


*****************************************************************************/

#ifndef SPI_CONTROL_H_
#define SPI_CONTROL_H_


unsigned int    spi_initialize(void);
void            spi_deinitialize(void);
void            spi_stop(void);
void            spi_start(void);
unsigned int    spi_send_dataNoDMA(unsigned char* data, unsigned int size);
unsigned int    spi_read_dataNoDMA(unsigned char* data, unsigned int size);

unsigned int spi_map_registers(void);
void         spi_unmap_registers(void);
void         spi_config_registers(void);

unsigned int    spi_sendCMD0(void);
unsigned int    spi_sendCMD52(unsigned int* data, unsigned int * rFlags);
unsigned int    spi_readCMD53(unsigned char* data, unsigned int size);
unsigned int    spi_sendCMD53(unsigned char* data, unsigned int size);
#endif //SPI_CONTROL_H_
