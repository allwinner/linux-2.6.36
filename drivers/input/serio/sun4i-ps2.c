
#include <linux/module.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <mach/clock.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

//#define AW1623_FPGA

#define swps2_msg(...)  printk("[ps2]: " __VA_ARGS__);

#define RESSIZE(res)        (((res)->end - (res)->start)+1)

/* register define */
#define SW_PS2_0_PBASE       (0x01C2a000)
#define SW_PS2_1_PBASE       (0x01c2a400)

#define SW_PS2_GCTRL        (0x00)
#define SW_PS2_DATA         (0x04)
#define SW_PS2_LCTRL        (0x08)
#define SW_PS2_LSTAT        (0x0c)
#define SW_PS2_FCTRL        (0x10)
#define SW_PS2_FSTAT        (0x14)
#define SW_PS2_CLKDR        (0x18)

/* SW_PS2_GCTRL */
#define SWPS2_BUSEN         (1 << 0)
#define SWPS2_MASTER        (1 << 1)
#define SWPS2_RESET         (1 << 2)
#define SWPS2_INTEN         (1 << 3)
#define SWPS2_INTFLAG       (1 << 3)

/* SW_PS2_LCTRL */
#define SWPS2_LCTL_NOACK        (0x0 << 18)
#define SWPS2_LCTL_TXDTOEN      (0x1 << 8)
#define SWPS2_LCTL_STOPERREN    (0x1 << 3)
#define SWPS2_LCTL_ACKERREN     (0x1 << 2)
#define SWPS2_LCTL_PARERREN     (0x1 << 1)
#define SWPS2_LCTL_RXDTOEN      (0x1 << 0)

/* SW_PS2_FSTAT */
#define SWPS2_FSTA_RXRDY        (1 << 0)
#define SWPS2_FSTA_RXOF         (1 << 1)
#define SWPS2_FSTA_RXUF         (1 << 2)
#define SWPS2_FSTA_TXRDY        (1 << 8)
#define SWPS2_FSTA_TXOF         (1 << 9)
#define SWPS2_FSTA_TXUF         (1 << 10)

#define SW_PS2_SAMPLE_CLK      (1000000)
#define SW_PS2_SCLK            (125000)

struct sw_ps2_host {
    struct platform_device *pdev;
    void __iomem*       base;
    struct resource*    res;      /* resources found       */
    u32                 irq;
    struct clk*         pclk;
    struct clk*         mclk;
    u32                 pio_hdle;
    struct serio        serio;
};

static struct sw_ps2_host *sw_ps2c_host[2] = {NULL, NULL};
static int ps2_used[2] = {0};

static int sw_ps2_resource_request(struct sw_ps2_host* ps2_host)
{
    struct platform_device *pdev = ps2_host->pdev;
    u32 ps2_no = pdev->id;
    char* pio_para[] = {"ps2_0_para", "ps2_1_para"};
    u32 pio_hdle = 0;
    int ret;
    
    swps2_msg("ps2 %d request resource\n", ps2_no);
    
    /* request pins */
    #ifndef AW1623_FPGA
    pio_hdle = gpio_request_ex(pio_para[ps2_no], NULL);
    if (!pio_hdle)
    {
        swps2_msg("ps2 %d request pio parameter failed\n", ps2_no);
        goto out;
    }
    ps2_host->pio_hdle = pio_hdle;
    #else
    {
        #include <mach/platform.h>
        void __iomem* pi_cfg2 = (void __iomem*)(SW_VA_PORTC_IO_BASE+0x128);
        u32 rval = readl(pi_cfg2) & (~(0x77<<16));
        writel(rval|(0x22<<16), pi_cfg2);
    }
    ps2_host->pio_hdle = pio_hdle;
    #endif
    
    /* request memory */
    ps2_host->res  = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!ps2_host->res) 
    {
        swps2_msg("Failed to get io memory region resouce.\n");
        ret = -ENOENT;
        goto release_pins;
    }
    /* ps2 address remap */
    ps2_host->res = request_mem_region(ps2_host->res->start, RESSIZE(ps2_host->res), pdev->name);
    if (!ps2_host->res) 
    {
        swps2_msg("Failed to request io memory region.\n");
        ret = -ENOENT;
        goto release_pins;
    }
    ps2_host->base = ioremap(ps2_host->res->start, RESSIZE(ps2_host->res));
    if (!ps2_host->base) 
    {
        swps2_msg("Failed to ioremap() io memory region.\n");
        ret = -EINVAL;
        goto free_mem_region;
    }
    
    goto out;
    
free_mem_region:
    release_mem_region(ps2_host->res->start, RESSIZE(ps2_host->res));
release_pins:
    gpio_release(ps2_host->pio_hdle, 1);
out:
    return 0;
}

static int sw_ps2_resource_release(struct sw_ps2_host* ps2_host)
{
    swps2_msg("ps2 %d release resource\n", ps2_host->pdev->id);
    iounmap(ps2_host->base);
    release_mem_region(ps2_host->res->start, RESSIZE(ps2_host->res));
    gpio_release(ps2_host->pio_hdle, 1);
    
    return 0;
}

static int sw_ps2c_set_sclk(struct sw_ps2_host* ps2_host)
{
    struct clk* apb1_clk = NULL;
    u32 src_clk = 0;
    u32 clk_scdf;
    u32 clk_pcdf;
    u32 rval;
    
    apb1_clk = clk_get(&ps2_host->pdev->dev, "apb1");
    src_clk = clk_get_rate(apb1_clk);
    clk_put(apb1_clk);
    
    swps2_msg("source clock %d\n", src_clk);
    #ifdef AW1623_FPGA
    src_clk = 24000000;
    #endif
    
    if (!src_clk)
    {
        swps2_msg("sw_ps2c_set_sclk error, source clock is 0.\n");
        return -1;
    }
        
    clk_scdf = ((src_clk+(SW_PS2_SAMPLE_CLK>>1))/SW_PS2_SAMPLE_CLK -1);
    clk_pcdf = ((SW_PS2_SAMPLE_CLK+(SW_PS2_SCLK>>1)) /SW_PS2_SCLK - 1);
    rval = (clk_scdf<<8) | clk_pcdf;// | (PS2_DEBUG_SEL<<16);
    writel(rval, ps2_host->base + SW_PS2_CLKDR);
    
    return 0;
}

static int sw_ps2c_init(struct sw_ps2_host* ps2_host)
{
    u32 rval;
    
    //Set Line Control And Enable Interrupt
    rval = SWPS2_LCTL_TXDTOEN|SWPS2_LCTL_STOPERREN|SWPS2_LCTL_ACKERREN|SWPS2_LCTL_PARERREN|SWPS2_LCTL_RXDTOEN;
    writel(rval, ps2_host->base + SW_PS2_LCTRL);

    //Reset FIFO
    writel(0x3<<16 | 0x607, ps2_host->base + SW_PS2_FCTRL);

    //Set Clock Divider Register
    sw_ps2c_set_sclk(ps2_host);
    
    //Set Global Control Register
    rval = SWPS2_RESET|SWPS2_INTEN|SWPS2_MASTER|SWPS2_BUSEN;
    writel(rval, ps2_host->base + SW_PS2_GCTRL);
    
    udelay(100);
    
    return 0;    	      
}

static int sw_ps2_write(struct serio *dev, unsigned char val)
{
    struct sw_ps2_host* ps2_host = (struct sw_ps2_host *)dev->port_data;
    u32 timeout = 10000;
    
    do {
        if (readl(ps2_host->base + SW_PS2_FSTAT) & SWPS2_FSTA_TXRDY) {
           // swps2_msg("ps2 %d send byte %02x\n", ps2_host->pdev->id, val);
        	writel(val, ps2_host->base + SW_PS2_DATA);
        	return 0;
        }
    } while (timeout--);

    return -1;
}

static irqreturn_t sw_ps2_irq(int irq, void *dev_id)
{
    struct serio *dev = dev_id;
    struct sw_ps2_host* ps2_host = (struct sw_ps2_host *)dev->port_data;
    unsigned char byte;
    u32 rval;
    u32 line_sta;
    u32 fifo_sta;
    u32 error = 0;
    
    line_sta = readl(ps2_host->base + SW_PS2_LSTAT);
    fifo_sta = readl(ps2_host->base + SW_PS2_FSTAT);
    
//    swps2_msg("ps2 %d, irq ls %08x fs %08x\n", ps2_host->pdev->id, line_sta, fifo_sta);
    
    //Check Line Status Register
    if (line_sta & 0x10f)
    {
        if (line_sta & 0x08)
            swps2_msg("PS/2 %d Stop Bit Error!\n", ps2_host->pdev->id);
        if (line_sta & 0x04)
            swps2_msg("PS/2 %d Acknowledge Error!\n", ps2_host->pdev->id);
        if (line_sta & 0x02)
            swps2_msg("PS/2 %d Parity Error!\n", ps2_host->pdev->id);
        if (line_sta & 0x100)
            swps2_msg("PS/2 %d Transmit Data Timeout!\n", ps2_host->pdev->id);
        if (line_sta & 0x01)
            swps2_msg("PS/2 %d Receive Data Timeout!\n", ps2_host->pdev->id);
        
        writel(readl(ps2_host->base + SW_PS2_GCTRL)|0x4, ps2_host->base + SW_PS2_GCTRL);//reset PS/2 controller
        writel(0x10f, ps2_host->base + SW_PS2_LSTAT);
        error = 1;
    }
    
    //Check FIFO Status Register
    if (fifo_sta & 0x0606)
    {
        if (fifo_sta & 0x400)
            swps2_msg("PS/2 %d Tx FIFO Underflow!\n", ps2_host->pdev->id);
        if (fifo_sta & 0x200)
            swps2_msg("PS/2 %d Tx FIFO Overflow!\n", ps2_host->pdev->id);
        if (fifo_sta & 0x04)
            swps2_msg("PS/2 %d Rx FIFO Underflow!\n", ps2_host->pdev->id);
        if (fifo_sta & 0x02)
            swps2_msg("PS/2 %d Rx FIFO Overflow!\n", ps2_host->pdev->id);
            
        writel(readl(ps2_host->base + SW_PS2_GCTRL)|0x4, ps2_host->base + SW_PS2_GCTRL);//reset PS/2 controller
        writel(0x707, ps2_host->base + SW_PS2_FSTAT);
        error = 1;
    }
    
    rval = (fifo_sta >> 16) & 0x3;
    while (!error && rval--)
    {
        byte = readl(ps2_host->base + SW_PS2_DATA) & 0xff;
        //swps2_msg("ps2 %d rcv %02x\n", ps2_host->pdev->id, byte);
        serio_interrupt(dev, byte, 0);
    }
    
    writel(line_sta, ps2_host->base + SW_PS2_LSTAT);
    writel(fifo_sta, ps2_host->base + SW_PS2_FSTAT);
    return IRQ_HANDLED;
}

static int sw_ps2_open(struct serio *dev)
{
    struct sw_ps2_host* ps2_host = (struct sw_ps2_host *)dev->port_data;
    struct platform_device *pdev = ps2_host->pdev;
    char* ps2_pclk_name[] = {"apb_ps0", "apb_ps1"};
    int ret;
    //int i;
 
    swps2_msg("ps2 %d open\n", pdev->id);
/*	
    for (i=0; i<0x200; i+=4)                                        
    {                                                            
        if (!(i&0xf))                                             
            printk("\n0x%08x : ", i);                     
        printk("%08x ", readl((void __iomem*)SW_VA_PORTC_IO_BASE + i)); 
    }                    
    printk("\n");
*/
    /* get irq resource*/
    ps2_host->irq = platform_get_irq(pdev, 0);
    if (ps2_host->irq == 0) 
    {
        swps2_msg("Failed to get interrupt resouce.\n");
        ret = -EINVAL;
        goto out;
    }

    /* request irq */
    if (request_irq(ps2_host->irq, sw_ps2_irq, 0, "sw-ps2", dev)) 
    {
        swps2_msg("Failed to request ps2 interrupt.\n");
        ret = -EBUSY;
        goto out;
    }
    
    /* request clock */
    ps2_host->pclk = clk_get(&pdev->dev, ps2_pclk_name[pdev->id]);
    if (IS_ERR(ps2_host->pclk)) 
    {
        ret = PTR_ERR(ps2_host->pclk);
        swps2_msg("Error to get ahb clk for %s\n", ps2_pclk_name[pdev->id]);
        goto free_irq;
    }
    clk_enable(ps2_host->pclk);
    
    /* initial ps2 controller */
    ret = sw_ps2c_init(ps2_host);
    if (ret)
        goto close_pclk;
        
    ret = 0;
    goto out;
        
close_pclk:
    clk_disable(ps2_host->pclk);
    clk_put(ps2_host->pclk);

free_irq:
    free_irq(ps2_host->irq, dev);
    
out:
    return ret;
}

static void sw_ps2_close(struct serio *dev)
{
    struct sw_ps2_host* ps2_host = (struct sw_ps2_host *)dev->port_data;

    swps2_msg("ps2 %d close\n", ps2_host->pdev->id);
    
    writel(0, ps2_host->base + SW_PS2_GCTRL);
    
    free_irq(ps2_host->irq, dev);
    
    clk_disable(ps2_host->pclk);
    clk_put(ps2_host->pclk);
}

static struct resource sw_ps2_0_resource[] = {
        [0] = {
                .start  = SW_PS2_0_PBASE,
                .end    = SW_PS2_0_PBASE + 0x300 - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = SW_INT_IRQNO_PS2_0,
                .end    = SW_INT_IRQNO_PS2_0,
                .flags  = IORESOURCE_IRQ,
        },
};

static struct resource sw_ps2_1_resource[] = {
        [0] = {
                .start  = SW_PS2_1_PBASE,
                .end    = SW_PS2_1_PBASE + 0x300 - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = SW_INT_IRQNO_PS2_1,
                .end    = SW_INT_IRQNO_PS2_1,
                .flags  = IORESOURCE_IRQ,
        },
};

static struct platform_device sw_ps2_device[] = {
    [0] = {
        .name           = "sw-ps2",
        .id             = 0,
        .num_resources    = ARRAY_SIZE(sw_ps2_0_resource),
        .resource       = sw_ps2_0_resource,
        .dev            = {
        }
    },
    [1] = {
        .name           = "sw-ps2",
        .id             = 1,
        .num_resources    = ARRAY_SIZE(sw_ps2_1_resource),
        .resource       = sw_ps2_1_resource,
        .dev            = {
        }
    }
};

static int __devinit sw_ps2_probe(struct platform_device *pdev)
{
    int ret;
    struct serio *serio = NULL;
    struct sw_ps2_host* ps2_host = NULL;
    
    swps2_msg("ps2 %d probe\n", pdev->id);
    
    ps2_host = (struct sw_ps2_host*)kzalloc(sizeof(struct sw_ps2_host), GFP_KERNEL);
    if (!ps2_host)
    {
        ret = -ENOMEM;
        goto probe_out;
    }
    ps2_host->pdev = pdev;
    sw_ps2c_host[pdev->id] = ps2_host;
    
    ret = sw_ps2_resource_request(ps2_host);
    if (ret)
    {
        swps2_msg("sw_ps2_resource_request failed\n");
        kfree(ps2_host);
        ps2_host = NULL;
        sw_ps2c_host[pdev->id] = NULL;
        goto probe_out;
    }
    
    
    serio           = &ps2_host->serio;
    serio->id.type  = SERIO_8042;
    serio->write    = sw_ps2_write;
    serio->open     = sw_ps2_open;
    serio->close    = sw_ps2_close;
    snprintf(serio->name, sizeof(serio->name), "SW PS/2 port%d", pdev->id);
    snprintf(serio->phys, sizeof(serio->phys), "SW/serio%d", pdev->id);
    serio->port_data    = sw_ps2c_host[pdev->id];
    serio->dev.parent   = &sw_ps2_device->dev;
    
    serio_register_port(serio);
    
    swps2_msg("ps2 %d probe done, dev(%p), host(%p), serio(%p), base(%p)\n", pdev->id, pdev, ps2_host, serio, ps2_host->base);
    
    ret = 0;
probe_out:
    return ret;
}

static int __devexit sw_ps2_remove(struct platform_device *dev)
{
    struct sw_ps2_host* ps2_host = sw_ps2c_host[dev->id];
    struct serio *serio = &ps2_host->serio;
    
    swps2_msg("ps2 %d remove\n", dev->id);
    serio_unregister_port(serio);
    
    mdelay(2);
    sw_ps2_resource_release(ps2_host);
    
    kfree(ps2_host);
    ps2_host = NULL;
    sw_ps2c_host[dev->id] = NULL;
    
    return 0;
}


#ifdef CONFIG_PM
static int sw_ps2_suspend(struct device *dev)
{
    return 0;
}

static int sw_ps2_resume(struct device *dev)
{
    return 0;
}

static const struct dev_pm_ops sw_ps2_pm = {
    .suspend    = sw_ps2_suspend,
    .resume        = sw_ps2_resume,
};

#define sw_ps2_pm_ops &sw_ps2_pm

#else /* CONFIG_PM */

#define sw_ps2_pm_ops NULL

#endif /* CONFIG_PM */

static struct platform_driver sw_ps2_driver = {
    .driver.name    = "sw-ps2",
    .driver.owner   = THIS_MODULE,
    .driver.pm        = sw_ps2_pm_ops,
    .probe          = sw_ps2_probe,
    .remove         = __devexit_p(sw_ps2_remove),
};

static int __init sw_ps2_init(void)
{
    int ret;
    
    swps2_msg("sw_ps2_init\n");
    
    memset((void*)ps2_used, 0, sizeof(ps2_used));
    ret = script_parser_fetch("ps2_0_para","ps2_used", &ps2_used[0], sizeof(int));
    if (ret)
    {
        printk("sw_ps2_init fetch ps2_0 using configuration failed\n");
    }    
    ret = script_parser_fetch("ps2_1_para","ps2_used", &ps2_used[1], sizeof(int));
    if (ret)
    {
        printk("sw_ps2_init fetch ps2_1 using onfiguration failed\n");
    }
    #ifdef AW1623_FPGA
    ps2_used[0] = 1;
    #endif
    if (ps2_used[0])
    {
        platform_device_register(&sw_ps2_device[0]);
    }
    if (ps2_used[1])
    {
        platform_device_register(&sw_ps2_device[1]);
    }
    
    if (ps2_used[0] || ps2_used[1])
        return platform_driver_register(&sw_ps2_driver);
    else
    {
        pr_warning("ps2: cannot find any unsing configuration for 2 ps/2 controller, return directly!\n");
        return 0;
    }
}

static void __exit sw_ps2_exit(void)
{
    swps2_msg("sw_ps2_exit\n");
    
    if (ps2_used[0] || ps2_used[1])
    {
        ps2_used[0] = 0;
        ps2_used[1] = 0;
        platform_driver_unregister(&sw_ps2_driver);
    }
}

module_init(sw_ps2_init);
module_exit(sw_ps2_exit);

MODULE_AUTHOR("Aaron.maoye<leafy.myeh@allwinnertech.com");
MODULE_DESCRIPTION("SW F3X PS2 controller driver");
MODULE_LICENSE("GPL");
