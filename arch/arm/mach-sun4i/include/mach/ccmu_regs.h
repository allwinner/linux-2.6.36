/*
**********************************************************************************************************************
*                                                  CCMU BSP for sun
*                                 CCMU hardware registers definition and BSP interfaces
*
*                             Copyright(C), 2006-2009, uLIVE
*											       All Rights Reserved
*
* File Name : ccmu.h
*
* Author : Jerry
*
* Version : 1.1.0
*
* Date : 2009-8-20 11:10:01
*
* Description : This file provides some definition of CCMU's hardware registers and BSP interfaces.
*             This file is very similar to file "ccmu.inc"; the two files should be modified at the
*             same time to keep coherence of information.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Jerry      2008.05.23       1.1.0        build the file
*
* Jerry      2009-8-20        2.1.0        updata for new vision
*
**********************************************************************************************************************
*/
#ifndef _CCMU_H_
#define _CCMU_H_

#define CCMU_REGS_BASE 0xf1c20000
#define bCCMU_CTL(Nb)	(CCMU_REGS_BASE + (Nb))

#define CCMU_Read_Reg(x, pos, width)          ((__REG(x)>>pos)&((1<<width)-1))
#define CCMU_Write_Reg(x, pos, width, val)    (__REG(x)=(__REG(x)&(__u32)(~(((1<<width)-1)<<pos)))|(__u32)((val&((1<<width)-1))<<pos))

#define PA_CCMU_BASE		0x01c20000
#define VA_CCMU_BASE		IO_ADDRESS(PA_CCMU_BASE)

/* Offset */
#define CCMU_REG_o_AC320PLL         0x00
#define CCMU_REG_o_AUDIOPLL         0x04
#define CCMU_REG_o_AC320AHB         0x08
#define CCMU_REG_o_AHBMOD           0x0c
#define CCMU_REG_o_APBMOD           0x10
#define CCMU_REG_o_NANDMS           0x14
#define CCMU_REG_o_SDMMC01          0x18
#define CCMU_REG_o_SDMMC23          0x1C
#define CCMU_REG_o_DRAMPLL          0x20
#define CCMU_REG_o_DE               0x24
#define CCMU_REG_o_LCDMACC          0x28
#define CCMU_REG_o_TVCSI            0x2C
#define CCMU_REG_o_VIDEOPLL         0x30
#define CCMU_REG_o_IR               0x34
#define CCMU_REG_o_SPAC97           0x38
#define CCMU_REG_o_TS               0x3C
#define CCMU_REG_o_AVSUSB           0x40
#define CCMU_REG_o_CGTM             0xD0
#define CCMU_REG_o_SWPR             0xD4

/* registers */
#define CCMU_REG_AC320PLL           bCCMU_CTL(CCMU_REG_o_AC320PLL   )
#define CCMU_REG_AUDIOPLL           bCCMU_CTL(CCMU_REG_o_AUDIOPLL   )
#define CCMU_REG_AC320AHB           bCCMU_CTL(CCMU_REG_o_AC320AHB   )
#define CCMU_REG_AHBMOD             bCCMU_CTL(CCMU_REG_o_AHBMOD     )
#define CCMU_REG_APBMOD             bCCMU_CTL(CCMU_REG_o_APBMOD     )
#define CCMU_REG_NANDMS             bCCMU_CTL(CCMU_REG_o_NANDMS     )
#define CCMU_REG_SDMMC01            bCCMU_CTL(CCMU_REG_o_SDMMC01    )
#define CCMU_REG_SDMMC23            bCCMU_CTL(CCMU_REG_o_SDMMC23    )
#define CCMU_REG_DRAMPLL            bCCMU_CTL(CCMU_REG_o_DRAMPLL    )
#define CCMU_REG_DE                 bCCMU_CTL(CCMU_REG_o_DE         )
#define CCMU_REG_LCDMACC            bCCMU_CTL(CCMU_REG_o_LCDMACC    )
#define CCMU_REG_TVCSI              bCCMU_CTL(CCMU_REG_o_TVCSI      )
#define CCMU_REG_VIDEOPLL           bCCMU_CTL(CCMU_REG_o_VIDEOPLL   )
#define CCMU_REG_IR                 bCCMU_CTL(CCMU_REG_o_IR         )
#define CCMU_REG_SPAC97             bCCMU_CTL(CCMU_REG_o_SPAC97     )
#define CCMU_REG_TS                 bCCMU_CTL(CCMU_REG_o_TS         )
#define CCMU_REG_AVSUSB             bCCMU_CTL(CCMU_REG_o_AVSUSB     )
#define CCMU_REG_CGTM               bCCMU_CTL(CCMU_REG_o_CGTM       )
#define CCMU_REG_SWPR               bCCMU_CTL(CCMU_REG_o_SWPR       )

/* CCMU  AC320 / MACC PLL */
#define CCMU_BP_CORE_PLL_EN                 29
#define CCMU_BP_CORE_PLL_BYPASS_EN          26
#define CCMU_BP_CORE_PLL_FACTOR             16
#define CCMU_BP_MACC_PLL_EN                 15
#define CCMU_BP_MACC_PLL_BYPASS_EN          12
#define CCMU_BP_MACC_PLL_FACTOR             0
/* CCMU  AUDIO PLL / HOSC */
#define CCMU_BP_AUDIO_PLL_EN                29
#define CCMU_BP_AUDIO_PLL_OUTSEL            26
#define CCMU_BP_LDO_EN                      15
#define CCMU_BP_HOSC_EN                     0
/* CCMU  AC320 / AHB / APB */
#define CCMU_BP_AC320_CLK_SRC               9
#define CCMU_BP_APB_CLK_DIV                 6
#define CCMU_BP_AHB_CLK_DIV                 3
#define CCMU_BP_AC320_CLK_DIV               0
/* CCMU  AHB Module */
#define CCMU_BP_SPI1_AHB_GATE               20
#define CCMU_BP_SPI0_AHB_GATE               19
#define CCMU_BP_TS_AHB_GATE                 18
#define CCMU_BP_EMAC_AHB_GATE               17
#define CCMU_BP_BIST_AHB_GATE               16
#define CCMU_BP_MACC_AHB_GATE               15
#define CCMU_BP_LCD_AHB_GATE                14
#define CCMU_BP_SDRAM_AHB_GATE              13
#define CCMU_BP_NAND_AHB_GATE               12
#define CCMU_BP_MS_AHB_GATE                 11
#define CCMU_BP_SDMMC3_AHB_GATE             10
#define CCMU_BP_SDMMC2_AHB_GATE             9
#define CCMU_BP_SDMMC1_AHB_GATE             8
#define CCMU_BP_SDMMC0_AHB_GATE             7
#define CCMU_BP_DMA_AHB_GATE                6
#define CCMU_BP_CSI_AHB_GATE                5
#define CCMU_BP_TVE_AHB_GATE                4
#define CCMU_BP_DE_AHB_GATE                 3
#define CCMU_BP_SS_AHB_GATE                 2
#define CCMU_BP_USB1_AHB_GATE               1
#define CCMU_BP_USB0_AHB_GATE               0
/* CCMU  APB Module */
#define CCMU_BP_PS2_APB_GATE                14
#define CCMU_BP_AC97_APB_GATE               13
#define CCMU_BP_SPDIF_APB_GATE              12
#define CCMU_BP_IIS_APB_GATE                11
#define CCMU_BP_IR_APB_GATE                 10
#define CCMU_BP_CODEC_APB_GATE              9
#define CCMU_BP_UART3_APB_GATE              8
#define CCMU_BP_UART2_APB_GATE              7
#define CCMU_BP_UART1_APB_GATE              6
#define CCMU_BP_UART0_APB_GATE              5
#define CCMU_BP_PIO_APB_GATE                4
#define CCMU_BP_TWI1_APB_GATE               3
#define CCMU_BP_TWI0_APB_GATE               2
/* CCMU  NAND / MS Module */
#define CCMU_BP_NAND_MDL_GATE               23
#define CCMU_BP_NAND_MDL_SRC                22
#define CCMU_BP_NAND_MDL_DIV                16
#define CCMU_BP_MS_MDL_GATE                 11
#define CCMU_BP_MS_MDL_SRC                  10
#define CCMU_BP_MS_MDL_DIV                  0
/* CCMU  SDMMC 01 Module */
#define CCMU_BP_SDMMC1_MDL_GATE             31
#define CCMU_BP_SDMMC1_MDL_SRC              30
#define CCMU_BP_SDMMC1_MDL_DIV              16
#define CCMU_BP_SDMMC0_MDL_GATE             15
#define CCMU_BP_SDMMC0_MDL_SRC              14
#define CCMU_BP_SDMMC0_MDL_DIV              0
/* CCMU  SDMMC 23 Module */
#define CCMU_BP_SDMMC3_MDL_GATE             31
#define CCMU_BP_SDMMC3_MDL_SRC              30
#define CCMU_BP_SDMMC3_MDL_DIV              16
#define CCMU_BP_SDMMC2_MDL_GATE             15
#define CCMU_BP_SDMMC2_MDL_SRC              14
#define CCMU_BP_SDMMC2_MDL_DIV              0
/* CCMU  DRAM PLL */
#define CCMU_BP_MACC_DRAM_GATE              31
#define CCMU_BP_DEFE_DRAM_GATE              30
#define CCMU_BP_DEBE_DRAM_GATE              29
#define CCMU_BP_CSI_DRAM_GATE               28
#define CCMU_BP_TS_DRAM_GATE                27
#define CCMU_BP_DRAM_CKB_EN                 16
#define CCMU_BP_DRAM_CLK_EN                 15
#define CCMU_BP_DRAM_CLK_DIV                12
#define CCMU_BP_DRAM_PLL_EN                 11
#define CCMU_BP_DRAM_PLL_BYPASS_EN          10
#define CCMU_BP_DRAM_PLL_BIAS               8
#define CCMU_BP_DRAM_PLL_FACTOR             0
/* CCMU  DE Clock */
#define CCMU_BP_DEBE_SPEC_MDL_GATE          31
#define CCMU_BP_DEBE_SPEC_DIV               29
#define CCMU_BP_DEFE_SPEC_MDL_GATE          15
#define CCMU_BP_DEFE_SPEC_SRC               13
#define CCMU_BP_DEFE_SPEC_DIV               0
/* CCMU  LCD/MACC */
#define CCMU_BP_LCD_BYPASS_EN               16
#define CCMU_BP_MACC_SPEC_MDL_GATE          7
#define CCMU_BP_MACC_RST                    5
/* CCMU  TV / CSI */
#define CCMU_BP_TV2_SPEC_MDL_GATE           31
#define CCMU_BP_TV2_SPEC_SRC                30
#define CCMU_BP_TV2_SPEC_DIV                24
#define CCMU_BP_TV1_SPEC_MDL_GATE           23
#define CCMU_BP_TV1_SPEC_SRC                22
#define CCMU_BP_CSI_SPEC_MDL_GATE           11
#define CCMU_BP_CSI_SPEC_SRC                9
#define CCMU_BP_CSI_SPEC_DIV                0
/* CCMU  VIDEO PLL */
#define CCMU_BP_VIDEO_PLL_FACTOR            8
#define CCMU_BP_VIDEO_PLL_EN                7
/* CCMU  IR */
#define CCMU_BP_FIR_MDL_SRC                 8
#define CCMU_BP_FIR_MDL_GATE                7
#define CCMU_BP_FIR_MDL_DIV                 0
/* CCMU SPDIF / AC97 / IIS / CODEC */
#define CCMU_BP_CODEC_MDL_GATE              3
#define CCMU_BP_SPDIF_MDL_GATE              2
#define CCMU_BP_IIS_MDL_GATE                1
#define CCMU_BP_AC97_MDL_GATE               0
/* CCMU TS/ SS */
#define CCMU_BP_SS_MDL_GATE                 21
#define CCMU_BP_SS_MDL_SRC                  20
#define CCMU_BP_SS_MDL_DIV                  16
#define CCMU_BP_TS_MDL_GATE                 5
#define CCMU_BP_TS_MDL_SRC                  4
#define CCMU_BP_TS_MDL_DIV                  0
/* CCMU AVS CODEC USB */
#define CCMU_BP_AVS_MDL_GATE                4
#define CCMU_BP_USBPHY0_RST                 2
#define CCMU_BP_USBPHY1_RST                 1
#define CCMU_BP_USBPHY_CLK_GATE             0


  /* bit field mask */
#define CCMU_BITS_0					        0x00
#define CCMU_BITS_1                         0x01
#define CCMU_BITS_2                         0x03
#define CCMU_BITS_3                         0x07
#define CCMU_BITS_4                         0x0F
#define CCMU_BITS_5                         0x1F
#define CCMU_BITS_6                         0x3F
#define CCMU_BITS_7                         0x7F

/* CCMU  AC320 / MACC PLL */
#define CCMU_MASK_CORE_PLL_EN               ( CCMU_BITS_1 << CCMU_BP_CORE_PLL_EN        )
#define CCMU_MASK_CORE_PLL_BYPASS_EN        ( CCMU_BITS_1 << CCMU_BP_CORE_PLL_BYPASS_EN )
#define CCMU_MASK_CORE_PLL_FACTOR           ( CCMU_BITS_7 << CCMU_BP_CORE_PLL_FACTOR    )
 /* CCMU  AC320 / AHB / APB */
#define CCMU_MASK_AC320_CLK_SRC             ( CCMU_BITS_2 << CCMU_BP_AC320_CLK_SRC )
#define CCMU_MASK_APB_CLK_DIV               ( CCMU_BITS_2 << CCMU_BP_APB_CLK_DIV   )
#define CCMU_MASK_AHB_CLK_DIV               ( CCMU_BITS_2 << CCMU_BP_AHB_CLK_DIV   )
#define CCMU_MASK_AC320_CLK_DIV             ( CCMU_BITS_1 << CCMU_BP_AC320_CLK_DIV )
/* CCMU  AHB Module */
#define CCMU_MASK_SPI1_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_SPI1_AHB_GATE   )
#define CCMU_MASK_SPI0_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_SPI0_AHB_GATE   )
#define CCMU_MASK_TS_AHB_GATE               ( CCMU_BITS_1 << CCMU_BP_TS_AHB_GATE     )
#define CCMU_MASK_EMAC_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_EMAC_AHB_GATE   )
#define CCMU_MASK_BIST_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_BIST_AHB_GATE   )
#define CCMU_MASK_MACC_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_MACC_AHB_GATE   )
#define CCMU_MASK_LCD_AHB_GATE              ( CCMU_BITS_1 << CCMU_BP_LCD_AHB_GATE    )
#define CCMU_MASK_SDRAM_AHB_GATE            ( CCMU_BITS_1 << CCMU_BP_SDRAM_AHB_GATE  )
#define CCMU_MASK_NAND_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_NAND_AHB_GATE   )
#define CCMU_MASK_MS_AHB_GATE               ( CCMU_BITS_1 << CCMU_BP_MS_AHB_GATE     )
#define CCMU_MASK_SDMMC3_AHB_GATE           ( CCMU_BITS_1 << CCMU_BP_SDMMC3_AHB_GATE )
#define CCMU_MASK_SDMMC2_AHB_GATE           ( CCMU_BITS_1 << CCMU_BP_SDMMC2_AHB_GATE )
#define CCMU_MASK_SDMMC1_AHB_GATE           ( CCMU_BITS_1 << CCMU_BP_SDMMC1_AHB_GATE )
#define CCMU_MASK_SDMMC0_AHB_GATE           ( CCMU_BITS_1 << CCMU_BP_SDMMC0_AHB_GATE )
#define CCMU_MASK_DMA_AHB_GATE              ( CCMU_BITS_1 << CCMU_BP_DMA_AHB_GATE    )
#define CCMU_MASK_CSI_AHB_GATE              ( CCMU_BITS_1 << CCMU_BP_CSI_AHB_GATE    )
#define CCMU_MASK_TVE_AHB_GATE              ( CCMU_BITS_1 << CCMU_BP_TVE_AHB_GATE    )
#define CCMU_MASK_DE_AHB_GATE               ( CCMU_BITS_1 << CCMU_BP_DE_AHB_GATE     )
#define CCMU_MASK_SS_AHB_GATE               ( CCMU_BITS_1 << CCMU_BP_SS_AHB_GATE     )
#define CCMU_MASK_USB1_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_USB1_AHB_GATE   )
#define CCMU_MASK_USB0_AHB_GATE             ( CCMU_BITS_1 << CCMU_BP_USB0_AHB_GATE   )
/* CCMU  APB Module */
#define CCMU_MASK_PS2_APB_GATE              ( CCMU_BITS_1 << CCMU_BP_PS2_APB_GATE   )
#define CCMU_MASK_AC97_APB_GATE             ( CCMU_BITS_1 << CCMU_BP_AC97_APB_GATE  )
#define CCMU_MASK_SPDIF_APB_GATE            ( CCMU_BITS_1 << CCMU_BP_SPDIF_APB_GATE )
#define CCMU_MASK_IIS_APB_GATE              ( CCMU_BITS_1 << CCMU_BP_IIS_APB_GATE   )
#define CCMU_MASK_IR_APB_GATE               ( CCMU_BITS_1 << CCMU_BP_IR_APB_GATE    )
#define CCMU_MASK_CODEC_APB_GATE            ( CCMU_BITS_1 << CCMU_BP_CODEC_APB_GATE )
#define CCMU_MASK_UART3_APB_GATE            ( CCMU_BITS_1 << CCMU_BP_UART3_APB_GATE )
#define CCMU_MASK_UART2_APB_GATE            ( CCMU_BITS_1 << CCMU_BP_UART2_APB_GATE )
#define CCMU_MASK_UART1_APB_GATE            ( CCMU_BITS_1 << CCMU_BP_UART1_APB_GATE )
#define CCMU_MASK_UART0_APB_GATE            ( CCMU_BITS_1 << CCMU_BP_UART0_APB_GATE )
#define CCMU_MASK_PIO_APB_GATE              ( CCMU_BITS_1 << CCMU_BP_PIO_APB_GATE   )
#define CCMU_MASK_TWI1_APB_GATE             ( CCMU_BITS_1 << CCMU_BP_TWI1_APB_GATE  )
#define CCMU_MASK_TWI0_APB_GATE             ( CCMU_BITS_1 << CCMU_BP_TWI0_APB_GATE  )
/* CCMU  DRAM PLL */
#define CCMU_MASK_MACC_DRAM_GATE            ( CCMU_BITS_1 << CCMU_BP_MACC_DRAM_GATE  )
#define CCMU_MASK_DEFE_DRAM_GATE            ( CCMU_BITS_1 << CCMU_BP_DEFE_DRAM_GATE  )
#define CCMU_MASK_DEBE_DRAM_GATE            ( CCMU_BITS_1 << CCMU_BP_DEBE_DRAM_GATE  )
#define CCMU_MASK_CSI_DRAM_GATE             ( CCMU_BITS_1 << CCMU_BP_CSI_DRAM_GATE   )
#define CCMU_MASK_TS_DRAM_GATE              ( CCMU_BITS_1 << CCMU_BP_TS_DRAM_GATE    )
#define CCMU_MASK_DRAM_CKB_EN               ( CCMU_BITS_1 << CCMU_BP_DRAM_CKB_EN     )
#define CCMU_MASK_DRAM_CLK_EN               ( CCMU_BITS_1 << CCMU_BP_DRAM_CLK_EN     )
#define CCMU_MASK_DRAM_CLK_DIV              ( CCMU_BITS_2 << CCMU_BP_DRAM_CLK_DIV    )
#define CCMU_MASK_DRAM_PLL_EN               ( CCMU_BITS_1 << CCMU_BP_DRAM_PLL_EN     )
#define CCMU_MASK_DRAM_PLL_BIAS             ( CCMU_BITS_2 << CCMU_BP_DRAM_PLL_BIAS   )
#define CCMU_MASK_DRAM_PLL_FACTOR           ( CCMU_BITS_6 << CCMU_BP_DRAM_PLL_FACTOR )


#endif    // #ifndef _CCMU_H_
