#include <linux/types.h>


#define MMC_BASE 0xFFFB7800

#define MMC_CMD     ((volatile uint16_t*) (MMC_BASE + 0x00))
#define CMD_DDIR  (1 << 15)
#define CMD_TYPE1 (1 << 13)
#define CMD_TYPE0 (1 << 12)
#define CMD_BUSY  (1 << 11)
#define CMD_RSP2  (1 << 10)
#define CMD_RSP1  (1 << 9)
#define CMD_RSP0  (1 << 8)
#define CMD_INAB  (1 << 7)
#define CMD_ODTO  (1 << 6)

#define MMC_ARGL    ((volatile uint16_t*) (MMC_BASE + 0x04))

#define MMC_ARGH    ((volatile uint16_t*) (MMC_BASE + 0x08))

#define MMC_CON     ((volatile uint16_t*) (MMC_BASE + 0x0C))
#define CON_POWER_UP        (1 << 11)

#define MMC_STAT    ((volatile uint16_t*) (MMC_BASE + 0x10))
#define STAT_CERR           (1 << 14)
#define STAT_CIRQ           (1 << 13)
#define STAT_OCRB           (1 << 12)
#define STAT_AE             (1 << 11)
#define STAT_AF             (1 << 10)
#define STAT_CRW            (1 << 9)
#define STAT_CCRC           (1 << 8)
#define STAT_CTO            (1 << 7)
#define STAT_DCRC           (1 << 6)
#define STAT_DTO            (1 << 5)
#define STAT_EOFB           (1 << 4)
#define STAT_BRS            (1 << 3)
#define STAT_CB             (1 << 2)
#define STAT_CD             (1 << 1)
#define STAT_EOC            (1 << 0)


#define MMC_IE      ((volatile uint16_t*) (MMC_BASE + 0x14))
/* Use status bits STAT_* */

#define MMC_CTO     ((volatile uint16_t*) (MMC_BASE + 0x18))

#define MMC_DTO     ((volatile uint16_t*) (MMC_BASE + 0x1C))

#define MMC_DATA    ((volatile uint16_t*) (MMC_BASE + 0x20))

#define MMC_BLEN    ((volatile uint16_t*) (MMC_BASE + 0x24))

#define MMC_NBLK    ((volatile uint16_t*) (MMC_BASE + 0x28))

#define MMC_BUF     ((volatile uint16_t*) (MMC_BASE + 0x2C))
#define BUF_RXDE               (1 << 15)
#define BUF_AFL4               (1 << 12)
#define BUF_AFL3               (1 << 11)
#define BUF_AFL2               (1 << 10)
#define BUF_AFL1               (1 << 9)
#define BUF_AFL0               (1 << 8)

#define BUF_TXDE               (1 << 7)
#define BUF_AEL4               (1 << 4)
#define BUF_AEL3               (1 << 3)
#define BUF_AEL2               (1 << 2)
#define BUF_AEL1               (1 << 1)
#define BUF_AEL0               (1 << 0)


#define MMC_SPI     ((volatile uint16_t*) (MMC_BASE + 0x30))

#define MMC_SDIO    ((volatile uint16_t*) (MMC_BASE + 0x34))
#define SDIO_IRQE              (1 << 0)

#define MMC_SYST    ((volatile uint16_t*) (MMC_BASE + 0x38))
#define MMC_REV     ((volatile uint16_t*) (MMC_BASE + 0x3C))
#define MMC_RSP0    ((volatile uint16_t*) (MMC_BASE + 0x40))

#define MMC_RSP1    ((volatile uint16_t*) (MMC_BASE + 0x44))

#define MMC_RSP2    ((volatile uint16_t*) (MMC_BASE + 0x48))

#define MMC_RSP3    ((volatile uint16_t*) (MMC_BASE + 0x4C))

#define MMC_RSP4    ((volatile uint16_t*) (MMC_BASE + 0x50))

#define MMC_RSP5    ((volatile uint16_t*) (MMC_BASE + 0x54))

#define MMC_RSP6    ((volatile uint16_t*) (MMC_BASE + 0x58))

#define MMC_RSP7    ((volatile uint16_t*) (MMC_BASE + 0x5C))


#define MMC_SYSC    ((volatile uint16_t*) (MMC_BASE + 0x64))
#define SYSC_SRST (1 << 1)

#define MMC_SYSS    ((volatile uint16_t*) (MMC_BASE + 0x68))
#define SYSS_RSTD (1 << 0)

