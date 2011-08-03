#ifndef _LINUX_SW_SYS_H_
#define _LINUX_SW_SYS_H_

#include <mach/script_v2.h>

#define SW_SCRIPT_PARA_MAX_NAME_LEN 32
#define SW_SCRIPT_PARA_VALUE_BUF_SIZE 128

struct sw_script_para{
	char main_name[SW_SCRIPT_PARA_MAX_NAME_LEN];
	char sub_name[SW_SCRIPT_PARA_MAX_NAME_LEN];
	int value[SW_SCRIPT_PARA_VALUE_BUF_SIZE];
	script_parser_value_type_t value_type;
}; 

typedef struct sw_script_para sw_script_para_t;

#define SW_SYS_IOC_GET_TOTAL_MAINKEY     _IOR('s', 0x01, struct sw_script_para)
#define SW_SYS_IOC_GET_TOTAL_SUBKEY      _IOR('s', 0x02, struct sw_script_para)
#define SW_SYS_IOC_GET_KEY_VALUE         _IOR('s', 0x03, struct sw_script_para)
#define SW_SYS_IOC_GET_TOTAL_GPIO        _IOR('s', 0x04, struct sw_script_para)
#define SW_SYS_IOC_GET_GPIO_CFG          _IOR('s', 0x05, struct sw_script_para)



#endif /* _LINUX_SW_SYS_H_ */




