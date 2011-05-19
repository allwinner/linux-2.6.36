#ifndef __HDMI_I2C_H__
#define __HDMI_I2C_H__

#include "../hdmi_hal.h"

__s32 ANX7150_i2c_Request(void);
__s32 ANX7150_i2c_Release(void);
__s32 ANX7150_i2c_write_p0_reg(__u8 offset, __u8 d);
__s32 ANX7150_i2c_write_p1_reg(__u8 offset, __u8 d);
__s32 ANX7150_i2c_read_p0_reg(__u8 offset, __u8 *d);
__s32 ANX7150_i2c_read_p1_reg(__u8 offset, __u8 *d);

#endif