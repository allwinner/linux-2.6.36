/*
 *  linux/drivers/serial/pxa.c
 *
 *  Based on drivers/serial/8250.c by Russell King.
 *
 *  Author:	Nicolas Pitre
 *  Created:	Feb 20, 2003
 *  Copyright:	(C) 2003 Monta Vista Software, Inc.
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


struct uart_sw_port {
	struct uart_port        port;
	unsigned char           ier;
	unsigned char           lcr;
	unsigned char           mcr;
	unsigned int            lsr_break_flag;
	struct clk		*clk;
	char			name[16];

#ifdef CONFIG_CPU_FREQ
	struct notifier_block		freq_transition;
#endif

};

extern struct platform_device awuart_device;

static inline unsigned int serial_in(struct uart_sw_port *up, int offset)
{
	offset <<= 2;
	return readl(up->port.membase + offset);
}

static inline void serial_out(struct uart_sw_port *up, int offset, int value)
{
	offset <<= 2;
	writel(value, up->port.membase + offset);
}

static void serial_sw_enable_ms(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	up->ier |= UART_IER_MSI;
	serial_out(up, UART_IER, up->ier);
}

static void serial_sw_stop_tx(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	if (up->ier & UART_IER_THRI) {
		up->ier &= ~UART_IER_THRI;
		serial_out(up, UART_IER, up->ier);
	}
}

static void serial_sw_stop_rx(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	up->ier &= ~UART_IER_RLSI;
	up->port.read_status_mask &= ~UART_LSR_DR;
	serial_out(up, UART_IER, up->ier);
}

static inline void receive_chars(struct uart_sw_port *up, int *status)
{
	struct tty_struct *tty = up->port.state->port.tty;
	unsigned int ch, flag;
	int max_count = 256;

	do {
		ch = serial_in(up, UART_RX);
		flag = TTY_NORMAL;
		up->port.icount.rx++;

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
	} while ((*status & UART_LSR_DR) && (max_count-- > 0));
	tty_flip_buffer_push(tty);
}

static void transmit_chars(struct uart_sw_port *up)
{
	struct circ_buf *xmit = &up->port.state->xmit;
	int count;

	if (up->port.x_char) {
		serial_out(up, UART_TX, up->port.x_char);
		up->port.icount.tx++;
		up->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&up->port)) {
		serial_sw_stop_tx(&up->port);
		return;
	}

	count = up->port.fifosize / 2;
	do {
		serial_out(up, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		up->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&up->port);


	if (uart_circ_empty(xmit))
		serial_sw_stop_tx(&up->port);
}

static void serial_sw_start_tx(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	if (!(up->ier & UART_IER_THRI)) {
		up->ier |= UART_IER_THRI;
		serial_out(up, UART_IER, up->ier);
	}
}

static inline void check_modem_status(struct uart_sw_port *up)
{
	int status;

	status = serial_in(up, UART_MSR);

	if ((status & UART_MSR_ANY_DELTA) == 0)
		return;

	if (status & UART_MSR_TERI)
		up->port.icount.rng++;
	if (status & UART_MSR_DDSR)
		up->port.icount.dsr++;
	if (status & UART_MSR_DDCD)
		uart_handle_dcd_change(&up->port, status & UART_MSR_DCD);
	if (status & UART_MSR_DCTS)
		uart_handle_cts_change(&up->port, status & UART_MSR_CTS);

	wake_up_interruptible(&up->port.state->port.delta_msr_wait);
}

/*
 * This handles the interrupt from one port.
 */
static inline irqreturn_t serial_sw_irq(int irq, void *dev_id)
{
	struct uart_sw_port *up = dev_id;
	unsigned int iir, lsr;

	iir = serial_in(up, UART_IIR);
	if (iir & UART_IIR_NO_INT)
		return IRQ_NONE;
	lsr = serial_in(up, UART_LSR);
	if (lsr & UART_LSR_DR)
		receive_chars(up, &lsr);
	check_modem_status(up);
	if (lsr & UART_LSR_THRE)
		transmit_chars(up);
	return IRQ_HANDLED;
}

static unsigned int serial_sw_tx_empty(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned long flags;
	unsigned int ret;

	spin_lock_irqsave(&up->port.lock, flags);
	ret = serial_in(up, UART_LSR) & UART_LSR_TEMT ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&up->port.lock, flags);

	return ret;
}

static unsigned int serial_sw_get_mctrl(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned char status;
	unsigned int ret;

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

static void serial_sw_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned char mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= UART_MCR_DTR;
	if (mctrl & TIOCM_OUT1)
		mcr |= UART_MCR_OUT1;
	if (mctrl & TIOCM_OUT2)
		mcr |= UART_MCR_OUT2;
	if (mctrl & TIOCM_LOOP)
		mcr |= UART_MCR_LOOP;

	mcr |= up->mcr;

	serial_out(up, UART_MCR, mcr);
}

static void serial_sw_break_ctl(struct uart_port *port, int break_state)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);
	if (break_state == -1)
		up->lcr |= UART_LCR_SBC;
	else
		up->lcr &= ~UART_LCR_SBC;
	serial_out(up, UART_LCR, up->lcr);
	spin_unlock_irqrestore(&up->port.lock, flags);
}

#if 0
static void serial_sw_dma_init(struct sw_uart *up)
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

static int serial_sw_startup(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned long flags;
	int retval;

	clk_enable(up->clk);

	up->port.uartclk = clk_get_rate(up->clk);

	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(up->port.irq, serial_sw_irq, 0, up->name, up);
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
	serial_sw_set_mctrl(&up->port, up->port.mctrl);
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

static void serial_sw_shutdown(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned long flags;

	free_irq(up->port.irq, up);

	/*
	 * Disable interrupts from this port
	 */
	up->ier = 0;
	serial_out(up, UART_IER, 0);

	spin_lock_irqsave(&up->port.lock, flags);
	up->port.mctrl &= ~TIOCM_OUT2;
	serial_sw_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);

	/*
	 * Disable break condition and FIFOs
	 */
	serial_out(up, UART_LCR, serial_in(up, UART_LCR) & ~UART_LCR_SBC);
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
				  UART_FCR_CLEAR_RCVR |
				  UART_FCR_CLEAR_XMIT);
	serial_out(up, UART_FCR, 0);

	//clk_disable(up->clk);
}

static void
serial_sw_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned char cval, fcr = 0;
	unsigned long flags;
	unsigned int baud, quot;

	clk_enable(up->clk);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cval = UART_LCR_WLEN5;
		break;
	case CS6:
		cval = UART_LCR_WLEN6;
		break;
	case CS7:
		cval = UART_LCR_WLEN7;
		break;
	default:
	case CS8:
		cval = UART_LCR_WLEN8;
		break;
	}
	
	if (termios->c_cflag & CSTOPB)
		cval |= UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		cval |= UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		cval |= UART_LCR_EPAR;
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
	if (UART_ENABLE_MS(&up->port, termios->c_cflag))
		up->ier |= UART_IER_MSI;

	serial_out(up, UART_IER, up->ier);

	if (termios->c_cflag & CRTSCTS)
		up->mcr |= UART_MCR_AFE;
	else
		up->mcr &= ~UART_MCR_AFE;

	serial_out(up, UART_LCR, cval | UART_LCR_DLAB);/* set DLAB */
	serial_out(up, UART_DLL, quot & 0xff);		/* LS of divisor */
	serial_out(up, UART_DLM, quot >> 8);		/* MS of divisor */
	serial_out(up, UART_LCR, cval);		/* reset DLAB */
	up->lcr = cval;					/* Save LCR */
	serial_sw_set_mctrl(&up->port, up->port.mctrl);
	serial_out(up, UART_FCR, fcr);
	spin_unlock_irqrestore(&up->port.lock, flags);

	//clk_disable(up->clk);
}

static void
serial_sw_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	if (!state)
		clk_enable(up->clk);
	else
		clk_disable(up->clk);
}

static void serial_sw_release_port(struct uart_port *port)
{
}

static int serial_sw_request_port(struct uart_port *port)
{
	return 0;
}

static void serial_sw_config_port(struct uart_port *port, int flags)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	up->port.type = PORT_SW;
}

static int
serial_sw_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	/* we don't want the core code to modify any port params */
	return -EINVAL;
}

static const char *
serial_sw_type(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	return up->name;
}

static struct uart_sw_port *serial_sw_ports[4];
static struct uart_driver serial_sw_reg;

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

/*
 *	Wait for transmitter & holding register to empty
 */
static inline void wait_for_xmitr(struct uart_sw_port *up)
{
	unsigned int status, tmout = 10000;

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

static void serial_sw_console_putchar(struct uart_port *port, int ch)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	wait_for_xmitr(up);
	serial_out(up, UART_TX, ch);
}

/*
 * Print a string to the serial port trying not to disturb
 * any possible real use of the port...
 *
 *	The console_lock must be held when we get here.
 */
static void
serial_sw_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_sw_port *up = serial_sw_ports[co->index];
	unsigned int ier;

	clk_enable(up->clk);

	/*
	 *	First save the IER then disable the interrupts
	 */
	ier = serial_in(up, UART_IER);
	serial_out(up, UART_IER, UART_IER_UUE);

	uart_console_write(&up->port, s, count, serial_sw_console_putchar);

	/*
	 *	Finally, wait for transmitter to become empty
	 *	and restore the IER
	 */
	wait_for_xmitr(up);
	serial_out(up, UART_IER, ier);

	//clk_disable(up->clk);
}

static int __init
serial_sw_console_setup(struct console *co, char *options)
{
	struct uart_sw_port *up;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index == -1 || co->index >= serial_sw_reg.nr)
		co->index = 0;
	up = serial_sw_ports[co->index];
	if (!up)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(&up->port, co, baud, parity, bits, flow);
}

static struct console serial_sw_console = {
	.name		= "ttyS",
	.write		= serial_sw_console_write,
	.device		= uart_console_device,
	.setup		= serial_sw_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &serial_sw_reg,
};

#define SW_CONSOLE	&serial_sw_console
#else
#define SW_CONSOLE	NULL
#endif


#ifdef CONFIG_CONSOLE_POLL
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int serial_sw_get_poll_char(struct uart_port *port)
{
	struct uart_sw_port *up = (struct uart_sw_port *)port;
	unsigned char lsr;
	int res;

	clk_enable(up->clk);

	lsr = serial_in(up, UART_LSR);

	while (!(lsr & UART_LSR_DR))
		lsr = serial_in(up, UART_LSR);

	res = serial_in(up, UART_RX);

	//clk_disable(up->clk);

	return res;
}


static void serial_sw_put_poll_char(struct uart_port *port,
			 unsigned char c)
{
	unsigned int ier;
	struct uart_sw_port *up = (struct uart_sw_port *)port;

	clk_enable(up->clk);

	/*
	 *	First save the IER then disable the interrupts
	 */
	ier = serial_in(up, UART_IER);
	serial_out(up, UART_IER, 0);

	wait_for_xmitr(up);
	/*
	 *	Send the character out.
	 *	If a LF, also do CR...
	 */
	serial_out(up, UART_TX, c);
	if (c == 10) {
		wait_for_xmitr(up);
		serial_out(up, UART_TX, 13);
	}

	/*
	 *	Finally, wait for transmitter to become empty
	 *	and restore the IER
	 */
	wait_for_xmitr(up);
	serial_out(up, UART_IER, ier);

	//clk_disable(up->clk);
}

#endif /* CONFIG_CONSOLE_POLL */

struct uart_ops serial_sw_pops = {
	.tx_empty	= serial_sw_tx_empty,
	.set_mctrl	= serial_sw_set_mctrl,
	.get_mctrl	= serial_sw_get_mctrl,
	.stop_tx	= serial_sw_stop_tx,
	.start_tx	= serial_sw_start_tx,
	.stop_rx	= serial_sw_stop_rx,
	.enable_ms	= serial_sw_enable_ms,
	.break_ctl	= serial_sw_break_ctl,
	.startup	= serial_sw_startup,
	.shutdown	= serial_sw_shutdown,
	.set_termios	= serial_sw_set_termios,
	.pm		= serial_sw_pm,
	.type		= serial_sw_type,
	.release_port	= serial_sw_release_port,
	.request_port	= serial_sw_request_port,
	.config_port	= serial_sw_config_port,
	.verify_port	= serial_sw_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = serial_sw_get_poll_char,
	.poll_put_char = serial_sw_put_poll_char,
#endif
};

static struct uart_driver serial_sw_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= "SW serial",
	.dev_name	= "ttyS",
	.major		= TTY_MAJOR,
	.minor		= 64,
	.nr		= 4,
	.cons		= SW_CONSOLE,
};

static int serial_sw_suspend(struct platform_device *dev, pm_message_t state)
{
        struct uart_sw_port *sport = platform_get_drvdata(dev);

        if (sport)
                uart_suspend_port(&serial_sw_reg, &sport->port);

        return 0;
}

static int serial_sw_resume(struct platform_device *dev)
{
        struct uart_sw_port *sport = platform_get_drvdata(dev);

        if (sport)
                uart_resume_port(&serial_sw_reg, &sport->port);

        return 0;
}

#if 0
//#ifdef CONFIG_CPU_FREQ

static int sw_serial_cpufreq_transition(struct notifier_block *nb,
					     unsigned long val, void *data)
{
	struct uart_sw_port *port;
	struct uart_port *uport;

	port = container_of(nb, struct uart_sw_port, freq_transition);
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

		serial_sw_set_termios(uport, termios, NULL);
	}

 exit:
	return 0;
}

static inline int sw_serial_cpufreq_register(struct uart_sw_port *port)
{
	port->freq_transition.notifier_call = sw_serial_cpufreq_transition;

	return cpufreq_register_notifier(&port->freq_transition,
					 CPUFREQ_TRANSITION_NOTIFIER);
}

static inline void sw_serial_cpufreq_deregister(struct uart_sw_port *port)
{
	cpufreq_unregister_notifier(&port->freq_transition,
				    CPUFREQ_TRANSITION_NOTIFIER);
}

#else
static inline int sw_serial_cpufreq_register(struct uart_sw_port *port)
{
	return 0;
}

static inline void sw_serial_cpufreq_deregister(struct uart_sw_port *port)
{
}
#endif


static int serial_sw_probe(struct platform_device *dev)
{
	struct uart_sw_port *sport;
	struct resource *mmres, *irqres;
	struct clk *p = NULL;
	char name_buf[10];
	int ret;

	mmres = platform_get_resource(dev, IORESOURCE_MEM, 0);
	irqres = platform_get_resource(dev, IORESOURCE_IRQ, 0);
	if (!mmres || !irqres)
		return -ENODEV;


	sport = kmalloc(sizeof(struct uart_sw_port), GFP_KERNEL);
	if (!sport)
		return -ENOMEM;

	memset(sport, 0, sizeof(*sport));

	sprintf(name_buf, "uart%d", dev->id);
	sport->clk = clk_get(&dev->dev, name_buf);

	if (IS_ERR(sport->clk)) {
		ret = PTR_ERR(sport->clk);
		dev_err(&dev->dev, "Error to get clk for uart\n");
		goto err_free;
	}

	/* set uart clk as its parent */
	p = clk_get_parent(sport->clk);
	if(p == NULL || IS_ERR(p)){
		ret = -ENODEV;
		dev_err(&dev->dev, " Error to get parent clk for uart\n");
		goto err_free;
	}

	ret = clk_set_rate(sport->clk, clk_get_rate(p));
	clk_enable(sport->clk);

	sport->port.type = PORT_SW;
	sport->port.iotype = UPIO_MEM;
	sport->port.mapbase = mmres->start;
	sport->port.irq = irqres->start;
	sport->port.fifosize = 16;
	sport->port.ops = &serial_sw_pops;
	sport->port.line = dev->id;
	sport->port.dev = &dev->dev;
	sport->port.flags = UPF_IOREMAP | UPF_BOOT_AUTOCONF;
	sport->port.uartclk = clk_get_rate(sport->clk);

	sprintf(sport->name, "SWUART-%d", dev->id);

	sport->port.membase = ioremap(mmres->start, mmres->end - mmres->start + 1);
	if (!sport->port.membase) {
		ret = -ENOMEM;
		goto err_clk;
	}

	serial_sw_ports[dev->id] = sport;

	uart_add_one_port(&serial_sw_reg, &sport->port);
	platform_set_drvdata(dev, sport);

	ret = sw_serial_cpufreq_register(sport);
	if (ret < 0)
		dev_err(&dev->dev, "failed to add cpufreq notifier\n");

	switch (dev->id) {
		case 0:
			//only set to gpio 0 CFG
			__REG(VA_PIO_BASE + PIOC_REG_o_B_PULL0) &= ~((3<<4)|(3<<8));
			__REG(VA_PIO_BASE + PIOC_REG_o_B_PULL0) |=((1<<4)|(1<<8));
			__REG(VA_PIO_BASE + PIOC_REG_o_B_CFG0)  &=~((7<<16)|(7<<8));
			__REG(VA_PIO_BASE + PIOC_REG_o_B_CFG0)  |=	( ( 4<<16 ) | ( 4<<8 ) );
			break;

		case 1:
			//only set to gpio 0 CFG
			__REG(VA_PIO_BASE + PIOC_REG_o_B_PULL0) &= ~( ( 3<<18 ) | ( 3<<20 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_B_PULL0) |=	( ( 1<<18 ) | ( 1<<20 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_B_CFG1)  &= ~( ( 7<<4 ) | ( 7<<8 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_B_CFG1)  |=	( ( 2<<4 ) | ( 2<<8 ) );
			break;

		case 2:
			//only set to gpio 0 CFG
			__REG(VA_PIO_BASE + PIOC_REG_o_B_PULL1) &= ~( ( 3<<0 ) | ( 3<<2 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_B_PULL1) |=	( ( 1<<0 ) | ( 1<<2 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_B_CFG2)  &= ~( ( 7<<4 ) | ( 7<<0 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_B_CFG2)  |=	( ( 2<<4 ) | ( 2<<0 ) );
			break;

		case 3:
			//only set to gpio 0 CFG
			__REG(VA_PIO_BASE + PIOC_REG_o_A_PULL0) &= ~( ( 3<<0 ) | ( 3<<2 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_A_PULL0) |=	( ( 1<<0 ) | ( 1<<2 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_A_CFG0)  &= ~( ( 7<<4 ) | ( 7<<0 ) );
			__REG(VA_PIO_BASE + PIOC_REG_o_A_CFG0)  |=	( ( 3<<4 ) | ( 3<<0 ) );
			break;
	} 


	return 0;

 err_clk:
	clk_put(sport->clk);
 err_free:
	kfree(sport);
	return ret;
}

static int serial_sw_remove(struct platform_device *dev)
{
	struct uart_sw_port *sport = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	uart_remove_one_port(&serial_sw_reg, &sport->port);
	clk_put(sport->clk);
	kfree(sport);

	return 0;
}

static struct platform_driver serial_sw_driver = {
        .probe          = serial_sw_probe,
        .remove         = serial_sw_remove,

	.suspend	= serial_sw_suspend,
	.resume		= serial_sw_resume,
	.driver		= {
	        .name	= "sw2xx-uart",
		.owner	= THIS_MODULE,
	},
};

static struct resource aw_console_resources0[] = {
	[0] = {
		.start	= (UART_BASE0-0xf0000000) + (0 * 0x400),
		.end	= (UART_BASE0-0xf0000000) + (0 * 0x400) + 0x400,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SW_INT_IRQNO_UART0 + 0,
		.end	= SW_INT_IRQNO_UART0 + 0,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device awuart_device0 = {
	.name		= "sw2xx-uart",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(aw_console_resources0),
	.resource	=aw_console_resources0,
	.dev		= {
	}
};

static struct resource aw_console_resources3[] = {
	[0] = {
		.start	= (UART_BASE0-0xf0000000) + (3 * 0x400),
		.end	= (UART_BASE0-0xf0000000) + (3 * 0x400) + 0x400,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
			.start	= SW_INT_IRQNO_UART0 + 3,
			.end	= SW_INT_IRQNO_UART0 + 3,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device awuart_device3 = {
	.name		= "sw2xx-uart",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(aw_console_resources3),
	.resource	=aw_console_resources3,
	.dev		= {
	}

};

#if 1
void sw_reconfig_uart(void)
{
	struct uart_sw_port *port;
	struct uart_port *uport;
	struct ktermios *termios;
	struct tty_struct *tty;
	int i;

	for(i=0; i<4; i++){
		port = serial_sw_ports[i];
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

		serial_sw_set_termios(uport, termios, NULL);
	}
}

EXPORT_SYMBOL_GPL(sw_reconfig_uart);

#endif

int __init serial_sw_init(void)
{
	int ret;

	ret = uart_register_driver(&serial_sw_reg);
	if (ret != 0)
		return ret;

	ret = platform_driver_register(&serial_sw_driver);
	if (ret != 0){
		uart_unregister_driver(&serial_sw_reg);
		return ret;
	}

	ret = platform_device_register(&awuart_device0);
	if (ret != 0){
		platform_driver_unregister(&serial_sw_driver);
		uart_unregister_driver(&serial_sw_reg);
		return ret;
	}

#if 0
	ret = platform_device_register(&awuart_device3);
	if (ret != 0){
		platform_driver_unregister(&serial_sw_driver);
		uart_unregister_driver(&serial_sw_reg);
		platform_device_unregister(&awuart_device0);
		return ret;
	}
#endif

	return ret;
}

void __exit serial_sw_exit(void)
{
	platform_driver_unregister(&serial_sw_driver);
	uart_unregister_driver(&serial_sw_reg);
	platform_device_unregister(&awuart_device);
}

module_init(serial_sw_init);
module_exit(serial_sw_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sw2xx-uart");
