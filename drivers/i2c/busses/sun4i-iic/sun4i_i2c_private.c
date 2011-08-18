/*
*********************************************************************************************************
*                                                      eBase
*                                           the Abstract of Hardware
*
*                                   (c) Copyright 2006-2010, AW China
*                                                   All Rights Reserved
*
* File        :  twi_private.c
* Date        :  2010-11-13
* Author      :  victor
* Version     :  v1.00
* Description:  
*                                   Operation for TWI module,aw1620
* History     :
*      <author>          <time>             <version >          <desc>
*       victor         2010-11-13              1.0           create this file     
*
*********************************************************************************************************
*/
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/dma.h>

#include "sun4i_i2c_private.h"
//#define CONFIG_FPGA_SIM
/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_get_byte
*
*  Description:
*               先取数据，再把flag清掉。
*  Parameters:
*       base_addr  :  寄存器基地址
*       buffer : 存放数据的指针
*
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_get_byte(void *base_addr, unsigned char  *buffer)
{    
    *buffer = (unsigned char)( TWI_DATA_MASK & readl(base_addr + TWI_DATA_REG) );
    aw_twi_clear_irq_flag(base_addr);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_get_last_byte
*
*  Description:
*              只取数据，清flag在stop里做。
*  Parameters:
*       base_addr  :  寄存器基地址
*       buffer : 存放数据的指针
*
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_get_last_byte(void *base_addr, unsigned char  *buffer)
{    
    *buffer = (unsigned char)( TWI_DATA_MASK & readl(base_addr + TWI_DATA_REG) );
    return;
}


/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_put_byte
*
*  Description:
*               先把数据放到data register，再把irq flag清零，触发发送。
*  Parameters:
*       base_addr  :  寄存器基地址
*          buffer  :  发送数据的指针
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_put_byte(void *base_addr, const unsigned char *buffer)
{
    //write data and clear irq flag to trigger send flow
    writel((unsigned int)*buffer, base_addr + TWI_DATA_REG);
    aw_twi_clear_irq_flag(base_addr);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_enable_irq
*
*  Description:
*               控制寄存器,中断使能位
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_enable_irq(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    
    /* enable irq: 2011-8-18 13:39:05
     * 1 when enable irq for next operation, set intflag to 1 to prevent to clear it by a mistake
     *   (intflag bit is write-0-to-clear bit)
     * 2 Similarly, mask startbit and stopbit to prevent to set it twice by a mistake
     *   (start bit and stop bit are self-clear-to-0 bits)
     */
    reg_val |= (TWI_CTL_INTEN | TWI_CTL_INTFLG);
    reg_val &= ~(TWI_CTL_STA | TWI_CTL_STP);
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_disable_irq
*
*  Description:
*               控制寄存器,中断使能位，关闭中断
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_disable_irq(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val &= ~TWI_CTL_INTEN;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_disable_bus
*
*  Description:
*               控制寄存器,关闭twi模块。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_disable_bus(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val &= ~TWI_CTL_BUSEN;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_enable_bus
*
*  Description:
*               控制寄存器,打开twi模块。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_enable_bus(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val |= TWI_CTL_BUSEN;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_set_start
*
*  Description:
*               控制寄存器,触发start信号。start位会自动清零。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_set_start(void *base_addr)
{    
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val |= TWI_CTL_STA;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;    
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_get_start
*
*  Description:
*              获取start bit的状态，主要用作查询start是否发出。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           start的状态
*  Notes:
*   
****************************************************************************************************
*/ 
unsigned int aw_twi_get_start(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val >>= 5;
    return reg_val & 1;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_set_stop
*
*  Description:
*               控制寄存器,触发stop信号。stop位会自动清零。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_set_stop(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val |= TWI_CTL_STP;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_get_stop
*
*  Description:
*              获取stop bit的状态，主要用作查询stop是否发出。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           stop的状态
*  Notes:
*   
****************************************************************************************************
*/ 
unsigned int aw_twi_get_stop(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val >>= 4;
    return reg_val & 1;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_disable_ack
*
*  Description:
*               控制寄存器,在发送ack或nack期间，会自动发送nack。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_disable_ack(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val &= ~TWI_CTL_ACK;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_enable_ack
*
*  Description:
*               控制寄存器,在发送ack或nack期间，会自动发送ack。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_enable_ack(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val |= TWI_CTL_ACK;
    writel(reg_val, base_addr + TWI_CTL_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_clear_irq_flag
*
*  Description:
*               控制寄存器,清除中断到来标志位。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_clear_irq_flag(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    reg_val &= ~TWI_CTL_INTFLG;//0x 1111_0111
    writel(reg_val ,base_addr + TWI_CTL_REG);
    
    //read two more times to make sure that interrupt flag does really be cleared
    {
        unsigned int temp;
        temp = readl(base_addr + TWI_CTL_REG);
        temp |= readl(base_addr + TWI_CTL_REG);
    }
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_query_irq_flag
*
*  Description:
*               控制寄存器,查询中断标志位。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*           返回中断标志位状态。
*  Notes:
*   
****************************************************************************************************
*/ 
unsigned int aw_twi_query_irq_flag(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    return (reg_val & TWI_CTL_INTFLG);//0x 0000_1000
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_query_irq_status
*
*  Description:
*               控制寄存器,查询中断状态码。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*          中断状态码。
*  Notes:
*   
****************************************************************************************************
*/ 
unsigned int aw_twi_query_irq_status(void *base_addr)
{ 
    unsigned int reg_val = readl(base_addr + TWI_STAT_REG);
    return (reg_val & TWI_STAT_MASK);
}

/*
****************************************************************************************************
*
*  FunctionName:           _twi_set_clk
*
*  Description:
*              时钟寄存器
*  Parameters:
*       clk_n   :  分频系数CLK_N
*       clk_m   :  分频系数CLK_M
*       base_addr  :  寄存器基地址
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/ 
static void _twi_set_clk(unsigned int clk_n, unsigned int clk_m, void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CLK_REG);
    printk("_twi_set_clk: clk_n = %d, clk_m = %d\n", clk_n, clk_m);
    reg_val &= ~(TWI_CLK_DIV_M | TWI_CLK_DIV_N);
    reg_val |= ( clk_n |(clk_m << 3) );
    writel(reg_val, base_addr + TWI_CLK_REG);
    return;
}

/*   
*/
/*
****************************************************************************************************
*
*  FunctionName:      aw_twi_set_clock
*
*  Description:
*             时钟寄存器,要求设置的scl时钟频率跟实际计算出来的存在一定误差，但是逼近。
*                    Fin is APB CLOCK INPUT;
*                    Fsample = F0 = Fin/2^CLK_N; 
*                              F1 = F0/(CLK_M+1);
*                              
*                    Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10); 
*                    Foscl is clock SCL;100KHz or 400KHz      
*                    clk为传入的apb时钟源，要求的理想时钟为sclk
*  Parameters:
*       clk_in   :  apb clk时钟
*       sclk_req   :  要设置的scl时钟频率 单位:HZ
*       base_addr  :  寄存器基地址
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_set_clock(unsigned int clk_in, unsigned int sclk_req, void *base_addr)
{ 
    unsigned int clk_m = 0;
    unsigned int clk_n = 0;
    unsigned int _2_pow_clk_n = 1;
    unsigned int src_clk      = clk_in/10;   
    unsigned int divider      = src_clk/sclk_req;  // 400khz or 100khz  
    unsigned int sclk_real    = 0;      // the real clock frequency

    //added by young, for testing
#ifdef CONFIG_FPGA_SIM
   {
        clk_m = 2;
        clk_n = 3;
        goto set_clk;
    }
#endif

    if (divider==0)
    {
        clk_m = 1;
        goto set_clk;
    }
    // search clk_n and clk_m,from large to small value so that can quickly find suitable m & n.
    while (clk_n < 8) // 3bits max value is 8 
    {
        // (m+1)*2^n = divider -->m = divider/2^n -1;
        clk_m = (divider/_2_pow_clk_n) - 1; 
        //clk_m = (divider >> (_2_pow_clk_n>>1))-1;
        while (clk_m < 16) // 4bits max value is 16
        {
            sclk_real = src_clk/(clk_m + 1)/_2_pow_clk_n; //src_clk/((m+1)*2^n)
            if (sclk_real <= sclk_req)
            {
                goto set_clk;
            }
            else
            {
                clk_m++;
            }
        }
        clk_n++;
        _2_pow_clk_n *= 2; // mutilple by 2
    }

set_clk:    
    _twi_set_clk(clk_n, clk_m, base_addr);

    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_soft_reset
*
*  Description:
*              软复位寄存器
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_soft_reset(void *base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_SRST_REG);
    reg_val |= TWI_SRST_SRST;//set soft reset bit,0x0000 0001
    writel(reg_val, base_addr + TWI_SRST_REG);    
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_soft_reset
*
*  Description:
*              增强特性寄存器,SLAVE ADDRESS+R收到ACK后如果后面需要写data register 的话，要设置这个寄存器
*           属于特殊的I2C读方式。
*  Parameters:
*       base_addr  :  寄存器基地址
*           efr    :   可以取值0x00,0x01,0x02,0x03
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_set_EFR(void *base_addr, unsigned int efr)
{
    unsigned int reg_val = readl(base_addr + TWI_EFR_REG);
    
    reg_val &= ~TWI_EFR_MASK;
    efr     &= TWI_EFR_MASK;
    reg_val |= efr;    
    writel(reg_val, base_addr + TWI_EFR_REG);
    return;	
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_set_sda
*
*  Description:
*                   设置sda的高低电平,在开启使能SDA的情况下调用这个接口才有意义。
*               TWI线控制寄存器,可供上层的ioctrl调用，设置当前的sda状态以便出错处理。
*  Parameters:
*       base_addr  :  寄存器基地址
*           hi_lo  :  1-high level，0-low level
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_set_sda(void *base_addr, unsigned int hi_lo)
{
    unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
    reg_val &= ~TWI_LCR_SDA_CTL;
    hi_lo   &= 0x01;// mask 
    reg_val |= (hi_lo << 1);
    writel(reg_val, base_addr + TWI_LCR_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_get_sda
*
*  Description:
*               得到sda的状态,这个接口可供上层直接调用，不需要设置使能sda控制位。
*           TWI线控制寄存器,可供上层的ioctrl调用，得到当前的sda状态以便出错处理。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/
unsigned int aw_twi_get_sda(void *base_addr)
{
    unsigned int status = 0;
    status = TWI_LCR_SDA_STATE_MASK & readl(base_addr + TWI_LCR_REG);
    status >>= 4;
    return  (status&0x1);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_set_scl
*
*  Description:
*               设置scl的高低电平,在开启使能SCL的情况下调用这个接口才有意义。
*               TWI线控制寄存器,可供上层的ioctrl调用，设置当前的scl状态以便出错处理。
*  Parameters:
*       base_addr  :  寄存器基地址
*           hi_lo  :  1-high level，0-low level
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/ 
void aw_twi_set_scl(void *base_addr, unsigned int hi_lo)
{
    unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
    reg_val &= ~TWI_LCR_SCL_CTL;
    hi_lo   &= 0x01;// mask 
    reg_val |= (hi_lo<<3);
    writel(reg_val, base_addr + TWI_LCR_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_get_scl
*
*  Description:
*               得到scl的状态,这个接口可供上层直接调用，不需要设置使能scl控制位。
*           TWI线控制寄存器,可供上层的ioctrl调用，得到当前的scl状态以便出错处理。
*  Parameters:
*       base_addr  :  寄存器基地址
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/
unsigned int aw_twi_get_scl(void *base_addr)
{
    unsigned int status = 0;
    status = TWI_LCR_SCL_STATE_MASK & readl(base_addr + TWI_LCR_REG);
    status >>= 5;
    return  (status&0x1);    
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_enable_LCR
*
*  Description:
*               使能函数，使能sda，scl线控。
*           TWI线控制寄存器,可供上层的ioctrl调用，以便出错处理。
*           
*  Parameters:
*       base_addr  :  寄存器基地址
*       sda_scl    :  0-使能sda，1-使能scl
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/
void aw_twi_enable_LCR(void *base_addr, unsigned int sda_scl)
{
    unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
    sda_scl &= 0x01;// mask 
    if(sda_scl)
    {
        reg_val |= TWI_LCR_SCL_EN;//enable scl line control
    }
    else
    {
        reg_val |= TWI_LCR_SDA_EN;//enable sda line control
    }
    writel(reg_val, base_addr + TWI_LCR_REG);
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_twi_disable_LCR
*
*  Description:
*               使能函数，关闭sda，scl控制。
*           TWI线控制寄存器,可供上层的ioctrl调用，以便出错处理。
*           
*  Parameters:
*       base_addr  :  寄存器基地址
*       sda_scl    :  0-关闭sda，1-关闭scl
*  Return value:
*          无。
*  Notes:
*   
****************************************************************************************************
*/
void aw_twi_disable_LCR(void *base_addr, unsigned int sda_scl)
{
    unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
    sda_scl &= 0x01;// mask 
    if(sda_scl)
    {
        reg_val &= ~TWI_LCR_SCL_EN;//disable scl line control
    }
    else
    {
        reg_val &= ~TWI_LCR_SDA_EN;//disable sda line control
    }
    writel(reg_val, base_addr + TWI_LCR_REG);
    return;
}
