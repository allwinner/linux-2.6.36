/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : aw_ccu.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-13 18:52
* Descript: ccu operation fucntion for allwinners chips.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __AW_CCU_H__
#define __AW_CCU_H__

#include <linux/kernel.h>
#include <linux/string.h>

/* define clock error type      */
typedef enum __AW_CCU_ERR
{
    AW_CCU_ERR_NONE         = 0,
    AW_CCU_ERR_PARA_NULL    = -1,
    AW_CCU_ERR_PARA_INVALID = -1,

} __aw_ccu_err_e;


typedef enum __AW_CCU_CLK_ONOFF
{
    AW_CCU_CLK_OFF,
    AW_CCU_CLK_ON,

} __aw_ccu_clk_onff_e;


typedef enum __AW_CCU_CLK_RESET
{
    AW_CCU_CLK_RESET,
    AW_CCU_CLK_NRESET,

} __aw_ccu_clk_reset_e;


/* define system clock id       */
typedef enum __AW_CCU_SYS_CLK
{
    AW_SYS_CLK_NONE,    /* invalid clock id         */

    AW_SYS_CLK_LOSC,    /* LOSC, 32768 hz clock     */
    AW_SYS_CLK_HOSC,    /* HOSC, 24Mhz clock        */

    AW_SYS_CLK_PLL1,    /* PLL1 clock               */
    AW_SYS_CLK_PLL2,    /* PLL2 clock               */
    AW_SYS_CLK_PLL3,    /* PLL3 clock               */
    AW_SYS_CLK_PLL4,    /* PLL4 clock               */
    AW_SYS_CLK_PLL5,    /* PLL5 clock               */
    AW_SYS_CLK_PLL5M,   /* PLL5 M clock             */
    AW_SYS_CLK_PLL5P,   /* PLL5 P clock             */
    AW_SYS_CLK_PLL6,    /* PLL6 clock               */
    AW_SYS_CLK_PLL7,    /* PLL7 clock               */
    AW_SYS_CLK_200M,    /* 200Mhz clock             */

    AW_SYS_CLK_CPU,     /* CPU clock                */
    AW_SYS_CLK_AXI,     /* AXI clock                */
    AW_SYS_CLK_AHB,     /* AHB clock                */
    AW_SYS_CLK_APB0,    /* APB0 clock               */
    AW_SYS_CLK_APB1,    /* APB1 clock               */

    AW_SYS_CLK_CNT      /* invalid id, for calc count   */

} __aw_ccu_sys_clk_e;


/* define module clock id       */
typedef enum __AW_CCU_MOD_CLK
{
    AW_MOD_CLK_NONE,/* invalid clock id             */

    AW_MOD_CLK_NFC,
    AW_MOD_CLK_MSC,
    AW_MOD_CLK_SDC0,
    AW_MOD_CLK_SDC1,
    AW_MOD_CLK_SDC2,
    AW_MOD_CLK_SDC3,
    AW_MOD_CLK_TS,
    AW_MOD_CLK_SS,
    AW_MOD_CLK_SPI0,
    AW_MOD_CLK_SPI1,
    AW_MOD_CLK_SPI2,
    AW_MOD_CLK_PATA,
    AW_MOD_CLK_IR0,
    AW_MOD_CLK_IR1,
    AW_MOD_CLK_I2S,
    AW_MOD_CLK_AC97,
    AW_MOD_CLK_SPDIF,
    AW_MOD_CLK_KEYPAD,
    AW_MOD_CLK_SATA,
    AW_MOD_CLK_USBPHY,
    AW_MOD_CLK_USBPHY0,
    AW_MOD_CLK_USBPHY1,
    AW_MOD_CLK_USBPHY2,
    AW_MOD_CLK_USBOHCI0,
    AW_MOD_CLK_USBOHCI1,
    AW_MOD_CLK_GPS,
    AW_MOD_CLK_SPI3,
    AW_MOD_CLK_DEBE0,
    AW_MOD_CLK_DEBE1,
    AW_MOD_CLK_DEFE0,
    AW_MOD_CLK_DEFE1,
    AW_MOD_CLK_DEMIX,
    AW_MOD_CLK_LCD0CH0,
    AW_MOD_CLK_LCD1CH0,
    AW_MOD_CLK_CSIISP,
    AW_MOD_CLK_TVD,
    AW_MOD_CLK_LCD0CH1,
    AW_MOD_CLK_LCD1CH1,
    AW_MOD_CLK_CSI0,
    AW_MOD_CLK_CSI1,
    AW_MOD_CLK_VE,
    AW_MOD_CLK_ADDA,
    AW_MOD_CLK_AVS,
    AW_MOD_CLK_ACE,
    AW_MOD_CLK_LVDS,
    AW_MOD_CLK_HDMI,
    AW_MOD_CLK_MALI,
    AW_MOD_CLK_TWI0,
    AW_MOD_CLK_TWI1,
    AW_MOD_CLK_TWI2,
    AW_MOD_CLK_CAN,
    AW_MOD_CLK_SCR,
    AW_MOD_CLK_PS20,
    AW_MOD_CLK_PS21,
    AW_MOD_CLK_UART0,
    AW_MOD_CLK_UART1,
    AW_MOD_CLK_UART2,
    AW_MOD_CLK_UART3,
    AW_MOD_CLK_UART4,
    AW_MOD_CLK_UART5,
    AW_MOD_CLK_UART6,
    AW_MOD_CLK_UART7,

    /* clock gating for hang to AXI bus */
    AW_MOD_CLK_AXI_DRAM,

    /* clock gating for hang to AHB bus */
    AW_MOD_CLK_AHB_USB0,
    AW_MOD_CLK_AHB_USB1,
    AW_MOD_CLK_AHB_USB2,
    AW_MOD_CLK_AHB_SS,
    AW_MOD_CLK_AHB_DMA,
    AW_MOD_CLK_AHB_BIST,
    AW_MOD_CLK_AHB_SDMMC0,
    AW_MOD_CLK_AHB_SDMMC1,
    AW_MOD_CLK_AHB_SDMMC2,
    AW_MOD_CLK_AHB_SDMMC3,
    AW_MOD_CLK_AHB_MS,
    AW_MOD_CLK_AHB_NAND,
    AW_MOD_CLK_AHB_SDRAM,
    AW_MOD_CLK_AHB_ACE,
    AW_MOD_CLK_AHB_EMAC,
    AW_MOD_CLK_AHB_TS,
    AW_MOD_CLK_AHB_SPI0,
    AW_MOD_CLK_AHB_SPI1,
    AW_MOD_CLK_AHB_SPI2,
    AW_MOD_CLK_AHB_SPI3,
    AW_MOD_CLK_AHB_PATA,
    AW_MOD_CLK_AHB_SATA,
    AW_MOD_CLK_AHB_GPS,
    AW_MOD_CLK_AHB_VE,
    AW_MOD_CLK_AHB_TVD,
    AW_MOD_CLK_AHB_TVE0,
    AW_MOD_CLK_AHB_TVE1,
    AW_MOD_CLK_AHB_LCD0,
    AW_MOD_CLK_AHB_LCD1,
    AW_MOD_CLK_AHB_CSI0,
    AW_MOD_CLK_AHB_CSI1,
    AW_MOD_CLK_AHB_HDMI,
    AW_MOD_CLK_AHB_DEBE0,
    AW_MOD_CLK_AHB_DEBE1,
    AW_MOD_CLK_AHB_DEFE0,
    AW_MOD_CLK_AHB_DEFE1,
    AW_MOD_CLK_AHB_MP,
    AW_MOD_CLK_AHB_MALI,

    /* clock gating for hang APB bus */
    AW_MOD_CLK_APB_ADDA,
    AW_MOD_CLK_APB_SPDIF,
    AW_MOD_CLK_APB_AC97,
    AW_MOD_CLK_APB_I2S,
    AW_MOD_CLK_APB_PIO,
    AW_MOD_CLK_APB_IR0,
    AW_MOD_CLK_APB_IR1,
    AW_MOD_CLK_APB_KEYPAD,
    AW_MOD_CLK_APB_TWI0,
    AW_MOD_CLK_APB_TWI1,
    AW_MOD_CLK_APB_TWI2,
    AW_MOD_CLK_APB_CAN,
    AW_MOD_CLK_APB_SCR,
    AW_MOD_CLK_APB_PS20,
    AW_MOD_CLK_APB_PS21,
    AW_MOD_CLK_APB_UART0,
    AW_MOD_CLK_APB_UART1,
    AW_MOD_CLK_APB_UART2,
    AW_MOD_CLK_APB_UART3,
    AW_MOD_CLK_APB_UART4,
    AW_MOD_CLK_APB_UART5,
    AW_MOD_CLK_APB_UART6,
    AW_MOD_CLK_APB_UART7,

    /* clock gating for access dram */
    AW_MOD_CLK_SDRAM_VE,
    AW_MOD_CLK_SDRAM_CSI0,
    AW_MOD_CLK_SDRAM_CSI1,
    AW_MOD_CLK_SDRAM_TS,
    AW_MOD_CLK_SDRAM_TVD,
    AW_MOD_CLK_SDRAM_TVE0,
    AW_MOD_CLK_SDRAM_TVE1,
    AW_MOD_CLK_SDRAM_DEFE0,
    AW_MOD_CLK_SDRAM_DEFE1,
    AW_MOD_CLK_SDRAM_DEBE0,
    AW_MOD_CLK_SDRAM_DEBE1,
    AW_MOD_CLK_SDRAM_DEMP,
    AW_MOD_CLK_SDRAM_ACE,

    AW_MOD_CLK_CNT

} __aw_ccu_mod_clk_e;


/* define handle for moduel clock   */
typedef struct __AW_CCU_CLK
{
    __s32       id;     /* clock id         */
    __s32       parent; /* parent clock id  */
    char        *name;  /* clock name       */
    __s32       onoff;  /* on/off status    */
    __s32       reset;  /* reset status     */
    __u64       rate;   /* clock rate, frequency for system clock, division for module clock */
    __s32       hash;   /* hash value, for fast search without string compare   */

}__aw_ccu_clk_t;


/*
*********************************************************************************************************
*                           mod_clk_calc_hash
*
*Description: calculate hash value of a string;
*
*Arguments  : string    string whose hash value need be calculate;
*
*Return     : hash value
*
*Notes      :
*
*********************************************************************************************************
*/
static inline __s32 ccu_clk_calc_hash(char *string)
{
    __s32   tmpLen, i, tmpHash = 0;

    if(!string)
    {
        return 0;
    }

    tmpLen = strlen(string);
    for(i=0; i<tmpLen; i++)
    {
        tmpHash += string[i];
    }

    return tmpHash;
}


static inline __u64 ccu_clk_uldiv(__u64 dividend, __u32 divisior)
{
    __u64   tmpDiv = (__u64)divisior;
    __u64   tmpQuot = 0;
    __s32   shift = 0;

    if(!divisior)
    {
        /* divide 0 error abort */
        return (__u32)dividend/divisior;
    }

    while(!(tmpDiv & ((__u64)1<<63)))
    {
        tmpDiv <<= 1;
        shift ++;
    }

    do
    {
        if(dividend >= tmpDiv)
        {
            dividend -= tmpDiv;
            tmpQuot = (tmpQuot << 1) | 1;
        }
        else
        {
            tmpQuot = (tmpQuot << 1) | 0;
        }
        tmpDiv >>= 1;
        shift --;
    } while(shift >= 0);

    return tmpQuot;
}


__s32 aw_ccu_init(void);
__s32 aw_ccu_exit(void);

__s32 aw_ccu_get_sys_clk_cnt(void);
__aw_ccu_clk_t *aw_ccu_get_sys_clk(__aw_ccu_sys_clk_e id);
__aw_ccu_err_e aw_ccu_set_sys_clk(__aw_ccu_clk_t *clk);

__s32 aw_ccu_get_mod_clk_cnt(void);
__aw_ccu_clk_t *aw_ccu_get_mod_clk(__aw_ccu_mod_clk_e id);
__aw_ccu_err_e aw_ccu_set_mod_clk(__aw_ccu_clk_t *clk);


#endif /* #ifndef __AW_CCU_H__ */

