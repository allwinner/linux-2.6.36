/*
 * ---------------------------------------------------------------------------
 *
 *  FILE:     card_sdio.h
 *
 *  PURPOSE:
 *      Internal header for Card API for SDIO.
 *
 * Copyright (C) 2005-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
#ifndef __CARD_SDIO_H__
#define __CARD_SDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/unifi.h"
#include "driver/unifi_udi.h"
#include "unifihw.h"
#include "driver/unifiversion.h"
#include "ta_sampling.h"
#include "xbv.h"
#include "chiphelper.h"


/*
 *
 * Configuration items.
 * Which of these should go in a platform unifi_config.h file?
 *
 */

/* The number of traffic queues to provide. This is 4 for WME/WMM. */
#define UNIFI_WME_NO_OF_QS              4

/*
 * When the traffic queues contain more signals than there is space for on
 * UniFi, a limiting algorithm comes into play.
 * If a traffic queue has enough slots free to buffer more traffic from the
 * network stack, then the following check is applied. The number of free
 * slots is RESUME_XMIT_THRESHOLD.
 */
#define RESUME_XMIT_THRESHOLD           4


/*
 * When reading signals from UniFi, the host processes pending all signals
 * and then acknowledges them together in a single write to update the
 * to-host-chunks-read location.
 * When there is more than one bulk data transfer (e.g. one received data
 * packet and a request for the payload data of a transmitted packet), the
 * update can be delayed significantly. This ties up resources on chip.
 *
 * To remedy this problem, to-host-chunks-read is updated after processing
 * a signal if TO_HOST_FLUSH_THRESHOLD bytes of bulk data have been
 * transferred since the last update.
 */
#define TO_HOST_FLUSH_THRESHOLD (500*5)






/* SDIO Card Common Control Registers */
#define SDIO_CCCR_SDIO_REVISION     (0x00)
#define SDIO_SD_SPEC_REVISION       (0x01)
#define SDIO_IO_ENABLE              (0x02)
#define SDIO_IO_READY               (0x03)
#define SDIO_INT_ENABLE             (0x04)
#define SDIO_INT_PENDING            (0x05)
#define SDIO_IO_ABORT               (0x06)
#define SDIO_BUS_IFACE_CONTROL      (0x07)
#define SDIO_CARD_CAPABILOTY        (0x08)
#define SDIO_COMMON_CIS_POINTER     (0x09)
#define SDIO_BUS_SUSPEND            (0x0C)
#define SDIO_FUNCTION_SELECT        (0x0D)
#define SDIO_EXEC_FLAGS             (0x0E)
#define SDIO_READY_FLAGS            (0x0F)
#define SDIO_FN0_BLOCK_SIZE         (0x10)
#define SDIO_POWER_CONTROL          (0x12)
#define SDIO_VENDOR_START           (0xF0)

#define SDIO_CSR_HOST_WAKEUP        (0xf0)
#define SDIO_CSR_HOST_INT_CLEAR     (0xf1)
#define SDIO_CSR_FROM_HOST_SCRATCH0 (0xf2)
#define SDIO_CSR_FROM_HOST_SCRATCH1 (0xf3)
#define SDIO_CSR_TO_HOST_SCRATCH0   (0xf4)
#define SDIO_CSR_TO_HOST_SCRATCH1   (0xf5)
#define SDIO_CSR_FUNC_EN            (0xf6)
#define SDIO_CSR_CSPI_MODE          (0xf7)
#define SDIO_CSR_CSPI_STATUS        (0xf8)
#define SDIO_CSR_CSPI_PADDING       (0xf9)



#define UNIFI_SD_INT_ENABLE_IENM 0x0001    /* Master INT Enable */



/*
 * Structure to hold configuration information read from UniFi.
 */
typedef struct {

    /*
     * The version of the SDIO signal queues and bulk data pools
     * configuration structure. The MSB is the major version number, used to
     * indicate incompatible changes. The LSB gives the minor revision number,
     * used to indicate changes that maintain backwards compatibility.
     */
    CsrUint16 version;

    /*
     * offset from the start of the shared data memory to the SD IO
     * control structure.
     */
    CsrUint16 sdio_ctrl_offset;

    /* Buffer handle of the from-host signal queue */
    CsrUint16 fromhost_sigbuf_handle;

    /* Buffer handle of the to-host signal queue */
    CsrUint16 tohost_sigbuf_handle;

    /*
     * Maximum number of signal primitive or bulk data command fragments that may be
     * pending in the to-hw signal queue.
     */
    CsrUint16 num_fromhost_sig_frags;

    /*
     * Number of signal primitive or bulk data command fragments that must be pending
     * in the to-host signal queue before the host will generate an interrupt
     * to indicate that it has read a signal. This will usually be the total
     * capacity of the to-host signal buffer less the size of the largest signal
     * primitive divided by the signal primitive fragment size, but may be set
     * to 1 to request interrupts every time that the host read a signal.
     * Note that the hw may place more signals in the to-host signal queue
     * than indicated by this field.
     */
    CsrUint16 num_tohost_sig_frags;

    /*
     * Number of to-hw bulk data slots. Slots are numbered from 0 (zero) to
     * one less than the value in this field
     */
    CsrUint16 num_fromhost_data_slots;

    /*
     * Number of frm-hw bulk data slots. Slots are numbered from 0 (zero) to
     * one less than the value in this field
     */
    CsrUint16 num_tohost_data_slots;

    /*
     * Size of the bulk data slots (2 octets)
     * The size of the bulk data slots in octets. This will usually be
     * the size of the largest MSDU. The value should always be even.
     */
    CsrUint16 data_slot_size;

    /*
     * Indicates that the host has finished the initialisation sequence.
     * Initialised to 0x0000 by the firmware, and set to 0x0001 by us.
     */
    CsrUint16 initialised;

    /* Added by protocol version 0x0001 */
    CsrUint32 overlay_size;

    /* Added by protocol version 0x0300 */
    CsrUint16 data_slot_round;
    CsrUint16 sig_frag_size;

    /* Added by protocol version 0x0500 */
    CsrUint16 tohost_signal_padding;

} sdio_config_data_t;

/*
 * These values may change with versions of the Host Interface Protocol.
 */
/*
 * Size of config info block pointed to by the CSR_SLT_SDIO_SLOT_CONFIG
 * entry in the f/w symbol table
 */
#define SDIO_CONFIG_DATA_SIZE 30

/* Offset of the INIT flag in the config info block. */
#define SDIO_INIT_FLAG_OFFSET 0x12
#define SDIO_TO_HOST_SIG_PADDING_OFFSET 0x1C



/* Structure for a bulk data transfer command */
typedef struct {
    CsrUint16 cmd_and_len;   /* bits 12-15 cmd, bits 0-11 len */
    CsrUint16 data_slot;     /* slot number, perhaps OR'd with SLOT_DIR_TO_HOST */
    CsrUint16 offset;
    CsrUint16 buffer_handle;
} bulk_data_cmd_t;


/* Bulk Data signal command values */
#define SDIO_CMD_SIGNAL                 0x00
#define SDIO_CMD_TO_HOST_TRANSFER       0x01
#define SDIO_CMD_TO_HOST_TRANSFER_ACK   0x02 /*deprecated*/
#define SDIO_CMD_FROM_HOST_TRANSFER     0x03
#define SDIO_CMD_FROM_HOST_TRANSFER_ACK 0x04 /*deprecated*/
#define SDIO_CMD_CLEAR_SLOT             0x05
#define SDIO_CMD_OVERLAY_TRANSFER       0x06
#define SDIO_CMD_OVERLAY_TRANSFER_ACK   0x07 /*deprecated*/
#define SDIO_CMD_FROM_HOST_AND_CLEAR    0x08
#define SDIO_CMD_PADDING                0x0f

#define SLOT_DIR_TO_HOST 0x8000



static INLINE void unifi_init_bulk_data(bulk_data_desc_t *bulk_data_slot)
{
    bulk_data_slot->os_data_ptr = NULL;
    bulk_data_slot->data_length = 0;
    bulk_data_slot->os_net_buf_ptr = NULL;
    bulk_data_slot->net_buf_length = 0;
}

/*
 * Structure to contain a SIGNAL datagram.
 * This is used to build signal queues between the main driver and the
 * i/o thread.
 * The fields are:
 *      sigbuf          Contains the HIP signal is wire-format (i.e. packed,
 *                      little-endian)
 *      bulkdata        Contains a copy of any associated bulk data
 *      signal_length   The size of the signal in the sigbuf
 */
typedef struct card_signal {
    CsrUint8 sigbuf[UNIFI_PACKED_SIGBUF_SIZE];

    /* Length of the SIGNAL inside sigbuf */
    CsrUint16 signal_length;

    bulk_data_desc_t bulkdata[UNIFI_MAX_DATA_REFERENCES];

} card_signal_t;



/*
 * Control structure for a generic ring buffer.
 */
typedef struct {
    card_signal_t *q_body;

    /* Num elements in queue (capacity is one less than this!) */
    CsrUint16 q_length;

    CsrUint16 q_wr_ptr;
    CsrUint16 q_rd_ptr;

    CsrInt8 name[16];
} q_t;


#define RESERVED_COMMAND_SLOTS   2

/* Considering approx 500 us per packet giving 0.5 secs */
#define PACKETS_INTERVAL         1000

/*
 * Dynamic slot reservation for QoS
 */
typedef struct { 
    CsrUint16 from_host_used_slots[UNIFI_WME_NO_OF_QS];
    CsrUint16 from_host_max_slots[UNIFI_WME_NO_OF_QS];
    CsrUint16 from_host_reserved_slots[UNIFI_WME_NO_OF_QS];

    /* Parameters to determine if a queue was active. 
       If number of packets sent is greater than the threshold
       for the queue, the queue is considered active and no
       re reservation is done, it is important not to keep this
       value too low */
    /* Packets sent during this interval */
    CsrUint16 packets_txed[UNIFI_WME_NO_OF_QS];
    CsrUint16 total_packets_txed;

    /* Number of packets to see if slots need to be reassigned */
    CsrUint16 packets_interval;

    /* Once a queue reaches a stable state, avoid processing */
    CsrBool queue_stable[UNIFI_WME_NO_OF_QS];
} card_dynamic_slot_t;


/* These are type-safe and don't write incorrect values to the
 * structure. */
static INLINE CsrUint16 q_slots_used(const q_t *q)
{
    CsrInt16 t = q->q_wr_ptr - q->q_rd_ptr;
    if (t < 0)
    {
    t += q->q_length;
    ASSERT(t > 0);
    }
    return (CsrUint16)t;
}
static INLINE CsrUint16 q_slots_free(const q_t *q)
{
    return (q->q_length - q_slots_used(q)) - 1;
}
static INLINE card_signal_t *q_slot_data(const q_t *q, CsrUint16 slot)
{
    return q->q_body + slot;
}
static INLINE CsrUint16 q_next_r_slot(const q_t *q)
{
    return q->q_rd_ptr;
}
static INLINE CsrUint16 q_next_w_slot(const q_t *q)
{
    return q->q_wr_ptr;
}
static INLINE CsrUint16 q_wrap(const q_t *q, CsrUint16 x)
{
    if (x >= q->q_length)
    return x % q->q_length;
    else
    return x;
}
static INLINE void q_inc_r(q_t *q)
{
    q->q_rd_ptr = q_wrap(q, q->q_rd_ptr + 1);
}
static INLINE void q_inc_w(q_t *q)
{
    q->q_wr_ptr = q_wrap(q, q->q_wr_ptr + 1);
}



enum unifi_host_state {
    UNIFI_HOST_STATE_AWAKE   = 0,
    UNIFI_HOST_STATE_DROWSY  = 1,
    UNIFI_HOST_STATE_TORPID  = 2
};

typedef struct 
{
    bulk_data_desc_t bd;
    unifi_TrafficQueue queue; /* Used for dynamic slot reservation */
} slot_desc_t;

/*
 * Structure describing a UniFi SDIO card.
 */
struct card {

    /*
     * Back pointer for the higher level OS code. This is passed as
     * an argument to callbacks (e.g. for received data and indications).
     */
    void *ospriv;


    /* Info read from Symbol Table during probe */
    CsrUint32 build_id;
    CsrInt8 build_id_string[128];

    /* Retrieve from SDIO driver. */
    CsrUint16 chip_id;

    /* Read from GBL_CHIP_VERSION. */
    CsrUint16 chip_version;

    /* From the SDIO driver (probably 1) */
    CsrUint8 function;

    /* This is sused to get the register addresses and things. */
    ChipDescript *helper;

    /* What state is the hardware currently in? */
    /*HERE*//*enum hw_state hw_state;*/


    /*
     * Bit mask of PIOs for the loader to waggle during download.
     * We assume these are connected to LEDs. The main firmware gets
     * the mask from a MIB entry.
     */
    CsrInt32 loader_led_mask;

    /*
     * Support for flow control. When the from-host queue of signals
     * is full, we ask the host upper layer to stop sending packets. When
     * the queue drains we tell it that it can send packets again.
     * We use this flag to remember the current state.
     */
#define card_is_tx_q_paused(card, q)   (card->tx_q_paused_flag[q])
#define card_tx_q_unpause(card, q)   (card->tx_q_paused_flag[q] = 0)
#define card_tx_q_pause(card, q)   (card->tx_q_paused_flag[q] = 1)

    CsrUint16 tx_q_paused_flag[UNIFI_TRAFFIC_Q_MAX + 1 + UNIFI_WME_NO_OF_QS]; /* defensive more than big enough */

    /* UDI callback for logging UniFi interactions */
    udi_func_t udi_hook;

    CsrUint8 bh_reason_host;
    CsrUint8 bh_reason_unifi;

    /* SDIO clock speed request from OS layer */
    CsrUint8 request_max_clock;

    /* Last SDIO clock frequency set */
    CsrUint32 sdio_clock_speed;

    /*
     * Current host state (copy of value in IOABORT register and
     * spinlock to protect it.
     */
    enum unifi_host_state host_state;

    enum unifi_low_power_mode low_power_mode;
    enum unifi_periodic_wake_mode periodic_wake_mode;

    /*
     * Ring buffer of signal structs for a queue of data packets from
     * the host.
     * The queue is empty when fh_data_q_num_rd == fh_data_q_num_wr.
     * To add a packet to the queue, copy it to index given by
     * (fh_data_q_num_wr%UNIFI_SOFT_Q_LENGTH) and advance fh_data_q_num_wr.
     * To take a packet from the queue, copy data from index given by
     * (fh_data_q_num_rd%UNIFI_SOFT_Q_LENGTH) and advance fh_data_q_num_rd.
     * fh_data_q_num_rd and fh_data_q_num_rd are both modulo 256.
     */
    card_signal_t fh_command_q_body[UNIFI_SOFT_COMMAND_Q_LENGTH];
    q_t fh_command_queue;

    card_signal_t fh_traffic_q_body[UNIFI_WME_NO_OF_QS][UNIFI_SOFT_TRAFFIC_Q_LENGTH];
    q_t fh_traffic_queue[UNIFI_WME_NO_OF_QS];

    /*
     * Signal counts from UniFi SDIO Control Data Structure.
     * These are cached and synchronised with the UniFi before and after
     * a batch of operations.
     *
     * These are the modulo-256 count of signals written to or read from UniFi
     * The value is incremented for every signal.
     */
    CsrInt32 from_host_signals_w;
    CsrInt32 from_host_signals_r;
    CsrInt32 to_host_signals_r;
    CsrInt32 to_host_signals_w;


    /* Should specify buffer size as a number of signals */
    /*
     * Enough for 10 th and 10 fh data slots:
     *   1 * 10 * 8 =  80
     *   2 * 10 * 8 = 160
     */
#define UNIFI_FH_BUF_SIZE 1024
    struct sigbuf {
        CsrUint8 *buf;     /* buffer area */
        CsrUint8 *ptr;     /* current pos */
        CsrUint16 count;     /* signal count */
        CsrUint16 bufsize;
    } fh_buffer;
    struct sigbuf th_buffer;


    /*
     * Field to use for the incrementing value to write to the UniFi
     * SHARED_IO_INTERRUPT register.
     * Flag to say we need to generate an interrupt at end of processing.
     */
    CsrUint32 unifi_interrupt_seq;
    CsrUint8 generate_interrupt;


    /* Pointers to the bulk data slots */
    slot_desc_t *from_host_data;
    bulk_data_desc_t *to_host_data;


    /*
     * Index of the next (hopefully) free data slot.
     * This is an optimisation that starts searching at a more likely point
     * than the beginning.
     */
    CsrInt16 from_host_data_head;

    /* Dynamic slot allocation for queues */
    card_dynamic_slot_t dynamic_slot_data;

    /*
     * SDIO specific fields
     */

    /* Interface pointer for the SDIO library */
    CsrSdioFunction *sdio_if;

    /* Copy of config_data struct from the card */
    sdio_config_data_t config_data;

    /* SDIO address of the Initialised flag and Control Data struct */
    CsrUint32 init_flag_addr;
    CsrUint32 sdio_ctrl_addr;

    /* The last value written to the Shared Data Memory Page register */
    CsrUint32 proc_select;
    CsrUint32 dmem_page;
    CsrUint32 pmem_page;

    /* SDIO traffic counters limited to 32 bits for Synergy compatibility */
    CsrUint32 sdio_bytes_read;
    CsrUint32 sdio_bytes_written;

    CsrUint8 memory_resources_allocated;

    /* UniFi SDIO I/O Block size. */
    CsrUint16 sdio_io_block_size;

    /* Pad transfer sizes to SDIO block boundaries */
    CsrBool sdio_io_block_pad;

    /* Read from the XBV */
    struct FWOV fwov;


    /* TA sampling */
    ta_data_t ta_sampling;


    /* Auto-coredump */
    CsrInt16 request_coredump_on_reset;   /* request coredump on next reset */
    struct coredump_buf *dump_buf;        /* root node */
    struct coredump_buf *dump_next_write; /* node to fill at next dump */
    struct coredump_buf *dump_cur_read;   /* valid node to read, or NULL */

#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
    struct cmd_profile {
        unsigned int cmd52_count;
        unsigned int cmd53_count;
        unsigned int tx_count;
        unsigned int tx_cfm_count;
        unsigned int rx_count;
        unsigned int bh_count;
        unsigned int process_count;
        unsigned int protocol_count;
    } hip_prof;
    struct cmd_profile cmd_prof;
#endif

#ifdef CSR_WIFI_TRANSPORT_CSPI
    CsrUint32 cspi_cleanup_handle;
#endif

    /* Interrupt processing mode flags */
    CsrUint32 intmode;

}; /* struct card */

/*
 * Core dump
 */
/* Sizes */
/* Default no. of coredumps can be modified by the coredump_max module param */
#define HIP_NUM_COREDUMP_BUFFERS        0
#define HIP_COREDUMP_DATA_SIZE          (sizeof(struct regdump_buf))
#define HIP_COREDUMP_NUM_CPU            (2)
#define HIP_COREDUMP_NUM_CPU_REGS       (10)
#define HIP_COREDUMP_NUM_WORDS_LOW      (0x0100)
#define HIP_COREDUMP_NUM_WORDS_HIGH     (0x0400)
#define HIP_COREDUMP_NUM_WORDS_SHARED   (0x0500)
/* Addresses */
#define HIP_COREDUMP_FIRST_CPU_REG          (0xFFE0)
#define HIP_COREDUMP_FIRST_WORD_LOW         (0)
#define HIP_COREDUMP_FIRST_WORD_HIGH_MAC    (0x3C00)
#define HIP_COREDUMP_FIRST_WORD_HIGH_PHY    (0x1C00)
#define HIP_COREDUMP_FIRST_WORD_SHARED      (0)

typedef struct cpudump_buf {
    CsrUint16 regs[HIP_COREDUMP_NUM_CPU_REGS];
    CsrUint16 data_low[HIP_COREDUMP_NUM_WORDS_LOW];
    CsrUint16 data_high[HIP_COREDUMP_NUM_WORDS_HIGH];
} cpudump_buffer;

typedef struct regdump_buf {
    CsrUint16 drv_ver;
    CsrUint16 chip_ver;
    CsrUint16 fw_ver;
    CsrUint16 shared[HIP_COREDUMP_NUM_WORDS_SHARED];
    struct cpudump_buf cpu[HIP_COREDUMP_NUM_CPU];
} regdump_buffer;

typedef struct coredump_buf {
    CsrUint16 count;               /* serial number of dump */
    CSR_TIME timestamp;            /* host's system time at capture */
    CsrInt16 requestor;            /* requestor: 0=auto dump, 1=manual */
    struct regdump_buf *regs;      /* buffer of register values */
    struct coredump_buf *next;     /* circular list */
    struct coredump_buf *prev;     /* circular list */
} coredump_buffer;


/* This enum corresponds with unifiio_coredump_space_t,
 * defined in the Linux-only unifiio.h public header, and must stay in line.
 * Note that, crucially, these values are NOT the same as UNIFI_REGISTERS, etc
 * in unifihw.h which don't allow selection of register areas for each XAP
 */
typedef enum unifi_coredump_space {
    UNIFI_COREDUMP_MAC_REG,
    UNIFI_COREDUMP_PHY_REG,
    UNIFI_COREDUMP_SH_DMEM,
    UNIFI_COREDUMP_MAC_DMEM,
    UNIFI_COREDUMP_PHY_DMEM,
    UNIFI_COREDUMP_TRIGGER_MAGIC = 0xFEED
} unifi_coredump_space_t;

/* This structure corresponds with unifiio_coredump_req_t,
 * defined in the Linux-only unifiio.h public header.
 */
typedef struct unifi_coredump_req {
    /* From user */
    CsrInt32 index;                      /* 0=newest, -1=oldest */
    unifi_coredump_space_t space;   /* memory space */
    CsrUint32 offset;            /* register offset in space */
    /* From driver */
    CsrUint32 drv_build;         /* Driver build id */
    CsrUint32 chip_ver;          /* Chip version */
    CsrUint32 fw_ver;            /* Firmware version */
    CsrInt32 requestor;                  /* Requestor: 0=auto dump, 1=manual */
    CSR_TIME timestamp;             /* time of capture by driver */
    CsrUint32 serial;            /* capture serial number */
    CsrInt32 value;                      /* register value */
} unifi_coredump_req_t;             /* Core-dumped reg value request */


/* Reset types */
enum unifi_reset_type {
    UNIFI_COLD_RESET = 1,
    UNIFI_WARM_RESET = 2
};

/*
 * unifi_set_host_state() implements signalling for waking UniFi from
 * deep sleep. The host indicates to UniFi that it is in one of three states:
 *   Torpid - host has nothing to send, UniFi can go to sleep.
 *   Drowsy - host has data to send to UniFi. UniFi will respond with an
 *            SDIO interrupt. When hosts responds it moves to Awake.
 *   Awake  - host has data to transfer, UniFi must stay awake.
 *            When host has finished, it moves to Torpid.
 */
CsrInt32 unifi_set_host_state(card_t *card, enum unifi_host_state state);


CsrInt32 card_read_signal_counts(card_t *card);
bulk_data_desc_t *card_find_data_slot(card_t *card, CsrInt16 slot);


CsrInt32 unifi_set_proc_select(card_t *card, CsrInt16 select);

CsrInt32 unifi_read16(card_t *card, CsrUint32 unifi_addr, CsrUint16 *pdata);
CsrInt32 unifi_read32(card_t *card, CsrUint32 unifi_addr, CsrUint32 *pdata);
CsrInt32 unifi_readn(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len);
CsrInt32 unifi_readnz(card_t *card, CsrUint32 unifi_addr,
                      void *pdata, CsrUint16 len);
CsrInt32 unifi_read_shared_count(card_t *card, CsrUint32 addr);

CsrInt32 unifi_write16(card_t *card, CsrUint32 unifi_addr, CsrUint16 data);
CsrInt32 unifi_writen(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len);

CsrInt32 unifi_bulk_rw(card_t *card, CsrUint32 handle,
                       void *pdata, CsrUint32 len, CsrInt16 direction);
CsrInt32 unifi_bulk_rw_noretry(card_t *card, CsrUint32 handle,
                               void *pdata, CsrUint32 len, CsrInt16 direction);
#define UNIFI_SDIO_READ       0
#define UNIFI_SDIO_WRITE      1

CsrInt32 unifi_read_8_or_16(card_t *card, CsrUint32 unifi_addr, CsrUint8 *pdata);
CsrInt32 unifi_write_8_or_16(card_t *card, CsrUint32 unifi_addr, CsrUint8 data);
CsrInt32 unifi_read_direct_8_or_16(card_t *card, CsrUint32 addr, CsrUint8 *pdata);
CsrInt32 unifi_write_direct_8_or_16(card_t *card, CsrUint32 addr, CsrUint8 data);

CsrInt32 unifi_read_direct16(card_t *card, CsrUint32 addr, CsrUint16 *pdata);
CsrInt32 unifi_read_direct32(card_t *card, CsrUint32 addr, CsrUint32 *pdata);
CsrInt32 unifi_read_directn(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len);

CsrInt32 unifi_write_direct16(card_t *card, CsrUint32 addr, CsrUint16 data);
CsrInt32 unifi_write_directn(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len);

CsrInt32 sdio_read_f0(card_t *card, CsrUint32 addr, CsrUint8 *pdata);
CsrInt32 sdio_write_f0(card_t *card, CsrUint32 addr, CsrUint8 data);


/* For diagnostic use */
void dump(void *mem, CsrUint16 len);
void dump16(void *mem, CsrUint16 len);

#ifdef __cplusplus
}
#endif

#endif /* __CARD_SDIO_H__ */
