/* 
******************************************************************************* 
*           				eBase 
*                 the Abstract of Hardware 
*                     
* 
*              (c) Copyright 2006-2010, ALL WINNER TECH. 
*           								All Rights Reserved 
* 
* File     :  D:\winners\eBase\eBSP\CSP\sun_20\HW_CCMU\ccm_para.h 
* Date     :  2010/11/27 12:19 
* By       :  Sam.Wu
* Version  :  V1.00 
* Description :  
* Update   :  date      author      version     
*
* notes    :
******************************************************************************* 
*/ 
#ifndef _CSP_CCM_PARA_H_
#define _CSP_CCM_PARA_H_



typedef enum {
    CSP_CCM_ERR_NONE,
    CSP_CCM_ERR_PARA_NULL,
    CSP_CCM_ERR_OSC_FREQ_CANNOT_BE_SET,
    CSP_CCM_ERR_PLL_FREQ_LOW,
    CSP_CCM_ERR_PLL_FREQ_HIGH,
    CSP_CCM_ERR_FREQ_NOT_STANDARD,
    CSP_CCM_ERR_CLK_NUM_NOT_SUPPORTED,
    CSP_CCM_ERR_DIVIDE_RATIO,
    CSP_CCM_ERR_CLK_IS_OFF,
    CSP_CCM_ERR_SRC_CLK_NOT_AVAILABLE,
    CSP_CCM_ERR_GET_CLK_FREQ,
    CSP_CCM_ERR_CLK_NO_INVALID,

    CSP_CCM_ERR_RESET_CONTROL_DENIED,
    CSP_CCM_ERR_NULL_PARA,
    CSP_CCM_ERR_PARA_VALUE,
}CSP_CCM_err_t;

/************************************************************************/
/* SYS CLK: system clocks, which are the source clocks of the chip 
 * 3 kinds of system clock: oscillate, PLL output, and CPU/AHB/APB.
*Note: when the frequency of the system clock has been changed, the clock frequency of
*all the clocks sourced form it will changed. As a result, we must reconfigure the module
*clocks which  source clock is been changed!*/
/************************************************************************/






#endif //#ifndef _CSP_CCM_PARA_H_




