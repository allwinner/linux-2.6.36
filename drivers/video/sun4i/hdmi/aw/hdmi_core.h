#ifndef _ANX7150_Sys_H
#define _ANX7150_Sys_H

#include "../hdmi_hal.h"
#include "hdmi_interface.h"


//#define HDMI1440_480I
//#define HDMI1440_576I
//#define HDMI480P
//#define HDMI576P
#define HDMI720P_50
//#define HDMI720P_60 
//#define HDMI1080I_50
//#define HDMI1080I_60
//#define HDMI1080P_50
//#define HDMI1080P_60 
//#define HDMI1080P_24


//#define AUDIO_32K
#define AUDIO_44K
//#define AUDIO_48K
//#define AUDIO_192K

#define AUDIO_2CH
//#define AUDIO_8CH

#define AUDIO_DDMA

#define Abort_Current_Operation				0
#define Special_Offset_Address_Read 		1
#define Explicit_Offset_Address_Write		2
#define Implicit_Offset_Address_Write		3
#define Explicit_Offset_Address_Read		4
#define Implicit_Offset_Address_Read 		5
#define Explicit_Offset_Address_E_DDC_Read	6
#define Implicit_Offset_Address_E_DDC_Read	7




///////////////////////////////////////////
#ifdef  AUDIO_32K
#define CH_STATUS0 (3<<24)
#define CH_STATUS1 0x0000000b
#endif

#ifdef  AUDIO_44K
#define CH_STATUS0 (0<<24)
#define CH_STATUS1 0x0000000b
#endif


#ifdef  AUDIO_48K
#define CH_STATUS0 (2<<24)
#define CH_STATUS1 0x0000000b
#endif

#ifdef  AUDIO_192K
#define CH_STATUS0 (0x0e<<24)
#define CH_STATUS1 0x0000000b
#endif

////////////////////////////////////////////
#ifdef  HDMI1440_480I

#define PIX_13M5
#define VIC  	6
#define AVI_PR 	1
 
#define INPUTX 	720
#define INPUTY 	240

#define HT 		858
#define HBP 	119
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	62

#define VT 		525
#define VBP 	21
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	3

#ifdef  AUDIO_32K

#define CTS 	27000
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	30000
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	27000
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	27000
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
#ifdef  HDMI1440_576I

#define PIX_13M5
#define VIC 	21
#define AVI_PR 	1
 
#define INPUTX 	720
#define INPUTY 	288

#define HT 		864
#define HBP 	132
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	63

#define VT 		625
#define VBP 	22
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	3

#ifdef  AUDIO_32K

#define CTS 	27000
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	30000
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	27000
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	27000
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
#ifdef  HDMI576P

#define PIX_27M
#define VIC 	17
#define AVI_PR 	0
 
#define INPUTX 	720
#define INPUTY 	576

#define HT 		864
#define HBP 	132
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	64

#define VT 		1250
#define VBP 	44
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	27000
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	30000
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	27000
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	27000
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
#ifdef  HDMI480P

#define PIX_27M
#define VIC 	2
#define AVI_PR 	0
 
#define INPUTX 	720
#define INPUTY 	480

#define HT 		858
#define HBP 	122
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	62

#define VT 		1050
#define VBP 	36
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	6

#ifdef  AUDIO_32K

#define CTS 	27000
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	30000
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	27000
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	27000
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
#ifdef  HDMI720P_50

#define PIX_745M
#define VIC 	19
#define AVI_PR 	0
 
#define INPUTX 	1280
#define INPUTY 	720

#define HT 		1980
#define HBP 	260
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	40

#define VT 		1500
#define VBP 	25
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	74250
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	82500
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	72500
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	72500
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
///////////////////////////////////////////
#ifdef  HDMI720P_60

#define PIX_745M
#define VIC 	4
#define AVI_PR 	0
 
#define INPUTX 	1280
#define INPUTY 	720

#define HT 		1650
#define HBP 	260
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	40

#define VT 		1500
#define VBP 	25
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	74250
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	82500
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	74250
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	74250
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
#ifdef  HDMI1080I_50

#define PIX_745M
#define VIC 	39
#define AVI_PR 	0
 
#define INPUTX 	1920
#define INPUTY 	540

#define HT 		2640
#define HBP 	192
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	44

#define VT 		1125
#define VBP 	20
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	74250
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	82500
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	72500
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	72500
#define ACRN 	24576

#endif


#endif
///////////////////////////////////////////
#ifdef  HDMI1080I_60

#define PIX_745M
#define VIC 	5
#define AVI_PR 	0
 
#define INPUTX 	1920
#define INPUTY 	540

#define HT 		2200
#define HBP 	192
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	44

#define VT 		1125
#define VBP 	20
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	74250
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	82500
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	72500
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	72500
#define ACRN 	24576

#endif


#endif

///////////////////////////////////////////
#ifdef  HDMI1080P_50

#define PIX_148M
#define VIC 	31
#define AVI_PR 	0
 
#define INPUTX 	1920
#define INPUTY 	1080

#define HT 		2640
#define HBP 	192
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	44

#define VT 		(1125*2)
#define VBP 	41
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	148500
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	165000
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	148500
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	148500
#define ACRN 	24576

#endif


#endif
/////////////////////////////////////
#ifdef  HDMI1080P_60

#define PIX_148M
#define VIC 	16
#define AVI_PR 	0
 
#define INPUTX 	1920
#define INPUTY 	1080

#define HT 		2200
#define HBP 	192
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	44

#define VT 		(1125*2)
#define VBP 	41
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	148500
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	165000
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	148500
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	148500
#define ACRN 	24576

#endif


#endif


/////////////////////////////////////
#ifdef  HDMI1080P_24

#define PIX_745M
#define VIC 	21
#define AVI_PR 	0
 
#define INPUTX 	1920
#define INPUTY 	1080

#define HT 		2750
#define HBP 	192
#define HFP 	(HT - INPUTX - HBP)
#define HPSW 	44

#define VT 		(1125*2)
#define VBP 	41
#define VFP 	(VT/2 - INPUTY - VBP)
#define VPSW 	5

#ifdef  AUDIO_32K

#define CTS 	74250
#define ACRN 	4096

#endif

#ifdef  AUDIO_44K

#define CTS 	82500
#define ACRN 	6272

#endif


#ifdef  AUDIO_48K

#define CTS 	74250
#define ACRN 	6144

#endif

#ifdef  AUDIO_192K

#define CTS 	74250
#define ACRN 	24576

#endif

#endif


///////////////////////////////////////////////////
#ifdef PIX_13M5

#define VIDEO_PLL				108
#define CLK_DIV    				3			/*div = 4*/

#endif


#ifdef PIX_27M

#define VIDEO_PLL				108
#define CLK_DIV    				3			/*div = 4*/

#endif


#ifdef PIX_745M

#define VIDEO_PLL				297
#define CLK_DIV    				3			/*div = 4*/

#endif

#ifdef PIX_148M

#define VIDEO_PLL				297
#define CLK_DIV    				1			/*div = 2*/

#endif

#define  clkdiv 	(CLK_DIV+ 1)	//must be even

void HDMI_SET(void);
#endif


