#ifndef _ANX7150_Sys_H
#define _ANX7150_Sys_H
#include "../hdmi_hal.h"
#include "hdmi_interface.h"

extern __u32 HDMI_BASE;

#define HDMI_WUINT32(offset,value)  (*((volatile __u32 *)(HDMI_BASE + offset))=(value))
#define HDMI_RUINT32(offset)        (*((volatile __u32 *)(HDMI_BASE + offset)))
#define HDMI_WUINT16(offset,value)  (*((volatile __u16 *)(HDMI_BASE + offset))=(value))
#define HDMI_RUINT16(offset)        (*((volatile __u16 *)(HDMI_BASE + offset)))
#define HDMI_WUINT8(offset,value)  (*((volatile __u8 *)(HDMI_BASE + offset))=(value))
#define HDMI_RUINT8(offset)        (*((volatile __u8 *)(HDMI_BASE + offset)))

#define HDMI_State_Idle 			0x00
#define HDMI_State_Wait_Hpd			0x02
#define HDMI_State_Rx_Sense			0x03
#define HDMI_State_EDID_Parse		0x04
#define HDMI_State_Video_config		0x05
#define HDMI_State_Audio_config		0x06
#define HDMI_State_Playback			0x09

#define HDMI1440_480I		6
#define HDMI1440_576I		21
#define HDMI480P			2
#define HDMI576P			17
#define HDMI720P_50			19
#define HDMI720P_60 		4
#define HDMI1080I_50		20
#define HDMI1080I_60		5
#define HDMI1080P_50		31
#define HDMI1080P_60 		16
#define HDMI1080P_24 		32
#define HDMI1080P_25 		33
#define HDMI1080P_24_3D_FP  (HDMI1080P_24 +0x10000)


#define Abort_Current_Operation				0
#define Special_Offset_Address_Read 		1
#define Explicit_Offset_Address_Write		2
#define Implicit_Offset_Address_Write		3
#define Explicit_Offset_Address_Read		4
#define Implicit_Offset_Address_Read 		5
#define Explicit_Offset_Address_E_DDC_Read	6
#define Implicit_Offset_Address_E_DDC_Read	7



typedef struct video_timing
{
	__s32 VIC;
	__s32 PCLK; 
	__s32 AVI_PR;
	 
	__s32 INPUTX; 	
	__s32 INPUTY; 	
	__s32 HT; 		
	__s32 HBP; 	
	__s32 HFP; 	
	__s32 HPSW; 	
	__s32 VT; 		
	__s32 VBP; 	
	__s32 VFP; 	
	__s32 VPSW; 	

}HDMI_VIDE_INFO;

typedef struct audio_timing
{

 	__s32 audio_en;
 	__s32 sample_rate;
 	__s32 channel_num;
	 
	__s32 CTS; 	
	__s32 ACR_N; 	
	__s32 CH_STATUS0; 		
	__s32 CH_STATUS1; 	 	

}HDMI_AUDIO_INFO;

__s32 hdmi_core_initial(void);
__s32 hdmi_core_open(void);
__s32 hdmi_core_close(void);
__s32 hdmi_main_task_loop(void);
__s32 Hpd_Check(void);
__s32 get_video_info(__s32 vic);
__s32 get_audio_info(__s32 sample_rate);
__s32 video_config(__s32 vic);
__s32 audio_config(void);





void DDC_Init(void);
void send_ini_sequence(void);
void DDC_Read(char cmd,char pointer,char offset,int nbyte,char * pbuf);
#endif

