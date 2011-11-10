 /*
*********************************************************************************************************
*                                                    eBase
*                                           the Abstract of Hardware
*
*                                   (c) Copyright 2006-2010, holigun China
*                                                All Rights Reserved
*
* File        :  bsp_sdc.c
* Date        :  2012-11-25 14:24:12
* Author      :  Aaron.Maoye
* Version     :  v1.00
* Description:  
*                                   Operation for SD Controller module, aw1620
* History     :
*      <author>         <time>                  <version >      <desc>
*       Maoye           2012-11-25 14:25:25     1.0             create this file    
*
*********************************************************************************************************
*/
#include "host_op.h"
#include "sdxc.h"
#include "smc_syscall.h"

extern unsigned int smc_debug;

/******************************************************************************************************
 *                                       SD3.0 controller operation                                   *
 ******************************************************************************************************/

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_fifo_reset(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)|SDXC_FIFOReset);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_dma_reset(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)|SDXC_DMAReset);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_int_enable(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)|SDXC_INTEnb);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_int_disable(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)&(~SDXC_INTEnb));
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_dma_enable(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)|SDXC_DMAEnb);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_dma_disable(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)|SDXC_DMAReset);
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)&(~SDXC_DMAEnb));
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_idma_reset(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_DMAC, SDXC_IDMACSoftRST);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_idma_on(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_DMAC, SDXC_IDMACFixBurst | SDXC_IDMACIDMAOn);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_idma_off(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_DMAC, 0);
}
/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_idma_int_enable(struct awsmc_host* smc_host, u32 int_mask)
{
    sdc_write(SDXC_REG_IDIE, sdc_read(SDXC_REG_IDIE)|int_mask);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_idma_int_disable(struct awsmc_host* smc_host, u32 int_mask)
{
    sdc_write(SDXC_REG_IDIE, sdc_read(SDXC_REG_IDIE) & (~int_mask));
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_cd_debounce_on(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)|SDXC_DebounceEnb);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_cd_debounce_off(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL)&(~SDXC_DebounceEnb));
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_sel_access_mode(struct awsmc_host* smc_host, u32 access_mode)
{
    sdc_write(SDXC_REG_GCTRL, (sdc_read(SDXC_REG_GCTRL)&(~SDXC_ACCESS_BY_AHB)) | access_mode);
}
/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline u32 sdxc_enable_imask(struct awsmc_host* smc_host, u32 imask)
{
	u32 newmask = sdc_read(SDXC_REG_IMASK) | imask;
    
	sdc_write(SDXC_REG_IMASK, newmask);
	return newmask;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline u32 sdxc_disable_imask(struct awsmc_host* smc_host, u32 imask)
{
    u32 newmask = sdc_read(SDXC_REG_IMASK) & (~imask);
    
    sdc_write(SDXC_REG_IMASK, newmask);
	return newmask;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static __inline void sdxc_clear_imask(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_IMASK, sdc_read(SDXC_REG_IMASK)&(SDXC_SDIOInt|SDXC_CardInsert|SDXC_CardRemove));
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_enable_sdio_irq(struct awsmc_host* smc_host, u32 enable)
{
    if (enable)
        sdxc_enable_imask(smc_host, SDXC_SDIOInt);
    else
        sdxc_disable_imask(smc_host, SDXC_SDIOInt);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_sel_ddr_mode(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL) | SDXC_DDR_MODE);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_sel_sdr_mode(struct awsmc_host* smc_host)
{
    sdc_write(SDXC_REG_GCTRL, sdc_read(SDXC_REG_GCTRL) & (~SDXC_DDR_MODE));
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_set_buswidth(struct awsmc_host* smc_host, u32 width)
{
    switch(width)
    {
        case 1:
            sdc_write(SDXC_REG_WIDTH, SDXC_WIDTH1);
            break;
        case 4:
            sdc_write(SDXC_REG_WIDTH, SDXC_WIDTH4);
            break;
        case 8:
            sdc_write(SDXC_REG_WIDTH, SDXC_WIDTH8);
            break;
    }
}


/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
s32 sdxc_reset(struct awsmc_host* smc_host)
{
    u32 rval = sdc_read(SDXC_REG_GCTRL) | SDXC_SoftReset | SDXC_FIFOReset | SDXC_DMAReset;
    s32 time = 0xffff;
    
    sdc_write(SDXC_REG_GCTRL, rval);
    while((sdc_read(SDXC_REG_GCTRL) & 0x7) && time--);
    if (time <= 0)
    {
        awsmc_dbg_err("sdc %d reset failed\n", smc_host->pdev->id);
        return -1;
    }
    return 0;
}
/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
s32 sdxc_program_clk(struct awsmc_host* smc_host)
{
  	u32 rval;
  	s32 time = 0xf000;
  	s32 ret = 0;
  	
  	rval = SDXC_Start|SDXC_UPCLKOnly|SDXC_WaitPreOver;
  	sdc_write(SDXC_REG_CMDR, rval);
	
	//disable command done interrupt
	sdxc_disable_imask(smc_host, SDXC_CmdDone);
	
	do {
	    rval = sdc_read(SDXC_REG_CMDR);
	    time--;
	} while(time && (rval & SDXC_Start));
	
	if (time <= 0)
	{
		ret = -1;
	}
	
	//clear command cone flag
	rval = sdc_read(SDXC_REG_RINTR);
	sdc_write(SDXC_REG_RINTR, rval);
	
	//enable command done interrupt
	sdxc_enable_imask(smc_host, SDXC_CmdDone);
	
	return ret;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
s32 sdxc_update_clk(struct awsmc_host* smc_host, u32 sclk, u32 cclk)
{
    u32 rval;
    u32 clk_div;
    u32 real_clk;
    
    //caculate new clock divider
    clk_div = (sclk / cclk)>>1;
    real_clk = clk_div ? sclk/(clk_div<<1) : sclk;
    while (real_clk > cclk)
    {
        clk_div++;
        real_clk = sclk/(clk_div<<1);
    }
    
    awsmc_dbg("sdc %d change clock over, src_clk %d, req_clk %d, real_clk %d, div %d\n", smc_host->pdev->id, sclk, cclk, real_clk, clk_div);
    
    //update new clock
    //disable clock 
    rval = sdc_read(SDXC_REG_CLKCR) & (~SDXC_CardClkOn) & (~SDXC_LowPowerOn);
    sdc_write(SDXC_REG_CLKCR, rval);
    if (-1 == sdxc_program_clk(smc_host))
    {
        awsmc_dbg_err("clock program failed in step 1\n");
        return -1;
    }
    
    //update divider
    rval = sdc_read(SDXC_REG_CLKCR);
    rval &= ~0xff;
    rval |= clk_div & 0xff;
    sdc_write(SDXC_REG_CLKCR, rval);
    if (-1 == sdxc_program_clk(smc_host))
    {
        awsmc_dbg_err("clock program failed in step 2\n");
        return -1;
    }
    
    //re-enable clock
    rval = sdc_read(SDXC_REG_CLKCR) | SDXC_CardClkOn ;//| SDXC_LowPowerOn;
    sdc_write(SDXC_REG_CLKCR, rval);
    if (-1 == sdxc_program_clk(smc_host))
    {
        awsmc_dbg_err("clock program failed in step 3\n");
        return -1;
    }
    
    smc_host->real_cclk = real_clk;
    return real_clk;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static void sdxc_send_cmd(struct awsmc_host* smc_host, struct mmc_command* cmd)
{
    u32 imask;
    u32 cmd_val = SDXC_Start|(cmd->opcode&0x3f);
    
    imask = SDXC_CmdDone|SDXC_IntErrBit|SDXC_WaitPreOver;
    
    if (cmd->opcode == MMC_GO_IDLE_STATE) 
    {
        cmd_val |= SDXC_SendInitSeq;
        smc_host->wait = SDC_WAIT_CMD_DONE;
    } 
    else 
    {
        if ((cmd->flags & MMC_CMD_MASK) != MMC_CMD_BC) //with response
        {
            cmd_val |= SDXC_RspExp;
            
            if (cmd->flags & MMC_RSP_136)                                   //long response
                cmd_val |= SDXC_LongRsp;
                
            if (cmd->flags & MMC_RSP_CRC)                                   //check response CRC
            {
                cmd_val |= SDXC_CheckRspCRC;
            }
                
            smc_host->wait = SDC_WAIT_CMD_DONE;
            
            if ((cmd->flags & MMC_CMD_MASK) == MMC_CMD_ADTC)                //with data transfer
            {
                cmd_val |= SDXC_DataExp | SDXC_WaitPreOver;
                smc_host->wait = SDC_WAIT_DATA_OVER;
                imask |= SDXC_DataOver;
                
                if (cmd->data->flags & MMC_DATA_STREAM)        //sequence mode
                {    
                    imask |= SDXC_AutoCMDDone;
                    cmd_val |= SDXC_Seqmod | SDXC_SendAutoStop;
                    smc_host->wait = SDC_WAIT_AUTOCMD_DONE;
                }
                
                if (smc_host->with_autostop)
                {
                    imask |= SDXC_AutoCMDDone;
                    cmd_val |= SDXC_SendAutoStop;
                    smc_host->wait = SDC_WAIT_AUTOCMD_DONE;
                }
                
                if (cmd->data->flags & MMC_DATA_WRITE)           //read
                {
                    cmd_val |= SDXC_Write;
                }
                else
                {
                    if (!smc_host->dodma)
                        imask &= ~(SDXC_AutoCMDDone | SDXC_DataOver);
                }
            }
        }
    }
    
    sdxc_enable_imask(smc_host, imask);
	
	awsmc_info("smc %d send cmd %d(%08x), imask = 0x%08x, wait = %d\n", smc_host->pdev->id, cmd_val&0x3f, cmd_val, imask, smc_host->wait);
    
    sdc_write(SDXC_REG_CARG, cmd->arg);
    sdc_write(SDXC_REG_CMDR, cmd_val);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
/*
static void sdxc_trans_by_ahb(struct awsmc_host* smc_host, struct mmc_data* data)
{
    u32 i, j;
    u32* buf_temp;
    u32 dword_cnt;
    u32 remain;
    
    awsmc_dbg("sg_len %d !!\n", data->sg_len);
    
    for (i=0; i<data->sg_len; i++)
    {
        buf_temp = sg_virt(&data->sg[i]);
        dword_cnt = data->sg[i].length>>2;
        remain = data->sg[i].length - (dword_cnt<<2);
        
        awsmc_dbg("dword %d, remain %d\n", dword_cnt, remain);
        
        if (data->flags & MMC_DATA_WRITE)
        {
        	awsmc_dbg("-> ahb wr sg[%d] = %d !!\n", i, data->sg[i].length);
            for(j=0; j<dword_cnt; j++)
            {
                while(sdc_read(SDXC_REG_STAS)&SDXC_FIFOFull);
                sdc_write(SDXC_REG_FIFO, buf_temp[j]);
            }
            
            if (remain)
            {
                u32 temp = buf_temp[j];
                while(sdc_read(SDXC_REG_STAS)&SDXC_FIFOFull);
                sdc_write(SDXC_REG_FIFO, temp);
            }
        }
        else
        {
        	awsmc_dbg("-> ahb red sg[%d] = %d !!\n", i, data->sg[i].length);
            for (j=0; j<dword_cnt; j++)
            {
                while(sdc_read(SDXC_REG_STAS) & SDXC_FIFOEmpty);
                buf_temp[j] = sdc_read(SDXC_REG_FIFO);
            }
            if (remain)
            {
                u8* p = (u8*)&buf_temp[j];
                u32 temp = 0;
                
                while(sdc_read(SDXC_REG_STAS) & SDXC_FIFOEmpty);
                temp = sdc_read(SDXC_REG_FIFO);
                while (remain)
                {
                    *p++ = (u8)(0xff & (temp >> (8*(3-remain))));
                    remain--;
                }
            }
        }
    }
    
}
*/

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static void  sdxc_init_idma_des(struct awsmc_host* smc_host, struct mmc_data* data)
{
    struct smc_idma_des* pdes = smc_host->pdes;
    u32 des_idx = 0;
    u32 buff_frag_num = 0;
    u32 remain;
    u32 i, j;
    
    /* ≥ı ºªØIDMA Descriptor */
    #if SDXC_DES_MODE == 0      //chain mode
    for (i=0; i<data->sg_len; i++)
    {
        buff_frag_num = data->sg[i].length >> SDXC_DES_NUM_SHIFT;   //SDXC_DES_NUM_SHIFT == 13, num = len/8192 = len>>13
        remain = data->sg[i].length & (SDXC_DES_BUFFER_MAX_LEN-1);
        if (remain)
        {
            buff_frag_num ++;
        }
        else
        {
            remain = SDXC_DES_BUFFER_MAX_LEN;
        }
        
        eLIBs_CleanFlushDCacheRegion(sg_virt(&data->sg[i]), data->sg[i].length);
        for (j=0; j < buff_frag_num; j++, des_idx++)
        {
			memset((void*)&pdes[des_idx], 0, sizeof(struct smc_idma_des));
            pdes[des_idx].des_chain = 1;
            pdes[des_idx].own = 1;
            pdes[des_idx].dic = 1;
            if (buff_frag_num > 1 && j != buff_frag_num-1)
            {
                pdes[des_idx].data_buf1_sz = 0x1fff & SDXC_DES_BUFFER_MAX_LEN;
            }
            else
            {
                pdes[des_idx].data_buf1_sz = remain;
            }
            
            pdes[des_idx].buf_addr_ptr1 = sg_dma_address(&data->sg[i]) + j * SDXC_DES_BUFFER_MAX_LEN;
            if (i==0 && j==0)
            {
                pdes[des_idx].first_des = 1;
            }
            
            if ((i == data->sg_len-1) && (j == buff_frag_num-1))
            {
                pdes[des_idx].dic = 0;
                pdes[des_idx].last_des = 1;
                pdes[des_idx].end_of_ring = 1;
                pdes[des_idx].buf_addr_ptr2 = 0;
            }
            else
            {
                pdes[des_idx].buf_addr_ptr2 = __pa(&pdes[des_idx+1]);
            }
			
//            awsmc_info("sg %d, frag %d, remain %d, des[%d](%08x): [0] = %08x, [1] = %08x, [2] = %08x, [3] = %08x\n", i, j, remain, 
//                                                                             des_idx, (u32)&pdes[des_idx], 
//                                                                             (u32)((u32*)&pdes[des_idx])[0], (u32)((u32*)&pdes[des_idx])[1], 
//                                                                             (u32)((u32*)&pdes[des_idx])[2], (u32)((u32*)&pdes[des_idx])[3]);
																			 
        }
    }
    #else      //fix length skip mode
    
    #endif
    
    eLIBs_CleanFlushDCacheRegion(pdes, sizeof(struct smc_idma_des) * (des_idx+1));
    
    return;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
static int sdxc_prepare_dma(struct awsmc_host* smc_host, struct mmc_data* data)
{
    u32 dma_len;
    u32 i;
   
	if (smc_host->pdes == NULL)
	{
		return -ENOMEM;
	}

	dma_len = dma_map_sg(mmc_dev(smc_host->mmc), data->sg, data->sg_len, (data->flags & MMC_DATA_WRITE) ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	if (dma_len == 0)
	{
		awsmc_dbg_err("no dma map memory\n");
		return -ENOMEM;
	}	
		
    for (i=0; i<data->sg_len; i++)
    {
        if (sg_dma_address(&data->sg[i]) & 3)
        {
		    awsmc_dbg_err("unaligned dma address[%d] %p\n", i, (void*)sg_dma_address(&data->sg[i]));
			return -EINVAL;
        }
    }

    sdxc_init_idma_des(smc_host, data);
	sdxc_dma_enable(smc_host);
    sdxc_dma_reset(smc_host);
    sdxc_idma_reset(smc_host);
    sdxc_idma_on(smc_host);
    sdxc_idma_int_disable(smc_host, SDXC_IDMACTransmitInt|SDXC_IDMACReceiveInt);
    if (data->flags & MMC_DATA_WRITE)
    {
        sdxc_idma_int_enable(smc_host, SDXC_IDMACTransmitInt);
    }
    else
    {
        sdxc_idma_int_enable(smc_host, SDXC_IDMACReceiveInt);
    }
    
    //write descriptor address to register
    sdc_write(SDXC_REG_DLBA, __pa(smc_host->pdes));

    //write water level
    sdc_write(SDXC_REG_FTRGL, (2U<<28)|(7<<16)|8);
    
    return 0;
}


int sdxc_check_r1_ready(struct awsmc_host* smc_host)
{
    return sdc_read(SDXC_REG_STAS) & SDXC_CardDataBusy ? 0 : 1;
}

int sdxc_send_manual_stop(struct awsmc_host* smc_host, struct mmc_request* request)
{
	struct mmc_data* data = request->data;
	u32 cmd_val = SDXC_Start | SDXC_RspExp | SDXC_CheckRspCRC | MMC_STOP_TRANSMISSION;
	u32 iflags = 0;
	int ret = 0;
	
	if (!data || !data->stop)
	{
		awsmc_dbg_err("no stop cmd request\n");
		return -1;
	}

	sdxc_int_disable(smc_host);
	
	sdc_write(SDXC_REG_CARG, 0);
	sdc_write(SDXC_REG_CMDR, cmd_val);
	do {
		iflags = sdc_read(SDXC_REG_RINTR);
	} while(!(iflags & (SDXC_CmdDone | SDXC_IntErrBit)));
	
	if (iflags & SDXC_IntErrBit)
	{
		awsmc_dbg_err("sdc %d send stop command failed\n", smc_host->pdev->id);
		data->stop->error = ETIMEDOUT;
		ret = -1;
	}

	sdc_write(SDXC_REG_RINTR, iflags);
    data->stop->resp[0] = sdc_read(SDXC_REG_RESP0);
	
	sdxc_int_enable(smc_host);

	return ret;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_request(struct awsmc_host* smc_host, struct mmc_request* request)
{
    struct mmc_command* cmd = request->cmd;
    struct mmc_data* data = request->data;
    struct scatterlist* sg = NULL;
    u32 byte_cnt = 0;
    int ret;
    
    smc_host->mrq = request;
    smc_host->int_sum = 0;
    awsmc_dbg("smc %d, cmd %d, arg %08x\n", smc_host->pdev->id, cmd->opcode, cmd->arg);
    if (data)
    {
        sg = data->sg;
        byte_cnt = data->blksz * data->blocks;
        
        sdc_write(SDXC_REG_BLKSZ, data->blksz);
        sdc_write(SDXC_REG_BCNTR, byte_cnt);
        
        awsmc_dbg("-> with data %d bytes, sg_len %d\n", byte_cnt, data->sg_len);
        //if (byte_cnt > 0)
        {
            sdxc_sel_access_mode(smc_host, SDXC_ACCESS_BY_DMA);
            smc_host->todma = 0;
            ret = sdxc_prepare_dma(smc_host, data);
            if (ret < 0)
            {
                awsmc_dbg_err("smc %d prepare DMA failed\n", smc_host->pdev->id);
		        smc_host->dodma = 0;
		        
                awsmc_dbg_err("data prepare error %d\n", ret);
    			cmd->error = ret;
    			cmd->data->error = ret;
    			mmc_request_done(smc_host->mmc, request);
    			return;
            }
            smc_host->dodma = 1;
        }
        
        /*
        if (!smc_host->dodma)
        {
            sdxc_sel_access_mode(smc_host, SDXC_ACCESS_BY_AHB);
            smc_host->dodma = 0;
            uart_send_char('h');
        }
        */
        if (data->stop)
            smc_host->with_autostop = 1;
        else
            smc_host->with_autostop = 0;
    }
    
    /* disable card detect debounce */
    sdxc_cd_debounce_off(smc_host);
    sdxc_send_cmd(smc_host, cmd);
    
    /*
    if (data && !smc_host->dodma)
    {
	    sdxc_disable_imask(smc_host, SDXC_DataOver | SDXC_AutoCMDDone);
        sdxc_trans_by_ahb(smc_host, data);
        sdxc_sel_access_mode(smc_host, SDXC_ACCESS_BY_DMA);
        if (smc_host->with_autostop)
        {
            sdxc_enable_imask(smc_host, SDXC_DataOver | SDXC_AutoCMDDone);
        }
        else
        {
            sdxc_enable_imask(smc_host, SDXC_DataOver);
        }
    }
    */
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_check_status(struct awsmc_host* smc_host)
{
    u32 raw_int;
    u32 msk_int;
	u32 idma_inte;
    u32 idma_int;
    
    sdxc_int_disable(smc_host);
    
    idma_int = sdc_read(SDXC_REG_IDST);
    idma_inte = sdc_read(SDXC_REG_IDIE);
    raw_int = sdc_read(SDXC_REG_RINTR);
    msk_int = sdc_read(SDXC_REG_MISTA);
    
    smc_host->int_sum |= raw_int;
    awsmc_info("smc %d int, ri %08x(%08x) mi %08x ie %08x idi %08x\n", smc_host->pdev->id, raw_int, smc_host->int_sum, msk_int, idma_inte, idma_int);
    
	if (msk_int & SDXC_SDIOInt) 
	{
		smc_host->sdio_int = 1;
    	sdc_write(SDXC_REG_RINTR, SDXC_SDIOInt);
	}
	
	if (smc_host->cd_gpio == CARD_DETECT_BY_DATA3)
    {
        if (msk_int&SDXC_CardInsert) 
        {
    	    awsmc_dbg("card detect insert\n");
    	    smc_host->present = 1;
    	    smc_host->change = 1;
    	    sdc_write(SDXC_REG_RINTR, SDXC_CardInsert);
    		goto irq_out;
    	}
    	if (msk_int&SDXC_CardRemove) 
    	{
    	    awsmc_dbg("card detect remove\n");
    	    smc_host->present = 0;
    	    smc_host->change = 1;
    	    sdc_write(SDXC_REG_RINTR, SDXC_CardRemove);
    		goto irq_out;
    	}
    }
    
    if (smc_host->wait == SDC_WAIT_NONE && !smc_host->sdio_int)
    {
    	awsmc_dbg_err("smc %x, nothing to complete, raw_int = %08x, mask_int = %08x\n", smc_host->pdev->id, raw_int, msk_int);
    	sdxc_clear_imask(smc_host);
		goto irq_normal_out;
    }
    
    if ((raw_int & SDXC_IntErrBit) || (idma_int & SDXC_IDMA_ERR))
    {
        smc_host->error = raw_int & SDXC_IntErrBit;
        smc_host->wait = SDC_WAIT_FINALIZE;
        goto irq_normal_out;
    }
    
	if (smc_host->wait == SDC_WAIT_AUTOCMD_DONE && (msk_int&SDXC_AutoCMDDone))
	{
	    smc_host->wait = SDC_WAIT_FINALIZE;
	}
	else if (smc_host->wait == SDC_WAIT_DATA_OVER && (msk_int&SDXC_DataOver))
	{
	    smc_host->wait = SDC_WAIT_FINALIZE;
	}
	else if (smc_host->wait == SDC_WAIT_CMD_DONE && (msk_int&SDXC_CmdDone) && !(smc_host->int_sum&SDXC_IntErrBit))
	{
	    smc_host->wait = SDC_WAIT_FINALIZE;
	}
    
//    if (idma_int & (SDXC_IDMACTransmitInt | SDXC_IDMACReceiveInt))
//    {
//    }
irq_normal_out:
    sdc_write(SDXC_REG_RINTR, (~SDXC_SDIOInt) & msk_int);
	sdc_write(SDXC_REG_IDST, idma_int);
	
irq_out:
    
    sdxc_int_enable(smc_host);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
s32 sdxc_request_done(struct awsmc_host* smc_host)
{
    struct mmc_request* req = smc_host->mrq;
    u32 temp;
    s32 ret = 0;
    
    if (smc_host->int_sum & SDXC_IntErrBit)
    {
        awsmc_dbg_err("smc %d err, cmd %d, %s%s%s%s%s%s%s%s%s%s !!\n", 
            smc_host->pdev->id, req->cmd->opcode,
    		smc_host->int_sum & SDXC_RespErr     ? " RE"     : "",
    		smc_host->int_sum & SDXC_RespCRCErr  ? " RCE"    : "",
    		smc_host->int_sum & SDXC_DataCRCErr  ? " DCE"    : "",
    		smc_host->int_sum & SDXC_RespTimeout ? " RTO"    : "",
    		smc_host->int_sum & SDXC_DataTimeout ? " DTO"    : "",
    		smc_host->int_sum & SDXC_DataStarve  ? " DS"     : "",
    		smc_host->int_sum & SDXC_FIFORunErr  ? " FE"     : "",
    		smc_host->int_sum & SDXC_HardWLocked ? " HL"     : "",
    		smc_host->int_sum & SDXC_StartBitErr ? " SBE"    : "",
    		smc_host->int_sum & SDXC_EndBitErr   ? " EBE"    : ""
    		);
        
        if (req->data)
        {
            awsmc_dbg_err("In data %s operation\n", req->data->flags & MMC_DATA_WRITE ? "write" : "read");
        }
    	ret = -1;
        goto _out_;
    }
    
    if (req->cmd)
    {
        if (req->cmd->flags & MMC_RSP_136)
    	{
    		req->cmd->resp[0] = sdc_read(SDXC_REG_RESP3);
    		req->cmd->resp[1] = sdc_read(SDXC_REG_RESP2);
    		req->cmd->resp[2] = sdc_read(SDXC_REG_RESP1);
    		req->cmd->resp[3] = sdc_read(SDXC_REG_RESP0);
    	}
    	else
    	{
    		req->cmd->resp[0] = sdc_read(SDXC_REG_RESP0);
    	}
    }
    
_out_:
    if (req->data)
    {
        if (!(req->data->flags & MMC_DATA_WRITE) && (sdc_read(SDXC_REG_STAS) & SDXC_DataFSMBusy))
        {
            printk("data fsm busy\n");
        }
        if (smc_host->dodma)
        {
    		smc_host->dma_done = 0;
            sdc_write(SDXC_REG_IDST, 0x337);
            sdc_write(SDXC_REG_IDIE, 0);
            sdxc_idma_off(smc_host);
            sdxc_dma_disable(smc_host);
        }
        
        sdxc_fifo_reset(smc_host);
    }
    
    temp = sdc_read(SDXC_REG_STAS);
    if ((temp & SDXC_DataFSMBusy) || (smc_host->int_sum & (SDXC_RespErr | SDXC_HardWLocked | SDXC_RespTimeout)))
    {
        awsmc_dbg("sdc %d abnormal status: %s %s\n", smc_host->pdev->id,
                                                  temp & SDXC_DataFSMBusy ? "DataFSMBusy" : "",
                                                  smc_host->int_sum & SDXC_HardWLocked ? "HardWLocked" : "");
        sdxc_reset(smc_host);
        sdxc_program_clk(smc_host);
    }
    
    sdc_write(SDXC_REG_RINTR, 0xffff);
    sdxc_clear_imask(smc_host);
    //re-enable card detect debounce
    if (smc_host->cd_gpio == CARD_DETECT_BY_DATA3)
    {
        sdxc_cd_debounce_on(smc_host);
    }

    awsmc_dbg("smc %d done, resp %08x %08x %08x %08x\n", smc_host->pdev->id, req->cmd->resp[0], req->cmd->resp[1], req->cmd->resp[2], req->cmd->resp[3]);
    
	if (req->data && req->data->stop && (smc_host->int_sum & SDXC_IntErrBit))
	{
		awsmc_msg("found data error, need to send stop command !!\n");
		sdxc_send_manual_stop(smc_host, req);
	}
    
    if ((smc_host->pdev->id==3) && req->data && (req->data->flags & MMC_DATA_WRITE) && !(smc_host->int_sum & SDXC_IntErrBit))
    {
        u32 ready = 0;
//        u32 i = 0;
        do {
//            awsmc_msg("smc %d check ready %d \r", smc_host->pdev->id, i++);
            ready = sdxc_check_r1_ready(smc_host);
        } while (!ready);
//        awsmc_msg("\n");
    }
    return ret;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
u32 sdxc_check_card_busy(struct awsmc_host* smc_host)
{
    return sdc_read(SDXC_REG_STAS) & SDXC_CardDataBusy;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_regs_save(struct awsmc_host* smc_host)
{
	struct awsmc_ctrl_regs* bak_regs = &smc_host->bak_regs;

	bak_regs->gctrl		= sdc_read(SDXC_REG_GCTRL);
	bak_regs->clkc		= sdc_read(SDXC_REG_CLKCR);
	bak_regs->timeout	= sdc_read(SDXC_REG_TMOUT);
	bak_regs->buswid	= sdc_read(SDXC_REG_WIDTH);
	bak_regs->waterlvl	= sdc_read(SDXC_REG_FTRGL);
	bak_regs->funcsel	= sdc_read(SDXC_REG_FUNS);
	bak_regs->debugc	= sdc_read(SDXC_REG_DBGC);
	bak_regs->idmacc	= sdc_read(SDXC_REG_DMAC);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
void sdxc_regs_restore(struct awsmc_host* smc_host)
{
	struct awsmc_ctrl_regs* bak_regs = &smc_host->bak_regs;

    sdc_write(SDXC_REG_GCTRL, bak_regs->gctrl);
    sdc_write(SDXC_REG_CLKCR, bak_regs->clkc);
    sdc_write(SDXC_REG_TMOUT, bak_regs->timeout);
    sdc_write(SDXC_REG_WIDTH, bak_regs->buswid);
    sdc_write(SDXC_REG_FTRGL, bak_regs->waterlvl);
    sdc_write(SDXC_REG_FUNS, bak_regs->funcsel);
    sdc_write(SDXC_REG_DBGC, bak_regs->debugc);
    sdc_write(SDXC_REG_DMAC, bak_regs->idmacc);
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
s32 sdxc_init(struct awsmc_host* smc_host)
{
	struct smc_idma_des* pdes = NULL;
	
    /* reset controller */
    if (-1 == sdxc_reset(smc_host))
    {
        return -1;
    }
    
    sdc_write(SDXC_REG_GCTRL, SDXC_PosedgeLatchData);
    
    /* config DMA/Interrupt Trigger threshold */
    sdc_write(SDXC_REG_FTRGL, 0x70008);
    
    /* config timeout register */
    sdc_write(SDXC_REG_TMOUT, 0xffffffff);
    
    /* clear interrupt flags */
    sdc_write(SDXC_REG_RINTR, 0xffffffff);
    
    sdc_write(SDXC_REG_DBGC, 0xdeb);
    sdc_write(SDXC_REG_FUNS, 0xceaa0000);
    
    sdxc_int_enable(smc_host);

   	/* alloc idma descriptor structure */ 
	pdes = (struct smc_idma_des*)kmalloc(sizeof(struct smc_idma_des) * SDXC_MAX_DES_NUM, GFP_DMA | GFP_KERNEL);
	if (pdes == NULL)
	{
	    awsmc_dbg_err("alloc dma des failed\n");
	    return -1;
	}
	smc_host->pdes = pdes;
	awsmc_msg("sdc %d idma des address %p\n", smc_host->pdev->id, pdes);
    return 0;
}

/*********************************************************************
* Method	 :  
* Description:  
* Parameters :  
*	            
* Returns    :  
* Note       :
*********************************************************************/
s32 sdxc_exit(struct awsmc_host* smc_host)
{
	/* free idma descriptor structrue */
	if (smc_host->pdes)
	{
    	kfree((void*)smc_host->pdes);
		smc_host->pdes = NULL;
	}

    /* reset controller */
    if (-1 == sdxc_reset(smc_host))
    {
        return -1;
    }
    
    return 0;
}

