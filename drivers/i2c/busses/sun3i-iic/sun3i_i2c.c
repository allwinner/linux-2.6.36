/*
 * allwinner 
 * victor.wei
 * 2011-03-09
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <asm/irq.h>

#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#include "sun3i_i2c_private.h"
#include "sun3i_i2c.h"


static unsigned int i2c_debug = 0;

#define AWXX_I2C_OK      0
#define AWXX_I2C_FAIL   -1
#define AWXX_I2C_RETRY  -2
#define AWXX_I2C_SFAIL  -3  /* start fail */
#define AWXX_I2C_TFAIL  -4  /* stop  fail */

/*aw_i2c_adapter: transfer status */
enum
{
    I2C_XFER_IDLE    = 0x1,
    I2C_XFER_START   = 0x2,
    I2C_XFER_RUNNING = 0x4,    
};

struct awxx_i2c {
	
	int bus_num; 
	unsigned int      status; /* start, running, idle */

	spinlock_t          lock; /* syn */
	wait_queue_head_t   wait;
	struct i2c_msg      *msg;
	unsigned int		msg_num;
	unsigned int		msg_idx;
	unsigned int		msg_ptr;

	struct i2c_adapter	adap;
	
	struct clk		 *clk;
	unsigned int     bus_freq;
	unsigned int     gpio_hdle;

	void __iomem	 *base_addr;

	unsigned long		iobase; // for remove
	unsigned long		iosize; // for remove
	
	int			irq;
		
#ifdef CONFIG_CPU_FREQ
	struct notifier_block	freq_transition;
#endif	

	 unsigned int debug_state; /* log the twi machine state */
	 
};


#define SYS_I2C_PIN

#ifndef SYS_I2C_PIN

static void* __iomem gpio_addr = NULL;
// gpio twi0
#define _PIO_BASE_ADDRESS    (0x01c20800)
#define _Pn_CFG0(n) ( (n)*0x24 + 0x00 + gpio_addr )
#define _Pn_DRV0(n) ( (n)*0x24 + 0x14 + gpio_addr )
#define _Pn_PUL0(n) ( (n)*0x24 + 0x1C + gpio_addr )
//gpio twi1 twi2
#define _Pn_CFG1(n) ( (n)*0x24 + 0x08 + gpio_addr )
#define _Pn_DRV1(n) ( (n)*0x24 + 0x18 + gpio_addr )
#define _Pn_PUL1(n) ( (n)*0x24 + 0x20 + gpio_addr )

#endif

/*
 * Forward declarations
 *
 */
static int i2c_awxx_xfer_complete(struct awxx_i2c *i2c, int code);
static int i2c_awxx_do_xfer(struct awxx_i2c *i2c, struct i2c_msg *msgs, int num);


static int aw_twi_enable_sys_clk(struct awxx_i2c *i2c)
{	
	return clk_enable(i2c->clk);
}

static void aw_twi_disable_sys_clk(struct awxx_i2c *i2c)
{
    clk_disable(i2c->clk); 
}

static int aw_twi_request_gpio(struct awxx_i2c *i2c)
{
#ifndef SYS_I2C_PIN	
	if(i2c->bus_num == 0) {

	    //configuration register
	    unsigned int  reg_val = readl(_Pn_CFG0(1));
	    reg_val &= ~(0x77);/* pb0-pb1 TWI0 SDA,SCK */
	    reg_val |= (0x22);
	    writel(reg_val, _Pn_CFG0(1));
	
	    // pull up resistor
	    reg_val = readl(_Pn_PUL0(1));
	    reg_val &= ~(0xf);
	    reg_val |= 0x5;
	    writel(reg_val, _Pn_PUL0(1)); 
	
	    //driving default 
	    reg_val = readl(_Pn_DRV0(1));
	    reg_val &= ~(0xf);
	    reg_val |= 0x5;
	    writel(reg_val, _Pn_DRV0(1));
	}
	else if(i2c->bus_num == 1) {
	    //configuration register
	    unsigned int  reg_val = readl(_Pn_CFG1(1));
	    reg_val &= ~(0x7700);/* pb18-pb19 TWI1 scl,sda */
	    reg_val |= (0x2200);
	    writel(reg_val, _Pn_CFG1(1));
	
	    // pull up resistor
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= 0x50;
	    writel(reg_val, _Pn_PUL1(1)); 
	
	    //driving default 
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= 0x50;
	    writel(reg_val, _Pn_DRV1(1));		
	}
	else if(i2c->bus_num == 2) {
	    //configuration register
	    unsigned int  reg_val = readl(_Pn_CFG1(1));
	    reg_val &= ~(0x770000);/* pb20-pb21 TWI2 scl,sda */
	    reg_val |= (0x220000);
	    writel(reg_val, _Pn_CFG1(1));
	
	    // pull up resistor
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= 0x500;
	    writel(reg_val, _Pn_PUL1(1)); 
	
	    //driving default 
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= 0x500;
	    writel(reg_val, _Pn_DRV1(1));		
	}  	
#else
    if(i2c->bus_num == 0) {
        /* pb0-pb1 TWI0 SDA,SCK */        

        i2c->gpio_hdle = gpio_request_ex("twi0_para", NULL);
        if(!i2c->gpio_hdle) {
            pr_warning("twi0 request gpio fail!\n");
            return -1;
        }
    }
    else if(i2c->bus_num == 1) {
        /* pb18-pb19 TWI1 scl,sda */
        i2c->gpio_hdle = gpio_request_ex("twi1_para", NULL);
        if(!i2c->gpio_hdle) {
            pr_warning("twi1 request gpio fail!\n");
            return -1;
        }           
    }
    else if(i2c->bus_num == 2) {
        /* pb20-pb21 TWI2 scl,sda */
        i2c->gpio_hdle = gpio_request_ex("twi2_para", NULL);
        if(!i2c->gpio_hdle) {
            pr_warning("twi2 request gpio fail!\n");
            return -1;
        }         
    }  

#endif
    return 0;
}

static void aw_twi_release_gpio(struct awxx_i2c *i2c)
{
#ifndef SYS_I2C_PIN
	if(i2c->bus_num == 0) {
		
	    //config reigster
	    unsigned int  reg_val = readl(_Pn_CFG0(1));
	    reg_val &= ~(0x77);
	    writel(reg_val, _Pn_CFG0(1));    
	
	    //driving register
	    reg_val = readl(_Pn_DRV0(1));
	    reg_val &= ~(0xf);
	    reg_val |= (0x5);
	    writel(reg_val, _Pn_DRV0(1));
	
	    // pull up resistor
	    reg_val = readl(_Pn_PUL0(1));
	    reg_val &= ~(0xf);
	    reg_val |= (0x5);
	    writel(reg_val, _Pn_PUL0(1));
	}else if(i2c->bus_num == 1){
		
	    //config reigster
	    unsigned int  reg_val = readl(_Pn_CFG1(1));
	    reg_val &= ~(0x7700);
	    writel(reg_val, _Pn_CFG1(1));    
	
	    //driving register
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= (0x50);
	    writel(reg_val, _Pn_DRV1(1));
	
	    // pull up resistor
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= (0x50);
	    writel(reg_val, _Pn_PUL1(1));		
	}else if(i2c->bus_num == 2){
		
	    //config reigster
	    unsigned int  reg_val = readl(_Pn_CFG1(1));
	    reg_val &= ~(0x770000);
	    writel(reg_val, _Pn_CFG1(1));    
	
	    //driving register
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= (0x500);
	    writel(reg_val, _Pn_DRV1(1));
	
	    // pull up resistor
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= (0x500);
	    writel(reg_val, _Pn_PUL1(1));		
	}    
#else
    if(i2c->bus_num == 0) {
        /* pb0-pb1 TWI0 SDA,SCK */
        gpio_release(i2c->gpio_hdle, 0);
    }
    else if(i2c->bus_num == 1) {
        /* pb18-pb19 TWI1 scl,sda */
        gpio_release(i2c->gpio_hdle, 0);   
    }
    else if(i2c->bus_num == 2) {
        /* pb20-pb21 TWI2 scl,sda */
        gpio_release(i2c->gpio_hdle, 0); 
    }  

#endif

}


/* function  */
static int aw_twi_start(void *base_addr)
{
    unsigned int timeout = 0xff;
    
    aw_twi_set_start(base_addr);
    while((1 == aw_twi_get_start(base_addr))&&(--timeout));
    if(timeout == 0)
    {
        printk("START can't sendout!\n");
        return AWXX_I2C_FAIL;
    }
    
    return AWXX_I2C_OK;
}

static int aw_twi_restart(void  *base_addr)
{
    int ret = AWXX_I2C_FAIL;
    
    ret = aw_twi_start(base_addr);
    if(ret == AWXX_I2C_FAIL)
    {
        return AWXX_I2C_FAIL;
    }
    aw_twi_clear_irq_flag(base_addr);    
    return AWXX_I2C_OK;
}

static int aw_twi_stop(void *base_addr)
{
    unsigned int timeout = 0xff;
    
    aw_twi_set_stop(base_addr);
    aw_twi_clear_irq_flag(base_addr);

    aw_twi_get_stop(base_addr);/* it must delay 1 nop to check stop bit */
    while(( 1 == aw_twi_get_stop(base_addr))&& (--timeout));
    if(timeout == 0)
    {
	    pr_debug("1.STOP can't sendout!\n"); 
	    return AWXX_I2C_FAIL;
    }
    
    timeout = 0xff;
    while((TWI_STAT_IDLE != readl(base_addr + TWI_STAT_REG))&&(--timeout));
    if(timeout == 0)
    {
        pr_debug("i2c state isn't idle(0xf8)\n");
        return AWXX_I2C_FAIL;
    }
    
    timeout = 0xff;
    while((TWI_LCR_IDLE_STATUS != readl(base_addr + TWI_LCR_REG))&&(--timeout));
    if(timeout == 0)
    {
        pr_debug("2.STOP can't sendout!\n");
        return AWXX_I2C_FAIL;
    }
   
    return AWXX_I2C_OK;
}

/*
****************************************************************************************************
*
*  FunctionName:           i2c_awxx_addr_byte
*
*  Description:
*            发送slave地址，7bit的全部信息，及10bit的第一部分地址。供外部接口调用，内部实现。
*         7bits addr: 7-1bits addr+0 bit r/w
*         10bits addr: 1111_11xx_xxxx_xxxx-->1111_0xx_rw,xxxx_xxxx
*         send the 7 bits addr,or the first part of 10 bits addr
*  Parameters:
*       
*       
*  Return value:
*           无
*  Notes:
*       
****************************************************************************************************
*/
static void i2c_awxx_addr_byte(struct awxx_i2c *i2c)
{  
    unsigned char addr = 0;//address
    unsigned char tmp  = 0;

    if(i2c->msg[i2c->msg_idx].flags & I2C_M_TEN)
    {
        tmp = 0x78 | ( ( (i2c->msg[i2c->msg_idx].addr)>>8 ) & 0x03);//0111_10xx,ten bits address--9:8bits
        addr = tmp << 1;//1111_0xx0
        //how about the second part of ten bits addr???
        //Answer: deal at twi_core_process()
    }
    else
    {
        addr = (i2c->msg[i2c->msg_idx].addr & 0x7f) << 1;// 7-1bits addr,xxxx_xxx0
    }    
    //read,default value is write
    if (i2c->msg[i2c->msg_idx].flags & I2C_M_RD)
    {
        addr |= 1;
    } 	
    //send 7bits+r/w or the first part of 10bits
    aw_twi_put_byte(i2c->base_addr, &addr);

    return;	
}


static int i2c_awxx_core_process(struct awxx_i2c *i2c)
{
    void *base_addr = i2c->base_addr;
    int  ret        = AWXX_I2C_OK;
    int  err_code   = 0;
    unsigned char  state = 0;
    unsigned char  tmp   = 0;     
    
    state = aw_twi_query_irq_status(base_addr);
    
    //printk(" state = %x\n", state);
  
    switch(state) 
    {
    case 0xf8: /* On reset or stop the bus is idle, use only at poll method */
        {   
            err_code = 0xf8;  
            goto err_out;
        }
    case 0x08: /* A START condition has been transmitted */
    case 0x10: /* A repeated start condition has been transmitted */
        {     
            i2c_awxx_addr_byte(i2c);/* send slave address */
            break;
        }
    case 0xd8: /* second addr has transmitted, ACK not received!    */
    case 0x20: /* SLA+W has been transmitted; NOT ACK has been received */
        {   
            err_code = 0x20;
            goto err_out;
        }        
    case 0x18: /* SLA+W has been transmitted; ACK has been received */
        {     
            /* if any, send second part of 10 bits addr */
            if(i2c->msg[i2c->msg_idx].flags & I2C_M_TEN)
            {
                tmp = i2c->msg[i2c->msg_idx].addr & 0x7f;  /* the remaining 8 bits of address */
                aw_twi_put_byte(base_addr, &tmp); /* case 0xd0: */
                break;
            }             
            /* for 7 bit addr, then directly send data byte--case 0xd0:  */
        }
    case 0xd0: /* second addr has transmitted,ACK received!     */      
    case 0x28: /* Data byte in DATA REG has been transmitted; ACK has been received */
        {   
            /* after send register address then START send write data  */
            if(i2c->msg_ptr < i2c->msg[i2c->msg_idx].len) 
            {
                aw_twi_put_byte(base_addr, &(i2c->msg[i2c->msg_idx].buf[i2c->msg_ptr]));
                i2c->msg_ptr++;
                break;
            }                                  

            i2c->msg_idx++; /* the other msg */
            i2c->msg_ptr = 0;
            if (i2c->msg_idx == i2c->msg_num)
            {
                err_code = AWXX_I2C_OK;/* Success,wakeup */
                goto ok_out;
            }            
            else if(i2c->msg_idx < i2c->msg_num)/* for restart pattern */
            {                    
                ret = aw_twi_restart(base_addr);/* read spec, two msgs */
                if(ret == AWXX_I2C_FAIL)
                {
                    err_code = AWXX_I2C_SFAIL;
                    goto err_out;/* START can't sendout */
                }
            }       
            else
            {   /* unknown error  */
                err_code = AWXX_I2C_FAIL;
                goto err_out;
            }
            break;
        }
    case 0x30: /* Data byte in I2CDAT has been transmitted; NOT ACK has been received */
        {     
            err_code = 0x30;//err,wakeup the thread            
            goto err_out;
        }
    case 0x38: /* Arbitration lost during SLA+W, SLA+R or data bytes */
        {                    
            err_code = 0x38;//err,wakeup the thread
            goto err_out;
        }
    case 0x40: /* SLA+R has been transmitted; ACK has been received */
        {     
            /* with Restart,needn't to send second part of 10 bits addr,refer-"I2C-SPEC v2.1" */
            /* enable A_ACK need it(receive data len) more than 1. */
            if(i2c->msg[i2c->msg_idx].len > 1)
            {
                /* send register addr complete,then enable the A_ACK and get ready for receiving data */
                aw_twi_enable_ack(base_addr);
                aw_twi_clear_irq_flag(base_addr);/* jump to case 0x50 */
            }
            else if(i2c->msg[i2c->msg_idx].len == 1)
            {
                aw_twi_clear_irq_flag(base_addr);/* jump to case 0x58 */
            }
            break;
        }
    case 0x48: /* SLA+R has been transmitted; NOT ACK has been received */
        {    
            err_code = 0x48;//err,wakeup the thread
            goto err_out;
        }
    case 0x50: /* Data bytes has been received; ACK has been transmitted */
        {
            /* receive first data byte */
            if (i2c->msg_ptr < i2c->msg[i2c->msg_idx].len) 
            { 
                /* more than 2 bytes, the last byte need not to send ACK */
                if( (i2c->msg_ptr + 2) == i2c->msg[i2c->msg_idx].len )
                {
                    aw_twi_disable_ack(base_addr);/* last byte no ACK */
                } 
                /* get data then clear flag,then next data comming */                
                aw_twi_get_byte(base_addr, &i2c->msg[i2c->msg_idx].buf[i2c->msg_ptr]);
                i2c->msg_ptr++;  
               
                break;
            }
            /* err process, the last byte should be @case 0x58 */
            err_code = AWXX_I2C_FAIL;/* err, wakeup */
            goto err_out;             
        }
    case 0x58: /* Data byte has been received; NOT ACK has been transmitted */
        {    /* received the last byte  */
            if ( i2c->msg_ptr == i2c->msg[i2c->msg_idx].len - 1 ) 
            {                
                aw_twi_get_last_byte(base_addr, &i2c->msg[i2c->msg_idx].buf[i2c->msg_ptr]);
                i2c->msg_idx++; 
                i2c->msg_ptr = 0;
                if (i2c->msg_idx == i2c->msg_num)
                {
                    err_code = AWXX_I2C_OK; // succeed,wakeup the thread
                    goto ok_out;                   
                }
                else if(i2c->msg_idx < i2c->msg_num)
                {
                    /* repeat start */
                    ret = aw_twi_restart(base_addr);
                    if(ret == AWXX_I2C_FAIL) /* START fail */
                    {
                        err_code = AWXX_I2C_SFAIL;
                        goto err_out;
                    }                    
                    break;                
                }
            } 
            else 
            {
                err_code = 0x58;
                goto err_out;            
            }
        }     
    case 0x00: /* Bus error during master or slave mode due to illegal level condition */
        {      
            err_code = 0xff;
            goto err_out;
        }      
    default:
        {   
            err_code = state;
            goto err_out;
        }    
    }
    i2c->debug_state = state;/* just for debug */
    return ret;

ok_out:
err_out:
    if(AWXX_I2C_FAIL == aw_twi_stop(base_addr))
    {
        pr_debug("STOP failed!\n");
    }

    ret = i2c_awxx_xfer_complete(i2c, err_code);/* wake up */
    
    i2c->debug_state = state;/* just for debug */    
    return ret;
}

static irqreturn_t  i2c_awxx_handler(int this_irq, void * dev_id)
{
    struct awxx_i2c *i2c = (struct awxx_i2c *)dev_id;
    int ret = AWXX_I2C_FAIL;    
   
    if(!aw_twi_query_irq_flag(i2c->base_addr))
    {
        pr_warning("unknown interrupt!");
        return ret;
        //return IRQ_HANDLED;
    } 
    
    /* disable irq */
    aw_twi_disable_irq(i2c->base_addr);
    //twi core process 
    ret = i2c_awxx_core_process(i2c);
    
    /* enable irq only when twi is transfering, otherwise,disable irq */
    if(i2c->status != I2C_XFER_IDLE)
    {
        aw_twi_enable_irq(i2c->base_addr);   
    }
    
    return IRQ_HANDLED;
}

static int i2c_awxx_xfer_complete(struct awxx_i2c *i2c, int code)
{
    int ret = AWXX_I2C_OK; 
    
    i2c->msg     = NULL;
    i2c->msg_num = 0;
    i2c->msg_ptr = 0;        
    i2c->status  = I2C_XFER_IDLE;
    /* i2c->msg_idx  store the information */

    if(code == AWXX_I2C_FAIL)
    {
        pr_debug("Maybe Logic Error,debug it!\n");
        i2c->msg_idx = code;
        ret = AWXX_I2C_FAIL;
    }
    else if(code != AWXX_I2C_OK)
    {
        //return the ERROR code, for debug or detect error type
        i2c->msg_idx = code;
        ret = AWXX_I2C_FAIL;        
    }

    wake_up(&i2c->wait);
    
    return ret;
}


static int i2c_awxx_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct awxx_i2c *i2c = (struct awxx_i2c *)adap->algo_data;
	int ret = AWXX_I2C_FAIL;
	int i   = 0;

	for(i = adap->retries; i >= 0; i--) 
	{	
		ret = i2c_awxx_do_xfer(i2c, msgs, num);
		
		if(ret != AWXX_I2C_RETRY) 
		{
			goto out;
		}

		if(i2c_debug) 
		{
			dev_dbg(&adap->dev, "Retrying transmission\n");
		}
		udelay(100);		
	}	
	
	ret = -EREMOTEIO;
	
out:	
	return ret;
}

static int i2c_awxx_do_xfer(struct awxx_i2c *i2c, struct i2c_msg *msgs, int num)
{
	unsigned long timeout = 0;
	int ret = AWXX_I2C_FAIL;    
	//int i = 0, j =0;
    
	aw_twi_soft_reset(i2c->base_addr);
	udelay(100);
    
	/* test the bus is free,already protect by the semaphore at DEV layer */
	while( TWI_STAT_IDLE != aw_twi_query_irq_status(i2c->base_addr)&& 
	       TWI_STAT_BUS_ERR != aw_twi_query_irq_status(i2c->base_addr) && 
	       TWI_STAT_ARBLOST_SLAR_ACK != aw_twi_query_irq_status(i2c->base_addr) ) {
		pr_debug("bus is busy, status = %x\n", aw_twi_query_irq_status(i2c->base_addr));
		ret = AWXX_I2C_RETRY;
		goto out;
	}    	
	//printk("bus num = %d\n", i2c->adap.nr);
	//printk("bus name = %s\n", i2c->adap.name);    
	/* may conflict with xfer_complete */
	spin_lock_irq(&i2c->lock);
	i2c->msg     = msgs;
	i2c->msg_num = num;
	i2c->msg_ptr = 0;
	i2c->msg_idx = 0;
	i2c->status  = I2C_XFER_START;	
	spin_unlock_irq(&i2c->lock);	
/*
	for(i =0 ; i < num; i++){
		for(j = 0; j < msgs->len; j++){
			printk("baddr = %x \n",msgs->addr);
			printk("data = %x \n", msgs->buf[j]);
		}
		printk("\n\n");
}
*/ 	
	aw_twi_enable_irq(i2c->base_addr);  /* enable irq */   
	aw_twi_disable_ack(i2c->base_addr); /* disabe ACK */
	aw_twi_set_EFR(i2c->base_addr, 0);  /* set the special function register,default:0. */
    
	ret = aw_twi_start(i2c->base_addr);/* START signal,needn't clear int flag  */
	if(ret == AWXX_I2C_FAIL) {
		aw_twi_soft_reset(i2c->base_addr);
		aw_twi_disable_irq(i2c->base_addr);  /* disable irq */ 
		i2c->status  = I2C_XFER_IDLE;
		ret = AWXX_I2C_RETRY;
		goto out;
	}    
	/* sleep and wait, do the transfer at interrupt handler ,timeout = 5*HZ */
	timeout = wait_event_timeout(i2c->wait, i2c->msg_num == 0, i2c->adap.timeout);
	/* return code,if(msg_idx == num) succeed */
	ret = i2c->msg_idx;

	if (timeout == 0){
		//dev_dbg(i2c->adap.dev, "timeout \n");
		pr_warning("i2c-%d, xfer timeout\n", i2c->bus_num);
	}
	else if (ret != num){
		pr_warning("incomplete xfer (%d\n", ret);
		//dev_dbg(i2c->adap.dev, "incomplete xfer (%d)\n", ret);
	}	    
out:
	return ret;
}

static unsigned int i2c_awxx_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C|I2C_FUNC_10BIT_ADDR|I2C_FUNC_SMBUS_EMUL;
}


static const struct i2c_algorithm i2c_awxx_algorithm = {
	.master_xfer	  = i2c_awxx_xfer,
	.functionality	  = i2c_awxx_functionality,
};

static int i2c_awxx_hw_init(struct awxx_i2c *i2c)
{
    unsigned int apb_clk = 0;
    int ret = 0;

    // enable GPIO pin
    ret = aw_twi_request_gpio(i2c);
    if(ret == -1){
    	pr_debug("request i2c gpio failed!\n");
    	return -1;
    }
    
    // enable APB clk
    ret = aw_twi_enable_sys_clk(i2c);
    if(ret == -1){
    	pr_debug("enable i2c clock failed!\n");
    	return -1;
    }
 
    // enable twi bus    
    aw_twi_enable_bus(i2c->base_addr);
    
    // set twi module clock
    apb_clk  =  clk_get_rate(i2c->clk);
    if(apb_clk == 0){
    	pr_debug("get i2c source clock frequency failed!\n");
    	return -1;
    }
    pr_debug("twi%d, apb clock = %d \n",i2c->bus_num, apb_clk);
    aw_twi_set_clock(apb_clk, i2c->bus_freq, i2c->base_addr);
    // soft reset
    aw_twi_soft_reset(i2c->base_addr);
    
    return ret;
}

static void i2c_awxx_hw_exit(struct awxx_i2c *i2c)
{
    void *base_addr = i2c->base_addr;
   // aw_twi_disable_irq(base_addr);        
    // disable twi bus    
    aw_twi_disable_bus(base_addr);
    // disable APB clk
    aw_twi_disable_sys_clk(i2c);
    // disable GPIO pin
    aw_twi_release_gpio(i2c);
}

static int i2c_awxx_probe(struct platform_device *dev)
{
	struct awxx_i2c *i2c = NULL;
	struct resource *res = NULL;
	struct aw_i2c_platform_data *pdata = NULL;
	char *i2c_clk[] ={"twi0","twi1","twi2"};
	int ret;
	int irq;
	
	pdata = dev->dev.platform_data;
	if(pdata == NULL) {
		return -ENODEV;
	}

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(dev, 0);
	if (res == NULL || irq < 0) {
		return -ENODEV;
	}

	if (!request_mem_region(res->start, resource_size(res), res->name)) {
		return -ENOMEM;
	}

	i2c = kzalloc(sizeof(struct awxx_i2c), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto emalloc;
	}

	strlcpy(i2c->adap.name, "aw16xx-i2c", sizeof(i2c->adap.name));
	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.nr      = pdata->bus_num;
	i2c->adap.retries = 2;		
	i2c->adap.timeout = 5*HZ;
	i2c->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD; 
	i2c->bus_freq     = pdata->frequency;
	i2c->irq 		  = irq;
	i2c->bus_num      = pdata->bus_num;
	spin_lock_init(&i2c->lock);	 
	init_waitqueue_head(&i2c->wait);
	
	i2c->clk = clk_get(NULL, i2c_clk[i2c->adap.nr]);
	if(NULL == i2c->clk){
		pr_debug("request i2c clock failed\n");
		ret = -EIO;
		goto eremap;
	}
	
	snprintf(i2c->adap.name, sizeof(i2c->adap.name), "aw16xx-i2c.%u", i2c->adap.nr);

	i2c->base_addr = ioremap(res->start, resource_size(res));
	if (!i2c->base_addr) {
		ret = -EIO;
		goto eremap;
	}
	
#ifndef SYS_I2C_PIN
	gpio_addr = ioremap(_PIO_BASE_ADDRESS, 0x1000);
	if(!gpio_addr) {
	    ret = -EIO;
	    goto ereqirq;	
	}
#endif

	i2c->adap.algo = &i2c_awxx_algorithm;
	ret = request_irq(irq, i2c_awxx_handler, IRQF_DISABLED, i2c->adap.name, i2c);
	if (ret)
	{
		goto ereqirq;
	}
	
	if(-1 == i2c_awxx_hw_init(i2c)){
		ret = -EIO;
		goto eadapt;
	}

	i2c->adap.algo_data  = i2c;
	i2c->adap.dev.parent = &dev->dev;

	i2c->iobase = res->start; // for release
	i2c->iosize = resource_size(res);

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		pr_debug(KERN_INFO "I2C: Failed to add bus\n");
		goto eadapt;
	}

	platform_set_drvdata(dev, i2c);

	pr_debug(KERN_INFO "I2C: %s: AW16XX I2C adapter\n",
	       dev_name(&i2c->adap.dev));

	return 0;

eadapt:	
	free_irq(irq, i2c);
	clk_disable(i2c->clk);
	
ereqirq:
#ifndef SYS_I2C_PIN
    if(gpio_addr){
        iounmap(gpio_addr);
    }
#endif    
	iounmap(i2c->base_addr);
	clk_put(i2c->clk);
	
eremap:	
	kfree(i2c);
	
emalloc:
	release_mem_region(i2c->iobase, i2c->iosize);
	
	return ret;	
}


static int __exit i2c_awxx_remove(struct platform_device *dev)
{
	struct awxx_i2c *i2c = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	i2c_del_adapter(&i2c->adap);
	
	free_irq(i2c->irq, i2c);

	i2c_awxx_hw_exit(i2c); // disable clock and release gpio
	clk_put(i2c->clk);
	
	iounmap(i2c->base_addr);
	release_mem_region(i2c->iobase, i2c->iosize);
	kfree(i2c);

	return 0;
}



#ifdef CONFIG_PM
static int i2c_awxx_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct awxx_i2c *i2c = platform_get_drvdata(pdev);

	printk("[i2c%d] suspend okay.. \n", i2c->bus_num);

	//i2c_awxx_hw_exit(i2c);

	return 0;
}

static int i2c_awxx_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct awxx_i2c *i2c = platform_get_drvdata(pdev);
	
    printk("[i2c%d] resume okay.. \n", i2c->bus_num);
    
	//i2c_awxx_hw_init(i2c);

	return 0;
}

static const struct dev_pm_ops i2c_awxx_dev_pm_ops = {
	.suspend = i2c_awxx_suspend,
	.resume = i2c_awxx_resume,
};

#define I2C_AWXX_DEV_PM_OPS (&i2c_awxx_dev_pm_ops)
#else
#define I2C_AWXX_DEV_PM_OPS NULL
#endif


static struct platform_driver twi_awxx_driver = {
	.probe		= i2c_awxx_probe,
	.remove		= __exit_p(i2c_awxx_remove),
	.driver		= {
			.name	= "aw16xx-i2c",
			.owner	= THIS_MODULE,
			.pm	= I2C_AWXX_DEV_PM_OPS,
	},
};


void __init aw_register_device(struct platform_device *dev, void *data)
{
	int ret;

	dev->dev.platform_data = data;

	ret = platform_device_register(dev);
	if (ret)
		dev_err(&dev->dev, "unable to register device: %d\n", ret);
}


static struct aw_i2c_platform_data aw_twi0_pdata = {
	.bus_num   = 0,
	.frequency = 400000,
};

static struct resource aw_twi0_resources[] = {
	{
		.start	= TWI0_BASE_ADDR_START,
		.end	= TWI0_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= INTC_IRQNO_TWI0,
		.end	= INTC_IRQNO_TWI0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device aw_twi0_device = {
	.name		= "aw16xx-i2c",
	.id		    = 0,
	.resource	= aw_twi0_resources,
	.num_resources	= ARRAY_SIZE(aw_twi0_resources),
};

//twi1 resource
static struct aw_i2c_platform_data aw_twi1_pdata = {
	.bus_num   = 1,
	.frequency = 400000,
};

static struct resource aw_twi1_resources[] = {
	{
		.start	= TWI1_BASE_ADDR_START,
		.end	= TWI1_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= INTC_IRQNO_TWI1,
		.end	= INTC_IRQNO_TWI1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device aw_twi1_device = {
	.name		= "aw16xx-i2c",
	.id		    = 1,
	.resource	= aw_twi1_resources,
	.num_resources	= ARRAY_SIZE(aw_twi1_resources),
};

// twi2 resource
static struct aw_i2c_platform_data aw_twi2_pdata = {
	.bus_num   = 2,
	.frequency = 400000,
};

static struct resource aw_twi2_resources[] = {
	{
		.start	= TWI2_BASE_ADDR_START,
		.end	= TWI2_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= INTC_IRQNO_TWI2,
		.end	= INTC_IRQNO_TWI2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device aw_twi2_device = {
	.name		= "aw16xx-i2c",
	.id		    = 2,
	.resource	= aw_twi2_resources,
	.num_resources	= ARRAY_SIZE(aw_twi2_resources),
};



// should be in the system initialization.
static struct i2c_board_info __initdata i2c_info_hdmi[] =  {
	{
		I2C_BOARD_INFO("anx7150_0", 0x76),
		.platform_data	= NULL,
	},
	{
		I2C_BOARD_INFO("anx7150_1", 0x7e),
		.platform_data	= NULL,
	},
};
/*
static struct i2c_board_info __initdata i2c_info_power[] =  {
	{
		I2C_BOARD_INFO("axp199", 0x68),
		.platform_data	= NULL,
	},
};
*/

/*
*    0  not used or error
*    1 used   
*/
static int i2c_awxx_get_cfg(int bus_num)
{
    int value = 0;
    int ret   = 0;
    char *main_name[] = {"twi0_para", "twi1_para", "twi2_para"};
    char *sub_name[] = {"twi0_used","twi1_used","twi2_used"};
    
    ret = script_parser_fetch(main_name[bus_num], sub_name[bus_num], &value, sizeof(int));
    if(ret != SCRIPT_PARSER_OK){
        pr_warning("get twi0 para failed,err code = %d \n",ret);
        return 0;
    }
    pr_debug("bus num = %d, twi used = %d \n", bus_num, value);
    return value;
}

static int __init i2c_adap_awxx_init(void)
{
	int status = 0;
	// bus-0
	//status = i2c_register_board_info(0, i2c_info_power, ARRAY_SIZE(i2c_info_power));
//	pr_debug("power, status = %d \n",status);
	// bus-1
	status = i2c_register_board_info(1, i2c_info_hdmi, ARRAY_SIZE(i2c_info_hdmi));	
	pr_debug("hdmi, status = %d \n",status);
	
	if(i2c_awxx_get_cfg(0)){
	    aw_register_device(&aw_twi0_device, &aw_twi0_pdata);
	}
    if(i2c_awxx_get_cfg(1)){
	    aw_register_device(&aw_twi1_device, &aw_twi1_pdata);
	}
    if(i2c_awxx_get_cfg(2)){
	    aw_register_device(&aw_twi2_device, &aw_twi2_pdata);
	}

	return platform_driver_register(&twi_awxx_driver);	
}


static void __exit i2c_adap_awxx_exit(void)
{
	platform_driver_unregister(&twi_awxx_driver);
}

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:awxx-i2c");
MODULE_DESCRIPTION("awxx I2C Bus driver");
MODULE_AUTHOR("VictorWei");


module_init(i2c_adap_awxx_init);
module_exit(i2c_adap_awxx_exit);

