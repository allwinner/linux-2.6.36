/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : clock_dbg.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-18 18:18
* Descript: clock management debug.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/debugfs.h>
#include <linux/io.h>

#include "ccm_i.h"


#define print_clk_inf(x, y)     do{printk(#x"."#y":%d\n", aw_ccu_reg->x.y);}while(0)


void clk_dbg_inf(void)
{
    printk("---------------------------------------------\n");
    printk("    dump clock information                   \n");
    printk("---------------------------------------------\n");

    printk("PLL1 infor:\n");
    print_clk_inf(Pll1Ctl, FactorM      );
    print_clk_inf(Pll1Ctl, SigmaEn      );
    print_clk_inf(Pll1Ctl, SigmaIn      );
    print_clk_inf(Pll1Ctl, FactorK      );
    print_clk_inf(Pll1Ctl, FactorN      );
    print_clk_inf(Pll1Ctl, LockTime     );
    print_clk_inf(Pll1Ctl, PLLDivP      );
    print_clk_inf(Pll1Ctl, PLLBias      );
    print_clk_inf(Pll1Ctl, ExchangeEn   );
    print_clk_inf(Pll1Ctl, VCOBias      );
    print_clk_inf(Pll1Ctl, VCORstIn     );
    print_clk_inf(Pll1Ctl, PLLEn        );

    printk("\nPLL2 infor:\n");
    print_clk_inf(Pll2Ctl, VCOBias      );
    print_clk_inf(Pll2Ctl, PLLBias      );
    print_clk_inf(Pll2Ctl, OutputSel    );
    print_clk_inf(Pll2Ctl, SigmaOut     );
    print_clk_inf(Pll2Ctl, PLLEn        );

    printk("\nPLL3 infor:\n");
    print_clk_inf(Pll3Ctl, FactorM      );
    print_clk_inf(Pll3Ctl, PLLBias      );
    print_clk_inf(Pll3Ctl, FracSet      );
    print_clk_inf(Pll3Ctl, ModeSel      );
    print_clk_inf(Pll3Ctl, VCOBias      );
    print_clk_inf(Pll3Ctl, DampFactor   );
    print_clk_inf(Pll3Ctl, PLLEn        );

    printk("\nPLL4 infor:\n");
    print_clk_inf(Pll4Ctl, FactorM      );
    print_clk_inf(Pll4Ctl, FactorK      );
    print_clk_inf(Pll4Ctl, FactorN      );
    print_clk_inf(Pll4Ctl, FactorP      );
    print_clk_inf(Pll4Ctl, VCOGain      );
    print_clk_inf(Pll4Ctl, PLLBias      );
    print_clk_inf(Pll4Ctl, VCOBias      );
    print_clk_inf(Pll4Ctl, PLLBypass    );
    print_clk_inf(Pll4Ctl, PLLEn        );

    printk("\nPLL5 infor:\n");
    print_clk_inf(Pll5Ctl, FactorM      );
    print_clk_inf(Pll5Ctl, FactorM1     );
    print_clk_inf(Pll5Ctl, FactorK      );
    print_clk_inf(Pll5Ctl, LDO2En       );
    print_clk_inf(Pll5Ctl, FactorN      );
    print_clk_inf(Pll5Ctl, VCOGain      );
    print_clk_inf(Pll5Ctl, FactorP      );
    print_clk_inf(Pll5Ctl, BandWidth    );
    print_clk_inf(Pll5Ctl, VCOGainEn    );
    print_clk_inf(Pll5Ctl, PLLBias      );
    print_clk_inf(Pll5Ctl, VCOBias      );
    print_clk_inf(Pll5Ctl, OutputEn     );
    print_clk_inf(Pll5Ctl, PLLBypass    );
    print_clk_inf(Pll5Ctl, PLLEn        );

    printk("\nPLL6 infor:\n");
    print_clk_inf(Pll6Ctl, FactorM      );
    print_clk_inf(Pll6Ctl, FactorK      );
    print_clk_inf(Pll6Ctl, DampFactor   );
    print_clk_inf(Pll6Ctl, FactorN      );
    print_clk_inf(Pll6Ctl, OutputEn     );
    print_clk_inf(Pll6Ctl, BandWidth    );
    print_clk_inf(Pll6Ctl, PLLBias      );
    print_clk_inf(Pll6Ctl, VCOBias      );
    print_clk_inf(Pll6Ctl, PLLBypass    );
    print_clk_inf(Pll6Ctl, PLLEn        );

    printk("\nPLL7 infor:\n");
    print_clk_inf(Pll7Ctl, FactorM      );
    print_clk_inf(Pll7Ctl, PLLBias      );
    print_clk_inf(Pll7Ctl, FracSet      );
    print_clk_inf(Pll7Ctl, ModeSel      );
    print_clk_inf(Pll7Ctl, VCOBias      );
    print_clk_inf(Pll7Ctl, DampFactor   );
    print_clk_inf(Pll7Ctl, PLLEn        );

    printk("\nHOSC infor:\n");
    print_clk_inf(HoscCtl, OSC24MEn     );
    print_clk_inf(HoscCtl, OSC24MGsm    );
    print_clk_inf(HoscCtl, PLLBiasEn    );
    print_clk_inf(HoscCtl, LDOEn        );
    print_clk_inf(HoscCtl, PLLInPower   );
    print_clk_inf(HoscCtl, LDOOutput    );
    print_clk_inf(HoscCtl, KeyField     );

    printk("\nCPU clk infor:\n");
    print_clk_inf(SysClkDiv, AXIClkDiv  );
    print_clk_inf(SysClkDiv, AHBClkDiv  );
    print_clk_inf(SysClkDiv, APB0ClkDiv );
    print_clk_inf(SysClkDiv, AC328ClkSrc);

    printk("\nAPB1 clk infor:\n");
    print_clk_inf(Apb1ClkDiv, ClkDiv    );
    print_clk_inf(Apb1ClkDiv, PreDiv    );
    print_clk_inf(Apb1ClkDiv, ClkSrc    );

    printk("\nAxiGate clk infor:\n");
    print_clk_inf(AxiGate, SdramGate    );

    printk("\nAhbGate0 clk infor:\n");
    print_clk_inf(AhbGate0, Usb0Gate    );
    print_clk_inf(AhbGate0, Usb1Gate    );
    print_clk_inf(AhbGate0, Usb2Gate    );
    print_clk_inf(AhbGate0, SsGate      );
    print_clk_inf(AhbGate0, DmaGate     );
    print_clk_inf(AhbGate0, BistGate    );
    print_clk_inf(AhbGate0, Sdmmc0Gate  );
    print_clk_inf(AhbGate0, Sdmmc1Gate  );
    print_clk_inf(AhbGate0, Sdmmc2Gate  );
    print_clk_inf(AhbGate0, Sdmmc3Gate  );
    print_clk_inf(AhbGate0, MsGate      );
    print_clk_inf(AhbGate0, NandGate    );
    print_clk_inf(AhbGate0, SdramGate   );
    print_clk_inf(AhbGate0, AceGate     );
    print_clk_inf(AhbGate0, EmacGate    );
    print_clk_inf(AhbGate0, TsGate      );
    print_clk_inf(AhbGate0, Spi0Gate    );
    print_clk_inf(AhbGate0, Spi1Gate    );
    print_clk_inf(AhbGate0, Spi2Gate    );
    print_clk_inf(AhbGate0, Spi3Gate    );
    print_clk_inf(AhbGate0, PataGate    );
    print_clk_inf(AhbGate0, SataGate    );
    print_clk_inf(AhbGate0, GpsGate     );

    printk("\nAhbGate1 clk infor:\n");
    print_clk_inf(AhbGate1, VeGate      );
    print_clk_inf(AhbGate1, TvdGate     );
    print_clk_inf(AhbGate1, Tve0Gate    );
    print_clk_inf(AhbGate1, Tve1Gate    );
    print_clk_inf(AhbGate1, Lcd0Gate    );
    print_clk_inf(AhbGate1, Lcd1Gate    );
    print_clk_inf(AhbGate1, Csi0Gate    );
    print_clk_inf(AhbGate1, Csi1Gate    );
    print_clk_inf(AhbGate1, HdmiDGate   );
    print_clk_inf(AhbGate1, DeBe0Gate   );
    print_clk_inf(AhbGate1, DeBe1Gate   );
    print_clk_inf(AhbGate1, DeFe0Gate   );
    print_clk_inf(AhbGate1, DeFe1Gate   );
    print_clk_inf(AhbGate1, MpGate      );
    print_clk_inf(AhbGate1, Gpu3DGate   );

    printk("\nApb0Gate clk infor:\n");
    print_clk_inf(Apb0Gate, AddaGate    );
    print_clk_inf(Apb0Gate, SpdifGate   );
    print_clk_inf(Apb0Gate, Ac97Gate    );
    print_clk_inf(Apb0Gate, IisGate     );
    print_clk_inf(Apb0Gate, PioGate     );
    print_clk_inf(Apb0Gate, Ir0Gate     );
    print_clk_inf(Apb0Gate, Ir1Gate     );
    print_clk_inf(Apb0Gate, KeypadGate  );

    printk("\nApb1Gate clk infor:\n");
    print_clk_inf(Apb1Gate, Twi0Gate    );
    print_clk_inf(Apb1Gate, Twi1Gate    );
    print_clk_inf(Apb1Gate, Twi2Gate    );
    print_clk_inf(Apb1Gate, CanGate     );
    print_clk_inf(Apb1Gate, ScrGate     );
    print_clk_inf(Apb1Gate, Ps20Gate    );
    print_clk_inf(Apb1Gate, Ps21Gate    );
    print_clk_inf(Apb1Gate, Uart0Gate   );
    print_clk_inf(Apb1Gate, Uart1Gate   );
    print_clk_inf(Apb1Gate, Uart2Gate   );
    print_clk_inf(Apb1Gate, Uart3Gate   );
    print_clk_inf(Apb1Gate, Uart4Gate   );
    print_clk_inf(Apb1Gate, Uart5Gate   );
    print_clk_inf(Apb1Gate, Uart6Gate   );
    print_clk_inf(Apb1Gate, Uart7Gate   );

    printk("\nNandClk clk infor:\n");
    print_clk_inf(NandClk, ClkDiv       );
    print_clk_inf(NandClk, ClkPreDiv    );
    print_clk_inf(NandClk, ClkSrc       );
    print_clk_inf(NandClk, SpecClkGate  );

    printk("\nMsClk clk infor:\n");
    print_clk_inf(MsClk, ClkDiv         );
    print_clk_inf(MsClk, ClkPreDiv      );
    print_clk_inf(MsClk, ClkSrc         );
    print_clk_inf(MsClk, SpecClkGate    );

    printk("\nSdMmc0Clk clk infor:\n");
    print_clk_inf(SdMmc0Clk, ClkDiv     );
    print_clk_inf(SdMmc0Clk, ClkPreDiv  );
    print_clk_inf(SdMmc0Clk, ClkSrc     );
    print_clk_inf(SdMmc0Clk, SpecClkGate);

    printk("\nSdMmc1Clk clk infor:\n");
    print_clk_inf(SdMmc1Clk, ClkDiv     );
    print_clk_inf(SdMmc1Clk, ClkPreDiv  );
    print_clk_inf(SdMmc1Clk, ClkSrc     );
    print_clk_inf(SdMmc1Clk, SpecClkGate);

    printk("\nSdMmc2Clk clk infor:\n");
    print_clk_inf(SdMmc2Clk, ClkDiv     );
    print_clk_inf(SdMmc2Clk, ClkPreDiv  );
    print_clk_inf(SdMmc2Clk, ClkSrc     );
    print_clk_inf(SdMmc2Clk, SpecClkGate);

    printk("\nSdMmc3Clk clk infor:\n");
    print_clk_inf(SdMmc3Clk, ClkDiv     );
    print_clk_inf(SdMmc3Clk, ClkPreDiv  );
    print_clk_inf(SdMmc3Clk, ClkSrc     );
    print_clk_inf(SdMmc3Clk, SpecClkGate);

    printk("\nTsClk clk infor:\n");
    print_clk_inf(TsClk, ClkDiv         );
    print_clk_inf(TsClk, ClkPreDiv      );
    print_clk_inf(TsClk, ClkSrc         );
    print_clk_inf(TsClk, SpecClkGate    );

    printk("\nSsClk clk infor:\n");
    print_clk_inf(SsClk, ClkDiv         );
    print_clk_inf(SsClk, ClkPreDiv      );
    print_clk_inf(SsClk, ClkSrc         );
    print_clk_inf(SsClk, SpecClkGate    );

    printk("\nSpi0Clk clk infor:\n");
    print_clk_inf(Spi0Clk, ClkDiv       );
    print_clk_inf(Spi0Clk, ClkPreDiv    );
    print_clk_inf(Spi0Clk, ClkSrc       );
    print_clk_inf(Spi0Clk, SpecClkGate  );

    printk("\nSpi1Clk clk infor:\n");
    print_clk_inf(Spi1Clk, ClkDiv       );
    print_clk_inf(Spi1Clk, ClkPreDiv    );
    print_clk_inf(Spi1Clk, ClkSrc       );
    print_clk_inf(Spi1Clk, SpecClkGate  );

    printk("\nSpi2Clk clk infor:\n");
    print_clk_inf(Spi2Clk, ClkDiv       );
    print_clk_inf(Spi2Clk, ClkPreDiv    );
    print_clk_inf(Spi2Clk, ClkSrc       );
    print_clk_inf(Spi2Clk, SpecClkGate  );

    printk("\nPataClk clk infor:\n");
    print_clk_inf(PataClk, ClkDiv       );
    print_clk_inf(PataClk, ClkPreDiv    );
    print_clk_inf(PataClk, ClkSrc       );
    print_clk_inf(PataClk, SpecClkGate  );

    printk("\nIr0Clk clk infor:\n");
    print_clk_inf(Ir0Clk, ClkDiv        );
    print_clk_inf(Ir0Clk, ClkPreDiv     );
    print_clk_inf(Ir0Clk, ClkSrc        );
    print_clk_inf(Ir0Clk, SpecClkGate   );

    printk("\nIr1Clk clk infor:\n");
    print_clk_inf(Ir1Clk, ClkDiv        );
    print_clk_inf(Ir1Clk, ClkPreDiv     );
    print_clk_inf(Ir1Clk, ClkSrc        );
    print_clk_inf(Ir1Clk, SpecClkGate   );

    printk("\nI2sClk clk infor:\n");
    print_clk_inf(I2sClk, ClkDiv        );
    print_clk_inf(I2sClk, SpecClkGate   );


    printk("\nAc97Clk clk infor:\n");
    print_clk_inf(Ac97Clk, ClkDiv       );
    print_clk_inf(Ac97Clk, SpecClkGate  );

    printk("\nSpdifClk clk infor:\n");
    print_clk_inf(SpdifClk, ClkDiv      );
    print_clk_inf(SpdifClk, SpecClkGate );

    printk("\nKeyPadClk clk infor:\n");
    print_clk_inf(KeyPadClk, ClkDiv         );
    print_clk_inf(KeyPadClk, ClkPreDiv      );
    print_clk_inf(KeyPadClk, ClkSrc         );
    print_clk_inf(KeyPadClk, SpecClkGate    );

    printk("\nSataClk clk infor:\n");
    print_clk_inf(SataClk, ClkSrc       );
    print_clk_inf(SataClk, SpecClkGate  );

    printk("\nUsbClk clk infor:\n");
    print_clk_inf(UsbClk, UsbPhy0Rst        );
    print_clk_inf(UsbClk, UsbPhy1Rst        );
    print_clk_inf(UsbClk, UsbPhy2Rst        );
    print_clk_inf(UsbClk, OHCIClkSrc        );
    print_clk_inf(UsbClk, OHCI0SpecClkGate  );
    print_clk_inf(UsbClk, OHCI1SpecClkGate  );
    print_clk_inf(UsbClk, PhySpecClkGate    );

    printk("\nGpsClk clk infor:\n");
    print_clk_inf(GpsClk, Reset         );
    print_clk_inf(GpsClk, SpecClkGate   );

    printk("\nSpi3Clk clk infor:\n");
    print_clk_inf(Spi3Clk, ClkDiv       );
    print_clk_inf(Spi3Clk, ClkPreDiv    );
    print_clk_inf(Spi3Clk, ClkSrc       );
    print_clk_inf(Spi3Clk, SpecClkGate  );

    printk("\nDramGate clk infor:\n");
    print_clk_inf(DramGate, VeGate      );
    print_clk_inf(DramGate, Csi0Gate    );
    print_clk_inf(DramGate, Csi1Gate    );
    print_clk_inf(DramGate, TsGate      );
    print_clk_inf(DramGate, TvdGate     );
    print_clk_inf(DramGate, Tve0Gate    );
    print_clk_inf(DramGate, Tve1Gate    );
    print_clk_inf(DramGate, ClkOutputEn );
    print_clk_inf(DramGate, DeFe0Gate   );
    print_clk_inf(DramGate, DeFe1Gate   );
    print_clk_inf(DramGate, DeBe0Gate   );
    print_clk_inf(DramGate, DeBe1Gate   );
    print_clk_inf(DramGate, DeMpGate    );
    print_clk_inf(DramGate, AceGate     );

    printk("\nDeBe0Clk clk infor:\n");
    print_clk_inf(DeBe0Clk, ClkDiv      );
    print_clk_inf(DeBe0Clk, ClkSrc      );
    print_clk_inf(DeBe0Clk, Reset       );
    print_clk_inf(DeBe0Clk, SpecClkGate );

    printk("\nDeBe1Clk clk infor:\n");
    print_clk_inf(DeBe1Clk, ClkDiv      );
    print_clk_inf(DeBe1Clk, ClkSrc      );
    print_clk_inf(DeBe1Clk, Reset       );
    print_clk_inf(DeBe1Clk, SpecClkGate );

    printk("\nDeFe0Clk clk infor:\n");
    print_clk_inf(DeFe0Clk, ClkDiv      );
    print_clk_inf(DeFe0Clk, ClkSrc      );
    print_clk_inf(DeFe0Clk, Reset       );
    print_clk_inf(DeFe0Clk, SpecClkGate );

    printk("\nDeFe1Clk clk infor:\n");
    print_clk_inf(DeFe1Clk, ClkDiv      );
    print_clk_inf(DeFe1Clk, ClkSrc      );
    print_clk_inf(DeFe1Clk, Reset       );
    print_clk_inf(DeFe1Clk, SpecClkGate );

    printk("\nDeMpClk clk infor:\n");
    print_clk_inf(DeMpClk, ClkDiv       );
    print_clk_inf(DeMpClk, ClkSrc       );
    print_clk_inf(DeMpClk, Reset        );
    print_clk_inf(DeMpClk, SpecClkGate  );

    printk("\nLcd0Ch0Clk clk infor:\n");
    print_clk_inf(Lcd0Ch0Clk, ClkSrc        );
    print_clk_inf(Lcd0Ch0Clk, Reset         );
    print_clk_inf(Lcd0Ch0Clk, SpecClkGate   );

    printk("\nLcd1Ch0Clk clk infor:\n");
    print_clk_inf(Lcd1Ch0Clk, ClkSrc        );
    print_clk_inf(Lcd1Ch0Clk, Reset         );
    print_clk_inf(Lcd1Ch0Clk, SpecClkGate   );

    printk("\nCsiIspClk clk infor:\n");
    print_clk_inf(CsiIspClk, ClkDiv         );
    print_clk_inf(CsiIspClk, ClkSrc         );
    print_clk_inf(CsiIspClk, SpecClkGate    );

    printk("\nTvdClk clk infor:\n");
    print_clk_inf(TvdClk, ClkSrc        );
    print_clk_inf(TvdClk, SpecClkGate   );

    printk("\nLcd0Ch1Clk clk infor:\n");
    print_clk_inf(Lcd0Ch1Clk, ClkDiv        );
    print_clk_inf(Lcd0Ch1Clk, SpecClk1Src   );
    print_clk_inf(Lcd0Ch1Clk, SpecClk1Gate  );
    print_clk_inf(Lcd0Ch1Clk, SpecClk2Src   );
    print_clk_inf(Lcd0Ch1Clk, SpecClk2Gate  );

    printk("\nLcd1Ch1Clk clk infor:\n");
    print_clk_inf(Lcd1Ch1Clk, ClkDiv        );
    print_clk_inf(Lcd1Ch1Clk, SpecClk1Src   );
    print_clk_inf(Lcd1Ch1Clk, SpecClk1Gate  );
    print_clk_inf(Lcd1Ch1Clk, SpecClk2Src   );
    print_clk_inf(Lcd1Ch1Clk, SpecClk2Gate  );

    printk("\nCsi0Clk clk infor:\n");
    print_clk_inf(Csi0Clk, ClkDiv       );
    print_clk_inf(Csi0Clk, ClkSrc       );
    print_clk_inf(Csi0Clk, Reset        );
    print_clk_inf(Csi0Clk, SpecClkGate  );

    printk("\nCsi1Clk clk infor:\n");
    print_clk_inf(Csi1Clk, ClkDiv       );
    print_clk_inf(Csi1Clk, ClkSrc       );
    print_clk_inf(Csi1Clk, Reset        );
    print_clk_inf(Csi1Clk, SpecClkGate  );

    printk("\nVeClk clk infor:\n");
    print_clk_inf(VeClk, Reset          );
    print_clk_inf(VeClk, ClkDiv         );
    print_clk_inf(VeClk, SpecClkGate    );

    printk("\nAddaClk clk infor:\n");
    print_clk_inf(AddaClk, SpecClkGate  );

    printk("\nAvsClk clk infor:\n");
    print_clk_inf(AvsClk, SpecClkGate   );

    printk("\nAceClk clk infor:\n");
    print_clk_inf(AceClk, ClkDiv        );
    print_clk_inf(AceClk, Reset         );
    print_clk_inf(AceClk, ClkSrc        );
    print_clk_inf(AceClk, SpecClkGate   );

    printk("\nLvdsClk clk infor:\n");
    print_clk_inf(LvdsClk, Reset        );

    printk("\nHdmiClk clk infor:\n");
    print_clk_inf(HdmiClk, ClkDiv       );
    print_clk_inf(HdmiClk, ClkSrc       );
    print_clk_inf(HdmiClk, SpecClkGate  );

    printk("\nMaliClk clk infor:\n");
    print_clk_inf(MaliClk, ClkDiv       );
    print_clk_inf(MaliClk, ClkSrc       );
    print_clk_inf(MaliClk, Reset        );
    print_clk_inf(MaliClk, SpecClkGate  );
}
EXPORT_SYMBOL(clk_dbg_inf);

