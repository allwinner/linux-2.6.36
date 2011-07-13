#ifndef SUN3I_SPI_NOR_COMMON_H
#define SUN3I_SPI_NOR_COMMON_H

//define the type of spi nor flash
//#define ST_M25P16
//#define ST_M25P32
//#define ST_M25P64
//#define SPANSION_S25FL064A
//#define SPANSION_S25FL032A
#define SPANSION_S25FL008A
//#define SPANSION_S25FL016A
//#define SPANSION_S25FL040A
//#define ATMEL_AT26DF321
//#define AMIC_A25L40PUM
//#define AMIC_A25L16PUM
//#define AMIC_A25L016
//#define AMIC_A25L80P
//#define AMIC_A25L080
//#define SST_25VF016B
//#define SST_25VF080B
//#define EON_EN25F80
//#define EON_EN25B16
//#define EON_EN25B32
//#define WINBOND_25x16VSIG
//#define WINBOND_25x80VSIG

#ifdef WR_LONG_DATA
#define WDATA_LEN 300
#elif defined WR_SHORT_DATA
#define WDATA_LEN 200
#else
#define WDATA_LEN 256
#endif

//define the memory organization
#ifdef ST_M25P16
#define PAGE_SIZE 256   //Byte
#define SECTOR_SIZE (512*1024) //bit
#define BULK_SIZE (16*1024*1024)   //Mbit
#elif defined ST_M25P32
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (32*1024*1024)
#elif defined ST_M25P64
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (64*1024*1024)
#elif defined SPANSION_S25FL064A
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (64*1024*1024)
#elif defined SPANSION_S25FL032A
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (32*1024*1024)
#elif defined SPANSION_S25FL008A
#define PAGE_SIZE_ 256   // page_size
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (8*1024*1024)
#elif defined SPANSION_S25FL016A
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (16*1024*1024)
#elif defined SPANSION_S25FL040A
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (4*1024*1024)
#elif defined ATMEL_AT26DF321
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (32*1024*1024)
#elif defined AMIC_A25L40PUM
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (4*1024*1024)
#elif defined AMIC_A25L16PUM
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (16*1024*1024)
#elif defined AMIC_A25L016
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (16*1024*1024)
#elif defined AMIC_A25L80P
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (8*1024*1024)
#elif defined AMIC_A25L080//??
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (8*1024*1024)
#elif defined SST_25VF016B//??
#define SST_OP
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (16*1024*1024)
#elif defined SST_25VF080B//no datasheet
#define SST_OP
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (8*1024*1024)
#elif defined EON_EN25F80
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (8*1024*1024)
#elif defined EON_EN25B16
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (16*1024*1024)
#elif defined EON_EN25B32
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (32*1024*1024)
#elif defined WINBOND_25x16VSIG
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (16*1024*1024)
#elif defined WINBOND_25x80VSIG
#define PAGE_SIZE 256
#define SECTOR_SIZE (512*1024)
#define BULK_SIZE (8*1024*1024)
#endif


#endif
