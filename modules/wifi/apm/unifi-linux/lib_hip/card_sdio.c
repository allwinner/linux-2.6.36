/*
 * ---------------------------------------------------------------------------
 * FILE: card_sdio.c
 *
 * PURPOSE: Implementation of the Card API for SDIO.
 *
 * NOTES:
 *      CardInit() is called from the SDIO probe callback when a card is
 *      inserted. This performs the basic SDIO initialisation, enabling i/o
 *      etc.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "driver/conversions.h"
#include "driver/unifiversion.h"
#include "card.h"
#include "card_sdio.h"
#include "chiphelper.h"



/* Time to wait between attempts to read MAILBOX0 */
#define MAILBOX1_TIMEOUT                10      /* in millisecs */
#define MAILBOX1_ATTEMPTS               200     /* 2 seconds */

#define MAILBOX2_TIMEOUT                5       /* in millisecs */
#define MAILBOX2_ATTEMPTS               10      /* 50ms */

#define MAILBOX2_RESET_ATTEMPTS         10
#define MAILBOX2_RESET_TIMEOUT          5       /* in millisecs */

#define RESET_SETTLE_DELAY              25      /* in millisecs */


static CsrInt32 card_init_slots(card_t *card);
static CsrInt32 card_hw_init(card_t *card);
static CsrInt32 firmware_present_in_flash(card_t *card);
static void bootstrap_chip_hw(card_t *card);
static CsrInt32 unifi_reset_hardware(card_t *card);
static CsrInt32 CardHardReset(card_t *card);
static CsrInt32 unifi_hip_init(card_t *card);

extern int coredump_max;

/*
 * ---------------------------------------------------------------------------
 *  unifi_sdio_init
 *
 *      Legacy function, kept for backwards compatibility. It doesn't
 *      report enough status when an error occurs to be able to determine
 *      the correct recovery action.
 *      The body is split into:
 *              unifi_init_card()
 *              unifi_alloc_card()
 *
 *  Arguments:
 *      sdio            Pointer to SDIO context pointer to pass to low
 *                      level i/o functions.
 *      ospriv          Pointer to O/S private struct to pass when calling
 *                      callbacks to the higher level system.
 *
 *  Returns:
 *      Pointer to card struct, which represents the driver context,
 *      or NULL on error.
 *
 *  Notes:
 *      Doesn't do anything that involves interaction with the SDIO f/w,
 *      as this takes a bit longer to start up.
 * ---------------------------------------------------------------------------
 */
card_t *
unifi_sdio_init(void *sdio, void *ospriv)
{
    card_t *card;

    card = unifi_alloc_card(sdio, ospriv);
    if (card == NULL) {
        return NULL;
    }

    if (unifi_init_card(card, 0x0c00) < 0) {
        unifi_free_card(card);
        return NULL;
    }

    return card;

} /* unifi_sdio_init() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_alloc_card
 *
 *      Allocate and initialise the card context structure.
 *
 *  Arguments:
 *      sdio            Pointer to SDIO context pointer to pass to low
 *                      level i/o functions.
 *      ospriv          Pointer to O/S private struct to pass when calling
 *                      callbacks to the higher level system.
 *
 *  Returns:
 *      Pointer to card struct, which represents the driver context or
 *      NULL if the allocation failed.
 * ---------------------------------------------------------------------------
 */
card_t *
unifi_alloc_card(CsrSdioFunction *sdio, void *ospriv)
{
    card_t *card;
    CsrUint32 i, r;

    func_enter();


    card = (card_t*)CsrMemAlloc(sizeof(card_t));
    if (card == NULL) {
        return NULL;
    }
    CsrMemSet(card, 0, sizeof(card_t));


    card->sdio_if = sdio;
    card->ospriv  = ospriv;

    card->unifi_interrupt_seq = 1;

    /* Make these invalid. */
    card->proc_select = (CsrUint32)(-1);
    card->dmem_page = (CsrUint32)(-1);
    card->pmem_page = (CsrUint32)(-1);

    card->bh_reason_host = 0;
    card->bh_reason_unifi = 0;

    for (i = 0; i < sizeof(card->tx_q_paused_flag)/sizeof(card->tx_q_paused_flag[0]); i++)
    {
        card->tx_q_paused_flag[i] = 0;
    }
    card->memory_resources_allocated = 0;

    card->low_power_mode = UNIFI_LOW_POWER_DISABLED;
    card->periodic_wake_mode = UNIFI_PERIODIC_WAKE_HOST_DISABLED;

    card->host_state = UNIFI_HOST_STATE_AWAKE;
    card->intmode = CSR_WIFI_INTMODE_DEFAULT;

    /*
     * Memory resources for buffers are allocated when the chip is initialised
     * because we need configuration information from the firmware.
     */

    /*
     * Initialise wait queues and lists
     */
    card->fh_command_queue.q_body = card->fh_command_q_body;
    card->fh_command_queue.q_length = UNIFI_SOFT_COMMAND_Q_LENGTH;

    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        card->fh_traffic_queue[i].q_body = card->fh_traffic_q_body[i];
        card->fh_traffic_queue[i].q_length = UNIFI_SOFT_TRAFFIC_Q_LENGTH;
    }


    /* Initialise the auto-coredump capture buffers */
    r = unifi_coredump_init(card, (CsrUint16)coredump_max);
    if (r) {
        unifi_error(card->ospriv, "Couldn't allocate coredump buffers\n");
    }

    func_exit();
    return card;
} /* unifi_alloc_card() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_init_card
 *
 *      Reset the hardware, load the firmware and perform HIP initialization
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      0 on success
 *      1 success and chip needs firmware download
 *      -CSR error code on failure.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_init_card(card_t *card, CsrInt32 led_mask)
{
    CsrInt32 r;

    func_enter();

    if (card == NULL) {
        func_exit_r(-CSR_EINVAL);
        return -CSR_EINVAL;
    }

    r = unifi_init(card);

    if(r==1) {
        /* chip needs firmware download */
        r = unifi_download(card, led_mask);
    }

    if(r) {
        func_exit_r(r);
        return r;
    }

    r = unifi_hip_init(card);
    if (r == -CSR_ENODEV) {
        func_exit_r(r);
        return r;
    }
    if (r) {
        unifi_error(card->ospriv, "Failed to start host protocol.\n");
        func_exit_r(r);
        return r;
    }

    func_exit();
    return 0;

}

/*
 * ---------------------------------------------------------------------------
 *  unifi_init
 *
 *      Init the hardware.
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      0 on success and chip doesn't need firmware download
 *      1 on success and chip needs firmware download
 *      -CSR error code on failure.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_init(card_t *card)
{
    CsrInt32 r;

    func_enter();

    if (card == NULL) {
        func_exit_r(-CSR_EINVAL);
        return -CSR_EINVAL;
    }

    /*
     * Disable the SDIO interrupts while initialising UniFi.
     * Re-enable them when f/w is running.
     */
    r = CsrSdioInterruptDisable(card->sdio_if);
    if (r == -CSR_ENODEV) return r;

    /*
     * UniFi's PLL may start with a slow clock (~ 1 MHz) so initially
     * set the SDIO bus clock to a similar value or SDIO accesses may
     * fail.
     */
    r = csr_sdio_set_max_clock_speed(card->sdio_if, UNIFI_SDIO_CLOCK_SAFE);
    if (r < 0) {
        func_exit_r(r);
        return r;
    }
    card->sdio_clock_speed = UNIFI_SDIO_CLOCK_SAFE;

    /*
     * Reset UniFi. Note, this only resets the WLAN function part of the chip,
     * the SDIO interface is not reset.
     */
    unifi_trace(card->ospriv, UDBG1, "Resetting UniFi\n");
    r = unifi_reset_hardware(card);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to reset UniFi\n");
        func_exit_r(r);
        return r;
    }

    /* Reset the power save mode, to be active until the MLME-reset is complete */
    r = unifi_configure_low_power_mode(card,
            UNIFI_LOW_POWER_DISABLED, UNIFI_PERIODIC_WAKE_HOST_DISABLED);
    if (r) {
        unifi_error(card->ospriv, "Failed to set power save mode\n");
        func_exit_r(r);
        return r;
    }
    
    /*
     * Set initial value of page registers.
     * The page registers will be maintained by unifi_read...() and
     * unifi_write...().
     */
    card->proc_select = (CsrUint32)(-1);
    card->dmem_page = (CsrUint32)(-1);
    card->pmem_page = (CsrUint32)(-1);
    r = unifi_write_direct16(card, ChipHelper_HOST_WINDOW3_PAGE(card->helper) * 2, 0);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to write SHARED_DMEM_PAGE\n");
        func_exit_r(r);
        return r;
    }
    r = unifi_write_direct16(card, ChipHelper_HOST_WINDOW2_PAGE(card->helper) * 2, 0);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to write PROG_MEM2_PAGE\n");
        func_exit_r(r);
        return r;
    }

    /*
     * If the driver has reset UniFi due to previous SDIO failure, this may
     * have been due to a chip watchdog reset. In this case, the driver may
     * have requested a mini-coredump which needs to be captured now the
     * SDIO interface is alive.
     */
    unifi_coredump_handle_request(card);

    /*
     * Probe to see if the UniFi has Flash to boot from.
     * If not (i.e. it needs a RAM download), stop it now to prevent
     * the watchdog going off.
     */
    r = firmware_present_in_flash(card);
    if (r == -CSR_ENODEV) return r;
    if (r < 0) {
        unifi_error(card->ospriv, "Probe for Flash failed\n");
        func_exit_r(r);
        return r;
    }

    if (r == 0) {
        /* needs download */
        unifi_notice(card->ospriv, "needs firmware\n");

        /* Stop the processor now to avoid confusion if the watchdog goes off
         */
        r = card_stop_processor(card, UNIFI_PROC_BOTH);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to stop UniFi processors\n");
            func_exit_r(r);
            return r;
        }

        /* return 1 to indicate success and chip needs firmware download */
        r = 1;
    } else {
        r = 0;
        /* return 0 to indicate success and chip doesn't need firmware download */
    }

    func_exit();

    return r;

} /* unifi_init() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_download
 *
 *      Load the firmware.
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      0 on success
 *      -CSR error code on failure.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_download(card_t *card, CsrInt32 led_mask)
{
    CsrInt32 r;
    void *dlpriv;
    CsrInt8 stage;

    func_enter();

    if (card == NULL) {
        func_exit_r(-CSR_EINVAL);
        return -CSR_EINVAL;
    }

    /* Set the loader led mask */
    card->loader_led_mask = led_mask;

    /* 0 means use CMD52's, 1 means use CMD53's */
    stage = 0;

    /* Get the loader file information */
    unifi_trace(card->ospriv, UDBG1, "downloading loader...\n");

    dlpriv = unifi_dl_fw_read_start(card, UNIFI_FW_LOADER);
    if (dlpriv == NULL) {
        unifi_info(card->ospriv, "No loader found, loading firmware the slow way\n");
    } else {

        /* Download the loader. */
        r = unifi_dl_firmware(card, dlpriv, stage);
        if (r) {
            unifi_error(card->ospriv, "Failed to download loader\n");
            func_exit_r(r);
            return r;
        }

        /* Free the loader file information. */
        unifi_fw_read_stop(card->ospriv, dlpriv);

        /* We can now load the firmware with CMD53's */
        stage++;
    }

    /* Get the firmware file information */
    unifi_trace(card->ospriv, UDBG1, "downloading firmware...\n");

    dlpriv = unifi_dl_fw_read_start(card, UNIFI_FW_STA);
    if (dlpriv == NULL) {
        func_exit_r(-CSR_ENODATA);
        return -CSR_ENODATA;
    }

    /* Download the firmware. */
    r = unifi_dl_firmware(card, dlpriv, stage);
    if (r) {
        unifi_error(card->ospriv, "Failed to download firmware\n");
        func_exit_r(r);
        return r;
    }

    /* Free the firmware file information. */
    unifi_fw_read_stop(card->ospriv, dlpriv);

    func_exit();

    return 0;
} /* unifi_download() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_hip_init
 *
 *      This function performs the f/w initialisation sequence as described
 *      in the Unifi Host Interface Protocol Specification.
 *      It allocates memory for host-side slot data and signal queues.
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      0 on success or else a CSR error code
 *
 *  Notes:
 *      The firmware must have been downloaded.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_hip_init(card_t *card)
{
    CsrInt32 r;

    func_enter();

    r = card_hw_init(card);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to establish communication with UniFi\n");
        func_exit_r(r);
        return r;
    }

    /*
     * Allocate memory for host-side slot data and signal queues.
     * We need the config info read from the firmware to know how much
     * memory to allocate.
     */
    r = card_init_slots(card);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Init slots failed: %d\n", r);
        func_exit_r(r);
        return r;
    }

    unifi_trace(card->ospriv, UDBG2, "Sending first UniFi interrupt\n");

    r = unifi_set_host_state(card, UNIFI_HOST_STATE_AWAKE);
    if (r) {
        func_exit_r(r);
        return r;
    }

    /* Enable the SDIO interrupts now that the f/w is running. */
    r = CsrSdioInterruptEnable(card->sdio_if);
    if (r == -CSR_ENODEV) return r;

    /* Signal the UniFi to start handling messages */
    r = CardGenInt(card);
    if (r) {
        func_exit_r(r);
        return r;
    }


    func_exit();

    return 0;
} /* unifi_hip_init() */



/*
 * ---------------------------------------------------------------------------
 *  _build_sdio_config_data
 *
 *      Unpack the SDIO configuration information from a buffer read from
 *      UniFi into a host structure.
 *      The data is byte-swapped for a big-endian host if necessary by the
 *      UNPACK... macros.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      cfg_data        Destination structure to unpack into.
 *      cfg_data_buf    Source buffer to read from. This should be the raw
 *                      data read from UniFi.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
_build_sdio_config_data(sdio_config_data_t *cfg_data,
                        const CsrUint8 *cfg_data_buf)
{
    CsrInt16 offset = 0;

    cfg_data->version = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->sdio_ctrl_offset = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->fromhost_sigbuf_handle = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->tohost_sigbuf_handle = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->num_fromhost_sig_frags = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->num_tohost_sig_frags = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->num_fromhost_data_slots = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->num_tohost_data_slots = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->data_slot_size = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->initialised = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->overlay_size = COAL_GET_UINT32_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT32;

    cfg_data->data_slot_round = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->sig_frag_size = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);
    offset += SIZEOF_UINT16;

    cfg_data->tohost_signal_padding = COAL_GET_UINT16_FROM_LITTLE_ENDIAN(cfg_data_buf + offset);

} /* _build_sdio_config_data() */

/*
 * - Function ----------------------------------------------------------------
 * card_hw_init()
 *
 *      Perform the initialisation procedure described in the UniFi Host
 *      Interface Protocol document (section 3.3.8) and read the run-time
 *      configuration information from the UniFi. This is stuff like number
 *      of bulk data slots etc.
 *
 *      The card enumeration and SD initialisation has already been done by
 *      the SDIO library, see card_sdio_init().
 *
 *      The initialisation is done when firmware is ready, i.e. this may need
 *      to be called after a f/w download operation.
 *
 *      The initialisation procedure goes like this:
 *       - Wait for UniFi to start-up by polling SHARED_MAILBOX1
 *       - Find the symbol table and look up SLT_SDIO_SLOT_CONFIG
 *       - Read the config structure
 *       - Check the "SDIO initialised" flag, if not zero do a h/w reset and
 *         start again
 *       - Decide the number of bulk data slots to allocate, allocate them and
 *         set "SDIO initialised" flag (and generate an interrupt) to say so.
 *
 * Arguments:
 *      card        Pointer to card struct
 *
 * Returns:
 *      0 on success,
 *      a CSR error code on failure
 *
 * Notes:
 *      All data in the f/w is stored in a little endian format, without any
 *      padding bytes. Every read from this memory has to be transformed in
 *      host (cpu specific) format, before it is stored in driver's parameters
 *      or/and structures. Athough unifi_read16() and unifi_read32() do perform
 *      the convertion internally, unifi_readn() does not.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_hw_init(card_t *card)
{
    CsrUint32 slut_address;
    CsrUint16 initialised;
    CsrUint16 finger_print;
    symbol_t slut;
    sdio_config_data_t *cfg_data;
    CsrUint8 cfg_data_buf[SDIO_CONFIG_DATA_SIZE];
    CsrInt32 r;
    void *dlpriv;
    CsrInt16 major, minor;
    CsrInt16 search_4slut_again;

    func_enter();

    /*
     * The device revision from the TPLMID_MANF and TPLMID_CARD fields
     * of the CIS are available as
     *   card->sdio_if->pDevice->ManfID
     *   card->sdio_if->pDevice->AppID
     */

    /*
     * Run in a loop so we can patch.
     */
    do {
        /* Reset these each time around the loop. */
        search_4slut_again = 0;
        cfg_data = NULL;

        r = card_wait_for_firmware_to_start(card, &slut_address);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Firmware hasn't started\n");
            func_exit_r(r);
            return r;
        }
        unifi_trace(card->ospriv, UDBG4, "SLUT addr 0x%lX\n", slut_address);

        /*
         * Firmware has started, but doesn't know full clock configuration yet
         * as some of the information may be in the MIB. Therefore we set an
         * initial SDIO clock speed, faster than UNIFI_SDIO_CLOCK_SAFE, for
         * the patch download and subsequent firmware initialisation, and
         * full speed UNIFI_SDIO_CLOCK_MAX will be set once the f/w tells us
         * that it is ready.
         */
        r = csr_sdio_set_max_clock_speed(card->sdio_if, UNIFI_SDIO_CLOCK_INIT);
        if (r < 0) {
            func_exit_r(r);
            return r;
        }
        card->sdio_clock_speed = UNIFI_SDIO_CLOCK_INIT;

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
            unifi_error(card->ospriv, "Failed to find Symbol lookup table fingerprint\n");
            func_exit_r(-CSR_EIO);
            return -CSR_EIO;
        }

        /* Symbol table starts imedately after the fingerprint */
        slut_address += 2;

        /* Search the table until either the end marker is found, or the
         * loading of patch firmware invalidates the current table.
         */
        while (!search_4slut_again) {

            CsrUint16 s;
            CsrUint32 l;

            r = unifi_read16(card, slut_address, &s);
            if (r < 0) {
                func_exit_r(r);
                return r;
            }
            slut_address += 2;

            if (s == CSR_SLT_END) {
                unifi_trace(card->ospriv, UDBG3, "  found CSR_SLT_END\n");
                break;
            }

            r = unifi_read32(card, slut_address, &l);
            if (r < 0) {
                func_exit_r(r);
                return r;
            }
            slut_address += 4;

            slut.id = s;
            slut.obj = l;

            unifi_trace(card->ospriv, UDBG3, "  found SLUT id %02d.%08lx\n", slut.id, slut.obj);
            switch (slut.id) {

            case CSR_SLT_SDIO_SLOT_CONFIG:
                cfg_data = &card->config_data;
                /*
                 * unifi_readn reads n bytes from the card, where data is stored
                 * in a little endian format, without any padding bytes. So, we
                 * can not just pass the cfg_data pointer or use the
                 * sizeof(sdio_config_data_t) since the structure in the host can
                 * be big endian formatted or have padding bytes for alignment.
                 * We use a char buffer to read the data from the card.
                 */
                r = unifi_readn(card, slut.obj, cfg_data_buf, SDIO_CONFIG_DATA_SIZE);
                if (r == -CSR_ENODEV) return r;
                if (r) {
                    unifi_error(card->ospriv, "Failed to read config data\n");
                    func_exit_r(r);
                    return r;
                }
                /* .. and then we copy the data to the host structure */
                _build_sdio_config_data(cfg_data, cfg_data_buf);

                /* Make sure the from host data slots are what we expect
                   we reserve 2 for commands and there should be at least
                   1 left for each access category */
                if ((cfg_data->num_fromhost_data_slots - RESERVED_COMMAND_SLOTS)/4 < 1) {
                    unifi_error(card->ospriv, "From host data slots %d\n", cfg_data->num_fromhost_data_slots);
                    unifi_error(card->ospriv, "need to be (queues * x + 2) (RESERVED_COMMAND_SLOTS for commands)\n");
                    func_exit_r(-CSR_EINVAL);
                    return -CSR_EINVAL;
                }

                /* Configure SDIO to-block-size padding */
                if (card->sdio_io_block_pad) {
                    /*
                     * Firmware limits the maximum padding size via data_slot_round.
                     * Therefore when padding to whole block sizes, the block size
                     * must be configured correctly by adjusting CSR_WIFI_HIP_SDIO_BLOCK_SIZE.
                     */
                    if (cfg_data->data_slot_round < card->sdio_io_block_size) {
                        unifi_error(card->ospriv,
                                    "Configuration error: Block size of %d exceeds f/w data_slot_round of %d\n",
                                    card->sdio_io_block_size, cfg_data->data_slot_round);
                        return -CSR_EINVAL;
                    }

                    /*
                     * To force the To-Host signals to be rounded up to the SDIO block
                     * size, we need to write the To-Host Signal Padding Fragments
                     * field of the SDIO configuration in UniFi.
                     */
                    if ((card->sdio_io_block_size % cfg_data->sig_frag_size) != 0) {
                        unifi_error(card->ospriv, "Configuration error: Can not pad to-host signals.\n");
                        func_exit_r(-CSR_EINVAL);
                        return -CSR_EINVAL;
                    }
                    cfg_data->tohost_signal_padding = (CsrUint16) (card->sdio_io_block_size / cfg_data->sig_frag_size);
                    r = unifi_write16(card, slut.obj + SDIO_TO_HOST_SIG_PADDING_OFFSET, cfg_data->tohost_signal_padding);
                    if (r == -CSR_ENODEV) return r;
                    if (r) {
                        unifi_error(card->ospriv, "Failed to write To-Host Signal Padding Fragments\n");
                        func_exit_r(r);
                        return r;
                    }
                }

                /* Reconstruct the Generic Pointer address of the
                 * SDIO Control Data Struct.
                 */
                card->sdio_ctrl_addr = cfg_data->sdio_ctrl_offset | (UNIFI_SH_DMEM << 24);
                card->init_flag_addr = slut.obj + SDIO_INIT_FLAG_OFFSET;
                break;

            case CSR_SLT_BUILD_ID_NUMBER:
            {
                CsrUint32 n;
                r = unifi_read32(card, slut.obj, &n);
                if (r == -CSR_ENODEV) return r;
                if (r) {
                    unifi_error(card->ospriv, "Failed to read build id\n");
                    func_exit_r(r);
                    return r;
                }
                card->build_id = n;
            }
            break;

            case CSR_SLT_BUILD_ID_STRING:
                r = unifi_readnz(card, slut.obj, card->build_id_string,
                                 sizeof(card->build_id_string));
                if (r == -CSR_ENODEV) return r;
                if (r) {
                    unifi_error(card->ospriv, "Failed to read build string\n");
                    func_exit_r(r);
                    return r;
                }
                break;

            case CSR_SLT_PERSISTENT_STORE_DB:
                break;

            case CSR_SLT_BOOT_LOADER_CONTROL:

                /* This command copies most of the station firmware
                 * image from ROM into program RAM.  It also clears
                 * out the zerod data and sets up the initialised
                 * data. */
                r = unifi_do_loader_op(card, slut.obj+6, UNIFI_BOOT_LOADER_LOAD_STA);
                if (r) {
                    unifi_error(card->ospriv, "Failed to write loader load image command\n");
                    func_exit_r(r);
                    return r;
                }

                dlpriv = unifi_dl_fw_read_start(card, UNIFI_FW_STA);

                /* dlpriv might be NULL, we still need to do the do_loader_op step. */
                if (dlpriv != NULL) {
                    /* Download the firmware. */
                    r = unifi_dl_patch(card, dlpriv, slut.obj);

                    /* Free the firmware file information. */
                    unifi_fw_read_stop(card->ospriv, dlpriv);

                    if (r) {
                        unifi_error(card->ospriv, "Failed to patch firmware\n");
                        func_exit_r(r);
                        return r;
                    }
                }

                /* This command starts the firmware image that we want (the
                * station by default) with any patches required applied. */
                r = unifi_do_loader_op(card, slut.obj+6, UNIFI_BOOT_LOADER_RESTART);
                if (r) {
                    unifi_error(card->ospriv, "Failed to write loader restart command\n");
                    func_exit_r(r);
                    return r;
                }

                /* The now running patch f/w defines a new SLUT data structure -
                 * the current one is no longer valid. We must drop out of the
                 * processing loop and enumerate the new SLUT (which may appear
                 * at a different offset).
                 */
                search_4slut_again = 1;
                break;

            default:
                /* do nothing */
                break;
            }
        } /* while */
    } while (search_4slut_again);

    /* Did we find the Config Data ? */
    if (cfg_data == NULL) {
        unifi_error(card->ospriv, "Failed to find SDIO_SLOT_CONFIG Symbol\n");
        func_exit_r(-CSR_EIO);
        return -CSR_EIO;
    }

    /*
     * Has ths card already been initialised?
     * If so, return an error so we do a h/w reset and start again.
     */
    r = unifi_read16(card, card->init_flag_addr, &initialised);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to read init flag at %08lx\n",
                    card->init_flag_addr);
        func_exit_r(r);
        return r;
    }
    if (initialised != 0) {
        func_exit_r(-CSR_EIO);
        return -CSR_EIO;
    }


    /*
     * Now check the UniFi firmware version
     */
    major = (cfg_data->version >> 8) & 0xFF;
    minor = cfg_data->version & 0xFF;
    unifi_info(card->ospriv, "UniFi f/w protocol version %d.%d (driver %d.%d)\n",
               major, minor,
               UNIFI_HIP_MAJOR_VERSION, UNIFI_HIP_MINOR_VERSION);

    unifi_info(card->ospriv, "Firmware build %d: %s\n",
               card->build_id, card->build_id_string);

    if (major != UNIFI_HIP_MAJOR_VERSION) {
        unifi_error(card->ospriv, "UniFi f/w protocol major version (%d) is different from driver (v%d.%d)\n",
                    major, UNIFI_HIP_MAJOR_VERSION, UNIFI_HIP_MINOR_VERSION);
        func_exit_r(-CSR_EIO);
        return -CSR_EIO;
    }
    if (minor < UNIFI_HIP_MINOR_VERSION) {
        unifi_error(card->ospriv, "UniFi f/w protocol version (v%d.%d) is older than minimum required by driver (v%d.%d).\n",
                    major, minor,
                    UNIFI_HIP_MAJOR_VERSION, UNIFI_HIP_MINOR_VERSION);
        func_exit_r(-CSR_EIO);
        return -CSR_EIO;
    }

    func_exit();
    return 0;

} /* card_hw_init() */



/*
 * ---------------------------------------------------------------------------
 *  card_wait_for_unifi_to_reset
 *
 *      Waits for a reset to complete by polling the WLAN function enable
 *      bit (which is cleared on reset).
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      0 on success, CSR error code on failure.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_wait_for_unifi_to_reset(card_t *card)
{
    CsrInt16 i;
    CsrInt32 r;
    CsrUint8 io_enable;

    func_enter();

    r = 0;
    for (i = 0; i < MAILBOX2_ATTEMPTS; i++) {
        unifi_trace(card->ospriv, UDBG1, "waiting for reset to complete, attempt %d\n", i);
        if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {
            /* It's quite likely that this read will timeout for the
             * first few tries - especially if we have reset via
             * DBG_RESET.
             */
            r = CsrSdioF0Read8(card->sdio_if, SDIO_IO_READY, &io_enable);
        } else {
            r = sdio_read_f0(card, SDIO_IO_ENABLE, &io_enable);
        }
        if (r == -CSR_ENODEV) return r;
        if (r == 0) {
            CsrUint16 mbox2;
            CsrInt16 enabled = io_enable & (1 << card->function);

            if (!enabled) {
                unifi_trace(card->ospriv, UDBG1,
                            "Reset complete (function %d is disabled) in ~ %u msecs\n",
                            card->function, i * MAILBOX2_TIMEOUT);

                /* Enable WLAN function and verify MAILBOX2 is zero'd */
                r = CsrSdioFunctionEnable(card->sdio_if);
                if (r) {
                    unifi_error(card->ospriv, "CsrSdioFunctionEnable failed %d\n", r);
                    break;
                }
            }

            r = unifi_read_direct16(card, ChipHelper_SDIO_HIP_HANDSHAKE(card->helper) * 2, &mbox2);
            if (r)
            {
                unifi_error(card->ospriv, "read HIP_HANDSHAKE failed %d\n", r);
                break;
            }
            if (mbox2 != 0) {
                unifi_error(card->ospriv, "MAILBOX2 non-zero after reset (mbox2 = %04x)\n", mbox2);
                r = -CSR_EIO;
            }
            break;

        } else {
            if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {
                /* We ignore read failures for the first few reads,
                 * they are probably benign. */
                if (i > MAILBOX2_ATTEMPTS / 4) {
                    unifi_trace(card->ospriv, UDBG1, "Failed to read CCCR IO Ready register while polling for reset\n");
                }
            } else {
                unifi_trace(card->ospriv, UDBG1, "Failed to read CCCR IO Enable register while polling for reset\n");
            }
        }
        CsrThreadSleep(MAILBOX2_TIMEOUT);
    }

    if ((r == 0) && (i == MAILBOX2_ATTEMPTS)) {
        unifi_trace(card->ospriv, UDBG1, "Timeout waiting for UniFi to complete reset\n");
        r = -CSR_ETIMEDOUT;
    }

    func_exit();
    return r;
} /* card_wait_for_unifi_to_reset() */


/*
 * ---------------------------------------------------------------------------
 *  card_wait_for_unifi_to_disable
 *
 *      Waits for the function to become disabled by polling the
 *      IO_READY bit.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      0 on success, CSR error code on failure.
 *
 *  Notes: This function can only be used with
 *         card->chip_id > SDIO_CARD_ID_UNIFI_2
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_wait_for_unifi_to_disable(card_t *card)
{
    CsrInt16 i;
    CsrInt32 r;
    CsrUint8 io_enable;

    func_enter();

    if (card->chip_id <= SDIO_CARD_ID_UNIFI_2) {
        unifi_error(card->ospriv,
                    "Function reset method not supported for chip_id=%d\n",
                    card->chip_id);
        func_exit();
        return -CSR_EIO;
    }

    r = 0;
    for (i = 0; i < MAILBOX2_ATTEMPTS; i++) {
        unifi_trace(card->ospriv, UDBG1, "waiting for disable to complete, attempt %d\n", i);

        /*
         * It's quite likely that this read will timeout for the
         * first few tries - especially if we have reset via
         * DBG_RESET.
         */
        r = CsrSdioF0Read8(card->sdio_if, SDIO_IO_READY, &io_enable);
        if (r == -CSR_ENODEV) return r;
        if (r == 0) {
            CsrInt16 enabled = io_enable & (1 << card->function);

            if (!enabled) {
                unifi_trace(card->ospriv, UDBG1,
                            "Disable complete (function %d is disabled) in ~ %u msecs\n",
                            card->function, i * MAILBOX2_TIMEOUT);

                break;
            }
        } else {
            /*
             * We ignore read failures for the first few reads,
             * they are probably benign.
             */
            if (i > (MAILBOX2_ATTEMPTS / 4)) {
                unifi_trace(card->ospriv, UDBG1,
                            "Failed to read CCCR IO Ready register while polling for disable\n");
            }
        }
        CsrThreadSleep(MAILBOX2_TIMEOUT);
    }

    if ((r == 0) && (i == MAILBOX2_ATTEMPTS)) {
        unifi_trace(card->ospriv, UDBG1, "Timeout waiting for UniFi to complete disable\n");
        r = -CSR_ETIMEDOUT;
    }

    func_exit();
    return r;
} /* card_wait_for_unifi_to_reset() */


/*
 * ---------------------------------------------------------------------------
 *  card_wait_for_firmware_to_start
 *
 *      Polls the MAILBOX1 register for a non-zero value.
 *      Then reads MAILBOX0 and forms the two values into a 32-bit address
 *      which is returned to the caller.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      paddr           Pointer to receive the UniFi address formed
 *                      by concatenating MAILBOX1 and MAILBOX0.
 *
 *  Returns:
 *      0 on success, CSR error code on failure.
 * ---------------------------------------------------------------------------
 */
CsrInt32
card_wait_for_firmware_to_start(card_t *card, CsrUint32 *paddr)
{
    CsrInt32 i, r;
    CsrUint16 mbox0, mbox1;

    func_enter();

    /*
     * Wait for UniFi to initialise its data structures by polling
     * the SHARED_MAILBOX1 register.
     * Experience shows this is typically 120ms.
     */
    CsrThreadSleep(MAILBOX1_TIMEOUT);

    mbox1 = 0;
    unifi_trace(card->ospriv, UDBG1, "waiting for MAILBOX1 to be non-zero...\n");
    for (i = 0; i < MAILBOX1_ATTEMPTS; i++)
    {
        r = unifi_read_direct16(card, ChipHelper_MAILBOX1(card->helper) * 2, &mbox1);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            /* These reads can fail if UniFi isn't up yet, so try again */
            unifi_warning(card->ospriv, "Failed to read UniFi Mailbox1 register\n");
        }

        if ((r == 0) && (mbox1 != 0)) {
            unifi_trace(card->ospriv, UDBG1, "MAILBOX1 ready (0x%04X) in %u millisecs\n",
                  mbox1, i * MAILBOX1_TIMEOUT);

            /* Read the MAILBOX1 again in case we caught the value as it
             * changed. */
            r = unifi_read_direct16(card, ChipHelper_MAILBOX1(card->helper) * 2, &mbox1);
            if (r == -CSR_ENODEV) return r;
            if (r) {
                unifi_error(card->ospriv, "Failed to read UniFi Mailbox1 register for second time\n");
                func_exit_r(r);
                return r;
            }
            unifi_trace(card->ospriv, UDBG1, "MAILBOX1 value=0x%04X\n", mbox1);

            break;
        }

        CsrThreadSleep(MAILBOX1_TIMEOUT);
        if ((i % 100) == 99) {
            unifi_trace(card->ospriv, UDBG2, "MAILBOX1 not ready (0x%X), still trying...\n", mbox1);
        }
    }

    if ((r == 0) && (mbox1 == 0)) {
        unifi_trace(card->ospriv, UDBG1, "Timeout waiting for firmware to start, Mailbox1 still 0 after %d ms\n",
              MAILBOX1_ATTEMPTS * MAILBOX1_TIMEOUT);
        func_exit_r(-CSR_EIO);
        return -CSR_EIO;
    }


    /*
     * Complete the reset handshake by setting MAILBOX2 to 0xFFFF
     */
    r = unifi_write_direct16(card, ChipHelper_SDIO_HIP_HANDSHAKE(card->helper) * 2, 0xFFFF);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to write f/w startup handshake to MAILBOX2\n");
        func_exit_r(r);
        return r;
    }


    /*
     * Read the Symbol Look Up Table (SLUT) offset.
     * Top 16 bits are in mbox1, read the lower 16 bits from mbox0.
     */
    mbox0 = 0;
    r = unifi_read_direct16(card, ChipHelper_MAILBOX0(card->helper) * 2, &mbox0);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to read UniFi Mailbox0 register\n");
        func_exit_r(r);
        return r;
    }

    *paddr = (((CsrUint32)mbox1 << 16) | mbox0);

    func_exit();
    return 0;
} /* card_wait_for_firmware_to_start() */


/*
 * ---------------------------------------------------------------------------
 *  card_allocate_memory_resources
 *
 *      Allocates memory for the from-host, to-host bulk data slots,
 *      soft queue buffers and bulk data buffers.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      0 on success, CSR error code on failure.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_allocate_memory_resources(card_t *card)
{
    CsrInt16 n, i, k, r;
    sdio_config_data_t *cfg_data;

    func_enter();

    /* Reset any state carried forward from a previous life */
    card->fh_command_queue.q_rd_ptr = 0;
    card->fh_command_queue.q_wr_ptr = 0;
    CsrSprintf(card->fh_command_queue.name, "fh_cmd_q");
    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        card->fh_traffic_queue[i].q_rd_ptr = 0;
        card->fh_traffic_queue[i].q_wr_ptr = 0;
        CsrSprintf(card->fh_traffic_queue[i].name, "fh_data_q%d", i);
    }
    unifi_ta_sampling_init(card);

    /* Convenience short-cut */
    cfg_data = &card->config_data;

    /*
     * Allocate memory for the from-host and to-host signal buffers.
     */
    card->fh_buffer.buf = CsrMemAlloc(UNIFI_FH_BUF_SIZE);
    if (card->fh_buffer.buf == NULL) {
        unifi_error(card->ospriv, "Failed to allocate memory for F-H signals\n");
        func_exit_r(-CSR_ENOMEM);
        return -CSR_ENOMEM;
    }
    card->fh_buffer.bufsize = UNIFI_FH_BUF_SIZE;
    card->fh_buffer.ptr = card->fh_buffer.buf;
    card->fh_buffer.count = 0;

    card->th_buffer.buf = CsrMemAlloc(UNIFI_FH_BUF_SIZE);
    if (card->th_buffer.buf == NULL) {
        unifi_error(card->ospriv, "Failed to allocate memory for T-H signals\n");
        func_exit_r(-CSR_ENOMEM);
        return -CSR_ENOMEM;
    }
    card->th_buffer.bufsize = UNIFI_FH_BUF_SIZE;
    card->th_buffer.ptr = card->th_buffer.buf;
    card->th_buffer.count = 0;



    /*
     * Allocate memory for the from-host and to-host bulk data slots.
     * This is done as separate CsrPmallocs because lots of smaller
     * allocations are more likely to succeed than one huge one.
     */

    /* Allocate memory for the array of pointers */
    n = cfg_data->num_fromhost_data_slots;

    unifi_trace(card->ospriv, UDBG3, "Alloc from-host resources, %d slots.\n", n);
    card->from_host_data =
            (slot_desc_t*)CsrMemAlloc(n * sizeof(slot_desc_t));
    if (card->from_host_data == NULL) {
        unifi_error(card->ospriv, "Failed to allocate memory for F-H bulk data array\n");
        func_exit_r(-CSR_ENOMEM);
        return -CSR_ENOMEM;
    }

    /* Initialise from-host bulk data slots */
    for (i = 0; i < n; i++) {
        unifi_init_bulk_data(&card->from_host_data[i].bd);
    }


    /* Allocate memory for the array of pointers */
    n = cfg_data->num_tohost_data_slots;

    unifi_trace(card->ospriv, UDBG3, "Alloc to-host resources, %d slots.\n", n);
    card->to_host_data =
            (bulk_data_desc_t*)CsrMemAlloc(n * sizeof(bulk_data_desc_t));
    if (card->to_host_data == NULL) {
        unifi_error(card->ospriv, "Failed to allocate memory for T-H bulk data array\n");
        func_exit_r(-CSR_ENOMEM);
        return -CSR_ENOMEM;
    }

    /* Initialise to-host bulk data slots */
    for (i = 0; i < n; i++) {
        unifi_init_bulk_data(&card->to_host_data[i]);
    }

    /*
     * Initialise buffers for soft Q
     */
    for (i = 0; i < UNIFI_SOFT_COMMAND_Q_LENGTH; i++) {
        for (r = 0; r < UNIFI_MAX_DATA_REFERENCES; r++) {
            unifi_init_bulk_data(&card->fh_command_q_body[i].bulkdata[r]);
        }
    }

    for (k = 0; k < UNIFI_WME_NO_OF_QS; k++) {
        for (i = 0; i < UNIFI_SOFT_TRAFFIC_Q_LENGTH; i++) {
            for (r = 0; r < UNIFI_MAX_DATA_REFERENCES; r++) {
                unifi_init_bulk_data(&card->fh_traffic_q_body[k][i].bulkdata[r]);
            }
        }
    }

    card->memory_resources_allocated = 1;

    func_exit();
    return 0;
} /* card_allocate_memory_resources() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_free_bulk_data
 *
 *      Free the data associated to a bulk data structure.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      bulk_data_slot  Pointer to bulk data structure
 *
 *  Returns:
 *      None.
 *
 * ---------------------------------------------------------------------------
 */
static void
unifi_free_bulk_data(card_t *card, bulk_data_desc_t *bulk_data_slot)
{
    if (bulk_data_slot->data_length != 0) {
        unifi_net_data_free(card->ospriv, bulk_data_slot);
    }
} /* unifi_free_bulk_data() */


/*
 * ---------------------------------------------------------------------------
 *  card_free_memory_resources
 *
 *      Frees memory allocated for the from-host, to-host bulk data slots,
 *      soft queue buffers and bulk data buffers.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
card_free_memory_resources(card_t *card)
{
    func_enter();

    unifi_trace(card->ospriv, UDBG1, "Freeing card memory resources.\n");

    /* Clear our internal queues */
    unifi_cancel_pending_signals(card);


    if (card->to_host_data) {
        CsrMemFree(card->to_host_data);
        card->to_host_data = NULL;
    }

    if (card->from_host_data) {
        CsrMemFree(card->from_host_data);
        card->from_host_data = NULL;
    }


    if (card->fh_buffer.buf) {
        CsrMemFree(card->fh_buffer.buf);
    }
    card->fh_buffer.ptr = card->fh_buffer.buf = NULL;
    card->fh_buffer.bufsize = 0;
    card->fh_buffer.count = 0;

    if (card->th_buffer.buf) {
        CsrMemFree(card->th_buffer.buf);
    }
    card->th_buffer.ptr = card->th_buffer.buf = NULL;
    card->th_buffer.bufsize = 0;
    card->th_buffer.count = 0;


    card->memory_resources_allocated = 0;

    func_exit();
} /* card_free_memory_resources() */


static void
card_init_soft_queues(card_t *card)
{
    CsrInt16 i;

    func_enter();

    unifi_trace(card->ospriv, UDBG1, "Initialising internal signal queues.\n");
    /* Reset any state carried forward from a previous life */
    card->fh_command_queue.q_rd_ptr = 0;
    card->fh_command_queue.q_wr_ptr = 0;
    CsrSprintf(card->fh_command_queue.name, "fh_cmd_q");
    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        card->fh_traffic_queue[i].q_rd_ptr = 0;
        card->fh_traffic_queue[i].q_wr_ptr = 0;
        CsrSprintf(card->fh_traffic_queue[i].name, "fh_data_q%d", i);
    }
    unifi_ta_sampling_init(card);

    func_exit();
}


/*
 * ---------------------------------------------------------------------------
 *  unifi_cancel_pending_signals
 *
 *      Free the signals and associated bulk data, pending in the core.
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
unifi_cancel_pending_signals(card_t *card)
{
    CsrInt16 i, n, r;
    func_enter();

    unifi_trace(card->ospriv, UDBG1, "Canceling pending signals.\n");

    if (card->to_host_data) {
        /*
         * Free any bulk data buffers allocated for the t-h slots
         * This will clear all buffers that did not make it to
         * unifi_receive_event() before cancel was request.
         */
        n = card->config_data.num_tohost_data_slots;
        unifi_trace(card->ospriv, UDBG3, "Freeing to-host resources, %d slots.\n", n);
        for (i = 0; i < n; i++) {
            unifi_free_bulk_data(card, &card->to_host_data[i]);
        }
    }

    /*
     * If any of the from-host bulk data has reached the card->from_host_data
     * but not UniFi, we need to free the buffers here.
     */
    if (card->from_host_data) {
        /* Free any bulk data buffers allocated for the f-h slots */
        n = card->config_data.num_fromhost_data_slots;
        unifi_trace(card->ospriv, UDBG3, "Freeing from-host resources, %d slots.\n", n);
        for (i = 0; i < n; i++) {
            unifi_free_bulk_data(card, &card->from_host_data[i].bd);
        }
        
        for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
            card->dynamic_slot_data.from_host_used_slots[i] = 0;
            card->dynamic_slot_data.from_host_max_slots[i] = 0;
            card->dynamic_slot_data.from_host_reserved_slots[i] = 0;
        }
    }

    /*
     * Free any bulk data buffers allocated in the soft queues.
     * This covers the case where a bulk data pointer has reached the soft queue
     * but not the card->from_host_data.
     */
    unifi_trace(card->ospriv, UDBG3, "Freeing cmd q resources.\n");
    for (i = 0; i < UNIFI_SOFT_COMMAND_Q_LENGTH; i++) {
        for (r = 0; r < UNIFI_MAX_DATA_REFERENCES; r++) {
            unifi_free_bulk_data(card, &card->fh_command_q_body[i].bulkdata[r]);
        }
    }

    unifi_trace(card->ospriv, UDBG3, "Freeing traffic q resources.\n");
    for (n = 0; n < UNIFI_WME_NO_OF_QS; n++) {
       for (i = 0; i < UNIFI_SOFT_TRAFFIC_Q_LENGTH; i++) {
           for (r = 0; r < UNIFI_MAX_DATA_REFERENCES; r++) {
               unifi_free_bulk_data(card, &card->fh_traffic_q_body[n][i].bulkdata[r]);
           }
       }
    }

    card_init_soft_queues(card);

    func_exit();

} /* unifi_cancel_pending_signals() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_free_card
 *
 *      Free the memory allocated for the card structure and buffers.
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
unifi_free_card(card_t *card)
{
    func_enter();

    /* Free any memory allocated. */
    card_free_memory_resources(card);

    /* Free any coredump buffers that may have been created */
    unifi_coredump_free(card);

    CsrMemFree(card);

    func_exit();
} /* unifi_free_card() */



/*
 * ---------------------------------------------------------------------------
 *  card_init_slots
 *
 *      Allocate memory for host-side slot data and signal queues.
 *
 * Arguments:
 *      card            Pointer to card object
 *
 * Returns:
 *      CSR error code.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_init_slots(card_t *card)
{
    CsrInt32 r;
    CsrUint8 i;

    func_enter();

    /* Allocate the buffers we need, only once. */
    if (card->memory_resources_allocated == 1) {
        card_free_memory_resources(card);
    } else {
        /* Initialise our internal command and traffic queues */
        card_init_soft_queues(card);
    }

    r = card_allocate_memory_resources(card);
    if (r) {
        unifi_error(card->ospriv, "Failed to allocate card memory resources.\n");
        card_free_memory_resources(card);
        func_exit_r(r);
        return r;
    }

    if (card->sdio_ctrl_addr == 0) {
        unifi_error(card->ospriv, "Failed to find config struct!\n");
        func_exit_r(-CSR_EINVAL);
        return -CSR_EINVAL;
    }

    /*
     * Set initial counts.
     */

    card->from_host_data_head = 0;

    /* Get initial signal counts from UniFi, in case it has not been reset. */
    {
        CsrUint16 s;
#ifdef CSR_WIFI_TRANSPORT_CSPI
        CsrUint8 rewind_h;
#endif

        /* Get the from-host-signals-written count */
        r = unifi_read16(card, card->sdio_ctrl_addr+0, &s);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to read from-host sig read count\n");
            func_exit_r(r);
            return r;
        }
        card->from_host_signals_w = (CsrInt16)s;

        /* Get the from-host-signals-written count */
        r = unifi_read16(card, card->sdio_ctrl_addr+6, &s);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to read from-host sig read count\n");
            func_exit_r(r);
            return r;
        }
        card->to_host_signals_r = (CsrInt16)s;

#ifdef CSR_WIFI_TRANSPORT_CSPI
        r = unifi_read_8_or_16(card, card->sdio_ctrl_addr+8,
                               &rewind_h);
        if (r == -CSR_ENODEV) {
            return r;
        }
        if (r) {
            /* 
             * If we can't even do CMD52 (register read/write) then
             * stop here.
             */
            unifi_error(card->ospriv, "Failed to read rewind handle\n");
            return r;
        }
        if (rewind_h) {
            unifi_trace(card->ospriv, UDBG4,
                        "Found CSPI rewind handle 0x%x\n", rewind_h);
            card->cspi_cleanup_handle = (CsrUint32)rewind_h;

            r = unifi_write_8_or_16(card, card->sdio_ctrl_addr+8, 0);
            if (r == -CSR_ENODEV) {
                return r;
            }
            if (r) {
                unifi_error(card->ospriv, "Failed to reset rewind handle\n");
                return r;
            }
        } else {
            unifi_warning(card->ospriv, "Could not find rewind handle\n");
        }
#endif

    }

    /* Set Initialised flag. */
    r = unifi_write16(card, card->init_flag_addr, 0x0001);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to write initialised flag\n");
        func_exit_r(r);
        return r;
    }

    /* Dynamic queue reservation */
    CsrMemSet(&card->dynamic_slot_data, 0, sizeof(card_dynamic_slot_t));

    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++)
    {
        card->dynamic_slot_data.from_host_max_slots[i] = card->config_data.num_fromhost_data_slots - 
                                                       RESERVED_COMMAND_SLOTS;
        card->dynamic_slot_data.queue_stable[i] = FALSE;
    }

    card->dynamic_slot_data.packets_interval = PACKETS_INTERVAL;
    
    func_exit();
    return 0;

} /* card_init_slots() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_set_udi_hook
 *
 *      Registers the udi hook that reports the sent signals to the core.
 *
 *  Arguments:
 *      card            Pointer to the card context struct
 *      udi_fn          Pointer to the callback function.
 *
 *  Returns:
 *      -CSR_EINVAL if the card pointer is invalid, 0 on success.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_set_udi_hook(card_t *card, udi_func_t udi_fn)
{
    if (card == NULL) {
        return -CSR_EINVAL;
    }

    if (card->udi_hook == NULL) {
        card->udi_hook = udi_fn;
    }

    return 0;
} /* unifi_set_udi_hook() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_remove_udi_hook
 *
 *      Removes the udi hook that reports the sent signals from the core.
 *
 *  Arguments:
 *      card            Pointer to the card context struct
 *      udi_fn          Pointer to the callback function.
 *
 *  Returns:
 *      -CSR_EINVAL if the card pointer is invalid, 0 on success.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_remove_udi_hook(card_t *card, udi_func_t udi_fn)
{
    if (card == NULL) {
        return -CSR_EINVAL;
    }

    if (card->udi_hook == udi_fn) {
        card->udi_hook = NULL;
    }

    return 0;
} /* unifi_remove_udi_hook() */

void
CardReassignDynamicReservation(card_t *card)
{
    CsrUint8 i;

    func_enter();

    unifi_trace(card->ospriv, UDBG5, "Packets Txed %d %d %d %d\n",
                   card->dynamic_slot_data.packets_txed[0],
                   card->dynamic_slot_data.packets_txed[1],
                   card->dynamic_slot_data.packets_txed[2],
                   card->dynamic_slot_data.packets_txed[3]);

    /* Clear reservation and recalculate max slots */
    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        card->dynamic_slot_data.queue_stable[i] = FALSE;
        card->dynamic_slot_data.from_host_reserved_slots[i] = 0;
        card->dynamic_slot_data.from_host_max_slots[i] = card->config_data.num_fromhost_data_slots - 
                                                       RESERVED_COMMAND_SLOTS;
        card->dynamic_slot_data.packets_txed[i] = 0;

        unifi_trace(card->ospriv, UDBG5, "CardReassignDynamicReservation: queue %d reserved %d Max %d\n", i, 
                        card->dynamic_slot_data.from_host_reserved_slots[i],
                        card->dynamic_slot_data.from_host_max_slots[i]);
    }
  
    card->dynamic_slot_data.total_packets_txed = 0;
    func_exit();
}

void
CardCheckDynamicReservation(card_t *card, unifi_TrafficQueue queue)
{
    CsrUint16 q_len, active_queues = 0, excess_queue_slots, div_extra_slots,
              queue_fair_share, reserved_slots = 0, q, excess_need_queues = 0, unmovable_slots = 0;
    int i;
    q_t *sigq;

    func_enter();
#if 0
    if (queue >= UNIFI_WME_NO_OF_QS) {
        unifi_error(card->ospriv, "CardCheckDynamicReservation: incorrect q %d\n", queue);
        return;
    }
#endif

    /* Calculate the pending queue length */
    sigq = &card->fh_traffic_queue[queue];
    q_len = q_slots_used(sigq);

    if (q_len <= card->dynamic_slot_data.from_host_reserved_slots[queue]) {
        unifi_trace(card->ospriv, UDBG5, "queue %d q_len %d already has that many reserved slots, exiting\n", queue, q_len);
        func_exit();
        return;
    }

    /* Upper limit */
    if (q_len > (card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS)) {
        q_len = card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS;
    }
 
    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        if (i != queue) {
            reserved_slots += card->dynamic_slot_data.from_host_reserved_slots[i];
        }
        if ((i == queue) || (card->dynamic_slot_data.from_host_reserved_slots[i] > 0)) {
            active_queues++;
        }
    }

    unifi_trace(card->ospriv, UDBG5, "CardCheckDynamicReservation: queue %d q_len %d\n", queue, q_len);
    unifi_trace(card->ospriv, UDBG5, "Active queues %d reserved slots on other queues %d\n", 
                    active_queues, reserved_slots);
    
    if (reserved_slots + q_len <= card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS) {
        card->dynamic_slot_data.from_host_reserved_slots[queue] = q_len;
        if (q_len == card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS) {
            /* This is the common case when just 1 stream is going */
            card->dynamic_slot_data.queue_stable[queue] = TRUE;
        }
    } else {
        queue_fair_share = (card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS)/active_queues;
        unifi_trace(card->ospriv, UDBG5, "queue fair share %d\n", queue_fair_share);

        /* Evenly distribute slots among active queues */
        /* Find out the queues that need excess of fair share. Also find slots allocated
         * to queues less than their fair share, these slots cannot be reallocated */

        card->dynamic_slot_data.from_host_reserved_slots[queue] = q_len;

        for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
            if (card->dynamic_slot_data.from_host_reserved_slots[i] > queue_fair_share) {
                excess_need_queues++;
            } else {
                unmovable_slots += card->dynamic_slot_data.from_host_reserved_slots[i];
            }
        }
     
#if 0
        unifi_trace(card->ospriv, UDBG5, "Excess need queues %d\n", excess_need_queues);
        if (excess_need_queues == 0) {
            unifi_trace(card->ospriv, UDBG1, "Excess need queues %d exit\n", excess_need_queues);
            return;
        }
#endif
        /* Now find the slots per excess demand queue */
        excess_queue_slots = (card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS - unmovable_slots)/excess_need_queues;
        div_extra_slots = (card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS - unmovable_slots) 
                              - excess_queue_slots * excess_need_queues;
        for (i = UNIFI_WME_NO_OF_QS - 1; i >= 0; i--) {
            if (card->dynamic_slot_data.from_host_reserved_slots[i] > queue_fair_share) {
                card->dynamic_slot_data.from_host_reserved_slots[i] = excess_queue_slots;
                if (div_extra_slots > 0) {
                    card->dynamic_slot_data.from_host_reserved_slots[i]++;
                    div_extra_slots--;
                }
                /* No more slots will be allocated to this queue during the current interval */
                card->dynamic_slot_data.queue_stable[i] = TRUE;
                unifi_trace(card->ospriv, UDBG5, "queue stable %d\n", i);
            }
        }
    }
 
    /* Redistribute max slots */
    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
        reserved_slots = 0;
        for (q = 0; q < UNIFI_WME_NO_OF_QS; q++) {
            if (i != q) {
                reserved_slots += card->dynamic_slot_data.from_host_reserved_slots[q];
            }
        }

        card->dynamic_slot_data.from_host_max_slots[i] = card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS
                                                           - reserved_slots;
        unifi_trace(card->ospriv, UDBG5, "queue %d reserved %d Max %d\n", i, 
                        card->dynamic_slot_data.from_host_reserved_slots[i],
                        card->dynamic_slot_data.from_host_max_slots[i]);
    }

    func_exit();
}

/*
 * ---------------------------------------------------------------------------
 *  CardGetFromHostDataSlot
 *
 *      Find a free from-host bulk data slot.
 *      A slot is free if it's length member is zero.
 *      This function returns the slot number or error code if a slot
 *      is not available.
 *
 *  Arguments:
 *      card            Pointer to the card context struct
 *      len             Number of bytes of bulk data that the slot will carry.
 *
 *  Returns:
 *      The index of a free slot in the from_host_data array or
 *      -1 if no slot is available.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardGetFromHostDataSlot(card_t *card, CsrUint32 len, unifi_TrafficQueue queue)
{
    CsrInt32 s;
    CsrInt32 i;
    CsrInt16 h;
    CsrInt16 nslots;

    func_enter();

    s = -1;
    ASSERT(len);
    ASSERT(len <= card->config_data.data_slot_size);

#if 0
    /* Check if there are free slots */
    if (card->fh_data_slots_free == 0) {
        unifi_error(card->ospriv, "No from-host data slots free\n");
        return -1;
    }
#endif

    /* Last 2 slots for MLME */
    if (queue == UNIFI_TRAFFIC_Q_MLME) {
        CsrInt32 h = card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS;
        for (i = 0; i < card->config_data.num_fromhost_data_slots; i++)
        {
            if (card->from_host_data[h].bd.data_length == 0) {
                /* Free data slot, claim it */
                s = h;
                break;
            }

            if (++h >= card->config_data.num_fromhost_data_slots) {
                h = 0;
            }
        }
    } else {

        if (card->dynamic_slot_data.from_host_used_slots[queue] 
                < card->dynamic_slot_data.from_host_max_slots[queue]) {

            /* Data commands get a free slot only after a few checks */
            nslots = card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS;

            h = card->from_host_data_head;

            for (i = 0; i < nslots; i++)
            {
                if (card->from_host_data[h].bd.data_length == 0) {
                   /* Free data slot, claim it */
                   s = h;
                   break;
                }

                if (++h >= nslots) {
                    h = 0;
                }
            }
            card->from_host_data_head = h;
        }
     }

        /* If we found a slot then return */
     /*
     * We should be guaranteed a free slot, report an error if we
     * didn't find one.
     */
    if (s < 0) {
        unifi_error(card->ospriv, "Internal error, didn't find a free data slot\n");
        for (i = 0; i < card->config_data.num_fromhost_data_slots; i++) {
            unifi_error(card->ospriv, "fh data slot %d: %d\n", i, card->from_host_data[i].bd.data_length);
        }
        s = -CSR_EINVAL;
    }

    unifi_trace(card->ospriv, UDBG4, "CardGetFromHostDataSlot: slot %d, p:0x%x, %d bytes\n", s, &len, len);

    func_exit_r(s);
    return s;
} /* CardGetFromHostDataSlot() */


/*
 * ---------------------------------------------------------------------------
 *  CardClearFromHostDataSlot
 *
 *      Clear a the given data slot, making it available again.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *      slot            Index of the signal slot to clear.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
CardClearFromHostDataSlot(card_t *card, const CsrInt16 slot)
{
    func_enter();

    if (card->from_host_data[slot].bd.data_length == 0) {
        unifi_warning(card->ospriv,
               "Surprise: request to clear an already free FH data slot: %d\n",
               slot);
        func_exit();
        return;
    }

    /* Free card->from_host_data[slot].bd.os_net_ptr here. */
    /* Mark slot as free by setting length to 0. */
    if (card->from_host_data[slot].bd.data_length) {
        CsrUint8 queue = card->from_host_data[slot].queue;

        unifi_free_bulk_data(card, &card->from_host_data[slot].bd);
        if (queue < UNIFI_WME_NO_OF_QS) {
            if (card->dynamic_slot_data.from_host_used_slots[queue] == 0) {
                unifi_error(card->ospriv, "Goofed up used slots q = %d used slots = %d\n",
                                queue, 
                                card->dynamic_slot_data.from_host_used_slots[queue]);
            } else {
                card->dynamic_slot_data.from_host_used_slots[queue]--;
            }
            card->dynamic_slot_data.packets_txed[queue]++;
            card->dynamic_slot_data.total_packets_txed++;
            if (card->dynamic_slot_data.total_packets_txed >= card->dynamic_slot_data.packets_interval) {
                CardReassignDynamicReservation(card);
            }
         }
    }

    unifi_trace(card->ospriv, UDBG4, "CardClearFromHostDataSlot: slot %d recycled\n", slot);

    func_exit();
} /* CardClearFromHostDataSlot() */



CsrUint16
CardGetDataSlotSize(card_t *card)
{
    return card->config_data.data_slot_size;
} /* CardGetDataSlotSize() */


/*
 * ---------------------------------------------------------------------------
 *  CardGetFreeFromHostDataSlots
 *
 *      Retrieve the number of from-host bulk data slots available.
 *
 *  Arguments:
 *      card            Pointer to the card context struct
 *
 *  Returns:
 *      Number of free from-host bulk data slots.
 * ---------------------------------------------------------------------------
 */
CsrUint16
CardGetFreeFromHostDataSlots(card_t *card,  unifi_TrafficQueue queue)
{
    CsrUint16 nslots;
    CsrUint16 i, n = 0;
    CsrUint16 h = card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS;

    func_enter();

    /* First two slots reserved for MLME */
    if (queue == UNIFI_TRAFFIC_Q_MLME) {
        for (i = 0; i < card->config_data.num_fromhost_data_slots; i++)
        {
            if (card->from_host_data[h].bd.data_length == 0) {
                /* Free slot */
                n++;
            }

            if (++h >= card->config_data.num_fromhost_data_slots) {
                h = 0;
            }
        }
    } else {
        if (card->dynamic_slot_data.from_host_used_slots[queue] 
                < card->dynamic_slot_data.from_host_max_slots[queue]) {

            /* Data commands get a free slot only after a few checks */
            nslots = card->config_data.num_fromhost_data_slots - RESERVED_COMMAND_SLOTS;

            h = card->from_host_data_head;

            for (i = 0; i < nslots; i++)
            {
                if (card->from_host_data[h].bd.data_length == 0) {
                   /* Free data slot */
                   n++;
                }

                if (++h >= nslots) {
                    h = 0;
                }
            }
            card->from_host_data_head = h;
        }

        if (n == 0) {
            /* If we haven't already reached the stable state we can ask for reservation */
            if (card->dynamic_slot_data.queue_stable[queue] == FALSE) {
                CardCheckDynamicReservation(card, queue);
            }
        }
    }

    func_exit();
    return n;
} /* CardGetFreeFromHostDataSlots() */

/*
 * ---------------------------------------------------------------------------
 *  CardAreAllFromHostDataSlotsEmpty
 *
 *      Returns the state of from-host bulk data slots.
 *
 *  Arguments:
 *      card            Pointer to the card context struct
 *
 *  Returns:
 *      1       The from-host bulk data slots are all empty (available).
 *      0       Some or all the from-host bulk data slots are in use.
 * ---------------------------------------------------------------------------
 */
CsrUint16
CardAreAllFromHostDataSlotsEmpty(card_t *card)
{
    CsrUint16 i;

    for (i = 0; i < card->config_data.num_fromhost_data_slots; i++)
        if (card->from_host_data[i].bd.data_length != 0)
            return 0;

    return 1;
} /* CardGetFreeFromHostDataSlots() */



static CsrInt32
unifi_identify_hw(card_t *card)
{
    func_enter();

    card->chip_id = card->sdio_if->sdioId.cardId;
    card->function = card->sdio_if->sdioId.sdioFunction;
    card->sdio_io_block_size = card->sdio_if->blockSize;

    /* If SDIO controller doesn't support byte mode CMD53, pad transfers to block sizes */
    card->sdio_io_block_pad = (card->sdio_if->features & CSR_SDIO_FEATURE_BYTE_MODE) ? FALSE : TRUE;

    /*
     * Setup the chip helper so that we can access the regisers (and
     * also tell what sub-type of HIP we shold use).
     */
    card->helper = ChipHelper_GetVersionSdio((CsrUint8)card->chip_id);

    unifi_info(card->ospriv, "Chip ID 0x%02X  Function %u  Block Size %u  Name %s(%s)\n",
               card->chip_id, card->function, card->sdio_io_block_size,
               ChipHelper_MarketingName(card->helper),
               ChipHelper_FriendlyName(card->helper));

    func_exit();
    return 0;

} /* unifi_identify_hw() */


static CsrInt32
unifi_prepare_hw(card_t *card)
{
    CsrUint16 ver;
    CsrInt32 r;
    CsrUint32 gbl_chip_version;

    func_enter();

    r = unifi_identify_hw(card);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to identify hw\n");
        func_exit_r(r);
        return r;
    }

    unifi_trace(card->ospriv, UDBG1,
                "%s mode SDIO\n", card->sdio_io_block_pad ? "Block" : "Byte");
    /*
     * Chip must be a awake or blocks that are asleep may not get
     * reset.  We can only do this after we have read the chip_id.
     */
    r = unifi_set_host_state(card, UNIFI_HOST_STATE_AWAKE);
    if (r == -CSR_ENODEV) return r;

    /*
     * The WLAN function must be enabled to access MAILBOX2 and DEBUG_RST
     * registers.
     */
    r = CsrSdioFunctionEnable(card->sdio_if);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        /* Can't enable WLAN function. Try resetting the SDIO block. */
        unifi_error(card->ospriv, "Failed to re-enable function %d.\n", card->function);
        func_exit_r(r);
        return r;
    }

    /*
     * Poke some registers to make sure the PLL has started,
     * otherwise memory accesses are likely to fail.
     */
    bootstrap_chip_hw(card);

    gbl_chip_version = ChipHelper_GBL_CHIP_VERSION(card->helper);

    /* Try to read the chip version from register. */
    if (gbl_chip_version != 0) {
        r = unifi_read_direct16(card, gbl_chip_version * 2, &ver);
        if (r == -CSR_ENODEV) {
            return r;
        }
        if (r) {
            unifi_error(card->ospriv, "Failed to read GBL_CHIP_VERSION\n");
            func_exit_r(r);
            return r;
        }
        card->chip_version = ver;
    } else {
        unifi_info(card->ospriv, "Unknown Chip ID, cannot locate GBL_CHIP_VERSION\n");
    }
    unifi_info(card->ospriv, "Chip Version 0x%04X\n", card->chip_version);

    func_exit();
    return 0;
} /* unifi_prepare_hw() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_reset_hardware
 *
 *      Execute the UniFi reset sequence.
 *
 *      Note: This may fail if the chip is going TORPID so retry at
 *      least once.
 *
 *  Arguments:
 *      card - pointer to card context structure
 *
 *  Returns:
 *      0 on success, CSR error otherwise.
 *
 *  Notes:
 *      Some platforms (e.g. Windows Vista) do not allow access to registers
 *      that are necessary for a software soft reset.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_reset_hardware(card_t *card)
{
    CsrInt32 r;
    CsrUint16 new_block_size = UNIFI_IO_BLOCK_SIZE;

    func_enter();

    /* Errors returned by unifi_prepare_hw() are not critical at this point */
    r = unifi_prepare_hw(card);
    if (r == -CSR_ENODEV) return r;

    /* First try SDIO controller reset, which may power cycle the UniFi, assert
     * its reset line, or not be implemented depending on the platform.
     */
    r = CsrSdioHardReset(card->sdio_if);
    if (r == 0) {
        unifi_info(card->ospriv, "CsrSdioHardReset succeeded on reseting UniFi\n");
        r = unifi_prepare_hw(card);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "unifi_prepare_hw failed after hard reset\n");
            func_exit_r(r);
            return r;
        }
    } else if (r == -CSR_ENODEV) {
        return r;
    } else {
        /* Falling back to software hard reset methods */
        unifi_info(card->ospriv, "Falling back to software hard reset\n");
        r = CardHardReset(card);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "software hard reset failed\n");
            func_exit_r(r);
            return r;
        }
    }

#ifdef CSR_WIFI_HIP_SDIO_BLOCK_SIZE
    new_block_size = CSR_WIFI_HIP_SDIO_BLOCK_SIZE;
#endif

    /* After hard reset, we need to restore the SDIO block size */
    r = CsrSdioBlockSizeSet(card->sdio_if, new_block_size);

    /* Warn if a different block size was achieved by the transport */
    if (card->sdio_if->blockSize != new_block_size) {
        unifi_info(card->ospriv,
                   "Actually got block size %d\n", card->sdio_if->blockSize);
    }

    /* sdio_io_block_size always needs be updated from the achieved block size,
     * as it is used by the OS layer to allocate memory in unifi_net_malloc().
     * Controllers which don't support block mode (e.g. CSPI) will report a
     * block size of zero.
     */
    if (card->sdio_if->blockSize == 0) {
        unifi_info(card->ospriv, "Block size 0, block mode not available\n");

        /* Set sdio_io_block_size to 1 so that unifi_net_data_malloc() has a
         * sensible rounding value. Elsewhere padding will already be
         * disabled because the controller supports byte mode.
         */
        card->sdio_io_block_size = 1;

        /* Controller features must declare support for byte mode */
        if (!(card->sdio_if->features & CSR_SDIO_FEATURE_BYTE_MODE)) {
            unifi_error(card->ospriv, "Requires byte mode\n");
            r = -CSR_EINVAL;
        }
    } else {
        /* Padding will be enabled if CSR_SDIO_FEATURE_BYTE_MODE isn't set */
        card->sdio_io_block_size = card->sdio_if->blockSize;
    }

    func_exit_r(r);
    return r;
} /* unifi_reset_hardware() */




/*
 * ---------------------------------------------------------------------------
 *  card_reset_method_io_enable
 *
 *      Issue a hard reset to the hw writing the IO_ENABLE.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 *      -CSR_ETIMEDOUT      if a response was not seen in the expected time
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_reset_method_io_enable(card_t *card)
{
    CsrInt32 r;

    func_enter();

    /*
     * This resets only function 1, so should be used in
     * preference to the method below (CSR_FUNC_EN)
     */
    unifi_trace(card->ospriv, UDBG1, "Hard reset (IO_ENABLE)\n");

    r = CsrSdioFunctionDisable(card->sdio_if);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_warning(card->ospriv, "SDIO error writing IO_ENABLE: %d\n", r);
    } else {
        /* Delay here to let the reset take affect. */
        CsrThreadSleep(RESET_SETTLE_DELAY);

        r = card_wait_for_unifi_to_disable(card);
        if (r == -CSR_ENODEV) return r;

        if (r == 0) {
            r = card_wait_for_unifi_to_reset(card);
            if (r == -CSR_ENODEV) return r;
        }
    }

    if (r) {
        unifi_trace(card->ospriv, UDBG1, "Hard reset (CSR_FUNC_EN)\n");

        r = sdio_write_f0(card, SDIO_CSR_FUNC_EN, 0);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_warning(card->ospriv, "SDIO error writing SDIO_CSR_FUNC_EN: %d\n", r);
            func_exit_r(r);
            return r;
        } else {
            /* Delay here to let the reset take affect. */
            CsrThreadSleep(RESET_SETTLE_DELAY);

            r = card_wait_for_unifi_to_reset(card);
            if (r == -CSR_ENODEV) return r;
        }
    }

    if (r) {
        unifi_warning(card->ospriv, "card_reset_method_io_enable failed to reset UniFi\n");
    }

    func_exit();
    return r;
} /* card_reset_method_io_enable() */


/*
 * ---------------------------------------------------------------------------
 *  card_reset_method_dbg_reset
 *
 *      Issue a hard reset to the hw writing the DBG_RESET.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 *      -CSR_ETIMEDOUT      if a response was not seen in the expected time
 * ---------------------------------------------------------------------------
 */
static CsrInt32
card_reset_method_dbg_reset(card_t *card)
{
    CsrInt32 r;

    func_enter();

    /*
     * Prepare UniFi for h/w reset
     */
    if (card->host_state == UNIFI_HOST_STATE_TORPID) {
        r = unifi_set_host_state(card, UNIFI_HOST_STATE_DROWSY);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to set UNIFI_HOST_STATE_DROWSY\n");
            func_exit_r(r);
            return r;
        }
        CsrThreadSleep(5);
    }

    r = card_stop_processor(card, UNIFI_PROC_BOTH);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Can't stop processors\n");
        func_exit();
        return r;
    }

    unifi_trace(card->ospriv, UDBG1, "Hard reset (DBG_RESET)\n");

    /*
     * This register write may fail. The debug reset resets
     * parts of the Function 0 sections of the chip, and
     * therefore the response cannot be sent back to the host.
     */
    r = unifi_write_direct_8_or_16(card, ChipHelper_DBG_RESET(card->helper) * 2, 1);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_warning(card->ospriv, "SDIO error writing DBG_RESET: %d\n", r);
        func_exit_r(r);
        return r;
    }

    /* Delay here to let the reset take affect. */
    CsrThreadSleep(RESET_SETTLE_DELAY);

    r = card_wait_for_unifi_to_reset(card);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_warning(card->ospriv, "card_reset_method_dbg_reset failed to reset UniFi\n");
    }

    func_exit();
    return r;
} /* card_reset_method_dbg_reset() */


/*
 * ---------------------------------------------------------------------------
 *  CardHardReset
 *
 *      Issue reset to the hw, by writing to registers on the card.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 *      -CSR_ETIMEDOUT      if a response was not seen in the expected time
 * ---------------------------------------------------------------------------
 */
static CsrInt32
CardHardReset(card_t *card)
{
    CsrInt32 r;
    CsrUint32 ram_offset;
    const struct chip_helper_reset_values *init_data;
    CsrUint16 i, j, chunks;

    func_enter();

    /* Clear cache of page registers */
    card->proc_select = (CsrUint32)(-1);
    card->dmem_page = (CsrUint32)(-1);
    card->pmem_page = (CsrUint32)(-1);

    /*
     * We need to have a valid card->helper before we use software hard reset.
     * If unifi_identify_hw() fails to get the card ID, it probably means
     * that there is no way to talk to the h/w.
     */
    r = unifi_identify_hw(card);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "CardHardReset failed to identify h/w\n");
        func_exit();
        return r;
    }

    /* Search for some reset code. */
    chunks = ChipHelper_HostResetSequence(card->helper, &init_data);
    if (chunks != 0) {

        unifi_trace(card->ospriv, UDBG1, "Hard reset (Code download)\n");

        if (card->host_state == UNIFI_HOST_STATE_TORPID) {
            /* Write the new state to UniFi. */
            r = sdio_write_f0(card, SDIO_IO_ABORT, UNIFI_HOST_STATE_DROWSY);
            if (r == -CSR_ENODEV) return r;
            if (r) {
                unifi_error(card->ospriv, "Failed to write to UniFi IOABORT register\n");
            }

            CsrThreadSleep(5);
        }

        r = card_stop_processor(card, UNIFI_PROC_BOTH);
        if (r) {
            unifi_error(card->ospriv, "Failed to stop processors\n");
            func_exit();
            return r;
        }

        /* Select MAC */
        r = unifi_set_proc_select(card, UNIFI_PROC_MAC);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to write to Proc Select register\n");
            func_exit();
            return r;
        }

        /* Write the chunks */
        for (j = 0; j < chunks; j++, init_data++) {
            for (i = 0; i < init_data->len; i++) {
                r = unifi_write16(card, init_data->gp_address + (i * 2), init_data->data[i]);
                if (r == -CSR_ENODEV) return r;
                if (r) {
                    unifi_error(card->ospriv, "Failed to write to Reset Program\n");
                    func_exit();
                    return r;
                }
            }
        }

        ram_offset = ChipHelper_PROGRAM_MEMORY_RAM_OFFSET(card->helper);

        /* Write PCL and PCH */
        r = unifi_write_direct16(card, ChipHelper_XAP_PCL(card->helper) * 2,
                                 (CsrUint16)(ram_offset & 0xffff));
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to set PCL\n");
            func_exit();
            return r;
        }
        r = unifi_write_direct16(card, ChipHelper_XAP_PCH(card->helper) * 2,
                                 (CsrUint16)(ram_offset >> 16));
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to set PCH\n");
            func_exit();
            return r;
        }

        r = card_start_processor(card, UNIFI_PROC_MAC);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to start MAC processor\n");
            func_exit();
            return r;
        }

        /* Delay here to let the reset take affect. */
        CsrThreadSleep(RESET_SETTLE_DELAY);

        return card_wait_for_unifi_to_reset(card);
    }

    if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {
        /* The HIP spec considers this a bus-specific reset.
         * This resets only function 1, so should be used in
         * preference to the method below (CSR_FUNC_EN)
         * If this method fails, it means that the f/w is probably
         * not running. In this case, try the DBG_RESET method.
         */
        r = card_reset_method_io_enable(card);
        if (r == -CSR_ENODEV) return r;
        if (r == 0) {
            func_exit();
            return r;
        }
    }

    /* Software hard reset */
    r = card_reset_method_dbg_reset(card);

    func_exit();
    return r;
} /* CardHardReset() */


/*
 * ---------------------------------------------------------------------------
 *
 *  CardGenInt
 *
 *      Prod the card.
 *      This function causes an internal interrupt to be raised in the
 *      UniFi chip. It is used to signal the firmware that some action has
 *      been completed.
 *      The UniFi Host Interface asks that the value used increments for
 *      debugging purposes.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 *      -CSR_ETIMEDOUT      if a response was not seen in the expected time
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardGenInt(card_t *card)
{
    CsrInt32 r;

    func_enter();

    if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {
        r = sdio_write_f0(card, SDIO_CSR_FROM_HOST_SCRATCH0,
                          (CsrUint8)card->unifi_interrupt_seq);
    } else {
        r = unifi_write_direct_8_or_16(card,
                                       ChipHelper_SHARED_IO_INTERRUPT(card->helper) * 2,
                                       (CsrUint8)card->unifi_interrupt_seq);
    }
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error writing UNIFI_SHARED_IO_INTERRUPT: %d\n", r);
        func_exit_r(r);
        return r;
    }

    card->unifi_interrupt_seq++;

    func_exit();
    return 0;
} /* CardGenInt() */


/*
 * ---------------------------------------------------------------------------
 *  CardEnableInt
 *
 *      Enable the outgoing SDIO interrupt from UniFi to the host.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardEnableInt(card_t *card)
{
    CsrInt32 r;
    CsrUint8 int_enable;

    r = sdio_read_f0(card, SDIO_INT_ENABLE, &int_enable);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error reading SDIO_INT_ENABLE\n");
        return r;
    }

    int_enable |= (1 << card->function) | UNIFI_SD_INT_ENABLE_IENM;

    r = sdio_write_f0(card, SDIO_INT_ENABLE, int_enable);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error writing SDIO_INT_ENABLE\n");
        return r;
    }

    return 0;
} /* CardEnableInt() */


/*
 * ---------------------------------------------------------------------------
 *  CardDisableInt
 *
 *      Disable the outgoing SDIO interrupt from UniFi to the host.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardDisableInt(card_t *card)
{
    CsrInt32 r;
    CsrUint8 int_enable;

    r = sdio_read_f0(card, SDIO_INT_ENABLE, &int_enable);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error reading SDIO_INT_ENABLE\n");
        return r;
    }

    int_enable &= ~(1 << card->function);

    r = sdio_write_f0(card, SDIO_INT_ENABLE, int_enable);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error writing SDIO_INT_ENABLE\n");
        return r;
    }

    return 0;
} /* CardDisableInt() */


/*
 * ---------------------------------------------------------------------------
 *  CardPendingInt
 *
 *      Determine whether UniFi is currently asserting the SDIO interrupt
 *      request.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      1  - There is a pending interrupt
 *      0  - There is not a pending interrupt
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardPendingInt(card_t *card)
{
    CsrInt32 r;
    CsrUint8 pending;

    r = sdio_read_f0(card, SDIO_INT_PENDING, &pending);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error reading SDIO_INT_PENDING\n");
        return r;
    }

    return (pending & (1 << card->function)) ? 1 : 0;
} /* CardPendingInt() */


/*
 * ---------------------------------------------------------------------------
 *  CardClearInt
 *
 *      Clear the UniFi SDIO interrupt request.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      0  if pending interrupt was cleared, or no pending interrupt.
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardClearInt(card_t *card)
{
    CsrInt32 r;
    if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {

        /* CardPendingInt() returns 1, if there is a pending interrupt */
        r = CardPendingInt(card);
        if (r != 1) {
            return r;
        }

        r = sdio_write_f0(card, SDIO_CSR_HOST_INT_CLEAR, 1);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "SDIO error writing SDIO_CSR_HOST_INT_CLEAR\n");
        }
    } else {
        r = unifi_write_direct_8_or_16(card,
                                       ChipHelper_SDIO_HOST_INT(card->helper) * 2,
                                       0);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "SDIO error writing UNIFI_SDIO_HOST_INT\n");
        }
    }

    return r;
} /* CardClearInt() */


/*
 * ---------------------------------------------------------------------------
 *  CardIntEnabled
 *
 *      Determine whether UniFi is currently asserting the SDIO interrupt
 *      request.
 *
 *  Arguments:
 *      card            Pointer to Card object
 *
 *  Returns:
 *      1   interrupts are enabled
 *      0   interrupts are disabled
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardIntEnabled(card_t *card)
{
    CsrInt32 r;
    CsrUint8 int_enable;

    r = sdio_read_f0(card, SDIO_INT_ENABLE, &int_enable);
    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "SDIO error reading SDIO_INT_ENABLE\n");
        return r;
    }

    return (int_enable & (1 << card->function)) ? 1 : 0;
} /* CardIntEnabled() */




/*
 * ---------------------------------------------------------------------------
 *  CardWriteBulkData
 *
 *      Claim a bulk data slot and copy data to it.
 *
 *  Arguments:
 *      card            Pointer to the card context struct
 *      bulkdata        Pointer to bulk data structure that
 *                      describes the data to copy to slot.
 *
 *  Returns:
 *      slot number or -1 if no slot was available.
 * ---------------------------------------------------------------------------
 */
CsrInt32
CardWriteBulkData(card_t *card, bulk_data_desc_t* bulkdata, unifi_TrafficQueue queue)
{
    CsrInt32 slot;

    func_enter();

    slot = CardGetFromHostDataSlot(card, bulkdata->data_length, queue);
    if (slot < 0) {
        func_exit_r(slot);
        return slot;
    }

    /* Do not copy the data, just store the information to them */
    card->from_host_data[slot].bd.os_data_ptr = bulkdata->os_data_ptr;
    card->from_host_data[slot].bd.os_net_buf_ptr = bulkdata->os_net_buf_ptr;
    card->from_host_data[slot].bd.data_length = bulkdata->data_length;
    card->from_host_data[slot].bd.net_buf_length = bulkdata->net_buf_length;
    card->from_host_data[slot].queue = queue;

    if (queue >= 0 && queue < UNIFI_WME_NO_OF_QS) {
        if (card->dynamic_slot_data.from_host_used_slots[queue] 
                > card->dynamic_slot_data.from_host_max_slots[queue]) {
            unifi_error(card->ospriv, "Goofed up used slots q= %d used slots= %d max= %d\n",
                            queue, card->dynamic_slot_data.from_host_used_slots[queue],
                            card->dynamic_slot_data.from_host_max_slots[queue]);
        }
        card->dynamic_slot_data.from_host_used_slots[queue]++;
    }

    /* Pass the free responsibility to the lower layer. */
    unifi_init_bulk_data(bulkdata);

    func_exit();

    return slot;
} /*  CardWriteBulkData() */


/*
 * ---------------------------------------------------------------------------
 *  card_find_data_slot
 *
 *      Dereference references to bulk data slots into pointers to real data.
 *
 *  Arguments:
 *      card            Pointer to the card struct.
 *      slot            Slot number from a signal structure
 *
 *  Returns:
 *      Pointer to entry in bulk_data_slot array.
 * ---------------------------------------------------------------------------
 */
bulk_data_desc_t *
card_find_data_slot(card_t *card, CsrInt16 slot)
{
    CsrInt16 sn;
    bulk_data_desc_t *bd;

    sn = slot & 0x7FFF;

    /* ?? check sanity of slot number ?? */

    if (slot & SLOT_DIR_TO_HOST) {
        bd = &card->to_host_data[sn];
    } else {
        bd = &card->from_host_data[sn].bd;
    }

    return bd;
} /* card_find_data_slot() */



/*
 * ---------------------------------------------------------------------------
 *  firmware_present_in_flash
 *
 *      Probe for external Flash that looks like it might contain firmware.
 *
 *      If Flash is not present, reads always return 0x0008.
 *      If Flash is present, but empty, reads return 0xFFFF.
 *      Anything else is considered to be firmware.
 *
 *  Arguments:
 *      card        Pointer to card struct
 *
 *  Returns:
 *      1               firmware is present
 *      0               firmware is not present
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred,
 * ---------------------------------------------------------------------------
 */
static CsrInt32
firmware_present_in_flash(card_t *card)
{
    CsrInt32 r;
    CsrInt32 present = 1;
    CsrUint16 m1, m5;

    if (ChipHelper_HasRom(card->helper))
        return 1;
    if (!ChipHelper_HasFlash(card->helper))
        return 0;

    /*
     * Examine the Flash locations that are the power-on default reset
     * vectors of the XAP processors.
     * These are words 1 and 5 in Flash.
     */
    r = unifi_read16(card, UNIFI_MAKE_GP(EXT_FLASH, 2), &m1);
    if (r) return r;

    r = unifi_read16(card, UNIFI_MAKE_GP(EXT_FLASH, 10), &m5);
    if (r) return r;

    if ((m1 == 0x0008) || (m1 == 0xFFFF) ||
        (m1 == 0x0004) || (m5 == 0x0004) ||
        (m5 == 0x0008) || (m5 == 0xFFFF))
    {
        present = 0;
    }

    return present;
} /* firmware_present_in_flash() */


/*
 * ---------------------------------------------------------------------------
 *  bootstrap_chip_hw
 *
 *      Perform chip specific magic to "Get It Working" TM.  This will
 *      increase speed of PLLs in analogue and maybe enable some
 *      on-chip regulators.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
static void
bootstrap_chip_hw(card_t *card)
{
    const struct chip_helper_init_values *vals;
    CsrUint16 i, len;
    void *sdio = card->sdio_if;
    CsrInt32 r;

    len = ChipHelper_ClockStartupSequence(card->helper, &vals);
    if (len != 0) {
        for (i=0; i<len; i++) {
            r = CsrSdioWrite16(sdio, vals[i].addr * 2, vals[i].value);
            if (r) {
                unifi_warning(card->ospriv, "Failed to write bootstrap value %d\n", i);
                /* Might not be fatal */
            }

            CsrThreadSleep(1);
        }
    }

} /* bootstrap_chip_hw() */


/*
 * ---------------------------------------------------------------------------
 *  card_stop_processor
 *
 *      Stop the UniFi XAP processors.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      which           One of UNIFI_PROC_MAC, UNIFI_PROC_PHY, UNIFI_PROC_BOTH
 *
 *  Returns:
 *      CSR error code.
 * ---------------------------------------------------------------------------
 */
CsrInt32
card_stop_processor(card_t *card, CsrInt16 which)
{
    CsrInt32 r = 0;
    CsrUint8 status;
    CsrInt16 retry = 100;

    while (retry--) {
        /* Select both XAPs */
        r = unifi_set_proc_select(card, which);
        if (r) break;

        /* Stop processors */
        r = unifi_write_direct16(card, ChipHelper_DBG_EMU_CMD(card->helper) * 2, 2);
        if (r) break;

        /* Read status */
        r = unifi_read_direct_8_or_16(card,
                                      ChipHelper_DBG_HOST_STOP_STATUS(card->helper) * 2,
                                      &status);
        if (r) break;

        if ((status & 1) == 1) {
            /* Success! */
            return 0;
        }

        /* Processors didn't stop, try again */
    }

    if (r) {
        /* An SDIO error occurred */
        unifi_error(card->ospriv, "Failed to stop processors: SDIO error\n");
    } else {
        /* If we reach here, we didn't the status in time. */
        unifi_error(card->ospriv, "Failed to stop processors: timeout waiting for stopped status\n");
        r = -CSR_ETIMEDOUT;
    }

    return r;
} /* card_stop_processor() */



/*
 * ---------------------------------------------------------------------------
 *  card_start_processor
 *
 *      Start the UniFi XAP processors.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      which           One of UNIFI_PROC_MAC, UNIFI_PROC_PHY, UNIFI_PROC_BOTH
 *
 *  Returns:
 *      CSR error code
 * ---------------------------------------------------------------------------
 */
CsrInt32
card_start_processor(card_t *card, CsrInt16 which)
{
    CsrInt32 r;

    /* Select both XAPs */
    r = unifi_set_proc_select(card, which);
    if (r) {
        unifi_error(card->ospriv, "unifi_set_proc_select failed: %d.\n", r);
        return r;
    }


    r = unifi_write_direct_8_or_16(card,
                                   ChipHelper_DBG_EMU_CMD(card->helper) * 2, 8);
    if (r) return r;

    r = unifi_write_direct_8_or_16(card,
                                   ChipHelper_DBG_EMU_CMD(card->helper) * 2, 0);
    if (r) return r;

    return 0;

} /* card_start_processor() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_set_interrupt_mode
 *
 *      Configure the interrupt processing mode used by the HIP
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      mode            Interrupt mode to apply
 *
 *  Returns:
 *      None
 * ---------------------------------------------------------------------------
 */
void
unifi_set_interrupt_mode(card_t *card, CsrUint32 mode)
{
    if (mode == CSR_WIFI_INTMODE_RUN_BH_ONCE) {
        unifi_info(card->ospriv, "Scheduled interrupt mode");
    }
    card->intmode = mode;

} /* unifi_set_interrupt_mode() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_start_processors
 *
 *      Start all UniFi XAP processors.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      0 on success, CSR error code on error
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_start_processors(card_t *card)
{
    return card_start_processor(card, UNIFI_PROC_BOTH);

} /* unifi_start_processors() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_request_max_sdio_clock
 *
 *      Requests that the maximum SDIO clock rate is set at the next suitable
 *      opportunity (e.g. when the BH next runs, so as not to interfere with
 *      any current operation).
 *      
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      None
 * ---------------------------------------------------------------------------
 */
void
unifi_request_max_sdio_clock(card_t *card)
{
    card->request_max_clock = 1;
        
} /* unifi_request_max_sdio_clock() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_set_host_state
 *
 *      Set the host deep-sleep state.
 *
 *      If transitioning to TORPID, the SDIO driver will be notified
 *      that the SD bus will be unused (idle) and conversely, when
 *      transitioning from TORPID that the bus will be used (active).
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      state           New deep-sleep state.
 *
 *  Returns:
 *      0               on success
 *      -CSR_ENODEV         if the card was ejected
 *      -CSR_EIO            if an SDIO error occurred
 *
 *  Notes:
 *      We need to reduce the SDIO clock speed before trying to wake up the
 *      chip. Actually, in the implementation below we reduce the clock speed
 *      not just before we try to wake up the chip, but when we put the chip to
 *      deep sleep. This means that if the f/w wakes up on its' own, we waste
 *      a reduce/increace cycle. However, trying to eliminate this overhead is
 *      proved difficult, as the current state machine in the HIP lib does at
 *      least a CMD52 to disable the interrupts before we configure the host
 *      state.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_set_host_state(card_t *card, enum unifi_host_state state)
{
    CsrInt32 r = 0;
    static const CsrInt8 *const states[] = {
        "AWAKE", "DROWSY", "TORPID"
    };
    static const CsrUint8 state_csr_host_wakeup[] = {
        1, 3, 0
    };
    static const CsrUint8 state_io_abort[] = {
        0, 2, 3
    };

    unifi_trace(card->ospriv, UDBG4, "State %s to %s\n",
                states[card->host_state], states[state]);

    if (card->host_state == UNIFI_HOST_STATE_TORPID) {
        CsrSdioFunctionActive(card->sdio_if);
    }

    /* Write the new state to UniFi. */
    if (card->chip_id > SDIO_CARD_ID_UNIFI_2) {
        r = sdio_write_f0(card, SDIO_CSR_HOST_WAKEUP,
                          (card->function << 4) | state_csr_host_wakeup[state]);
    } else {
        r = sdio_write_f0(card, SDIO_IO_ABORT, state_io_abort[state]);
    }

    if (r == -CSR_ENODEV) return r;
    if (r) {
        unifi_error(card->ospriv, "Failed to write UniFi deep sleep state\n");
    } else {

        /*
         * If the chip was in state TORPID then we can now increase
         * the maximum bus clock speed.
         */
        if (card->host_state == UNIFI_HOST_STATE_TORPID) {
            r = csr_sdio_set_max_clock_speed(card->sdio_if,
                                             UNIFI_SDIO_CLOCK_MAX);
            /* Non-fatal error */
            if (r && (r != -CSR_ENODEV)) {
                unifi_warning(card->ospriv,
                              "Failed to increase the SDIO clock speed\n");
            } else {
                card->sdio_clock_speed = UNIFI_SDIO_CLOCK_MAX;
            }
        }

        /*
         * Cache the current state in the card structure to avoid
         * unnecessary SDIO reads.
         */
        card->host_state = state;

        if (state == UNIFI_HOST_STATE_TORPID) {
            /*
             * If the chip is now in state TORPID then we must now decrease
             * the maximum bus clock speed.
             */
            r = csr_sdio_set_max_clock_speed(card->sdio_if,
                                             UNIFI_SDIO_CLOCK_SAFE);

            /* Non-fatal error */
            if (r && (r != -CSR_ENODEV)) {
                unifi_warning(card->ospriv,
                              "Failed to decrease the SDIO clock speed\n");
            } else {
                card->sdio_clock_speed = UNIFI_SDIO_CLOCK_SAFE;
            }
            CsrSdioFunctionIdle(card->sdio_if);
        }
    }

    return r;

} /* unifi_set_host_state() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_card_info
 *
 *      Update the card information data structure
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      card_info       Pointer to info structure to update
 *
 *  Returns:
 *      None
 * ---------------------------------------------------------------------------
 */
void
unifi_card_info(card_t *card, card_info_t *card_info)
{
    card_info->chip_id = card->chip_id;
    card_info->chip_version = card->chip_version;
    card_info->fw_build = card->build_id;
    card_info->fw_hip_version = card->config_data.version;
    card_info->sdio_block_size = card->sdio_io_block_size;
} /* unifi_card_info() */


CsrInt32
unifi_check_io_status(card_t *card)
{
    CsrUint8 io_en;
    CsrInt32 r;

    r = sdio_read_f0(card, SDIO_IO_ENABLE, &io_en);
    if (r == -CSR_ENODEV) {
        return r;
    }
    if (r) {
        unifi_error(card->ospriv, "Failed to read SDIO_IO_ENABLE to check for spontaneous reset\n");
        return r;
    }

    if ((io_en & (1 << card->function)) == 0)
    {
        unifi_error(card->ospriv, "UniFi has spontaneously reset.\n");
        return 1;
    }
    else
    {
        unifi_info(card->ospriv, "UniFi function %d is enabled.\n", card->function);
    }

    return 0;
} /* unifi_check_io_status() */

void 
unifi_get_hip_qos_info(card_t *card, unifi_HipQosInfo *hipqosinfo)
{
    CsrInt32 r;
    CsrInt16 t;
    CsrUint32 occupied_fh, count_fhr;
    q_t *sigq;
    CsrUint16 nslots, i;

    CsrMemSet(hipqosinfo, 0, sizeof(unifi_HipQosInfo));

    nslots = card->config_data.num_fromhost_data_slots;

    for (i = 0; i < nslots; i++) {
        if (card->from_host_data[i].bd.data_length == 0) {
            hipqosinfo->free_fh_bulkdata_slots++;
        }
    }

    for (i = 0; i < UNIFI_WME_NO_OF_QS; i++) {
         sigq = &card->fh_traffic_queue[i];
         t = sigq->q_wr_ptr - sigq->q_rd_ptr;
         if (t < 0) {
             t += sigq->q_length;
         }
         hipqosinfo->free_fh_sig_queue_slots[i] = (sigq->q_length - t) - 1;
    }

    r = unifi_read_shared_count(card, card->sdio_ctrl_addr+2);
    if (r < 0) {
        unifi_error(card->ospriv, "Failed to read from-host sig read count - %d\n", r);
        hipqosinfo->free_fh_fw_slots = 0xfa;
        return;
    }

    count_fhr = r;
    occupied_fh = (card->from_host_signals_w - count_fhr) % 128;

    hipqosinfo->free_fh_fw_slots = (CsrUint16)(card->config_data.num_fromhost_sig_frags - occupied_fh);
}



