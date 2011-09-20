#ifndef SW_UART_DBG_H
#define SW_UART_DBG_H

#define DBG_UART_RBR    (0x00)
#define DBG_UART_THR    (0x00) 
#define DBG_UART_DLL    (0x00)
#define DBG_UART_DLH    (0x04)
#define DBG_UART_IER    (0x04)
#define DBG_UART_IIR    (0x08)
#define DBG_UART_FCR    (0x08)
#define DBG_UART_LCR    (0x0c)
#define DBG_UART_MCR    (0x10)
#define DBG_UART_LSR    (0x14)
#define DBG_UART_MSR    (0x18)
#define DBG_UART_SCH    (0x1C)
#define DBG_UART_USR    (0x7C)
#define DBG_UART_TFL    (0x80)
#define DBG_UART_RFL    (0x84)
#define DBG_UART_HALT   (0xa4)

#define DBG_UART 3

static void uart_onoff(u32 id, u32 onoff)
{
    u32 rval;
    switch (id)
    {
        case 0:
            
            //clock
            rval = readl(SW_VA_CCM_IO_BASE + 0x6c); //apb gate
            rval |= 1 << 16;
            writel(rval, SW_VA_CCM_IO_BASE + 0x6c);
            break;
            
        case 1:
            
            //clock
            rval = readl(SW_VA_CCM_IO_BASE + 0x6c); //apb gate
            rval |= 1 << 17;
            writel(rval, SW_VA_CCM_IO_BASE + 0x6c);
            break;
            
        case 2: //PI 16, 17, 18, 19 - CTS RTS TX RX
            rval = readl(SW_VA_PORTC_IO_BASE + 0x128); //pi cfg2
            rval &= ~(0x77 << 8);
            rval |= 0x33 << 8;
            writel(rval, SW_VA_PORTC_IO_BASE + 0x128);
            
            rval = readl(SW_VA_PORTC_IO_BASE + 0x140);
            rval |= 0x5 << 4;
            writel(rval, SW_VA_PORTC_IO_BASE + 0x140);

            //clock
            rval = readl(SW_VA_CCM_IO_BASE + 0x6c); //apb gate
            rval |= 1 << 18;
            writel(rval, SW_VA_CCM_IO_BASE + 0x6c);
            break;
            
        case 3:
            //pg 6 7
            rval = readl(SW_VA_PORTC_IO_BASE + 0xd8); //pg cfg0
            rval &= ~(0x77 << 24);
            rval |= 0x44 << 24;
            writel(rval, SW_VA_PORTC_IO_BASE + 0xd8);
            
            rval = readl(SW_VA_PORTC_IO_BASE + 0xf4);
            rval |= 0x5 << 12;
            writel(rval, SW_VA_PORTC_IO_BASE + 0xf4);
            
            //clock
            rval = readl(SW_VA_CCM_IO_BASE + 0x6c); //apb gate
            rval |= 1 << 19;
            writel(rval, SW_VA_CCM_IO_BASE + 0x6c);
            break;
    }
}

static void uart_init(u32 id, u32 baudrate)
{
    void __iomem* base = (void __iomem*)SW_VA_UART0_IO_BASE + 0x400 * id;
    u32 pclk = 24000000;
    u32 df = (pclk + (baudrate << 3))/(baudrate << 4);
    u32 lcr = readl(base + DBG_UART_LCR);

    uart_onoff(id, 1);
    //set baudrate
    writel(1, base + DBG_UART_HALT);
    writel(lcr|0x80, base + DBG_UART_LCR);
    writel(df >> 8, base + DBG_UART_DLH);
    writel(df & 0xff, base + DBG_UART_DLL);
    writel((~0x80)&lcr, base + DBG_UART_LCR);
    writel(0, base + DBG_UART_HALT);
    
    //set mode
    writel(3, base + DBG_UART_LCR);
    //enable FIFO
    writel(7, base + DBG_UART_FCR);
}

//static void uart_send_char(u32 id, char c)
//{
//    void __iomem* base = (void __iomem*)SW_VA_UART0_IO_BASE + 0x400 * id;
//    while(!(readl(base + DBG_UART_USR)&0x2));
//    writel(c, base + DBG_UART_THR);
//}

static void uart_send_string(__u32 id, char* pstr)
{
    void __iomem* base = (void __iomem*)SW_VA_UART0_IO_BASE + 0x400 * id;
    while(*pstr != '\0')
    {
        while(!(readl(base + DBG_UART_USR)&0x02)); 
        writel((__u32)*pstr, base + DBG_UART_THR);
        pstr++;
    }
}
static void my_printk(const char* format, ...)
{
    char printf_buf[512] = {0};
    u32 index = 0;
    char* string;
	u32 value = 0;
    char fmt[8] = {0};
    u32 fmt_idx = 0;
	
	va_list arg_list;
	va_start(arg_list, format);
	
	while (*format!='\0')
	{
	    if (*format=='%')
	    {
            fmt_idx = 0;
            memset(fmt, 0, 8);
	        while (*format != 'x' && *format != 'X' && 
	                *format != 'd' && *format != 'D' && 
	                *format != 's' && *format != 'S')
	        {
	            fmt[fmt_idx++] = *format++;
	        }
	        fmt[fmt_idx++] = *format;
	        
			switch (*format)
			{
			    case 'x':
			    case 'X':
			    case 'd':
			    case 'D':
			        value = va_arg(arg_list, unsigned);
			        sprintf(&printf_buf[index], fmt, value);
                    break;
                case 's':
                case 'S':
                    string = va_arg(arg_list, char*);
			        sprintf(&printf_buf[index], fmt, string);
			    	break;
			}
			index += strlen(&printf_buf[index]);
			format++;
	    }
	    else
	    {
	        printf_buf[index++] = *format++;
	    }
	    
	}
	
	uart_send_string(DBG_UART, printf_buf);
	va_end(arg_list);
	
}


#endif
