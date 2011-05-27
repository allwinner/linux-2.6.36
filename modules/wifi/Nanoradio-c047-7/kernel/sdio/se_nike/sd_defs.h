/// Nanoradio 
/// Samsung NIKE platform

#include <asm-arm/arch-s5c73xx/hardware.h>

/// BASE 
//#define SDHC_SDMMC0_BASE S5C73XX_VA_SDMMC // 0x4E000000
//#define SDHC_SDMMC1_BASE S5C73XX_VA_SDMMC1// 0x4E800000
#define SDHC_SDMMC0_BASE  0x4E000000
#define SDHC_SDMMC1_BASE  0x4E800000


/// todo : choose with ifdef what poet using.
#define SDHC_SDMMC_BASE SDHC_SDMMC0_BASE 

/// offset for registers
#define SDHC_SYS_ADDR                   0x00	//SDI control register
#define SDHC_BLK_SIZE                   0x04	//Host buffer boundary and transfer block size register
#define SDHC_BLK_COUNT                  0x06	//Block count for current transfer
#define SDHC_ARG                        0x08	//Command Argument
#define SDHC_TRANS_MODE                 0x0C	//Transfer Mode setting register
#define SDHC_COMMAND                    0x0E	//Command register
#define SDHC_RSP01                      0x10	//Response 0					//32bit
#define SDHC_RSP23                      0x14	//Response 1					//32bit
#define SDHC_RSP45                      0x18	//Response 2					//32bit
#define SDHC_RSP67                      0x1C	//Response 3					//32bit
#define SDHC_BUF_DAT_PORT               0x20	//Buffer Data register
#define SDHC_PRESENT_STAT               0x24	//Present state					//32bit
#define SDHC_HOST_CTRL                  0x28	//Host control					//8bit
#define SDHC_PWR_CTRL                   0x29	//Power control					//8bit
#define SDHC_BLOCKGAP_CTRL              0x2A	//Block Gap control				//8bit
#define SDHC_WAKEUP_CTRL                0x2B	//Wakeup control				//8bit
#define SDHC_CLK_CTRL                   0x2C	//Clock control					
#define SDHC_TIMEOUT_CTRL               0x2E	//Time out control				//8bit
#define SDHC_SOFTWARE_RESET             0x2F	//Software reset				
#define SDHC_NORMAL_INT_STAT            0x30	//Normal interrupt status 		
#define SDHC_ERROR_INT_STAT             0x32	//Error interrupt status  		
#define SDHC_NORMAL_INT_STAT_ENABLE     0x34	//Normal interrupt status enable
#define SDHC_ERROR_INT_STAT_ENABLE      0x36	//Error interrupt status enable	
#define SDHC_NORMAL_INT_SIGNAL_ENABLE   0x38	//Normal interrupt signal enable
#define SDHC_ERROR_INT_SIGNAL_ENABLE    0x3A	//Error interrupt signal enable	
#define SDHC_AUTO_CMD12_ERR_STAT        0x3C	//Auto CMD12 Error Status		
#define SDHC_CAPA                       0x40	//Capability					//32bit
#define SDHC_MAX_CURRENT_CAPA           0x48	//Maximum current Capability	//32bit
#define SDHC_CONTROL2                   0x80	//Control 2						//32bit
#define SDHC_CONTROL3                   0x84	//Control 3						//32bit
#define SDHC_HOST_CONTROLLER_VERSION    0xFE	//Host controller version   	

// GPIO
#define GPIO_ADRS_BASE			S5C73XX_VA_GPIO

#define GPIO_CON_L			(GPIO_ADRS_BASE + 0x00) 
#define GPIO_CON_H			(GPIO_ADRS_BASE + 0x04) 
#define GPIO_DOUT_L			(GPIO_ADRS_BASE + 0x08) 
#define GPIO_DOUT_H			(GPIO_ADRS_BASE + 0x0C) 
#define GPIO_SEL_L			(GPIO_ADRS_BASE + 0x34) 
#define GPIO_SEL_H			(GPIO_ADRS_BASE + 0x38) 
#define GPIO_DIN_L			(GPIO_ADRS_BASE + 0x3C) 
#define GPIO_DIN_H			(GPIO_ADRS_BASE + 0x40) 

#define GPIO_P15			0x00008000
#define GPIO_P16			0x00010000
#define GPIO_P17			0x00020000
#define GPIO_P18			0x00040000
#define GPIO_P42			0x00000400
#define GPIO_P46			0x00004000

//=====================================================
//  SD/MMC SELECT DISCRIMINATION
//=====================================================
// void sd_bus_power_onoff(SINT onoff)
#define SDAL_BUS_POWER_OFF		0
#define SDAL_BUS_POWER_ON		1

// void sd_clock_onoff( SINT onoff)
#define SDAL_CARD_CLOCK_OFF		0
#define SDAL_CARD_CLOCK_ON		1

// sd_select_bit_width(sel)
#define SDAL_BUS_WIDTH_1BIT		0x00
#define SDAL_BUS_WIDTH_4BIT		0x02
#define SDAL_BUS_WIDTH_8BIT		0x20

// sd_change_clk_src(src)
#define SDAL_CLOCK_SRC_EPLL		0 // 48   MHz
#define SDAL_CLOCK_SRC_HCLK		1 // system clock
#define SDAL_CLOCK_SRC_XTI		2 // external

//=====================================================
//  SD/MMC CARD DISCRIMINATION
//=====================================================
//sdhndl->ResponseType
#define SDAL_RESP_NONE		0
#define SDAL_RESP_TYPE1		1
#define SDAL_RESP_TYPE1B	2
#define SDAL_RESP_TYPE2		3
#define SDAL_RESP_TYPE3		4
#define SDAL_RESP_TYPE6		5
#define SDAL_RESP_TYPE7		6
#define SDAL_RESP_TYPE_SDIO	7

// sdhndl->Detect
#define SDAL_CARD_REMOVE	0x00	// card removed
#define SDAL_CARD_INSERT	0x01	// card inserted

// sdhndl->CardType
#define SDAL_CARD_UNKNOWN			0x0000	// Other Medias out of SD Standard
#define SDAL_CARD_MMC				0x0010	// MultiMedia Card
#define SDAL_CARD_SD				0x0020	// SD Memory Card
#define SDAL_CARD_BYTE_ACCESS		0x0001	// SDMMC Byte Access Indentifier
#define SDAL_CARD_SECTOR_ACCESS		0x0002	// SDMMC Sector Access Indentifier
#define SDAL_CARD_MMC_BYTE_ACCESS	(SDAL_CARD_MMC|SDAL_CARD_BYTE_ACCESS)
#define SDAL_CARD_MMC_SECTOR_ACCESS	(SDAL_CARD_MMC|SDAL_CARD_SECTOR_ACCESS)
#define SDAL_CARD_SD_BYTE_ACCESS	(SDAL_CARD_SD|SDAL_CARD_BYTE_ACCESS)
#define SDAL_CARD_SD_SECTOR_ACCESS	(SDAL_CARD_SD|SDAL_CARD_SECTOR_ACCESS)
#define SDAL_CARD_SD_ROM			0x0024	// SD Memory Card (ROM)
#define SDAL_CARD_SDIO				0x0040	// SDIO Card (unsupported)
#define SDAL_CARD_COMBO				0x0080	// SD and SDIO (unsupported)

// sdhndl->IfMode
#define SDAL_MODE_SDMC		0		// SD/MC Mode (default)
#define SDAL_MODE_SPI		1		// SPI mode (unsupported)

// sdhndl->Wp
#define SDAL_WRITE_ENABLE	0		// write enable (default)
#define SDAL_WRITE_DISABLE	1		// write protected

// sdhndl->RetryCnt
#define SDAL_RETRY_COUNT	6

// sdhndl->BlockSize
#define SDAL_DEFAULT_SIZE	0x0200	// 512Byte

// sdhndl->CapaIdx
#define SDAL_CAPA_IDX_4M	0
#define SDAL_CAPA_IDX_8M	1
#define SDAL_CAPA_IDX_16M	2
#define SDAL_CAPA_IDX_32M	3
#define SDAL_CAPA_IDX_64M	4
#define SDAL_CAPA_IDX_128M	5
#define SDAL_CAPA_IDX_256M	6
#define SDAL_CAPA_IDX_512M	7
#define SDAL_CAPA_IDX_1G	8
#define SDAL_CAPA_IDX_2G	9
#define SDAL_CAPA_IDX_4G	10
#define SDAL_CAPA_IDX_8G	11
#define SDAL_CAPA_IDX_16G	12
#define SDAL_CAPA_IDX_32G	13

// sdhndl->FormatType
#define SDAL_FORMAT_MBR		0
#define SDAL_FORMAT_PBR		1

// sdhndl->Error (ERROR CODE DEFINE)
#define SDAL_ERR_NONE				0x00000000
#define SDAL_ERR_CARD_EJECT			0x00000001	// during process, card is out
#define SDAL_ERR_SW_TIMEOUT			0x00000002	// sw timeout
#define SDAL_ERR_BUS_WIDTH			0x00000004	// bus width error
#define SDAL_ERR_UNKNOWN_CARD		0x00000008	// unknown card
#define SDAL_ERR_READONLY_CARD		0x00000010	// read only card
#define SDAL_ERR_CAPACITY_OUT		0x00000020	// capacity out card (under 128MB, over 32GB)
#define SDAL_ERR_NOT_SUPPORT		0x00000040	// not support card (mmc)
#define SDAL_ERR_SPEC_OUT			0x00000080	// spec out card (sd 1.1 4G, sd 2.0 high capa byte access)
#define SDAL_ERR_NEED_FORMAT		0x00000100	// various situations which need format
#define SDAL_ERR_RW_PARAMETER		0x00000200	// RW parameter error
#define SDAL_ERR_CARD_STATUS		0x00000400	// card status error
#define SDAL_ERR_AUTOCMD12_ERR		0x00000800	// AUTOCMD12 error
#define SDAL_ERR_CMDLINE_CONFLICT	0x00010000	// CMD line conflict (not high)
#define SDAL_ERR_RESPONSE_TIMEOUT	0x00020000	// response timeout error
#define SDAL_ERR_RESPONSE_CRC		0x00040000	// response CRC error
#define SDAL_ERR_CMD_ENDBIT			0x00080000	// command end bit error (not 1)
#define SDAL_ERR_CMD_INDEX			0x00100000	// command index error
#define SDAL_ERR_DATA_TIMEOUT		0x01000000	// data timeout error
#define SDAL_ERR_DATA_CRC			0x02000000	// data CRC error
#define SDAL_ERR_DATA_ENDBIT		0x04000000	// data end bit error (not 1)
#define SDAL_ERR_DATA_REMAIN		0x08000000	// all data not transferred

//=====================================================
//  FUNCTION RETURN CODE DEFINE (value is provisional)
//=====================================================
#define SDR_OK					0	// normal
#define SDR_ERR					-1	// error
#define SDR_NOT_SUPPORT			-2	// not support message
#define SDR_NEED_FORMAT			-3	// need format message
#define SDR_WRITE_PROTECTED		-4	// cannot write data

//=====================================================
//	SD Clock Value
//=====================================================
#define CLOCK_IDENTIFICATION	0	// for identification of SD/MMC
#define CLOCK_NORMAL_SD			1	// for operation of normal SD card
#define CLOCK_NORMAL_MMC		2	// for operation of normal MMC
#define CLOCK_HIGH_SD			3
#define CLOCK_HIGH_MMC_26		4
#define CLOCK_HIGH_MMC_52		5

//=====================================================
//   TIMEOUT DISCRIMINATION
//=====================================================
// sd_wait()CLOCK
#define SD_POWUP_WAIT		250		// 250ms : bus power up time
#define SD_RAMPUP_WAIT		1		//   1ms : supply ramp up time
#define SD_CLK_CHANGE_WAIT	2		//   2ms : clock change time
#define SD_RESET_WAIT		100		//  100ms: Reset time After CMD0
									//		   change 10ms to 100ms for
									//		   Mount RiData 1G Normal SD ver1.1
#define SD_WAIT_1MS			1

// sd_start_timer()
#define SD_RESET_TIMEOUT	1500	// 1500ms
#define SD_INITIAL_TIMEOUT	800		// 800ms - card mount
#define SD_PREPARE_TIMEOUT	1000	// 1000ms - checking before read/write access
#define SD_RW_TIMEOUT		2000	// 2000ms - read/write access
									// change from 1s to 2s for BUFFALO 16G class 4 (2008.03.26)
#define SD_ERASE_TIMEOUT	10000	// 10s - erase when format
#define SD_FORMAT_TIMEOUT	60000	// 60s - format
	
//=====================================================
//  R1 Card Status definition
//
//  note: See 'SD Physical Layer SpecificationV2.00 (Part1)' and
//            'MultiMediaCard System Specification 4.2' for details.
//=====================================================
#define SD_R1_OUT_OF_RANGE		0x80000000	// [31] The argument was out of the allowed range
#define SD_R1_ADDRESS_ERROR		0x40000000	// [30] command addresss is misaligned
#define SD_R1_BLOCK_LEN_ERROR		0x20000000	// [29] The transferred block length error
#define SD_R1_ERASE_SEQ_ERROR		0x10000000	// [28] erase command sequence error
#define SD_R1_ERASE_PARAM		0x08000000	// [27] erase command argument error
#define SD_R1_WP_VIOLATION		0x04000000	// [26] illegal write attempt to write-protected block
#define SD_R1_CARD_IS_LOCKED		0x02000000	// [25] card is locked by the host
#define SD_R1_LOCK_UNLOCK_FAILED	0x01000000	// [24] lock/unlock sequence or password error
#define SD_R1_COM_CRC_ERROR		0x00800000	// [23] previous command CRC error
#define SD_R1_ILLEGAL_COMMAND		0x00400000	// [22] command not legal for the card state
#define SD_R1_CARD_ECC_FAILED		0x00200000	// [21] card internal ECC correction fail
#define SD_R1_CC_ERROR			0x00100000	// [20] internal card controller error
#define SD_R1_ERROR			0x00080000	// [19] general or unknown error
#define SD_R1_UNDERRUN			0x00040000	// [18] (mmc only) card can't maintain data in read
#define SD_R1_OVERRUN			0x00020000	// [17] (mmc only) card can't maintain data in write
#define SD_R1_CID_CSD_OVERWRITE		0x00010000	// [16] illegal overwriting CID/CSD
#define SD_R1_WP_ERASE_SKIP		0x00008000	// [15] partial erase for wp block
#define SD_R1_CARD_ECC_DISABLE		0x00004000	// [14] (sd only) command without internal ECC
#define SD_R1_ERASE_RESET		0x00002000	// [13] erase aequence was cleard before executeing
#define SD_R1R6_CURRENT_STATE_FIELD	0x00001E00	// [12:9] card current state field
#define SD_R1R6_CURRENT_IDLE		0x00000000	//		0: idle state
#define SD_R1R6_CURRENT_READY		0x00000200	//		1: ready state
#define SD_R1R6_CURRENT_IDENT		0x00000400	//		2: identification state
#define SD_R1R6_CURRENT_STBY		0x00000600	//		3: stand-by state
#define SD_R1R6_CURRENT_TRAN		0x00000800	//		4: transfer state
#define SD_R1R6_CURRENT_DATA		0x00000A00	//		5: sending-data state
#define SD_R1R6_CURRENT_RCV		0x00000C00	//		6: receive-data state
#define SD_R1R6_CURRENT_PRG		0x00000E00	//		7: programming state
#define SD_R1R6_CURRENT_DIS		0x00001000	//		8: disconnect state
#define SD_R1_CURRENT_BTST		0x00001200	//		9: (mmc only) bus test state
#define SD_R1R6_READY_FOR_DATA		0x00000100	// [8] buffer empty signal
#define SD_R1_SWITCH_ERROR		0x00000080	// [7] (mmc olny) switch fail
#define SD_R1R6_APP_CMD			0x00000020  	// [5] card will expect ACMD
#define SD_R1R6_AKE_SEQ_ERROR		0x00000008  	// [3] (sd only) authentication process error

#define SD_R1R6_CURRENT_STATE_SHIFT	9
#define SD_R1_STATUS_ERR            	0xFDFF0088  	// R1 Status Error

//=====================================================
//  R6 Card Status definition for SD only
//
//  note: See 'SD Physical Layer SpecificationV2.00 (Part1)' and
//=====================================================
#define SD_R6_COM_CRC_ERROR			0x00008000	// [15] previous command CRC error
#define SD_R6_ILLEGAL_COMMAND		0x00004000	// [14] command not legal for the card state
#define SD_R6_ERROR					0x00002000	// [13] general or unknown error

#define SD_R6_STATUS_ERR            0x0000E008  // R1 Status Error

//=====================================================
//  SD/MMC OCR definition
//
//  note: See 'SD Physical Layer SpecificationV2.00 (Part1)' and
//            'MultiMediaCard System Specification 4.2' for details.
//=====================================================
#define SD_OCR_POWUP_COMPLETE		0x80000000	// [31] 0=card busy, 1=card ready
#define SD_OCR_SECTOR_ACCESS_MODE	0x40000000	// [30] 0=byte access, 1=sector access
#define SD_OCR_VDD_36_35		0x00800000	// [23] VDD voltage 3.6-3.5
#define SD_OCR_VDD_35_34		0x00400000	// [22] VDD voltage 3.5-3.4
#define SD_OCR_VDD_34_33		0x00200000	// [21] VDD voltage 3.4-3.3
#define SD_OCR_VDD_33_32		0x00100000	// [20] VDD voltage 3.3-3.2
#define SD_OCR_VDD_32_31		0x00080000	// [19] VDD voltage 3.2-3.1
#define SD_OCR_VDD_31_30		0x00040000	// [18] VDD voltage 3.1-3.0
#define SD_OCR_VDD_30_29		0x00020000	// [17] VDD voltage 3.0-2.9
#define SD_OCR_VDD_29_28		0x00010000	// [16] VDD voltage 2.9-2.8
#define SD_OCR_VDD_28_27		0x00008000	// [15] VDD voltage 2.8-2.7
#define SD_OCR_VDD_26_20		0x00007F00	// [14: 8] (mmc only) VDD voltage 2.6-2.0
#define SD_OCR_VDD_195_170		0x00000080	// [7] (mmc only) VDD voltage 1.95-1.70
#define SD_OCR_VDD_36_27		0x00FF8000	// [23:15] VDD voltage 3.6-2.7

//=====================================================
//  R7 Card Interface Condition (for SD card CMD08)
//
//  note: See 'SD Physical Layer SpecificationV2.00 (Part1)' 49 page
//=====================================================
#define SD_R7_VHS_36_27				0x00000100	// [19:16] VHS 3.6-2.7
#define SD_R7_VHS_LOW_VOL			0x00000200	//		   Low Voltage
#define SD_R7_CHECK_PATTERN			0x000000AA	// [15: 8] check pattern

///====================================================
//  HS-MMC EXT_CSD DISCRIPTION
//=====================================================
// sdhndl->EXTCSD.s_cmd_set
#define MM_EXTCSD_CMDSET_STD		0x01	// Standard MMC
#define MM_EXTCSD_CMDSET_SECU		0x02	// SecureMMC
#define MM_EXTCSD_CMDSET_SECU_CP	0x04	// Content Protection SecureMMC
#define MM_EXTCSD_CMDSET_SECU_20	0x08	// SecureMMC 2.0
#define MM_EXTCSD_CMDSET_ATA		0x10	// ATA on MMC
// sdhndl->EXTCSD.min_perf_*_**_**
#define MM_EXTCSD_MINPERF_UNDER_2_4	0x00	// under 2.4MB/s
#define MM_EXTCSD_MINPERF_CLASS_A	0x08	// Class A(2.4MB/s)
#define MM_EXTCSD_MINPERF_CLASS_B	0x0A	// Class B(3.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_C	0x0F	// Class C(4.5MB/s)
#define MM_EXTCSD_MINPERF_CLASS_D	0x14	// Class D(6.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_E	0x1E	// Class E(9.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_F	0x28	// Class F(12.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_G	0x32	// Class G(15.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_H	0x3C	// Class H(18.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_J	0x46	// Class J(21.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_K	0x50	// Class K(24.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_M	0x64	// Class M(30.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_O	0x78	// Class O(36.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_R	0x8C	// Class R(42.0MB/s)
#define MM_EXTCSD_MINPERF_CLASS_T	0xA0	// Class T(48.0MB/s)
// sdhndl->EXTCSD.power_class
#define MM_EXTCSD_PC_360_200		0
#define MM_EXTCSD_PC_360_220		1
#define MM_EXTCSD_PC_360_250		2
#define MM_EXTCSD_PC_360_280		3
#define MM_EXTCSD_PC_360_300		4
#define MM_EXTCSD_PC_360_320		5
#define MM_EXTCSD_PC_360_350		6
#define MM_EXTCSD_PC_360_400		7
#define MM_EXTCSD_PC_360_450		8
#define MM_EXTCSD_PC_360_500		9
#define MM_EXTCSD_PC_360_550		10

#define MM_EXTCSD_PC_195_130		0
#define MM_EXTCSD_PC_195_140		1
#define MM_EXTCSD_PC_195_160		2
#define MM_EXTCSD_PC_195_180		3
#define MM_EXTCSD_PC_195_200		4
#define MM_EXTCSD_PC_195_220		5
#define MM_EXTCSD_PC_195_240		6
#define MM_EXTCSD_PC_195_260		7
#define MM_EXTCSD_PC_195_280		8
#define MM_EXTCSD_PC_195_300		9
#define MM_EXTCSD_PC_195_350		10
// sdhndl->EXTCSD.card_type
#define MM_EXTCSD_HSMMC_26M			0x01	// High Speed MMC @ 26MHz
#define MM_EXTCSD_HSMMC_52M			0x02	// High Speed MMC @ 52MHz
// sdhndl->EXTCSD.csd_structure
#define MM_EXTCSD_CSD_VERNO_10		0		// Sepc 1.0-1.2
#define MM_EXTCSD_CSD_VERNO_11		1		// Sepc 1.4-2.2
#define MM_EXTCSD_CSD_VERNO_12		2		// Sepc 3.1-4.2
// sdhndl->EXTCSD.ext_csd_rev
#define MM_EXTCSD_EXTCSD_REV10		0		// Rev 1.0
#define MM_EXTCSD_EXTCSD_REV11		1		// Rev 1.1
#define MM_EXTCSD_EXTCSD_REV12		2		// Rev 1.2
// sdhndl->EXTCSD.hs_timing
#define MM_EXTCSD_TIMING_NORMAL		0
#define MM_EXTCSD_TIMING_HIGH		1		// high speed interface timing
// sdhndl->EXTCSD.bus_width
#define MM_EXTCSD_BUS_1BIT			0
#define MM_EXTCSD_BUS_4BIT			1
#define MM_EXTCSD_BUS_8BIT			2

//=====================================================
//  access for SWITCH Command (for HSMMC only)
//=====================================================
#define MM_CMD6_ACC_CMD_SET			0x00
#define MM_CMD6_ACC_SET_BITS		0x01
#define MM_CMD6_ACC_CLR_BITS		0x02
#define MM_CMD6_ACC_WRITE_BYTE		0x03
//=====================================================
//  Index(= EXTCSD byte offset) for SWITCH Command ( only for EXT_CSD are writable)
//=====================================================
#define MM_CMD6_IDX_CMD_SET			191
#define MM_CMD6_IDX_PWR_CLASS		187
#define MM_CMD6_IDX_HS_TIMING		185
#define MM_CMD6_IDX_BUS_WIDTH		183

//=====================================================
//  mode for SD switch command
//=====================================================
#define SD_CMD6_CHECK_MODE			0x00	// mode 0
#define SD_CMD6_SWITCH_MODE			0x80	// mode 1
//=====================================================
//  group for SD switch command
//=====================================================
#define SD_CMD6_FUNCTION_DEFAULT	0x00
#define SD_CMD6_FUNCTION_GROUP1		0x01
#define SD_CMD6_FUNCTION_GROUP2		0x02
#define SD_CMD6_FUNCTION_GROUP3		0x03
#define SD_CMD6_FUNCTION_GROUP4		0x04
#define SD_CMD6_FUNCTION_GROUP5		0x05
#define SD_CMD6_FUNCTION_GROUP6		0x06
//=====================================================
//  function for SD switch command
//=====================================================
#define SD_CMD6_FUNCTION0			0x00
#define SD_CMD6_HIGH_SPEED     		0x01	// Group 1
#define SD_CMD6_E_COMMERCE			0x02	// Group 2
#define SD_CMD6_VENDOR_SPECIFIC		0x0E	// Group 2

#define SD_CMD6_SUPPORT_HIGH_SPEED	0x0002	// data[51:50]

//=====================================================
//  SD SCR definition
//  note: See 'SD Physical Layer SpecificationV2.00 (Part1)' and
//            'MultiMediaCard System Specification 4.2' for details.
//=====================================================
#define SD_SCR_STRUCTURE_FIELD	0xF0000000	// bit[63:60]
#define SD_SCR_SPEC_1_10		0x01000000	// bit[59:56]
#define SD_SCR_SPEC_2_00		0x02000000
#define SD_SCR_SPEC_FIELD		0x0F000000
#define SD_SCR_SECURITY_FIELD	0x00700000	// bit[54:52]
#define SD_SCR_BUS_WIDTH_1		0x00010000  // bit[51:48]
#define SD_SCR_BUS_WIDTH_4		0x00040000
#define SD_SCR_BUS_WIDTH_FIELD	0x000F0000

//=====================================================
//  SD Status definition
//  note: See 'SD Physical Layer SpecificationV2.00 (Part1)' and
//            'MultiMediaCard System Specification 4.2' for details.
//=====================================================
#define SD_STS_BUS_WIDTH_1		0x00000000	// bit[511:510]
#define SD_STS_BUS_WIDTH_4		0x80000000
#define SD_STS_BUS_WIDTH_FIELD	0xC0000000
#define SD_STS_CARD_TYPE_SD_ROM 0x00000001	// bit[495:480]

//========================================================================================= registers field


/*=====================================================
    S5C7329 SD HOST CONTROLER REGISTER OF BIT FIELD
=====================================================*/
#define REG_CLEAR_32BIT					0x00000000
#define REG_CLEAR_16BIT					0x0000
#define REG_CLEAR_8BIT					0x00
#define SD_CMD_APP_CMD					0x4000	// flag for distinguishing APP CMD from NORMAL CMD

// Block Size Register (16bit)
#define BLKSIZE_SDMA_BUF_BOUNDARY_4K	0x0000	// Host SDMA Buffer Boundary - bit[14:12]
#define BLKSIZE_SDMA_BUF_BOUNDARY_8K	0x1000
#define BLKSIZE_SDMA_BUF_BOUNDARY_16K	0x2000
#define BLKSIZE_SDMA_BUF_BOUNDARY_32K	0x3000
#define BLKSIZE_SDMA_BUF_BOUNDARY_64K	0x4000
#define BLKSIZE_SDMA_BUF_BOUNDARY_128K	0x5000
#define BLKSIZE_SDMA_BUF_BOUNDARY_256K	0x6000
#define BLKSIZE_SDMA_BUF_BOUNDARY_512K	0x7000
#define BLKSIZE_SDMA_BUF_BOUNDARY_FIELD	BLKSIZE_SDMA_BUF_BOUNDARY_512K
#define BLKSIZE_TRANS_BLKSIZE_512B		0x0200	// Transfer Block Size - bit[11:0]
#define BLKSIZE_TRANS_BLKSIZE_64B		0x0040
#define BLKSIZE_TRANS_BLKSIZE_8B		0x0008
#define BLKSIZE_TRANS_BLKSIZE_4B		0x0004
#define BLKSIZE_TRANS_BLKSIZE_2B		0x0002
#define BLKSIZE_TRANS_BLKSIZE_FIELD		0x0FFF

// Transfer Mode Register (16bit)
#define TRANSMODE_NO_CCS_CON			0x0000	// Command Completion Signal Control - bit[13:12]
#define TRANSMODE_DATA_TRANS_CCS_EN		0x1000	// 		R/W Data Transfer CCS Enable
#define TRANSMODE_NO_DATA_CCS_EN		0x2000	// 		without Data Transfer CCS Enable
#define TRANSMODE_ACS_GENERATION		0x3000	// 		Abort Completion Signal(ACS) Generation
#define TRANSMODE_CCS_CON_FIELD			TRANSMODE_ACS_GENERATION
#define TRANSMODE_MULTIPLE_BLOCK		0x0020	// Multi Block Select - bit[5]
#define TRANSMODE_DIRECTION_READ		0x0010	// Data Transfer Direction - bit[4] : CARD->HOST
#define TRANSMODE_AUTO_CMD12_EN			0x0004	// Auto CMD12 Enable - bit[2]
#define TRANSMODE_BLOCK_COUNT_EN		0x0002	// Block Count Enable - bit[1]
#define TRANSMODE_DMA_EN				0x0001	// DMA Enable - bit[0]

// Command register (16bit)
#define CMD_INDEX_FIELD					0x3F00	// Command Index - bit[13:8]
#define CMD_TYPE_SUSPEND				0x0040	// Suspend Command Type - bit[7:6]
#define CMD_TYPE_RESUME					0x0080	// 		Resume Command Type
#define CMD_TYPE_ABORT					0x00C0	// 		Abort Command Type
#define CMD_TYPE_FIELD					CMD_TYPE_ABORT
#define CMD_DAT_PRESENT					0x0020	// Data Present - bit[5]
#define CMD_CHK_IDX_EN					0x0010	// Command Index Check Enable - bit[4]
#define CMD_CHK_CRC_EN					0x0008	// Command CRC Check Enable - bit[3]
#define CMD_RESP_136B_R2				(0x0001|CMD_CHK_CRC_EN) // Response Length 136Bit - bit[1:0]
#define CMD_RESP_48B_NOBUSY_R3R4		0x0002	// Response Length 48Bit with No Busy
#define CMD_RESP_48B_NOBUSY_R1R5R6R7	(CMD_RESP_48B_NOBUSY_R3R4|CMD_CHK_IDX_EN|CMD_CHK_CRC_EN)
#define CMD_RESP_48B_BUSY_R1BR5B		(0x0003|CMD_CHK_IDX_EN|CMD_CHK_CRC_EN) // Response Length 48Bit with Busy
#define CMD_RESP_FIELD					0x0003

// Present State register (32bit)
#define PRESTATE_CMD_SIG_LEVEL_HIGH		0x01000000	// CMD Line Signal Level - bit[24]
#define PRESTATE_DAT_SIG_LEVEL_FIELD	0x00F00000	// DAT[3:0] Line Signal Level - bit[23:20]
#define PRESTATE_NO_WRITE_PROTECT		0x00080000	// WP Switch Pin Level(invert) - bit[19]
#define PRESTATE_CARD_DETECT			0x00040000	// Card DET Pin Level (for Debugging) - bit[18]
#define PRESTATE_CARD_STABLE			0x00020000	// Card State Stable (for Debugging) - bit[17]
#define PRESTATE_CARD_INSERTED			0x00010000	// Card Inserted - bit[16]
#define PRESTATE_FIFO_PT_DIFF_4WORD		0x00002000	// FIFO Pointer Difference 4-Word - bit[13]
#define PRESTATE_FIFO_PT_DIFF_1WORD		0x00001000	// FIFO Pointer Difference 1-Word - bit[12]
#define PRESTATE_BUFFER_READ_EN			0x00000800	// Buffer Read Enable - bit[11]
#define PRESTATE_BUFFER_WRITE_EN		0x00000400	// Buffer Write Enable - bit[10]
#define PRESTATE_READ_DATA_TRANSFERING	0x00000200	// Read Transfer Active - bit[9]
#define PRESTATE_WRITE_DATA_TRANSFERING	0x00000100	// Write Transfer Active - bit[8]
#define PRESTATE_DATLINE_ACTIVE			0x00000004	// DAT Line Active - bit[2]
#define PRESTATE_DATA_CMD_INHIBIT		0x00000002	// Command Inhibit(DAT) - bit[1]
#define PRESTATE_NONDATA_CMD_INHIBIT	0x00000001	// Command Inhibit(CMD) - bit[0]

// Host Control register (8bit)
#define HOSTCON_DET_CARD_BY_TEST_LEVEL	0x80	// Card Detect Signal - bit[7]
#define HOSTCON_CARD_DET_TEST_LEVEL		0x40	// Card Detect Test Level - bit[6]
#define HOSTCON_DATA_WIDTH8				0x20	// Extended Data Transfer Width(for MMC+) - bit[5]
#define HOSTCON_HIGH_SPEED_EN			0x04	// High Speed Enable - bit[2]
#define HOSTCON_DATA_WIDTH4				0x02	// Data Transfer Width - bit[1]
#define HOSTCON_LED_ON					0x01	// LCD Control - bit[0]

// Power Control register (8bit)
#define POWCON_BUS_VOLTAGE18			0x0A	// SD Bus Voltage 1.8V - bit[3:1]
#define POWCON_BUS_VOLTAGE30			0x0C	// SD Bus Voltage 3.0V
#define POWCON_BUS_VOLTAGE33			0x0E	// SD Bus Voltage 3.3V
#define POWCON_BUS_VOLTAGE_FIELD		POWCON_BUS_VOLTAGE33
#define POWCON_BUS_POWER_ON				0x01	// SD Bus Power - bit[0]

// Block Gap Control register (8bit)
#define BLKGAPCON_INTERRUPT_EN			0x08	// Interrupt Enable at Block Gap - bit[3]
#define BLKGAPCON_READWAIT_EN			0x04	// Read Wait Control Enable - bit[2]
#define BLKGAPCON_RESTART_TRANS			0x02	// Continue Request - bit[1]
#define BLKGAPCON_STOP_AT_NEXT_BLKGAP	0x01	// Stop at Block Gap Request - bit[0]

// Wakeup Control register (8bit)
#define WAKECON_EVENT_ON_REMOVAL_EN		0x04	// Wakeup Event Enable of SD Card Removal - bit[2]
#define WAKECON_EVENT_ON_INSERT_EN		0x02	// Wakeup Event Enable of SD Card Insert - bit[1]
#define WAKECON_EVENT_ON_INTERRUPT_EN	0x01	// Wakeup Event Enable of SD Card Interrupt - bit[0]

// Clock Control register (16bit)
#define CLKCON_SDCLK_DIV1				0x0000	// SDCLK Frequency Divided by 1 - bit[15:8]
#define CLKCON_SDCLK_DIV2				0x0100	// SDCLK Frequency Divided by 2
#define CLKCON_SDCLK_DIV4				0x0200	// SDCLK Frequency Divided by 4
#define CLKCON_SDCLK_DIV8				0x0400	// SDCLK Frequency Divided by 8
#define CLKCON_SDCLK_DIV16				0x0800	// SDCLK Frequency Divided by 16
#define CLKCON_SDCLK_DIV32				0x1000	// SDCLK Frequency Divided by 32
#define CLKCON_SDCLK_DIV64				0x2000	// SDCLK Frequency Divided by 64
#define CLKCON_SDCLK_DIV128				0x4000	// SDCLK Frequency Divided by 128
#define CLKCON_SDCLK_DIV256				0x8000	// SDCLK Frequency Divided by 256
#define CLKCON_SDCLK_DIV_FIELD			0xFF00
#define CLKCON_SDCLK_SD375KHz			CLKCON_SDCLK_DIV64
#define CLKCON_SDCLK_SD281KHz			CLKCON_SDCLK_DIV64
#define CLKCON_EXTCLK_STABLE			0x0008	// External Clock Stable - bit[3]
#define CLKCON_SDCLK_EN					0x0004	// SD Clock Enable - bit[2]
#define CLKCON_INTCLK_STABLE			0x0002	// Internal Clock Stable - bit[1]
#define CLKCON_INTCLK_EN				0x0001	// Internal Clock Enable - bit[0]

// Timeout Control register (8bit)
#define TIMEOUTCON_DATA_TIMEOUT_14		0x01	// Data Timeout Counter TMCLK X 2^14 - bit[3:0]
#define TIMEOUTCON_DATA_TIMEOUT_15		0x02	// 		TMCLK X 2^15
#define TIMEOUTCON_DATA_TIMEOUT_16		0x03	// 		TMCLK X 2^16
#define TIMEOUTCON_DATA_TIMEOUT_17		0x04	// 		TMCLK X 2^17
#define TIMEOUTCON_DATA_TIMEOUT_18		0x05	// 		TMCLK X 2^18
#define TIMEOUTCON_DATA_TIMEOUT_19		0x06	// 		TMCLK X 2^19
#define TIMEOUTCON_DATA_TIMEOUT_20		0x07	// 		TMCLK X 2^20
#define TIMEOUTCON_DATA_TIMEOUT_21		0x08	// 		TMCLK X 2^21
#define TIMEOUTCON_DATA_TIMEOUT_22		0x09	// 		TMCLK X 2^22
#define TIMEOUTCON_DATA_TIMEOUT_23		0x0A	// 		TMCLK X 2^23
#define TIMEOUTCON_DATA_TIMEOUT_24		0x0B	// 		TMCLK X 2^24
#define TIMEOUTCON_DATA_TIMEOUT_25		0x0C	// 		TMCLK X 2^25
#define TIMEOUTCON_DATA_TIMEOUT_26		0x0D	// 		TMCLK X 2^26
#define TIMEOUTCON_DATA_TIMEOUT_27		0x0E	// 		TMCLK X 2^27
#define TIMEOUTCON_DATA_TIMEOUT_FIELD	0x0F

// Software Reset register (8bit)
#define SWRESET_DATLINE					0x04	// Software Reset for DAT Line - bit[2]
#define SWRESET_CMDLINE					0x02	// Software Reset for CMD Line - bit[1]
#define SWRESET_ALLLINE					0x01	// Software Reset for All Line - bit[0]

// Normal Interrupt Status register (16bit)
#define NORINTSTA_ERR					0x8000	// Error Interrupt - bit[15]
#define NORINTSTA_ADDR_POINTER3			0x4000	// FIFO SD Addr Pointer Interrupt3 Status - bit[14]
#define NORINTSTA_ADDR_POINTER2			0x2000	// FIFO SD Addr Pointer Interrupt2 Status - bit[13]
#define NORINTSTA_ADDR_POINTER1			0x1000	// FIFO SD Addr Pointer Interrupt1 Status - bit[12]
#define NORINTSTA_ADDR_POINTER0			0x0800	// FIFO SD Addr Pointer Interrupt0 Status - bit[11]
#define NORINTSTA_READ_WAIT				0x0400	// Read Wait Interrupt Status - bit[10]
#define NORINTSTA_CCS					0x0200	// CCS Interrupt Status - bit[9]
#define NORINTSTA_GENERATE_CARD_INT		0x0100	// Card Interrupt - bit[8]
#define NORINTSTA_CARD_REMOVED			0x0080	// Card Removal - bit[7]
#define NORINTSTA_CARD_INSERTED			0x0040	// Card Insertion - bit[6]
#define NORINTSTA_BUF_READ_READY		0x0020	// Buffer Read Ready - bit[5]
#define NORINTSTA_BUF_WRITE_READY		0x0010	// Buffer Write Ready - bit[4]
#define NORINTSTA_DET_DMA_BOUNDARY		0x0008	// DMA Interrupt - bit[3]
#define NORINTSTA_BLOCK_GAP				0x0004	// Block Gap Event - bit[2]
#define NORINTSTA_TRANS_COMPLETE		0x0002	// Transfer Complete - bit[1]
#define NORINTSTA_CMD_COMPLETE			0x0001	// Command Complete - bit[0]

// Error Interrupt Status register (16bit)
#define ERRINTSTA_AUTOCMD12				0x0100	// Auto CMD12 Error - bit[8]
#define ERRINTSTA_DATA_ENDBIT			0x0040	// Data End Bit Error - bit[6]
#define ERRINTSTA_DATA_CRC				0x0020	// Data CRC Error - bit[5]
#define ERRINTSTA_DATA_TIMEOUT			0x0010	// Data Timeout Error - bit[4]
#define ERRINTSTA_CMD_INDEX				0x0008	// Command Index Error - bit[3]
#define ERRINTSTA_CMD_ENDBIT			0x0004	// Command End Bit Error - bit[2]
#define ERRINTSTA_CMD_CRC				0x0002	// Command CRC Error - bit[1]
#define ERRINTSTA_CMD_TIMEOUT			0x0001	// Command Timeout Error - bit[0]

// Normal Interrupt Status Enable register (16bit)
#define NORINTSTAEN_ADDR_POINTER3		0x4000	// FIFO SD Addr Pointer INT #3 Status En - bit[14]
#define NORINTSTAEN_ADDR_POINTER2		0x2000	// FIFO SD Addr Pointer INT #2 Status En - bit[13]
#define NORINTSTAEN_ADDR_POINTER1		0x1000	// FIFO SD Addr Pointer INT #1 Status En - bit[12]
#define NORINTSTAEN_ADDR_POINTER0		0x0800	// FIFO SD Addr Pointer INT #0 Status En - bit[11]
#define NORINTSTAEN_READ_WAIT			0x0400	// Read Wait Interrupt Status Enable - bit[10]
#define NORINTSTAEN_CCS					0x0200	// CCS Interrupt Status Enable - bit[9]
#define NORINTSTAEN_GENERATE_CARD_INT	0x0100	// Card Interrupt Status Enable - bit[8]
#define NORINTSTAEN_CARD_REMOVED		0x0080	// Card Removal Status Enable - bit[7]
#define NORINTSTAEN_CARD_INSERTED		0x0040	// Card Insertion Status Enable - bit[6]
#define NORINTSTAEN_BUF_READ_READY		0x0020	// Buffer Read Ready Status Enable - bit[5]
#define NORINTSTAEN_BUF_WRITE_READY		0x0010	// Buffer Write Ready Status Enable - bit[4]
#define NORINTSTAEN_DET_DMA_BOUNDARY	0x0008	// DMA Interrupt Enable - bit[3]
#define NORINTSTAEN_BLOCK_GAP			0x0004	// Block Gap Event Status Enable - bit[2]
#define NORINTSTAEN_TRANS_COMPLETE		0x0002	// Transfer Complete Status Enable - bit[1]
#define NORINTSTAEN_CMD_COMPLETE		0x0001	// Command Complete Status Enable - bit[0]

// Error Interrupt Status Enable register (16bit)
#define ERRINTSTAEN_AUTOCMD12			0x0100	// Auto CMD12 Error Status Enable - bit[8]
#define ERRINTSTAEN_DATA_ENDBIT			0x0040	// Data End Bit Error Status Enable - bit[6]
#define ERRINTSTAEN_DATA_CRC			0x0020	// Data CRC Error Status Enable - bit[5]
#define ERRINTSTAEN_DATA_TIMEOUT		0x0010	// Data Timeout Error Status Enable - bit[4]
#define ERRINTSTAEN_CMD_INDEX			0x0008	// Command Index Error Status Enable - bit[3]
#define ERRINTSTAEN_CMD_ENDBIT			0x0004	// Command End Bit Error Status Enable - bit[2]
#define ERRINTSTAEN_CMD_CRC				0x0002	// Command CRC Error Status Enable - bit[1]
#define ERRINTSTAEN_CMD_TIMEOUT			0x0001	// Command Timeout Error Status Enable - bit[0]

// Normal Interrupt Signal Enable register (16bit)
#define NORINTSIGEN_ADDR_POINTER3		0x4000	// FIFO SD Addr Pointer INT #3 Signal En - bit[14]
#define NORINTSIGEN_ADDR_POINTER2		0x2000	// FIFO SD Addr Pointer INT #2 Signal En - bit[13]
#define NORINTSIGEN_ADDR_POINTER1		0x1000	// FIFO SD Addr Pointer INT #1 Signal En - bit[12]
#define NORINTSIGEN_ADDR_POINTER0		0x0800	// FIFO SD Addr Pointer INT #0 Signal En - bit[11]
#define NORINTSIGEN_READ_WAIT			0x0400	// Read Wait Interrupt Signal Enable - bit[10]
#define NORINTSIGEN_CCS					0x0200	// CCS Interrupt Signal Enable - bit[9]
#define NORINTSIGEN_GENERATE_CARD_INT	0x0100	// Card Interrupt Signal Enable - bit[8]
#define NORINTSIGEN_CARD_REMOVED		0x0080	// Card Removal Signal Enable - bit[7]
#define NORINTSIGEN_CARD_INSERTED		0x0040	// Card Insertion Signal Enable - bit[6]
#define NORINTSIGEN_BUF_READ_READY		0x0020	// Buffer Read Ready Signal Enable - bit[5]
#define NORINTSIGEN_BUF_WRITE_READY		0x0010	// Buffer Write Ready Signal Enable - bit[4]
#define NORINTSIGEN_DET_DMA_BOUNDARY	0x0008	// DMA Interrupt Signal Enable - bit[3]
#define NORINTSIGEN_BLOCK_GAP			0x0004	// Block Gap Event Signal Enable - bit[2]
#define NORINTSIGEN_TRANS_COMPLETE		0x0002	// Transfer Complete Signal Enable - bit[1]
#define NORINTSIGEN_CMD_COMPLETE		0x0001	// Command Complete Signal Enable - bit[0]

// Error Interrupt Signal Enable register (16bit)
#define ERRINTSIGEN_AUTOCMD12			0x0100	// Auto CMD12 Error Signal Enable - bit[8]
#define ERRINTSIGEN_DATA_ENDBIT			0x0040	// Data End Bit Error Signal Enable - bit[6]
#define ERRINTSIGEN_DATA_CRC			0x0020	// Data CRC Error Signal Enable - bit[5]
#define ERRINTSIGEN_DATA_TIMEOUT		0x0010	// Data Timeout Error Signal Enable - bit[4]
#define ERRINTSIGEN_CMD_INDEX			0x0008	// Command Index Error Signal Enable - bit[3]
#define ERRINTSIGEN_CMD_ENDBIT			0x0004	// Command End Bit Error Signal Enable - bit[2]
#define ERRINTSIGEN_CMD_CRC				0x0002	// Command CRC Error Signal Enable - bit[1]
#define ERRINTSIGEN_CMD_TIMEOUT			0x0001	// Command Timeout Error Signal Enable - bit[0]

// Auto CMD 12 Error Status	register (16bit)
#define AUTCMD12ERRSTA_CMD_NOT_EXEC		0x0080	// CMD_wo_DAT not exec by Auto CMD12 ERR - bit[7]
#define AUTCMD12ERRSTA_INDEX			0x0010	// Auto CMD12 Index Error - bit[4]
#define AUTCMD12ERRSTA_ENDBIT			0x0008	// Auto CMD12 End Bit Error - bit[3]
#define AUTCMD12ERRSTA_CRC				0x0004	// Auto CMD12 Response CRC Error - bit[2]
#define AUTCMD12ERRSTA_TIMEOUT			0x0002	// Auto CMD12 Response Timeout Error - bit[1]
#define AUTCMD12ERRSTA_NOT_EXEC			0x0001	// Auto CMD12 not Executed Error - bit[0]

// Capabilities0 register (32bit)
#define CAPA0_VOLT_1_8_SUPPORT			0x04000000	// Voltage Support 1.8V - bit[26](HWInit = 1b)
#define CAPA0_VOLT_3_0_SUPPORT			0x02000000	// Voltage Support 3.0V - bit[25](HWInit = 0b)
#define CAPA0_VOLT_3_3_SUPPORT			0x01000000	// Voltage Support 3.3V - bit[24](HWInit = 1b)
#define CAPA0_SUSPEND_RESUME_SUPPORT	0x00800000	// Susp, Resume Support - bit[23](HWInit = 1b)
#define CAPA0_SDMA_SUPPORT				0x00400000	// SDMA Support - bit[22](HWInit = 1b)
#define CAPA0_HIGH_SPEED_SUPPORT		0x00200000	// High Speed Support - bit[21](HWInit = 1b)
#define CAPA0_MAX_BLKLEN_512B			0x00000000	// Max BlkLen 512Byte - bit[17:16](HWInit = 00b)
#define CAPA0_MAX_BLKLEN_1K				0x00010000	// 		1KByte
#define CAPA0_MAX_BLKLEN_2K				0x00020000	// 		2KByte
#define CAPA0_MAX_BLKLEN_FIELD			0x00030000
#define CAPA0_BASE_CLOCK_FIELD			0x00003F00	// Base Clk Freq - bit[13:8](HWInit = 000000b)
#define CAPA0_TIMEOUT_CLK_UNIT_MHZ		0x00000080	// Timeout Clk Unit - bit[7](HWInit = 1b)
#define CAPA0_TIMEOUT_CLK_FREQ_FIELD	0x0000003F	// Timeout Clk Freq - bit[5:0](HWInit = 000000b)

// Maximum Current Capabilities0 register (32bit)
#define MAXCURCAPA0_VOLT_1_8_FIELD		0x00FF0000	// Max Curr for 1.8V - bit[23:16](HWInit = 0x00)
#define MAXCURCAPA0_VOLT_3_0_FIELD		0x0000FF00	// Max Curr for 3.0V - bit[15:8](HWInit = 0x00)
#define MAXCURCAPA0_VOLT_3_3_FIELD		0x000000FF	// Max Curr for 3.3V - bit[7:0](HWInit = 0x00)

// Control Register 2 (32bit)
#define CON2_WR_STATUS_CLEAR_ASYN_EN	0x80000000
#define CON2_CMD_CONFLICT_MASK_EN		0x40000000
#define CON2_CARDDET_USING_DAT3_EN		0x20000000	// Card DET Signal Invers for RX_DAT[3] - bit[29]
#define CON2_CHK_NOCARD_USING_FILTER	0x10000000	// Card Removed Condition Selection - bit[28]
#define CON2_FILTER_CLK_SEL1			0x01000000	// Filter Clk Period = [2^( 1+5)] * iSDCLK - bit[27:24]
#define CON2_FILTER_CLK_SEL2			0x02000000	// Filter Clk Period = [2^( 2+5)] * iSDCLK
//...
//...
#define CON2_FILTER_CLK_SEL14			0x0E000000	// Filter Clock Period = [2^(14+5)] * iSDCLK
#define CON2_FILTER_CLK_SEL15			0x0F000000	// Filter Clock Period = [2^(15+5)] * iSDCLK
#define CON2_FILTER_CLK_SEL_FIELD		CON2_FILTER_CLK_SEL15
#define CON2_DAT_LINE_ALL_HIGH			0x00FF0000	// DAT Line Level(Read Only) - bit[23:16]
#define CON2_TX_FEEDBACK_CLK_EN			0x00008000	// Feedback Clk En for Tx Data/Cmd Clk - bit[15]
#define CON2_RX_FEEDBACK_CLK_EN			0x00004000	// Feedback Clk En for Rx Data/Cmd Clk - bit[14]
#define CON2_CARDDET_USING_DAT3			0x00002000	// Card DET Signal Invers for RX_DAT[3] - bit[13]
#define CON2_SIGNAL_SYNC_DET			0x00001000	// SD Card Detect Sync Support - bit[12]
#define CON2_CHK_BSY_BEFORE_TX			0x00000800	// CE-ATA I/F mode - bit[11]
#define CON2_DEBOUNCE_FILTER_CNT4		0x00000200	// Debounce Filter Count 4 iSDCLK - bit[10:9]
#define CON2_DEBOUNCE_FILTER_CNT6		0x00000400	// 		16 iSDCLK
#define CON2_DEBOUNCE_FILTER_CNT64		0x00000600	// 		64 iSDCLK
#define CON2_DEBOUNCE_FILTER_CNT_FIELD	CON2_DEBOUNCE_FILTER_CNT64
#define CON2_SDCLK_HOLD_ENABLE			0x00000100	// SDCLK Hold Enable (Always SET) - bit[8]
#define CON2_DEV_RELEASE_READWAIT		0x00000080	// Read Wait Release Control - bit[7]
#define CON2_BUFFER_READ_DISABLE		0x00000040	// Buffer Read Disable - bit[6]
#define CON2_CLK_SRC_HCLK				0x00000010	// Base Clock Source Select - bit[5:4]
#define CON2_CLK_SRC_SDCLK				0x00000020	// 		48MHz
#define CON2_CLK_SRC_EXTCLK				0x00000030
#define CON2_CLK_SRC_FIELD				CON2_CLK_SRC_EXTCLK
#define CON2_CONTROL_PWR_SWITCH			0x00000008	// SD OP PWR Sync Support with SD Card - bit[3]
#define CON2_PWR_ON_LEVEL_PINMODE		0x00000004	// Power Pin Use mode select - bit[2]
#define CON2_NOSTOP_CLK_IN_NOCARD		0x00000002	// SDCLK output clk masking when Card Insert cleared - bit[1]
#define CON2_HOSTCON_HWINIT_FINISH		0x00000001	// SD Host Controller HWInit Finish - bit[0]

// Control Register 3 / FIFO Interrupt Control  (32bit)
#define CON3_INITIAL_VALUE				0x7F5F3F1F
#define CON3_TX_FEEDBACK_CLK_DELAY1		0x00800000
#define CON3_TX_FEEDBACK_CLK_DELAY2		0x80800000	// More Delay
#define CON3_TX_FEEDBACK_CLK_DELAY4		0x80000000
#define CON3_TX_FEEDBACK_CLK_FIELD		CON3_TX_FEEDBACK_CLK_DELAY4
#define CON3_RX_FEEDBACK_CLK_DELAY2		0x00000080
#define CON3_RX_FEEDBACK_CLK_DELAY3		0x00008000
#define CON3_RX_FEEDBACK_CLK_DELAY4		0x00008080	// More Delay
#define CON3_RX_FEEDBACK_CLK_FIELD		CON3_RX_FEEDBACK_CLK_DELAY4

// Host Controller Version Register (16bit)
#define HOSTCONVER_VENDOR_VER_FIELD		0xFF00	// Vendor Ver Number - bit[15:8](HWInit = 0x03)
#define HOSTCONVER_SPEC_VER_FIELD		0x00FF	// Spec Version Number - bit[7:0](HWInit = 0x00)

// command index - SD, MMC common
#define SD_CMD00_GO_IDLE_STATE          0x0000  // reset all cards(->idle state)
#define SD_CMD02_ALL_SEND_CID           0x0200  // ask all cards to send CID Number(use CMD Line)
#define SD_CMD03_SEND_RELATIVE_ADDR     0x0300  // ask card to publish a new RCA
#define SD_CMD04_SET_DSR				0x0400	// programs the DSR of all cards
#define SD_CMD06_SWITCH                 0x0600  // support switching function
#define SD_CMD07_SELECT_CARD            0x0700  // select card using RCA
#define SD_CMD09_SEND_CSD               0x0900  // addressed card sends CSD(use CMD Line)
#define SD_CMD10_SEND_CID               0x0A00  // addressed card sends CID(use CMD Line)
#define SD_CMD12_STOP_TRANSMISSION      0x0C00  // forces the card to stop transmission
#define SD_CMD13_SEND_STATUS            0x0D00  // addressed card sends its status register
#define SD_CMD15_GO_INACTIVE_STATUS     0x0F00  // set card to inactive state
#define SD_CMD16_SET_BLOCKLEN           0x1000  // set the block length
#define SD_CMD17_READ_SINGLE_BLOCK      0x1100  // read single block
#define SD_CMD18_READ_MULTIPLE_BLOCK    0x1200  // read multiple block
#define SD_CMD24_WRITE_BLOCK            0x1800  // write single block
#define SD_CMD25_WRITE_MULTIPLE_BLOCK   0x1900  // write multiple block
#define SD_CMD27_PROGRAM_CSD            0x1B00  // program CSD
#define SD_CMD28_SET_WRITE_PROT			0x1C00	// set wp bit(refer CSD WP_GRP_SIZE)
#define SD_CMD29_CLR_WRITE_PROT			0x1D00	// clear wp bit
#define SD_CMD30_SEND_WRITE_PROT		0x1E00	// ask status of wp bit (use DAT Line)
#define SD_CMD38_ERASE					0x2600	// erase selected write block
#define SD_CMD42_LOCK_UNLOCK			0x2A00	// set,reset PW or lock,unlock card
#define SD_CMD55_APP_CMD                0x3700  // indicate next CMD is Application Command                       */
#define SD_CMD56_GEN_CMD				0x3800	// read,write single block
// command index - SD specific
#define SD_CMD08_SEND_IF_COND           0x0800  // send SD card interface condition
#define SD_CMD32_ERASE_WR_BLK_START		0x2000	// set first write block address will be erased
#define SD_CMD33_ERASE_WR_BLK_END		0x2100	// set last write block address will be erased
#define SD_ACMD06_SET_BUS_WIDTH         (SD_CMD_APP_CMD|0x0600)	// set data bus width
#define SD_ACMD13_SD_STATUS             (SD_CMD_APP_CMD|0x0D00)	// request SD_CARD status
#define SD_ACMD22_SEND_NUM_WR_BLOCKS	(SD_CMD_APP_CMD|0x1600)	// return num of no err write block
#define SD_ACMD23_SET_WRBLK_ERASE_COUNT (SD_CMD_APP_CMD|0x1700)	// set pre-erase blcok
#define SD_ACMD41_SD_APP_OP_COND        (SD_CMD_APP_CMD|0x2900)	// request OCR
#define SD_ACMD42_SET_CLR_CARD_DETECT   (SD_CMD_APP_CMD|0x2A00)	// conn,disconn pull-up on CD/DAT3
#define SD_ACMD51_SEND_SCR              (SD_CMD_APP_CMD|0x3300)	// read SCR
// command index - MMC specific
#define SD_CMD01_SEND_OP_COND           0x0100  // ask card to send operation cond(use CMD Line)
#define SD_CMD08_SEND_EXT_CSD           0x0800  // read EXT_CSD from HS-MMC
#define SD_CMD11_READ_DAT_UNTIL_STOP	0x0B00	// read data until CMD12
#define SD_CMD14_BUSTEST_R				0x0E00	// read reversed bus testing data pattern
#define SD_CMD19_BUSTEST_W				0x1300	// send bus data pattern to card
#define SD_CMD20_WRITE_DAT_UNTIL_STOP	0x1400	// write data until CMD12
#define SD_CMD23_SET_BLOCK_COUNT        0x1700  // set block count
#define SD_CMD26_PROGRAM_CID			0x1A00	// program CID (for manufacture)
#define SD_CMD35_ERASE_GROUP_START		0x2300	// set first write group address will be erased
#define SD_CMD36_ERASE_GROUP_END		0x2400	// set last write group address will be erased
#define SD_CMD39_FAST_IO				0x2700	// read,write 8bit data field
#define SD_CMD40_GO_IRQ_STATE			0x2800	// set system into interrupt mode
