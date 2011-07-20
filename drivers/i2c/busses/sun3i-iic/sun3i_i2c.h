/*
 *  
 *	
 */
#ifndef _SUN3I_I2C_H_
#define _SUN3I_I2C_H_

#define INTC_IRQNO_TWI0          7
#define INTC_IRQNO_TWI1          8
#define INTC_IRQNO_TWI2          62

#define AW_TWI_ADDR_SIZE		0x3ff

#define TWI0_BASE_ADDR_START  (0x01C24000)
#define TWI0_BASE_ADDR_END    (TWI0_BASE_ADDR_START + AW_TWI_ADDR_SIZE)
#define TWI1_BASE_ADDR_START  (0x01C24400)
#define TWI1_BASE_ADDR_END    (TWI1_BASE_ADDR_START + AW_TWI_ADDR_SIZE)
#define TWI2_BASE_ADDR_START  (0x01C26C00)
#define TWI2_BASE_ADDR_END    (TWI2_BASE_ADDR_START + AW_TWI_ADDR_SIZE)


struct aw_i2c_platform_data {
	int 		 bus_num;
	unsigned int frequency;
};

extern void aw_set_i2c_info(struct aw_i2c_platform_data *info);



#endif
