/*
 * ---------------------------------------------------------------------------
 *
 * FILE: sdio_emb.c
 *
 * PURPOSE: Driver instantiation and deletion for SDIO on Linux.
 *
 *      This file brings together the SDIO bus interface, the UniFi
 *      driver core and the Linux net_device stack.
 *
 * Copyright (C) 2007-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include <linux/kmod.h>
#include <linux/init.h>
#include "driver/unifi.h"
#include "unifi_priv.h"

#include "sdioemb/sdio_api.h"

/* The function driver context, i.e the UniFi Driver */
static CsrSdioFunctionDriver *sdio_func_drv;



/* sdioemb driver uses POSIX error codes */
#define convert_sdio_error(r) (r)


CsrInt32
CsrSdioRead8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int err;
    err = sdioemb_read8(fdev, address, data);
    return convert_sdio_error(err);
} /* CsrSdioRead8() */

CsrInt32
CsrSdioWrite8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int err;
    err = sdioemb_write8(fdev, address, data);
    return convert_sdio_error(err);
} /* CsrSdioWrite8() */

CsrInt32
CsrSdioRead16(CsrSdioFunction *function, CsrUint32 address, CsrUint16 *data)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;
    uint8_t b0, b1;

    r = sdioemb_read8(fdev, address, &b0);
    if (r) {
        return convert_sdio_error(r);
    }

    r = sdioemb_read8(fdev, address+1, &b1);
    if (r) {
        return convert_sdio_error(r);
    }

    *data = ((uint16_t)b1 << 8) | b0;

    return 0;
} /* CsrSdioRead16() */

CsrInt32
CsrSdioWrite16(CsrSdioFunction *function, CsrUint32 address, CsrUint16 data)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;
    uint8_t b0, b1;

    /* r = sdioemb_write16(fdev, addr, data); */

    b1 = (data >> 8) & 0xFF;
    r = sdioemb_write8(fdev, address+1, b1);
    if (r) {
        return convert_sdio_error(r);
    }

    b0 = data & 0xFF;
    r = sdioemb_write8(fdev, address, b0);
    if (r) {
        return convert_sdio_error(r);
    }
    return 0;
} /* CsrSdioWrite16() */


CsrInt32
CsrSdioF0Read8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 *data)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int err;
    err = sdioemb_f0_read8(fdev, address, data);
    return convert_sdio_error(err);
} /* CsrSdioF0Read8() */


CsrInt32
CsrSdioF0Write8(CsrSdioFunction *function, CsrUint32 address, CsrUint8 data)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int err;
    err = sdioemb_f0_write8(fdev, address, data);
    return convert_sdio_error(err);
} /* CsrSdioF0Write8() */

CsrInt32
CsrSdioRead(CsrSdioFunction *function, CsrUint32 address, void *data, CsrUint32 length)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int err;
    err = sdioemb_read(fdev, address, data, length);
    return convert_sdio_error(err);
} /* CsrSdioRead() */

CsrInt32
CsrSdioWrite(CsrSdioFunction *function, CsrUint32 address, const void *data, CsrUint32 length)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int err;
    err = sdioemb_write(fdev, address, data, length);
    return convert_sdio_error(err);
} /* CsrSdioWrite() */


CsrInt32
CsrSdioBlockSizeSet(CsrSdioFunction *function, CsrUint16 blockSize)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r = 0;

    /* Module parameter overrides */
    if (sdio_block_size > -1) {
        blockSize = sdio_block_size;
    }

    unifi_trace(NULL, UDBG1, "Set SDIO function block size to %d\n",
                blockSize);

    r = sdioemb_set_block_size(fdev, blockSize);
    if (r) {
        unifi_error(NULL, "Error %d setting block size\n", r);
    }

    /* Determine the achieved block size to report to the core */
    function->blockSize = fdev->blocksize;

    return convert_sdio_error(r);
} /* CsrSdioBlockSizeSet() */


/*
 * ---------------------------------------------------------------------------
 *  csr_sdio_set_max_clock_speed
 *
 *      Set the maximum SDIO bus clock speed to use.
 *
 *  Arguments:
 *      sdio            SDIO context pointer
 *      max_khz         maximum clock speed in kHz
 *
 *  Returns:
 *      Set clock speed in kHz; or a UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
CsrInt32
csr_sdio_set_max_clock_speed(CsrSdioFunction *sdio, CsrUint32 max_khz)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)sdio->priv;

    if (!max_khz || max_khz > sdio_clock) {
        max_khz = sdio_clock;
    }
    unifi_trace(NULL, UDBG1, "Setting SDIO bus clock to %d kHz\n", max_khz);
    sdioemb_set_max_bus_freq(fdev, 1000 * max_khz);

    return 0;
} /* csr_sdio_set_max_clock_speed() */


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioInterruptEnable
 *  CsrSdioInterruptDisable
 *
 *      Enable or disable the SDIO interrupt.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 *  Returns:
 *      Zero on success or a UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CsrSdioInterruptEnable(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;

    r = sdioemb_interrupt_enable(fdev);
    if (r) {
        return convert_sdio_error(r);
    }

    return 0;
} /* CsrSdioInterruptEnable() */

CsrInt32
CsrSdioInterruptDisable(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;

    r = sdioemb_interrupt_disable(fdev);
    if (r) {
        return convert_sdio_error(r);
    }

    return 0;
} /* CsrSdioInterruptDisable() */


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioInterruptAcknowledge
 *
 *      Acknowledge an SDIO interrupt.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 *  Returns:
 *      Zero on success or a UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
void CsrSdioInterruptAcknowledge(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;

    sdioemb_interrupt_acknowledge(fdev);
} /* CsrSdioInterruptAcknowledge() */


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioFunctionEnable
 *
 *      Enable i/o on this function.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CsrSdioFunctionEnable(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;

    /* Enable UniFi function (the 802.11 part). */
    r = sdioemb_enable_function(fdev);
    if (r) {
        unifi_error(NULL, "Failed to enable SDIO function %d\n", fdev->function);
        return convert_sdio_error(r);
    }
    return 0;
} /* CsrSdioFunctionEnable() */


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioFunctionDisable
 *
 *      Disable i/o on this function.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CsrSdioFunctionDisable(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;

    /* Disable UniFi function (the 802.11 part). */
    r = sdioemb_disable_function(fdev);
    if (r) {
        unifi_error(NULL, "Failed to disable SDIO function %d\n", fdev->function);
        return convert_sdio_error(r);
    }
    return 0;
} /* CsrSdioFunctionDisable() */


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioFunctionActive
 *
 *      No-op as the bus goes to an active state at the start of every
 *      command.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 * ---------------------------------------------------------------------------
 */
void
CsrSdioFunctionActive(CsrSdioFunction *function)
{
} /* CsrSdioFunctionActive() */

/*
 * ---------------------------------------------------------------------------
 *  CsrSdioFunctionIdle
 *
 *      Set the function as idle.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 * ---------------------------------------------------------------------------
 */
void
CsrSdioFunctionIdle(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;

    sdioemb_idle_function(fdev);
} /* CsrSdioFunctionIdle() */


CsrInt32
CsrSdioPowerOn(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;

    if (disable_power_control != 1) {
        sdioemb_power_on(fdev);
    }

    return CSR_SDIO_RESULT_DEVICE_WAS_RESET;
} /* CsrSdioPowerOn() */

void
CsrSdioPowerOff(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    if (disable_power_control != 1) {
        sdioemb_power_off(fdev);
    }
} /* CsrSdioPowerOff() */


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioHardReset
 *
 *      Hard Resets UniFi is possible.
 * 
 *  Arguments:
 *      sdio            SDIO context pointer
 *
 * Returns:
 *      1       if the SDIO driver is not capable of doing a hard reset.
 *      0       if a hard reset was successfully performed.
 *      -CSR_EIO if an I/O error occured while re-initializing the card.
 *              This is a fatal, non-recoverable error.
 *      -CSR_ENODEV if the card is no longer present.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CsrSdioHardReset(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;
    int r;

    /* Hard reset can be disabled by a module parameter */
    r = 1;
    if (disable_hw_reset != 1) {
        r = sdioemb_hard_reset(fdev); /* may return 1 if can't reset */
        if (r < 0) {
            return convert_sdio_error(r);   /* fatal error */
        }
    }

    /* Set the SDIO bus width after a hard reset */
    if (buswidth == 1) {
        unifi_info(NULL, "Setting SDIO bus width to 1\n");
        sdioemb_set_bus_width(fdev, buswidth);
    } else if (buswidth == 4) {
        unifi_info(NULL, "Setting SDIO bus width to 4\n");
        sdioemb_set_bus_width(fdev, buswidth);
    }

    return convert_sdio_error(r);

} /* CsrSdioHardReset() */


int csr_sdio_linux_remove_irq(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;

    return sdioemb_interrupt_disable(fdev);
}

int csr_sdio_linux_install_irq(CsrSdioFunction *function)
{
    struct sdioemb_dev *fdev = (struct sdioemb_dev *)function->priv;

    return sdioemb_interrupt_enable(fdev);
}


/*
 * ---------------------------------------------------------------------------
 *  uf_glue_sdio_int_handler
 *      Card interrupt callback.
 *
 * Arguments:
 *      fdev            SDIO context pointer
 *
 * Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
uf_glue_sdio_int_handler(struct sdioemb_dev *fdev)
{
    CsrSdioFunction *sdio_ctx = fdev->drv_data;
    CsrSdioDsrCallback func_dsr_callback;

    /* If the function driver has registered a handler, call it */
    if (sdio_func_drv && sdio_func_drv->interrupt) {
        /* The function driver may return a DSR. */
        func_dsr_callback = sdio_func_drv->interrupt(sdio_ctx);
        /* If it did return a DSR handle, call it */
        if (func_dsr_callback) {
            func_dsr_callback(sdio_ctx, 0);
        }
    }
}


/*
 * ---------------------------------------------------------------------------
 *  uf_glue_sdio_probe
 *
 *      Card insert callback.
 *
 * Arguments:
 *      fdev            SDIO context pointer
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
static int
uf_glue_sdio_probe(struct sdioemb_dev *fdev)
{
    int r;
    CsrSdioFunction *sdio_ctx;

    unifi_info(NULL, "UniFi card inserted\n");

    /* Allocate context and private in one lump */
    sdio_ctx = (CsrSdioFunction *)kmalloc(sizeof(CsrSdioFunction),
                                          GFP_KERNEL);
    if (sdio_ctx == NULL) {
        return -ENOMEM;
    }


    sdio_ctx->sdioId.manfId = fdev->vendor_id;
    sdio_ctx->sdioId.cardId = fdev->device_id;
    sdio_ctx->sdioId.sdioFunction = fdev->function;
    sdio_ctx->sdioId.sdioInterface = 0;
    sdio_ctx->blockSize = fdev->blocksize;
    sdio_ctx->priv = (void *)fdev;
    sdio_ctx->features = 0;

    /* Module parameter enables byte mode */
    if (sdio_byte_mode) {
        sdio_ctx->features |= CSR_SDIO_FEATURE_BYTE_MODE;
    }

    /* Set up pointer to func_priv in middle of lump */
    fdev->drv_data = sdio_ctx;

    /* Always override default SDIO bus clock */
    unifi_trace(NULL, UDBG1, "Setting SDIO bus clock to %d kHz\n", sdio_clock);
    sdioemb_set_max_bus_freq(fdev, 1000 * sdio_clock);

    /* Call the main UniFi driver inserted handler */
    r = -EINVAL;
    if (sdio_func_drv && sdio_func_drv->inserted) {
        uf_add_os_device(fdev->slot_id, fdev->os_device);
        r = sdio_func_drv->inserted(sdio_ctx);
    }

    return r;
} /* uf_glue_sdio_probe() */


/*
 * ---------------------------------------------------------------------------
 *  uf_sdio_remove
 *
 *      Card removal callback.
 *
 * Arguments:
 *      fdev            SDIO device
 *
 * Returns:
 *      UniFi driver error code.
 * ---------------------------------------------------------------------------
 */
static void
uf_sdio_remove(struct sdioemb_dev *fdev)
{
    CsrSdioFunction *sdio_ctx = fdev->drv_data;

    unifi_info(NULL, "UniFi card removed\n");

    /* Clean up the SDIO function driver */
    if (sdio_func_drv && sdio_func_drv->removed) {
        sdio_func_drv->removed(sdio_ctx);
    }

    kfree(sdio_ctx);

} /* uf_sdio_remove */


/*
 * ---------------------------------------------------------------------------
 *  uf_glue_sdio_suspend
 *
 *      System suspend callback.
 *
 * Arguments:
 *      fdev            SDIO device
 *
 * Returns:
 *
 * ---------------------------------------------------------------------------
 */
static void
uf_glue_sdio_suspend(struct sdioemb_dev *fdev)
{
    CsrSdioFunction *sdio_ctx = fdev->drv_data;

    unifi_trace(NULL, UDBG3, "Suspending...\n");

    /* Pass event to UniFi Driver. */
    if (sdio_func_drv && sdio_func_drv->suspend) {
        sdio_func_drv->suspend(sdio_ctx);
    }

} /* uf_glue_sdio_suspend() */


/*
 * ---------------------------------------------------------------------------
 *  uf_glue_sdio_resume
 *
 *      System resume callback.
 *
 * Arguments:
 *      fdev            SDIO device
 *
 * Returns:
 * 
 * ---------------------------------------------------------------------------
 */
static void
uf_glue_sdio_resume(struct sdioemb_dev *fdev)
{
    CsrSdioFunction *sdio_ctx = fdev->drv_data;

    unifi_trace(NULL, UDBG3, "Resuming...\n");

    /* Pass event to UniFi Driver. */
    if (sdio_func_drv && sdio_func_drv->resume) {
        sdio_func_drv->resume(sdio_ctx);
    }
} /* uf_glue_sdio_resume() */




static struct sdioemb_func_driver unifi_sdioemb = {
    .name = "unifi",
    .id_table = NULL,           /* Filled in when main driver registers */

    .probe  = uf_glue_sdio_probe,
    .remove = uf_sdio_remove,
    .card_int_handler = uf_glue_sdio_int_handler,
    .suspend  = uf_glue_sdio_suspend,
    .resume = uf_glue_sdio_resume,
};


/*
 * ---------------------------------------------------------------------------
 *  CsrSdioFunctionDriverRegister
 *  CsrSdioFunctionDriverUnregister
 *
 *      These functions are called from the main module load and unload
 *      functions. They perform the appropriate operations for the
 *      SDIOemb driver.
 * 
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CsrSdioFunctionDriverRegister(CsrSdioFunctionDriver *sdio_drv)
{
    int r;
    int i;

    printk("Unifi: Using CSR embedded SDIO driver\n");

    if (sdio_func_drv) {
        unifi_error(NULL, "sdio_emb: UniFi driver already registered\n");
        return -CSR_EINVAL;
    }

    /* Build ID table to pass to sdioemb */
    unifi_sdioemb.id_table = CsrPmalloc(sizeof(struct sdioemb_id_table) * (sdio_drv->idsCount + 1));
    if (unifi_sdioemb.id_table == NULL) {
        unifi_error(NULL, "sdio_emb: Failed to allocate memory for ID table (%d IDs)\n", sdio_drv->idsCount);
        return -CSR_ENOMEM;
    }
    for (i = 0; i < sdio_drv->idsCount; i++) {
        unifi_sdioemb.id_table[i].vendor_id = sdio_drv->ids[i].manfId;
        unifi_sdioemb.id_table[i].device_id = sdio_drv->ids[i].cardId;
        unifi_sdioemb.id_table[i].function  = sdio_drv->ids[i].sdioFunction;
        unifi_sdioemb.id_table[i].interface = sdio_drv->ids[i].sdioInterface;
    }
    unifi_sdioemb.id_table[i].vendor_id = 0;
    unifi_sdioemb.id_table[i].device_id = 0;
    unifi_sdioemb.id_table[i].function  = 0;
    unifi_sdioemb.id_table[i].interface = 0;

    /* Save the registered driver description */
    sdio_func_drv = sdio_drv;

    /* Register ourself with sdioemb */
    r = sdioemb_driver_register(&unifi_sdioemb);
    if (r) {
        unifi_error(NULL, "Failed to register UniFi SDIO driver: %d\n", r);
        return convert_sdio_error(r);
    }

    return 0;
} /* CsrSdioFunctionDriverRegister() */


void
CsrSdioFunctionDriverUnregister(CsrSdioFunctionDriver *sdio_drv)
{
    sdioemb_driver_unregister(&unifi_sdioemb);

    sdio_func_drv = NULL;

    CsrPfree(unifi_sdioemb.id_table);
    unifi_sdioemb.id_table = NULL;
} /* CsrSdioFunctionDriverUnregister() */

