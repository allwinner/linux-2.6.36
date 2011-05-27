/**
*   $Id: host.c 9527 2008-07-16 14:45:01Z phth $
*
*/

#include "host.h"
#include "nanoutil.h"

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <asm/io.h>
#include <asm/arch/registers-s3c6400.h>

#include "s3c-hsmmc.h"

#define BUSY_WAIT_TIMEOUT_VALUE	10000

struct s3c_sdio_t {
	uint32_t dma_tx_channel;
	uint32_t dma_rx_channel;
} *s3c_sdio;

static void s3c_clk_enable(uint clocks, uint enable, ulong gate_reg);
static void s3c_sdio_write_to_fifo(unsigned long phys, unsigned int *buf, unsigned int total);
static void s3c_sdio_read_from_fifo(unsigned long phys, unsigned int *buf, unsigned int total);
static void dump_regs(const char *label, int line);

static void __iomem *SDIO_S3C_BASE;
static uint32_t cmd_reg;

#define s3c_hsmmc_readl(x)	ioread32(SDIO_S3C_BASE + (x))
#define s3c_hsmmc_readw(x)	ioread16(SDIO_S3C_BASE + (x))
#define s3c_hsmmc_readb(x)	ioread8(SDIO_S3C_BASE + (x))
#define s3c_hsmmc_writel(v,x)	iowrite32((v), SDIO_S3C_BASE + (x))
#define s3c_hsmmc_writew(v,x)	iowrite16((v), SDIO_S3C_BASE + (x))
#define s3c_hsmmc_writeb(v,x)	iowrite8((v), SDIO_S3C_BASE + (x))


int host_init(transfer_mode_t transfer_mode)
{
	uint32_t reg;
	unsigned long timeout;

	if (transfer_mode != TRANSFER_MODE_1BIT_SDIO) {
		TRSP_MSG("Only 1-bit SDIO is supported (4-bit SDIO generates CRC errors)\n");
		return -EINVAL;
	}

	/* Map SD host controller registers. */
	SDIO_S3C_BASE = ioremap(S3C_PA_HSMMC, SZ_4K);
	if (!SDIO_S3C_BASE) {
		TRSP_MSG("ioremap for SD host controller failed.\n");
		return -ENXIO;
	}

	/* Assign pins GPH0-GPH5 to MMC1 module (see manual pg. 208) */
	reg = readl(S3C_GPH0CON) & 0xff000000;
	writel(reg | 0x00222222, S3C_GPH0CON);
	reg = readl(S3C_GPHPU) & 0xfffff000;
	writel(reg | 0x00000aaa, S3C_GPHPU);

	/* Enable clocks for MMC1 module and USB clock
	   (probably USB shares a clock with the host contoller) */
	s3c_clk_enable(S3C_CLKCON_HCLK_HSMMC1, 1, (unsigned long) S3C_HCLK_GATE);
	s3c_clk_enable(S3C_CLKCON_SCLK_MMC1_48, 1, (unsigned long) S3C_SCLK_GATE);
	s3c_clk_enable(S3C_OTHERS_USB_SIG_MASK, 1, (unsigned long) S3C_OTHERS);

	/* Reset MMC1 module */
	s3c_hsmmc_writeb(S3C_HSMMC_RESET_ALL, HM_SWRST);
	msleep(1);
	if (s3c_hsmmc_readb(HM_SWRST) != 0) {
		TRSP_MSG("Failed to reset the SD host controller.\n");
		return -EIO;
	}

	/* Disable all interrupts */
	s3c_hsmmc_writel(0, HM_NORINTSTSEN);
	s3c_hsmmc_writel(0, HM_NORINTSIGEN);

	/* Power on SD bus with 3.3V (see manual pg. 855) */
	s3c_hsmmc_writeb(S3C_HSMMC_POWER_ON_ALL, HM_PWRCON);
	msleep(1);

	/* See manual pg. 880 - 882 */
	reg = S3C_HSMMC_CTRL2_CMD_CNFL_EN | S3C_HSMMC_CTRL2_SDCLK_HOLD
	    | S3C_HSMMC_CTRL2_CLKSEL_EXT;
	s3c_hsmmc_writel(reg, HM_CONTROL2);
	s3c_hsmmc_writel(0x80808080, HM_CONTROL3);
	s3c_hsmmc_writeb(S3C_HSMMC_TIMEOUT_MAX, HM_TIMEOUTCON);

	/* Start MMC1 module clock and wait until it is stable. */
	s3c_hsmmc_writew(S3C_HSMMC_CLOCK_INT_EN, HM_CLKCON);
	timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_CLKCON) & S3C_HSMMC_CLOCK_INT_STABLE)) {
		if (!timeout--) {
			TRSP_MSG("Host controller clock not stable\n");
			return -EIO;	
		}
	}

	/* Start SD bus clock at 12MHz and wait until it is stable. */
	s3c_hsmmc_writew(S3C_HSMMC_CLOCK_DIV4 | S3C_HSMMC_CLOCK_INT_EN  |
					 S3C_HSMMC_CLOCK_CARD_EN, HM_CLKCON);
	timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_CLKCON) & S3C_HSMMC_CLOCK_CARD_EN)) {
		if (!timeout--) {
			TRSP_MSG("SD bus clock not stable\n");
			return -EIO;
		}
	}

	/* Enable all bits in status register (this does not enable interrupts) */
	s3c_hsmmc_writew(0xffff, HM_NORINTSTSEN);
	s3c_hsmmc_writew(0xffff, HM_ERRINTSTSEN);
	return 0;
}

int host_reset(void)
{
	return 0;
}

void host_clock(clk_mode_t clk_mode)
{
	uint32_t clk_ctrl_reg;
	unsigned long timeout;

	switch (clk_mode) {
		case CLK_MODE_OFF:
			clk_ctrl_reg = S3C_HSMMC_CLOCK_INT_EN;
			break;
		case CLK_MODE_FAST:
			clk_ctrl_reg = S3C_HSMMC_CLOCK_INT_EN | S3C_HSMMC_CLOCK_CARD_EN
			             | S3C_HSMMC_CLOCK_DIV2; /* 24 MHz */
			break;
		case CLK_MODE_SLOW:
			clk_ctrl_reg = S3C_HSMMC_CLOCK_INT_EN | S3C_HSMMC_CLOCK_CARD_EN
						 | S3C_HSMMC_CLOCK_DIV256; /* 190 kHz */
			break;
		default:
			TRSP_ASSERT(0);
			return;
	}

	s3c_hsmmc_writew(clk_ctrl_reg, HM_CLKCON);
	if (clk_ctrl_reg & S3C_HSMMC_CLOCK_CARD_EN) {
		for (timeout = BUSY_WAIT_TIMEOUT_VALUE; timeout > 0; timeout--)
			if (s3c_hsmmc_readw(HM_CLKCON) & S3C_HSMMC_CLOCK_INT_STABLE)
				return;
		TRSP_ASSERT(0); /* SD bus clock still not stable */
	}
}

int host_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{
	unsigned int reg;
	unsigned int reason;

	switch (irq_id) {
	case IRQ_ID_RESPONSE_COMPLETION:
		reason = S3C_HSMMC_NIS_CMDCMP;
		break;
	case IRQ_ID_RW_ACCESS_COMPLETION:
		reason = S3C_HSMMC_NIS_TRSCMP;
		break;
	case IRQ_ID_FIFO_READ_READY:
		reason = S3C_HSMMC_NIS_RDRDY;
		break;
	case IRQ_ID_FIFO_WRITE_READY:
		reason = S3C_HSMMC_NIS_WRRDY;
		break;
	case IRQ_ID_TARGET:
		reason = S3C_HSMMC_NIS_CRDINT;
		break;
	default:
		TRSP_ASSERT(0);
		return IRQ_STATUS_FAIL;
	}

	switch (irq_op) {
	case IRQ_OP_ENABLE:
		reg = s3c_hsmmc_readl(HM_NORINTSIGEN);
		reg |= reason;
		s3c_hsmmc_writew(reg, HM_NORINTSIGEN);
		TRSP_ASSERT((s3c_hsmmc_readw(HM_NORINTSIGEN) & reason));
		break;
	case IRQ_OP_DISABLE:
		reg = s3c_hsmmc_readl(HM_NORINTSIGEN);
		reg &= ~reason;
		s3c_hsmmc_writew(reg, HM_NORINTSIGEN);
		TRSP_ASSERT(!(s3c_hsmmc_readw(HM_NORINTSIGEN) & reason));
		break;
	case IRQ_OP_STATUS:
		/* phth: Bus error status is currently ignored */
		reg = s3c_hsmmc_readl(HM_NORINTSTS) & s3c_hsmmc_readl(HM_NORINTSIGEN);
		return (reg & reason) ? IRQ_STATUS_ACTIVE : IRQ_STATUS_INACTIVE;
	case IRQ_OP_ACKNOWLEDGE:
		s3c_hsmmc_writew(reason, HM_NORINTSTS);
		while (s3c_hsmmc_readl(HM_NORINTSTS) & reason) /* phth: Could be safer */
			;
		break;
	default:
		TRSP_ASSERT(0);
		return IRQ_STATUS_FAIL;
	}

	return IRQ_STATUS_SUCCESS;
}

int host_fifo(void *data, unsigned long data_phys, uint16_t len, uint32_t flags)
{
	unsigned int *ptr = (unsigned int *) data;

	if (flags & FIFO_FLAG_TO_HOST)
		s3c_sdio_read_from_fifo(data_phys, ptr, len);
	else
		s3c_sdio_write_to_fifo(data_phys, ptr, len);

	sdio_fifo_callback();
	return 0;
}

irq_status_t host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
					  uint32_t flags, uint32_t arg, void *data)
{
	unsigned long timeout __attribute__ ((unused));
	uint32_t trns_reg;
	uint32_t len, data_phys;

	TRSP_ASSERT(cmd_resp == CMD_RESP_R1 || cmd_resp == CMD_RESP_R5
				|| cmd_resp == CMD_RESP_R6);

	/* Program argument register */
	s3c_hsmmc_writel(arg, HM_ARGUMENT);

	cmd_reg = (cmd << 8) | S3C_HSMMC_CMD_RESP_SHORT
			| S3C_HSMMC_CMD_CRC_CHECK | S3C_HSMMC_CMD_INDEX_CHECK;

	if (flags & CMD_FLAG_DATA_EXISTS) {
		cmd_reg |= S3C_HSMMC_CMD_DATA_EXISTS;

		/* Program transfer mode register */
		trns_reg = 0;
#ifdef USE_DMA
		trns_reg |= S3C_HSMMC_TRNS_DMA;
#endif
		if (flags & CMD_FLAG_DIR_DATA_TO_HOST)
			trns_reg |= S3C_HSMMC_TRNS_READ;
		s3c_hsmmc_writew(trns_reg, HM_TRNMOD);

		len = arg & 0x1ff;
		if (len == 0)
			len = 512;
		data_phys = (unsigned long) dma_map_single(NULL, data, len,
					(flags & CMD_FLAG_DIR_DATA_TO_HOST) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
		TRSP_ASSERT(data_phys);
		s3c_hsmmc_writel(data_phys, HM_SYSAD);
	}

	s3c_hsmmc_writew(cmd_reg, HM_CMDREG);

#ifndef USE_IRQ_ID_RESPONSE_COMPLETION
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_CMDCMP)) {
		if (!timeout--) {
			dump_regs(__FUNCTION__, __LINE__);
			return IRQ_STATUS_FAIL;
		}
	}

	s3c_hsmmc_writew(S3C_HSMMC_NIS_CMDCMP, HM_NORINTSTS);
	/* Don't attempt to verify that S3C_HSMMC_NIS_CMDCMP has been cleared,
	   as this may take some time (depending of SD bus clock)
	*/
#endif

	return IRQ_STATUS_SUCCESS;
}

uint32_t host_control(ctl_id_t ctl_id, uint32_t param)
{
	unsigned long timeout;
	
	switch (ctl_id) {

	case CTL_ID_LENGTH:
		s3c_hsmmc_writew(param, HM_BLKSIZE);
		break;

	case CTL_ID_WAIT_CMD_READY:
		for (timeout = BUSY_WAIT_TIMEOUT_VALUE; timeout > 0; timeout--)
			if (!(s3c_hsmmc_readw(HM_PRNSTS) & S3C_HSMMC_CMD_INHIBIT))
				return 0;
		TRSP_ASSERT(0); /* S3C_HSMMC_CMD_INHIBIT bit still set */
		break;

	case CTL_ID_RESPONSE:
		return (uint32_t) s3c_hsmmc_readl(HM_RSPREG0);

	default:
		TRSP_ASSERT(0);
	}

	return 0;
}

void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id)
{
}

void host_exit(void)
{
	volatile unsigned long reg __attribute__ ((unused));

	/* Disable all interrupts */
	s3c_hsmmc_writel(0, HM_NORINTSTSEN);
	s3c_hsmmc_writel(0, HM_NORINTSIGEN);

	/* Power off SD bus, reset MMC1 and stop its clock. */
	s3c_hsmmc_writeb(S3C_HSMMC_POWER_OFF, HM_PWRCON);
	s3c_hsmmc_writeb(S3C_HSMMC_RESET_ALL, HM_SWRST);
	msleep(1);
	s3c_hsmmc_writew(0, HM_CLKCON);

	/* Disable clocks for MMC1 module and USB clock */
	s3c_clk_enable(S3C_CLKCON_HCLK_HSMMC1, 0, (unsigned long) S3C_HCLK_GATE);
	s3c_clk_enable(S3C_CLKCON_SCLK_MMC1_48, 0, (unsigned long) S3C_SCLK_GATE);
	s3c_clk_enable(S3C_OTHERS_USB_SIG_MASK, 0, (unsigned long) S3C_OTHERS); /* phth: restore instead of disabling */

	/* Program pins GPH0-GPH5 as inputs (see manual pg. 208) */
	reg = readl(S3C_GPH0CON) & 0xff000000;
	writel(reg, S3C_GPH0CON);

	/* Unmap SD host controller registers. */
	iounmap(SDIO_S3C_BASE);
}

void s3c_dma_rx_callback(int error)
{
	sdio_fifo_callback();
}

void s3c_dma_tx_callback(int error)
{
	sdio_fifo_callback();
}

void s3c_clk_enable(uint clocks, uint enable, ulong gate_reg)
{
	unsigned long clkcon;
	unsigned long flags;

	local_irq_save(flags);

	clkcon = __raw_readl(gate_reg);
	clkcon &= ~clocks;

	if (enable)
		clkcon |= clocks;

	__raw_writel(clkcon, gate_reg);

	local_irq_restore(flags);
}

/* phth: s3c_sdio_read_from_fifo and s3c_sdio_write_to_fifo could be unified */
static void s3c_sdio_read_from_fifo(unsigned long phys, unsigned int *buf, unsigned int total)
{    
    unsigned long timeout __attribute__ ((unused));
    uint16_t err = 0;

    total = (total + sizeof(unsigned int) - 1) / sizeof(unsigned int);

    err = s3c_hsmmc_readw(HM_ERRINTSTS);
    if (err) {
	   dump_regs(__FUNCTION__, __LINE__);
       return;
    }

#ifndef USE_DMA
	timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_RDRDY)) {
		if (!timeout--) {
			dump_regs(__FUNCTION__, __LINE__);
			return;
		}
	}

    s3c_hsmmc_writew(S3C_HSMMC_NIS_RDRDY, HM_NORINTSTS);
	TRSP_ASSERT(!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_RDRDY));

    while (total--)
		*buf++ = s3c_hsmmc_readl(HM_BDATA);
#endif /* USE_DMA */

#ifndef USE_IRQ_ID_RW_ACCESS_COMPLETION
	timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_TRSCMP)) {
		if (!timeout--) {
			dump_regs(__FUNCTION__, __LINE__);
			return;
		}
	}
	
	s3c_hsmmc_writew(S3C_HSMMC_NIS_TRSCMP, HM_NORINTSTS);
	ndelay(10);
	TRSP_ASSERT(!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_TRSCMP));
#endif /* USE_IRQ_ID_RW_ACCESS_COMPLETION */
}

static void s3c_sdio_write_to_fifo(unsigned long phys, unsigned int *buf, unsigned int total)
{   
    unsigned long timeout __attribute__ ((unused));
    uint16_t err = 0;

	total = (total + sizeof(unsigned int) - 1) / sizeof(unsigned int);

    err = s3c_hsmmc_readw(HM_ERRINTSTS);
    if (err) {
	   dump_regs(__FUNCTION__, __LINE__);
       return;
    }

#ifndef USE_DMA
	timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_WRRDY)) {
		if (!timeout--) {
			dump_regs(__FUNCTION__, __LINE__);
			return;
		}
	}

    s3c_hsmmc_writew(S3C_HSMMC_NIS_WRRDY, HM_NORINTSTS);
	TRSP_ASSERT(!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_WRRDY));

    while (total--)
		s3c_hsmmc_writel(*buf++, HM_BDATA);
#endif /* USE_DMA */

#ifndef USE_IRQ_ID_RW_ACCESS_COMPLETION
    timeout = BUSY_WAIT_TIMEOUT_VALUE;
	while (!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_TRSCMP)) {
		if (!timeout--) {
			dump_regs(__FUNCTION__, __LINE__);
			return;
		}
	}

	s3c_hsmmc_writew(S3C_HSMMC_NIS_TRSCMP, HM_NORINTSTS);
	ndelay(10);
	TRSP_ASSERT(!(s3c_hsmmc_readw(HM_NORINTSTS) & S3C_HSMMC_NIS_TRSCMP));
#endif /* USE_IRQ_ID_RW_ACCESS_COMPLETION */
}

void dump_regs(const char *label, int line)
{
	printk
	    ("\n################## |         Dumping Regs at func [ %s ] and line : %d           | #####################\n",
	     label, line);
	printk("\nHM_SYSAD\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_SYSAD));
	printk("HM_BLKSIZE\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_BLKSIZE));
	printk("HM_BLKCNT\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_BLKCNT));
	printk("HM_ARGUMENT\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_ARGUMENT));
	printk("HM_TRNMOD\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_TRNMOD));
	printk("HM_CMDREG\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_CMDREG));
	printk("HM_RSPREG0\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_RSPREG0));
	printk("HM_RSPREG1\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_RSPREG1));
	printk("HM_RSPREG2\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_RSPREG2));
	printk("HM_RSPREG3\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_RSPREG3));
//  printk( "HM_BDATA\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_BDATA));
	printk("HM_PRNSTS\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_PRNSTS));
	printk("HM_HOSTCTL\t\t\t\t: 0x%x\n", s3c_hsmmc_readb(HM_HOSTCTL));
	printk("HM_PWRCON\t\t\t\t: 0x%x\n", s3c_hsmmc_readb(HM_PWRCON));
	printk("HM_BLKGAP\t\t\t\t: 0x%x\n", s3c_hsmmc_readb(HM_BLKGAP));
	printk("HM_WAKCON\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_WAKCON));
	printk("HM_CLKCON\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_CLKCON));
	printk("HM_TIMEOUTCON\t\t\t\t: 0x%x\n", s3c_hsmmc_readb(HM_TIMEOUTCON));
	printk("HM_SWRST\t\t\t\t: 0x%x\n", s3c_hsmmc_readb(HM_SWRST));
	printk("HM_NORINTSTS\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_NORINTSTS));
	printk("HM_ERRINTSTS\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_ERRINTSTS));
	printk("HM_NORINTSTSEN\t\t\t\t: 0x%x\n",
	       s3c_hsmmc_readw(HM_NORINTSTSEN));
	printk("HM_ERRINTSTSEN\t\t\t\t: 0x%x\n",
	       s3c_hsmmc_readw(HM_ERRINTSTSEN));
	printk("HM_NORINTSIGEN\t\t\t\t: 0x%x\n",
	       s3c_hsmmc_readw(HM_NORINTSIGEN));
	printk("HM_ERRINTSIGEN\t\t\t\t: 0x%x\n",
	       s3c_hsmmc_readw(HM_ERRINTSIGEN));
	printk("HM_ACMD12ERRSTS\t\t\t\t: 0x%x\n",
	       s3c_hsmmc_readw(HM_ACMD12ERRSTS));
	printk("HM_CAPAREG\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_CAPAREG));
	printk("HM_MAXCURR\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_MAXCURR));
	printk("HM_CONTROL2\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_CONTROL2));
	printk("HM_CONTROL3\t\t\t\t: 0x%x\n", s3c_hsmmc_readl(HM_CONTROL3));
	printk("HM_HCVER\t\t\t\t: 0x%x\n", s3c_hsmmc_readw(HM_HCVER));
	printk
	    ("\n########################################################################################################\n");

}
