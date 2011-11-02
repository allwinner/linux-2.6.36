/*
 * ---------------------------------------------------------------------------
 * FILE: dump.c
 *
 * PURPOSE:
 *      Routines for retrieving and buffering core status from the UniFi
 *
 * Copyright (C) 2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#include "driver/unifi.h"
#include "driver/unifiversion.h"
#include "card.h"
 
static CsrInt32 unifi_coredump_from_sdio(card_t *card,
                                         coredump_buffer *dump_buf);
static CsrInt32 unifi_coredump_read_common(card_t *card,
                                           regdump_buffer *dump_buf);
static CsrInt32 unifi_coredump_read_cpu(card_t *card, CsrInt16 cpu,
                                        cpudump_buffer *dump_buf);
static CsrInt32 unifi_coredump_get_data_area(coredump_buffer *dump,
                                             CsrInt16 cpu, CsrUint32 offset);

/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_request_at_next_reset
 *
 *      Request that a mini-coredump is performed when the driver has
 *      completed resetting the UniFi device.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      enable          If non-zero, sets the request.
 *                      If zero, cancels any pending request.
 * 
 *  Returns:
 *      0 on success,
 *      -CSR_EIO            SDIO error
 *
 *  Notes:
 *      This function is typically called once the driver has detected that
 *      the UniFi device has become unresponsive due to crash, or internal 
 *      watchdog reset. The driver must reset it to regain communication and,
 *      immediately after that, the mini-coredump can be captured.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_coredump_request_at_next_reset(card_t *card, CsrInt8 enable)
{
    CsrInt32 r;

    func_enter();
    unifi_trace(card->ospriv, UDBG2, "Mini-coredump requested after reset\n");

    if (card == NULL) {
        r = -CSR_EINVAL;
    } else {
        card->request_coredump_on_reset = enable ? 1 : 0;
        r = 0;
    }

    func_exit();
    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_handle_request
 *
 *      Performs a coredump now, if one was requested, and clears the request.
 *
 *  Arguments:
 *      card            Pointer to card struct
 * 
 *  Returns:
 *      0  on success,
 *      nonzero on error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
CsrInt32 unifi_coredump_handle_request(card_t *card)
{
    CsrInt32 r = 0;

    func_enter();

    if (card == NULL) {
        r = -CSR_EINVAL;
    } else {
        if (card->request_coredump_on_reset == 1) {
            card->request_coredump_on_reset = 0;
            r = unifi_coredump_capture(card, NULL);
        }
    }

    func_exit();
    return r;
}

/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_capture
 *
 *      Capture the current status of the UniFi device.
 *      Various registers are buffered for future offline inspection.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      req             Pointer to request struct, or NULL:
 *                          A coredump requested manually by the user app
 *                          will have a request struct pointer, an automatic
 *                          coredump will have a NULL pointer.
 *  Returns:
 *      0 on success,
 *      -CSR_EIO            SDIO error
 *
 *  Notes:
 *      The result is a filled entry in the circular buffer of core dumps,
 *      values from which can be extracted to userland via an ioctl.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_coredump_capture(card_t *card, unifi_coredump_req_t *req)
{
    CsrInt32 r;
    static CsrUint16 dump_seq_no = 1;

    func_enter();

    if (card->dump_next_write == NULL) {
        r = 0;
        goto done;
    }


    /*
     * Force a mini-coredump capture right now
     */
    unifi_info(card->ospriv, "Mini-coredump capture\n");

    /* Wake up the processors so we can talk to them */
    r = unifi_set_host_state(card, UNIFI_HOST_STATE_AWAKE);
    if (r) {
        unifi_error(card->ospriv, "Failed to wake UniFi\n");
        goto done;
    }
    CsrThreadSleep(20);

    /* Stop both XAPs */
    unifi_trace(card->ospriv, UDBG4, "Stopping XAPs for coredump capture\n");
    r = card_stop_processor(card, UNIFI_PROC_BOTH);
    if (r) {
        unifi_error(card->ospriv, "Failed to stop UniFi XAPs\n");
        goto done;
    }

    /* Dump core into the next available slot in the circular list */
    r = unifi_coredump_from_sdio(card, card->dump_next_write);
    if (r == 0) {
        /* Record whether the dump was manual or automatic */
        card->dump_next_write->requestor = (req ? 1 : 0);
        /* Advance to the next buffer */
        card->dump_next_write->count = dump_seq_no++;
        card->dump_cur_read = card->dump_next_write;
        card->dump_next_write = card->dump_next_write->next;

        /* Sequence no. of zero indicates slot not in use, so handle wrap */
        if (dump_seq_no == 0) {
            dump_seq_no = 1;
        }

        unifi_trace(card->ospriv, UDBG3,
                    "Coredump (%p), SeqNo=%d, cur_read=%p, next_write=%p\n",
                    req,
                    card->dump_cur_read->count,
                    card->dump_cur_read, card->dump_next_write);
    }

    /* Start both XAPs */
    unifi_trace(card->ospriv, UDBG4, "Restart XAPs after coredump\n");
    r = card_start_processor(card, UNIFI_PROC_BOTH);
    if (r) {
        unifi_error(card->ospriv, "Failed to start UniFi XAPs\n");
        goto done;
    }

done:
    func_exit();
    return r;

} /* unifi_coredump_capture() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_get_value
 *
 *      Retrieve the value of a register buffered from a previous core dump,
 *      so that it may be reported back to application code.
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      req_reg         Pointer to request parameter partially filled. This
 *                      function puts in the values retrieved from the dump.
 *
 *  Returns:
 *      0 on success,
 *      -CSR_EINVAL         Null parameter error
 *      -CSR_ERANGE         Register out of range or Dump index not (yet) captured
 *      -CSR_EFAULT         Memory space out of range
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_coredump_get_value(card_t *card, unifi_coredump_req_t *req)
{
    CsrInt32 r;
    CsrInt32 i = 0;
    coredump_buffer *find_dump = NULL;

    func_enter();

    if (req == NULL || card == NULL) {
        r = -CSR_EINVAL;
        goto done;
    }
    req->value = -1;
    if (card->dump_buf == NULL) {
        unifi_trace(card->ospriv, UDBG2, "No coredump buffers\n");
        r = -CSR_EINVAL;     /* Coredumping disabled */
        goto done;
    }
    if (card->dump_cur_read == NULL) {
        unifi_trace(card->ospriv, UDBG4, "No coredumps captured\n");
        r = -CSR_ERANGE;     /* No coredump yet captured */
        goto done;
    }

    /* Find the requested dump buffer */
    switch (req->index) {
        case 0:     /* Newest */
            find_dump = card->dump_cur_read;
            break;
        case -1:    /* Oldest: The next used slot forward */
            for (find_dump = card->dump_cur_read->next;
                 (find_dump->count == 0) && (find_dump != card->dump_cur_read);
                 find_dump = card->dump_cur_read->next)
            {
            }
            break;
        default:    /* Number of steps back from current read position */
            for (i = 0, find_dump = card->dump_cur_read;
                 i < req->index;
                 i++, find_dump = find_dump->prev) {
                /* Walk the list for the index'th entry, but
                 * stop when about to wrap. */
                unifi_trace(card->ospriv, UDBG6,
                            "%d: %d, @%p, p=%p, n=%p, cr=%p, h=%p\n",
                            i, find_dump->count, find_dump, find_dump->prev,
                            find_dump->next, card->dump_cur_read, card->dump_buf);
                if (find_dump->prev == card->dump_cur_read) {
                    /* Wrapped but still not found, index out of range */
                    if (i != req->index) {
                        unifi_trace(card->ospriv, UDBG6, "Out of range @%d\n", i);
                        r = -CSR_ERANGE;
                        goto done;
                    }
                    break;
                }
            }
            break;
    }

    /* Check if the slot is actually filled with a core dump */
    if (find_dump->count == 0) {
        unifi_trace(card->ospriv, UDBG4, "Not captured %d\n", req->index);
        r = -CSR_ERANGE;
        goto done;
    }

    unifi_trace(card->ospriv, UDBG4, "Req index %d, found seq %d at step %d\n",
                req->index, find_dump->count, i);

    /* Find the appropriate entry in the buffer */
    switch (req->space) {
        case UNIFI_COREDUMP_MAC_REG:
            if (req->offset < HIP_COREDUMP_NUM_CPU_REGS) {
                req->value = find_dump->regs->cpu[UNIFI_PROC_MAC].regs[req->offset];
            }
            break;
        case UNIFI_COREDUMP_PHY_REG:
            if (req->offset < HIP_COREDUMP_NUM_CPU_REGS) {
                req->value = find_dump->regs->cpu[UNIFI_PROC_PHY].regs[req->offset];
            }
            break;
        case UNIFI_COREDUMP_SH_DMEM:   
            if (req->offset < HIP_COREDUMP_NUM_WORDS_SHARED) {
                req->value = find_dump->regs->shared[req->offset];
            }
            break;
        case UNIFI_COREDUMP_MAC_DMEM:
            req->value = unifi_coredump_get_data_area(find_dump, UNIFI_PROC_MAC,
                                                      req->offset);
            break;
        case UNIFI_COREDUMP_PHY_DMEM:
            req->value = unifi_coredump_get_data_area(find_dump, UNIFI_PROC_PHY,
                                                      req->offset);
            break;
        default:
            r = -CSR_EFAULT; /* Unknown memory space */
            break;
    }

    if (req->value < 0) {
        r = -CSR_ERANGE;     /* Un-captured register */
        unifi_trace(card->ospriv, UDBG4,
                    "Can't read space %d, reg 0x%x from coredump buffer %d\n",
                    req->space, req->offset, req->index);
    } else {
        r = 0;
    }

    req->chip_ver = find_dump->regs->chip_ver;
    req->fw_ver = find_dump->regs->fw_ver;
    req->timestamp = find_dump->timestamp;
    req->requestor = find_dump->requestor;
    req->serial = find_dump->count;

done:
    func_exit();
    return r;
} /* unifi_coredump_get_value() */

/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_get_data_area
 *      
 *      Retrieve the previously coredump-buffered XAP data RAM value
 *      for the appropriate offset.
 *
 *  Arguments:
 *      dump            Pointer to coredump buffer being examined
 *      cpu             UNIFI_PROC_PHY/MAC validated outside of this function
 *      offset          16-bit word offset to retreive
 *
 *  Returns:
 *      0 on success,
 *      -1 if the offset is not in the recorded range
 *
 *  Notes:
 *      The bottom and top ends of the data RAM is copied into different
 *      arrays purely to save memory and reduce unnecessary copies by leaving
 *      out the "don't care" stuff in the middle.
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_coredump_get_data_area(coredump_buffer *dump, CsrInt16 cpu, CsrUint32 offset)
{
    CsrUint32 first_hi;
    first_hi = (cpu == UNIFI_PROC_PHY) ? HIP_COREDUMP_FIRST_WORD_HIGH_PHY :
                                         HIP_COREDUMP_FIRST_WORD_HIGH_MAC;

    if (offset < HIP_COREDUMP_FIRST_WORD_LOW + HIP_COREDUMP_NUM_WORDS_LOW) {
        return (CsrInt32)dump->regs->cpu[cpu].data_low[offset];
    } else if (offset >= first_hi &&
               offset < first_hi + HIP_COREDUMP_NUM_WORDS_HIGH) {
        offset -= first_hi;
        return (CsrInt32)dump->regs->cpu[cpu].data_high[offset];
    } else {
        return -CSR_ERANGE;  /* Out of range */
    }
}


/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_read_common
 *
 *      Capture the content of the UniFi processors shared data RAM, over SDIO
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      reg_buffer      Buffer into which register values will be dumped
 *
 *  Returns:
 *      0 on success,
 *      -CSR_EIO            SDIO error
 *      -CSR_EINVAL         Parameter error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_coredump_read_common(card_t *card, regdump_buffer *dump_buf)
{
    CsrInt32 r;
    CsrUint16 val;
    CsrUint32 sdio_addr, gen_addr;

    func_enter();

    /* Chip version */
    unifi_trace(card->ospriv, UDBG4, "Get chip version\n");
    sdio_addr = 2 * ChipHelper_GBL_CHIP_VERSION(card->helper);
    if (sdio_addr != 0) {
        r = unifi_read_direct16(card, sdio_addr, &val);
        if (r == -CSR_ENODEV) {
            goto done;
        }
        if (r) {
            unifi_error(card->ospriv, "Can't read GBL_CHIP_VERSION\n");
            goto done;
        }
    }
    dump_buf->chip_ver = val;
    dump_buf->fw_ver = (CsrUint16) card->build_id;
    unifi_trace(card->ospriv, UDBG4, "chip_ver 0x%04x, fw_ver %d\n",
                dump_buf->chip_ver, dump_buf->fw_ver);

    /* Shared data RAM (byte-wise) */
    gen_addr = UNIFI_MAKE_GP(SH_DMEM, HIP_COREDUMP_FIRST_WORD_SHARED * 2);
    unifi_trace(card->ospriv, UDBG4, "Dump shared data RAM @%8x\n", gen_addr);
    r = unifi_readn(card, gen_addr, dump_buf->shared, HIP_COREDUMP_NUM_WORDS_SHARED * 2);
    if (r == -CSR_ENODEV) {
       goto done;
    }
    if (r) {
        unifi_error(card->ospriv, "Can't read UniFi shared data area\n");
        goto done;
    }

done:
    func_exit();
    return r;
} /* unifi_coredump_read_common */



/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_read_cpu
 *
 *      Capture the content of a UniFi CPU's local data RAM, and its
 *      registers valuesover SDIO
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      cpu             XAP to probe: UNIFI_PROC_MAC or UNIFI_PROC_PHY
 *      reg_buffer      Buffer into which register values will be dumped
 *
 *  Returns:
 *      0 on success,
 *      -CSR_EIO            SDIO error
 *      -CSR_EINVAL         Parameter error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_coredump_read_cpu(card_t *card, CsrInt16 cpu, cpudump_buffer *dump_buf)
{
    CsrInt32 r;
    CsrUint32 gen_addr;
    CsrUint32 first_hi;

    func_enter();

    /* Select the XAP */
    if (cpu != UNIFI_PROC_MAC && cpu != UNIFI_PROC_PHY) {
        r = -CSR_EINVAL;
        goto done;
    }
    r = unifi_set_proc_select(card, cpu);
    if (r) {
        goto done;
    }

    /* XAP registers (bytewise) */
    gen_addr = UNIFI_MAKE_GP(REGISTERS, HIP_COREDUMP_FIRST_CPU_REG * 2);
    unifi_trace(card->ospriv, UDBG4, "Dump CPU %d registers @%08x\n", cpu, gen_addr);
    r = unifi_readn( card, gen_addr, &dump_buf->regs, HIP_COREDUMP_NUM_CPU_REGS * 2);
    if (r == -CSR_ENODEV) {
       goto done;
    }
    if (r) {
        unifi_error(card->ospriv, "Can't read XAP %d registers\n", cpu);
        goto done;
    } 

    /* Data RAM low area (bytewise) */
    if (cpu == UNIFI_PROC_MAC) {
        gen_addr = UNIFI_MAKE_GP(MAC_DMEM, HIP_COREDUMP_FIRST_WORD_LOW * 2);
    } else {
        gen_addr = UNIFI_MAKE_GP(PHY_DMEM, HIP_COREDUMP_FIRST_WORD_LOW * 2);
    }
    unifi_trace(card->ospriv, UDBG4, "Dump CPU %d low RAM @%08x\n", cpu, gen_addr);
    r = unifi_readn(card, gen_addr, dump_buf->data_low, HIP_COREDUMP_NUM_WORDS_LOW * 2);
    if (r == -CSR_ENODEV) {
       goto done;
    }
    if (r) {
        unifi_error(card->ospriv, "Can't read XAP %d low data area\n", cpu);
        goto done;
    }

    /* Data RAM high area (bytewise) */    
    first_hi = (cpu == UNIFI_PROC_PHY) ? HIP_COREDUMP_FIRST_WORD_HIGH_PHY :
                                         HIP_COREDUMP_FIRST_WORD_HIGH_MAC;
    if (cpu == UNIFI_PROC_MAC) {
        gen_addr = UNIFI_MAKE_GP(MAC_DMEM, first_hi * 2);
    } else {
        gen_addr = UNIFI_MAKE_GP(PHY_DMEM, first_hi * 2);
    }
    unifi_trace(card->ospriv, UDBG4, "Dump CPU %d high RAM @%08x\n", cpu, gen_addr);
    r = unifi_readn(card, gen_addr, dump_buf->data_high, HIP_COREDUMP_NUM_WORDS_HIGH * 2);
    if (r == -CSR_ENODEV) {
       goto done;
    }
    if (r) {
        unifi_error(card->ospriv, "Can't read XAP %d low data area\n", cpu);
        goto done;
    }

done:
    func_exit();
    return r;
} /* unifi_coredump_read_cpu */


/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_from_sdio
 *
 *      Capture the status of the UniFi processors, over SDIO
 *
 *  Arguments:
 *      card            Pointer to card struct
 *      reg_buffer      Buffer into which register values will be dumped
 *
 *  Returns:
 *      0 on success,
 *      -CSR_EIO            SDIO error
 *      -CSR_EINVAL         Parameter error
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
static CsrInt32
unifi_coredump_from_sdio(card_t *card, coredump_buffer *dump_buf)
{
    CsrInt16 i;
    CsrInt32 r;

    func_enter();

    if (dump_buf == NULL) {
        r = -CSR_EINVAL;
        goto done;
    }
    if (dump_buf->regs == NULL) {
        r = -CSR_EINVAL;
        goto done;
    }


    /* The host needs to fill this, if it needs to */
    dump_buf->timestamp = 0;

    /* Common area */
    r = unifi_coredump_read_common(card, dump_buf->regs);
    if (r != 0) {
        goto done;
    }

    /* Each XAP area */
    for (i = 0; i < HIP_COREDUMP_NUM_CPU; i++) {
        r = unifi_coredump_read_cpu(card, i, &dump_buf->regs->cpu[i]);
        if (r != 0) {
            goto done;
        }
    }

done:
    func_exit();
    return r;

} /* unifi_coredump_from_sdio() */

#ifndef UNIFI_DISABLE_COREDUMP
/*
 * ---------------------------------------------------------------------------
 *  new_coredump_node
 *
 *      Allocates a coredump linked-list node, and links it to the previous.
 *
 *  Arguments:
 *      ospriv          OS context
 *      prevnode        Previous node to link into
 *                      
 *  Returns:
 *      Pointer to valid coredump_buffer on success
 *      NULL on memory allocation failure
 *
 *  Notes:
 *      Allocates "all or nothing"
 * ---------------------------------------------------------------------------
 */
static
coredump_buffer *new_coredump_node(void *ospriv, coredump_buffer *prevnode)
{
    coredump_buffer *newnode = NULL;

    /* Allocate node header */
    newnode = (coredump_buffer *)CsrMemAlloc(sizeof(coredump_buffer));
    if (newnode == NULL) {
        return NULL;
    }
    CsrMemSet(newnode, 0, sizeof(coredump_buffer));

    /* Allocate space for register capture */
    newnode->regs = CsrMemAlloc(HIP_COREDUMP_DATA_SIZE);
    if (newnode->regs == NULL) {
       CsrMemFree(newnode);
       return NULL;
    }
    CsrMemSet(newnode->regs, 0, HIP_COREDUMP_DATA_SIZE);

    /* Link to previous node */
    newnode->prev = prevnode;
    if (prevnode) {
        prevnode->next = newnode;
    }
    newnode->next = NULL;

    return newnode;
}
#endif /* UNIFI_DISABLE_COREDUMP */

/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_init
 *
 *      Allocates buffers for the automatic SDIO core dump
 *
 *  Arguments:
 *      card                Pointer to card struct
 *      num_dump_buffers    Number of buffers to reserve for coredumps
 *
 *  Returns:
 *      0 on success,
 *      -CSR_ENOMEM         memory allocation failed
 *
 *  Notes:
 *      Allocates space in advance, to be used for the last n coredump buffers
 *      the intention being that the size is sufficient for at least one dump,
 *      probably several.
 *      It's probably advisable to have at least 2 coredump buffers to allow
 *      one to be enquired with the unifi_coredump tool, while leaving another
 *      free for capturing.
 * ---------------------------------------------------------------------------
 */
CsrInt32
unifi_coredump_init(card_t *card, CsrUint16 num_dump_buffers)
{
#ifndef UNIFI_DISABLE_COREDUMP
    void *ospriv = card->ospriv;
    coredump_buffer *prev = NULL;
    coredump_buffer *newnode = NULL;
    CsrUint32 i = 0;
#endif

    func_enter();

    card->request_coredump_on_reset = 0;
    card->dump_next_write = NULL;
    card->dump_cur_read = NULL;
    card->dump_buf = NULL;

#ifndef UNIFI_DISABLE_COREDUMP
    unifi_trace(ospriv, UDBG1, "Allocate %d bytes for %d core dumps\n",
               num_dump_buffers * HIP_COREDUMP_DATA_SIZE, num_dump_buffers);
    if (num_dump_buffers == 0) {
        goto done;
    }

    /* Root node */
    card->dump_buf = new_coredump_node(ospriv, NULL);
    if (card->dump_buf == NULL) {
        goto fail;
    }
    prev = card->dump_buf;
    newnode = card->dump_buf;

    /* Add each subsequent node at tail */
    for (i = 1; i < num_dump_buffers; i++) {
        newnode = new_coredump_node(ospriv, prev);
        if (newnode == NULL) {
            goto fail;
        }
        prev = newnode;
    }

    /* Link the first and last nodes to make the list circular */
    card->dump_buf->prev = newnode;
    newnode->next = card->dump_buf;

    /* Set initial r/w access pointers */
    card->dump_next_write = card->dump_buf;
    card->dump_cur_read = NULL;

    unifi_trace(ospriv, UDBG2, "Core dump configured (%d dumps max)\n", i);

done:
#endif
    func_exit();
    return 0;

#ifndef UNIFI_DISABLE_COREDUMP
fail:
    /* Unwind what we allocated so far */
    unifi_error(ospriv, "Out of memory allocating core dump node %d\n", i);
    unifi_coredump_free(card);    
    func_exit();
    return -CSR_ENOMEM;
#endif
} /* unifi_coreump_init() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_coredump_free
 *
 *      Free all memory dynamically allocated for core dump
 *
 *  Arguments:
 *      card            Pointer to card struct
 *
 *  Returns:
 *      None
 *
 *  Notes:
 * ---------------------------------------------------------------------------
 */
void
unifi_coredump_free(card_t *card)
{
    void *ospriv = card->ospriv;
    coredump_buffer *node, *del_node;
    CsrInt16 i = 0;

    func_enter();
    unifi_trace(ospriv, UDBG2, "Core dump de-configured\n");

    if (card->dump_buf == NULL) {
        return;
    }

    node = card->dump_buf;
    do {
        /* Free payload */
        if (node->regs != NULL) {
            CsrMemFree(node->regs);
        }
        node->regs = NULL;

        del_node = node;
        node = node->next;

        /* Free header */
        CsrMemFree(del_node);
        i++;
    } while ((node != NULL) && (node != card->dump_buf));

    unifi_trace(ospriv, UDBG3, "Freed %d coredump buffers\n", i);

    card->dump_buf = NULL;
    card->dump_next_write = NULL;
    card->dump_cur_read = NULL;

    func_exit();
    return;

} /* unifi_coredump_free() */

