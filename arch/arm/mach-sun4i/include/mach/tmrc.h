/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : tmrc.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-4-11 15:47
* Descript: timer register define.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __TMRC_H__
#define __TMRC_H__

typedef struct __TMRC_IRQ_EN_REG0000
{
    __u32   Tmr0IntEn:1;            //bit0,  Timer 0 interrupt enable, 0-no effect, 1-interrupt enable
    __u32   Tmr1IntEn:1;            //bit1,  Timer 1 interrupt enable, 0-no effect, 1-interrupt enable
    __u32   Tmr2IntEn:1;            //bit2,  Timer 2 interrupt enable, 0-no effect, 1-interrupt enable
    __u32   Tmr3IntEn:1;            //bit3,  Timer 3 interrupt enable, 0-no effect, 1-interrupt enable
    __u32   Tmr4IntEn:1;            //bit4,  Timer 4 interrupt enable, 0-no effect, 1-interrupt enable
    __u32   Tmr5IntEn:1;            //bit5,  Timer 5 interrupt enable, 0-no effect, 1-interrupt enable
    __u32   reserved0:2;            //bit6,  reserved
    __u32   WDogIntEn:1;            //bit8,  Watchdog interrupt enable, 0-No effect, 1-interrupt enable
    __u32   reserved1:23;           //bit9,  reserved

} __tmrc_irq_en_reg0000_t;


typedef struct __TMRC_IRQ_STAT_REG0004
{
    __u32   Tmr0IrqPend:1;          //bit0,  Timer 0 irq pendig, 0-No effect, 1-pending
    __u32   Tmr1IrqPend:1;          //bit1,  Timer 1 irq pendig, 0-No effect, 1-pending
    __u32   Tmr2IrqPend:1;          //bit2,  Timer 2 irq pendig, 0-No effect, 1-pending
    __u32   Tmr3IrqPend:1;          //bit3,  Timer 3 irq pendig, 0-No effect, 1-pending
    __u32   Tmr4IrqPend:1;          //bit4,  Timer 4 irq pendig, 0-No effect, 1-pending
    __u32   Tmr5IrqPend:1;          //bit5,  Timer 5 irq pendig, 0-No effect, 1-pending
    __u32   reserved0:2;            //bit6,  reserved
    __u32   WDogIrqPend:1;          //bit8,  Watchdog irq pendig, 0-No effect, 1-pending
    __u32   reserved1:23;           //bit9,  reserved

} __tmrc_irq_stat_reg0004_t;


typedef struct __TMRC_TMR01245_CTL_REG
{
    __u32   Enable:1;               //bit0,  Timer enable, 0-stop/pause, 1-start
    __u32   Reload:1;               //bit1,  Timer reload, 0-no effect, 1-reload
    __u32   ClkSrc:2;               //bit2,  Timer clock source, 00-LOSC, 01-HOSC,
                                    //       10-define as follow:
                                    //          PLL6/6          (timer0/1)
                                    //          reserved        (timer2)
                                    //          External CLKIN0 (timer4)
                                    //          External CLKIN1 (timer5)
                                    //       11-rerserved
    __u32   SclkDiv:3;              //bit4,  Select the pre-scale of timer clock source, div = 2^n
    __u32   Mode:1;                 //bit7,  Timer mode, 0-continous mode, 1-single mode
    __u32   reserved:24;            //bit8,  reserved
} __tmrc_tmr01245_ctl_reg_t;


typedef struct __TMRC_TMR3_CTL_REG0040
{
    __u32   Enable:1;               //bit0,  Timer 3 enable, 0-stop/pause, 1-start
    __u32   reserved0:1;            //bit1,  reserved
    __u32   SclkDiv:2;              //bit2,  Select the pre-scale of timer 3 clock source, div = 2^n
    __u32   Mode:1;                 //bit4,  Timer 3 mode, 0-continous mode, 1-single mode
    __u32   reserved1:27;           //bit5,  reserved
} __tmrc_tmr3_ctl_reg0040_t;


typedef struct __TMRC_AVSCNTR_CTL_REG0080
{
    __u32   Cnt0En:1;               //bit0,  avs counter 0 enable, 0-disable, 1-enable, couter source is HOSC(24Mhz)
    __u32   Cnt1En:1;               //bit1,  avs counter 1 enable, 0-disable, 1-enable, couter source is HOSC(24Mhz)
    __u32   reserved0:6;            //bit2,  reserved
    __u32   Cnt0Pause:1;            //bit8,  avs counter 1 pause, 0-not pause, 1-pause
    __u32   Cnt1Pause:1;            //bit9,  avs counter 1 pause, 0-not pause, 1-pause
    __u32   reserved1:22;           //bit10, reserved

}__tmrc_avscntr_ctl_reg0080_t;


typedef struct __TMRC_AVSCNTR_DIV_REG008C
{
    __u32   AvcCntr1Div:12;         //bit0,  division of avs counter 0
    __u32   reserved0:4;            //bit12, reserved
    __u32   AvcCntr1Div:12;         //bit16, division of avs counter 1
    __u32   reserved1:4;            //bit28, reserved
} __tmrc_avscntr_div_reg008c_t;


typedef struct __TMRC_WDOG_CTL_REG0090
{
    __u32   WDogRestart:1;          //bit0,  watch-dog restart, 0-no effect, 1-restart watch-dog
    __u32   KeyFiled:12;            //bit1,  keyfiled, should be 0xA57 for modify this register
    __u32   reserved:18;            //bit13, reserved
} __tmrc_wdog_ctl_reg0090_t;


typedef struct __TMRC_WDOG_MODE_REG0094
{
    __u32   WDogEn:1;               //bit0,  watch-dog enable, 0-no effect, 1-enable watch-dog
    __u32   WDogRstEn:1;            //bit1,  watch-dog reset enable, 0-no effect, 1-enable watch-dog active system reset
    __u32   reserved0:1;            //bit2,  reserved
    __u32   WDogIntVal:4;           //bit3,  watch-dog interval value, clock source is HOSC
                                    //       0000-0.5 second, 0001-1  second, 0010-2  second, 0011-3  second
                                    //       1000-4   second, 1001-5  second, 1010-6  second, 1011-8  second
                                    //       1100-10  second, 1101-12 second, 1110-14 second, 1111-16 second
    __u32   reserved1:24;           //bit7,  reserved
} __tmrc_wdog_mode_reg0094_t;


typedef struct __TMRC_64BCNTR_CTL_REG00A0
{
    __u32   ClearEn:1;              //bit0,  64-bit counter clear enable, 0-no effect, 1-clear, it will be 0 affter cleared
    __u32   LatchEn:1;              //bit1,  64-bit counter read latch enable, 0-no effect, 1-latched, it will be 0 affter latched
    __u32   ClkSrc:1;               //bit2,  64-bit counter clock source select, 0-HOSC, 1-PLL6/6
    __u32   reserved:29;            //bit3,  reserved
} __tmrc_64bcntr_ctl_reg00a0_t;


typedef struct __TMRC_LOSC_CTL_REG0100
{
    __u32   LoscSrc:1;              //bit0,  LOSC clock source select, 0-internal OSC, 1-external OSC
    __u32   reserved0:1;            //bit1,  reserved
    __u32   ExtLoscGsm:2;           //bit2,  external LOSC GSM
    __u32   reserved1:3;            //bit4,  reserved
    __u32   RtcYMDAcs:1;            //bit7,  RTC YY-MM-DD access, the bit is set untill the real writing is finish
    __u32   RtcHMSAcs:1;            //bit8,  RTC HH-MM-SS access, the bit is set untill the real writing is finish
    __u32   AlarmAcs:1;             //bit9,  alarm DD-HH-MM-SS access, the bit is set untill the real writing is finish
    __u32   reserved2:4;            //bit10, reserved
    __u32   LoscSwitchEn:1;         //bit14, CLK32K auto switch enable
    __u32   LoscSwitchPend:1;       //bit15, CLK32K auto switch pending
    __u32   KeyFiled:16;            //bit16, key filed for modify the register, should be 0x16AA
} __tmrc_losc_ctl_reg0100_t;



typedef struct __TMRC_RTC_YMD_REG0104
{
    __u32   Day:5;                  //bit0, day, range from 1~31
    __u32   reserved0:3;            //bit5,  reserved
    __u32   Month:4;                //bit8,  month, range from 1~12
    __u32   reserved1:4;            //bit12, reserved
    __u32   Year:6;                 //bit16, year, range from 0~63
    __u32   LeapYear:1;             //bit22, Leap year, 0-not, 1-leap year, can't set by hardware
    __u32   reserved2:7;            //bit23, reserved
    __u32   SimulatCtl:1;           //bit30, RTC simulation control bit
    __u32   TstModeCtl:1;           //bit31, RTC test mode control bit
} __tmrc_rtc_ymd_reg0104_t;

typedef struct __TMRC_RTC_HMS_REG0108
{
    __u32   Second:6;               //bit0,  second, range from 0~59
    __u3    reserved0:2;            //bit6,  reserved
    __u32   Minute:6;               //bit8,  minute, reange from 0~59
    __u32   reserved1:2;            //bit14, reserved
    __u32   Hour:5;                 //bit16, hour, range from 0~23
    __u32   reserved2:8;            //bit21, reserved
    __u32   WeekNum:3;              //bit29, week number, 0~6 -> Monday~Sunday
} __tmrc_rtc_hms_reg0108_t;


typedef struct __TMRC_ALARM_DHMS_REG010C
{
    __u32   Second:6;               //bit0,  second, range from 0~59
    __u32   reserved0:2;            //bit6,  reserved
    __u32   Minute:5;               //bit8,  minute, range from 0~59
    __u32   reserved1:2;            //bit14, reserved
    __u32   Hour:5;                 //bit16, hour, range from 0~23
    __u32   Day:11;                 //bit21, day, range from 0~1023
} __tmrc_alarm_dhms_reg010c_t;


typedef struct __TMRC_ALARM_WHMS_REG0110
{
    __u32   Second:6;               //bit0,  second, range from 0~59
    __u32   reserved0:2;            //bit6,  reserved
    __u32   Minute:5;               //bit8,  minute, range from 0~59
    __u32   reserved1:2;            //bit14, reserved
    __u32   Hour:5;                 //bit16, hour, range from 0~23
    __u32   reserved2:11;           //bit21, reserved
} __tmrc_alarm_whms_reg0110_t;


typedef struct __TMRC_ALARM_CTL_REG0114
{

    __u32   MonAlarmEn:1;           //bit0,  Monday alarm enable
    __u32   TueAlarmEn:1;           //bit1,  Tuesday alarm enable
    __u32   WedAlarmEn:1;           //bit2,  Wednesday alarm enable
    __u32   ThuAlarmEn:1;           //bit3,  Thursday alarm enable
    __u32   FriAlarmEn:1;           //bit4,  Friday alarm enable
    __u32   SatAlarmEn:1;           //bit5,  Saturday alarm enable
    __u32   SunAlarmEn:1;           //bit6,  Sunday alarm enable
    __u32   reserved0:1;            //bit7,  reserved
    __u32   AlarmEn:8;              //bit8,  alarm counter enable, 0-disable, 1-enable
    __u32   reserved1:23;           //bit9,  reserved
} __tmrc_alarm_ctl_reg0114_t;


typedef struct __TMRC_ALARM_IRQEN_REG0118
{
    __u32   CntrIrqEn:1;            //bit0,  alarm counter irq enable
    __u32   WeekIrqEn:1;            //bit1,  alarm week irq enable
    __u32   reserved:30;            //bit2,  reserved
} __tmrc_alarm_irqen_reg0118_t;


typedef struct __TMRC_ALARM_IRQPEND_REG011C
{
    __u32   CntrIrqPend:1;          //bit0,  alarm counter irq pending
    __u32   WeekIrqPend:1;          //bit1,  alarm week irq pending
    __u32   reserved:30;            //bit2,  reserved
} __tmrc_alarm_irqpend_reg011c_t;


typedef struct __TMRC_AC328_CFG_REG013C
{
    __u32   L2DCacheInv:1;          //bit0, enable L2 data cache invalidation at reset
    __u32   L1DCacheInv:1;          //bit1,  enable L1 data cache invalidation at reset
    __u32   reserved:30;            //bit2,  reserved

} __tmrc_ac328_cfg_reg013c_t;



typedef struct __TMRC_REG_LIST
{
    //timer top control
    volatile __tmrc_irq_en_reg0000_t        TmrIrqEn;       //0x0000, timer irq enable
    volatile __tmrc_irq_stat_reg0004_t      TmrIrqStat;     //0x0004, timer irq status
    volatile __u32                          reserved0[2];   //0x0008, reserved

    //timer 0 registers
    volatile __tmrc_tmr01245_ctl_reg_t      Tmr0Ctl;        //0x0010, timer 0 control register
    volatile __u32                          Tmr0IntVal;     //0x0014, timer 0 interval value
    volatile __u32                          Tmr0CurVal;     //0x0018, timer 0 current value
    volatile __u32                          reserved1;      //0x001C, reserved

    //timer1 registers
    volatile __tmrc_tmr01245_ctl_reg_t      Tmr1Ctl;        //0x0020, timer 1 control register
    volatile __u32                          Tmr1IntVal;     //0x0024, timer 1 interval value
    volatile __u32                          Tmr1CurVal;     //0x0028, timer 1 current value
    volatile __u32                          reserved2;      //0x002C, reserved

    //timer2 registers
    volatile __tmrc_tmr01245_ctl_reg_t      Tmr2Ctl;        //0x0030, timer 2 control register
    volatile __u32                          Tmr2IntVal;     //0x0034, timer 2 interval value
    volatile __u32                          Tmr2CurVal;     //0x0038, timer 2 current value
    volatile __u32                          reserved3;      //0x003C, reserved

    //timer3 register
    volatile __tmrc_tmr3_ctl_reg0040_t      Tmr3Ctr;        //0x0040, timer 3 control register
    volatile __u32                          Tmr3IntVal;     //0x0044, timer 3 interval value
    volatile __u32                          reserved4[2];   //0x0048, reserved

    //timer4 registers
    volatile __tmrc_tmr01245_ctl_reg_t      Tmr4Ctl;        //0x0050, timer 4 control register
    volatile __u32                          Tmr4IntVal;     //0x0054, timer 4 interval value
    volatile __u32                          Tmr4CurVal;     //0x0058, timer 4 current value
    volatile __u32                          reserved5;      //0x005C, reserved

    //timer5 registers
    volatile __tmrc_tmr01245_ctl_reg_t      Tmr5Ctl;        //0x0060, timer 5 control register
    volatile __u32                          Tmr5IntVal;     //0x0064, timer 5 interval value
    volatile __u32                          Tmr5CurVal;     //0x0068, timer 5 current value
    volatile __u32                          reserved6;      //0x006C, reserved

    volatile __u32                          reserved7[4];   //0x0070, reserved

    //vas counter registers
    volatile __tmrc_avscntr_ctl_reg0080_t   AvsCtl;         //0x0080, avs counter control register
    volatile __u32                          Avs0Val;        //0x0084, avs counter 0 value, high 32bits of a 33bits register
    volatile __u32                          Avs1Val;        //0x0088, avs counter 1 value, high 32bits of a 33bits register
    volatile __tmrc_avscntr_div_reg008c_t   AvsDiv;         //0x008C, avs counter division register

    //watch-dog registers
    volatile __tmrc_wdog_ctl_reg0090_t      WdogCtl;        //0x0090, watch-dog control register
    volatile __tmrc_wdog_mode_reg0094_t     WdogMode;       //0x0094, watch-dog mode register
    volatile __u32                          reserved8[2];

    //64bits counter registers
    volatile __tmrc_64bcntr_ctl_reg00a0_t   LCntrCtl;       //0x00A0, 64-bits counter control register
    volatile __u32                          LCntrLow;       //0x00A4, low 32bits value of the 64-bits counter
    volatile __u32                          LcntrHigh;      //0x00A8, high 32bits value of teh 64-bits counter
    volatile __u32                          reserved9;      //0x00AC, reserved

    volatile __u32                          reserved10[20]; //0x00B0, reserved

    //rtc registers
    volatile __tmrc_losc_ctl_reg0100_t      LoscCtl;        //0x0100, LOSC control
    volatile __tmrc_rtc_ymd_reg0104_t       RtcYmd;         //0x0104, RTC Year-Month-Day
    volatile __tmrc_rtc_hms_reg0108_t       RtcHms;         //0x0108, RTC Hour-minute-second
    volatile __tmrc_alarm_dhms_reg010c_t    AlarmDhms;      //0x010C, Alarm Day-Hour-Minute-Second
    volatile __tmrc_alarm_whms_reg0110_t    AlarmWhms;      //0x0110, Alarm Week-Hour-Minute-Second
    volatile __tmrc_alarm_ctl_reg0114_t     AlarmCtl;       //0x0114, Alarm Enable
    volatile __tmrc_alarm_irqen_reg0118_t   AlarmIrqEn;     //0x0118, Alarm Irq enable
    volatile __tmrc_alarm_irqpend_reg011c_t AlarmIrqPend;   //0x011C, Alarm IRQ status
    volatile __u32                          TmrGPReg0;      //0x0120, timer general purpose register 0
    volatile __u32                          TmrGPReg1;      //0x0124, timer general purpose register 1
    volatile __u32                          TmrGPReg2;      //0x0128, timer general purpose register 2
    volatile __u32                          TmrGPReg3;      //0x012C, timer general purpose register 3
    volatile __u32                          reserved11[4];  //0x0130, reserved
    volatile __tmrc_ac328_cfg_reg013c_t     Ac328Cfg;       //0x0140, AC328 configuration register


} __tmrc_reg_list_t;


#endif  // #ifndef __TMRC_H__

