/****************************************************************************/
/* MODULE:                                                                  								*/
/*	Globals.H 
*/	 
/****************************************************************************/
/*
 *   TCC Version 0.0
 *   Copyright (c) telechips, Inc.
 *   ALL RIGHTS RESERVED
*/
/****************************************************************************/

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

//#include "include\config.h"
#include "buffer.h"
//#include "fs/fsapp.h"
#include "disk.h"

//****************************************************************************
// The following values define the flags contained in the system flags
// variable.
//****************************************************************************
#define SYSFLAG_KEYBOARD_COUNT_MASK             0x00000007
#define SYSFLAG_PROCESS_OUTPUT                  0x00000008
#define SYSFLAG_START_DOWNLOAD                  0x00000010
#define SYSFLAG_INPUT_ENABLED                   0x00000020
#define SYSFLAG_OUTPUT_ENABLED                  0x00000040
#define SYSFLAG_MEDIA_INSERT                    0x00000080
#define SYSFLAG_MEDIA_REMOVE                    0x00000100
#define SYSFLAG_REPEAT_MASK                     0x00000600
#define SYSFLAG_REPEAT_OFF                      0x00000000
#define SYSFLAG_REPEAT_ONE                      0x00000200
#define SYSFLAG_REPEAT_ALL                      0x00000400
#define SYSFLAG_REPEAT_SHIFT                    9


//****************************************************************************
// The following values are the IOCTLs which are sent to the individual codec
// drivers.
//****************************************************************************
enum
{
    SUBFN_CODEC_GETNAME,
    SUBFN_CODEC_GETARTIST,
    SUBFN_CODEC_GETTITLE,
    SUBFN_CODEC_GETBITRATE,
    SUBFN_CODEC_GETSAMPLERATE,
    SUBFN_CODEC_GETCHANNELS,
    SUBFN_CODEC_GETLENGTH,
    SUBFN_CODEC_GETTIME,
    SUBFN_CODEC_OPEN_DEC,
    SUBFN_CODEC_OPEN_ENC,
    SUBFN_CODEC_GETCAPTUREBUFFER,
    SUBFN_CODEC_SETBUFFER,
    SUBFN_CODEC_DECODE,
    SUBFN_CODEC_ENCODE,
    SUBFN_CODEC_SEEK,
#ifdef AUDIBLE_INCLUDE
	SUBFN_CODEC_SETTIME,
	SUBFN_CODEC_GETINFO,
	SUBFN_CODEC_CURRCHAPTER,
	SUBFN_CODEC_TOTALCHAPTER,
#endif
    SUBFN_CODEC_CLOSE
};

//****************************************************************************
// The following are the flags passed to CodecOpen.
//****************************************************************************
#define CODEC_OPEN_ENCODE                       0x00000001
#define CODEC_OPEN_DECODE                       0x00000002
#define CODEC_OPEN_PLAYLIST						0x00000004

//****************************************************************************
// The following are the flags passed to CodecSeek.
//****************************************************************************
#define CODEC_SEEK_FF                       0x00000000
#define CODEC_SEEK_REW                      0x00000001
#define CODEC_SEEK_OTHER                    0x00000002
#ifdef AUDIBLE_INCLUDE
#define CODEC_SEEK_SECTIONFF				0x00000003
#define CODEC_SEEK_SECTIONREW				0x00000004
#endif
//****************************************************************************
// Function prototypes and global variables.
//****************************************************************************

// pMP3.C
extern unsigned long MP3Function(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2,unsigned long ulParam3,
								 unsigned long ulParam4);

// pMPEG4Audio.C	MPEG4 Audio Decode
extern unsigned long MPEG4AudioDecodeFunction(unsigned long ulSubFn, unsigned long ulParam1, unsigned long ulParam2,
								unsigned long ulParam3, unsigned long ulParam4);

// pMPEG4Video.C	MPEG4 Video Decode
extern unsigned long MPEG4VideoDecodeFunction(unsigned long ulSubFn, unsigned long ulParam1, 
								unsigned long ulParam2, unsigned long ulParam3, unsigned long ulParam4);

// pWMVAudio.C	WMV Audio Decode
extern unsigned long WMVAudioDecodeFunction(unsigned long ulSubFn, unsigned	long ulParam1, unsigned	long ulParam2,
								unsigned long ulParam3,	unsigned long ulParam4);

// pWMVVideo.C	WMV Video Decode
extern unsigned long WMVVideoDecodeFunction(unsigned long ulSubFn, unsigned long ulParam1, 
								unsigned long ulParam2, unsigned long ulParam3, unsigned long ulParam4);

// pWMA.C
extern unsigned long WMAFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2,unsigned long ulParam3,
								 unsigned long ulParam4);

// pMP3.C
extern unsigned long MP2Function(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2,unsigned long ulParam3,
								 unsigned long ulParam4);

// pVOBIS.C
extern unsigned long OGGFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2,unsigned long ulParam3,
								 unsigned long ulParam4);

// pJPEG.C
extern unsigned long JPGFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2,unsigned long ulParam3,
								 unsigned long ulParam4);
// pText.C
extern unsigned long TEXTFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2,unsigned long ulParam3,
								 unsigned long ulParam4);

// pAudible.C
#ifdef AUDIBLE_INCLUDE
extern unsigned long AUDFunction(unsigned long ulSubFn, unsigned long ulParam1, unsigned long ulParam2,
								unsigned long ulParam3, unsigned long ulParam4);
#endif

// pMp3Enc.C
extern unsigned long MP3MP4AudioEncodeFunction(unsigned long ulSubFn, unsigned long ulParam1, unsigned long ulParam2,
								unsigned long ulParam3, unsigned long ulParam4);

// pWMAEnc.C
extern unsigned long WMAEncFunction(unsigned long ulSubFn, unsigned long ulParam1,
									unsigned long ulParam2,	unsigned long ulParam3,
									unsigned long ulParam4);

#ifdef ADPCM_INCLUDE
// pMSADPCM.C
extern unsigned long MSADPCMFunction(unsigned long ulSubFn, unsigned long ulParam1, 
									unsigned long ulParam2,unsigned long ulParam3, 
									unsigned long ulParam4);
#endif
// pWAV.C
extern unsigned long PCMFunction(unsigned long ulIoctl, unsigned long ulParam1,
								unsigned long ulParam2, unsigned long ulParam3,        
								unsigned long ulParam4);

/*
// From aac.c
extern unsigned long AACFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2, unsigned long ulParam3,
								 unsigned long ulParam4);
*/

// From pBSAC.c
extern unsigned long BSACFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2, unsigned long ulParam3,
								 unsigned long ulParam4);

// From pLocalBASAC.c
extern unsigned long LocalBSACFunction(unsigned long ulSubFn, unsigned long ulParam1, unsigned long ulParam2,
								unsigned long ulParam3, unsigned long ulParam4);
								

// From pAACPlus.c
extern unsigned long AACPlusFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2, unsigned long ulParam3,
								 unsigned long ulParam4);

// From pAACDecode.c
extern unsigned long AACDecodeFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2, unsigned long ulParam3,
								 unsigned long ulParam4);

// pMpeg4VideoEncode.C
extern long MPEG4VideoEncodeFunction(unsigned long ulSubFn, unsigned long ulParam1, unsigned long ulParam2,
								unsigned long ulParam3, unsigned long ulParam4);

// pH264Dec.c	H.264 Video
extern unsigned long H264DecodeFunction(unsigned long ulSubFn,  unsigned long ulParam1, 
								 unsigned long ulParam2, unsigned long ulParam3, 
								 unsigned long ulParam4);

// pLocalH264.c
extern unsigned long LocalH264Function(unsigned long ulSubFn, unsigned long ulParam1, 
								 unsigned long ulParam2, unsigned long ulParam3, unsigned long ulParam4);

// pAACEnc.c
extern unsigned long AACEncodeFunction(unsigned long ulSubFn, unsigned long ulParam1,
								 unsigned long ulParam2, unsigned long ulParam3, unsigned long ulParam4);

// From codec.c
///////////////////////////////////////////////////////////////////////////////
extern unsigned long CodecOpen(unsigned int ulCodec, unsigned long ulFlags, int fhandle);
#if 0
extern unsigned long CodecGetIndex(unsigned long *pulIndex);
extern unsigned long CodecGetName(char **ppcName);
extern unsigned long CodecGetNameByIndex(unsigned long ulIndex, char **ppcName);
extern unsigned long CodecGetArtist(char **ppcName);
extern unsigned long CodecGetTitle(char **ppcName);
extern unsigned long CodecGetBitRate(unsigned long *pulBitRate);
#endif
extern unsigned long CodecGetSampleRate(unsigned int ulCodec, unsigned long *pulSampleRate);
extern unsigned long CodecGetChannels(unsigned int ulCodec, unsigned long *pulChannels);
extern unsigned long CodecGetLength(unsigned int ulCodec, unsigned long *pulLength);
extern unsigned long CodecGetTime(unsigned int ulCodec, unsigned long *pulTime);
extern unsigned long CodecGetCaptureBuffer(unsigned int ulCodec, short **ppsBuffer, long *plLength);
extern unsigned long CodecSetBuffer(unsigned int ulCodec, BufferState *psBuffer);
extern unsigned long CodecDecode(unsigned int ulCodec, int fhandle);
extern unsigned long CodecEncode(unsigned int ulCodec, int fhandle);
extern unsigned long CodecSeek(unsigned int ulCodec, unsigned long ulTime, unsigned long ulSeekType, int fhandle);
extern unsigned long CodecClose(unsigned int ulCodec, int fhandle);
extern unsigned long CodecGetBitrate(unsigned int ulCodec, unsigned long *pulBitrate);
#ifdef AUDIBLE_INCLUDE
extern unsigned long CodecAudibleInfo(unsigned int ulCodec, int *fHandle, unsigned long *CodecData);
extern unsigned long CodecSetCurrentTime(unsigned int ulCodec);
#endif


// using for multi codec
///////////////////////////////////////////////////////////////////////////////
extern unsigned long MultiCodecOpen(unsigned long ulCodec, unsigned long ulFlags, int fhandle);
extern unsigned long MultiCodecDecode(int fhandle);
extern unsigned long MultiCodecSeek(unsigned long ulTime, int fhandle);
#if 0
extern unsigned long MultiCodecGetTime(unsigned long *pulTime);
extern unsigned long MultiCodecGetLength(unsigned long *pulLength);
extern unsigned long MultiCodecSetBuffer(BufferState *psBuffer);
#endif
extern unsigned long MultiCodecClose(int fhandle);


// From dai.c
///////////////////////////////////////////////////////////////////////////////
extern void DAIInit(void);
extern void NormalInit(void);
extern void DAIEnable_Play(void);
extern void DAIEnable_Record(void);
extern void DAIDisable(void);
extern BufferState *DAIGetPlayBuffer(void);
extern BufferState *DAIGetRecordBuffer(void);
extern void	ResetOutputBuffer(void);

// From i2c.c
///////////////////////////////////////////////////////////////////////////////
//extern void I2CWrite(unsigned char ucAddr, unsigned char ucReg,
 //                    unsigned char ucValue);



// From input.c
///////////////////////////////////////////////////////////////////////////////
extern unsigned long InputEnable(long lSampleRate, short *psBuffer, long lLength);
extern void			 InputDisable(void);
extern BufferState*	 InputGetOutputBuffer(void);


// From irq.s
///////////////////////////////////////////////////////////////////////////////
//extern void EnableINT(void);
//extern void DisableINT(void);
//extern void DisableFIQ(void);
//extern void EnableFIQ(void);
//extern void DisableIRQ(void);
//extern void EnableIRQ(void);
//extern void DisableTimer(void);
//extern void EnableTimer(void);
extern void CleanCACHE(void);

// From output.c
///////////////////////////////////////////////////////////////////////////////
extern void OutputInit(void);
extern void OutputEnable(void);
extern void OutputDisable(void);
extern BufferState *OutputGetInputBuffer(void);
extern unsigned long OutputSetFormat(long lSampleRate, long lChannels);
extern void OutputSetRate(long lInputSampleRate, long bIsMono);
//extern void OutputSetTone(long lTreble, long lBass, long lGain);
//extern void OutputSetTreble(long lTreble);
//extern void OutputSetBass(long lBass);
//extern void OutputSetGain(long lGain);
//extern void OutputSetVolume(long lGain);
extern void OutputSetWidth(long lWidth);
extern void OutputSetTruBass(long lBass);
extern void OutputSetWow(long lBass);
//extern void OutputSetQSoundMode(long lMode);
extern void SRS_SetSamplingFreq(unsigned int SamplingFreq);

// From power.s
///////////////////////////////////////////////////////////////////////////////
extern void Standby(void);
extern void Halt(unsigned uCondition);


// From support.s
///////////////////////////////////////////////////////////////////////////////
extern void *memcpy(void *, const void *, unsigned int);
extern void *memmove(void *, const void *, unsigned int);
extern int  memcmp(const void *, const void *, unsigned int);
extern int  strcmp(const char *, const char *);
extern int  strncmp(const char *, const char *, unsigned int);
extern char *strcpy(char *, const char *);
extern char *strcat(char *, const char *);
extern void *memset(void *, int , unsigned int);
extern unsigned int  strlen(const char *);

extern unsigned	fmemcpy16(void *dest, void *src, unsigned length);

// From vectors.s
///////////////////////////////////////////////////////////////////////////////


// Codec Library Loader
///////////////////////////////////////////////////////////////////////////////
//extern void DynamicLoadADPCM(void);
extern void DynamicLoadMP3DEC(void);
extern void DynamicLoadMP3ENC(void);
extern void DynamicLoadWMADEC(void);
extern void DynamicLoadMP2DEC(void);
extern void DynamicLoadOGGDEC(void);
extern void DynamicLoadBSACDEC(void);
extern void DynamicLoadWMAENC(void);
extern void DynamicLoadingJDRM2SDRAM(void);	// JANUS Library
extern unsigned long JDRM_MakeDC(void);
extern void DynamicLoadCodecH264(void);
extern void DynamicLoadCodecmemH264(void);
#ifdef _TCC7801_
extern void DynamicLoadCodecWMV(void);
extern void DynamicLoadCodecmemWMV_C(void);
extern void DynamicLoadCodecmemWMV_NC(void);
#endif

enum {
	NOTWORK=0,
	DECODING,
	ENCODING,
	PAUSE
};

enum {
	BIDLE=0,
	BPLAY,
	BRADIO,
	BRECORD,
	BPLAYPAUSE,
	BRECPAUSE,
	BDMB,
	BCAPTURE,
	BCAMREC,
	NumOfBasicMode
};

enum {
	VIDLE=0,
	VPLAY,
	VRADIO,
	VRECORD,
	VMODESELECT,
	VBROWSING,
	VMENU,
	VEQAPP
};

/*
enum {
	LCDFUNCTIONCLASS1=0,
	LCDFUNCTIONCLASS2,
	LCDFUNCTIONCLASS3,
//	LCDFUNCTIONCLASS4
};
*/


enum {
	CODEC_MP3=0,

#ifdef MTX_INCLUDE
	CODEC_MTX,
#endif
#ifdef 	WMA_INCLUDE
	CODEC_WMA,
#endif
	CODEC_MP2,
#ifdef 	OGG_INCLUDE
	CODEC_OGG,
#endif
#ifdef WAV_INCLUDE
	CODEC_WAV,
#endif
#ifdef BSAC_INCLUDE
	CODEC_BSAC,
#endif
#ifdef AACPLUS_INCLUDE
	CODEC_AACP,
#endif
#ifdef 	MP4_DEC_INCLUDE
	CODEC_M4A_DEC,		// MPEG4 Audio Decode
#endif
#ifdef 	WMV_INCLUDE
	CODEC_WMV_AUD,		// WMV Audio Decode
#endif
#ifdef 	TES_INCLUDE
	CODEC_TES_AUD,		// WMV Audio Decode
#endif
#ifdef 	CAM_INCLUDE
	CODEC_CAM_AUD,		// AAC-LC Audio Decode
#endif
#if defined(JPG_ENC_INCLUDE)||defined(JPG_DEC_INCLUDE)
	CODEC_JPG,
#endif
#if defined (TXT_DEC_INCLUDE)
	CODEC_TEXT,
#endif
#ifdef AUDIBLE_INCLUDE
	CODEC_AUDIBLE,
#endif	
#ifdef	M3U_INCLUDE
	CODEC_M3U,
#endif
#ifdef ADPCM_INCLUDE
	CODEC_ADPCM,
#endif
#ifdef MP3_ENC_INCLUDE
	CODEC_MP3_ENC,
#endif
#ifdef WMA_ENC_INCLUDE
	CODEC_WMA_ENC,
#endif
#ifdef 	MP4_DEC_INCLUDE
	CODEC_M4V_DEC,		// MPEG4 Video Decode
#endif
#ifdef 	WMV_INCLUDE
	CODEC_WMV_VID,		// WMV Video Decode
#endif
#ifdef 	TES_INCLUDE
	CODEC_TES_VID,		// WMV Video Decode
#endif
#ifdef 	CAM_INCLUDE
	CODEC_CAM_VID,		// MPEG4 Video Decode
#endif
#ifdef 	MP4_ENC_INCLUDE
	#if (defined(CAM_ENC_INCLUDE) && defined(AAC_ENC_INCLUDE))
		CODEC_AAC_ENC,
	#elif (defined(MP4_ENC_INCLUDE))
		CODEC_M4A_ENC,		// MPEG4 Audio Encode
	#endif
	CODEC_M4V_ENC,		// MPEG4 Video Encode
#endif
#ifdef H264_INCLUDE
	CODEC_H264,
#endif
	NUMCODECS
};

typedef struct
{
	unsigned	uFpll;
	unsigned	uSpeed;
} sSpeedGrade;
#define	SPEEDGRADE_NUM		10
#define	SPEEDGRADE_MAX		(SPEEDGRADE_NUM-1)
#define	SPEEDGRADE_NORMAL	(5)
extern sSpeedGrade	gSpeedGrade[][SPEEDGRADE_NUM];

#ifdef	BATTERY_INCLUDE
typedef struct
{
    unsigned char		nBatteryCheck;
    unsigned char		battery;
    unsigned int		batteryValue;
    unsigned int		PrevbatteryValue;
    unsigned int		batterySum;
    unsigned int		drawOption;
}   tagSysInfoType;

#define	DRAW_BATTERY			0x00000001
#define	DRAW_BATTERY_BLINK		0x00000002

#endif	//BATTERY_INCLUDE

#define VOLUME_MAX				40
#define REC_VOLUME				0xEB		// input value == 235, volume level == 30
#define MAX_ROMSIZE     		0x100000
#define	MAX_ROMSIZE_NAND		0x200000

#define SIZE_OF_AUDIO_BUFFER	0x4000

extern unsigned short*	GetWMAHeader(unsigned int ulPos);
extern int 				SaveConfigData(unsigned char* ucSaveBuffer);
extern int 				LoadConfigData(unsigned char* ucLoadBuffer);
extern void 			mDelay_5mS(unsigned int input);
extern void 			Main_SearchExtDrv(void);
extern void 			ModeIdleInit(void);
extern void 			ModeRadioInit(void);
extern void				RADIO_TurnOff(void);
extern void 			SetJPGScreen(void);
extern unsigned char 	UART_KeyEnable(void);
extern unsigned char 	UART_GetKeyType(void);
extern unsigned char 	UART_GetKeyNum(void);
extern int 				FWUG_Main(unsigned int Drv,char *fname,char ClassMode);
extern void 			WriteCode(unsigned char *rombuf, unsigned int uiBufSize, unsigned int nFWpage, unsigned char *__pBitMapFont);
extern void 			DoReset(void);	
extern void 			PowerDownProcess(void);
extern unsigned int 	Initialize_GetDeviceStatus(int iDeviceNum,int iUSBMode,int iExtDevice);
extern void 			Initialize_MountSystem(void);
extern int 				RepeatOpenTrack(void);


extern unsigned char	CodecMode;
extern unsigned char	BasicMode;
extern unsigned char	VisualMode;
extern unsigned char 	OpenFileFlag;
extern unsigned int		CurrentFileNum;
extern unsigned int		TotalMusicFileNum;
extern unsigned char	CHECK_TIMER_INT;
extern unsigned char	CHECK_TIMER_BATTERY;
extern unsigned char	HoldFlag;
extern unsigned char 	SpeedVal;
extern unsigned char	ABFlag;
extern unsigned int		ABMusic[2];
extern unsigned long	ABTime[2];
extern unsigned char 	fEnableResume;
extern unsigned char	ValidMusicFlag;
extern unsigned char	gPlugInSkip;
extern unsigned char	gStartRec;
extern int 				gCurrentDisk;
extern int 				g_Part_ID;
extern unsigned long 	ulEndOfRAM;
extern unsigned char	ucSzSImageBuff[];
extern char		cStartDec;
extern unsigned char gVideoCodecArea[];
extern unsigned int	gMAX_ROMSIZE;
extern unsigned char usbFirmwareDownloadMode;

//****************************************************************************
// Audio Information 
//****************************************************************************
//****************************************************************************
extern unsigned char Audio_GetCurrentCodec(void);
extern int	Audio_GetNumOfChannels(void);
extern int	Audio_GetSamplingRate(void);
extern int	Audio_GetBitRate(void);
extern int	Audio_GetPlayTime(void);

extern void	Audio_SetCurrentCodec(unsigned char ucCodec);
extern void	Audio_SetNumOfChannels(unsigned char ucChannel);
extern void	Audio_SetSamplingRate(unsigned int uiSamplingRate);
extern void	Audio_SetBitRate(unsigned int uiBitrate);
extern void	Audio_SetPlayTime(unsigned int uiDecodeTime);
#endif	/* __GLOBALS_H__ */

/* end of file */

