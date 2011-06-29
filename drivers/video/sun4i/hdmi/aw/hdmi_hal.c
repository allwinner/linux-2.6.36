#include "../hdmi_hal.h"
#include "hdmi_interface.h"
#include "hdmi_core.h"


extern __s32 hdmi_state;
extern __s32 video_mode;
extern HDMI_AUDIO_INFO audio_info;
__u32 HDMI_BASE = 0;

void Hdmi_set_reg_base(__u32 base)
{
    HDMI_BASE = base;
}

__s32 Hdmi_hal_video_enable(__bool enable)
{
    if(enable)
    {
        hdmi_state	=	HDMI_State_Wait_Hpd;
    }
    else
    {
        hdmi_state	=	HDMI_State_Idle;
    }
    return 0;
}

__s32 Hdmi_hal_set_display_mode(__u8 hdmi_mode)
{
	if(hdmi_mode != video_mode)
	{
		hdmi_state = HDMI_State_Video_config;
		video_mode = hdmi_mode;
	}
    return 0;
}


__s32 Hdmi_hal_audio_enable(__u8 mode, __u8 channel)
{
	/////////????????????????????????
	hdmi_state 				= HDMI_State_Audio_config;
	audio_info.channel_num  = 2;
	audio_info.audio_en     = (channel == 0)?0:1;

    return 0;
}

__s32 Hdmi_hal_set_audio_para(hdmi_audio_t * audio_para)
{
    if(!audio_para)
    {
        return -1;
    }

    if(audio_para->sample_rate != audio_info.sample_rate)
    {
    	hdmi_state 				= HDMI_State_Audio_config;
    	audio_info.sample_rate 	= audio_para->sample_rate;
    	audio_info.channel_num  = 2;

    	__inf("sample_rate:%d in Hdmi_hal_set_audio_para\n", audio_info.sample_rate);
    }

    return 0;
}

__s32 Hdmi_hal_mode_support(__u8 mode)
{
	return 1;
}

__s32 Hdmi_hal_get_HPD(void)
{
	return Hpd_Check();
}

__s32 Hdmi_hal_get_state(void)
{
    return HDMI_State_Playback;
}

__s32 Hdmi_hal_main_task(void)
{
    hdmi_main_task_loop();
    return 0;
}

__s32 Hdmi_hal_init(void)
{	
    //hdmi_audio_t audio_para;
    
	hdmi_core_initial();

//for audio test
#if 0
    audio_para.ch0_en = 1;
    audio_para.sample_rate = 44100;
	Hdmi_hal_set_audio_para(&audio_para);

	Hdmi_hal_audio_enable(0, 1);
#endif

    return 0;
}

__s32 Hdmi_hal_exit(void)
{
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
