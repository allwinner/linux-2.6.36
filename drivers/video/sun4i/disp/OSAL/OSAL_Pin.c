/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Pin.h
*
* Author 		: javen
*
* Description 	: C库函数
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*       holi     	   2010-12-02          1.1         添加具体的接口，
*************************************************************************************
*/
#include "OSAL_Pin.h"

/*
****************************************************************************************************
*
*             OSAL_GPIO_Request
*
*  Description:
*       设置某个逻辑设备的PIN，这里允许申请某个设备的部分PIN，
*
*  Parameters:
* 		gpio_list	    :	GPIO的数据结构
*       group_count_max :   GPIO数据结构的最大个数
* Returns    :
*		成功                返回句柄
*       失败                返回空
* Notes      :
*
****************************************************************************************************
*/

__hdle OSAL_GPIO_Request(user_gpio_set_t *gpio_list, __u32 group_count_max)
{
    gpio_request(gpio_list, group_count_max);
    
    return 0;
}


/*
****************************************************************************************************
*
*             OSAL_GPIO_Request_Ex
*
*  Description:
*       根据配置脚本中的GPIO名称，设置和名称匹配的GPIO参数
*
*  Parameters:
* 		main_name	    :	配置脚本的GPIO名称
* Returns    :
*		成功                返回句柄
*       失败                返回空
* Notes      :
*
****************************************************************************************************
*/
__hdle OSAL_GPIO_Request_Ex(char *main_name)
{
    return 0;
}
/*
****************************************************************************************************
*
*             OSAL_GPIO_Release
*
*  Description:
*       释放GPIO
*
*  Parameters:
* 		p_handler	:	handler
*       if_release_to_default_status   :  0或者1   ：释放后GPIO成为输入状态
*                                              2   : 释放后GPIO状态保持不变
*
* Returns    :
*		成功            返回0
*       失败            返回-1
* Notes      :
*
****************************************************************************************************
*/
__s32 OSAL_GPIO_Release(__hdle p_handler, __s32 if_release_to_default_status)
{
    gpio_release(p_handler, if_release_to_default_status);
    
    return 0;
}


/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevGetAllPins_Status
*
* Description:
*		获取所有GPIO的信息
* Arguments  :
*		devpin  	           :	GPIO句柄
*		gpio_status	           :	存放GPIO信息的数据结构，地址
*		gpio_count_max	       :	用户的数据结构的最大个数
*		if_get_from_hardware   :	0 获取用户最初传入的参数  1 获取当前硬件设置的参数
* Returns    :
*		                            成功返回0, 失败返回-1
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevGetAllPins_Status(__hdle devpin,  user_gpio_set_t  *gpio_status,  __u32 gpio_count_max , __u32 if_get_from_hardware)
{
    return 0;
}


/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevGetONEPins_Status
*
* Description:
*		获取单个GPIO的信息
* Arguments  :
*		devpin  	           :	GPIO句柄
*		gpio_status	           :	存放GPIO信息的数据结构，地址
*		gpio_name   	       :	用户要获取的GPIO的名称，这个名称来源于配置脚本
*		if_get_from_hardware   :	0 获取用户最初传入的参数  1 获取当前硬件设置的参数
* Returns    :
*		成功返回0, 失败返回-1
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevGetONEPins_Status(__hdle devpin,    user_gpio_set_t  *gpio_status,  const char *gpio_name, __u32 if_get_from_hardware)
{
    return 0;
}



/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevSetONEPin_Status
*
* Description:
*		设置单个GPIO的状态
* Arguments  :
*		devpin  	           :	GPIO句柄
*		gpio_status	           :	存放GPIO信息的数据结构，地址
*		gpio_name   	       :	用户要获取的GPIO的名称，这个名称来源于配置脚本
*		if_get_from_hardware   :	0 按照用户最初传入的参数设置  1 按照当前用户传入的参数设置
* Returns    :
*		成功返回0, 失败返回-1
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevSetONEPin_Status(__hdle devpin,    user_gpio_set_t  *gpio_status,  const char *gpio_name, __u32 if_set_to_current_input_status)
{
    return 0;
}


/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevSetONEPIN_IO_STATUS
*
* Description:
*		设置单个GPIO的IO状态
* Arguments  :
*		devpin  	                :	GPIO句柄
*		if_set_to_output_status	    :	0 设置成输入状态  1 设置成输入状态
*		gpio_name   	            :	用户要获取的GPIO的名称，这个名称来源于配置脚本
*
* Returns    :
*		成功返回0, 失败返回-1
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevSetONEPIN_IO_STATUS(__hdle devpin,  __u32 if_set_to_output_status,  const char *gpio_name)
{
    return 0;
}


/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevSetONEPIN_PULL_STATUS
*
* Description:
*		设置单个GPIO的内置电阻状态
* Arguments  :
*		devpin  	                :	GPIO句柄
*		if_set_to_output_status	    :	0 设置成高阻  1 设置成上拉  2 设置成下拉
*		gpio_name   	            :	用户要获取的GPIO的名称，这个名称来源于配置脚本
*
* Returns    :
*		成功返回0, 失败返回-1
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevSetONEPIN_PULL_STATUS(__hdle devpin,  __u32 if_set_to_output_status,  const char *gpio_name)
{
    return 0;
}


/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevREAD_ONEPIN_DATA
*
* Description:
*		设置单个GPIO的驱动能力状态
* Arguments  :
*		devpin  	                :	GPIO句柄
*		gpio_name   	            :	用户要获取的GPIO的名称，这个名称来源于配置脚本
*
* Returns    :
*		获取GPIO的状态   0：低电平  1：高电平  -1：当前GPIO不是输入状态，无法获取值
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevREAD_ONEPIN_DATA(__hdle devpin,  const char *gpio_name)
{
    gpio_read_one_pin_value(devpin, gpio_name);
    
    return 0;
}


/*
**********************************************************************************************************************
*                                               OSAL_GPIO_DevWRITE_ONEPIN_DATA
*
* Description:
*		设置单个GPIO的驱动能力状态
* Arguments  :
*		devpin  	                :	GPIO句柄
*       value_to_gpio               :   用户设置的电平  0：低电平  1：高电平
*		gpio_name   	            :	用户要获取的GPIO的名称，这个名称来源于配置脚本
*
* Returns    :
*		成功返回0   失败返回-1    如果当前GPIO不是输出状态，会返回失败
* Notes      :
*
**********************************************************************************************************************
*/
__s32 OSAL_GPIO_DevWRITE_ONEPIN_DATA(__hdle devpin, __u32 value_to_gpio, const char *gpio_name)
{
    gpio_write_one_pin_value(devpin, value_to_gpio, gpio_name);
    
    return 0;
}


