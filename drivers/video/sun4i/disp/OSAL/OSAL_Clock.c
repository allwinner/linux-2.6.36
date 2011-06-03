/*
********************************************************************************
*                                                   OSAL
*
*                                     (c) Copyright 2008-2009, Kevin China
*                                             				All Rights Reserved
*
* File    : OSAL_Clock.c
* By      : Sam.Wu
* Version : V1.00
* Date    : 2011/3/25 20:25 
* Description :  
* Update   :  date      author      version     notes    
********************************************************************************
*/
#include "OSAL.h"
#include "OSAL_Clock.h"

#if 0

#define CLK_DEBE0_AHB_ON	    0x00000001
#define CLK_DEBE0_MOD_ON 	    0x00000002
#define CLK_DEBE0_DRAM_ON	    0x00000004
#define CLK_DEBE1_AHB_ON	    0x00000010
#define CLK_DEBE1_MOD_ON 	    0x00000020
#define CLK_DEBE1_DRAM_ON	    0x00000040
#define CLK_DEFE0_AHB_ON	    0x00000100
#define CLK_DEFE0_MOD_ON 	    0x00000200
#define CLK_DEFE0_DRAM_ON	    0x00000400
#define CLK_DEFE1_AHB_ON	    0x00001000
#define CLK_DEFE1_MOD_ON 	    0x00002000
#define CLK_DEFE1_DRAM_ON	    0x00004000
#define CLK_LCDC0_AHB_ON	    0x00010000
#define CLK_LCDC0_CH0_MOD_ON  	0x00020000
#define CLK_LCDC0_CH1_MOD1x_ON  0x00040000
#define CLK_LCDC0_CH1_MOD2x_ON  0x00080000
#define CLK_LCDC1_AHB_ON	    0x00100000
#define CLK_LCDC1_CH0_MOD_ON  	0x00200000
#define CLK_LCDC1_CH1_MOD1x_ON  0x00400000
#define CLK_LCDC1_CH1_MOD2x_ON  0x00800000
#define CLK_TVE0_AHB_ON		    0x01000000	
#define CLK_TVE1_AHB_ON		    0x02000000	
#define CLK_HDMI_AHB_ON		    0x04000000	
#define CLK_HDMI_MOD_ON		    0x08000000	
#define CLK_LVDS_MOD_ON 	    0x10000000

static char* _sysClkName[CSP_CCM_SYS_CLK_TOTAL_NUM] =
{
    "sdram_pll",
    "video_pll0",
    "video_pll1",
    "video_pll0_2x",
    "video_pll1_2x",
};

static char* _modClkName[CSP_CCM_MOD_CLK_TOTAL_NUM] =
{
    "de_image0",
    "de_image1",
    "de_scale0",
    "de_scale1",
    "lcd0_ch0",
    "lcd0_ch1_clk2",
    "lcd0_ch1_clk1",
    "lcd1_ch0",
    "lcd1_ch1_clk2",
    "lcd1_ch1_clk1",
    "lvds",
    "hdmi",

    "ahb_de_image0",
    "ahb_de_image1",
    "ahb_de_scale0",
    "ahb_de_scale1",
    "ahb_lcd0",
    "ahb_lcd1",
    "ahb_tvenc0",
    "ahb_tvenc1",
    "ahb_hdmi"

    "sdram_de_image0",
    "sdram_de_image1",
    "sdram_de_scale0",
    "sdram_de_scale1",
};

__s32 OSAL_CCMU_SetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo, __u32 nFreq )
{
    struct clk* hSysClk = NULL;
    s32 retCode = -1;

    hSysClk = clk_get(NULL, _sysClkName[nSclkNo]);
    if(NULL == hSysClk){
        printk("Fail to get handle for system clock [%d].\n", nSclkNo);
        return -1;
    }
    if(nFreq == clk_get_rate(hSysClk)){
       // printk("Sys clk[%d] freq is alreay %d, not need to set.\n", nSclkNo, nFreq);
        clk_put(hSysClk);
        return 0;
    }
    retCode = clk_set_rate(hSysClk, nFreq);
    if(-1 == retCode){
        printk("Fail to set nFreq[%d] for sys clk[%d].\n", nFreq, nSclkNo);
        clk_put(hSysClk);
        return retCode;
    }
    clk_put(hSysClk);
    hSysClk = NULL;

    return retCode;
}

__u32 OSAL_CCMU_GetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo )
{
    struct clk* hSysClk = NULL;
    u32 nFreq = 0;

    hSysClk = clk_get(NULL, _sysClkName[nSclkNo]);
    if(NULL == hSysClk){
        printk("Fail to get handle for system clock [%d].\n", nSclkNo);
        return -1;
    }
    nFreq = clk_get_rate(hSysClk);
    clk_put(hSysClk);
    hSysClk = NULL;

    return nFreq;
}

__hdle OSAL_CCMU_OpenMclk( __s32 nMclkNo )
{
    struct clk* hModClk = NULL;

    hModClk = clk_get(NULL, _modClkName[nMclkNo]);

    return (__hdle)hModClk;
}

__s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
    struct clk* hModClk = (struct clk*)hMclk;

    clk_put(hModClk);

    return 0;
}

__s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk, CSP_CCM_sysClkNo_t nSclkNo )
{
    struct clk* hSysClk = NULL;
    struct clk* hModClk = (struct clk*)hMclk;
    s32 retCode = -1;

    hSysClk = clk_get(NULL, _sysClkName[nSclkNo]);
    if(NULL == hSysClk){
        printk("Fail to get handle for system clock [%d].\n", nSclkNo);
        return -1;
    }
    if(clk_get_parent(hModClk) == hSysClk){
        //printk("Parent is alreay %d, not need to set.\n", nSclkNo);
        clk_put(hSysClk);
        return 0;
    }
    retCode = clk_set_parent(hModClk, hSysClk);
    if(-1 == retCode){
        printk("Fail to set parent for clk.\n");
        clk_put(hSysClk);
        return -1;
    }
    clk_put(hSysClk);

    return retCode;
}

__s32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
    int sysClkNo = 0;
    struct clk* hModClk = (struct clk*)hMclk;
    struct clk* hParentClk = clk_get_parent(hModClk);
    const int TOTAL_SYS_CLK = sizeof(_sysClkName)/sizeof(char*);

    for (; sysClkNo <  TOTAL_SYS_CLK; sysClkNo++)
    {
        struct clk* tmpSysClk = clk_get(NULL, _sysClkName[sysClkNo]);
        
        if(tmpSysClk == NULL)
        	continue;

        if(hParentClk == tmpSysClk){
            clk_put(tmpSysClk);
            break;
        }
        clk_put(tmpSysClk);
    }

    if(sysClkNo >= TOTAL_SYS_CLK){
        printk("Failed to get parent clk.\n");
        return -1;
    }

    return sysClkNo;
}

__s32 OSAL_CCMU_SetMclkDiv( __hdle hMclk, __s32 nDiv )
{
    struct clk* hModClk     = (struct clk*)hMclk;
    struct clk* hParentClk  = clk_get_parent(hModClk);
    u32         srcRate     = clk_get_rate(hParentClk);
    
    if(nDiv == 0){
    	return -1;
    }
    
    return clk_set_rate(hModClk, srcRate/nDiv);
}

__u32 OSAL_CCMU_GetMclkDiv( __hdle hMclk )
{
    struct clk* hModClk = (struct clk*)hMclk;
    struct clk* hParentClk = clk_get_parent(hModClk);
    u32 mod_freq = clk_get_rate(hModClk);
    
    if(mod_freq == 0){
    	return 0;	
    }

    return clk_get_rate(hParentClk)/mod_freq;
}

__s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, __s32 bOnOff )
{
    struct clk* hModClk = (struct clk*)hMclk;

    if(bOnOff)
    {
        return clk_enable(hModClk);
    }

    clk_disable(hModClk);

    return 0;
}

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset)
{
    struct clk* hModClk = (struct clk*)hMclk;

    return clk_reset(hModClk, bReset);
}
#else
__s32 OSAL_CCMU_SetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo, __u32 nFreq )
{
    return 0;
}

__u32 OSAL_CCMU_GetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo )
{
    return 0;
}

__hdle OSAL_CCMU_OpenMclk( __s32 nMclkNo )
{
    return 0;
}

__s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
    return 0;
}

__s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk, CSP_CCM_sysClkNo_t nSclkNo )
{
    return 0;
}

__s32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
    return 0;
}

__s32 OSAL_CCMU_SetMclkDiv( __hdle hMclk, __s32 nDiv )
{
    return 0;
}

__u32 OSAL_CCMU_GetMclkDiv( __hdle hMclk )
{
    return 0;
}

__s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, __s32 bOnOff )
{
    return 0;
}

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset)
{
    return 0;
}
#endif

