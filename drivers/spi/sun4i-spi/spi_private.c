/*
*********************************************************************************************************
*                                                      eBase
*                                           the Abstract of Hardware
*
*                                   (c) Copyright 2006-2010, holigun China
*                                                   All Rights Reserved
*
* File        :  spi_private.h
* Date        :  2010-11-27
* Author      :  Victor
* Version     :  v1.00
* Description:  
*                                   Operation for SPI module,aw1620
* History     :
*      <author>          <time>             <version >          <desc>
*       Victor         2010-11-27              1.0           create this file     
*********************************************************************************************************
*/
#include "spi_private.h"
#include <asm/io.h>


/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_cs
*
*  Description:
*              配置片选信号,实际上芯片不同，片选数量不同，最大为4个。
*
*  Parameters:
*       chipselect :   片选选项:0,1,2,3
*       base_addr:   SPI寄存器基地址
*
*  Return value:
*       初始化成功返回AW_SPI_OK，失败返回AW_SPI_FAIL
*  Notes:
*   
****************************************************************************************************
*/
s32 aw_spi_set_cs(u32 chipselect, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
   
    if (chipselect < 4)
    {
        reg_val &= ~SPI_CTL_SS_MASK;//SS-chip select, clear two bits
        reg_val |= chipselect << SPI_SS_BIT_POS;//set chip select 

        writel(reg_val, base_addr + SPI_CTL_REG);
        
//        spi_msg("Chip Select set succeed! cs = %d\n", chipselect);
        return AW_SPI_OK;
    }
    else 
    {   
        spi_wrn("Chip Select set fail! cs = %d\n", chipselect);
        return AW_SPI_FAIL;
    }

}

s32 aw_spi_sel_dma_type(u32 dma_type, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    
//    spi_msg("set dma type %s\n", dma_mode ? "ddma" : "ndma");
    reg_val &= ~SPI_CTL_DMAMOD;
    if (dma_type)
    {
        reg_val |= 1 << 5;
    }
    writel(reg_val, base_addr + SPI_CTL_REG);
        
    return AW_SPI_OK;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_config
*
*  Description:
*             可配置,模式-POL,PHA,片选极性-SSPOL,传输bit开始-LMTF,自动填充0/1-DDB,放弃无效burst-DHB
*             暂时默认配置的有:SSCTL=0和SMC=1,TBW=0,是否在burst之间保持片选SSCTL为0， SMC默认为1,即smart模式；
*             TBW=0,表示8bit。
*  Parameters:
*           master:    slave配置还是master配置，1-master；0-slave
*           config:      配置位图
*           base_addr：    spi寄存器基地址
*  Return value:
*           无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_config(u32 master, u32 config, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);

    /*1. POL */
    if (config & SPI_POL_ACTIVE_)
    {
        reg_val |= SPI_CTL_POL;
    }
    else
    {
        reg_val &= ~SPI_CTL_POL;/*default POL = 0 */
    }
    /*2. PHA */
    if (config & SPI_PHA_ACTIVE_)
    {
        reg_val |= SPI_CTL_PHA;
    }
    else
    {
        reg_val &= ~SPI_CTL_PHA;/*default PHA = 0 */
    }   
    /*3. SSPOL,chip select signal polarity */
    if (config & SPI_CS_HIGH_ACTIVE_)
    {
        reg_val &= ~SPI_CTL_SSPOL;
    }
    else
    {
        reg_val |= SPI_CTL_SSPOL;/*default SSPOL = 1,Low level effective */
    }    
    /*4. LMTF--LSB/MSB transfer first select */
    if (config & SPI_LSB_FIRST_ACTIVE_)
    {
        reg_val |= SPI_CTL_LMTF;
    }
    else
    {
        reg_val &= ~SPI_CTL_LMTF;/*default LMTF =0, MSB first */
    }        
    
    /*master mode: set DDB,DHB,SMC,SSCTL*/
    if(master == 1)
    {   
        /*5. dummy burst type */
        if (config & SPI_DUMMY_ONE_ACTIVE_)
        {
            reg_val |= SPI_CTL_DDB;
        }
        else
        {
            reg_val &= ~SPI_CTL_DDB;/*default DDB =0, ZERO */
        } 
        /*6.discard hash burst-DHB */
        if (config & SPI_RECEIVE_ALL_ACTIVE_)
        {
            reg_val &= ~SPI_CTL_DHB;
        }
        else
        {
            reg_val |= SPI_CTL_DHB;/*default DHB =1, discard unused burst */
        } 
        
        /*7. set SMC = 1 , SSCTL = 0 ,TPE = 1 */
        reg_val &= ~SPI_CTL_SSCTL;     
        reg_val |=  SPI_CTL_T_PAUSE_EN;
    }    
    else
    {
        /* tips for slave mode config */
        spi_msg("slave mode configurate control register.\n");
    }

    writel(reg_val, base_addr + SPI_CTL_REG);

    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_restore_state
*
*  Description:
*              传输完成后，寄存器恢复默认配置
*               master : DHB=1,SMC=1,reset rxFIFO&txFIFO,SSPOL=1,POL=1,PHA=1,master,TPE=1,module enable；
*                        DDB=0,SS=0,XCH=0,SSCTL=0,LMTF=0,TBW=0；           
*               slave  : reset rxFIFO&txFIFO，SSPOL=1,POL=1,PHA=1,module enable.
*
*  Parameters:
*       master :   0-slave，1-master
*       base_addr:   SPI寄存器基地址
*
*  Return value:
*       none
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_restore_state(u32 master, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);

//config spi control register
//| 15 |  14  |  13  |  12  |  11  |  10  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
//| DHB|  DDB |     SS      | SMC  |  XCH |TFRST|RFRST|SSCTL| MSB | TBW |SSPOL| POL | PHA | MOD | EN  |
//| 1  |   0  |     00      |  1   |   0  |  0  |  0  |  0  |  0  |  0  |  1  |  1  |  1  |  1  |  1  | 
    //master mode
    if(master)
    {
        reg_val |= (SPI_CTL_DHB|SPI_CTL_SSPOL|SPI_CTL_POL|SPI_CTL_PHA
                        |SPI_CTL_FUNC_MODE|SPI_CTL_EN); 
        reg_val |= SPI_CTL_T_PAUSE_EN;/* new bit,transmit pause enable,stop smart dummy when rxfifo full*/                                    
                                    
        reg_val &= ~(SPI_CTL_DDB|SPI_CTL_SS_MASK|SPI_CTL_XCH|SPI_CTL_SSCTL|SPI_CTL_LMTF);//|SPI_CTL_TBW); //deleted SPI_CTL_TBW bit for aw1623, this bit is defined for dma mode select, 2011-5-26 19:55:32
    }
    else/* slave mode */
    {
         reg_val |= (SPI_CTL_SSPOL|SPI_CTL_POL|SPI_CTL_PHA||SPI_CTL_EN); 
                                    
        reg_val &= ~(SPI_CTL_DHB|SPI_CTL_FUNC_MODE|SPI_CTL_DDB|SPI_CTL_SS_MASK
                            |SPI_CTL_XCH|SPI_CTL_SSCTL|SPI_CTL_LMTF);//|SPI_CTL_TBW);            //deleted SPI_CTL_TBW bit for aw1623, this bit is defined for dma mode select, 2011-5-26 19:55:32               
        
    }

    spi_msg("control register set default value: %x \n", reg_val);
    writel(reg_val, base_addr + SPI_CTL_REG);
    
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_clk
*
*  Description:
*                   当(ahb_clk/2)/spi_clk 〈= 256 时采用CDR2，〉256时采用CDR1。
*               
*  Parameters:
*           spi_clk:  需要设置的spi工作时钟频率
*           ahb_clk： ahb总线的频率
*           base_addr: 寄存器基地址
*  Return value:
*           无
*  Notes:
*           仅仅用在master模式，slave模式不可用。
****************************************************************************************************
*/
void aw_spi_set_clk(u32 spi_clk, u32 ahb_clk, void *base_addr)
{
    u32 reg_val = 0;
    u32 N = 0;
    u32 div_clk = (ahb_clk>>1)/spi_clk;
    
//    spi_msg("set spi clock %d, mclk %d\n", spi_clk, ahb_clk);
    reg_val = readl(base_addr + SPI_CLK_RATE_REG);

    /* CDR2 */
    if(div_clk <= SPI_CLK_SCOPE)
    {
        if (div_clk != 0)
        {
            div_clk--;
        }
        reg_val &= ~SPI_CLKCTL_CDR2;
        reg_val |= (div_clk|SPI_CLKCTL_DRS);
        
//        spi_msg("CDR2 - n = %d \n", div_clk);
        
    }
    else /* CDR1 */
    {
        //search 2^N
        while(1)
        {                  
            if(div_clk == 1)
            {
                break;
            }
            div_clk >>= 1;//divide by 2
            N++;            
        };
        reg_val &= ~(SPI_CLKCTL_CDR1|SPI_CLKCTL_DRS);
        reg_val |= (N<<8);  
        
//        spi_msg("CDR1 - n = %d \n", N);
    }    
   
    writel(reg_val, base_addr + SPI_CLK_RATE_REG);
  
    return;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_start_xfer
*
*  Description:
*                   在SMC=1时，对XCH位置1，启动传输，传输完成这个bit会自动清零，
*                所以轮询这个bit也可以知道传输是否结束。
*
*  Parameters:
*           base_addr:   寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_start_xfer(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    reg_val |= SPI_CTL_XCH;
    writel(reg_val, base_addr + SPI_CTL_REG);    
}


/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_query_xfer
*
*  Description:
*                   在smc模式下，查询是否完成传输,返回0表示传输完成，返回非零表示正在传输中
*
*  Parameters:
*           base_addr:   寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
u32 aw_spi_query_xfer(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    
    return (reg_val & SPI_CTL_XCH);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_enable_bus
*
*  Description:
*                 使能spi bus
*
*  Parameters:
*           base_addr:   寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_enable_bus(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    reg_val |= SPI_CTL_EN;
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_disable_bus
*
*  Description:
*                关闭spi bus
*
*  Parameters:
*           base_addr:   寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_disable_bus(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    reg_val &= ~SPI_CTL_EN;
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_master
*
*  Description:
*                设置master模式
*
*  Parameters:
*           base_addr:   寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_set_master(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    reg_val |= SPI_CTL_FUNC_MODE;
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_slave
*
*  Description:
*                设置slave模式
*
*  Parameters:
*           base_addr:   寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_set_slave(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    reg_val &= ~SPI_CTL_FUNC_MODE;
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_tbw
*
*  Description:
*               设置spi的宽度0表示8bits，20的只有8bit，没有16bit或者32bit。
*
*  Parameters:
*           bits_per_word: 0-8bit；1-其他，20芯片不支持8bit以外的宽度。
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_set_tbw(u32 bits_per_word, void *base_addr)
{   
/* 
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    u32 flag = (0x1 & bits_per_word);
    
    if(flag)
    {
       spi_wrn("aw1623 just support 8 bits per word!\n");
    }

    reg_val &= ~SPI_CTL_TBW;// 8BITS              //deleted for aw1623, this bit is defined for dma mode select, 2011-5-26 19:55:32
    writel(reg_val, base_addr + SPI_CTL_REG);
*/
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_disable_irq
*
*  Description:
*               关闭中断类型
*
*  Parameters:
*           bitmap: 中断位图
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_disable_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_INT_CTL_REG);
    bitmap &= SPI_INTEN_MASK;
    reg_val &= ~bitmap;
    writel(reg_val, base_addr + SPI_INT_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_enable_irq
*
*  Description:
*              使能中断类型
*
*  Parameters:
*           bitmap: 中断位图
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_enable_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_INT_CTL_REG);
    bitmap &= SPI_INTEN_MASK;
    reg_val |= bitmap;
    writel(reg_val, (base_addr + SPI_INT_CTL_REG));
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_disable_dma_irq
*
*  Description:
*              关闭dma中断类型
*
*  Parameters:
*           bitmap: 中断位图
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_disable_dma_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_DMA_CTL_REG);
    bitmap &= SPI_DRQEN_MASK;
    reg_val &= ~bitmap;
    writel(reg_val, base_addr + SPI_DMA_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_enable_dma_irq
*
*  Description:
*              使能dma中断类型
*
*  Parameters:
*           bitmap: 中断位图
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_enable_dma_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_DMA_CTL_REG);
    bitmap &= SPI_DRQEN_MASK;
    reg_val |= bitmap;
    writel(reg_val, base_addr + SPI_DMA_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_qry_irq_pending
*
*  Description:
*              查询中断pending
*
*  Parameters:
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
u32 aw_spi_qry_irq_pending(void *base_addr)
{
    return ( SPI_STAT_MASK & readl(base_addr + SPI_STATUS_REG) );
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_clr_irq_pending
*
*  Description:      
*              清除对应中断pending
*
*  Parameters:
*           pending_bit:   中断pending
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_clr_irq_pending(u32 pending_bit, void *base_addr)
{
    pending_bit &= SPI_STAT_MASK;
    writel(pending_bit, base_addr + SPI_STATUS_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_query_txfifo
*
*  Description:      
*             查询发送fifo的字节数
*
*  Parameters:
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
u32 aw_spi_query_txfifo(void *base_addr)
{
    u32 reg_val = ( SPI_FIFO_TXCNT & readl(base_addr + SPI_FIFO_STA_REG) );
    
    reg_val >>= SPI_TXCNT_BIT_POS;
    
    return reg_val;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_query_rxfifo
*
*  Description:      
*            查询接收fifo的字节数
*
*  Parameters:
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
u32 aw_spi_query_rxfifo(void *base_addr)
{
    u32 reg_val = ( SPI_FIFO_RXCNT & readl(base_addr + SPI_FIFO_STA_REG) );
    
    reg_val >>= SPI_RXCNT_BIT_POS;
    
    return reg_val;
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_reset_fifo
*
*  Description:      
*            复位txFIFO和rxFIFO
*
*  Parameters:
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_reset_fifo(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);
    
    reg_val |= (SPI_CTL_RST_RXFIFO|SPI_CTL_RST_TXFIFO);
    
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_bc_wtc
*
*  Description:      
*           设置总的传输长度BC，设置发送的数据长度WTC,相减为接受数据的长度
*           总的传输长度大小限制为0xffffff，总的发送长度大小限制为0xffffff，
*           
*  Parameters:
*           tx_len :  发送长度
*           rx_len :  接收长度
*           base_addr:     寄存器基地址
*       
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_set_bc_wtc(u32 tx_len, u32 rx_len, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_BC_REG);
    reg_val &= ~SPI_BC_BC_MASK;
    reg_val |= ( SPI_BC_BC_MASK & (tx_len+rx_len) );
    writel(reg_val, base_addr + SPI_BC_REG);    
    
//    spi_msg("\n-- BC = %d --\n", readl(base_addr + SPI_BC_REG));
    
    reg_val = readl(base_addr + SPI_TC_REG);
    reg_val &= ~SPI_TC_WTC_MASK;
    reg_val |= (SPI_TC_WTC_MASK & tx_len);
    writel(reg_val, base_addr + SPI_TC_REG);
//    spi_msg("\n-- TC = %d --\n", readl(base_addr + SPI_TC_REG));
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_ss_ctrl
*
*  Description:      
*                  片选控制，是否由spi控制器控制片选，或手动控制。
*                   linux下建议手动控制，melis下建议自动控制，因为linux下，每个传输分为tx，rx，
*                   而有些芯片要求在一个命令过程中不允许片选变化，这个过程借助linux的cs_actived实现。
*
*  Parameters:
*           base_addr:   寄存器基地址
*           on_off   :   0-automatic control chipselect level, 1: manual
*
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_ss_ctrl(void *base_addr, u32 on_off)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);

    on_off &= 0x1;
    if(on_off)
    {
        reg_val |= SPI_CTL_SS_CTRL;
    }
    else
    {
        reg_val &= ~SPI_CTL_SS_CTRL;
    }
    
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_ss_level
*
*  Description:      
*            ss_ctrl开启后，这个接口才有效。
*
*  Parameters:
*           base_addr:     寄存器基地址
*           hi_lo    :  0:low level；   1:high level
*  Return value:
*          无
*  Notes:
*   
****************************************************************************************************
*/
void aw_spi_ss_level(void *base_addr, u32 hi_lo)
{
    u32 reg_val = readl(base_addr + SPI_CTL_REG);

    hi_lo &= 0x1;
    if(hi_lo)
    {
        reg_val |= SPI_CTL_SS_LEVEL;
    }
    else
    {
        reg_val &= ~SPI_CTL_SS_LEVEL;
    }
    
    writel(reg_val, base_addr + SPI_CTL_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_waitclk_cnt
*
*  Description:      
*                   wait clock counter 
*               在每个data transfer 中，插入N个等待状态，spi模块计数SPI_SCLK作为N个WCC，
*               对下一个data transfer作延迟。
*
*  Parameters:
*           base_addr    :     寄存器基地址
*           waitclk_cnt  :  0:low level；   1:high level
*  Return value:
*          无
*  Notes:
*           仅仅在maser mode 有效。
****************************************************************************************************
*/
void aw_spi_set_waitclk_cnt(u32 waitclk_cnt, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_WAIT_REG);
    reg_val &= ~SPI_WAIT_CLK_MASK;
    waitclk_cnt &= SPI_WAIT_CLK_MASK;
    reg_val |= waitclk_cnt;
    writel(reg_val, base_addr + SPI_WAIT_REG);
}

/*
****************************************************************************************************
*
*  FunctionName:           aw_spi_set_sample_delay
*
*  Description:      
*                   高速传输下使用。采样延时。
*
*  Parameters:
*           base_addr :  寄存器基地址
*           on_off    :  1: 开启高速传输模式，0: 低速传输模式，一般10M左右。  
*  Return value:
*          无
*  Notes:
*           
****************************************************************************************************
*/
void aw_spi_set_sample_delay(u32 on_off, void *base_addr)
{
    u32 reg_val = readl(base_addr+SPI_CTL_REG);
    reg_val &= ~SPI_CTL_MASTER_SDC;
    reg_val |= on_off;
    writel(reg_val, base_addr + SPI_CTL_REG);
}

