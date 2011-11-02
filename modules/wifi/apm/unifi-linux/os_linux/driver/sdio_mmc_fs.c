/*
 * ---------------------------------------------------------------------------
 *
 * FILE: sdio_mmc_fs.c
 *
 *  This file provides the portable driver bottom-edge API implementation
 *  for the Freescale imx31ads platform, using the Freescale SDIO glue module.
 *
 * Copyright (C) 2007-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include "unifi_priv.h"

#include "sdio_freescale/fs_sdio_api.h"

static CsrSdioFunctionDriver *sdio_func_drv;


/* MMC uses ENOMEDIUM to indicate card gone away */
static inline CsrInt32 convert_sdio_error(int r)
{
    switch (r) {
        case 0:
            return 0;
        case -ENOMEDIUM:
            return -CSR_ENODEV;
        case -EIO:
            return -CSR_EIO;
        case -ENODEV:
            return -CSR_ENODEV;
        case -ENOMEM:
            return -CSR_ENOMEM;
        case -EINVAL:
            return -CSR_EINVAL;
        case -ETIMEDOUT:
            return -CSR_ETIMEDOUT;
        default:
            return -CSR_EIO;
    }
}

CsrInt32
CsrSdioRead8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err = 0;

    err = fs_sdio_readb(fdev, 1, address, data);

    return convert_sdio_error(err);
} /* CsrSdioRead8() */

CsrInt32
CsrSdioWrite8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err = 0;

    err = fs_sdio_writeb(fdev, 1, address, data);

    return convert_sdio_error(err);
} /* CsrSdioWrite8() */

CsrInt32
CsrSdioRead16(CsrSdioFunction *function, CsrUint32 address, CsrUint16 *data)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err;
    CsrUint8 b0, b1;

    err = fs_sdio_readb(fdev, 1, address, &b0);
    if (err) {
        return convert_sdio_error(err);
    }

    err = fs_sdio_readb(fdev, 1, address+1, &b1);
    if (err) {
        return convert_sdio_error(err);
    }

    *data = ((CsrUint16)b1 << 8) | b0;

    return 0;
} /* CsrSdioRead16() */


CsrInt32
CsrSdioWrite16(CsrSdioFunction *function, CsrUint32 address, CsrUint16 data)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err;
    uint8_t b0, b1;

    b1 = (data >> 8) & 0xFF;
    err = fs_sdio_writeb(fdev, 1, address+1, b1);
    if (err) {
        return convert_sdio_error(err);
    }

    b0 = data & 0xFF;
    err = fs_sdio_writeb(fdev, 1, address, b0);
    if (err) {
        return convert_sdio_error(err);
    }

    return 0;
} /* CsrSdioWrite16() */


CsrInt32
CsrSdioF0Read8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err = 0;

    err = fs_sdio_readb(fdev, 0, address, data);

    return convert_sdio_error(err);
} /* CsrSdioF0Read8() */

CsrInt32
CsrSdioF0Write8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err = 0;

    err = fs_sdio_writeb(fdev, 0, address, data);

    return convert_sdio_error(err);
} /* CsrSdioF0Write8() */


CsrInt32
CsrSdioRead(CsrSdioFunction *function, CsrUint32 address, void *data, CsrUint32 length)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err;

    err = fs_sdio_block_rw(fdev, 1, address, data, length, 0);

    return convert_sdio_error(err);
} /* CsrSdioRead() */

CsrInt32
CsrSdioWrite(CsrSdioFunction *function, CsrUint32 address, const void *data, CsrUint32 length)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err;

    err = fs_sdio_block_rw(fdev, 1, address, (unsigned char *)data, length, 1);

    return convert_sdio_error(err);
} /* CsrSdioWrite() */


CsrInt32
CsrSdioInterruptEnable(CsrSdioFunction *function)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err = 0;

    err = fs_sdio_enable_interrupt(fdev, 1);

    return convert_sdio_error(err);
} /* CsrSdioInterruptEnable() */

CsrInt32
CsrSdioInterruptDisable(CsrSdioFunction *function)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err = 0;

    err = fs_sdio_enable_interrupt(fdev, 0);

    return convert_sdio_error(err);
} /* CsrSdioInterruptDisable() */


void CsrSdioInterruptAcknowledge(CsrSdioFunction *function)
{
}

CsrInt32 
CsrSdioFunctionEnable(CsrSdioFunction *function)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int err;

    err = fs_sdio_enable(fdev);

    return convert_sdio_error(err);
} /* CsrSdioFunctionEnable() */


CsrInt32
CsrSdioFunctionDisable(CsrSdioFunction *function)
{
    return CSR_ENOTSUP;
} /* CsrSdioFunctionDisable() */

void
CsrSdioFunctionActive(CsrSdioFunction *function)
{
} /* CsrSdioFunctionActive() */

void
CsrSdioFunctionIdle(CsrSdioFunction *function)
{
} /* CsrSdioFunctionIdle() */


CsrInt32
CsrSdioBlockSizeSet(CsrSdioFunction *function, CsrUint16 blockSize)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int r = 0;

    /* Module parameter overrides */
    if (sdio_block_size > -1) {
        blockSize = sdio_block_size;
    }

    unifi_trace(NULL, UDBG1, "Set SDIO function block size to %d\n", blockSize);
    r = fs_sdio_set_block_size(fdev, blockSize);
    if (r) {
        unifi_error(NULL, "Error %d setting block size\n", r);
    }

    function->blockSize = fdev->max_blocksize;

    return convert_sdio_error(r);
} /* CsrSdioBlockSizeSet() */

CsrInt32
CsrSdioPowerOn(CsrSdioFunction *function)
{
    return CSR_SDIO_RESULT_DEVICE_WAS_RESET;
} /* CsrSdioPowerOn() */

void
CsrSdioPowerOff(CsrSdioFunction *function)
{
} /* CsrSdioPowerOff() */

CsrInt32
CsrSdioHardReset(CsrSdioFunction *function)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int r;

    /* Hard reset can be disabled by a module parameter */
    r = 1;
    if (disable_hw_reset != 1) {
        r = fs_sdio_hard_reset(fdev); /* may return 1 if can't reset */
        if (r < 0) {
            return convert_sdio_error(r);   /* fatal error */
        }
    }

    return convert_sdio_error(r);
} /* CsrSdioHardReset() */

CsrInt32
csr_sdio_set_max_clock_speed(CsrSdioFunction *function, CsrUint32 max_khz)
{
    struct sdio_dev *fdev = (struct sdio_dev *)function->priv;
    int r;

    /* Respect the max set by the module parameter. */
    if (!max_khz || max_khz > sdio_clock) {
        max_khz = sdio_clock;
    }

    r = fs_sdio_set_max_clock_speed(fdev, max_khz);
    if (r < 0) {
        return convert_sdio_error(r);
    }

    return 0;
} /* unifi_sdio_set_max_clock_speed() */


#ifdef CONFIG_PM
static void
uf_glue_sdio_suspend(struct sdio_dev *fdev, pm_message_t state)
{
    CsrSdioFunction *sdio_ctx = (CsrSdioFunction *)fdev->drv_data;;

    func_enter();

    unifi_trace(NULL, UDBG1, "System Suspend...\n");

    /* Clean up the SDIO function driver */
    if (sdio_func_drv && sdio_func_drv->suspend) {
        sdio_func_drv->suspend(sdio_ctx);
    }

    func_exit();

} /* uf_glue_sdio_suspend */


static void
uf_glue_sdio_resume(struct sdio_dev *fdev)
{
    CsrSdioFunction *sdio_ctx = (CsrSdioFunction *)fdev->drv_data;;

    func_enter();

    unifi_trace(NULL, UDBG1, "System Resume...\n");

    /* Clean up the SDIO function driver */
    if (sdio_func_drv && sdio_func_drv->resume) {
        sdio_func_drv->resume(sdio_ctx);
    }

    func_exit();

} /* uf_glue_sdio_resume */
#else
#define uf_glue_sdio_suspend  NULL
#define uf_glue_sdio_resume   NULL
#endif

static void
uf_glue_sdio_int_handler(struct sdio_dev *fdev)
{
    CsrSdioDsrCallback func_dsr_callback;
    CsrSdioFunction *sdio_ctx;

    sdio_ctx = (CsrSdioFunction *)fdev->drv_data;;

    /* If the function driver has registered a handler, call it */
    if (sdio_func_drv && sdio_func_drv->interrupt) {

        func_dsr_callback = sdio_func_drv->interrupt(sdio_ctx);

        /* If interrupt handle returns a DSR handle, call it */
        if (func_dsr_callback) {
            func_dsr_callback(sdio_ctx, 0);
        }
    }

} /* uf_glue_sdio_int_handler() */


static int
uf_glue_sdio_probe(struct sdio_dev *fdev)
{
    int r;
    int instance = 0;
    CsrSdioFunction *sdio_ctx;

    func_enter();

    /* Assumes one card per host, which is true for SDIO */
    printk("UniFi card 0x%X inserted\n", instance);

    /* Allocate context */
    sdio_ctx = (CsrSdioFunction *)kmalloc(sizeof(CsrSdioFunction),
                                          GFP_KERNEL);
    if (sdio_ctx == NULL) {
        return -ENOMEM;
    }

    /* Initialise the context */
    sdio_ctx->sdioId.manfId  = fdev->vendor_id;
    sdio_ctx->sdioId.cardId  = fdev->device_id;
    sdio_ctx->sdioId.sdioFunction  = 1;
    sdio_ctx->sdioId.sdioInterface = 0;
    sdio_ctx->blockSize = fdev->max_blocksize;
    sdio_ctx->priv = (void *)fdev;
    sdio_ctx->features = 0;

    /* Module parameter enables byte mode */
    if (sdio_byte_mode) {
        sdio_ctx->features |= CSR_SDIO_FEATURE_BYTE_MODE;
    }

    /* Pass context to the SDIO driver */
    fdev->drv_data = sdio_ctx;

    /* Register this device with the SDIO function driver */
    /* Call the main UniFi driver inserted handler */
    r = -EINVAL;
    if (sdio_func_drv && sdio_func_drv->inserted) {
        uf_add_os_device(instance, NULL);
        r = sdio_func_drv->inserted(sdio_ctx);
    }

    func_exit();
    return r;
} /* uf_glue_sdio_probe() */


static void
uf_glue_sdio_remove(struct sdio_dev *fdev)
{
    CsrSdioFunction *sdio_ctx = (CsrSdioFunction *)fdev->drv_data;;
    func_enter();

    unifi_info(NULL, "UniFi card removed\n");

    /* Clean up the SDIO function driver */
    if (sdio_func_drv && sdio_func_drv->removed) {
        uf_remove_os_device(0);
        sdio_func_drv->removed(sdio_ctx);
    }

    kfree(sdio_ctx);

    func_exit();

} /* uf_glue_sdio_remove */


static struct fs_driver fs_driver = {
	.name	= "fs_driver",
	.probe		= uf_glue_sdio_probe,
	.remove		= uf_glue_sdio_remove,
	.card_int_handler = uf_glue_sdio_int_handler,
	.suspend	= uf_glue_sdio_suspend,
	.resume		= uf_glue_sdio_resume,
};


CsrInt32
CsrSdioFunctionDriverRegister(CsrSdioFunctionDriver *sdio_drv)
{
    int r;

    printk("Unifi: Using MMC/SDIO glue driver\n");

    if (sdio_func_drv) {
        unifi_error(NULL, "sdio_mmc_fs: UniFi driver already registered\n");
        return -EINVAL;
    }

    /* Save the registered driver description */
    /* 
     * FIXME:
     * Need a table here to handle a call to register for just one function.
     * mmc only allows us to register for the whole device
     */
    sdio_func_drv = sdio_drv;

    /* Register ourself with mmc_core */
    r = fs_sdio_register_driver(&fs_driver);
    if (r) {
        printk(KERN_ERR "unifi_sdio: Failed to register UniFi SDIO driver: %d\n", r);
        return convert_sdio_error(r);
    }

    return 0;
} /* CsrSdioFunctionDriverRegister() */



void
CsrSdioFunctionDriverUnregister(CsrSdioFunctionDriver *sdio_drv)
{
    printk(KERN_INFO "UniFi: unregister from sdio\n");

    fs_sdio_unregister_driver(&fs_driver);

    sdio_func_drv = NULL;
} /* CsrSdioFunctionDriverUnregister() */
