// -*- linux-c -*-
////////////////////////////////////////////////////////////////////////////////
//
// stmp36xx_gpio.h
//
////////////////////////////////////////////////////////////////////////////////
//
// Description:
//
//   This include file is intended to make GPIO manipulation under
//   STMP36xx a bit more convenient.
//
//   Honestly, I'm mostly just using it for debug purposes to blink
//   some LEDs, but it is potentially useful for all kinds of dirty
//   bit banging.  Regardless, right now it's just a gold-plated
//   toilet.
//
////////////////////////////////////////////////////////////////////////////////
//
// Acknowledgements:
//
//   Thanks to Pat Maupin for showing me the helper macro trick to get
//   the double expansion to work like I wanted it to
//
////////////////////////////////////////////////////////////////////////////////
//
// Credits:
//
//   Written by Tahoma Toelkes, <ttoelkes@sigmatel.com>
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) SigmaTel, Inc. Unpublished
//
// SigmaTel, Inc.
// Proprietary & Confidential
//
// This source code and the algorithms implemented therein constitute
// confidential information and may compromise trade secrets of SigmaTel, Inc.
// or its associates, and any unauthorized use thereof is prohibited.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _STMP36XX_GPIO_H_
#define _STMP36XX_GPIO_H_


// -----------------------------------------------------------------------------
// includes
// -----------------------------------------------------------------------------

#include <asm/hardware.h>
#include <asm/arch/regspinctrl.h>


// -----------------------------------------------------------------------------
// constant definitions
//
// The three PWM channels below are very useful for LED blinking on
// the Armadillo engineering board in combination with the macros
// defined further down.
//
// One of these days, if I am bored and need typing exercise, perhaps
// I will add more of these definitions.  Right...
//
// Yes, by the way, having to define the mux number sucks.  Calculating
// the number to substitute into the later symbol concatenations is
// territory where, as far as I know, C preprocessor magic cannot go.
// -----------------------------------------------------------------------------

#define STMP36XX_GPIO_PWM4_BANK         3
#define STMP36XX_GPIO_PWM4_BIT         14
#define STMP36XX_GPIO_PWM4_MUX          6
#define STMP36XX_GPIO_PWM4_FN           0

#define STMP36XX_GPIO_PWM3_BANK         3
#define STMP36XX_GPIO_PWM3_BIT         13
#define STMP36XX_GPIO_PWM3_MUX          6
#define STMP36XX_GPIO_PWM3_FN           0

#define STMP36XX_GPIO_PWM2_BANK         3
#define STMP36XX_GPIO_PWM2_BIT         12
#define STMP36XX_GPIO_PWM2_MUX          6
#define STMP36XX_GPIO_PWM2_FN           0

#define STMP36XX_GPIO_SSP_DETECT_BANK   0
#define STMP36XX_GPIO_SSP_DETECT_BIT   25
#define STMP36XX_GPIO_SSP_DETECT_MUX    1
#define STMP36XX_GPIO_SSP_DETECT_FN     0

#define STMP36XX_GPIO_SSP_SCK_BANK      0
#define STMP36XX_GPIO_SSP_SCK_BIT      27
#define STMP36XX_GPIO_SSP_SCK_MUX       1
#define STMP36XX_GPIO_SSP_SCK_FN        0

#define STMP36XX_GPIO_SSP_CMD_BANK      0
#define STMP36XX_GPIO_SSP_CMD_BIT      26
#define STMP36XX_GPIO_SSP_CMD_MUX       1
#define STMP36XX_GPIO_SSP_CMD_FN        0

#define STMP36XX_GPIO_SSP_DATA0_BANK    0
#define STMP36XX_GPIO_SSP_DATA0_BIT    28
#define STMP36XX_GPIO_SSP_DATA0_MUX     1
#define STMP36XX_GPIO_SSP_DATA0_FN      0

#define STMP36XX_GPIO_SSP_DATA1_BANK    0
#define STMP36XX_GPIO_SSP_DATA1_BIT    29
#define STMP36XX_GPIO_SSP_DATA1_MUX     1
#define STMP36XX_GPIO_SSP_DATA1_FN      0

#define STMP36XX_GPIO_SSP_DATA2_BANK    0
#define STMP36XX_GPIO_SSP_DATA2_BIT    30
#define STMP36XX_GPIO_SSP_DATA2_MUX     1
#define STMP36XX_GPIO_SSP_DATA2_FN      0

#define STMP36XX_GPIO_SSP_DATA3_BANK    0
#define STMP36XX_GPIO_SSP_DATA3_BIT    31
#define STMP36XX_GPIO_SSP_DATA3_MUX     1
#define STMP36XX_GPIO_SSP_DATA3_FN      0


// -----------------------------------------------------------------------------
// symbolic macros
//
// These macros take the symbolic name of the pin you wish to
// manipulate.  This is probably what you'll want to use, due to their
// (relative) brevity.
//
// For example (turn on LED attached to PWM4):
//
//      STMP36XX_GPIO_PCFG_OUT_4MA(PWM4);
//      STMP36XX_GPIO_PSET(PWM4);
//
// -----------------------------------------------------------------------------

#define STMP36XX_GPIO_PCFG_OUT_4MA(pin)                           \
        STMP36XX_GPIO_CFG_OUT_4MA(STMP36XX_GPIO_##pin##_BANK,     \
                                  STMP36XX_GPIO_##pin##_BIT,      \
                                  STMP36XX_GPIO_##pin##_MUX)

#define STMP36XX_GPIO_PCFG_OUT_8MA(pin)                           \
        STMP36XX_GPIO_CFG_OUT_8MA(STMP36XX_GPIO_##pin##_BANK,     \
                                  STMP36XX_GPIO_##pin##_BIT,      \
                                  STMP36XX_GPIO_##pin##_MUX)

#define STMP36XX_GPIO_PSET(pin)                         \
        STMP36XX_GPIO_SET(STMP36XX_GPIO_##pin##_BANK,   \
                          STMP36XX_GPIO_##pin##_BIT)

#define STMP36XX_GPIO_PCLR(pin)                         \
        STMP36XX_GPIO_CLR(STMP36XX_GPIO_##pin##_BANK,   \
                          STMP36XX_GPIO_##pin##_BIT)

#define STMP36XX_GPIO_PTOG(pin)                         \
        STMP36XX_GPIO_TOG(STMP36XX_GPIO_##pin##_BANK,   \
                          STMP36XX_GPIO_##pin##_BIT)


#define STMP36XX_GPIO_PCFG_IN(pin)                              \
        STMP36XX_GPIO_CFG_IN(STMP36XX_GPIO_##pin##_BANK,        \
                             STMP36XX_GPIO_##pin##_BIT,         \
                             STMP36XX_GPIO_##pin##_MUX)

#define STMP36XX_GPIO_PGET(pin)                                 \
        STMP36XX_GPIO_GET(STMP36XX_GPIO_##pin##_BANK,           \
                          STMP36XX_GPIO_##pin##_BIT)


// -----------------------------------------------------------------------------
// value macros
//
// These macros take numeric value arguments (or macros that expand
// into numeric arguments) for the bank and bit of the pin you wish to
// manipulate.  These might be quite useful for indexed GPIO
// operation.
//
// For example (again, turning on LED attached to PWM4):
//
//      STMP36XX_GPIO_CFG_OUT_4MA(3, 12, 6);
//      STMP36XX_GPIO_SET(STMP36XX_GPIO_PWM4_BANK, STMP36XX_GPIO_PWM4_BIT);
//
// -----------------------------------------------------------------------------

#define STMP36XX_GPIO_CFG_OUT_4MA(bank, bit, mux)       \
        _STMP36XX_GPIO_CFG_OUT_4MA(bank, bit, mux)

#define STMP36XX_GPIO_CFG_OUT_8MA(bank, bit, mux)       \
        _STMP36XX_GPIO_CFG_OUT_8MA(bank, bit, mux)

#define STMP36XX_GPIO_SET(bank, bit)            \
        _STMP36XX_GPIO_SET(bank, bit)

#define STMP36XX_GPIO_CLR(bank, bit)            \
        _STMP36XX_GPIO_CLR(bank, bit)

#define STMP36XX_GPIO_TOG(bank, bit)            \
        _STMP36XX_GPIO_TOG(bank, bit)


#define STMP36XX_GPIO_CFG_IN(bank, bit, mux)    \
        _STMP36XX_GPIO_CFG_IN(bank, bit, mux)

#define STMP36XX_GPIO_GET(bank, bit)            \
        _STMP36XX_GPIO_GET(bank, bit)


// -----------------------------------------------------------------------------
// helper definitions and macros
//
// Do not use these directly, or else you might have a black hole
// spontaneously appear on your desk (at which point, simulations
// suggest that it will take about 60 seconds worth of acceleration
// for the Earth to compress down to infinitesimal size...of course,
// you'll probably be out of your misery due to the G shock and
// subsequent implosion of the flesh well before then.  On the other
// hand, if this is true or not depends on the answer to the
// philisophical question of whether biological building blocks have
// an absolute reference or whether relativity has an infinitely
// sliding scale).  Either way, I suggest you play it safe and use the
// macros provided above.
// ------------------------------------------------------------------------------

#define _STMP36XX_GPIO_FN (0x3)

#define _STMP36XX_GPIO_CFG_OUT_4MA(bank, bit, mux)                      \
        (HW_PINCTRL_DRIVE##bank##_CLR(0x1 << (bit)),                    \
         HW_PINCTRL_DOE##bank##_SET(0x1 << (bit)),                      \
         HW_PINCTRL_MUXSEL##mux##_SET(_STMP36XX_GPIO_FN << ((2 * (bit)) % 32)))

#define _STMP36XX_GPIO_CFG_OUT_8MA(bank, bit, mux)                      \
        (HW_PINCTRL_DRIVE##bank##_SET(0x1 << (bit)),                    \
         HW_PINCTRL_DOE##bank##_SET(0x1 << (bit)),                      \
         HW_PINCTRL_MUXSEL##mux##_SET(_STMP36XX_GPIO_FN << ((2 * (bit)) % 32)))

#define _STMP36XX_GPIO_SET(bank, bit)                   \
        HW_PINCTRL_DOUT##bank##_SET(0x1 << (bit))

#define _STMP36XX_GPIO_CLR(bank, bit)                   \
        HW_PINCTRL_DOUT##bank##_CLR(0x1 << (bit))

#define _STMP36XX_GPIO_TOG(bank, bit)                   \
        HW_PINCTRL_DOUT##bank##_TOG(0x1 << (bit))

#define _STMP36XX_GPIO_CFG_IN(bank, bit, mux)                           \
        (HW_PINCTRL_DOE##bank##_CLR(0x1 << (bit)),                      \
         HW_PINCTRL_MUXSEL##mux##_SET(_STMP36XX_GPIO_FN << ((2 * (bit)) % 32)))

#define _STMP36XX_GPIO_GET(bank, bit)                           \
        (HW_PINCTRL_DIN##bank##_RD(0x1 << (bit)) >> (bit))


#endif // _STMP36XX_GPIO_H_

////////////////////////////////////////////////////////////////////////////////
