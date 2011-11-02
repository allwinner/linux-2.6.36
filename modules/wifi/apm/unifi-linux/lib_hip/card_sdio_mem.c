/*
 * ---------------------------------------------------------------------------
 * FILE: card_sdio_mem.c
 *
 * PURPOSE: Implementation of the Card API for SDIO. 
 *
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "card.h"

#define SDIO_RETRIES    3


#define retryable_sdio_error(_r) (((_r) == -CSR_EIO) || ((_r) == -CSR_ETIMEDOUT))


/*
 * ---------------------------------------------------------------------------
 *  retrying_read8
 *  retrying_write8
 *
 *      These functions provide the first level of retry for SDIO operations.
 *      If an SDIO command fails for reason of a response timeout or CRC
 *      error, it is retried immediately. If three attempts fail we report a
 *      failure.
 *      If the command failed for any other reason, the failure is reported
 *      immediately.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      funcnum         The SDIO function to access.
 *                      Function 0 is the Card Configuration Register space,
 *                      function 1/2 is the UniFi register space.
 *      addr            Address to access
 *      pdata           Pointer in which to return the value read.
 *      data            Value to write.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
static INLINE CsrInt32
retrying_read8(card_t *card, CsrInt16 funcnum,CsrUint32 addr, CsrUint8 *pdata)
{
    CsrSdioFunction *sdio = card->sdio_if;
    CsrInt32 r = 0;
    CsrInt16 retries;

    retries = 0;
    while (retries++ < SDIO_RETRIES) {
        if (funcnum == 0) {
            r = CsrSdioF0Read8(sdio, addr, pdata);
        } else {
#ifdef CSR_WIFI_TRANSPORT_CSPI
            unifi_error(card->ospriv,
                        "retrying_read_f0_8: F1 8-bit reads are not allowed.\n");
            return -CSR_EIO;
#else
            r = CsrSdioRead8(sdio, addr, pdata);
#endif
        }
        if (r == -CSR_ENODEV) {
            return r;
        }
        /* 
         * Try again for retryable (CRC or TIMEOUT) errors, 
         * break on success or fatal error
         */
        if (!retryable_sdio_error(r)) {
#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
            card->cmd_prof.cmd52_count ++;
#endif
            break;
        }
        unifi_trace(card->ospriv, UDBG2, "retryable SDIO error reading F%d 0x%lX\n", funcnum, addr);
    }

    if ((r == 0) && (retries > 1)) {
        unifi_warning(card->ospriv, "Read succeeded after %d attempts\n", retries);
    }

    if (r) {
        unifi_error(card->ospriv, "Failed to read from UniFi (addr 0x%lX) after %d tries\n",
                    addr, retries - 1);
        /* Report any SDIO error as a general i/o error */
        r = -CSR_EIO;
    }

    return r;
} /* retrying_read8() */


static INLINE CsrInt32
retrying_write8(card_t *card, CsrInt16 funcnum, CsrUint32 addr, CsrUint8 data)
{
    CsrSdioFunction *sdio = card->sdio_if;
    CsrInt32 r = 0;
    CsrInt16 retries;

    retries = 0;
    while (retries++ < SDIO_RETRIES) {
        if (funcnum == 0) {
            r = CsrSdioF0Write8(sdio, addr, data);
        } else {
#ifdef CSR_WIFI_TRANSPORT_CSPI
            unifi_error(card->ospriv,
                        "retrying_write_f0_8: F1 8-bit writes are not allowed.\n");
            return -CSR_EIO;
#else
            r = CsrSdioWrite8(sdio, addr, data);
#endif
        }
        if (r == -CSR_ENODEV) {
            return r;
        }
        /* 
         * Try again for retryable (CRC or TIMEOUT) errors, 
         * break on success or fatal error
         */
        if (!retryable_sdio_error(r)) {
#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
            card->cmd_prof.cmd52_count ++;
#endif
            break;
        }
        unifi_trace(card->ospriv, UDBG2, "retryable SDIO error writing %02X to F%d 0x%lX\n",
                    data, funcnum, addr);
    }

    if ((r == 0) && (retries > 1)) {
        unifi_warning(card->ospriv, "Write succeeded after %d attempts\n", retries);
    }

    if (r) {
        unifi_error(card->ospriv, "Failed to write to UniFi (addr 0x%lX) after %d tries\n",
                    addr, retries - 1);
        /* Report any SDIO error as a general i/o error */
        r = -CSR_EIO;
    }

    return r;
} /* retrying_write8() */


static INLINE CsrInt32
retrying_read16(card_t *card, CsrInt16 funcnum,
                CsrUint32 addr, CsrUint16 *pdata)
{
    CsrSdioFunction *sdio = card->sdio_if;
    CsrInt32 r = 0;
    CsrInt16 retries;

    retries = 0;
    while (retries++ < SDIO_RETRIES) {
        r = CsrSdioRead16(sdio, addr, pdata);
        if (r == -CSR_ENODEV) {
            return r;
        }

        /* 
         * Try again for retryable (CRC or TIMEOUT) errors, 
         * break on success or fatal error
         */
        if (!retryable_sdio_error(r)) {
#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
            card->cmd_prof.cmd52_count ++;
#endif
            break;
        }
        unifi_trace(card->ospriv, UDBG2, "retryable SDIO error reading F%d 0x%lX\n", funcnum, addr);
    }

    if ((r == 0) && (retries > 1)) {
        unifi_warning(card->ospriv, "Read succeeded after %d attempts\n", retries);
    }

    if (r) {
        unifi_error(card->ospriv, "Failed to read from UniFi (addr 0x%lX) after %d tries\n",
                    addr, retries - 1);
        /* Report any SDIO error as a general i/o error */
        r = -CSR_EIO;
    }

    return r;
} /* retrying_read16() */


static INLINE CsrInt32
retrying_write16(card_t *card, CsrInt16 funcnum,
                 CsrUint32 addr, CsrUint16 data)
{
    CsrSdioFunction *sdio = card->sdio_if;
    CsrInt32 r = 0;
    CsrInt16 retries;

    retries = 0;
    while (retries++ < SDIO_RETRIES) {
        r = CsrSdioWrite16(sdio, addr, data);
        if (r == -CSR_ENODEV) {
            return r;
        }

        /* 
         * Try again for retryable (CRC or TIMEOUT) errors, 
         * break on success or fatal error
         */
        if (!retryable_sdio_error(r)) {
#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
            card->cmd_prof.cmd52_count ++;
#endif
            break;
        }
        unifi_trace(card->ospriv, UDBG2, "retryable SDIO error writing %02X to F%d 0x%lX\n",
                    data, funcnum, addr);
    }

    if ((r == 0) && (retries > 1)) {
        unifi_warning(card->ospriv, "Write succeeded after %d attempts\n", retries);
    }

    if (r) {
        unifi_error(card->ospriv, "Failed to write to UniFi (addr 0x%lX) after %d tries\n",
                    addr, retries - 1);
        /* Report any SDIO error as a general i/o error */
        r = -CSR_EIO;
    }

    return r;
} /* retrying_write16() */





/*
 * ---------------------------------------------------------------------------
 *  sdio_read_f0
 *
 *      Reads a byte value from the CCCR (func 0) area of UniFi.
 * 
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to read from
 *      pdata   Pointer in which to store the read value.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
CsrInt32
sdio_read_f0(card_t *card, CsrUint32 addr, CsrUint8 *pdata)
{
    return retrying_read8(card, 0, addr, pdata);
} /* sdio_read_f0() */


/*
 * ---------------------------------------------------------------------------
 *  sdio_write_f0
 *
 *      Writes a byte value to the CCCR (func 0) area of UniFi.
 * 
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to read from
 *      data    Data value to write.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
CsrInt32
sdio_write_f0(card_t *card, CsrUint32 addr, CsrUint8 data)
{
    return retrying_write8(card, 0, addr, data);
} /* sdio_write_f0() */


/*
 * ---------------------------------------------------------------------------
 * unifi_read_direct_8_or_16
 *
 *      Read a 8-bit value from the UniFi SDIO interface.
 *
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to read from
 *      pdata   Pointer in which to return data.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *         -ENODEV  card was ejected
 *         -EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read_direct_8_or_16(card_t *card, CsrUint32 addr, CsrUint8 *pdata)
{
#ifdef CSR_WIFI_TRANSPORT_CSPI
    CsrUint16 w;
    CsrInt32 r;

    r = retrying_read16(card, card->function, addr, &w);
    *pdata = (CsrUint8)(w & 0xFF);
    return r;
#else
    return retrying_read8(card, card->function, addr, pdata);
#endif
} /* unifi_read_direct_8_or_16() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_write_direct_8_or_16
 *
 *      Write a byte value to the UniFi SDIO interface.
 * 
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to write to
 *      data    Value to write.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *         -ENODEV  card was ejected
 *         -EIO     an SDIO error occurred
 *
 *  Notes:
 *      If 8-bit write is used, the even address *must* be written second.
 *      This is because writes to odd bytes are cached and not committed
 *      to memory until the preceding even address is written.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_write_direct_8_or_16(card_t *card, CsrUint32 addr, CsrUint8 data)
{
    if (addr & 1) {
        unifi_warning(card->ospriv,
                      "Warning: Byte write to an odd address (0x%lX) is dangerous\n",
                      addr);
    }

#ifdef CSR_WIFI_TRANSPORT_CSPI
    return retrying_write16(card, card->function, addr, (CsrUint16)data);
#else
    return retrying_write8(card, card->function, addr, data);
#endif
} /* unifi_write_direct_8_or_16() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_read_direct16
 *
 *      Read a 16-bit value from the UniFi SDIO interface.
 *
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to read from
 *      pdata   Pointer in which to return data.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      The even address *must* be read first. This is because reads from
 *      odd bytes are cached and read from memory when the preceding
 *      even address is read.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read_direct16(card_t *card, CsrUint32 addr, CsrUint16 *pdata)
{
    return retrying_read16(card, card->function, addr, pdata);
} /* unifi_read_direct16() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_write_direct16
 *
 *      Write a 16-bit value to the UniFi SDIO interface.
 * 
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to write to
 *      data    Value to write.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      The even address *must* be written second. This is because writes to
 *      odd bytes are cached and not committed to memory until the preceding
 *      even address is written.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_write_direct16(card_t *card, CsrUint32 addr, CsrUint16 data)
{
    return retrying_write16(card, card->function, addr, data);
} /* unifi_write_direct16() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_read_direct32
 *
 *      Read a 32-bit value from the UniFi SDIO interface.
 *
 *  Arguments:
 *      card    Pointer to card structure.
 *      addr    Address to read from
 *      pdata   Pointer in which to return data.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read_direct32(card_t *card, CsrUint32 addr, CsrUint32 *pdata)
{
    CsrInt32 r;
    CsrUint16 w0, w1;

    r = retrying_read16(card, card->function, addr, &w0);
    if (r) {
        return r;
    }

    r = retrying_read16(card, card->function, addr+2, &w1);
    if (r) {
        return r;
    }

    *pdata = ((CsrUint32)w1 << 16) | (CsrUint32)w0;

    return 0;
} /* unifi_read_direct32() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_read_directn_match
 *
 *      Read multiple 8-bit values from the UniFi SDIO interface,
 *      stopping when either we have read 'len' bytes or we have read
 *      a octet equal to 'match'.  If 'match' is not a valid octet
 *      then this function is the same as 'unifi_read_directn'.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      addr            Start address to read from.
 *      pdata           Pointer to which to write data.
 *      len             Maximum umber of bytes to read
 *      match           The value to stop reading at.
 *
 *  Returns:
 *      number of octets read on success, negative error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      The even address *must* be read first. This is because reads from
 *      odd bytes are cached and read from memory when the preceding
 *      even address is read.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_read_directn_match(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len, CsrInt8 m)
{
    CsrInt32 r;
    CsrUint32 i;
    CsrUint8 *cptr;
    CsrUint16 w;

    cptr = (CsrUint8 *)pdata;
    for (i=0; i<len; i+=2) {

        r = retrying_read16(card, card->function, addr, &w);
        if (r) {
            return r;
        }

        *cptr++ = ((CsrUint8)w & 0xFF);
        if ((m >= 0) && (((CsrInt8)w & 0xFF) == m)) {
            break;
        }

        if (i+1 == len) {
            /* The len is odd. Ignore the last high byte */
            break;
        }

        *cptr++ = ((CsrUint8)(w >> 8) & 0xFF);
        if ((m >= 0) && (((CsrInt8)(w >> 8) & 0xFF) == m)) {
            break;
        }

        addr+=2;
    }

    return (CsrInt32)(cptr - (CsrUint8 *)pdata);
}


/*
 * ---------------------------------------------------------------------------
 *  unifi_read_directn
 *
 *      Read multiple 8-bit values from the UniFi SDIO interface.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      addr            Start address to read from.
 *      pdata           Pointer to which to write data.
 *      len             Number of bytes to read
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      The even address *must* be read first. This is because reads from
 *      odd bytes are cached and read from memory when the preceding
 *      even address is read.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read_directn(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len)
{
    CsrInt32 r;
    r = unifi_read_directn_match(card, addr, pdata, len, -1);
    return (r < 0) ? r : 0;
} /* unifi_read_directn() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_write_directn
 *
 *      Write multiple 8-bit values to the UniFi SDIO interface.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      addr            Start address to write to.
 *      pdata           Source data pointer.
 *      len             Number of bytes to write, must be even.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      The UniFi has a peculiar 16-bit bus architecture. Writes are only
 *      committed to memory when an even address is accessed. Writes to
 *      odd addresses are cached and only committed if the next write is
 *      to the preceding address.
 *      This means we must write data as pairs of bytes in reverse order.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_write_directn(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len)
{
    CsrInt32 r;
    CsrUint8 *cptr;
    CsrInt16 signed_len;

    cptr = (CsrUint8 *)pdata;
    signed_len = (CsrInt16)len;
    while (signed_len > 0) {
        /* This is UniFi-1 specific code. CSPI not supported so 8-bit write allowed */
        r = retrying_write16(card, card->function, addr, *cptr);
        if (r) {
            return r;
        }

        cptr += 2;
        addr += 2;
        signed_len -= 2;
    }

    return 0;
} /* unifi_write_directn() */



/*
 * ---------------------------------------------------------------------------
 *  set_dmem_page
 *  set_pmem_page
 *
 *      Set up the page register for the shared data memory window or program
 *      memory window.
 *  
 *  Arguments:
 *      card            Pointer to card structure.
 *      dmem_addr       UniFi shared-data-memory address to access.
 *      pmem_addr       UniFi program memory address to access. This includes
 *                        External FLASH memory at    0x000000
 *                        Processor program memory at 0x200000
 *                        External SRAM at memory     0x400000
 *
 *  Returns:
 *      Returns a SDIO address (24-bit) for use in a unifi_read_direct or
 *      unifi_write_direct call, or a negative error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
static CsrInt32
set_dmem_page(card_t *card, CsrUint32 dmem_addr)
{
    CsrUint16 page, addr;
    CsrUint32 len;
    CsrInt32 r;

    if (!ChipHelper_DecodeWindow(card->helper,
                                 CHIP_HELPER_WINDOW_3,
                                 CHIP_HELPER_WT_SHARED,
                                 dmem_addr / 2,
                                 &page, &addr, &len)) {
        unifi_error(card->ospriv, "Failed to decode SHARED_DMEM_PAGE %08lx\n", dmem_addr);
        return -CSR_EINVAL;
    }

    if (page != card->dmem_page) {
        unifi_trace(card->ospriv, UDBG6, "setting dmem page=0x%X, addr=0x%lX\n", page, addr);

        /* change page register */
        r = unifi_write_direct16(card, ChipHelper_HOST_WINDOW3_PAGE(card->helper) * 2, page);
        if (r) {
            unifi_error(card->ospriv, "Failed to write SHARED_DMEM_PAGE\n");
            return r;
        }

        card->dmem_page = page;
    }

    return ((CsrInt32)addr * 2) + (dmem_addr & 1);
} /* set_dmem_page() */



static CsrInt32
set_pmem_page(card_t *card, CsrUint32 pmem_addr,
              enum chip_helper_window_type mem_type)
{
    CsrUint16 page, addr;
    CsrUint32 len;
    CsrInt32 r;

    if (!ChipHelper_DecodeWindow(card->helper,
                                 CHIP_HELPER_WINDOW_2,
                                 mem_type,
                                 pmem_addr / 2,
                                 &page, &addr, &len)) {
        unifi_error(card->ospriv, "Failed to decode PROG MEM PAGE %08lx %d\n", pmem_addr, mem_type);
        return -CSR_EINVAL;
    }

    if (page != card->pmem_page) {
        unifi_trace(card->ospriv, UDBG6, "setting pmem page=0x%X, addr=0x%lX\n", page, addr);

        /* change page register */
        r = unifi_write_direct16(card, ChipHelper_HOST_WINDOW2_PAGE(card->helper) * 2, page);
        if (r) {
            unifi_error(card->ospriv, "Failed to write PROG MEM PAGE\n");
            return r;
        }

        card->pmem_page = page;
    }

    return ((CsrInt32)addr * 2) + (pmem_addr & 1);
} /* set_pmem_page() */




/*
 * ---------------------------------------------------------------------------
 *  set_page
 *
 *      Sets up the appropriate page register to access the given address.
 *      Returns the sdio address at which the unifi address can be accessed.
 *
 *  Arguments:
 *      card            Pointer to card structure.
 *      generic_addr    UniFi internal address to access, in Generic Pointer
 *                      format, i.e. top byte is space indicator.
 *
 *  Returns:
 *      Returns a SDIO address (24-bit) for use in a unifi_read_direct or
 *      unifi_write_direct call, or a negative error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *      -CSR_EINVAL  the address is invalid
 * ---------------------------------------------------------------------------
 */
static CsrInt32
set_page(card_t *card, CsrUint32 generic_addr)
{
    CsrInt32 space;
    CsrUint32 addr;
    CsrInt32 sdio_addr;
    CsrInt32 r;

    space = UNIFI_GP_SPACE(generic_addr);
    addr = UNIFI_GP_OFFSET(generic_addr);
    switch (space)
    {
    case UNIFI_SH_DMEM:
        /* Shared Data Memory is accessed via the Shared Data Memory window */
        sdio_addr = set_dmem_page(card, addr);
        if (sdio_addr < 0) {
            return sdio_addr;
        }
        break;

    case UNIFI_EXT_FLASH:
        if (!ChipHelper_HasFlash(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        /* External FLASH is accessed via the Program Memory window */
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_FLASH);
        break;

    case UNIFI_EXT_SRAM:
        if (!ChipHelper_HasExtSram(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        /* External SRAM is accessed via the Program Memory window */
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_EXT_SRAM);
        break;

    case UNIFI_REGISTERS:
        /* Registers are accessed directly */
        sdio_addr = addr;
        break;

    case UNIFI_PHY_DMEM:
        r = unifi_set_proc_select(card, UNIFI_PROC_PHY);
        if (r)
            return r;
        sdio_addr = ChipHelper_DATA_MEMORY_RAM_OFFSET(card->helper) * 2 + addr;
        break;

    case UNIFI_MAC_DMEM:
        r = unifi_set_proc_select(card, UNIFI_PROC_MAC);
        if (r)
            return r;
        sdio_addr = ChipHelper_DATA_MEMORY_RAM_OFFSET(card->helper) * 2 + addr;
        break;

    case UNIFI_BT_DMEM:
        if (!ChipHelper_HasBt(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        r = unifi_set_proc_select(card, UNIFI_PROC_BT);
        if (r)
            return r;
        sdio_addr = ChipHelper_DATA_MEMORY_RAM_OFFSET(card->helper) * 2 + addr;
        break;

    case UNIFI_PHY_PMEM:
        r = unifi_set_proc_select(card, UNIFI_PROC_PHY);
        if (r)
            return r;
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_CODE_RAM);
        break;

    case UNIFI_MAC_PMEM:
        r = unifi_set_proc_select(card, UNIFI_PROC_MAC);
        if (r)
            return r;
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_CODE_RAM);
        break;

    case UNIFI_BT_PMEM:
        if (!ChipHelper_HasBt(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        r = unifi_set_proc_select(card, UNIFI_PROC_BT);
        if (r)
            return r;
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_CODE_RAM);
        break;

    case UNIFI_PHY_ROM:
        if (!ChipHelper_HasRom(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        r = unifi_set_proc_select(card, UNIFI_PROC_PHY);
        if (r)
            return r;
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_ROM);
        break;

    case UNIFI_MAC_ROM:
        if (!ChipHelper_HasRom(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        r = unifi_set_proc_select(card, UNIFI_PROC_MAC);
        if (r)
            return r;
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_ROM);
        break;

    case UNIFI_BT_ROM:
        if (!ChipHelper_HasRom(card->helper) || !ChipHelper_HasBt(card->helper)) {
            unifi_error(card->ospriv, "Bad address space for chip in generic pointer 0x%08lX\n",
                        generic_addr);
            return -CSR_EINVAL;
        }
        r = unifi_set_proc_select(card, UNIFI_PROC_BT);
        if (r)
            return r;
        sdio_addr = set_pmem_page(card, addr, CHIP_HELPER_WT_ROM);
        break;

    default:
        unifi_error(card->ospriv, "Bad address space in generic pointer 0x%08lX\n",
                    generic_addr);
        return -CSR_EINVAL;
    }

    return sdio_addr;

} /* set_page() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_set_proc_select
 *
 *
 *  Arguments:
 *      card            Pointer to card structure.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_set_proc_select(card_t *card, CsrInt16 select)
{
    CsrInt32 r;

    /* Verify the the select value is allowed. */
    switch (select)
    {
    case UNIFI_PROC_MAC:
    case UNIFI_PROC_PHY:
    case UNIFI_PROC_BOTH:
        break;


    default:
        return -CSR_EINVAL;
    }

    if (card->proc_select != select) {

        r = unifi_write_direct16(card,
                                 ChipHelper_DBG_HOST_PROC_SELECT(card->helper) * 2,
                                 (CsrUint8)select);
        if (r == -CSR_ENODEV) return r;
        if (r) {
            unifi_error(card->ospriv, "Failed to write to Proc Select register\n");
            return r;
        }

        card->proc_select = select;
    }

    return 0;
}


/*
 * ---------------------------------------------------------------------------
 * unifi_read_8_or_16
 *
 * Performs a byte read of the given address in shared data memory.
 * Set up the shared data memory page register as required.
 *
 * Arguments:
 * card Pointer to card structure.
 * unifi_addr UniFi shared-data-memory address to access.
 * pdata Pointer to a byte variable for the value read.
 *
 * Returns:
 * 0 on success, non-zero error code on error:
 * -CSR_ENODEV card was ejected
 * -CSR_EIO an SDIO error occurred
 * -CSR_EINVAL a bad generic pointer was specified
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read_8_or_16(card_t *card, CsrUint32 unifi_addr, CsrUint8 *pdata)
{
    CsrInt32 sdio_addr;
#ifdef CSR_WIFI_TRANSPORT_CSPI
    CsrUint16 w;
    CsrInt32 r;
#endif

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
       return sdio_addr;
    }

#ifdef CSR_WIFI_TRANSPORT_CSPI
    r = retrying_read16(card, card->function, sdio_addr, &w);
    *pdata = (CsrUint8)(w & 0xFF);
    return r;
#else
    return retrying_read8(card, card->function, sdio_addr, pdata);
#endif
} /* unifi_read_8_or_16() */



/*
 * ---------------------------------------------------------------------------
 * unifi_write_8_or_16
 *
 * Performs a byte write of the given address in shared data memory.
 * Set up the shared data memory page register as required.
 *
 * Arguments:
 * card Pointer to card context struct.
 * unifi_addr UniFi shared-data-memory address to access.
 * data Value to write.
 *
 * Returns:
 * 0 on success, non-zero error code on error:
 * -CSR_ENODEV card was ejected
 * -CSR_EIO an SDIO error occurred
 * -CSR_EINVAL a bad generic pointer was specified
 *
 * Notes:
 * Beware using unifi_write8() because byte writes are not safe on UniFi.
 * Writes to odd bytes are cached, writes to even bytes perform a 16-bit
 * write with the previously cached odd byte.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_write_8_or_16(card_t *card, CsrUint32 unifi_addr, CsrUint8 data)
{
    CsrInt32 sdio_addr;
#ifdef CSR_WIFI_TRANSPORT_CSPI
    CsrUint16 w;
#endif

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
        return sdio_addr;
    }

    if (sdio_addr & 1) {
        unifi_warning(card->ospriv,
                      "Warning: Byte write to an odd address (0x%lX) is dangerous\n",
                      sdio_addr);
    }

#ifdef CSR_WIFI_TRANSPORT_CSPI
    w = data;
    return retrying_write16(card, card->function, sdio_addr, w);
#else
    return retrying_write8(card, card->function, sdio_addr, data);
#endif
} /* unifi_write_8_or_16() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_read16
 *
 *      Performs a 16-bit read of the given address in shared data memory.
 *      Set up the shared data memory page register as required.
 *
 *  Arguments:
 *      card            Pointer to card structure.
 *      unifi_addr      UniFi shared-data-memory address to access.
 *      pdata           Pointer to a 16-bit int variable for the value read.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *      -CSR_EINVAL  a bad generic pointer was specified
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read16(card_t *card, CsrUint32 unifi_addr, CsrUint16 *pdata)
{
    CsrInt32 sdio_addr;

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
        return sdio_addr;
    }

    return unifi_read_direct16(card, sdio_addr, pdata);
} /* unifi_read16() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_write16
 *
 *      Performs a 16-bit write of the given address in shared data memory.
 *      Set up the shared data memory page register as required.
 *
 *  Arguments:
 *      card            Pointer to card structure.
 *      unifi_addr      UniFi shared-data-memory address to access.
 *      pdata           Pointer to a byte variable for the value write.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *      -CSR_EINVAL  a bad generic pointer was specified
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_write16(card_t *card, CsrUint32 unifi_addr, CsrUint16 data)
{
    CsrInt32 sdio_addr;

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
        return sdio_addr;
    }

    return unifi_write_direct16(card, sdio_addr, data);
} /* unifi_write16() */



/*
 * ---------------------------------------------------------------------------
 *  unifi_read32
 *
 *      Performs a 32-bit read of the given address in shared data memory.
 *      Set up the shared data memory page register as required.
 *
 *  Arguments:
 *      card            Pointer to card structure.
 *      unifi_addr      UniFi shared-data-memory address to access.
 *      pdata           Pointer to a int variable for the value read.
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *      -CSR_EINVAL  a bad generic pointer was specified
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read32(card_t *card, CsrUint32 unifi_addr, CsrUint32 *pdata)
{
    CsrInt32 sdio_addr;

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
        return sdio_addr;
    }

    return unifi_read_direct32(card, sdio_addr, pdata);
} /* unifi_read32() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_readn
 *  unifi_readnz
 *
 *      Read multiple 8-bit values from the UniFi SDIO interface.
 *      This function interprets the address as a GenericPointer as
 *      defined in the UniFi Host Interface Protocol Specification.
 *      The readnz version of this function will stop when it reads a
 *      zero octet.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      unifi_addr      UniFi shared-data-memory address to access.
 *      pdata           Pointer to which to write data.
 *      len             Number of bytes to read
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *      -CSR_EINVAL  a bad generic pointer was specified
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_readn_match(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len, CsrInt8 match)
{
    CsrInt32 sdio_addr;
    CsrInt32 r;

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
        return sdio_addr;
    }

    r = unifi_read_directn_match(card, sdio_addr, pdata, len, match);
    return (r < 0) ? r : 0;
} /* unifi_readn_match() */

CsrInt32
unifi_readn(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len)
{
    return unifi_readn_match(card, unifi_addr, pdata, len, -1);
} /* unifi_readn() */

CsrInt32
unifi_readnz(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len)
{
    return unifi_readn_match(card, unifi_addr, pdata, len, 0);
} /* unifi_readnz() */

/*
 * ---------------------------------------------------------------------------
 *  read_shared_count
 *
 *      Read signal count locations, checking for an SDIO error.  The
 *      signal count locations only contain a valid number if the
 *      highest bit isn't set.
 *
 *  Arguments:
 *      card            Pointer to card context structure.
 *      addr            Shared-memory address to read.
 *
 *  Returns:
 *      Value read from memory (0-127) or CSR negative error code.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_read_shared_count(card_t *card, CsrUint32 addr)
{
    CsrUint8 b;
    /* I've increased this count, because I have seen cases where
     * there were three reads in a row with the top bit set.  I'm not
     * sure why this might have happened, but I can't see a problem
     * with increasing this limit.  It's better to take a while to
     * recover than to fail. */
#define SHARED_READ_RETRY_LIMIT 10
    CsrInt32 r, i;

    /* 
     * Get the to-host-signals-written count.
     * The top-bit will be set if the firmware was in the process of
     * changing the value, in which case we read again.
     */
    /* Limit the number of repeats so we don't freeze */
    for (i=0; i<SHARED_READ_RETRY_LIMIT; i++) {
        r = unifi_read_8_or_16(card, addr, &b);
        if (r) {
            return r;
        }
        if (!(b & 0x80)) {
            /* There is a chance that the MSB may have contained invalid data
             * (overflow) at the time it was read. Therefore mask off the MSB.
             * This avoids a race between driver read and firmware write of the
             * word, the value we need is in the lower 8 bits anway.
             */
            return (CsrInt32)(b & 0xff);
        }
    }

    return -CSR_EIO;
} /* read_shared_count() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_writen
 *
 *      Write multiple 8-bit values to the UniFi SDIO interface using CMD52
 *      This function interprets the address as a GenericPointer as
 *      defined in the UniFi Host Interface Protocol Specification.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      unifi_addr      UniFi shared-data-memory address to access.
 *      pdata           Pointer to which to write data.
 *      len             Number of bytes to write
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *      -CSR_EINVAL    an odd length or length too big.
 *      -CSR_ETIMEDOUT timed out waiting for rewind.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_writen(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len)
{
    CsrInt32 sdio_addr;

    sdio_addr = set_page(card, unifi_addr);
    if (sdio_addr < 0) {
        return sdio_addr;
    }

    return unifi_write_directn(card, sdio_addr, pdata, len);
} /* unifi_writen() */



static CsrInt32 
csr_sdio_block_rw(card_t *card, CsrInt16 funcnum,
                  CsrUint32 addr, CsrUint8 *pdata,
                  CsrUint16 count, CsrInt16 dir_is_write)
{
    CsrInt32 r;

    if (dir_is_write == UNIFI_SDIO_READ) {
        r = CsrSdioRead(card->sdio_if, addr, pdata, count);
    } else {
        r = CsrSdioWrite(card->sdio_if, addr, pdata, count);
    }

#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
    card->cmd_prof.cmd53_count ++;
#endif
    return r;
}


#ifdef CSR_WIFI_DEBUG_CSPI_CMD53_ERRORS
static CsrUint32 cmd53_read_count = 0;
static CsrUint32 cmd53_read_rewinds = 0;
#endif

/*
 * ---------------------------------------------------------------------------
 *  unifi_bulk_rw
 *
 *      Transfer bulk data to or from the UniFi SDIO interface.
 *      This function is used to read or write signals and bulk data.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      handle          Value to put in the Register Address field of the CMD53 req.
 *      data            Pointer to data to write.
 *      direction       One of UNIFI_SDIO_READ or UNIFI_SDIO_WRITE
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      This function uses SDIO CMD53, which is the block transfer mode.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_bulk_rw(card_t *card, CsrUint32 handle, void *pdata,
              CsrUint32 len, CsrInt16 direction)
{
#define CMD53_RETRIES 3
/*
 * Ideally instead of sleeping, we want to busy wait.
 * Currently there is no framework API to do this. When it becomes available,
 * we can use it to busy wait using usecs
 */
#define REWIND_RETRIES          15 /* when REWIND_DELAY==1msec, or 250 when REWIND_DELAY==50usecs */
#define REWIND_POLLING_RETRIES  5
#define REWIND_DELAY            1  /* msec or 50usecs */
#define CLEANUP_RETRIES         10
    CsrInt32 r = 0;
    CsrInt16 retries = CMD53_RETRIES;
    CsrInt16 stat_retries;
    CsrUint8 stat;
    CsrInt16 dump_read;
#ifdef CSR_WIFI_MAKE_FAKE_CMD53_ERRORS
    static CsrInt16 fake_error;
#endif
#ifdef CSR_WIFI_DEBUG_CSPI_CMD53_ERRORS
    CsrInt16 rewind_print = 0;
#endif

    dump_read = 0;
    if (((CsrUint32)pdata) & 1) {
        unifi_notice(card->ospriv, "CD53 request on a unaligned buffer (addr: 0x%X) dir %s-Host\n",
                     pdata, (direction == UNIFI_SDIO_READ) ? "To" : "From");
        if (direction == UNIFI_SDIO_WRITE) {
            dump(pdata, (CsrUint16)len);
        } else {
            dump_read = 1;
        }
    }

    /* Defensive checks */
    if (!pdata) {
        unifi_error(card->ospriv, "Null pdata for unifi_bulk_rw() len: %d\n", len);
        return -CSR_EINVAL;
    }
    if ((len & 1) || (len > 0xffff)) {
        unifi_error(card->ospriv, "Impossible CMD53 length requested: %d\n", len);
        return -CSR_EINVAL;
    }

#ifdef CSR_WIFI_DEBUG_CSPI_CMD53_ERRORS
    if (direction == UNIFI_SDIO_READ) {
        cmd53_read_count++;
    }
#endif

    while (1)
    {
        r = csr_sdio_block_rw(card, card->function, handle,
                              (CsrUint8 *)pdata, (CsrUint16)len, direction);
        if (r == -CSR_ENODEV) {
            return r;
        }
#ifdef CSR_WIFI_MAKE_FAKE_CMD53_ERRORS
        if (++fake_error > 100) {
            fake_error = 90;
            unifi_warning(card->ospriv, "Faking a CMD53 error,\n");
            if (r == 0) {
                r = -CSR_EIO;
            }
        }
#endif
        if (r == 0) {
#ifdef CSR_WIFI_TRANSPORT_CSPI
            /*
             * Using CSPI transport, a read transfer may succeed
             * however the data could be invalid.
             */
            if (direction == UNIFI_SDIO_WRITE) {
                /* success */
                if (dump_read) {
                    dump(pdata, (CsrUint16)len);
                }
                break;
            } else {
                CsrUint8 *p_data = (CsrUint8 *)pdata;
                CsrInt16 cleanup_retries = 0;
                CsrInt16 i;

#ifdef CSR_WIFI_DEBUG_CSPI_CMD53_ERRORS
                /* print some statistics after the rewind */
                if (rewind_print) {
                    unifi_error(card->ospriv,
                                "CMD53 read: cnt:%lu rew:%lu\n",
                                cmd53_read_count, cmd53_read_rewinds);
                }
#endif
                /* look for a failing pattern */
                for (i = 2; i < len; i++) {
                    if (p_data[i] != p_data[i-1]) {
                        break;
                    }
                }

                /* when a failure is detected, clear the h/w state and rewind */
                if ((i == len) && (len > 2)) {

                    if (!card->cspi_cleanup_handle) {
                        unifi_error(card->ospriv, "Can not retry CMD53 - no cleanup handle\n");
                        return -CSR_EINVAL;
                    }

                    cleanup_retries = 0;
                    while (cleanup_retries < CLEANUP_RETRIES) {

                        cleanup_retries ++;
                        r = csr_sdio_block_rw(card, card->function, card->cspi_cleanup_handle,
                                              (CsrUint8 *)pdata, 8, UNIFI_SDIO_WRITE);
                        if (r == 0) {
                            break;
                        } else {
                            unifi_warning(card->ospriv,
                                          "CMD53 cleanup cmd failed: %d\n", r);
                        }
                    }

                    /* If the cleanup fails, there is no point on retrying. */
                    if (cleanup_retries == CLEANUP_RETRIES) {
                        unifi_error(card->ospriv,
                                    "CMD53 cleanup retries exhausted\n");
                        return -CSR_EINVAL;
                    }

#ifdef CSR_WIFI_DEBUG_CSPI_CMD53_ERRORS
                    cmd53_read_rewinds++;
                    rewind_print = 1;
#endif
                    /* Set r for rewind */
                    r = -CSR_EIO;
                } else {
                    /* success */
                    if (dump_read) {
                        dump(pdata, (CsrUint16)len);
                    }
                    break;
                }
            }
#else
            /* success */
            if (dump_read) {
                dump(pdata, (CsrUint16)len);
            }
            break;
#endif
        }

        /*
         * At this point the SDIO driver should have written the I/O Abort
         * register to notify UniFi that the command has failed.
         * UniFi-1 and UniFi-2 (not UF6xxx) use the same register to store the 
         * Deep Sleep State. This means we have to restore the Deep Sleep
         * State (AWAKE in any case since we can not perform a CD53 in any other
         * state) by rewriting the I/O Abort register to its previous value.
         */
        if (card->chip_id <= SDIO_CARD_ID_UNIFI_2) {
            unifi_set_host_state(card, UNIFI_HOST_STATE_AWAKE);
        }
        
        if (!retryable_sdio_error(r)) {
            unifi_error(card->ospriv, "Fatal error in a CMD53 transfer\n");
            break;
        }

        /*
         * These happen from time to time, life sucks, try again
         */
        if (--retries == 0) {
            break;
        }

        unifi_trace(card->ospriv, UDBG4,
                    "Error in a CMD53 transfer, retrying (h:%d,l:%u)...\n",
                    (CsrInt16)handle & 0xff, len);

        /* The transfer failed, rewind and try again */
        r = unifi_write_8_or_16(card, card->sdio_ctrl_addr+8,
                                (CsrUint8)(handle & 0xff));
        if (r == -CSR_ENODEV) {
            return r;
        }
        if (r) {
            /* 
             * If we can't even do CMD52 (register read/write) then
             * stop here.
             */
            unifi_error(card->ospriv, "Failed to write REWIND cmd\n");
            return r;
        }

        /* Signal the UniFi to look for the rewind request. */
        r = CardGenInt(card);
        if (r) {
            return r;
        }

        /* Wait for UniFi to acknowledge the rewind */
        stat_retries = REWIND_RETRIES;
        while (1) {
            r = unifi_read_8_or_16(card, card->sdio_ctrl_addr+8, &stat);
            if (r == -CSR_ENODEV) {
                return r;
            }
            if (r) {
                unifi_error(card->ospriv, "Failed to read REWIND status\n");
                return -CSR_EIO;
            }

            if (stat == 0) {
                break;
            }
            if (--stat_retries == 0) {
                unifi_error(card->ospriv, "Timeout waiting for REWIND ready\n");
                return -CSR_ETIMEDOUT;
            }

            /* Poll for the ack a few times */
            if (stat_retries < REWIND_RETRIES - REWIND_POLLING_RETRIES) {
                CsrThreadSleep(REWIND_DELAY);
            }
        }
    }

    if (r) {
        unifi_error(card->ospriv, "Block %s failed after %d retries\n",
                    (direction == UNIFI_SDIO_READ) ? "read" : "write",
                    CMD53_RETRIES - retries);
        /* Report any SDIO error as a general i/o error */
        return -CSR_EIO;
    }

    /* Collect some stats */
    if (direction == UNIFI_SDIO_READ) {
        card->sdio_bytes_read += len;
    } else {
        card->sdio_bytes_written += len;
    }

    return 0;
} /* unifi_bulk_rw() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_bulk_rw_noretry
 *
 *      Transfer bulk data to or from the UniFi SDIO interface.
 *      This function is used to read or write signals and bulk data.
 * 
 *  Arguments:
 *      card            Pointer to card structure.
 *      handle          Value to put in the Register Address field of
 *                      the CMD53 req.
 *      data            Pointer to data to write.
 *      direction       One of UNIFI_SDIO_READ or UNIFI_SDIO_WRITE
 *
 *  Returns:
 *      0 on success, non-zero error code on error:
 *      -CSR_ENODEV  card was ejected
 *      -CSR_EIO     an SDIO error occurred
 *
 *  Notes:
 *      This function uses SDIO CMD53, which is the block transfer mode.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_bulk_rw_noretry(card_t *card, CsrUint32 handle, void *pdata,
                      CsrUint32 len, CsrInt16 direction)
{
    CsrInt32 r;

    r = csr_sdio_block_rw(card, card->function, handle,
                          (CsrUint8 *)pdata, (CsrUint16)len, direction);
    if (r == -CSR_ENODEV) {
        return r;
    }
    if (r) {
        unifi_error(card->ospriv, "Block %s failed\n",
                    (direction == UNIFI_SDIO_READ) ? "read" : "write");
        /* Report any SDIO error as a general i/o error */
        r = -CSR_EIO;
    }

    return r;
} /* unifi_bulk_rw_noretry() */


