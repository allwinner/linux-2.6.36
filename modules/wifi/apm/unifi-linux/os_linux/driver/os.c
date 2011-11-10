/*
 * ---------------------------------------------------------------------------
 *  FILE:     os.c
 * 
 *  PURPOSE:
 *      Routines to fulfil the OS-abstraction for the HIP lib.
 *      It is part of the porting exercise.
 *
 * Copyright (C) 2005-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

/**
 * The HIP lib OS abstraction consists of the implementation
 * of the functions in this file. It is part of the porting exercise.
 */

#include "unifi_priv.h"


/*
 * ---------------------------------------------------------------------------
 *  unifi_net_data_malloc
 *
 *      Allocate an OS specific net data buffer of "size" bytes.
 *      The bulk_data_slot.os_data_ptr must be initialised to point
 *      to the buffer allocated. The bulk_data_slot.length must be 
 *      initialised to the requested size, zero otherwise.
 *      The bulk_data_slot.os_net_buf_ptr can be initialised to
 *      an OS specific pointer to be used in the unifi_net_data_free().
 *
 * 
 *  Arguments:
 *      ospriv              Pointer to device private context struct.
 *      bulk_data_slot      Pointer to the bulk data structure to initialise.
 *      size                Size of the buffer to be allocated.
 *
 *  Returns:
 *      0 on success, -1 otherwise.
 * ---------------------------------------------------------------------------
 */
int
unifi_net_data_malloc(void *ospriv, bulk_data_desc_t *bulk_data_slot, unsigned int size)
{
    struct sk_buff *skb;
    unifi_priv_t *priv = (unifi_priv_t*)ospriv;
    int rounded_length;

    if (priv->card_info.sdio_block_size == 0) {
        unifi_error(priv, "unifi_net_data_malloc: Invalid SDIO block size\n");
        return -1;
    }

    rounded_length = (size + priv->card_info.sdio_block_size - 1) & ~(priv->card_info.sdio_block_size - 1);

    skb = dev_alloc_skb(rounded_length + 2 + ETH_HLEN);
    if (! skb) {
        unifi_error(ospriv, "alloc_skb failed.\n");
        bulk_data_slot->os_net_buf_ptr = NULL;
        bulk_data_slot->net_buf_length = 0;
        bulk_data_slot->os_data_ptr = NULL;
        bulk_data_slot->data_length = 0;
        return -1;
    }

    bulk_data_slot->os_net_buf_ptr = (const unsigned char*)skb;
    bulk_data_slot->net_buf_length = rounded_length + 2 + ETH_HLEN;
    bulk_data_slot->os_data_ptr = (const void*)skb->data;
    bulk_data_slot->data_length = size;

    return 0;
} /* unifi_net_data_malloc() */


/*
 * ---------------------------------------------------------------------------
 *  unifi_net_data_free
 *
 *      Free an OS specific net data buffer.
 *      The bulk_data_slot.length must be initialised to 0.
 *
 * 
 *  Arguments:
 *      ospriv              Pointer to device private context struct.
 *      bulk_data_slot      Pointer to the bulk data structure that 
 *                          holds the data to be freed.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
unifi_net_data_free(void *ospriv, bulk_data_desc_t *bulk_data_slot)
{
    struct sk_buff *skb;
    CSR_UNUSED(ospriv);

    skb = (struct sk_buff *)bulk_data_slot->os_net_buf_ptr;
    dev_kfree_skb(skb);

    bulk_data_slot->net_buf_length = 0;
    bulk_data_slot->data_length = 0;
    bulk_data_slot->os_data_ptr = bulk_data_slot->os_net_buf_ptr = NULL;

} /* unifi_net_data_free() */


/* Module parameters */
extern int unifi_debug;

#ifdef UNIFI_DEBUG
#define DEBUG_BUFFER_SIZE       80

#define FORMAT_TRACE(_s, _len, _args, _fmt)             \
    do {                                                \
        va_start(_args, _fmt);                          \
        _len += vsnprintf(&(_s)[_len],                  \
                         (DEBUG_BUFFER_SIZE - _len),    \
                         _fmt, _args);                  \
        va_end(_args);                                  \
        if (_len >= DEBUG_BUFFER_SIZE) {                \
            (_s)[DEBUG_BUFFER_SIZE - 2] = '\n';         \
            (_s)[DEBUG_BUFFER_SIZE - 1] = 0;            \
        }                                               \
    } while (0)
#endif /* UNIFI_DEBUG */

void
unifi_error(void* ospriv, const char *fmt, ...)
{
#ifdef UNIFI_DEBUG
    unifi_priv_t *priv = (unifi_priv_t*) ospriv;
    static char s[DEBUG_BUFFER_SIZE];
    va_list args;
    unsigned int len;

    if (priv != NULL) {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_ERR "unifi%d: ", priv->instance);
    } else {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_ERR "unifi: ");
    }

    FORMAT_TRACE(s, len, args, fmt);

    printk("%s", s);
#endif /* UNIFI_DEBUG */
}

void
unifi_warning(void* ospriv, const char *fmt, ...)
{
#ifdef UNIFI_DEBUG
    unifi_priv_t *priv = (unifi_priv_t*) ospriv;
    static char s[DEBUG_BUFFER_SIZE];
    va_list args;
    unsigned int len;

    if (priv != NULL) {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_WARNING "unifi%d: ", priv->instance);
    } else {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_WARNING "unifi: ");
    }

    FORMAT_TRACE(s, len, args, fmt);

    printk("%s", s);
#endif /* UNIFI_DEBUG */
}


void
unifi_notice(void* ospriv, const char *fmt, ...)
{
#ifdef UNIFI_DEBUG
    unifi_priv_t *priv = (unifi_priv_t*) ospriv;
    static char s[DEBUG_BUFFER_SIZE];
    va_list args;
    unsigned int len;

    if (priv != NULL) {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_NOTICE "unifi%d: ", priv->instance);
    } else {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_NOTICE "unifi: ");
    }

    FORMAT_TRACE(s, len, args, fmt);

    printk("%s", s);
#endif /* UNIFI_DEBUG */
}


void
unifi_info(void* ospriv, const char *fmt, ...)
{
#ifdef UNIFI_DEBUG
    unifi_priv_t *priv = (unifi_priv_t*) ospriv;
    static char s[DEBUG_BUFFER_SIZE];
    va_list args;
    unsigned int len;

    if (priv != NULL) {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_INFO "unifi%d: ", priv->instance);
    } else {
        len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_INFO "unifi: ");
    }

    FORMAT_TRACE(s, len, args, fmt);

    printk("%s", s);
#endif /* UNIFI_DEBUG */
}

/* debugging */
void
unifi_trace(void* ospriv, int level, const char *fmt, ...)
{
#ifdef UNIFI_DEBUG
    unifi_priv_t *priv = (unifi_priv_t*) ospriv;
    static char s[DEBUG_BUFFER_SIZE];
    va_list args;
    unsigned int len;

    if (unifi_debug >= level) {
        if (priv != NULL) {
            len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_ERR "unifi%d: ", priv->instance);
        } else {
            len = snprintf(s, DEBUG_BUFFER_SIZE, KERN_ERR "unifi: ");
        }

        FORMAT_TRACE(s, len, args, fmt);

        printk("%s", s);
    }
#endif /* UNIFI_DEBUG */
}


/*
 * ---------------------------------------------------------------------------
 *
 *      Debugging support.
 *
 * ---------------------------------------------------------------------------
 */

#ifdef UNIFI_DEBUG
void
dump(void *mem, CsrUint16 len)
{
    int i, col = 0;
    unsigned char *pdata = (unsigned char *)mem;

    for (i = 0; i < len; i++) {
        if (col == 0) {
            printk("0x%02X: ", i);
        }

        printk(" %02X", pdata[i]);

        if (++col == 16) {      
            printk("\n");
            col = 0;
        }
    }
    if (col) {      
        printk("\n");
    }
} /* dump() */


void
dump16(void *mem, CsrUint16 len)
{
    int i, col=0;
    unsigned short *p = (unsigned short *)mem;

    for (i = 0; i < len; i+=2) {
        if (col == 0) {
            printk("0x%02X: ", i);
        }

        printk(" %04X", *p++);

        if (++col == 8) {
            printk("\n");
            col = 0;
        }
    }
    if (col) {
        printk("\n");
    }
}


#ifdef CSR_WIFI_HIP_DEBUG_OFFLINE
void
dump_str(void *mem, CsrUint16 len)
{
    int i, col = 0;
    unsigned char *pdata = (unsigned char *)mem;

    for (i = 0; i < len; i++) {
        printk("%c", pdata[i]);
    }
    if (col) {
        printk("\n");
    }

} /* dump_str() */
#endif /* CSR_ONLY_NOTES */

#endif /* UNIFI_DEBUG */


/* ---------------------------------------------------------------------------
 *                              - End -
 * ------------------------------------------------------------------------- */
