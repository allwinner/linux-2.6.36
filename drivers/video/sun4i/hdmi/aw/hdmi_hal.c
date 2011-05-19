#include "../hdmi_hal.h"
#include "hdmi_i2c.h"
#include "hdmi_core.h"

__s32 Hdmi_hal_set_display_mode(__u8 hdmi_mode)
{
    return 0;
}

//0:not supported; 1:supported; 
__s32 Hdmi_hal_mode_support(__u8 mode)
{
    return 0;
}

//0:plug out; 1:plug in
__s32 Hdmi_hal_get_HPD_status(void)
{
    return 0;
}

__s32 Hdmi_hal_audio_enable(__u8 mode, __u8 channel)
{
    return 0;
}

__s32 Hdmi_hal_set_audio_para(hdmi_audio_t * audio_para)
{
    return 0;
}

__hdmi_state_t Hdmi_hal_get_connection_status(void)
{
    return 0;
}

__s32 Hdmi_hal_standby_exit(void)
{
    return 0;
}

__s32 Hdmi_hal_main_task(void)
{    
    return 0;
}

__s32 Hdmi_hal_init(void)
{	
    return 0;
}

__s32 Hdmi_hal_exit(void)
{
    return 0;
}

