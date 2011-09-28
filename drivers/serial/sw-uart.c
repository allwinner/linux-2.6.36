/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Note 1: This driver is made separate from the already too overloaded
 * 8250.c because it needs some kirks of its own and that'll make it
 * easier to add DMA support.
 *
 * Note 2: I'm too sick of device allocation policies for serial ports.
 * If someone else wants to request an "official" allocation of major/minor
 * for this driver please be my guest.  And don't forget that new hardware
 * to come from Intel might have more than 3 or 4 of those UARTs.  Let's
 * hope for a better port registration and dynamic device allocation scheme
 * with the serial core maintainer satisfaction to appear soon.
 */

#if defined(CONFIG_SERIAL_SW_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif


#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_reg.h>
#include <linux/circ_buf.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <asm/io.h>
#include <mach/platform.h>
#include <mach/hardware.h>
#include <mach/uart_regs.h>
#include <mach/pio_regs.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

//#define DEBUG_UART

#ifdef DEBUG_UART
#include "sw-uart-dbg.h"
#endif

/* Register base define */
#define UART_BASE       (0x01C28000)
#define UART_BASE_OS    (0x400)
#define UARTx_BASE(x)   (UART_BASE + (x) * UART_BASE_OS)

#define UART_USR        31

/* debug control */
#define DEBUG_NO        2
#ifdef DEBUG_UART
#define UART_DBG(up,...)   do { \
                                    if ((up)->port_no == DEBUG_NO) \
                                    { \
                                        my_printk("[uart(%d)]: ", (up)->port_no); \
                                        my_printk(__VA_ARGS__); \
                                    } \
                               } while(0)
#define UART_MSG(up,...)   do { \
                                    if ((up)->port_no == DEBUG_NO) \
                                    { \
                                        my_printk(__VA_ARGS__); \
                                    } \
                               } while(0)
#define UART_INF(...)       do {my_printk(__VA_ARGS__);} while(0)
#else
#define UART_DBG(up,...)
#define UART_MSG(up,...)
#define UART_INF(...)
#endif

#define RESSIZE(res)        (((res)->end - (res)->start)+1)

struct sw_uart_port {
    struct uart_port    port;       //must be first
    
    char                name[16];
    int                 port_no;
    int                 pin_num;
    u32                 pio_hdle;
    struct clk          *clk;
    u32                 sclk;
    struct resource     *mmres;
    u32                 irq;
    
    unsigned char       ier;
    unsigned char       lcr;
    unsigned char       mcr;
    unsigned int        lsr_break_flag;
    
    struct platform_device* pdev;
#ifdef CONFIG_CPU_FREQ
    struct notifier_block    freq_transition;
#endif
};

static struct sw_uart_port *sw_serial_ports[8];

static inline unsigned int serial_in(struct sw_uart_port *up, int offset)
{
    offset <<= 2;
    return readl(up->port.membase + offset);
}

static inline void serial_out(struct sw_uart_port *up, int offset, int value)
{
    offset <<= 2;
    writel(value, up->port.membase + offset);
}

static void sw_serial_enable_ms(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    
    up->ier |= UART_IER_MSI;
    serial_out(up, UART_IER, up->ier);
    UART_DBG(up, "L%d, %s, ier %02x\n", __LINE__, __FUNCTION__, up->ier);
}

static void sw_serial_stop_tx(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;

    if (up->ier & UART_IER_THRI) {
        up->ier &= ~UART_IER_THRI;
        serial_out(up, UART_IER, up->ier);
    }
    UART_DBG(up, "L%d, %s, ier %02x\n", __LINE__, __FUNCTION__, up->ier);
}

static void sw_serial_stop_rx(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;

    up->ier &= ~UART_IER_RLSI;
    up->port.read_status_mask &= ~UART_LSR_DR;
    serial_out(up, UART_IER, up->ier);
    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
}

static inline void receive_chars(struct sw_uart_port *up, int *status)
{
    struct tty_struct *tty = up->port.state->port.tty;
    unsigned int ch, flag, lsr = *status;
    int max_count = 256;

   UART_DBG(up, "L%d, %s, rx[] = \n", __LINE__, __FUNCTION__);
    do {
        if (likely(lsr & UART_LSR_DR))
            ch = serial_in(up, UART_RX);
        else
            ch = 0;
            
        flag = TTY_NORMAL;
        up->port.icount.rx++;
        UART_MSG(up, "%02x-", ch);
        if (unlikely(*status & (UART_LSR_BI | UART_LSR_PE |
                       UART_LSR_FE | UART_LSR_OE))) {
            /*
             * For statistics only
             */
            if (*status & UART_LSR_BI) {
                *status &= ~(UART_LSR_FE | UART_LSR_PE);
                up->port.icount.brk++;
                /*
                 * We do the SysRQ and SAK checking
                 * here because otherwise the break
                 * may get masked by ignore_status_mask
                 * or read_status_mask.
                 */
                if (uart_handle_break(&up->port))
                    goto ignore_char;
            } else if (*status & UART_LSR_PE)
                up->port.icount.parity++;
            else if (*status & UART_LSR_FE)
                up->port.icount.frame++;
            if (*status & UART_LSR_OE)
                up->port.icount.overrun++;

            /*
             * Mask off conditions which should be ignored.
             */
            *status &= up->port.read_status_mask;

#ifdef CONFIG_SERIAL_SW_CONSOLE
            if (up->port.line == up->port.cons->index) {
                /* Recover the break flag from console xmit */
                *status |= up->lsr_break_flag;
                up->lsr_break_flag = 0;
            }
#endif
            if (*status & UART_LSR_BI) {
                flag = TTY_BREAK;
            } else if (*status & UART_LSR_PE)
                flag = TTY_PARITY;
            else if (*status & UART_LSR_FE)
                flag = TTY_FRAME;
        }

        if (uart_handle_sysrq_char(&up->port, ch))
            goto ignore_char;

        uart_insert_char(&up->port, *status, UART_LSR_OE, ch, flag);

ignore_char:
        *status = serial_in(up, UART_LSR);
    } while ((*status & (UART_LSR_DR | UART_LSR_BI)) && (max_count-- > 0));
    UART_MSG(up, "\n");
    spin_unlock(&up->port.lock);
    tty_flip_buffer_push(tty);
    spin_lock(&up->port.lock);
}

static void transmit_chars(struct sw_uart_port *up)
{
    struct circ_buf *xmit = &up->port.state->xmit;
    int count;

    UART_DBG(up, "L%d, %s, tx[] = \n", __LINE__, __FUNCTION__);
    if (up->port.x_char) {
        UART_MSG(up, "%02x\n", up->port.x_char);
        serial_out(up, UART_TX, up->port.x_char);
        up->port.icount.tx++;
        up->port.x_char = 0;
        return;
    }
    if (uart_circ_empty(xmit) || uart_tx_stopped(&up->port)) {
        sw_serial_stop_tx(&up->port);
        return;
    }

    count = up->port.fifosize / 2;
    do {
        UART_MSG(up, "%02x-", xmit->buf[xmit->tail]);
        serial_out(up, UART_TX, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        up->port.icount.tx++;
        if (uart_circ_empty(xmit))
            break;
    } while (--count > 0);
    UART_MSG(up, "\n");

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(&up->port);


    if (uart_circ_empty(xmit))
        sw_serial_stop_tx(&up->port);
}

static void sw_serial_start_tx(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;

    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    if (!(up->ier & UART_IER_THRI)) {
        up->ier |= UART_IER_THRI;
        serial_out(up, UART_IER, up->ier);
    }
}

static inline void check_modem_status(struct sw_uart_port *up)
{
    int msr, mcr;

    msr = serial_in(up, UART_MSR);
    mcr = serial_in(up, UART_MCR);
    UART_DBG(up, "L%d, %s, msr %02x\n", __LINE__, __FUNCTION__, msr);
    if ((msr & UART_MSR_ANY_DELTA) == 0)
        return;

    if (msr & UART_MSR_TERI) {
        UART_DBG(up, "UART_MSR_TERI\n");
        up->port.icount.rng++;
    }
    if (msr & UART_MSR_DDSR) {
        UART_DBG(up, "UART_MSR_DDSR\n");
        up->port.icount.dsr++;
    }
    if (msr & UART_MSR_DDCD) {
        UART_DBG(up, "dcd change to %d\n", msr & UART_MSR_DCD);
        uart_handle_dcd_change(&up->port, msr & UART_MSR_DCD);
    }
    if (!(mcr & UART_MCR_AFE) && (msr & UART_MSR_DCTS)) {
        UART_DBG(up, "cts change to %d\n", msr & UART_MSR_CTS);
        uart_handle_cts_change(&up->port, msr & UART_MSR_CTS);
    }

    wake_up_interruptible(&up->port.state->port.delta_msr_wait);
}

/*
 * This handles the interrupt from one port.
 */
static inline irqreturn_t sw_serial_irq(int irq, void *dev_id)
{
    struct sw_uart_port *up = dev_id;
    unsigned int iir, lsr, usr, irq_id;
    unsigned long flags;

    spin_lock_irqsave(&up->port.lock, flags);
    
    iir = serial_in(up, UART_IIR);
    irq_id = iir & 0xf;
    lsr = serial_in(up, UART_LSR);
    usr = serial_in(up, UART_USR);
    
    UART_DBG(up, "L%d, %s, iir 0x%02x, lsr 0x%02x, usr 0x%02x\n", __LINE__, __FUNCTION__, iir, lsr, usr);
    if (irq_id == UART_IIR_NO_INT) {
        UART_DBG(up, "no irq pending\n");
        return IRQ_HANDLED;
    }
    if (irq_id == UART_IIR_BUSY) {
        UART_DBG(up, "busy detected\n");
    }
    
    if (lsr & (UART_LSR_DR | UART_LSR_BI)) {
        //UART_DBG(up, "rx\n");
        receive_chars(up, &lsr);
    }
    
    check_modem_status(up);
    if (lsr & UART_LSR_THRE) {
        //UART_DBG(up, "tx\n");
        transmit_chars(up);
    }
    spin_unlock_irqrestore(&up->port.lock, flags);
    return IRQ_HANDLED;
}

static unsigned int sw_serial_tx_empty(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned long flags;
    unsigned int ret;

    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    spin_lock_irqsave(&up->port.lock, flags);
    ret = serial_in(up, UART_LSR) & UART_LSR_TEMT ? TIOCSER_TEMT : 0;
    spin_unlock_irqrestore(&up->port.lock, flags);

    return ret;
}

static unsigned int sw_serial_get_mctrl(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned char status;
    unsigned int ret;

    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    status = serial_in(up, UART_MSR);

    ret = 0;
    if (status & UART_MSR_DCD)
        ret |= TIOCM_CAR;
    if (status & UART_MSR_RI)
        ret |= TIOCM_RNG;
    if (status & UART_MSR_DSR)
        ret |= TIOCM_DSR;
    if (status & UART_MSR_CTS)
        ret |= TIOCM_CTS;
    return ret;
}

static void sw_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned char mcr = 0;

    UART_DBG(up, "L%d, %s, mctrl %x\n", __LINE__, __FUNCTION__, mctrl);
    if (mctrl & TIOCM_RTS) {
        UART_DBG(up, "UART_MCR_RTS\n");
        mcr |= UART_MCR_RTS;
    }
    if (mctrl & TIOCM_DTR) {
        UART_DBG(up, "UART_MCR_DTR\n");
        mcr |= UART_MCR_DTR;
    }
    if (mctrl & TIOCM_OUT1) {
        UART_DBG(up, "UART_MCR_OUT1\n");
        mcr |= UART_MCR_OUT1;
    }
    if (mctrl & TIOCM_OUT2) {
        UART_DBG(up, "UART_MCR_OUT2\n");
        mcr |= UART_MCR_OUT2;
    }
    if (mctrl & TIOCM_LOOP) {
        UART_DBG(up, "UART_MCR_LOOP\n");
        mcr |= UART_MCR_LOOP;
    }

    mcr |= up->mcr;

    serial_out(up, UART_MCR, mcr);
}

static void sw_serial_break_ctl(struct uart_port *port, int break_state)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned long flags;

    UART_DBG(up, "L%d, %s, break state %d\n", __LINE__, __FUNCTION__, break_state);
    spin_lock_irqsave(&up->port.lock, flags);
    if (break_state == -1) {
        UART_DBG(up, "Set Break Control\n");
        up->lcr |= UART_LCR_SBC;
    }
    else {
        UART_DBG(up, "Clear Break Control\n");
        up->lcr &= ~UART_LCR_SBC;
    }
    serial_out(up, UART_LCR, up->lcr);
    spin_unlock_irqrestore(&up->port.lock, flags);
}

#if 0
static void sw_serial_dma_init(struct sw_uart *up)
{
    up->rxdma =
        sw_request_dma(up->name, DMA_PRIO_LOW, sw_receive_dma, up);
    if (up->rxdma < 0)
        goto out;
    up->txdma =
        sw_request_dma(up->name, DMA_PRIO_LOW, sw_transmit_dma, up);
    if (up->txdma < 0)
        goto err_txdma;
    up->dmadesc = kmalloc(4 * sizeof(sw_dma_desc), GFP_KERNEL);
    if (!up->dmadesc)
        goto err_alloc;

    /* ... */
err_alloc:
    sw_free_dma(up->txdma);
err_rxdma:
    sw_free_dma(up->rxdma);
out:
    return;
}
#endif

static int sw_serial_startup(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned long flags;
    int retval;

    clk_enable(up->clk);

    up->port.uartclk = clk_get_rate(up->clk);
    UART_DBG(up, "L%d, %s, uartclk %d\n", __LINE__, __FUNCTION__, up->port.uartclk);

    /*
     * Allocate the IRQ
     */
    retval = request_irq(up->port.irq, sw_serial_irq, 0, up->name, up);
    if (retval)
        return retval;

    /*
     * Clear the FIFO buffers and disable them.
     * (they will be reenabled in set_termios())
     */
    serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
    serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
            UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
    serial_out(up, UART_FCR, 0);

    /*
     * Clear the interrupt registers.
     */
    (void) serial_in(up, UART_LSR);
    (void) serial_in(up, UART_RX);
    (void) serial_in(up, UART_IIR);
    (void) serial_in(up, UART_MSR);

    /*
     * Now, initialize the UART
     */
    serial_out(up, UART_LCR, UART_LCR_WLEN8);

    spin_lock_irqsave(&up->port.lock, flags);
    up->port.mctrl |= TIOCM_OUT2;
    sw_serial_set_mctrl(&up->port, up->port.mctrl);
    spin_unlock_irqrestore(&up->port.lock, flags);

    /*
     * Finally, enable interrupts.  Note: Modem status interrupts
     * are set via set_termios(), which will be occurring imminently
     * anyway, so we don't enable them here.
     */
    up->ier = UART_IER_RLSI | UART_IER_RDI | UART_IER_RTOIE | UART_IER_UUE;
    serial_out(up, UART_IER, up->ier);

    /*
     * And clear the interrupt registers again for luck.
     */
    (void) serial_in(up, UART_LSR);
    (void) serial_in(up, UART_RX);
    (void) serial_in(up, UART_IIR);
    (void) serial_in(up, UART_MSR);

    return 0;
}

static void sw_serial_shutdown(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned long flags;

    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    free_irq(up->port.irq, up);

    /*
     * Disable interrupts from this port
     */
    up->ier = 0;
    serial_out(up, UART_IER, 0);

    spin_lock_irqsave(&up->port.lock, flags);
    up->port.mctrl &= ~TIOCM_OUT2;
    sw_serial_set_mctrl(&up->port, up->port.mctrl);
    spin_unlock_irqrestore(&up->port.lock, flags);

    /*
     * Disable break condition and FIFOs
     */
    serial_out(up, UART_LCR, serial_in(up, UART_LCR) & ~UART_LCR_SBC);
    serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
                  UART_FCR_CLEAR_RCVR |
                  UART_FCR_CLEAR_XMIT);
    serial_out(up, UART_FCR, 0);

    clk_disable(up->clk);
}

static void
sw_serial_set_termios(struct uart_port *port, struct ktermios *termios,
               struct ktermios *old)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned char cval, fcr = 0;
    unsigned long flags;
    unsigned int baud, quot;

    UART_DBG(up, "L%d, %s, termios %x\n", __LINE__, __FUNCTION__, termios->c_cflag);
    clk_enable(up->clk);

    switch (termios->c_cflag & CSIZE) {
    case CS5:
        cval = UART_LCR_WLEN5;
        break;
        UART_DBG(up, "data len %d\n", 5);
    case CS6:
        cval = UART_LCR_WLEN6;
        UART_DBG(up, "data len %d\n", 6);
        break;
    case CS7:
        cval = UART_LCR_WLEN7;
        UART_DBG(up, "data len %d\n", 7);
        break;
    default:
    case CS8:
        UART_DBG(up, "data len %d\n", 8);
        cval = UART_LCR_WLEN8;
        break;
    }

    if (termios->c_cflag & CSTOPB) {
        UART_DBG(up, "stop bit 1.5~2\n");
        cval |= UART_LCR_STOP;
    }
    if (termios->c_cflag & PARENB) {
        UART_DBG(up, "parity check enable\n");
        cval |= UART_LCR_PARITY;
    }
    if (!(termios->c_cflag & PARODD)) {
        UART_DBG(up, "even parity(%s)\n", termios->c_cflag & PARENB ? "enb" : "not enb");
        cval |= UART_LCR_EPAR;
    } else {
        UART_DBG(up, "odd parity(%s)\n", termios->c_cflag & PARENB ? "enb" : "not enb");
    }
    /*
     * Ask the core to calculate the divisor for us.
     */

#if CONFIG_CHIP_ID == 1123
    port->uartclk = 24000000; /* Only work on FPGA board */
#elif CONFIG_CHIP_ID == 1120
    port->uartclk = clk_get_rate(up->clk);
#else
    #error "no chip id defined"
#endif

    baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);
    quot = uart_get_divisor(port, baud);

    if ((up->port.uartclk / quot) < (2400 * 16))
        fcr = UART_FCR_ENABLE_FIFO | UART_FCR_PXAR1;
    else if ((up->port.uartclk / quot) < (230400 * 16))
        fcr = UART_FCR_ENABLE_FIFO | UART_FCR_PXAR8;
    else
        fcr = UART_FCR_ENABLE_FIFO | UART_FCR_PXAR32;
    UART_DBG(up, "uartclk %d, baud %d, quot %d, fcr 0x%x\n", port->uartclk, baud, quot, fcr);

    /*
     * Ok, we're now changing the port state.  Do it with
     * interrupts disabled.
     */
    spin_lock_irqsave(&up->port.lock, flags);

    /*
     * Ensure the port will be enabled.
     * This is required especially for serial console.
     */
    //up->ier |= IER_UUE;

    /*
     * Update the per-port timeout.
     */
    uart_update_timeout(port, termios->c_cflag, baud);

    up->port.read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
    if (termios->c_iflag & INPCK) 
        up->port.read_status_mask |= UART_LSR_FE | UART_LSR_PE;
    if (termios->c_iflag & (BRKINT | PARMRK))
        up->port.read_status_mask |= UART_LSR_BI;
    /*
     * Characters to ignore
     */
    up->port.ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR)
        up->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
    if (termios->c_iflag & IGNBRK) {
        up->port.ignore_status_mask |= UART_LSR_BI;
        /*
         * If we're ignoring parity and break indicators,
         * ignore overruns too (for real raw support).
         */
        if (termios->c_iflag & IGNPAR)
            up->port.ignore_status_mask |= UART_LSR_OE;
    }

    /*
     * ignore all characters if CREAD is not set
     */
    if ((termios->c_cflag & CREAD) == 0)
        up->port.ignore_status_mask |= UART_LSR_DR;

    /*
     * CTS flow control flag and modem status interrupts
     */
    up->ier &= ~UART_IER_MSI;
    if (UART_ENABLE_MS(&up->port, termios->c_cflag)) {
        UART_DBG(up, "UART_IER_MSI\n");
        up->ier |= UART_IER_MSI;
    }

    serial_out(up, UART_IER, up->ier);

    if (termios->c_cflag & CRTSCTS) {
        UART_DBG(up, "UART_MCR_AFE enb\n");
        up->mcr |= UART_MCR_AFE;
    }
    else {
        UART_DBG(up, "UART_MCR_AFE disb\n");
        up->mcr &= ~UART_MCR_AFE;
    }

    serial_out(up, UART_LCR, cval | UART_LCR_DLAB);/* set DLAB */
    serial_out(up, UART_DLL, quot & 0xff);        /* LS of divisor */
    serial_out(up, UART_DLM, quot >> 8);        /* MS of divisor */
    serial_out(up, UART_LCR, cval);        /* reset DLAB */
    up->lcr = cval;                    /* Save LCR */
    sw_serial_set_mctrl(&up->port, up->port.mctrl);
    serial_out(up, UART_FCR, fcr);
    spin_unlock_irqrestore(&up->port.lock, flags);

    //clk_disable(up->clk);
}

static void
sw_serial_pm(struct uart_port *port, unsigned int state,
          unsigned int oldstate)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;

    UART_DBG(up, "L%d, %s %d\n", __LINE__, __FUNCTION__, state);
    if (!state)
        clk_enable(up->clk);
    else
        clk_disable(up->clk);
}

static void sw_serial_release_port(struct uart_port *port)
{
    UART_DBG((struct sw_uart_port*)port, "L%d, %s\n", __LINE__, __FUNCTION__);
}

static int sw_serial_request_port(struct uart_port *port)
{
    UART_DBG((struct sw_uart_port*)port, "L%d, %s\n", __LINE__, __FUNCTION__);
    return 0;
}

static void sw_serial_config_port(struct uart_port *port, int flags)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    up->port.type = PORT_SW;
}

static int
sw_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    UART_DBG((struct sw_uart_port*)port, "L%d, %s\n", __LINE__, __FUNCTION__);
    /* we don't want the core code to modify any port params */
    return -EINVAL;
}

static const char *
sw_serial_type(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    return up->name;
}

static struct uart_driver sw_uart_driver;

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

/*
 *    Wait for transmitter & holding register to empty
 */
static inline void wait_for_xmitr(struct sw_uart_port *up)
{
    unsigned int status, tmout = 10000;

//    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    /* Wait up to 10ms for the character(s) to be sent. */
    do {
        status = serial_in(up, UART_LSR);

        if (status & UART_LSR_BI)
            up->lsr_break_flag = UART_LSR_BI;

        if (--tmout == 0)
            break;
        udelay(1);
    } while ((status & BOTH_EMPTY) != BOTH_EMPTY);

    /* Wait up to 1s for flow control if necessary */
    if (up->port.flags & UPF_CONS_FLOW) {
        tmout = 1000000;
        while (--tmout &&
               ((serial_in(up, UART_MSR) & UART_MSR_CTS) == 0))
            udelay(1);
    }
}

#ifdef CONFIG_SERIAL_SW_CONSOLE

static void sw_serial_console_putchar(struct uart_port *port, int ch)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;

//    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    wait_for_xmitr(up);
    serial_out(up, UART_TX, ch);
}

/*
 * Print a string to the serial port trying not to disturb
 * any possible real use of the port...
 *
 *    The console_lock must be held when we get here.
 */
static void
sw_serial_console_write(struct console *co, const char *s, unsigned int count)
{
    struct sw_uart_port *up = sw_serial_ports[co->index];
    unsigned int ier;

//    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
//    clk_enable(up->clk);

    /*
     *    First save the IER then disable the interrupts
     */
    ier = serial_in(up, UART_IER);
    serial_out(up, UART_IER, UART_IER_UUE);

    uart_console_write(&up->port, s, count, sw_serial_console_putchar);

    /*
     *    Finally, wait for transmitter to become empty
     *    and restore the IER
     */
    wait_for_xmitr(up);
    serial_out(up, UART_IER, ier);

    //clk_disable(up->clk);
}

static int __init
sw_serial_console_setup(struct console *co, char *options)
{
    struct sw_uart_port *up;
    int baud = 9600;
    int bits = 8;
    int parity = 'n';
    int flow = 'n';

//    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
    if (co->index == -1 || co->index >= sw_uart_driver.nr)
        co->index = 0;
    up = sw_serial_ports[co->index];
    if (!up)
        return -ENODEV;

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    return uart_set_options(&up->port, co, baud, parity, bits, flow);
}

static struct console sw_serial_console = {
    .name   = "ttyS",
    .write  = sw_serial_console_write,
    .device = uart_console_device,
    .setup  = sw_serial_console_setup,
    .flags  = CON_PRINTBUFFER,
    .index  = -1,
    .data   = &sw_uart_driver,
};

#define SW_CONSOLE    &sw_serial_console
#else
#define SW_CONSOLE    NULL
#endif


#ifdef CONFIG_CONSOLE_POLL
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int sw_serial_get_poll_char(struct uart_port *port)
{
    struct sw_uart_port *up = (struct sw_uart_port *)port;
    unsigned char lsr;
    int res;

//    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
//    clk_enable(up->clk);

    lsr = serial_in(up, UART_LSR);

    while (!(lsr & UART_LSR_DR))
        lsr = serial_in(up, UART_LSR);

    res = serial_in(up, UART_RX);

    //clk_disable(up->clk);

    return res;
}


static void sw_serial_put_poll_char(struct uart_port *port,
             unsigned char c)
{
    unsigned int ier;
    struct sw_uart_port *up = (struct sw_uart_port *)port;

    UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
//    clk_enable(up->clk);

    /*
     *    First save the IER then disable the interrupts
     */
    ier = serial_in(up, UART_IER);
    serial_out(up, UART_IER, 0);

    wait_for_xmitr(up);
    /*
     *    Send the character out.
     *    If a LF, also do CR...
     */
    serial_out(up, UART_TX, c);
    if (c == 10) {
        wait_for_xmitr(up);
        serial_out(up, UART_TX, 13);
    }

    /*
     *    Finally, wait for transmitter to become empty
     *    and restore the IER
     */
    wait_for_xmitr(up);
    serial_out(up, UART_IER, ier);

    //clk_disable(up->clk);
}

#endif /* CONFIG_CONSOLE_POLL */

struct uart_ops sw_serial_pops = {
    .tx_empty       = sw_serial_tx_empty,
    .set_mctrl      = sw_serial_set_mctrl,
    .get_mctrl      = sw_serial_get_mctrl,
    .stop_tx        = sw_serial_stop_tx,
    .start_tx       = sw_serial_start_tx,
    .stop_rx        = sw_serial_stop_rx,
    .enable_ms      = sw_serial_enable_ms,
    .break_ctl      = sw_serial_break_ctl,
    .startup        = sw_serial_startup,
    .shutdown       = sw_serial_shutdown,
    .set_termios    = sw_serial_set_termios,
    .pm             = sw_serial_pm,
    .type           = sw_serial_type,
    .release_port   = sw_serial_release_port,
    .request_port   = sw_serial_request_port,
    .config_port    = sw_serial_config_port,
    .verify_port    = sw_serial_verify_port,
#ifdef CONFIG_CONSOLE_POLL
    .poll_get_char  = sw_serial_get_poll_char,
    .poll_put_char  = sw_serial_put_poll_char,
#endif
};

static struct uart_driver sw_uart_driver = {
    .owner      = THIS_MODULE,
    .driver_name= "SW serial",
    .dev_name   = "ttyS",
    .major      = TTY_MAJOR,
    .minor      = 64,
    .nr         = 8,
    .cons       = SW_CONSOLE,
};

static int sw_serial_suspend(struct platform_device *dev, pm_message_t state)
{
#if 1
        struct sw_uart_port *up = platform_get_drvdata(dev);

        if (up)
                uart_suspend_port(&sw_uart_driver, &up->port);
                
        UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
#endif
        return 0;
}

static int sw_serial_resume(struct platform_device *dev)
{
#if 1
        struct sw_uart_port *up = platform_get_drvdata(dev);

        if (up)
                uart_resume_port(&sw_uart_driver, &up->port);
                
        UART_DBG(up, "L%d, %s\n", __LINE__, __FUNCTION__);
#endif
        return 0;
}

#if 0
//#ifdef CONFIG_CPU_FREQ

static int sw_serial_cpufreq_transition(struct notifier_block *nb,
                         unsigned long val, void *data)
{
    struct sw_uart_port *port;
    struct uart_port *uport;

    port = container_of(nb, struct sw_uart_port, freq_transition);
    uport = &port->port;

    /* try and work out if the baudrate is changing, we can detect
     * a change in rate, but we do not have support for detecting
     * a disturbance in the clock-rate over the change.
     */

    if (IS_ERR(port->clk))
        goto exit;

    if (uport->uartclk == clk_get_rate(port->clk))
        goto exit;

    if (val == CPUFREQ_PRECHANGE) {
        /* we should really shut the port down whilst the
         * frequency change is in progress. */

    } else if (val == CPUFREQ_POSTCHANGE) {
        struct ktermios *termios;
        struct tty_struct *tty;

        if (uport->info == NULL)
            goto exit;

        tty = uport->info->port.tty;

        if (tty == NULL)
            goto exit;

        termios = tty->termios;

        if (termios == NULL) {
            printk(KERN_WARNING "%s: no termios?\n", __func__);
            goto exit;
        }

        sw_serial_set_termios(uport, termios, NULL);
    }

 exit:
    return 0;
}

static inline int sw_serial_cpufreq_register(struct sw_uart_port *port)
{
    port->freq_transition.notifier_call = sw_serial_cpufreq_transition;

    return cpufreq_register_notifier(&port->freq_transition, CPUFREQ_TRANSITION_NOTIFIER);
}

static inline void sw_serial_cpufreq_deregister(struct sw_uart_port *port)
{
    cpufreq_unregister_notifier(&port->freq_transition, CPUFREQ_TRANSITION_NOTIFIER);
}

#else
static inline int sw_serial_cpufreq_register(struct sw_uart_port *port)
{
    return 0;
}

static inline void sw_serial_cpufreq_deregister(struct sw_uart_port *port)
{
}
#endif

static int sw_serial_get_resource(struct sw_uart_port *sport)
{
    char name[16];
    struct clk *pclk = NULL;
    char uart_para[16];
    int ret;
    
    UART_DBG(sport, "L%d, %s\n", __LINE__, __FUNCTION__);
    /* get register base */
    sport->mmres = platform_get_resource(sport->pdev, IORESOURCE_MEM, 0);
    if (!sport->mmres)
    {
        ret = -ENODEV;
        goto err_out;
    }
    
    sport->mmres = request_mem_region(sport->mmres->start, RESSIZE(sport->mmres), sport->pdev->name);
    if (!sport->mmres)
    {
        ret = -ENOENT;
        goto err_out;
    }
    
    sport->port.membase = ioremap(sport->mmres->start, sport->mmres->end - sport->mmres->start + 1);
    if (!sport->port.membase) {
        ret = -ENOMEM;
        goto free_mem_region;
    }

    /* get clock */
    pclk = clk_get(&sport->pdev->dev, "apb1");
    if (IS_ERR(pclk)) {
        ret = PTR_ERR(pclk);
        UART_DBG(sport, "Error to get clk\n");
        goto iounmap;
    }
    sport->sclk = clk_get_rate(pclk);
    clk_put(pclk);
    
    sprintf(name, "apb_uart%d", sport->port_no);
    sport->clk = clk_get(&sport->pdev->dev, name);
    if (IS_ERR(sport->clk)) {
        ret = PTR_ERR(sport->clk);
        UART_DBG(sport, "Error to get apb clk\n");
        goto iounmap;
    }
    
    /* get irq */
    sport->irq = platform_get_irq(sport->pdev, 0);
    if (sport->irq == 0)
    {
        UART_DBG(sport, "Failed to get interrupt resouce.\n");
        ret = -EINVAL;
        goto free_pclk;
    }
    
    /* get gpio resource */
    sprintf(uart_para, "uart_para%d", sport->port_no);
    sport->pio_hdle = gpio_request_ex(uart_para, NULL);
    if (!sport->pio_hdle)
    {
        UART_DBG(sport, "Failed to get gpio res.\n");
        ret = -EINVAL;
        goto free_pclk;
    }
    return 0;

free_pclk:
    clk_put(sport->clk);
iounmap:
    iounmap(sport->port.membase);
free_mem_region:
    release_mem_region(sport->mmres->start, RESSIZE(sport->mmres));
err_out:
    return ret;
}

static int sw_serial_put_resource(struct sw_uart_port *sport)
{
    UART_DBG(sport, "L%d, %s\n", __LINE__, __FUNCTION__);
    clk_disable(sport->clk);
    clk_put(sport->clk);
    
    iounmap(sport->port.membase);
    release_mem_region(sport->mmres->start, RESSIZE(sport->mmres));
    
    gpio_release(sport->pio_hdle, 1);
    return 0;
}

static int sw_serial_get_config(struct sw_uart_port *sport, u32 uart_id)
{
    char uart_para[16];
    int ret;
    
    sprintf(uart_para, "uart_para%d", uart_id);
    
    UART_DBG(sport, "sw_serial_get_config para %s\n", uart_para);
    ret = script_parser_fetch(uart_para, "uart_port", &sport->port_no, sizeof(int));
    if (ret) {
        UART_DBG(sport, "failed to get port num\n");
        return -1;
    }
    if (sport->port_no != uart_id) {
        UART_DBG(sport, "port NO. doesn't equals to device id(%d)\n", uart_id);
        return -1;
    }
    ret = script_parser_fetch(uart_para, "uart_type", &sport->pin_num, sizeof(int));
    if (ret) {
        UART_DBG(sport, "failed to get pin num\n");
        return -1;
    }
    
    return 0;
}

static int sw_serial_probe(struct platform_device *dev)
{
    struct sw_uart_port *sport;
    int ret;

    UART_INF("sw_serial_probe, %d\n", dev->id);
    sport = kzalloc(sizeof(struct sw_uart_port), GFP_KERNEL);
    if (!sport)
        return -ENOMEM;
    sport->port_no  = dev->id;
    sport->pdev     = dev;
    
    ret = sw_serial_get_config(sport, dev->id);
    if (ret)
    {
        UART_DBG(sport, "Failed to get config information\n");
        goto free_dev;
    }
    
    ret = sw_serial_get_resource(sport);
    if (ret)
    {
        UART_DBG(sport, "Failed to get resource\n");
        goto free_dev;
    }
    
    sport->port.type    = PORT_SW;
    sport->port.iotype  = UPIO_MEM;
    sport->port.mapbase = sport->mmres->start;
    sport->port.irq     = sport->irq;
    sport->port.fifosize= 64;
    sport->port.ops     = &sw_serial_pops;
    sport->port.line    = sport->port_no;
    sport->port.dev     = &dev->dev;
    sport->port.flags   = UPF_IOREMAP | UPF_BOOT_AUTOCONF;
    sport->port.uartclk = sport->sclk;

    sprintf(sport->name, "sw-uart%d", dev->id);
    sw_serial_ports[dev->id] = sport;

    uart_add_one_port(&sw_uart_driver, &sport->port);
    platform_set_drvdata(dev, sport);

    ret = sw_serial_cpufreq_register(sport);
    if (ret < 0)
        UART_DBG(sport, "failed to add cpufreq notifier\n");

    return 0;

free_dev:
    kfree(sport);
    sport = NULL;
    sw_serial_ports[dev->id] = NULL;
    return ret;
}

static int sw_serial_remove(struct platform_device *dev)
{
    struct sw_uart_port *sport = platform_get_drvdata(dev);

    platform_set_drvdata(dev, NULL);
    uart_remove_one_port(&sw_uart_driver, &sport->port);
    
    sw_serial_put_resource(sport);
    kfree(sport);

    return 0;
}

static struct platform_driver sw_serial_driver = {
    .probe  = sw_serial_probe,
    .remove = sw_serial_remove,
    .suspend= sw_serial_suspend,
    .resume = sw_serial_resume,
    .driver = {
        .name    = "sw-uart",
        .owner    = THIS_MODULE,
    },
};

static struct resource sw_uart_res[8][2] = {
    /* uart0 resource */
    {
        {.start = UARTx_BASE(0),      .end = UARTx_BASE(0) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART0, .end = SW_INT_IRQNO_UART0,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart1 resource */
    {
        {.start = UARTx_BASE(1),      .end = UARTx_BASE(1) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART1, .end = SW_INT_IRQNO_UART1,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart2 resource */
    {
        {.start = UARTx_BASE(2),      .end = UARTx_BASE(2) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART2, .end = SW_INT_IRQNO_UART2,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart3 resource */
    {
        {.start = UARTx_BASE(3),      .end = UARTx_BASE(3) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART3, .end = SW_INT_IRQNO_UART3,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart4 resource */
    {
        {.start = UARTx_BASE(4),      .end = UARTx_BASE(4) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART4, .end = SW_INT_IRQNO_UART4,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart5 resource */
    {
        {.start = UARTx_BASE(5),      .end = UARTx_BASE(5) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART5, .end = SW_INT_IRQNO_UART5,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart6 resource */
    {
        {.start = UARTx_BASE(6),      .end = UARTx_BASE(6) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART6, .end = SW_INT_IRQNO_UART6,           .flags = IORESOURCE_IRQ}, /*irq */
    },
    /* uart7 resource */
    {
        {.start = UARTx_BASE(7),      .end = UARTx_BASE(7) + UART_BASE_OS, .flags = IORESOURCE_MEM}, /*base*/
        {.start = SW_INT_IRQNO_UART7, .end = SW_INT_IRQNO_UART7,           .flags = IORESOURCE_IRQ}, /*irq */
    },
};

struct platform_device sw_uart_dev[] = {
    [0] = {.name = "sw-uart", .id = 0, .num_resources = ARRAY_SIZE(sw_uart_res[0]), .resource = &sw_uart_res[0][0], .dev = {}},
    [1] = {.name = "sw-uart", .id = 1, .num_resources = ARRAY_SIZE(sw_uart_res[1]), .resource = &sw_uart_res[1][0], .dev = {}},
    [2] = {.name = "sw-uart", .id = 2, .num_resources = ARRAY_SIZE(sw_uart_res[2]), .resource = &sw_uart_res[2][0], .dev = {}},
    [3] = {.name = "sw-uart", .id = 3, .num_resources = ARRAY_SIZE(sw_uart_res[3]), .resource = &sw_uart_res[3][0], .dev = {}},
    [4] = {.name = "sw-uart", .id = 4, .num_resources = ARRAY_SIZE(sw_uart_res[4]), .resource = &sw_uart_res[4][0], .dev = {}},
    [5] = {.name = "sw-uart", .id = 5, .num_resources = ARRAY_SIZE(sw_uart_res[5]), .resource = &sw_uart_res[5][0], .dev = {}},
    [6] = {.name = "sw-uart", .id = 6, .num_resources = ARRAY_SIZE(sw_uart_res[6]), .resource = &sw_uart_res[6][0], .dev = {}},
    [7] = {.name = "sw-uart", .id = 7, .num_resources = ARRAY_SIZE(sw_uart_res[7]), .resource = &sw_uart_res[7][0], .dev = {}},
};

#if 1
void sw_reconfig_uart(void)
{
    struct sw_uart_port *port;
    struct uart_port *uport;
    struct ktermios *termios;
    struct tty_struct *tty;
    int i;

    for(i=0; i<4; i++){
        port = sw_serial_ports[i];
        if (port == NULL)
            continue;

        uport = &port->port;
        if (uport == NULL)
            continue;

        if (uport->state == NULL)
            continue;

        tty = uport->state->port.tty;

        if (tty == NULL)
            continue;

        termios = tty->termios;

        if (termios == NULL) {
            printk(KERN_WARNING "%s: no termios?\n", __func__);
            continue;
        }

        sw_serial_set_termios(uport, termios, NULL);
    }
}

EXPORT_SYMBOL_GPL(sw_reconfig_uart);

#endif

static int uart_used;

int __init sw_serial_init(void)
{
    int ret;
    int i;
    int used = 0;
    char uart_para[16];
    
    #ifdef DEBUG_UART
    uart_init(DBG_UART, 115200);
    #endif
    UART_INF("debug uart init\n");
    
    uart_used = 0;
    for (i=0; i<8; i++, used=0)
    {
        sprintf(uart_para, "uart_para%d", i);
        ret = script_parser_fetch(uart_para, "uart_used", &used, sizeof(int));
        if (ret) {
            UART_INF("failed to get uart%d's used information\n", i);
        }
        if (used)
        {
            uart_used |= 1 << i;
            platform_device_register(&sw_uart_dev[i]);
        }
    }
    
    UART_INF("sw_serial_init\n");
    if (uart_used)
    {
        UART_INF("used uart info. %02x\n", uart_used);
        ret = uart_register_driver(&sw_uart_driver);
        if (likely(ret == 0)) {
            ret = platform_driver_register(&sw_serial_driver);
            if (unlikely(ret))
                uart_unregister_driver(&sw_uart_driver);
        }

        return ret;
    }

    UART_INF("sw_serial_init ok\n");
    return 0;
}

void __exit sw_serial_exit(void)
{
    if (uart_used) {
        platform_driver_unregister(&sw_serial_driver);
        uart_unregister_driver(&sw_uart_driver);
    }
}

module_init(sw_serial_init);
module_exit(sw_serial_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sw-uart");
