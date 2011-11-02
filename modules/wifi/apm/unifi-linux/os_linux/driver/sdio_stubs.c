/*
 * Stubs for some of the bottom edge functions.
 *
 * These stubs are optional functions in the bottom edge (SDIO driver
 * interface) API that not all platforms or SDIO drivers may support.
 *
 * They're declared as weak symbols so they can be overridden by
 * simply providing a non-weak declaration.
 *
 * Copyright (C) 2007-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 */
#include "driver/unifi.h"

void __attribute__((weak)) CsrSdioFunctionIdle(CsrSdioFunction *function)
{
}

void __attribute__((weak)) CsrSdioFunctionActive(CsrSdioFunction *function)
{
}

CsrInt32 __attribute__((weak)) CsrSdioPowerOn(CsrSdioFunction *function)
{
    return CSR_SDIO_RESULT_DEVICE_WAS_RESET;
}

void __attribute__((weak)) CsrSdioPowerOff(CsrSdioFunction *function)
{
}

CsrInt32 __attribute__((weak)) CsrSdioHardReset(CsrSdioFunction *function)
{
    return 1;
}

CsrInt32 __attribute__((weak)) CsrSdioBlockSizeSet(CsrSdioFunction *function,
                                                   CsrUint16 blockSize)
{
    return 0;
}

CsrInt32 __attribute__((weak)) CsrSdioSuspend(CsrSdioFunction *function)
{
    return 0;
}

CsrInt32 __attribute__((weak)) CsrSdioResume(CsrSdioFunction *function)
{
    return 0;
}

int __attribute__((weak)) csr_sdio_linux_install_irq(CsrSdioFunction *function)
{
    return 0;
}

int __attribute__((weak)) csr_sdio_linux_remove_irq(CsrSdioFunction *function)
{
    return 0;
}

