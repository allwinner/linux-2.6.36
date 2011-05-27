/*
 * $Id: sdio.c 9590 2008-07-25 09:08:01Z phth $
 */


/*! @defgroup sdio_generic Generic SDIO driver implementation
 *
 * @brief Generic SDIO driver implementation
 *
 * This file implements the generic SDIO driver where the general functionality
 * is implemented.
 *
 * The generic SDIO driver does not make any assumptions on the specific
 * SDIO host that is used, those low level operations are captured in the
 * SDIO host interface (host.h).
 *
 * Basically data transmission and reception is implemented. Hooks for firmware
 * download and misc functionality such as power management is implemented.
 *
 *  @{
 */

/******************************************************************************
I N C L U D E S
******************************************************************************/
#include <linux/types.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,41)
#include <linux/tqueue.h>
#else
#include <linux/workqueue.h>
#endif

#include <linux/mm.h>
#include <asm/semaphore.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include "host.h"

#ifdef USE_WIFI
#include <transport.h>
#endif /* USE_WIFI */

#include <linux/proc_fs.h>
#include <nanoutil.h>

MODULE_LICENSE("GPL");

/******************************************************************************
D E L A Y   F U N C T I O N A L I T Y
Can be used with preemptive & non-preemptive kernels
******************************************************************************/
#ifdef USE_NO_MSLEEP
/*! Implement our own msleep() instead of using kernel's */
void msleep(unsigned int msecs)
{
	unsigned long jiffies_ahead = (HZ * msecs + 999) / 1000;
	unsigned long timeout = jiffies + jiffies_ahead;
	while (time_before(jiffies, timeout))
		schedule();
}
#endif /* USE_NO_MSLEEP */

/*!
 * Using SAFE_DELAY() instead of <b>mdelay()</b>
 * ensures correct operation when running over preemptive kernel.
 */
#ifdef CONFIG_PREEMPT
#define SAFE_DELAY(msec) { mdelay(msec); }
#else
#define SAFE_DELAY(msec) { msleep(msec); }
#endif


/******************************************************************************
S D I O   C L O C K   T O G G L I N G
******************************************************************************/
#ifdef USE_NO_CLOCK
#define SET_SDIO_CLOCK_OFF()  host_clock(CLK_MODE_OFF)
#define SET_SDIO_CLOCK_SLOW() host_clock(CLK_MODE_SLOW)
#define SET_SDIO_CLOCK_FAST() host_clock(CLK_MODE_FAST)
#else
#define SET_SDIO_CLOCK_OFF() 
#define SET_SDIO_CLOCK_SLOW()
#define SET_SDIO_CLOCK_FAST() 
#endif

/******************************************************************************
G P I O   C P U   L O A D   M E A S U R E
******************************************************************************/
#ifdef USE_GPIO_CPU_MEASURE
static volatile long gpio_measure_status = 0;

#define GPIO_START_MEASURE() {                                       \
        if (++gpio_measure_status == 1)                              \
                host_gpio(GPIO_OP_HIGH, GPIO_ID_CPU_LOAD);           \
}

#define GPIO_STOP_MEASURE() {                                        \
        if (--gpio_measure_status <= 0)                              \
                host_gpio(GPIO_OP_LOW, GPIO_ID_CPU_LOAD);            \
}
#else
#define GPIO_START_MEASURE()
#define GPIO_STOP_MEASURE()
#endif



/******************************************************************************
D E F I N E S
******************************************************************************/
#define XMAC_RESET	              0x8801ea00
#define XMAC_ENABLE_SLEEP         0x8801e200
#define XMAC_DISABLE_SLEEP        0x8801e201
#define XMAC_NO_CLOCK_SDIO_IRQ    0x8801e401
#define XMAC_ENABLE_SDIO_IRQ      0x88000803
#define XMAC_ACKNOWLEDGE_SDIO_IRQ 0x8801E002
#define XMAC_ENABLE_SDIO_4BIT_MODE 0x88000E02

#define XMAC_MIN_SIZE       32
#define XMAC_ALIGNMENT      16
#define XMAC_HEADER_SIZE	18
#define XMAC_LEN_FIELD_SIZE 2
#define MAX_PACKET_SIZE     2048
#define DMA_ALIGNMENT       4

#define SDIO_IO_R_ARG_BASE  0x10000000
#define SDIO_IO_W_ARG_BASE  0x90000000

/******************************************************************************
G L O B A L S
******************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define NRX_MODULE_PARM(N, T1, T2, M) MODULE_PARM(N, T2)
#else
#define NRX_MODULE_PARM(N, T1, T2, M) module_param(N, T1, M)
#endif

/*! Module param nrx_fw_download enables firmware download if non-zero */
#ifdef USE_FW_DOWNLOAD
static int nrx_fw_download = 1;
NRX_MODULE_PARM(nrx_fw_download, bool, "i", S_IRUGO);
MODULE_PARM_DESC(nrx_fw_download, "NRX download firmware");
#else /* !USE_FW_DOWNLOAD */
static int nrx_fw_download = 0;
#endif /* USE_FW_DOWNLOAD */

/*! Module param nrx_4bit_sdio enables 4-bit SDIO operation if non-zero */
#ifdef USE_4BIT_SDIO
static int nrx_4bit_sdio = 1;
NRX_MODULE_PARM(nrx_4bit_sdio, bool, "i", S_IRUGO);
MODULE_PARM_DESC(nrx_4bit_sdio, "NRX use 4-bit SDIO");
#else /* !USE_4BIT_SDIO */
static int nrx_4bit_sdio = 0;
#endif /* USE_4BIT_SDIO */

/*!
 * Module param nrx_debug normally belongs to nano_if module.
 * Defined here only when WiFiEngine is not present.
 */
#ifndef USE_WIFI
unsigned int nrx_debug = (unsigned int) -1;
NRX_MODULE_PARM(nrx_debug, int, "i", S_IRUGO);
#else
extern unsigned int nrx_debug;
#endif /* !USE_WIFI */


/*!
 * @brief Used to signal host controller events (IRQs).
 *
 * See also event_init(), event_wait(), event_busywait() and event_fire().
 *
 */
struct sdio_event_t {
	
	irq_status_t irq_status; /*!< IRQ status on last event occurrence */
	atomic_t is_consumed;	 /*!< is latest status consumed? */
	struct semaphore sema;	 /*!< released on event occurrence */
};

/*! @brief Device driver state */
struct sdio_dev_t {

	/*! Is WiFiEngine shutting down target? */
	atomic_t shutdown_flag;

#ifdef USE_IF_REINIT
	/*! Should SDIO interface be reinitialized? */
	atomic_t if_reinit_flag;
#endif

	/*! Used by sdio_interrupt() to call host_interrupt()
	 *  with interrupts locked out.
	 */
	spinlock_t interrupt_lock;

	/*! Driver's network iface */
	struct net_device *iface;

	/*! Mutual exclusion between target power control vs.
	 *  send and receive activities on SDIO bus.
	 */
	struct semaphore power_ctl_sem;

	/*! Serializes commands on SDIO bus.
	 *  Should be acquired only after #power_ctl_sem has been acquired.
	 */
	struct semaphore sdio_mutex;

	/*! Signals FIFO transfer completion, see sdio_fifo_callback() */
	struct semaphore fifo_sem;

#ifdef USE_WIFI
	spinlock_t send_lock;				/*!< Used by wifi_send() */
	struct sk_buff_head skb_rx_head;	/*!< RX queue */
	struct sk_buff_head skb_tx_head;	/*!< TX queue */
#endif /* USE_WIFI */

#ifdef USE_IRQ_ID_RESPONSE_COMPLETION
	/*! Signals #IRQ_ID_RESPONSE_COMPLETION from host controller */
	struct sdio_event_t resp_comp;
#endif

#ifdef USE_IRQ_ID_RW_ACCESS_COMPLETION
	/*! Signals #IRQ_ID_RW_ACCESS_COMPLETION from host controller */
	struct sdio_event_t rw_comp;
#endif

#ifdef USE_IRQ_ID_FIFO_READ_READY
	/*! Signals #IRQ_ID_FIFO_READ_READY from host controller */
	struct sdio_event_t rd_fifo_rdy;
#endif

#ifdef USE_IRQ_ID_FIFO_WRITE_READY
	/*! Signals #IRQ_ID_FIFO_WRITE_READY from host controller */
	struct sdio_event_t wr_fifo_rdy;
#endif

	/*! Is target booting? */
	int is_booting;

	/*! for /proc/nanosdio entry */
	struct proc_dir_entry *proc_dir;

	/*! Signals shutdown completion to sdio_exit() */
	struct semaphore exit_sem;
};

/*! @brief Counts important events during transport operation. */
struct sdio_statistics_t
{
	unsigned irq_response_complete;		/*!< IRQs for command response */
	unsigned irq_rw_access_complete;	/*!< IRQs for CMD53 data completion */
	unsigned irq_fifo_read_ready;		/*!< ready IRQs from read fifo */
	unsigned irq_fifo_write_ready;		/*!< ready IRQs from write fifo */
	unsigned irq_target;				/*!< IRQs from target */
	unsigned tx_pkts;		/*!< packets transmitted */
	unsigned rx_pkts;		/*!< packets received */
	unsigned tx_fail;		/*!< packets failed to transmit */
	unsigned rx_fail;		/*!< packets failed to receive */
	unsigned wifi_send;		/*!< calls to wifi_send() */
	unsigned noise_pkts;	/*!< spurious receptions */
	unsigned sleep;			/*!< sleep commands to target */
	unsigned wakeup;		/*!< wakeup commands to target */
};

static struct sdio_dev_t sdio_dev;			/*!< See #sdio_dev_t */
static struct sdio_statistics_t statistics;	/*!< See #sdio_statistics_t */
#ifdef USE_WIFI
/*! network device creation parameters */
static struct nanonet_create_param create_param;
#endif



/******************************************************************************
W O R K Q U E U E S
******************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,41)
#define NRX_WORK_STRUCT tq_struct
#define NRX_INIT_WORK(T, F, P) INIT_TQUEUE(T, F, P)
#define NRX_SCHEDULE_WORK(T) schedule_task(T)
#define NRX_FLUSH_SCHEDULED_WORK() flush_scheduled_tasks()
typedef void nrx_work_data_t;
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
#define NRX_WORK_STRUCT work_struct
#define NRX_SCHEDULE_WORK(T) schedule_work(T)
#define NRX_FLUSH_SCHEDULED_WORK() flush_scheduled_work()
#define NRX_INIT_WORK(T,F,P) INIT_WORK(T,F)
typedef struct work_struct nrx_work_data_t;
#else
#define NRX_WORK_STRUCT work_struct
#define NRX_INIT_WORK(T, F, P) INIT_WORK(T, F, P)
#define NRX_SCHEDULE_WORK(T) schedule_work(T)
#define NRX_FLUSH_SCHEDULED_WORK() flush_scheduled_work()
typedef void nrx_work_data_t;
#endif

struct NRX_WORK_STRUCT sdio_rx_task; /*!< see sdio_recv() */

#ifdef USE_WIFI
struct NRX_WORK_STRUCT tx_task;	/*!< see do_tx_queue() */
struct NRX_WORK_STRUCT rx_task;	/*!< see do_rx_queue() */
#endif /* USE_WIFI */

#ifdef USE_POWER_MANAGEMENT
struct NRX_WORK_STRUCT sleep_task;	/*!< see do_sleep() */
struct NRX_WORK_STRUCT wakeup_task;	/*!< see do_wakeup() */
#endif /* USE_POWER_MANAGEMENT */

#ifdef USE_IF_REINIT
struct NRX_WORK_STRUCT if_reinit_task;	/*!< see do_if_reinit() */
#endif /* USE_IF_REINIT */


/******************************************************************************
F U N C T I O N   P R O T O T Y P E S
******************************************************************************/
int __init sdio_init(void);
void __exit sdio_exit(void);
static int sdio_read_proc(char* buf, char** start, off_t offset,
						  int count, int* eof, void* data);

static int sdio_reset_host(void);
static int sdio_reset_target(void);
static int sdio_init_target(void);
static void sdio_shutdown_target(void);

static irq_status_t sdio_interrupt(irq_op_t irq_op, irq_id_t irq_id);
static int sdio_control_interrupts(irq_op_t irq_op, int include_target_irq);
static int sdio_init_target_interrupt(void);

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void sdio_irq (int irq, void *dev_id, struct pt_regs *regs);
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
static irqreturn_t sdio_irq (int irq, void *dev_id);
#else /* LINUX_VERSION_CODE from (2,6,0) to (2,6,19) */
static irqreturn_t sdio_irq (int irq, void *dev_id, struct pt_regs *regs);
#endif /* LINUX_VERSION_CODE */

static int  sdio_cmd(uint8_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
                     uint32_t flags, uint32_t arg, void* data);
static void sdio_send(struct sk_buff *skb);
static int  sdio_send_single(void* buffer, uint32_t len);
static void sdio_recv(nrx_work_data_t* dummy);
static void sdio_recv_single(void* buffer, unsigned int len);
void sdio_fifo_callback(void);

#ifdef USE_WIFI
static int sdio_control(uint32_t command, uint32_t mode, void *data);
static int sdio_control_sleep(uint32_t mode, void *data);
static int sdio_control_boot(uint32_t mode, void *data);
static int sdio_fw_download(const void *fw_buf, size_t fw_len, void *data);
static void do_tx_queue(nrx_work_data_t *data);
static void do_rx_queue(nrx_work_data_t *data);
static int wifi_send(struct sk_buff *skb, void *data);
#else /* !USE_WIFI */
static int dump_bin_to_file(const char *fname, const void *data,
							size_t len) __attribute__ ((unused));
static int get_bin_from_file(const char *fname, void *data, size_t *len);
static int do_test(void* dummy);
static void do_test_rx(struct sk_buff *skb);
#endif /* USE_WIFI */

#ifdef USE_IF_REINIT
static int if_reinit(void);
static void do_if_reinit(void *data);
#endif /* USE_IF_REINIT */

#ifdef USE_POWER_MANAGEMENT
static void do_sleep(nrx_work_data_t *data);
static void do_wakeup(nrx_work_data_t *data);
#endif /* USE_POWER_MANAGEMENT */

/* If host_get_target_params() is not defined in host.c,
 * create a dummy one here.
 */
#ifndef USE_TARGET_PARAMS
inline size_t host_get_target_params(const void** params_buf)
{
	params_buf = NULL;
	return 0;
}
#endif /* !USE_TARGET_PARAMS */



/******************************************************************************
F U N C T I O N   D E F I N I T I O N S  ( sdio_event_t handling )
******************************************************************************/
#define UNUSED __attribute__ ((unused))
static void event_init(struct sdio_event_t *pevt) UNUSED;
static irq_status_t event_wait(struct sdio_event_t *pevt) UNUSED;
static irq_status_t event_busywait(struct sdio_event_t *pevt,
								  uint32_t attempts) UNUSED;
static void event_fire(struct sdio_event_t *pevt,
					   irq_status_t irq_status) UNUSED;
#undef UNUSED

/*!
 * @brief Initializes an event.
 *
 * @param pevt Pointer to event to be initialized.
 *
 */
static void event_init(struct sdio_event_t *pevt)
{
	pevt->irq_status = IRQ_STATUS_INACTIVE;
	atomic_set(&pevt->is_consumed, 1);
	sema_init(&pevt->sema, 0);
}

/*!
 * @brief Sleeps until an event occurs.
 *
 * @param pevt Pointer to event to wait on.
 *
 * @return
 * 	- On sucess, IRQ status when event occured.
 * 	- On interruption, #IRQ_STATUS_FAIL.
 */
static irq_status_t event_wait(struct sdio_event_t *pevt)
{
	irq_status_t irq_status;

	if (down_interruptible(&pevt->sema))
		return IRQ_STATUS_FAIL;

	irq_status = pevt->irq_status;
	atomic_inc(&pevt->is_consumed);
	return irq_status;
}

/*!
 * @brief Busy wait until an event occurs. Does not sleep.
 *
 * @param pevt 		Pointer to event to wait on.
 * @param attempts	Max number of loops spend on busy wait.
 *
 * @return
 *	- On sucess, IRQ status when event occured.
 *	- On interruption or timeout, #IRQ_STATUS_FAIL.
 */
static irq_status_t event_busywait(struct sdio_event_t *pevt,
								   uint32_t attempts)
{
	irq_status_t irq_status;
	int sema_lock_failed = 1;
	
	for (; attempts > 0; attempts--) {
		sema_lock_failed = down_trylock(&pevt->sema);
		if (!sema_lock_failed)
			break;
	}
	
	if (sema_lock_failed)
		return IRQ_STATUS_FAIL;
	
	irq_status = pevt->irq_status;
	atomic_inc(&pevt->is_consumed);
	return irq_status;
}

/*!
 * @brief Signals that an event occured.
 *
 * @param pevt 			Pointer to event.
 * @param irq_status	IRQ status when event occured.
 */
static void event_fire(struct sdio_event_t *pevt, irq_status_t irq_status)
{
	int is_not_consumed;

	/* If previous event has not been consumed, panic! */
	is_not_consumed = atomic_add_negative(-1, &pevt->is_consumed);
	BUG_ON(is_not_consumed);

	pevt->irq_status = irq_status;
	up(&pevt->sema);
}


/******************************************************************************
F U N C T I O N   D E F I N I T I O N S  ( T X )
******************************************************************************/
#ifdef USE_WIFI
/*!
 * @brief Top-level transmit function that is registered with the upper layers.
 *
 * Transmissions are issued from the upper layers, i.e. the WiFiEngine.
 * Upon transmission, wifi_send() will be called. The responsibility of
 * wifi_send() is to transfer the data to the target. The actual data transfer
 * is partitioned into three functions; wifi_send(), sdio_send() and
 * sdio_send_single().
 *
 * wifi_send() will put the data (<b>skb</b>) to be sent in a queue and return
 * immediately. A Linux workqueue is scheduled to handle the packet in process
 * context.
 *
 * wifi_send() can be called in interrupt context or in process context and
 * can also be called concurrently. Therefore the wifi_send() will not block
 * and its body is concurrency protected using sdio_dev_t::send_lock.
 *
 * @param skb The socket buffer to transmit
 * @param data Optional parameter (not used)
 *
 * @return 0
 */
static int wifi_send(struct sk_buff *skb, void *data)
{
	unsigned long flags;
	spin_lock_irqsave(&sdio_dev.send_lock, flags);

	statistics.wifi_send++;

	GPIO_START_MEASURE();
	TRSP_ASSERT((unsigned long) skb->data % DMA_ALIGNMENT == 0);
	skb_queue_tail(&sdio_dev.skb_tx_head, skb);
	if(atomic_read(&sdio_dev.shutdown_flag) == 0)
		NRX_SCHEDULE_WORK(&tx_task);
	GPIO_STOP_MEASURE();

	spin_unlock_irqrestore(&sdio_dev.send_lock,flags);
	return 0;
}


/*!
 * @brief Process the list of packets queued by wifi_send().
 *
 * Sends each packet using sdio_send(). Executed by the workqueue #tx_task.
 *
 * @param data Optional parameter (not used)
 */
static void do_tx_queue(nrx_work_data_t *data)
{
	struct sk_buff *skb;

	GPIO_START_MEASURE();
	while((skb = skb_dequeue(&sdio_dev.skb_tx_head)) != NULL)
		sdio_send(skb);
	GPIO_STOP_MEASURE();
}
#endif /* USE_WIFI */



/*!
 * @brief Sends a packet.
 *
 * This function will be executed by the workqueue #rx_task that was scheduled by
 * wifi_send() when a packet has been dequeued. 
 *
 * sdio_send() will fragmented the data (<b>skb</b>) according to the host
 * platform max SDIO transfer size #MAX_TRANSFER_SIZE. Each fragment will
 * be sent using sdio_send_single().
 *
 * The SDIO clock will be switched on during data transmission.
 *
 * @param skb The socket buffer to transmit
 */
static void sdio_send(struct sk_buff *skb)
{
	size_t transfer_len, remaining_len;
	size_t send_len = 0;
	int status;

	KDEBUG(TRANSPORT, "ENTER: skb->len=%d", skb->len);

	TRSP_ASSERT(!in_interrupt());
	TRSP_ASSERT(skb->len <= MAX_PACKET_SIZE);
	TRSP_ASSERT((unsigned long) skb->data % DMA_ALIGNMENT == 0);
	KDEBUG_BUF(TRANSPORT, skb->data, skb->len, "SDIO TX data:");

	/* Sync with power control */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.power_ctl_sem);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	remaining_len = skb->len;
	while(remaining_len) {
			
		/* How much data should be transferred in this chunk? */
		if(remaining_len > MAX_TRANSFER_SIZE) 
			transfer_len = MAX_TRANSFER_SIZE;
		else 
			transfer_len = remaining_len;

		TRSP_ASSERT(remaining_len >= transfer_len);
		send_len += sdio_send_single(skb->data + send_len,
									 transfer_len);
		remaining_len -= transfer_len;
	}

	up(&sdio_dev.power_ctl_sem);
	dev_kfree_skb(skb);
	statistics.tx_pkts++;

	KDEBUG(TRANSPORT, "EXIT");
}


/*!
 * @brief Sends a fragment of a packet with SDIO CMD53.
 *
 * This is where the actual transmission on the SDIO bus takes places.
 * sdio_send_single() sends a fragment of data (<b>buffer</b>) using either
 * CPU or DMA to fill the SDIO FIFO (this decision is made be the particular
 * host controller implementation).
 *
 * Prior to filling the FIFO, a SDIO CMD53 for writing is issued.
 *
 * sdio_send_single() relies on up to two interupts and one callback function
 * from the SDIO host controller. First of all the FIFO must be ready for
 * writing, this can be ensured by using #IRQ_ID_FIFO_WRITE_READY.
 * On some hosts this condition can be guaranteed without using a specific
 * interrupt so this interrupt is optional.
 *
 * When the data transfer is completed (e.g. the FIFO is filled)
 * sdio_send_single() expects that the sdio_fifo_callback() will be called.
 *
 * To make sure that all data is transmitted to the target and the SDIO bus
 * is free to use again the optional interrupt #IRQ_ID_RW_ACCESS_COMPLETION
 * can be used.
 *
 * See interrupt explanations in host.h for more information.
 * 
 * Since the sdio_send_single() requires exclusive access to the SDIO bus,
 * the body of sdio_send_single() is concurrency protected by
 * sdio_dev_t::sdio_mutex.
 *
 * @param buffer The buffer to transmit
 * @param len The length of the buffer
 *
 * @return The number of bytes that have been transferred
 */
static int sdio_send_single(void* buffer, uint32_t len)
{
	int status;
	irq_status_t irq_status __attribute__ ((unused));
	unsigned long buffer_phy = 0;

	TRSP_ASSERT(buffer);

	KDEBUG(TRANSPORT, "ENTER: len = %d", len);

	 /* Lock sdio */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.sdio_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

#ifdef USE_IF_REINIT
	/* The body of if_reinit will check whether re-init is really needed
	 * or not.
	 */
	(void) if_reinit();
#endif

	SET_SDIO_CLOCK_FAST ();

	/* Set up write command */
	host_control(CTL_ID_LENGTH, len);
	sdio_cmd(SDIO_RW_EXTENDED, CMD_TYPE_ADTC, CMD_RESP_R5,
			 CMD_FLAG_DATA_EXISTS, SDIO_IO_W_ARG_BASE + (len % 512), 
			 buffer);

#ifdef USE_IRQ_ID_FIFO_WRITE_READY
	/* Wait for access to FIFO */
	KDEBUG(TRANSPORT, "Waiting for access to FIFO");
	GPIO_STOP_MEASURE();
	irq_status = event_wait(&sdio_dev.wr_fifo_rdy);
	GPIO_START_MEASURE();
	TRSP_ASSERT(irq_status == IRQ_STATUS_ACTIVE);
#endif


#ifndef USE_NO_CACHE_COHERENCY
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	consistent_sync(buffer, len, PCI_DMA_TODEVICE);
	buffer_phy = (unsigned long) virt_to_bus(buffer);
#else
	/* TODO: Add a proper device structure */
	buffer_phy = (unsigned long) dma_map_single(NULL, buffer, len,
												DMA_TO_DEVICE);
#endif /* LINUX_VERSION_CODE */
#endif /* !USE_NO_CACHE_COHERENCY */
	host_fifo(buffer, buffer_phy, len, 0);

	KDEBUG(TRANSPORT, "Waiting for DMA callback");
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.fifo_sem);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

#ifdef USE_IRQ_ID_RW_ACCESS_COMPLETION
	/* Wait until R/W access completion, i.e I/O fifo data has been
	 * transmitted.
	 */
	KDEBUG(TRANSPORT, "Waiting for R/W access completion");
	GPIO_STOP_MEASURE();
	irq_status = event_wait(&sdio_dev.rw_comp);
	GPIO_START_MEASURE();
	TRSP_ASSERT(irq_status == IRQ_STATUS_ACTIVE);
#endif
	SET_SDIO_CLOCK_OFF ();

	/* Unlock */
	up(&sdio_dev.sdio_mutex);

	KDEBUG(TRANSPORT, "EXIT");
	return len;
}



/******************************************************************************
F U N C T I O N   D E F I N I T I O N S  ( R X )
******************************************************************************/
/*!
 * @brief This is the interrupt handler function. It should handle all the
 * interrupts from the SDIO host controller and from the target.
 *
 * When the target generates an SDIO interrupt (SDIO data[1] is lowered) data
 * is available on the target. The SDIO data[1] will be low until the
 * interrupt is acknowledged on the target. Linux workqueue #sdio_rx_task
 * is scheduled to acknowledge the interrupt and read the packet in process
 * context. Further SDIO interrupts should be disabled to prevent infinite
 * number of SDIO interrupts, since the SDIO data[1] is still low when the
 * interrupt handler returns.
 *
 * Interrupts from the SDIO host controller are used to synchronize the flow
 * of data in sdio_send_single(), sdio_recv_single() and sdio_cmd(). These
 * interrupts are optional and will simply fire an event.
 */
#if  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void sdio_irq (int irq, void *dev_id, struct pt_regs *regs)
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
static irqreturn_t sdio_irq (int irq, void *dev_id)
#else
static irqreturn_t sdio_irq (int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	int rx_task_status;
	irq_status_t irq_status __attribute__ ((unused));

	GPIO_START_MEASURE();
	if(0);

#ifdef USE_IF_REINIT        
#ifdef USE_IRQ_ID_IF_REINIT
	else if (sdio_interrupt(IRQ_OP_STATUS, IRQ_ID_IF_REINIT) > IRQ_STATUS_INACTIVE) {
#else
	/* This check if_reinit_flag is not mutex-protected together with
	 * following operations, however if_reinit_flag
	 * will be checked again in if_reinit(). We keep this for now to avoid
	 * scheuling work on every received packet for the case where we
	 * don't have the IRQ_ID_IF_REINIT.
	 */
	else if (atomic_read(&sdio_dev.if_reinit_flag)) {
#endif
		KDEBUG(TRANSPORT, "Reinit completion interrupt");

		/* Disable interrupts from target, enabled again by do_if_reinit */
		sdio_interrupt(IRQ_OP_DISABLE, IRQ_ID_TARGET);
		if (atomic_read(&sdio_dev.shutdown_flag) == 0)
			NRX_SCHEDULE_WORK(&if_reinit_task);
	}
#endif

#ifdef USE_IRQ_ID_RESPONSE_COMPLETION        
	/* Response completion interrupt */
	else if (IRQ_STATUS_INACTIVE < (irq_status = 
			 sdio_interrupt(IRQ_OP_STATUS, IRQ_ID_RESPONSE_COMPLETION))) {
				KDEBUG(TRANSPORT, "Response completion interrupt");
				statistics.irq_response_complete++;
				event_fire(&sdio_dev.resp_comp, irq_status);
				sdio_interrupt(IRQ_OP_ACKNOWLEDGE,
							   IRQ_ID_RESPONSE_COMPLETION);
	}
#endif
        
#ifdef USE_IRQ_ID_RW_ACCESS_COMPLETION
	/* R/W access completion interrupt */
	else if (IRQ_STATUS_INACTIVE < (irq_status = 
			 sdio_interrupt(IRQ_OP_STATUS, IRQ_ID_RW_ACCESS_COMPLETION))) {
				KDEBUG(TRANSPORT, "R/W access completion interrupt");
				statistics.irq_rw_access_complete++;
				event_fire(&sdio_dev.rw_comp, irq_status);
				sdio_interrupt(IRQ_OP_ACKNOWLEDGE,
							   IRQ_ID_RW_ACCESS_COMPLETION);
	}
#endif

#ifdef USE_IRQ_ID_FIFO_READ_READY
	/* Fifo ready for reading */
	else if (IRQ_STATUS_INACTIVE < (irq_status = 
			 sdio_interrupt(IRQ_OP_STATUS, IRQ_ID_FIFO_READ_READY))) {
				KDEBUG(TRANSPORT, "Fifo read ready interrupt");
				statistics.irq_fifo_read_ready++;
				event_fire(&sdio_dev.rd_fifo_rdy, irq_status);
				sdio_interrupt(IRQ_OP_ACKNOWLEDGE,
							   IRQ_ID_FIFO_READ_READY);
	}
#endif

#ifdef USE_IRQ_ID_FIFO_WRITE_READY
	/* Fifo ready for writing */
	else if (IRQ_STATUS_INACTIVE < (irq_status = 
			 sdio_interrupt(IRQ_OP_STATUS, IRQ_ID_FIFO_WRITE_READY))) {
				KDEBUG(TRANSPORT, "Fifo write ready interrupt");
				statistics.irq_fifo_write_ready++;
				event_fire(&sdio_dev.wr_fifo_rdy, irq_status);
				sdio_interrupt(IRQ_OP_ACKNOWLEDGE,
							   IRQ_ID_FIFO_WRITE_READY);
	}
#endif

	/* Target IRQ, this interrupt must be the last one in the if-else
	 * since the interrupt might still be enabled if the task has
	 * not acknowledged the interrupt on the target yet.
	 */
	else if (sdio_interrupt(IRQ_OP_STATUS, IRQ_ID_TARGET) > IRQ_STATUS_INACTIVE) {
		KDEBUG(TRANSPORT, "SDIO IRQ");
		statistics.irq_target++;
		/* Disable interrupts from target, enabled again by recv_task */
		sdio_interrupt(IRQ_OP_DISABLE, IRQ_ID_TARGET);

		if (atomic_read(&sdio_dev.shutdown_flag) == 0) {
			rx_task_status = NRX_SCHEDULE_WORK(&sdio_rx_task);
			if (!rx_task_status)
				statistics.rx_fail++;
		}
	}
	else
		TRSP_ASSERT(0);

	GPIO_STOP_MEASURE();
#if  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return;
#else
	return IRQ_HANDLED;
#endif        
}

#ifdef USE_IF_REINIT
/*!
 * @brief Calls if_reinit() to reinitialize the sdio interface.
 *
 * This function will be executed by the workqueue #if_reinit_task
 * that was scheduled in sdio_irq() on reinitialization request.
 *
 * @param data Optional data (not used)
 */
static void do_if_reinit(void *data)
{
	int status;

	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.sdio_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	if (if_reinit() == 0) {
		up(&sdio_dev.sdio_mutex);
		return;
	};

	/* Enable interrupt from target since it was disabled in sdio_irq(). */
	sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);

	up(&sdio_dev.sdio_mutex);
}

/*!
 * @brief Reinitializes the sdio interface.
 *
 * This function should be executed by the Linux workqueue that was scheduled
 * by sdio_irq(). if_reinit() will reinitialize the sdio interface in
 * the following way:
 *
 * 1. Call the host_if_power_on() function.\n
 * 2. Reinitialize the sdio controller.\n
 * 3. Clear the if_reinit flag.\n
 * 4. Reactivate periodic WiFiEngine activities.\n
 * 5. Enable SDIO interrupts.\n
 *
 * Note that the interrupt is not acknowledged, it will be handled again
 * when interrupts are reenabled (the triggering interrupt was really a
 * read interrupt).
 *
 * This function can be called in interrupt context.
 *
 * @return
 *   - 0 If reinit was performed
 *   - Negative otherwise
 *
 */
static int if_reinit(void)
{
	KDEBUG(TRANSPORT, "ENTER");

	if (atomic_read(&sdio_dev.if_reinit_flag) == 0)
		return -1;

	host_if_power_on();

	host_reset();

	sdio_control_interrupts(IRQ_OP_ENABLE, 0);
	sdio_interrupt(IRQ_OP_ACKNOWLEDGE, IRQ_ID_TARGET);

	atomic_set(&sdio_dev.if_reinit_flag, 0);

	nrx_drv_unquiesce();

	sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);

	return 0;
}


/*!
 * @brief Turns off the SDIO interface.
 *
 * Callback executed when the WiFiEngine inactivity timer expires.
 * It will turn off the SDIO interface by :
 *
 * 1. Inactivate WiFiEngine periodic activities (not data transmission).\n
 * 2. Set sdio_dev_t::if_reinit_flag flag.\n
 * 3. Call host_if_power_off() function.\n
 */
void driver_inact_cb(void **param, void *priv)
{
	int status;
	nrx_drv_quiesce();

	status = down_trylock(&sdio_dev.sdio_mutex);
	if (status != 0) {

		/* SDIO busy, the host should not sleep now. */
		nrx_drv_unquiesce();
		return;
	}

	atomic_set(&sdio_dev.if_reinit_flag, 1);

	host_if_power_off();

	up(&sdio_dev.sdio_mutex);
}
#endif /* USE_IF_REINIT */


/*!
 * @brief Receive next packet.
 *
 * This function should be executed by the Linux workqueue #sdio_rx_task that
 * was scheduled by sdio_irq(). sdio_recv() will read the next packet from the
 * queue in the following way:
 *
 * 1. Acknowledge the interrupt on the target by issuing a CMD52.\n
 * 2. Acknowledge the SDIO interrupt on the host.\n
 * 3. Read the first 16 bytes of data using sdio_recv_single() to get the size
 *    of the data (first two bytes).\n
 * 4. Read the remaining data in fragments according to the host platform max
 *    SDIO transfer size #MAX_TRANSFER_SIZE. Each fragment will be read using
 *    sdio_recv_single().\n
 * 5. Schedule workqueue #rx_task that will send the data to the upper layers,
 *    i.e. WiFiEngine.\n
 * 6. Enable SDIO interrupts again.\n
 *
 * @param dummy Optional data (not used)
 */
static void sdio_recv(nrx_work_data_t * dummy)
{
	uint8_t noise_packet;
	int status;
	struct sk_buff *skb;
	size_t remaining_len, transfer_len;
	unsigned int len;

	GPIO_START_MEASURE();
	TRSP_ASSERT(!in_interrupt());
	KDEBUG(TRANSPORT, "ENTER");

	statistics.rx_pkts++;

	/* Sync with power control */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.power_ctl_sem);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	/* Lock sdio */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.sdio_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	SET_SDIO_CLOCK_FAST ();

	/* Ack interrupt on target, data[1] should go high again */
	sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5, CMD_FLAG_SECURITY,
			 XMAC_ACKNOWLEDGE_SDIO_IRQ, 0);

	/* Make sure command response is received before we acknowledge
	 * the interrupt on the target.
	 */
	host_control(CTL_ID_WAIT_CMD_READY, 0);

	SET_SDIO_CLOCK_OFF ();

	up(&sdio_dev.sdio_mutex);

	/* Clear interrupt bit to ack interrupt on host,
	 * this must be done after the interrupt has been acked on the target
	 * or we will get an extra, false, interrupt.
	 */
	sdio_interrupt(IRQ_OP_ACKNOWLEDGE, IRQ_ID_TARGET);

	/* This buffer size is not really correct, it could probably 
	 * be a little bit tighter.
	 */
	skb = dev_alloc_skb(MAX_PACKET_SIZE);

	TRSP_ASSERT(skb);
	TRSP_ASSERT((unsigned long) skb->data % DMA_ALIGNMENT == 0);
	skb->dev = sdio_dev.iface;

	/* Read header */
	sdio_recv_single(skb_put(skb, XMAC_MIN_SIZE), XMAC_MIN_SIZE);

	len = ((uint16_t*) skb->data)[0];
	noise_packet = 0;

	/* Four first bytes equal? Then we have a 'noise packet'. */
	if (skb->data[0] == skb->data[1] &&
		skb->data[0] == skb->data[2] &&
		skb->data[0] == skb->data[3])
			noise_packet = 1;

	/* Invalid packet size, not necessarily where the first four bytes
	   have the same value? Then we have a 'noise packet'. */
	else if (!(5 <= len && len <= MAX_PACKET_SIZE))
		noise_packet = 1;

	if (noise_packet && !sdio_dev.is_booting) {

		TRSP_ASSERT(0);
		statistics.noise_pkts++;
#ifdef WIFI_DEBUG_ON
		nano_util_printbuf(skb->data, skb->len, "NOISE");
#endif
		dev_kfree_skb(skb);
		SET_SDIO_CLOCK_OFF ();

		up(&sdio_dev.power_ctl_sem);

		/* Enable interrupts from target again */
		sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);

		GPIO_STOP_MEASURE();
		return;
	}

	if (len + XMAC_LEN_FIELD_SIZE > XMAC_MIN_SIZE) 
	   remaining_len = len + XMAC_LEN_FIELD_SIZE - XMAC_MIN_SIZE;
	else 
	   remaining_len = 0;

	/* More data? */
	while (remaining_len) {
		if (remaining_len > MAX_TRANSFER_SIZE) 
			transfer_len = MAX_TRANSFER_SIZE;
		else 
			transfer_len = remaining_len;

		TRSP_ASSERT(remaining_len >= transfer_len);
		sdio_recv_single(skb_put(skb, transfer_len), transfer_len);
		remaining_len -= transfer_len;
	}

	KDEBUG_BUF(TRANSPORT, skb->data, skb->len, "SDIO RX data:");

#ifdef USE_WIFI
	skb_queue_tail(&sdio_dev.skb_rx_head, skb);
	if (atomic_read(&sdio_dev.shutdown_flag) == 0)
		NRX_SCHEDULE_WORK(&rx_task);
#else /* !USE_WIFI */
	do_test_rx(skb);
#endif /* USE_WIFI */

	up(&sdio_dev.power_ctl_sem);

	/* Enable interrupts from target again */
	sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);
	GPIO_STOP_MEASURE();
}


/*!
 * @brief Reads a fragment of a packet with CMD53.
 *
 * This is where the actual reception from the SDIO bus takes places.
 * sdio_recv_single() receives a fragment of data of a given length (<b>len</b>)
 * into a buffer (<b>buffer</b>) using either CPU or DMA to read the SDIO FIFO
 * (this decision is made by the specific host implementation).
 *
 * Prior to reading the FIFO, a SDIO CMD53 for reading must be issued.
 *
 * sdio_recv_single() relies on up to two interupts and one callback function
 * from the SDIO host controller. First of all the FIFO must be ready for
 * reading, this can be ensured by using #IRQ_ID_FIFO_READ_READY.
 * On some hosts this condition can be guaranteed without using a specific
 * interrupt so this interrupt is optional.
 *
 * When the data transfer is completed (e.g. all the FIFO is read)
 * sdio_recv_single() expects that the sdio_fifo_callback() will be called.
 *
 * To make sure that all data is transmitted to the host and the SDIO bus
 * is free to use again the optional interrupt #IRQ_ID_RW_ACCESS_COMPLETION
 * can be used.
 *
 * See interrupt explanations in host.h for more information.
 * 
 * Since the sdio_recv_single requires exclusive access to the SDIO bus,
 * the body of sdio_recv_single is concurrency protected by
 * sdio_dev_t::sdio_mutex.
 *
 * @param buffer The buffer which should hold the read data
 * @param len The number of bytes that should be read
 */
static void sdio_recv_single(void* buffer, unsigned int len)
{
	int status;
	irq_status_t irq_status __attribute__ ((unused));
	unsigned long buffer_phy = 0;

	KDEBUG(TRANSPORT, "ENTER: len = %d", len);
	TRSP_ASSERT((unsigned long) buffer % DMA_ALIGNMENT == 0);

	/* Lock sdio */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.sdio_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	SET_SDIO_CLOCK_FAST ();

	/* Set up read command */
	host_control(CTL_ID_LENGTH, len);
	sdio_cmd(SDIO_RW_EXTENDED, CMD_TYPE_ADTC, CMD_RESP_R5,
			 CMD_FLAG_DATA_EXISTS | CMD_FLAG_DIR_DATA_TO_HOST,
			 SDIO_IO_R_ARG_BASE + (len % 512), buffer);

#ifdef USE_IRQ_ID_FIFO_READ_READY
	/* Wait for access to FIFO */
	KDEBUG(TRANSPORT, "Waiting for access to FIFO");
	GPIO_STOP_MEASURE();
	irq_status = wait_on(&sdio_dev.rd_fifo_rdy);
	GPIO_START_MEASURE();
	TRSP_ASSERT(irq_status == IRQ_STATUS_ACTIVE);
#endif

#ifndef USE_NO_CACHE_COHERENCY
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	consistent_sync(buffer, len, PCI_DMA_FROMDEVICE);
	buffer_phy = (unsigned long) virt_to_bus(buffer);
#else
	/* TODO: Add a proper device structure */
	buffer_phy = (unsigned long) dma_map_single(NULL, buffer, len,
												DMA_FROM_DEVICE);
#endif /* LINUX_VERSION_CODE */
#endif /* !USE_NO_CACHE_COHERENCY */
	host_fifo(buffer, buffer_phy, len, FIFO_FLAG_TO_HOST);

	KDEBUG(TRANSPORT, "Waiting for DMA callback");
	GPIO_STOP_MEASURE();
	status = down_interruptible(&sdio_dev.fifo_sem);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

#ifdef USE_IRQ_ID_RW_ACCESS_COMPLETION
	/* Wait until reading is finished (signalled by controller) */
	KDEBUG(TRANSPORT, "Waiting R/W access completion");
	GPIO_STOP_MEASURE();
	irq_status = event_wait(&sdio_dev.rw_comp);
	GPIO_START_MEASURE();
	TRSP_ASSERT(irq_status == IRQ_STATUS_ACTIVE);
#endif

	SET_SDIO_CLOCK_OFF();
	KDEBUG(TRANSPORT, "Done!");

	/* Unlock sdio */
	up(&sdio_dev.sdio_mutex);
}



#ifdef USE_WIFI
/*!
 * @brief Process the list of packets queued by sdio_recv().
 *
 * Sends each packet to the upper layers using ns_net_rx().
 * This function is executed by the workqueue #rx_task.
 *
 * @param data Optional data (not used)
 */
static void do_rx_queue(nrx_work_data_t *data)
{
	struct sk_buff *skb;    

	GPIO_START_MEASURE();
	while ((skb = skb_dequeue(&sdio_dev.skb_rx_head)) != NULL)
		ns_net_rx(skb, skb->dev);
	GPIO_STOP_MEASURE();
}
#endif


/*!
 * @brief Access function for SDIO driver statistics through /proc interface.
 */
int sdio_read_proc (char* buf, char** start, off_t offset, int count, int* eof, void* data)
{
	int len = 0;
	len += sprintf (buf+len, "irq_response_complete : %u\n",
					statistics.irq_response_complete);
	len += sprintf (buf+len, "irq_rw_access_complete: %u\n",
					statistics.irq_rw_access_complete);
	len += sprintf (buf+len, "irq_fifo_read_ready   : %u\n",
					statistics.irq_fifo_read_ready);
	len += sprintf (buf+len, "irq_fifo_write_ready  : %u\n",
					statistics.irq_fifo_write_ready);
	len += sprintf (buf+len, "irq_target            : %u\n",
					statistics.irq_target);
	len += sprintf (buf+len, "Tx packets            : %u\n",
					statistics.tx_pkts);
	len += sprintf (buf+len, "Rx packets            : %u\n",
					statistics.rx_pkts);
	len += sprintf (buf+len, "Tx failed             : %u\n",
					statistics.tx_fail);
	len += sprintf (buf+len, "Rx failed             : %u\n",
					statistics.rx_fail);
	len += sprintf (buf+len, "wifi_send             : %u\n",
					statistics.wifi_send);
	len += sprintf (buf+len, "noise_pkts            : %u\n",
					statistics.noise_pkts);
	len += sprintf (buf+len, "sleep                 : %u\n",
					statistics.sleep);
	len += sprintf (buf+len, "wakeup                : %u\n",
					statistics.wakeup);

	*eof = 1;
	return len;
}


/******************************************************************************
F U N C T I O N   D E F I N I T I O N S  ( M I S C )
******************************************************************************/
/*!
 * @brief Module initialization on loading
 *
 * This function will be called when the SDIO driver module is loaded. Note
 * that the interface layer modules (nano_if.mod) must already be loaded.
 *
 * This function will perform the following steps:
 *
 * 1. Setup GPIO pins on the host.\n
 * 2. Setup internal synchronization primitives such as workqueues and
 *    semaphores.\n
 * 3. Initialize the SDIO host controller.\n
 * 4. Setup interrupt handler.\n
 * 5. Register hooks for firmware download and miscellaneous functions to
 *    upper layers. Firmware download can the be activated from the upper
 *    layers.\n
 * 6. Register /proc-files which will capture statistics of the SDIO driver.
 *    These statistics can be used as a debugging aid.\n
 *
 * Note that no messages will be sent to the target board at this time.
 * if #nrx_fw_download is non-zero.
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
int __init sdio_init(void)
{
	int status;
	int init_level = 0;
	unsigned long irqflags = 0;

	KDEBUG(TRANSPORT, "ENTER");
        
	GPIO_START_MEASURE();
	atomic_set(&sdio_dev.shutdown_flag, 0);
	spin_lock_init(&sdio_dev.interrupt_lock);
	sema_init(&sdio_dev.power_ctl_sem, 2);
	sema_init(&sdio_dev.sdio_mutex, 1);
	sema_init(&sdio_dev.fifo_sem, 0);
#ifdef USE_IRQ_ID_RESPONSE_COMPLETION
	event_init(&sdio_dev.resp_comp);
#endif
#ifdef USE_IRQ_ID_RW_ACCESS_COMPLETION
	event_init(&sdio_dev.rw_comp);
#endif
#ifdef USE_IRQ_ID_FIFO_READ_READY
	event_init(&sdio_dev.rd_fifo_rdy);
#endif
#ifdef USE_IRQ_ID_FIFO_WRITE_READY
	event_init(&sdio_dev.wr_fifo_rdy);	
#endif
	sdio_dev.is_booting = 0;
	sema_init(&sdio_dev.exit_sem, 0);
	memset(&statistics, 0, sizeof(statistics));

	NRX_INIT_WORK(&sdio_rx_task, sdio_recv, NULL);

#ifdef USE_WIFI

	spin_lock_init(&sdio_dev.send_lock);
	NRX_INIT_WORK(&rx_task, do_rx_queue, NULL);
	NRX_INIT_WORK(&tx_task, do_tx_queue, NULL);
   
#ifdef USE_IF_REINIT
	atomic_set(&sdio_dev.if_reinit_flag, 0);
	NRX_INIT_WORK(&if_reinit_task, do_if_reinit, NULL);
#endif /* USE_IF_REINIT */

#ifdef USE_POWER_MANAGEMENT
	NRX_INIT_WORK(&sleep_task, do_sleep, NULL);
	NRX_INIT_WORK(&wakeup_task, do_wakeup, NULL);
#endif /* USE_POWER_MANAGEMENT */

#endif /* USE_WIFI */
        
	status = host_init(nrx_4bit_sdio ? TRANSFER_MODE_4BIT_SDIO
									 : TRANSFER_MODE_1BIT_SDIO);
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 1);

#ifdef USE_DEBUG_PINS
	printk("nano_sdio: *** Debug pins active\n");
#endif /* USE_DEBUG_PINS */
	host_debug_pins(GPIO_OP_ENABLE, 0xffffffff);
	host_debug_pins(GPIO_OP_LOW, 0xffffffff);
	if (INIT_DEBUG_PINS != 0)
		host_debug_pins(GPIO_OP_HIGH, INIT_DEBUG_PINS);
			
#ifdef USE_GPIO_CPU_MEASURE
	host_gpio(GPIO_OP_ENABLE, GPIO_ID_CPU_LOAD);
	host_gpio(GPIO_OP_LOW, GPIO_ID_CPU_LOAD);
#endif /* USE_GPIO_CPU_MEASURE */

	/* FIXME: If running firmware under emulator control, we want to
	 * release the reset here to be able to download firmware.
	 */
	host_gpio(GPIO_OP_ENABLE, GPIO_ID_POWER);
	host_gpio(GPIO_OP_LOW, GPIO_ID_POWER);
	host_gpio(GPIO_OP_ENABLE, GPIO_ID_RESET_A);
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET_A);
	host_gpio(GPIO_OP_ENABLE,GPIO_ID_RESET_D);
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET_D);

	/* Disable and clear interrupt sources before handler registration
	   to avoid spurious interrupts. */
	status = sdio_control_interrupts(IRQ_OP_DISABLE, 1);
	if (status) goto exit;
	status = sdio_control_interrupts(IRQ_OP_ACKNOWLEDGE, 1);
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 2);

	/* Request irq and register handler */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
	irqflags |= SA_INTERRUPT;
#endif
	status = request_irq(HOST_SDIO_IRQ_NUM, sdio_irq, irqflags,
						 "sdio_irq", &sdio_dev);
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 3);

	if (!nrx_fw_download) {
		status = sdio_reset_host();
		if (status) goto exit;
		status = sdio_reset_target();
		if (status) goto exit;
		status = sdio_init_target();
		if (status) goto exit;
		status = sdio_init_target_interrupt();
		if (status) goto exit;
	}
	TRSP_ASSERT(++init_level == 4);

#ifdef USE_WIFI

	skb_queue_head_init(&sdio_dev.skb_tx_head);
	skb_queue_head_init(&sdio_dev.skb_rx_head);

	/* Create network device */
	create_param.size = sizeof(create_param);
	create_param.send = wifi_send;
	create_param.fw_download = nrx_fw_download ? sdio_fw_download : NULL;
	create_param.control = sdio_control;
	create_param.params_len = host_get_target_params(&create_param.params_buf);
	create_param.min_size = XMAC_MIN_SIZE;
	create_param.size_align = XMAC_ALIGNMENT;
	create_param.header_size = XMAC_HEADER_SIZE;
	create_param.host_attention = HIC_CTRL_ALIGN_HATTN_VAL_POLICY_NATIVE_SDIO;
	create_param.byte_swap_mode = HIC_CTRL_ALIGN_SWAP_NO_BYTESWAP;
	create_param.host_wakeup = HIC_CTRL_ALIGN_HWAKEUP_NOT_USED;
	create_param.force_interval = HIC_CTRL_ALIGN_FINTERVAL_NOT_USED;
        create_param.tx_headroom = 0;

	sdio_dev.iface = nanonet_create(NULL, NULL, &create_param);
	status = sdio_dev.iface ? 0 : -EPERM;
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 5);

#ifdef USE_IF_REINIT
	nrx_reg_inact_cb(driver_inact_cb);
#endif /* USE_IF_REINIT */

#else /* !USE_WIFI */
	kernel_thread(do_test, 0, 0);
#endif /* USE_WIFI */

	/* Initialize /proc interface. In case initialization fails,
	   this module will be loaded and operate without /proc interface. */
	{
		struct proc_dir_entry *proc_entry;
		
		sdio_dev.proc_dir = proc_mkdir("nanosdio", NULL);
		TRSP_ASSERT(sdio_dev.proc_dir);
		proc_entry = create_proc_read_entry("statistics",
											0, // Default mode
											sdio_dev.proc_dir, // Parent dir
											sdio_read_proc,
											NULL);
		TRSP_ASSERT(proc_entry);
	}

exit:
	if (status) {
		printk("nano_sdio: *** init failed, init_level = %d\n", init_level);
		switch (init_level) {
			case 4:
			case 3:
				sdio_control_interrupts(IRQ_OP_DISABLE, 1);
				free_irq(HOST_SDIO_IRQ_NUM, &sdio_dev);
			case 2:
			case 1:
				host_exit();
				sdio_shutdown_target();
				host_debug_pins(GPIO_OP_DISABLE, 0xffffffff);
			case 0:
				break;
			default:
				TRSP_ASSERT(0);
		}
	}
	GPIO_STOP_MEASURE();
#ifdef USE_GPIO_CPU_MEASURE
	if (status)
		host_gpio(GPIO_OP_DISABLE, GPIO_ID_CPU_LOAD);
#endif
	KDEBUG(TRANSPORT, "EXIT");
	return status;
}


/*!
 * @brief Called on module unloading.
 *
 * Disconnects from upper layers by calling nanonet_destroy(). Waits on
 * sdio_dev_t::exit_sem to sync with shutdown completion of upper layers.
 *
 * After the upper layers are disconnected then any resources used by the SDIO
 * driver will be returned and all SDIO interrupts will be disabled.
 * GPIO pins will be disabled.
 */
void __exit sdio_exit(void)
{
#ifdef USE_WIFI

	int status;
	struct sk_buff *skb;

#ifdef USE_IF_REINIT
	atomic_set(&sdio_dev.if_reinit_flag, 0);
	nrx_dereg_inact_cb();
#endif /* USE_IF_REINIT */

	/* nanonet_destroy() eventually calls sdio_control(NANONET_SHUTDOWN, ...)
	   exit_sem will signal shutdown completion */
	nanonet_destroy(sdio_dev.iface);
	status = down_interruptible(&sdio_dev.exit_sem);
	TRSP_ASSERT(status == 0);

	while ( (skb=skb_dequeue(&sdio_dev.skb_rx_head)) != NULL)
		dev_kfree_skb_any (skb);
	while ( (skb=skb_dequeue(&sdio_dev.skb_tx_head)) != NULL)
		dev_kfree_skb_any (skb);

#endif /* USE_WIFI */

	remove_proc_entry ("statistics", sdio_dev.proc_dir);
	remove_proc_entry ("nanosdio", NULL);
        
	free_irq (HOST_SDIO_IRQ_NUM, &sdio_dev);
	host_debug_pins(GPIO_OP_DISABLE, 0xffffffff);
}


/*!
 * @brief Reset the SDIO host controller.
 *
 * Calls host_reset(), enables interrupts except target interrupt
 * and stops SDIO clock.
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
static int sdio_reset_host(void)
{
	int status;
	
	status = host_reset();
	if (status) return status;

	/* Enable SDIO host interrupts */
	status = sdio_control_interrupts(IRQ_OP_ENABLE, 0);
	if (status) return status;

#ifndef USE_NO_CLOCK
	host_clock(CLK_MODE_FAST);
#endif

	return 0;
}


/*!
 * @brief Downloads firmware on the target.
 *
 * Callback function that is invoked when firmware download is started.
 *
 * This function will properly reset and setup the target and the transfer the
 * firmware buffer to the target.
 *
 * When the firmware has been transferred, the host SDIO controller will be
 * reset in order to send SDIO initalization commands again to the target
 * with the new firmware running.
 * 
 * @param fw_buf The buffer containing the firmware image
 * @param fw_len The size of the firmware image
 * @param data Optional data (not used)
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
#ifdef USE_FW_DOWNLOAD
static int sdio_fw_download(const void *fw_buf, size_t fw_len, void *data)
{
	int status;
	size_t transfer_len, remaining_len;
	size_t send_len = 0;
#ifdef WIFI_DEBUG_ON
	int debug_flag_saved = nrx_debug;
#endif
	u32 *buf_virt;   

	KDEBUG(TRANSPORT, "ENTER: fw_len=%d", fw_len);
	GPIO_START_MEASURE();

	status = sdio_reset_host();
	if (status) goto exit;
	status = sdio_reset_target();
	if (status) goto exit;
	status = sdio_init_target();
	if (status) goto exit;

	buf_virt = kmalloc(MAX_TRANSFER_SIZE, GFP_KERNEL | GFP_DMA);
	if (!buf_virt) {
		TRSP_ASSERT(0);
		status = -ENOMEM;
		goto exit;
	}

#ifdef WIFI_DEBUG_ON
	nrx_debug = 0;
#endif

	TRSP_MSG("Downloading firmware\n");
	remaining_len = fw_len;
	while (remaining_len) {

		/* How much data should be transferred in this chunk?
		 * The maximum transfer size is 16 support uninitialized PLL.
		 */
		if (remaining_len > 16) 
			transfer_len = 16;
		else 
			transfer_len = remaining_len;
		
		TRSP_ASSERT(remaining_len >= transfer_len);
		memcpy(buf_virt, fw_buf + send_len, transfer_len);
		send_len += sdio_send_single(buf_virt, transfer_len);
		remaining_len -= transfer_len;
	}

	kfree(buf_virt);
	SET_SDIO_CLOCK_OFF ();

#ifdef WIFI_DEBUG_ON
	nrx_debug = debug_flag_saved;
#endif

	/* Wait for target to boot */
	SAFE_DELAY(10);

	/* The CMD3, CMD7, etc. have to be issued again after the firmware have
	 * been downloaded, some hosts require reset for this to work properly.
	 */
	status = sdio_reset_host();
	if (status) goto exit;
	status = sdio_init_target();
	if (status) goto exit;
	status = sdio_init_target_interrupt();

exit:
	TRSP_MSG("Downloading firmware finished, status = %d\n", status);
	GPIO_STOP_MEASURE();
	KDEBUG(TRANSPORT, "EXIT");
	return status;
}
#endif /* USE_FW_DOWNLOAD */


/*!
 * @brief Resets the target.
 *
 * Properly resets the target using reset and optionally also shutdown pin.
 * The target will be reset when this function returns.
 *
 * @return
 * - 0 on success
 * - -EIO if any CMD fails
 */
static int sdio_reset_target(void)
{
	int status = 0;
	
	/* Reset target */
	KDEBUG(TRANSPORT, "INIT");

	host_gpio(GPIO_OP_LOW, GPIO_ID_POWER);
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET_A);
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET_D);

#ifdef USE_FAST_BOOT
	SAFE_DELAY(1);
	host_gpio(GPIO_OP_HIGH, GPIO_ID_POWER);
	SAFE_DELAY(1);
	host_gpio(GPIO_OP_HIGH, GPIO_ID_RESET_A);
	SAFE_DELAY(1);
	host_gpio(GPIO_OP_HIGH, GPIO_ID_RESET_D);
	SAFE_DELAY(30);
#else
	/* Should be long enough to drain enough power from target to get reset */
	SAFE_DELAY(500);
	host_gpio(GPIO_OP_HIGH, GPIO_ID_POWER);
	SAFE_DELAY(10);
	host_gpio(GPIO_OP_HIGH, GPIO_ID_RESET_A);
	SAFE_DELAY(10);
	host_gpio(GPIO_OP_HIGH, GPIO_ID_RESET_D);
	SAFE_DELAY(10);
#endif

	SET_SDIO_CLOCK_FAST ();

	status = sdio_cmd(SD_SEND_RELATIVE_ADDR, CMD_TYPE_BCR, CMD_RESP_R6, 0,0,0);
	if (status) goto exit;

	status = sdio_cmd(SD_SELECT_DESELECT_CARD, CMD_TYPE_AC, CMD_RESP_R1,
			 CMD_FLAG_BUSY, host_control(CTL_ID_RESPONSE, 0), 0);
	if (status) goto exit;

	/* If GPIO reset is not available, write 0x00 to register 0xF5
	 * to reset chip (same as RESET_D)
	 */
	status = sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5,
				      CMD_FLAG_SECURITY, XMAC_RESET, 0);

exit:
	/* Shutoff clock and wait until reset completes */
	SET_SDIO_CLOCK_OFF();
	SAFE_DELAY(10);

	KDEBUG(TRANSPORT, "EXIT");
	return status;
}


/*!
 * @brief Initializes target after reset.
 *
 * This function will send the required SDIO commands:
 *
 * 1. CMD3 (twice for selt-test if USE_CMD3_TEST is defined)\n
 * 2. CMD7\n
 * 3. CMD52 to enable SDIO interrupt.\n
 * 4. CMD52 to enable SDIO interrupt generation when SDIO clock is off.\n
 * If USE_CMD3_TEST is defined, target initialization is considered successful
 * only if responses to successive CMD3s carry different RCAs. 
 *
 * @return
 * - 0 on success
 * - -EIO if any CMD fails.
 * - -EFAULT if USE_CMD3_TEST is defined
 *			 and responses to successive CMD3 are identical.
 */
static int sdio_init_target(void)
{
	int status = 0;

	SET_SDIO_CLOCK_FAST ();

	status = sdio_cmd(SD_SEND_RELATIVE_ADDR, CMD_TYPE_BCR, CMD_RESP_R6, 0,0,0);
	if (status) goto exit;

#ifdef USE_CMD3_TEST
	{
		uint32_t prev_response = host_control(CTL_ID_RESPONSE, 0);
		status = sdio_cmd(SD_SEND_RELATIVE_ADDR, CMD_TYPE_BCR, CMD_RESP_R6, 0,0,0);
		if (status) goto exit;
		if (host_control(CTL_ID_RESPONSE, 0) == prev_response) {
			status = -EFAULT;
			goto exit;
		}
	}
#endif

	status = sdio_cmd(SD_SELECT_DESELECT_CARD, CMD_TYPE_AC, CMD_RESP_R1,
			 CMD_FLAG_BUSY, host_control(CTL_ID_RESPONSE, 0), 0);
	if (status) goto exit;

	status = sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5, CMD_FLAG_SECURITY,
			 XMAC_ENABLE_SDIO_IRQ, 0);
	if (status) goto exit;

#ifdef USE_NO_CLOCK
	/* Enable sdio interrupt without sdio clock */
	status = sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5, CMD_FLAG_SECURITY,
			 XMAC_NO_CLOCK_SDIO_IRQ, 0);
	if (status) goto exit;
#endif

	if (nrx_4bit_sdio)
		status = sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5, 
				 CMD_FLAG_SECURITY, XMAC_ENABLE_SDIO_4BIT_MODE, 0);

exit:
	SET_SDIO_CLOCK_OFF ();
	return status;
}


/*!
 * @brief Configure SDIO host interrupts in a concurrency protected way.
 *
 * @param irq_op The interrupt operation to perform, see host.h for more
 *               information
 * @param irq_id The interrupt id to perform the operation on, see host.h for
 *               more information.
 *
 * @return See definition of #irq_status_t
 */
static irq_status_t sdio_interrupt(irq_op_t irq_op, irq_id_t irq_id)
{
	irq_status_t status;
	unsigned long flags;

	spin_lock_irqsave(&sdio_dev.interrupt_lock, flags);
	status = host_interrupt(irq_op, irq_id);
	spin_unlock_irqrestore(&sdio_dev.interrupt_lock, flags);
	return status;
}


/*!
 * @brief Configure all interrupt sources.
 *
 * Performs requested operation on all used interrupt sources,
 * including target interrupt if <b>include_target_irq</b> is non-zero.
 * If requested, target interrupt manipulation will be done first.
 *
 * @param irq_op The interrupt operation to perform, see host.h for more
 *               information. Must not be #IRQ_OP_STATUS.
 * @param include_target_irq If non-zero, the operation will also be
 * 							 performed on target interrupt.
 *
 * @return
 * - 0 on success
 * - -EPERM if it fails for any IRQ_ID
 * - -EINVAL if called with invalid irq_op
 */
static int sdio_control_interrupts(irq_op_t irq_op, int include_target_irq)
{
	int failed = 0;

	if (irq_op != IRQ_OP_ENABLE && irq_op != IRQ_OP_DISABLE
								&& irq_op != IRQ_OP_ACKNOWLEDGE) {
		TRSP_ASSERT(0);
		return -EINVAL;
	}

	 /* Following error checks assume that IRQ_STATUS_SUCCESS is 0 */
	TRSP_ASSERT(IRQ_STATUS_SUCCESS == 0);

	if (include_target_irq)
		failed |= sdio_interrupt(irq_op, IRQ_ID_TARGET);

#ifdef USE_IRQ_ID_FIFO_READ_READY
	failed |= sdio_interrupt(irq_op, IRQ_ID_FIFO_READ_READY);
#endif
#ifdef USE_IRQ_ID_FIFO_WRITE_READY
	failed |= sdio_interrupt(irq_op, IRQ_ID_FIFO_WRITE_READY);
#endif
#ifdef USE_IRQ_ID_RW_ACCESS_COMPLETION
	failed |= sdio_interrupt(irq_op, IRQ_ID_RW_ACCESS_COMPLETION);
#endif
#ifdef USE_IRQ_ID_RESPONSE_COMPLETION
	failed |= sdio_interrupt(irq_op, IRQ_ID_RESPONSE_COMPLETION);
#endif

	return failed ? -EPERM : 0;
}


/*!
 * @brief Clears and enables interrupt from the target
 *        after target initialization.
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
static int sdio_init_target_interrupt(void)
{
	int status;

	status = sdio_interrupt(IRQ_OP_ACKNOWLEDGE, IRQ_ID_TARGET);
	if (status) return status;
	status = sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);
	return status;
}


/*!
 * @brief Does target power down.
 *
 * Called during module exit. Asserts analog reset and digital reset,
 * removes target power and disables host GPIOs driving those signals.
 * Does not affect #GPIO_ID_CPU_LOAD.
 */
static void sdio_shutdown_target(void)
{
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET_A);
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET_D);
	host_gpio(GPIO_OP_LOW, GPIO_ID_POWER);
	host_gpio(GPIO_OP_DISABLE,GPIO_ID_POWER);
	host_gpio(GPIO_OP_DISABLE,GPIO_ID_RESET_A);
	host_gpio(GPIO_OP_DISABLE,GPIO_ID_RESET_D);
}


/*!
 * @brief Sends an SDIO command.
 *
 * This function will first wait for the command line on
 * the SDIO bus to be ready before sending the command.
 * 
 * The optional interrupt #IRQ_ID_RESPONSE_COMPLETION can be used to
 * make sure the command response is received into the SDIO host controller
 * registers before the function returns.
 *
 * @param cmd The SDIO command to send
 * @param cmd_type The SDIO command type to send
 * @param cmd_resp The SDIO command response to expect
 * @param flags Command flags to use (see host.h for more information)
 * @param arg The SDIO command argument
 * @param data		Pointer to data for data phase (only for CMD53)
 *
 * @return
 * - 0 on success
 * - -EIO on failure
 */
static int sdio_cmd(uint8_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
					 uint32_t flags, uint32_t arg, void* data)
{
	irq_status_t irq_status;
	int is_wakeup_cmd, is_error;
	
	/* Wait until ready (transaction completion) */
	KDEBUG(TRANSPORT, "Waiting for command to be ready");
	host_control(CTL_ID_WAIT_CMD_READY, 0);

	/* Set up command */
	is_wakeup_cmd = (cmd == SDIO_RW_DIRECT) && (arg == XMAC_DISABLE_SLEEP);
	irq_status = host_cmd(cmd, cmd_type, cmd_resp, flags, arg, data);
	is_error = (irq_status != IRQ_STATUS_SUCCESS);

#ifdef USE_IRQ_ID_RESPONSE_COMPLETION
	if (!is_error) {

		/* Wait for command response */
		KDEBUG(TRANSPORT, "Waiting for command response");
		if (in_interrupt()) {
			KDEBUG(TRANSPORT, "Interrupt context -- busy waiting");
			irq_status = event_busywait(&sdio_dev.resp_comp, BUSYWAIT_LOOP_MAX);
		}
		else {
			KDEBUG(TRANSPORT, "Process context -- waiting for interrupt");
			GPIO_STOP_MEASURE();
			irq_status = event_wait(&sdio_dev.resp_comp);
			GPIO_START_MEASURE();
		}
		is_error = (irq_status != IRQ_STATUS_ACTIVE);
	}
#endif

	/* Tolerate CRC errors during wakeup command. */
	if (is_wakeup_cmd && (irq_status == IRQ_STATUS_CRC))
		is_error = 0;

	if (is_error)
		TRSP_MSG("irq_status = %d, cmd = %d, arg = 0x%08x\n",
				  irq_status, cmd, arg);

	return is_error ? -EIO : 0;
}


/*!
 * @brief FIFO transfer completion callback.
 *
 * This function should be called by the specific SDIO host controller
 * implementation when FIFO transfer (either by CPU or DMA) has finished
 * to release tasks waiting on sdio_dev_t::fifo_sem.
 */
void sdio_fifo_callback(void)
{
	GPIO_START_MEASURE();
	KDEBUG(TRANSPORT, "ENTER");
	up(&sdio_dev.fifo_sem);
	GPIO_STOP_MEASURE();
}


/*!
 * @brief Controls target power save mode.
 *
 * It can be called in interrupt context. Therefore, it uses workqueues
 * #sleep_task and #wakeup_task so that do_sleep() or do_wakeup() will
 * be eventually executed in process context to do the job requested.
 *
 * @param mode Specifies whether target should be allowed to sleep or not.
 * @param data Optional data (not used)
 * 
 * @return
 * - 0 on success,
 * - -EBUSY if failed to schedule work,
 * - -EOPNOTSUPP if power management is not supported
 *				 (i.e. USE_POWER_MANAGEMENT has not been defined)
 */
#if defined(USE_POWER_MANAGEMENT)
static int sdio_control_sleep(uint32_t mode, void *data)
{

	int ret_val = 0; /* assume success */
    KDEBUG(TRACE, "ENTRY");
	switch (mode) {
	
	case NANONET_SLEEP_TEST:
		/* Just return 0 to indicate that power mgmt is supported */
		break;

	case NANONET_SLEEP_ON:
		if (NRX_SCHEDULE_WORK(&sleep_task) == 0) {
			TRSP_ASSERT(0);
			ret_val = -EBUSY;
		}
		break;

	case NANONET_SLEEP_OFF:
		if (NRX_SCHEDULE_WORK(&wakeup_task) == 0) {
			TRSP_ASSERT(0);
			ret_val = -EBUSY;
		}
		break;

	default:
		TRSP_ASSERT(0);
		ret_val = -EOPNOTSUPP;
	}

    KDEBUG(TRACE, "EXIT");
	return ret_val;
}
#elif defined(USE_WIFI)
static int sdio_control_sleep(uint32_t mode, void *data)
{	
	return -EOPNOTSUPP;
}
#endif /* USE_POWER_MANAGEMENT, USE_WIFI */


#ifdef USE_POWER_MANAGEMENT
/*!
 * @brief Enables target's power save mode.
 *
 * Sends appropriate CMD52 to enable target's power save mode.
 * It is executed by workqueue #sleep_task.
 *
 * @param data Optional data (not used)
 */
static void do_sleep(nrx_work_data_t *data)
{
	int status;
	
	/* Wait for send & recv activity completion and block it. */
	status = down_interruptible(&sdio_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);
	status = down_interruptible(&sdio_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);
	
	/* After locking power_ctl_sem, locking sdio shouldn't be necessary. */

#ifdef USE_IF_REINIT
   /* The body of if_reinit will check whether re-init is really needed
    * or not.
    */
   (void) if_reinit();
#endif

	/* Disable interrupts from target. */
	sdio_interrupt(IRQ_OP_DISABLE, IRQ_ID_TARGET);

	/* Going from active to doze mode */
	statistics.sleep++;
	SET_SDIO_CLOCK_FAST();
	sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5,
		   CMD_FLAG_SECURITY, XMAC_ENABLE_SLEEP, 0);
	SET_SDIO_CLOCK_OFF();
	
   /* Enable interrupts from target. */
	sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);

   /* Unlock */
   up(&sdio_dev.power_ctl_sem);
   up(&sdio_dev.power_ctl_sem);
}


/*!
 * @brief Disables target's power save mode.
 *
 * Sends appropriate CMD52 to enable target's power save mode.
 * Slow SDIO clock must be used to ensure reliable target wakeup
 * if the target is sleeping (see also clk_mode_t::CLK_MODE_SLOW).
 * It is executed by workqueue #wakeup_task.
 *
 * @param data Optional data (not used)
 */
static void do_wakeup(nrx_work_data_t *data)
{
	int status;
	
	/* Wait for send & recv activity completion and block it. */
	status = down_interruptible(&sdio_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);
	status = down_interruptible(&sdio_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);

	/* After locking power_ctl_sem, locking sdio shouldn't be necessary. */

#ifdef USE_IF_REINIT
   /* The body of if_reinit will check whether re-init is really needed
    * or not.
    */
   (void) if_reinit();
#endif

	/* Disable interrupts from target. */
	sdio_interrupt(IRQ_OP_DISABLE, IRQ_ID_TARGET);

	/* Going from doze to active mode 
	 * use the slow clock to wake up the target.
	*/
	statistics.wakeup++;
	SET_SDIO_CLOCK_SLOW();
	sdio_cmd(SDIO_RW_DIRECT, CMD_TYPE_AC, CMD_RESP_R5,
		   CMD_FLAG_SECURITY, XMAC_DISABLE_SLEEP, 0);
	SET_SDIO_CLOCK_OFF();
	
   /* Enable interrupts from target. */
	sdio_interrupt(IRQ_OP_ENABLE, IRQ_ID_TARGET);

   /* Unlock */
	up(&sdio_dev.power_ctl_sem);
	up(&sdio_dev.power_ctl_sem);      
}
#endif /* USE_POWER_MANAGEMENT */


#ifdef USE_WIFI
/* phth: is this used anymore? */
static int sdio_control_boot(uint32_t mode, void *data)
{
	KDEBUG(TRACE, "ENTRY");
	switch (mode) {
		case NANONET_BOOT_TEST:
			return 0;
		case NANONET_BOOT_ENABLE:
			sdio_dev.is_booting = 1;
			return 0;
	  case NANONET_BOOT_DISABLE:
			sdio_dev.is_booting = 0;
			return 0;
	}
	return -EINVAL;
}


/*!
 * @brief Callback for misc SDIO functions.
 *
 * @param command Control command
 * @param mode The control command mode (enable/disable)
 * @param data Optional data (not used)
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
static int sdio_control(uint32_t command, uint32_t mode, void *data)
{
	int status;

	KDEBUG(TRANSPORT, "ENTRY: %u:%u", command, mode);
	switch (command) {
		case NANONET_INIT_SDIO:
			status = sdio_reset_host();
			if (status == 0)
				status = sdio_init_target();
			if (status == 0)
				status = sdio_init_target_interrupt();
			break;

		case NANONET_SLEEP:
			status = sdio_control_sleep(mode, data);
			break;

		case NANONET_BOOT:
			status = sdio_control_boot(mode, data);
			break;

		case NANONET_SHUTDOWN:
			atomic_set(&sdio_dev.shutdown_flag, 1);
			NRX_FLUSH_SCHEDULED_WORK();

			/* we have no outstanding tasks, so disable interrupts, and
			 * terminate target
			 */
			sdio_control_interrupts(IRQ_OP_DISABLE, 1);
			host_exit();
			sdio_shutdown_target();

			/* let sdio_exit() complete */
			up(&sdio_dev.exit_sem);
			status = 0;
			break;

		default:
			status = -EOPNOTSUPP;
			break;
	}
	return status;
}
#endif /* USE_WIFI */


#ifndef USE_WIFI

#define LOOPBACK_SEMA_INIT				7
#define LOOPBACK_HIC_CTRL_ALIGN_HATTN	0x01
#define LOOPBACK_FW_DOWNLOAD		sdio_fw_download
#define LOOPBACK_SEND				sdio_send
#define LOOPBACK_INTERRUPT(x)		sdio_control_interrupts(x, 1)
#define LOOPBACK_SHUTDOWN_TARGET	sdio_shutdown_target
#include "../loopback/loopback.c"

#endif /* USE_WIFI */


module_init(sdio_init);
module_exit(sdio_exit);


/*! @} */ /* End of sdio_generic group */
