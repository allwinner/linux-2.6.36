/*
 * ---------------------------------------------------------------------------
 * FILE: download.c
 *
 * PURPOSE:
 *      Routines for downloading firmware to UniFi.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "driver/unifiversion.h"
#include "card.h"
#include "xbv.h"

#undef CSR_WIFI_VERIFY_DOWNLOAD
#undef CSR_WIFI_IGNORE_PATCH_VERSION_MISMATCH

static CsrInt32 check_firmware_version(card_t *card, const xbv1_t *fwinfo);
static CsrInt32 set_entry_points(card_t *card, const symbol_t *slut_ptr,
                                 CsrUint32 slut_len);
static CsrInt32 set_an_entry_point(card_t *card, CsrUint32 gp, CsrInt16 which);
static CsrUint32 find_slut_entry(CsrUint16 id, const symbol_t *slut_ptr,
                                 CsrUint32 slut_len);
static CsrInt32 do_primary_download(card_t *card, void *dlpriv,
                                    xbv1_t *pfwinfo);
static CsrInt32 do_secondary_download(card_t *card, void *dlpriv,
                                      xbv1_t *pfwinfo);
static CsrInt32 do_patch_download(card_t *card, void *dlpriv,
                                  xbv1_t *pfwinfo, CsrUint32 boot_ctrl_addr);

#ifdef CSR_WIFI_VERIFY_DOWNLOAD
static CsrInt32 verify_download(card_t *card, CsrUint32 addr,
                                CsrUint8 *buf, CsrUint16 len);
#endif /* CSR_WIFI_VERIFY_DOWNLOAD */

static CsrInt32 do_patch_convert_download(card_t *card,
                                          void *dlpriv, xbv1_t *pfwinfo);

/*
 * ---------------------------------------------------------------------------
 *  _find_in_slut
 *
 *      Find the offset of the appropriate object in the SLUT of a card
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      psym            Pointer to symbol object.
 *                         id set up by caller
 *                         obj will be set up by this function
 *      pslut           Pointer to SLUT address, if 0xffffffff then it must be
 *                         read from the chip.
 *  Returns:
 *      0 on success
 *      Non-zero on error,
 *      -CSR_EEXIST if not found
 * ---------------------------------------------------------------------------
 */
static CsrInt32
_find_in_slut(card_t *card, symbol_t *psym, CsrUint32 *pslut)
{
    CsrUint32 slut_address;
    CsrUint16 finger_print;
    CsrInt32 r;

    /* Get SLUT address */
    if (*pslut == 0xffffffff) {
        r = card_wait_for_firmware_to_start(card, &slut_address);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Firmware hasn't started\n");
            func_exit_r(r);
            return r;
        }
        *pslut = slut_address;

        /*
         * Firmware has started so set the SDIO bus clock to the initial speed,
         * faster than UNIFI_SDIO_CLOCK_SAFE, to speed up the f/w download.
         */
        r = csr_sdio_set_max_clock_speed(card->sdio_if, UNIFI_SDIO_CLOCK_INIT);
        if (r < 0) {
            func_exit_r(r);
            return r;
        }
        card->sdio_clock_speed = UNIFI_SDIO_CLOCK_INIT;
    } else {
        slut_address = *pslut;  /* Use previously discovered address */
    }
    unifi_trace(card->ospriv, UDBG4, "SLUT addr: 0x%lX\n", slut_address);

    /*
     * Check the SLUT fingerprint.
     * The slut_address is a generic pointer so we must use unifi_read16().
     */
    unifi_trace(card->ospriv, UDBG4, "Looking for SLUT finger print\n");
    finger_print = 0;
    r = unifi_read16(card, slut_address, &finger_print);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to read SLUT finger print\n");
        func_exit_r(r);
        return r;
    }

    if (finger_print != SLUT_FINGERPRINT) {
        unifi_error(card->ospriv, "Failed to find SLUT fingerprint\n");
        func_exit_r(-CSR_EIO);
        return -CSR_EIO;
    }

    /* Symbol table starts imedately after the fingerprint */
    slut_address += 2;

    while (1) {
        CsrUint16 id;
        CsrUint32 obj;

        r = unifi_read16(card, slut_address, &id);
        if (r < 0) {
            func_exit_r(r);
            return r;
        }
        slut_address += 2;

        if (id == CSR_SLT_END) {
            /* End of table reached: not found */
            r = -CSR_ERANGE;
            break;
        }

        r = unifi_read32(card, slut_address, &obj);
        if (r < 0) {
            func_exit_r(r);
            return r;
        }
        slut_address += 4;

        unifi_trace(card->ospriv, UDBG3, "  found SLUT id %02d.%08lx\n", id, obj);

        r = -CSR_EEXIST;
        /* Found search term? */
        if (id == psym->id) {
            unifi_trace(card->ospriv, UDBG1, " matched SLUT id %02d.%08lx\n", id, obj);
            psym->obj = obj;
            r = 0;
            break;
        }
    }

    func_exit_r(r);
    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  do_patch_convert_download
 *
 *      Download the given firmware image to the UniFi, converting from FWDL
 *      to PTDL XBV format.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      dlpriv          Pointer to source firmware image
 *      fwinfo          Pointer to source firmware info struct
 *
 *  Returns:
 *      0 on success, CSR error code on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static CsrInt32
do_patch_convert_download(card_t *card, void *dlpriv, xbv1_t *pfwinfo)
{
    CsrInt32 r;
    CsrInt32 slut_base = 0xffffffff;
    void *pfw;
    CsrUint32 psize;
    symbol_t sym;

    /* Reset the chip to guarantee that the ROM loader is running */
    r = unifi_init(card);
    if (r && (r != 1)) {
        unifi_error(card->ospriv,
                    "do_patch_convert_download: failed to re-init UniFi\n");
        return r;
    }

    /* If no unifi_helper is running, the firmware version must be read */
    if (card->build_id == 0) {
        CsrUint32 ver = 0;
        sym.id = CSR_SLT_BUILD_ID_NUMBER;
        sym.obj = 0; /* To be updated by _find_in_slut() */

        unifi_trace(card->ospriv, UDBG1, "Need f/w version\n");

        /* Find chip build id entry in SLUT */
        r = _find_in_slut(card, &sym, &slut_base);
        if (r) {
            unifi_error(card->ospriv, "Failed to find CSR_SLT_BUILD_ID_NUMBER\n");
            return -CSR_EIO;
        }

        /* Read running f/w version */
        r = unifi_read32(card, sym.obj, &ver);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to read f/w id\n");
            return -CSR_EIO;
        }
        card->build_id = ver;
    }

    /* Convert the ptest firmware to a patch against the running firmware */
    pfw = xbv_to_patch(card, unifi_fw_read, dlpriv, pfwinfo, &psize);
    if (!pfw) {
        unifi_error(card->ospriv, "Failed to convert f/w to patch");
        return -CSR_ENOMEM;

    } else {

        void *desc;
        sym.id = CSR_SLT_BOOT_LOADER_CONTROL;
        sym.obj = 0; /* To be updated by _find_in_slut() */

        /* Find boot loader control entry in SLUT */
        r = _find_in_slut(card, &sym, &slut_base);
        if (r) {
            unifi_error(card->ospriv, "Failed to find BOOT_LOADER_CONTROL\n");
            return -CSR_EIO;
        }

        r = unifi_set_host_state(card, UNIFI_HOST_STATE_AWAKE);
        if (r) {
            unifi_error(card->ospriv, "Failed to wake UniFi\n");
        }

        /* Get a dlpriv for the patch buffer so that unifi_fw_read() can
         * access it.
         */
        desc = unifi_fw_open_buffer(card->ospriv, pfw, psize);
        if (!desc) {
            return -CSR_ENOMEM;
        }

        /* Download the patch */
        unifi_info(card->ospriv, "Downloading converted f/w as patch\n");
        r = unifi_dl_patch(card, desc, sym.obj);
        CsrMemFree(pfw);
        unifi_fw_close_buffer(card->ospriv, desc);

        if (r) {
            unifi_error(card->ospriv, "Converted patch download failed\n");
            func_exit_r(r);
            return r;
        } else {
            unifi_trace(card->ospriv, UDBG1, "Converted patch downloaded\n");
        }

        /* This command starts the firmware */
        r = unifi_do_loader_op(card, sym.obj+6, UNIFI_BOOT_LOADER_RESTART);
        if (r) {
            unifi_error(card->ospriv, "Failed to write loader restart cmd\n");
        }

        func_exit_r(r);
        return r;
    }
}


/*
 * ---------------------------------------------------------------------------
 *  unifi_dl_firmware
 *
 *      Download the given firmware image to the UniFi.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      iread           Pointer to a function to read parts of the firmware
 *                      XBV file.
 *      dlpriv          A context pointer from the calling function to be
 *                      passed when calling iread().
 *      secondary       If zero, indicates that the this is a primary
 *                      download to bare metal, i.e. CMD53 may not be used.
 *                      If non-zero, this indicates that a primary loader has
 *                      already been loaded and we can use the faster CMD53
 *                      to transfer download data.
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENOMEM         memory allocation failed
 *      -CSR_EINVAL         error in XBV file
 *      -CSR_EIO            SDIO error
 *
 *  Notes:
 *      Stops and resets the chip, does the download and runs the new
 *      firmware.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_dl_firmware(card_t *card, void *dlpriv, CsrInt8 secondary)
{
    xbv1_t *fwinfo;
    symbol_t slut[32];
    CsrInt32 num_slut_entries;
    CsrInt32 r;

    func_enter();

    fwinfo = CsrMemAlloc(sizeof(xbv1_t));
    if (fwinfo == NULL) {
        unifi_error(card->ospriv, "Failed to allocate memory for firmware\n");
        return -CSR_ENOMEM;
    }

    /*
     * Scan the firmware file to find the TLVs we are interested in.
     * These are:
     *   - check we support the file format version in VERF
     *   - SLTP Symbol Lookup Table Pointer
     *   - FWDL firmware download segments
     *   - FWOV firmware overlay segment
     *   - VMEQ Register probe tests to verify matching h/w
     */
    r = xbv1_parse(card, unifi_fw_read, dlpriv, fwinfo);
    if (r || fwinfo->mode != xbv_firmware) {
        unifi_error(card->ospriv, "File type is %s, expected firmware.\n",
                    fwinfo->mode == xbv_patch ? "patch" : "unknown");
        CsrMemFree(fwinfo);
        return -CSR_EINVAL;
    }

    /* UF6xxx doesn't accept firmware, only patches. Therefore we convert
     * the file to patch format with version numbers matching the current
     * running firmware, and then download via the patch mechanism.
     * The sole purpose of this is to support production test firmware across
     * different ROM releases, the test firmware being provided in non-patch
     * format.
     */
    if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {
        unifi_info(card->ospriv, "Must convert f/w to patch format\n");
        r = do_patch_convert_download(card, dlpriv, fwinfo);
        CsrMemFree(fwinfo);

        func_exit_r(r);
        return r;
    }

    /*
     * Firmware image download
     */

    /* Save the info on the overlay for later. */
    CsrMemCpy(&card->fwov, &fwinfo->fwov, sizeof(struct FWOV));

    /*
     * Read the SLUT from the f/w image.
     * This can only be done once the f/w image has been parsed so we
     * can find the SLUT address in the download data.
     */
    num_slut_entries = xbv1_read_slut(card, unifi_fw_read, dlpriv, fwinfo,
                                      slut, sizeof(slut)/sizeof(slut[0]));
    if (num_slut_entries <= 0) {
        CsrMemFree(fwinfo);
        return -CSR_EINVAL;
    }

    /*
     * A secondary download is done before stopping the XAPs
     */
    if (secondary) {
        r = check_firmware_version(card, fwinfo);
        if (r) {
            if (r > 0) {
                unifi_error(card->ospriv, "Wrong FW version for HW (HW=%04x)\n",
                            card->chip_version);
            } else {
                unifi_error(card->ospriv, "Error checking FW / HW version (HW=%04x)\n",
                            card->chip_version);
            }
            CsrMemFree(fwinfo);
            return r;
        }
        r = do_secondary_download(card, dlpriv, fwinfo);
        if (r) {
            unifi_error(card->ospriv, "Failed to download image using secondary loader\n");
            CsrMemFree(fwinfo);
            return r;
        }
    }


    /* Stop the UniFis on-board processors */
    r = card_stop_processor(card, UNIFI_PROC_BOTH);
    if (r) {
        unifi_error(card->ospriv, "Failed to stop UniFi processors\n");
        CsrMemFree(fwinfo);
        return r;
    }

    /*
     * The XAPs must be stopped before doing a primary download.
     */
    if (!secondary) {
        r = check_firmware_version(card, fwinfo);
        if (r) {
            if (r > 0) {
                unifi_error(card->ospriv, "Wrong FW version for HW (HW=%04x)\n",
                            card->chip_version);
            } else {
                unifi_error(card->ospriv, "Error checking FW / HW version (HW=%04x)\n",
                            card->chip_version);
            }
            CsrMemFree(fwinfo);
            return r;
        }
        r = do_primary_download(card, dlpriv, fwinfo);
        if (r) {
            unifi_error(card->ospriv, "Failed to copy image to UniFi memory\n");
            CsrMemFree(fwinfo);
            return r;
        }
    }


#if 0
    /* read */
    {
        CsrUint16 u;

        r = unifi_read16(card, fwinfo.slut_addr, &u);
        if (r) {
            unifi_error(card->ospriv, "Failed read test loc\n");
            return r;
        }
        unifi_trace(card->ospriv, UDBG2, "slut_addr 0x%lX = 0x%X\n", fwinfo.slut_addr, u);
    }
#endif

    /* We don't need the fwinfo any more. */
    CsrMemFree(fwinfo);

    /* Set the ResetVector registers to the entry point address */
    r = set_entry_points(card, slut, num_slut_entries);
    if (r) {
        unifi_error(card->ospriv, "Failed to set UniFi reset vectors\n");
        return r;
    }


    /*
     * Clear the SHARED_MAILBOX1 register, so the init code can see when
     * the firmware sets it to 1.
     */
    r = unifi_write_direct16(card, ChipHelper_MAILBOX1(card->helper) * 2, 0);
    if (r) {
        unifi_error(card->ospriv, "Failed to zero SHARED_MAILBOX1.\n");
        return r;
    }

    /* Start both XAPs */
    r = card_start_processor(card, UNIFI_PROC_BOTH);
    if (r) {
        unifi_error(card->ospriv, "Failed to start UniFi processors\n");
        return r;
    }

    func_exit();
    return r;

} /* unifi_dl_firmware() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_dl_patch
 *
 *      Load the given patch set into UniFi.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      dlpriv          The os specific handle to the firmware file.
 *      boot_ctrl       The address of the boot loader control structure.
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENOMEM         memory allocation failed
 *      -CSR_EINVAL         error in XBV file
 *      -CSR_EIO            SDIO error
 *
 *  Notes:
 *      This ends up telling UniFi to restart.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_dl_patch(card_t *card, void *dlpriv, CsrUint32 boot_ctrl)
{
    xbv1_t *fwinfo;
    CsrInt32 r;

    func_enter();

    unifi_info(card->ospriv, "unifi_dl_patch %p %08x\n", dlpriv, boot_ctrl);

    fwinfo = CsrMemAlloc(sizeof(xbv1_t));
    if (fwinfo == NULL) {
        unifi_error(card->ospriv, "Failed to allocate memory for patches\n");
        func_exit();
        return -CSR_ENOMEM;
    }

    /*
     * Scan the firmware file to find the TLVs we are interested in.
     * These are:
     *   - check we support the file format version in VERF
     *   - FWID The build ID of the ROM that we can patch
     *   - PTDL patch download segments
     */
    r = xbv1_parse(card, unifi_fw_read, dlpriv, fwinfo);
    if (r || fwinfo->mode != xbv_patch) {
        CsrMemFree(fwinfo);
        unifi_error(card->ospriv, "Failed to read in patch file\n");
        func_exit();
        return -CSR_EINVAL;
    }

    /*
     * We have to check the build id read from the SLUT against that
     * for the patch file.  They have to match exactly.
     *    "card->build_id" == XBV1.PTCH.FWID
     */
    if (card->build_id != fwinfo->build_id) {
        unifi_error(card->ospriv, "Wrong patch file for chip (chip = %lu, file = %lu)\n",
                    card->build_id, fwinfo->build_id);
        CsrMemFree(fwinfo);
#ifndef CSR_WIFI_IGNORE_PATCH_VERSION_MISMATCH
        func_exit();
        return -CSR_EINVAL;
#else
        fwinfo = NULL;
        dlpriv = NULL;
        return 0;
#endif
    }

    r = do_patch_download(card, dlpriv, fwinfo, boot_ctrl);
    if (r) {
        unifi_error(card->ospriv, "Failed to patch image\n");
    }

    CsrMemFree(fwinfo);

    func_exit();
    return r;
} /* unifi_dl_patch() */


void*
unifi_dl_fw_read_start(card_t *card, CsrInt8 is_fw) {
    card_info_t card_info;

    unifi_card_info(card, &card_info);
    unifi_trace(card->ospriv, UDBG5,
                "id=%d, ver=0x%x, fw_build=%d, fw_hip=0x%x, block_size=%d\n",
                card_info.chip_id, card_info.chip_version,
                card_info.fw_build, card_info.fw_hip_version,
                card_info.sdio_block_size);

    return unifi_fw_read_start(card->ospriv, is_fw, &card_info);
}


/*
 * ---------------------------------------------------------------------------
 *  check_firmware_version
 *
 *      Process the entries in the XBV file that verify compatibility with
 *      the chip by checking various registers.
 *      The VMEQ and VAND sections of the XBV file allow arbitrary chip
 *      registers to be checked.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      fwinfo          Pointer to a fwinfo struct describing the f/w
 *                      XBV file.
 *
 *  Returns:
 *      0 if the verify opertaion succeeded AND the chip is compatible.
 *      1 if the verify opertaion succeeded AND the chip is not compatible.
 *      CSR error code if a register read failed
 * ---------------------------------------------------------------------------
 */
static CsrInt32
check_firmware_version(card_t *card, const xbv1_t *fwinfo)
{
    CsrUint16 i, j;
    CsrInt32 r;
    CsrUint16 v;
    const struct VAND *vand;
    const struct VMEQ *vmeq;

    for (j = 0; j < fwinfo->vers.num_vand; j++)
    {
        vand = fwinfo->vand + j;

        for (i = 0, vmeq = fwinfo->vmeq + vand->first;
             i < vand->count;
             i++, vmeq++)
        {
            r = unifi_read16(card, vmeq->addr, &v);
            if (r) {
                unifi_error(card->ospriv, "Failed to read reg %05lx\n", vmeq->addr);
                return r;
            }
            if ((v & vmeq->mask) != vmeq->value)
            {
                unifi_trace(card->ospriv, UDBG1, "Version reg mismatch addr:%05lx val:%04x\n",
                      vmeq->addr, v);
                break;
            }
        }
        /* If all tests passed, we match the condition */
        if (i == vand->count) {
            return 0;
        }
    }

    /* Not compatible */
    return 1;
} /* check_firmware_version() */



/*
 * ---------------------------------------------------------------------------
 *  set_entry_points
 *
 *      Retrieve the entry point address for each XAP from the SLUT and
 *      use it to set the initial value of Program Counter.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      slut_ptr        Pointer to the table in memory.
 *      slut_len        Maximum length to search.
 *
 *  Returns:
 *      0 on success, CSR error code on error
 * ---------------------------------------------------------------------------
 */
static CsrInt32
set_an_entry_point(card_t *card, CsrUint32 gp, CsrInt16 which)
{
    CsrInt32 r;
    CsrUint32 ep;

    /*
     * Take the offset part of the GenericPointer amd convert to a XAP
     * 16-bit offset.
     */
    ep = UNIFI_GP_OFFSET(gp) / 2;
    switch (UNIFI_GP_SPACE(gp))
    {
    case UNIFI_PHY_PMEM:
    case UNIFI_MAC_PMEM:
        ep += ChipHelper_PROGRAM_MEMORY_RAM_OFFSET(card->helper);
        break;

    case UNIFI_EXT_FLASH:
        if (!ChipHelper_HasFlash(card->helper)) {
            return -CSR_EINVAL;
        }
        ep += ChipHelper_PROGRAM_MEMORY_FLASH_OFFSET(card->helper);
        break;

    case UNIFI_EXT_SRAM:
        if (!ChipHelper_HasExtSram(card->helper)) {
            return -CSR_EINVAL;
        }
        ep += ChipHelper_PROGRAM_MEMORY_EXT_SRAM_OFFSET(card->helper);
        break;

    default:
        return -CSR_EINVAL;
    }

    /* Select the XAP */
    r = unifi_set_proc_select(card, which);
    if (r) {
        return r;
    }

    /* Write PCL and PCH */
    r = unifi_write_direct16(card, ChipHelper_XAP_PCL(card->helper) * 2,
                             (CsrUint16)(ep & 0xFFFF));
    if (r) {
        return r;
    }
    r = unifi_write_direct16(card, ChipHelper_XAP_PCH(card->helper) * 2,
                             (CsrUint16)((ep >> 16) & 0xFF));
    if (r) {
        return r;
    }

    return 0;
}

static CsrInt32
set_entry_points(card_t *card, const symbol_t *slut_ptr, CsrUint32 slut_len)
{
    CsrUint32 ep;
    CsrInt32 r;

    /* Find SLT_Reset_Vector_PHY in SLUT */
    ep = find_slut_entry(CSR_SLT_RESET_VECTOR_PHY, slut_ptr, slut_len);
    if (ep == 0xFFFFFFFF) {
        return -CSR_EINVAL;
    }
    r = set_an_entry_point(card, ep, UNIFI_PROC_PHY);
    if (r) {
        return r;
    }

    /* Find SLT_Reset_Vector_MAC in SLUT */
    ep = find_slut_entry(CSR_SLT_RESET_VECTOR_MAC, slut_ptr, slut_len);
    if (ep == 0xFFFFFFFF) {
        return -CSR_EINVAL;
    }
    r = set_an_entry_point(card, ep, UNIFI_PROC_MAC);
    if (r) {
        return r;
    }

    return 0;
} /* set_entry_points() */



/*
 * ---------------------------------------------------------------------------
 *  find_slut_entry
 *
 *      Search the Symbol Look Up Table for the given id,
 *      return the 32-bit symbol value if found.
 *
 *  Arguments:
 *      id              The SLUT id to search for.
 *      slut_ptr        Pointer to the table in memory.
 *      slut_len        Maximum length to search.
 *                      This is a defensive measure to stop us walking off
 *                      the end of valid memory if the table is junk.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static CsrUint32
find_slut_entry(CsrUint16 id, const symbol_t *slut_ptr, CsrUint32 slut_len)
{
    CsrUint16 i;

    for (i = 0; i < slut_len; i++) {
        if (slut_ptr[i].id == id) {
            return slut_ptr[i].obj;
        }
    }

    return 0xFFFFFFFF;
} /* find_slut_entry() */



/*
 * ---------------------------------------------------------------------------
 *  do_primary_download
 *
 *      This function downloads a firmware image to the UniFi by
 *      writing directly to memory, one byte at a time over SDIO.
 *      It is assumed that the XAP processors have been stopped.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      iread           Pointer to a function to read parts of the firmware
 *                      XBV file.
 *      dlpriv          A context pointer from the calling function to be
 *                      passed when calling iread().
 *      pfwinfo         Pointer to a fwinfo struct describing the f/w
 *                      XBV file.
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENOMEM         memory allocation failed
 *      -CSR_EINVAL         error in XBV file
 *      -CSR_EIO            SDIO error
 * ---------------------------------------------------------------------------
 */
static CsrInt32
do_primary_download(card_t *card,
                    void *dlpriv,
                    xbv1_t *pfwinfo)
{
    CsrInt16 i;
    CsrUint8 *buf;
    /* Work in chunks of up to 2K in size */
    const CsrInt16 buf_size = 2*1024;

    buf = CsrMemAlloc(buf_size);
    if (buf == NULL) {
        unifi_error(card->ospriv, "Failed to allocate transfer buffer for firmware download\n");
        return -CSR_ENOMEM;
    }

    /* Copy download data to UniFi memory */
    for (i = 0; i < pfwinfo->num_fwdl; i++) {
        CsrUint32 addr;
        CsrUint32 offset;
        CsrInt32 remaining;

        unifi_trace(card->ospriv, UDBG3, "Downloading to 0x%lX for %d from offset %d\n",
              pfwinfo->fwdl[i].dl_addr,
              pfwinfo->fwdl[i].dl_size,
              pfwinfo->fwdl[i].dl_offset);

        addr      = pfwinfo->fwdl[i].dl_addr;
        offset    = pfwinfo->fwdl[i].dl_offset;
        remaining = pfwinfo->fwdl[i].dl_size;

        while (remaining > 0) {
            CsrInt32 r;
            CsrUint16 len;
            len = (CsrUint16)remaining;
            if (len > buf_size) {
                len = buf_size;
            }
            if (unifi_fw_read(card->ospriv, dlpriv, offset, buf, len) != len) {
                CsrMemFree(buf);
                return -CSR_EINVAL;
            }
            r = unifi_writen(card, addr, buf, len);
            if (r) {
            CsrMemFree(buf);
            return r;
        }

            addr += len;
            offset += len;
            remaining -= len;
        }
    }

    CsrMemFree(buf);

    return 0;
} /* do_primary_download() */




/*
 * ---------------------------------------------------------------------------
 *  safe_read_shared_location
 *
 *      Read a shared memory location repeatedly until we get two readings
 *      the same.
 *
 *  Arguments:
 *      card            Pointer to card context struct.
 *      unifi_addr      UniFi shared-data-memory address to access.
 *      pdata           Pointer to a byte variable for the value read.
 *
 *
 *  Returns:
 *      0 on success, CSR error code on failure
 * ---------------------------------------------------------------------------
 */
static CsrInt32
safe_read_shared_location(card_t *card, CsrUint32 address, CsrUint8 *pdata)
{
    CsrInt32 r;
    CsrUint16 limit = 1000;
    CsrUint8 b, b2;

    r = unifi_read_8_or_16(card, address, &b);
    if (r) {
        return r;
    }

    while (limit--) {
        r = unifi_read_8_or_16(card, address, &b2);
        if (r) {
            return r;
        }

        /* When we have a stable value, return it */
        if (b == b2) {
            *pdata = b;
            return 0;
        }

        b = b2;
    }

    return -CSR_EIO;
} /* safe_read_shared_location() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_do_loader_op
 *
 *      Send a loader / boot_loader command to the UniFi and wait for
 *      it to complete.
 *
 *  Arguments:
 *      card            Pointer to card context struct.
 *      op_addr         The address of the loader operation control word.
 *      opcode          The operation to perform.
 *
 *  Returns:
 *      Negative value indicating error code:
 *      -CSR_EIO            SDIO error
 *      -CSR_ETIMEDOUT      SDIO/XAP timeout
 * ---------------------------------------------------------------------------
 */

/*
 * Ideally instead of sleeping, we want to busy wait.
 * Currently there is no framework API to do this. When it becomes available,
 * we can use it to busy wait using usecs
 */
#define OPERATION_TIMEOUT_LOOPS (100)  /* when OPERATION_TIMEOUT_DELAY==1, (500) otherwise */
#define OPERATION_TIMEOUT_DELAY 1      /* msec, or 200usecs */

CsrInt32
unifi_do_loader_op(card_t *card, CsrUint32 op_addr, CsrUint8 opcode)
{
    CsrInt32 r;
    CsrInt16 op_retries;

    unifi_trace(card->ospriv, UDBG4, "Loader cmd 0x%0x -> 0x%08x\n", opcode, op_addr);

    /* Set the Operation command byte to the opcode */
    r = unifi_write_8_or_16(card, op_addr, opcode);
    if (r) {
        unifi_error(card->ospriv, "Failed to write loader copy command\n");
        return r;
    }

    /* Wait for Operation command byte to be Idle */
    /* Typically takes ~100us */
    op_retries = 0;
    r = 0;
    while (1) {
        CsrUint8 op;

        /*
         * Read the memory location until two successive reads give
         * the same value.
         * Then handle it.
         */
        r = safe_read_shared_location(card, op_addr, &op);
        if (r) {
            unifi_error(card->ospriv, "Failed to read loader status\n");
            break;
        }

        if (op == UNIFI_LOADER_IDLE) {
            /* Success */
            break;
        }

        if (op != opcode) {
            unifi_error(card->ospriv, "Error reported by loader: 0x%X\n", op);
            r = -CSR_EIO;
            break;
        }

        /* Allow 500us timeout */
        if (++op_retries >= OPERATION_TIMEOUT_LOOPS) {
            unifi_error(card->ospriv, "Timeout waiting for loader to ack transfer\n");
            /* Stop XAPs to aid post-mortem */
            r = card_stop_processor(card, UNIFI_PROC_BOTH);
            if (r) {
                unifi_error(card->ospriv, "Failed to stop UniFi processors\n");
            } else {
                r = -CSR_ETIMEDOUT;
            }
            break;
        }
        CsrThreadSleep(OPERATION_TIMEOUT_DELAY);

    } /* Loop exits with r != 0 on error */

    return r;
} /* unifi_do_loader_op() */

/*
 * ---------------------------------------------------------------------------
 *  send_fwdl_to_unifi
 *
 *      Copy a memory segment from userland to the UniFi.
 *      This function reads data, 2K at a time, from userland and writes
 *      it to the UniFi.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      Number of bytes sent (Positive) or negative value indicating
 *      error code:
 *      -CSR_ENOMEM         memory allocation failed
 *      -CSR_EINVAL         error in XBV file
 *      -CSR_EIO            SDIO error
 * ---------------------------------------------------------------------------
 */
static CsrInt32
send_fwdl_to_unifi(card_t *card, void *dlpriv,
              struct FWDL *fwdl, CsrUint32 handle, CsrUint32 op_addr, CsrInt8 can_round)
{
    CsrUint32 addr;
    CsrUint32 offset;
    CsrUint8 *buf;
    CsrInt32 remaining;
    CsrInt32 r;
    /* Loader protocol can handle transfers up to 2K in size */
    const CsrUint16 buf_size = 2*1024;
    /* Allow for 6 byte SDIO loader block header */
    const CsrInt16 maxlen = (buf_size) - 6;

    buf = CsrMemAlloc(buf_size);
    if (buf == NULL) {
        unifi_error(card->ospriv, "Failed to allocate transfer buffer for firmware download\n");
        return -CSR_ENOMEM;
    }

    addr = fwdl->dl_addr;
    offset = fwdl->dl_offset;
    remaining = fwdl->dl_size;

    r = 0;
    while (remaining > 0) {
        CsrInt32 data_len, write_len;

        data_len = remaining;
        if (data_len > maxlen) {
            data_len = maxlen;
        }
        /* Make sure the copy does not cross an 8K boundary */
        if (((addr + data_len) & ~0x1FFF) != (addr & ~0x1FFF)) {
            data_len = ((addr & ~0x1FFF) + 0x2000) - addr;
        }

        if (unifi_fw_read(card->ospriv, dlpriv, offset, buf+6, data_len) != data_len) {
            unifi_error(card->ospriv, "Failed to read from file\n");
            break;
        }


        /* Prepend the SDIO loader block header */
        buf[0] = (CsrUint8)data_len & 0xFF;
        buf[1] = (CsrUint8)(data_len >> 8) & 0xFF;
        buf[2] = (CsrUint8)(addr & 0xFF);
        buf[3] = (CsrUint8)((addr >> 8) & 0xFF);
        buf[4] = (CsrUint8)((addr >> 16) & 0xFF);
        buf[5] = (CsrUint8)((addr >> 24) & 0xFF);
        write_len = data_len + 6;

        if (card->sdio_io_block_pad && can_round) {
            write_len = (data_len + 6 + (card->sdio_io_block_size - 1)) &
                ~(card->sdio_io_block_size - 1);
            /* Zero out the rest of the buffer (This isn't needed, but
             * it makes debugging things later much easier). */
            CsrMemSet(buf + data_len + 6, 0, write_len - (data_len + 6));
        }

        r = unifi_bulk_rw_noretry(card, handle, buf, write_len, UNIFI_SDIO_WRITE);
        if (r) {
            unifi_error(card->ospriv, "CMD53 failed writing %d bytes to handle %ld\n",
                        data_len+6, handle);
            break;
        }

        /*
         * Can chage the order of things to overlap read from file
         * with copy to unifi
         */
        r = unifi_do_loader_op(card, op_addr, UNIFI_LOADER_COPY);
        if (r) {
            break;
        }

#ifdef CSR_WIFI_VERIFY_DOWNLOAD
        verify_download(card, addr, buf+6, data_len);
#endif /* CSR_WIFI_VERIFY_DOWNLOAD */

        addr += data_len;
        offset += data_len;
        remaining -= data_len;
    }

    CsrMemFree(buf);

    if (r && (r != -CSR_ENODEV)) {
        unifi_error(card->ospriv, "Failed to copy block to UniFi after %u bytes of %u\n",
                    (fwdl->dl_size - remaining), fwdl->dl_size);
    }

    return r;
} /* send_fwdl_to_unifi() */


/*
 * ---------------------------------------------------------------------------
 *  send_ptdl_to_unifi
 *
 *      Copy a patch block from userland to the UniFi.
 *      This function reads data, 2K at a time, from userland and writes
 *      it to the UniFi.
 *
 *  Arguments:
 *      card            A pointer to the card structure
 *      dlpriv          The os specific handle for the firmware file
 *      ptdl            A pointer ot the PTDL block
 *      handle          The buffer handle to use for the xfer
 *      op_addr         The address of the loader operation control word
 *
 *  Returns:
 *      Number of bytes sent (Positive) or negative value indicating
 *      error code:
 *      -CSR_ENOMEM         memory allocation failed
 *      -CSR_EINVAL         error in XBV file
 *      -CSR_EIO            SDIO error
 * ---------------------------------------------------------------------------
 */
static CsrInt32
send_ptdl_to_unifi(card_t *card, void *dlpriv,
                   const struct PTDL *ptdl, CsrUint32 handle,
                   CsrUint32 op_addr)
{
    CsrUint32 offset;
    CsrUint8 *buf;
    CsrInt32 data_len;
    CsrUint32 write_len;
    CsrInt32 r;
    const CsrUint16 buf_size = 2*1024;

    offset = ptdl->dl_offset;
    data_len = ptdl->dl_size;

    if (data_len > buf_size) {
        unifi_error(card->ospriv, "PTDL block is too large (%u)\n",
                    ptdl->dl_size);
        return -CSR_EINVAL;
    }

    buf = CsrMemAlloc(buf_size);
    if (buf == NULL) {
        unifi_error(card->ospriv, "Failed to allocate transfer buffer for firmware download\n");
        return -CSR_ENOMEM;
    }

    r = 0;

    if (unifi_fw_read(card->ospriv, dlpriv, offset, buf, data_len) != data_len) {
        unifi_error(card->ospriv, "Failed to read from file\n");
    } else {
        /* We can always round these if the host wants to */
        if (card->sdio_io_block_pad) {
            write_len = (data_len + (card->sdio_io_block_size - 1)) &
                ~(card->sdio_io_block_size - 1);

            /* Zero out the rest of the buffer (This isn't needed, but it
             * makes debugging things later much easier). */
            CsrMemSet(buf + data_len, 0, write_len - data_len);
        } else {
            write_len = data_len;
        }

        r = unifi_bulk_rw_noretry(card, handle, buf, write_len, UNIFI_SDIO_WRITE);
        if (r) {
            unifi_error(card->ospriv, "CMD53 failed writing %d bytes to handle %ld\n",
                        data_len, handle);
        } else {

            /*
             * Can change the order of things to overlap read from file
             * with copy to unifi
             */
            r = unifi_do_loader_op(card, op_addr, UNIFI_BOOT_LOADER_PATCH);
        }
    }

    CsrMemFree(buf);

    if (r && (r != -CSR_ENODEV)) {
        unifi_error(card->ospriv, "Failed to copy block of %u bytes to UniFi\n",
                    ptdl->dl_size);
    }

    return r;
} /* send_ptdl_to_unifi() */


/*
 * ---------------------------------------------------------------------------
 *  do_secondary_download
 *
 *      This function downloads a firmware image to the UniFi using the
 *      SDIO Loader protocol, supported by the secondary loader that is
 *      assumed to have been downloaded and started previously.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      iread           Pointer to a function to read parts of the firmware
 *                      XBV file.
 *      dlpriv          A context pointer from the calling function to be
 *                      passed when calling iread().
 *      pfwinfo         Pointer to a fwinfo struct describing the f/w
 *                      XBV file.
 *
 *  Returns:
 *      0 on success, or CSR error code
 * ---------------------------------------------------------------------------
 */
static CsrInt32
do_secondary_download(card_t *card,
                      void *dlpriv,
                      xbv1_t *pfwinfo)
{
    CsrUint32 slut_address, loader_ctrl_addr;
    CsrUint32 total_bytes = 0;
    CsrUint16 finger_print;
    CsrInt32 i, r;
    CsrUint16 loader_version;
    CsrUint16 handle;

    r = card_wait_for_firmware_to_start(card, &slut_address);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        func_exit();
        return r;
    }
    unifi_trace(card->ospriv, UDBG4, "Stage 2 download: SLUT addr 0x%lX\n", slut_address);

    /*
     * Check the SLUT fingerprint.
     * The slut_address is a generic pointer so we must use unifi_read16().
     */
    unifi_trace(card->ospriv, UDBG4, "Stage 2 download: Looking for SLUT finger print\n");
    finger_print = 0;
    r = unifi_read16(card, slut_address, &finger_print);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Stage 2 download: Failed to read SLUT finger print\n");
        func_exit();
        return r;
    }

    if (finger_print != SLUT_FINGERPRINT) {
        unifi_error(card->ospriv, "Stage 2 download: Failed to find Symbol lookup table fingerprint\n");
        func_exit();
        return -CSR_EINVAL;
    }

    /* Symbol table starts imedately after the fingerprint */
    slut_address += 2;
    loader_ctrl_addr = 0;
    while (!loader_ctrl_addr) {
        CsrUint16 id;

        r = unifi_read16(card, slut_address, &id);
        if (r < 0) {
            unifi_error(card->ospriv, "Stage 2 download: Failed to read SLT address\n");
            func_exit();
            return r;
        }
        slut_address += 2;

        if (id == CSR_SLT_END) {
            break;
        }

        if (id == CSR_SLT_SDIO_LOADER_CONTROL) {
            r = unifi_read32(card, slut_address, &loader_ctrl_addr);
            if (r < 0) {
                unifi_error(card->ospriv, "Stage 2 download: Failed to read SLT_SDIO_LOADER_CONTROL value\n");
                func_exit();
                return r;
            }
            unifi_trace(card->ospriv, UDBG3,
                        "Stage 2 download: SDIO loader control struct @ 0x%08lX\n",
                        loader_ctrl_addr);
        }
        slut_address += 4;
    }

    /*
     * Read info from the SDIO Loader Control Data Structure
     */
    /* Check the loader version */
    r = unifi_read16(card, loader_ctrl_addr, &loader_version);
    if (r < 0) {
        unifi_error(card->ospriv, "Stage 2 download: Failed to read loader version\n");
        func_exit();
        return r;
    }
    unifi_trace(card->ospriv, UDBG2, "Stage 2 download: SDIO loader version 0x%04X\n", loader_version);
    switch (loader_version) {
      case 0x0000:
      case 0x0001:
        /* These are the same, but with version 1 we can round up the
         * length of data transfered (so we only need one CMD53, not
         * two). */
        break;

      default:
        unifi_error(card->ospriv, "Secondary loader version (0x%04X) is not supported by this driver\n",
                    loader_version);
        return -CSR_EINVAL;
    }

    /* Retrieve the handle to use with CMD53 */
    r = unifi_read16(card, loader_ctrl_addr+4, &handle);
    if (r < 0) {
        unifi_error(card->ospriv, "Stage 2 download: Failed to read loader handle\n");
        func_exit();
        return r;
    }

    /* Set the mask of LEDs to flash */
    if (card->loader_led_mask) {
        r = unifi_write16(card, loader_ctrl_addr+2, (CsrUint16)card->loader_led_mask);
        if (r < 0) {
            unifi_error(card->ospriv, "Stage 2 download: Failed to write LED mask\n");
            func_exit();
            return r;
        }
    }

    /* Copy download data to UniFi memory */
    for (i = 0; i < pfwinfo->num_fwdl; i++)
    {
        unifi_trace(card->ospriv, UDBG3, "Stage 2 download: Downloading to 0x%lX for %d from offset %d\n",
              pfwinfo->fwdl[i].dl_addr,
              pfwinfo->fwdl[i].dl_size,
              pfwinfo->fwdl[i].dl_offset);
        r = send_fwdl_to_unifi(card, dlpriv, &pfwinfo->fwdl[i],
                               handle, loader_ctrl_addr+6,
                               loader_version != 0x0000);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Secondary download failed after %u bytes\n",
            total_bytes);
            return r;
        }
        total_bytes += pfwinfo->fwdl[i].dl_size;
    }

    return 0;
} /* do_secondary_download() */



/*
 * ---------------------------------------------------------------------------
 *  do_patch_download
 *
 *      This function downloads a set of patches to UniFi and then
 *      causes it to restart.
 *
 *  Arguments:
 *      card            Pointer to card struct.
 *      dlpriv          A context pointer from the calling function to be
 *                      used when reading the XBV file.  This can be NULL
 *                      in which case not patches are applied.
 *      pfwinfo         Pointer to a fwinfo struct describing the f/w
 *                      XBV file.
 *      boot_ctrl_addr  The address of the boot loader control structure.
 *
 *  Returns:
 *      0 on success, or an error code
 *      -CSR_EINVAL for a bad laoader version number
 * ---------------------------------------------------------------------------
 */
static CsrInt32
do_patch_download(card_t *card, void *dlpriv, xbv1_t *pfwinfo, CsrUint32 boot_ctrl_addr)
{
    CsrInt32 r, i;
    CsrUint16 loader_version;
    CsrUint16 handle;
    CsrUint32 total_bytes;

    /*
     * Read info from the SDIO Loader Control Data Structure
     */
    /* Check the loader version */
    r = unifi_read16(card, boot_ctrl_addr, &loader_version);
    if (r < 0) {
        unifi_error(card->ospriv, "Patch download: Failed to read loader version\n");
        return r;
    }
    unifi_trace(card->ospriv, UDBG2, "Patch download: boot loader version 0x%04X\n", loader_version);
    switch (loader_version) {
      case 0x0000:
        break;

      default:
        unifi_error(card->ospriv, "Patch loader version (0x%04X) is not supported by this driver\n",
                    loader_version);
        return -CSR_EINVAL;
    }

    /* Retrieve the handle to use with CMD53 */
    r = unifi_read16(card, boot_ctrl_addr+4, &handle);
    if (r < 0) {
        unifi_error(card->ospriv, "Patch download: Failed to read loader handle\n");
        return r;
    }

    /* Set the mask of LEDs to flash */
    if (card->loader_led_mask) {
        r = unifi_write16(card, boot_ctrl_addr+2,
                          (CsrUint16)card->loader_led_mask);
        if (r < 0) {
            unifi_error(card->ospriv, "Patch download: Failed to write LED mask\n");
            return r;
        }
    }

    total_bytes = 0;

    /* Copy download data to UniFi memory */
    for (i = 0; i < pfwinfo->num_ptdl; i++)
    {
        unifi_trace(card->ospriv, UDBG3, "Patch download: %d Downloading for %d from offset %d\n",
                    i,
                    pfwinfo->ptdl[i].dl_size,
                    pfwinfo->ptdl[i].dl_offset);

        r = send_ptdl_to_unifi(card, dlpriv, &pfwinfo->ptdl[i],
                                handle, boot_ctrl_addr+6);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Patch failed after %u bytes\n",
                        total_bytes);
            return r;
        }
        total_bytes += pfwinfo->ptdl[i].dl_size;
    }

    return 0;
} /* do_patch_download() */



/*
 * ---------------------------------------------------------------------------
 *  verify_download
 *
 *      Function to verify a segment of the download by reading back
 *      memory locations.
 *      This is extremely slow!
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      addr            Starting memory address of segment.
 *      buf             Pointer to download data that should be present in
 *                      memory.
 *      len             Number of bytes to check at this address.
 *
 *  Returns:
 *      Number of errors, i.e. bytes that differ from expected.
 * ---------------------------------------------------------------------------
 */
#ifdef CSR_WIFI_VERIFY_DOWNLOAD
static CsrInt32
verify_download(card_t *card, CsrUint32 addr, CsrUint8 *buf, CsrUint16 len)
{
    CsrInt32 a, r;
    CsrUint16 w;
    CsrInt32 errors = 0;

    unifi_info(card->ospriv, "verifying download to 0x%lX for %d bytes (src ptr %p)...\n",
               addr, len, buf);

    for (a = 0; a < len; a+=2) {
        r = unifi_read16(card, addr+a, &w);
        if (r) {
            unifi_error(card->ospriv, "Failed to read addr 0x%lX while verifying download\n",
                        addr+a);
            return r;
        }
        if (w != *((CsrUint16*)(buf+a))) {
            if (errors == 0) {
                unifi_error(card->ospriv,
                            "Verify failed @ 0x%08lX: expected %04X, read %04X\n",
                            addr+a, *((CsrUint16*)(buf+a)), w);
            }
            errors++;
        }
    }

    if (errors) {
        unifi_error(card->ospriv, "Verify of download to 0x%lX for %d bytes failed: %d errors\n",
                    addr, len, errors);
    }

    return errors;
} /* verify_download() */
#endif /* CSR_WIFI_VERIFY_DOWNLOAD */
