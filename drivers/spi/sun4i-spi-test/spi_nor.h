#ifndef SUN3I_SPI_NOR_H
#define SUN3I_SPI_NOR_H


typedef struct identification
{
	#ifdef LONG_ID
	u8 uid;
	u8 cfi[16];
	#endif
	u8 mid;
	u16 did;
} nor_id;

#define SPI_NOR_READ  0x03
#define SPI_NOR_FREAD 0x0b
#define SPI_NOR_WREN  0x06
#define SPI_NOR_WRDI  0x04
#define SPI_NOR_RDSR  0x05
#define SPI_NOR_WRSR  0x01
#define SPI_NOR_PP    0x02
#define SPI_NOR_SE    0xd8
#define SPI_NOR_BE    0xc7

//#define SPI_NOR_RDID  0x90 
#define SPI_NOR_RDID  0x9F 

#define SST_AAI_PRG   0xad

/* new
#define SPI_NOR_READ  0x03
#define SPI_NOR_FREAD 0x0b
#define SPI_NOR_RAPIDS_READ 0x1b
#define SPI_NOR_WREN  0x06
#define SPI_NOR_WRDI  0x04
#define SPI_NOR_RDSR  0x05
#define SPI_NOR_WRSR  0x01
#define SPI_NOR_WRSR2 0x31
#define SPI_NOR_PP    0x02
#define SPI_NOR_SE    0xd8
#define SPI_NOR_BE    0xc7

#define SPI_NOR_RDID  0x9f
#define SST_AAI_PRG   0xad
#define SPI_NOR_RESET 0xF0
#define SPI_NOR_RESET_CFM   0xd0
*/


extern s32 spic_rw(u32 tcnt, u8* txbuf,u32 rcnt, u8* rxbuf);

extern s32 spi_nor_rdid(u32* id);
extern s32 spi_nor_read(u32 page_num, u32 page_cnt, void* buf);
extern s32 spi_nor_write(u32 page_num, u32 page_cnt, void* buf);
extern s32 spi_nor_sector_erase(u32 page_num);
extern s32 spi_nor_bulk_erase(void);
extern s32 spi_nor_rw_test(void);



#endif 
