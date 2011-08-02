#ifndef _LINUX_SW_SYS_H_
#define _LINUX_SW_SYS_H_


#define MAX_KEY_NAME_LEN 32
#define MAX_BUF_LEN 128

struct sw_script_para {
	char main_name[MAX_KEY_NAME_LEN];
	char sub_name[MAX_KEY_NAME_LEN];
	char buf[MAX_BUF_LEN];
	int data;
};

#define SW_SYS_IOC_GET_TOTAL_MAINKEY     0x01
#define SW_SYS_IOC_GET_TOTAL_SUBKEY      0x02
#define SW_SYS_IOC_GET_KEY_INT           0x03
#define SW_SYS_IOC_GET_KEY_STRING        0x04
#define SW_SYS_IOC_GET_TOTAL_GPIO        0x05
#define SW_SYS_IOC_GET_GPIO_CFG          0x06



#endif /* _LINUX_SW_SYS_H_ */




