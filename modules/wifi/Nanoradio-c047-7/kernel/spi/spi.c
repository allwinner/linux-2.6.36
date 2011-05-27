/*
 * $Id: spi.c 16108 2010-09-20 08:06:34Z anbg $
 */


/*! @defgroup spi_generic Generic SPI driver implementation
 *
 * @brief Generic SPI driver implementation
 *
 * This file implements the generic SPI driver where the general functionality
 * is implemented.
 *
 * The generic SPI driver does not make any assumptions on the specific
 * SPI host that is used, those low level operations are captured in the
 * SPI host interface (host.h).
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
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,41)
#include <linux/tqueue.h>
#else
#include <linux/workqueue.h>
#endif

#include "host.h"

#ifdef USE_WIFI
#include <transport.h>
#endif /* USE_WIFI */

#include <linux/module.h>
#include <asm/semaphore.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <nanoutil.h>

MODULE_LICENSE("GPL");

/******************************************************************************
D E L A Y   F U N C T I O N A L I T Y
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
#define XMAC_RESET					0x8801ea00
#define XMAC_ENABLE_SLEEP         	0x8001e200
#define XMAC_DISABLE_SLEEP        	0x8001e201
#define XMAC_ACKNOWLEDGE_TARGET_IRQ	0x8001e002
#define XMAC_ENABLE_SDIO_IRQ      	0x80000803
#define XMAC_NO_CLOCK_SDIO_IRQ		0x8001e401
#define XMAC_NO_CHIP_SEL_SDIO_IRQ	0x88000e20

#define XMAC_MIN_SIZE       32
#define XMAC_ALIGNMENT      4
#define XMAC_HEADER_SIZE	18
#define XMAC_LEN_FIELD_SIZE 2
#define MAX_PACKET_SIZE     2048
#define DMA_ALIGNMENT       4


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


/*! @brief Device driver state */
struct spi_dev_t {

	/*! Is WiFiEngine shutting down the target? */
	atomic_t shutdown_flag;

	/*! Used by spi_interrupt() to call host_interrupt()
	 *  with interrupts locked out.
	 */
	spinlock_t interrupt_lock;

	/*! Driver's network iface */
	struct net_device *iface;

	/*! Mutual exclusion between target power control vs. send and receive
	 *  activities on SPI bus.
	 */
	struct semaphore power_ctl_sem;

	/*! Serializes SDIO commands on SPI bus.
	 *  Should be acquired only after #power_ctl_sem has been acquired.
	 */
	struct semaphore spi_mutex;

#ifdef USE_WIFI
	spinlock_t send_lock;				/*!< Used by wifi_send() */
	struct sk_buff_head skb_rx_head;	/*!< RX queue */
	struct sk_buff_head skb_tx_head;	/*!< TX queue */
#endif /* USE_WIFI */

	/*! Is target booting? */
	int is_booting;

	/*! for /proc/nanospi entry */
	struct proc_dir_entry *proc_dir;

	/*! Signals shutdown completion to spi_exit() */
	struct semaphore exit_sem;
};

/*! @brief Counts important events during transport operation. */
struct spi_statistics_t
{
	unsigned irq_spi;	/*!< IRQs from target */
	unsigned tx_pkts;	/*!< packets transmitted */
	unsigned rx_pkts;	/*!< packets received */
	unsigned rx_fail;	/*!< packets failed to receive */
	unsigned wifi_send;	/*!< calls to wifi_send() */
	unsigned noise_pkts;/*!< spurious receptions */
	unsigned sleep;		/*!< sleep commands to target */
	unsigned wakeup;	/*!< wakeup commands to target */
};

static struct spi_dev_t spi_dev; /*!< See #spi_dev_t */
static struct spi_statistics_t statistics; /*!< See #spi_statistics_t */
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

struct NRX_WORK_STRUCT spi_rx_task;	/*!< see spi_recv() */

#ifdef USE_WIFI
struct NRX_WORK_STRUCT tx_task;	/*!< see do_tx_queue() */
struct NRX_WORK_STRUCT rx_task;	/*!< see do_rx_queue() */
#endif /* USE_WIFI */

#ifdef USE_POWER_MANAGEMENT
struct NRX_WORK_STRUCT sleep_task;	/*!< see do_sleep() */
struct NRX_WORK_STRUCT wakeup_task;	/*!< see do_wakeup() */
#endif /* USE_POWER_MANAGEMENT */


/******************************************************************************
F U N C T I O N   P R O T O T Y P E S
******************************************************************************/
int __init spi_init(void);
void __exit spi_exit(void);
static int spi_read_proc(char* buf, char** start, off_t offset,
						 int count, int* eof, void* data);

static void spi_reset_target(void);
static int  spi_init_target(void);
static void spi_shutdown_target(void);

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void spi_irq (int irq, void *dev_id, struct pt_regs *regs);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
static irqreturn_t spi_irq (int irq, void *dev_id);
#else
static irqreturn_t spi_irq (int irq, void *dev_id, struct pt_regs *regs);
#endif

static uint32_t spi_interrupt(irq_op_t irq_op);
static int      spi_init_target_irq(void);

static void spi_send(struct sk_buff *skb);
static int  spi_send_single(void* buffer, uint32_t len);
static void spi_recv(nrx_work_data_t* dummy);
static void spi_recv_single(void* buffer, unsigned int len);

#ifdef USE_WIFI
static int spi_control(uint32_t command, uint32_t mode, void *data);
static int spi_control_sleep(uint32_t mode, void *data);
static int spi_control_boot(uint32_t mode, void *data);
static int spi_fw_download(const void *fw_buf, size_t fw_len, void *data);
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
F U N C T I O N   D E F I N I T I O N S  ( T X )
******************************************************************************/
#ifdef USE_WIFI
/*!
 * @brief Top-level transmit function that is registered with the upper layers.
 *
 * Transmissions are issued from the upper layers, i.e. the WiFiEngine.
 * Upon transmission, wifi_send() will be called. The responsibility of
 * wifi_send() is to transfer the data to the target. The actual data transfer
 * is partitioned into three functions; wifi_send(), spi_send() and
 * spi_send_single().
 *
 * wifi_send() will put the data (<b>skb</b>) to be sent in a queue and return
 * immediately. Linux workqueue #tx_task is scheduled to handle the packet
 * in process context.
 *
 * wifi_send() can be called in interrupt context or in process context and
 * can also be called concurrently. Therefore the wifi_send() will not block
 * and its body is concurrency protected using spi_dev_t::send_lock.
 *
 * @param skb The socket buffer to transmit
 * @param data Optional parameter (not used)
 *
 * @return 0
 */
static int wifi_send(struct sk_buff *skb, void *data)
{
	unsigned long flags;
	spin_lock_irqsave(&spi_dev.send_lock, flags);

	statistics.wifi_send++;

	GPIO_START_MEASURE();
	TRSP_ASSERT((unsigned long) skb->data % DMA_ALIGNMENT == 0);
	skb_queue_tail(&spi_dev.skb_tx_head, skb);
	if (atomic_read(&spi_dev.shutdown_flag) == 0)
		NRX_SCHEDULE_WORK(&tx_task);
	GPIO_STOP_MEASURE();

	spin_unlock_irqrestore(&spi_dev.send_lock,flags);
	return 0;
}


/*!
 * @brief Process the list of packets queued by wifi_send().
 *
 * Sends each packet using spi_send(). Executed by the workqueue #tx_task.
 *
 * @param data Optional parameter (not used)
 */
static void do_tx_queue(nrx_work_data_t *data)
{
	struct sk_buff *skb;

	GPIO_START_MEASURE();
	while((skb = skb_dequeue(&spi_dev.skb_tx_head)) != NULL)
		spi_send(skb);
	GPIO_STOP_MEASURE();
}
#endif /* USE_WIFI */


/*!
 * @brief Sends a packet.
 *
 * This function will be executed by the workqueue #rx_task that was scheduled by
 * wifi_send() when a packet has been dequeued. 
 *
 * spi_send() will fragmented the data (<b>skb</b>) according to the host
 * platform max transfer size #MAX_TRANSFER_SIZE. Each fragment will
 * be sent using spi_send_single().
 *
 * @param skb The socket buffer to transmit
 */
static void spi_send(struct sk_buff *skb)
{
	int status;
	size_t transfer_len, remaining_len;
	size_t send_len = 0;

	KDEBUG(TRANSPORT, "ENTER: skb->len=%d", skb->len);

	TRSP_ASSERT(!in_interrupt());
	TRSP_ASSERT(skb->len <= MAX_PACKET_SIZE);
	TRSP_ASSERT((unsigned long) skb->data % DMA_ALIGNMENT == 0);

	KDEBUG_BUF(TRANSPORT, skb->data, skb->len, "SPI TX data:");

	/* Sync with power control */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&spi_dev.power_ctl_sem);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	remaining_len = skb->len;
	while(remaining_len) {

		/* How much data should be transferred in this chunk? */
		if (remaining_len > MAX_TRANSFER_SIZE) 
			transfer_len = MAX_TRANSFER_SIZE;
		else
			transfer_len = remaining_len;

		TRSP_ASSERT(remaining_len >= transfer_len);
		send_len += spi_send_single(skb->data + send_len,
									transfer_len);
		remaining_len -= transfer_len;
	}

	up(&spi_dev.power_ctl_sem);
	dev_kfree_skb(skb);
	statistics.tx_pkts++;

	KDEBUG(TRANSPORT, "EXIT");
}


/*!
 * @brief Sends a fragment of a packet with SDIO CMD53.
 *
 * This is where the actual transmission takes places.
 * spi_send_single() sends a fragment of data (<b>buffer</b>) using
 * nano_spi_send_cmd53()
 * 
 * Since the spi_send_single() requires exclusive access to the SPI bus,
 * the body of spi_send_single() is concurrency protected by
 * spi_dev_t::spi_mutex.
 *
 * @param buffer The buffer to transmit
 * @param len The length of the buffer
 *
 * @return The number of bytes that have been transferred
 */
static int spi_send_single(void* buffer, uint32_t len)
{
	int status;

	TRSP_ASSERT(buffer);

	KDEBUG(TRANSPORT, "ENTER: len = %d", len);
	 /* Lock spi */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&spi_dev.spi_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	status = host_send_cmd53((unsigned char *)buffer, len);
	TRSP_ASSERT(status == 0);

	/* Unlock */
	up(&spi_dev.spi_mutex);

	KDEBUG(TRANSPORT, "EXIT");

	return len;
}



/******************************************************************************
F U N C T I O N   D E F I N I T I O N S  ( R X )
******************************************************************************/
/*!
 * @brief This is the interrupt handler function.
 * It should handle the interrupts from the target.
 *
 * When the target generates an interrupt, data is available on the target.
 * Linux workqueue #spi_rx_task is scheduled to acknowledge the interrupt
 * and read the packet in process context.
 * Further SPI interrupts should be disabled until reading has completed.
 */
#if  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void spi_irq (int irq, void *dev_id, struct pt_regs *regs)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
static irqreturn_t spi_irq (int irq, void *dev_id)
#else
static irqreturn_t spi_irq (int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	int status;
		
	GPIO_START_MEASURE();
	KDEBUG(TRANSPORT, "SPI IRQ %d", irq);

	/* phth: We would better assert that target IRQ is pending (IRQ_OP_STATUS) */
	
	statistics.irq_spi++;

	/* Disable interrupts from target, enabled again by recv_task */
	spi_interrupt(IRQ_OP_DISABLE);

	if(atomic_read(&spi_dev.shutdown_flag) == 0) {
		status = NRX_SCHEDULE_WORK(&spi_rx_task);
		if (!status)
			statistics.rx_fail++;
	}
	GPIO_STOP_MEASURE();

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return;
#else
	return IRQ_HANDLED;
#endif
}


/*!
 * @brief Receive next packet.
 *
 * This function should be executed by the Linux workqueue #spi_rx_task that
 * was scheduled by spi_irq(). spi_recv() will read the next packet from the
 * queue in the following way:
 *
 * 1. Acknowledge the interrupt on the target, if necessary.\n
 * 2. Acknowledge the interrupt on the host.\n
 * 3. Read the first XMAC_MIN_SIZE bytes of data using spi_recv_single()
 *    to get the size of the data (first two bytes).\n
 * 4. Read the remaining data in fragments according to the host platform max
 *    transfer size #MAX_TRANSFER_SIZE. Each fragment will be read using
 *    spi_recv_single().\n
 * 5. Schedule workqueue #rx_task that will send the data to the upper
 *    layers, i.e. WiFiEngine.\n
 * 6. Enable target interrupt again.\n
 *
 * @param dummy Optional data (not used)
 */
static void spi_recv(nrx_work_data_t * dummy)
{
	uint8_t noise_packet;
	int status __attribute__((unused));
	struct sk_buff *skb;
	size_t remaining_len, transfer_len;
	unsigned int len;

	GPIO_START_MEASURE();
	TRSP_ASSERT(!in_interrupt());
	KDEBUG(TRANSPORT, "ENTER");

	/* Sync with power control */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&spi_dev.power_ctl_sem);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	statistics.rx_pkts++;

#ifdef USE_SDIO_IRQ
	/* Lock spi */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&spi_dev.spi_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	/* Ack interrupt on target, IRQ line should go inactive again. */
	status = nano_spi_send_cmd52(XMAC_ACKNOWLEDGE_TARGET_IRQ, 0);
	TRSP_ASSERT(status == 0);

	up(&spi_dev.spi_mutex);
#endif /* USE_SDIO_IRQ */

	/* Clear interrupt bit to ack interrupt on host,
	 * this must be done after the interrupt has been acked on the target
	 * or we will get an extra, false, interrupt.
	 */
	spi_interrupt(IRQ_OP_ACKNOWLEDGE);

	/* This buffer size is not really correct, it could probably 
	 * be a little bit tighter.
	 */
	skb = dev_alloc_skb(MAX_PACKET_SIZE);

	TRSP_ASSERT(skb);
	TRSP_ASSERT((unsigned long) skb->data % DMA_ALIGNMENT == 0);
	skb->dev = spi_dev.iface;

	/* Read header */
	spi_recv_single(skb_put(skb, XMAC_MIN_SIZE), XMAC_MIN_SIZE);

	len = ((uint16_t*) skb->data)[0];

	noise_packet = 0;

	/* Four first bytes equal? Then we have a 'noise packet'. */
	if(skb->data[0] == skb->data[1] &&
	   skb->data[0] == skb->data[2] &&
	   skb->data[0] == skb->data[3])
		noise_packet = 1;

	/* Invalid packet size, not necessarily where the first four bytes
	have the same value? Then we have a 'noise packet'. */
	else if (!(5 <= len && len <= MAX_PACKET_SIZE))
		noise_packet = 1;

	if (noise_packet) {

		TRSP_ASSERT(0);
		statistics.noise_pkts++;
#ifdef WIFI_DEBUG_ON
		nano_util_printbuf(skb->data, skb->len, "NOISE");
#endif
		dev_kfree_skb(skb);
		goto exit;
	}

	if (len + XMAC_LEN_FIELD_SIZE > XMAC_MIN_SIZE) 
		remaining_len = len + XMAC_LEN_FIELD_SIZE - XMAC_MIN_SIZE;
	else
		remaining_len = 0;

	/* More data ? Just feeding the spi.*/
	while (remaining_len) {
		if(remaining_len > MAX_TRANSFER_SIZE) 
			transfer_len = MAX_TRANSFER_SIZE;
		else 
			transfer_len = remaining_len;

		TRSP_ASSERT(remaining_len >= transfer_len);
		spi_recv_single(skb_put(skb, transfer_len), transfer_len);
		remaining_len -= transfer_len;
	}

	KDEBUG_BUF(TRANSPORT, skb->data, skb->len, "SPI RX data:");

#ifdef USE_WIFI
	skb_queue_tail(&spi_dev.skb_rx_head, skb);

	if (atomic_read(&spi_dev.shutdown_flag) == 0)
		NRX_SCHEDULE_WORK(&rx_task);
#else
	do_test_rx(skb);
#endif

exit:
	up(&spi_dev.power_ctl_sem);

	/* Enable interrupt from target again */
	spi_interrupt(IRQ_OP_ENABLE);
	GPIO_STOP_MEASURE();
}


/*!
 * @brief Reads a fragment of a packet with SDIO CMD53.
 *
 * This is where the actual reception from the SPI bus takes places.
 * spi_recv_single() receives a fragment of data of a given length (<b>len</b>)
 * into a buffer (<b>buffer</b>) using nano_spi_read_cmd53().
 *
 * Since the spi_recv_single() requires exclusive access to the SPI bus,
 * the body of spi_recv_single() is concurrency protected by
 * spi_dev_t::spi_mutex.
 *
 * @param buffer The buffer which should hold the read data
 * @param len The number of bytes that should be read
 */
static void spi_recv_single(void* buffer, unsigned int len)
{
	int status;

	KDEBUG(TRANSPORT, "ENTER: len = %d", len);

	 /* Lock spi */
	GPIO_STOP_MEASURE();
	status = down_interruptible(&spi_dev.spi_mutex);
	GPIO_START_MEASURE();
	TRSP_ASSERT(status == 0);

	status = host_read_cmd53((unsigned char *)buffer, len);
	TRSP_ASSERT(status == 0);

	/* Unlock */
	up(&spi_dev.spi_mutex);

	KDEBUG(TRANSPORT, "EXIT: len = %d", len);      
}


#ifdef USE_WIFI
/*!
 * @brief Process the list of packets queued by spi_recv().
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
        while((skb = skb_dequeue(&spi_dev.skb_rx_head)) != NULL) {
                ns_net_rx(skb, skb->dev);
        }
        GPIO_STOP_MEASURE();
}
#endif /* USE_WIFI */


/*!
 * @brief Access function for SPI driver statistics through /proc interface.
 */
int spi_read_proc(char* buf, char** start, off_t offset, int count, int* eof, void* data)
{
	int len = 0;

	KDEBUG(TRANSPORT, "ENTER");
	len += sprintf (buf+len, "irq_spi               : %u\n",
					statistics.irq_spi);
	len += sprintf (buf+len, "Tx packets            : %u\n",
					statistics.tx_pkts);
	len += sprintf (buf+len, "Rx packets            : %u\n",
					statistics.rx_pkts);
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
 * This function will be called when the SPI driver module is loaded. Note
 * that the interface layer modules (nano_if.mod) must already be loaded.
 *
 * This function will perform the following steps:
 *
 * 1. Setup GPIO pins on the host.\n
 * 2. Setup internal synchronization primitives such as workqueues and
 *    semaphores.\n
 * 3. Setup interrupt handler.\n
 * 4. Register hooks for firmware download and miscellaneous functions to
 *    upper layers. Firmware download can the be activated from the upper
 *    layers.\n
 * 5. Register /proc-files which will capture statistics of the SPI driver.
 *    These statistics can be used as a debugging aid.\n
 *
 * Note that no messages will be sent to the target board at this time
 * if #nrx_fw_download is non-zero.
 * 
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
int __init spi_init(void)
{
	int status;
	int init_level = 0;
	unsigned long irqflags = 0;

	KDEBUG(TRANSPORT, "ENTER");
        
	GPIO_START_MEASURE();
	atomic_set(&spi_dev.shutdown_flag, 0);
	spin_lock_init(&spi_dev.interrupt_lock);
	sema_init(&spi_dev.power_ctl_sem, 2);
	sema_init(&spi_dev.spi_mutex, 1);
	sema_init(&spi_dev.exit_sem, 0);
	memset(&statistics, 0, sizeof(statistics));

	NRX_INIT_WORK(&spi_rx_task, spi_recv, NULL);

#ifdef USE_WIFI

	spin_lock_init(&spi_dev.send_lock);
	NRX_INIT_WORK(&rx_task, do_rx_queue, NULL);
	NRX_INIT_WORK(&tx_task, do_tx_queue, NULL);
   
#ifdef USE_POWER_MANAGEMENT
	NRX_INIT_WORK(&sleep_task, do_sleep, NULL);
	NRX_INIT_WORK(&wakeup_task, do_wakeup, NULL);
#endif /* USE_POWER_MANAGEMENT */

#endif /* USE_WIFI */

	status = host_init();
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 1);

#ifdef USE_DEBUG_PINS
	printk("nano_spi: *** Debug pins active ***\n");
#endif
	host_debug_pins(GPIO_OP_ENABLE, 0xffffffff);
	host_debug_pins(GPIO_OP_LOW, 0xffffffff);
	if (INIT_DEBUG_PINS != 0)
		host_debug_pins(GPIO_OP_HIGH, INIT_DEBUG_PINS);
	
#ifdef USE_GPIO_CPU_MEASURE
	host_gpio(GPIO_OP_ENABLE, GPIO_ID_CPU_LOAD);
	host_gpio(GPIO_OP_LOW, GPIO_ID_CPU_LOAD);
#endif

	host_gpio(GPIO_OP_ENABLE, GPIO_ID_POWER);
	host_gpio(GPIO_OP_LOW, GPIO_ID_POWER);

	host_gpio(GPIO_OP_ENABLE, GPIO_ID_RESET);
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET);

	/* Disable and clear interrupt from target before handler registration
	   to avoid spurious interrupts. */
	status = spi_interrupt(IRQ_OP_DISABLE);
	if (status) goto exit;
	status = spi_interrupt(IRQ_OP_ACKNOWLEDGE);
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 2);

	/* Request irq and register handler. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
	irqflags |= SA_INTERRUPT;
#endif
	status = request_irq (TARGET_IRQ_NUM, spi_irq, irqflags,
						  "spi_irq", &spi_dev);
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 3);

	if (!nrx_fw_download) {
		spi_reset_target();
		status = spi_init_target();
		if (status) goto exit;
	}
	TRSP_ASSERT(++init_level == 4);

#ifdef USE_WIFI
	skb_queue_head_init(&spi_dev.skb_tx_head);
	skb_queue_head_init(&spi_dev.skb_rx_head);

	/* Create network device */
	create_param.size = sizeof(create_param);
	create_param.send = wifi_send;
	create_param.fw_download = nrx_fw_download ? spi_fw_download : NULL;
	create_param.control = spi_control;
	create_param.params_len = host_get_target_params(&create_param.params_buf);
	create_param.min_size = XMAC_MIN_SIZE;
	create_param.size_align = XMAC_ALIGNMENT;
	create_param.header_size = XMAC_HEADER_SIZE;
#ifdef USE_SDIO_IRQ
	create_param.host_attention = HIC_CTRL_ALIGN_HATTN_VAL_POLICY_NATIVE_SDIO;
#else /* !USE_SDIO_IRQ */
	create_param.host_attention = HIC_CTRL_ALIGN_HATTN_VAL_POLICY_GPIO
								| HIC_CTRL_ALIGN_HATTN_VAL_OVERRIDE_DEFAULT_PARAM
								| (11 << HIC_CTRL_ALIGN_HATTN_OFFSET_GPIOPARAMS_GPIO_ID);
#endif /* USE_SDIO_IRQ */
	create_param.byte_swap_mode = HIC_CTRL_ALIGN_SWAP_NO_BYTESWAP;
	create_param.host_wakeup = HIC_CTRL_ALIGN_HWAKEUP_NOT_USED;
	create_param.force_interval = HIC_CTRL_ALIGN_FINTERVAL_NOT_USED;	
        create_param.tx_headroom = 0;

	spi_dev.iface = nanonet_create(NULL, NULL, &create_param);
	status = spi_dev.iface ? 0 : -EPERM;
	if (status) goto exit;
	TRSP_ASSERT(++init_level == 5);

#else /* !USE_WIFI */
	kernel_thread(do_test, 0, 0);
#endif /* USE_WIFI */

	/* Initialize /proc interface. In case initialization fails,
	   this module will be loaded and operate without /proc interface. */
	{
		struct proc_dir_entry *proc_entry;
		
		spi_dev.proc_dir = proc_mkdir("nanospi", NULL);
		TRSP_ASSERT(spi_dev.proc_dir);
		proc_entry = create_proc_read_entry("statistics",
											0, /* Default mode */
											spi_dev.proc_dir, /* Parent dir */
											spi_read_proc,
											NULL);
		TRSP_ASSERT(proc_entry);
	}

exit:
	if (status) {
		printk("nano_spi: *** init failed, init_level = %d\n", init_level);
		switch (init_level) {
			case 4:
			case 3:
				spi_interrupt(IRQ_OP_DISABLE);
				free_irq(TARGET_IRQ_NUM, &spi_dev);
			case 2:
			case 1:
				host_exit();
				spi_shutdown_target();
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
 * spi_dev_t::exit_sem to sync with shutdown completion of upper layers.
 *
 * After the upper layers are disconnected then any resources used by the SPI
 * driver will be returned and target interrupts will be disabled.
 * GPIO pins will be disabled.
 */
void __exit spi_exit(void)
{
#ifdef USE_WIFI
	int status;
	struct sk_buff *skb;
#endif

	KDEBUG(TRANSPORT, "ENTER");

#ifdef USE_WIFI
	/* nanonet_destroy() eventually calls spi_control(NANONET_SHUTDOWN, ...)
	   exit_sem will signal shutdown completion */
	nanonet_destroy(spi_dev.iface);
	status = down_interruptible(&spi_dev.exit_sem);
	TRSP_ASSERT(status == 0);

	while ((skb = skb_dequeue(&spi_dev.skb_rx_head)) != NULL)
		dev_kfree_skb_any(skb);
	while ((skb = skb_dequeue(&spi_dev.skb_tx_head)) != NULL)
		dev_kfree_skb_any(skb);
#endif /* USE_WIFI */

	remove_proc_entry("statistics", spi_dev.proc_dir);
	remove_proc_entry("nanospi", NULL);

	free_irq(TARGET_IRQ_NUM, &spi_dev);

	host_debug_pins(GPIO_OP_DISABLE, 0xffffffff);

	KDEBUG(TRANSPORT, "EXIT");
}


#ifdef USE_FW_DOWNLOAD
/*!
 * @brief Downloads firmware on the target.
 *
 * Callback function that is invoked when firmware download is started.
 *
 * This function will properly reset and setup the target and transfer the
 * firmware buffer to the target.
 *
 * When the firmware has been transferred, the target will be reset in
 * order to send initalization commands again to the target with the
 * new firmware running.
 * 
 * @param fw_buf The buffer containing the firmware image
 * @param fw_len The size of the firmware image
 * @param data Optional data (not used)
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */ 
static int spi_fw_download(const void *fw_buf, size_t fw_len, void *data)
{
	int status;
	size_t transfer_len, remaining_len;
	size_t send_len = 0;
	u32 *buf_virt;   
#ifdef WIFI_DEBUG_ON
	int nrx_debug_saved = nrx_debug;
#endif
        
	KDEBUG(TRANSPORT, "ENTER: fw_len=%d", fw_len);
	GPIO_START_MEASURE();

	spi_reset_target();
	status = spi_init_target();
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
	        if(remaining_len > 16) 
			transfer_len = 16;
		else 
			transfer_len = remaining_len;

		TRSP_ASSERT(remaining_len >= transfer_len);
		memcpy(buf_virt, fw_buf + send_len, transfer_len);
		send_len += spi_send_single(buf_virt, transfer_len);
		remaining_len -= transfer_len;
	}
#ifdef WIFI_DEBUG_ON
	nrx_debug = nrx_debug_saved;
#endif

	kfree(buf_virt);

	/* Wait for target to boot */
	mdelay(10);

	status = host_send_cmd0();

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
 * The target will be reset when this function returns, provided that 
 * <b>JTAG_CONNECTION</b> is not defined. If <b>JTAG_CONNECTION</b> is
 * defined, this function will do nothing.
 */
static void spi_reset_target(void)
{
   int status;

   KDEBUG(TRANSPORT, "INIT");

#ifndef JTAG_CONNECTION
   host_gpio(GPIO_OP_LOW, GPIO_ID_POWER);
   host_gpio(GPIO_OP_LOW, GPIO_ID_RESET);

   mdelay(100);
   host_gpio(GPIO_OP_HIGH, GPIO_ID_POWER);
   mdelay(10);
   host_gpio(GPIO_OP_HIGH, GPIO_ID_RESET);
   mdelay(250);
#endif

	/* If GPIO reset is not available, write 0x00 to register 0xF5
	 * to reset chip (same as asserting RESET)
	 */
	status = host_send_cmd52(XMAC_RESET, 0);
	TRSP_ASSERT(status == 0);

   KDEBUG(TRANSPORT, "EXIT");
}


/*!
 * @brief Initializes target interrupt.
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
static int spi_init_target_irq(void)
{
#ifdef USE_SDIO_IRQ
	int status;
	
	status = nano_spi_send_cmd52(XMAC_ENABLE_SDIO_IRQ, 0);
	TRSP_ASSERT(status == 0);
	if (status) return status;
	
	status = nano_spi_send_cmd52(XMAC_NO_CLOCK_SDIO_IRQ, 0);
	TRSP_ASSERT(status == 0);
	if (status) return status;

#if 0 /* The target does not support this feature, this would fail. */
	status = nano_spi_send_cmd52(XMAC_NO_CHIP_SEL_SDIO_IRQ, 0);
	TRSP_ASSERT(status == 0);
	if (status) return status;
#endif
#endif /* USE_SDIO_IRQ */

	return 0;
}


/*!
 * @brief Initializes target after reset.
 *
 * This function will:
 *
 * 1. Send SDIO CMD0 to set the target in SPI mode. This is what
 *    usually fails if there is something wrong with the hardware or
 *    the way it has been setup. In case of CMD0 failure, this function will
 *    repeat CMD0 five times before failing.\n
 * 2. Call spi_init_target_irq() to setup interrupt on target.\n
 * 3. Enable target interrupt on host.\n
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
static int spi_init_target(void)
{
	int count, status;

	KDEBUG(TRANSPORT, "ENTER");

	count = 5, status = -EIO;
	do {
		status = host_send_cmd0();
		if (status)
			spi_reset_target();

		count--;
	} while (status && count > 0);

	if (status == 0) {
		status = spi_init_target_irq();
		spi_interrupt(IRQ_OP_ENABLE);
	}
	else
		TRSP_MSG("SPI Card Initialization Failed, status = %d\n", status);

	KDEBUG(TRANSPORT, "EXIT");
	return status;
}


/*!
 * @brief Configure target interrupt in a concurrency protected way.
 *
 * @param irq_op The interrupt operation to perform, see host.h for more
 *               information
 *
 * @return See return value of host_interrupt().
 */
static uint32_t spi_interrupt(irq_op_t irq_op)
{
	uint32_t status;
	unsigned long flags;

	spin_lock_irqsave(&spi_dev.interrupt_lock, flags);
	status = host_interrupt(irq_op);
	spin_unlock_irqrestore(&spi_dev.interrupt_lock, flags);
	return status;
}


/*!
 * @brief Does target power down.
 *
 * Called during module exit. Asserts reset and removes target power
 * and then disables GPIOs driving those signals.
 * Does not affect #GPIO_ID_CPU_LOAD.
 */
static void spi_shutdown_target(void)
{
	host_gpio(GPIO_OP_LOW, GPIO_ID_RESET);
	host_gpio(GPIO_OP_LOW, GPIO_ID_POWER);
	host_gpio(GPIO_OP_DISABLE, GPIO_ID_POWER);
	host_gpio(GPIO_OP_DISABLE, GPIO_ID_RESET);
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
static int spi_control_sleep(uint32_t mode, void *data)
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
static int spi_control_sleep(uint32_t mode, void *data)
{	
	return -EOPNOTSUPP;
}
#endif /* USE_POWER_MANAGEMENT, USE_WIFI */


#ifdef USE_POWER_MANAGEMENT
/*!
 * @brief Enables target's power save mode.
 *
 * Sends appropriate SDIO CMD52 to enable target's power save mode.
 * It is executed by workqueue #sleep_task.
 *
 * @param data Optional data (not used)
 */
static void do_sleep(nrx_work_data_t *data)
{
	int status;
	
	/* Wait for send & recv activity completion and block it. */
	status = down_interruptible(&spi_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);
	status = down_interruptible(&spi_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);
	
	/* After locking power_ctl_sem, locking spi shouldn't be necessary. */

	/* Going from active to doze mode */
	statistics.sleep++;
	status = host_send_cmd52(XMAC_ENABLE_SLEEP, 0);
	TRSP_ASSERT(status == 0);

	/* Unlock */
	up(&spi_dev.power_ctl_sem);
	up(&spi_dev.power_ctl_sem);
}


/*!
 * @brief Disables target's power save mode.
 *
 * Sends appropriate SDIO CMD52 to enable target's power save mode.
 * Slow SPI clock must be used to ensure reliable target wakeup
 * if the target is sleeping, see also nano_spi_send_cmd52().
 * It is executed by workqueue #wakeup_task.
 *
 * @param data Optional data (not used)
 */
static void do_wakeup(nrx_work_data_t *data)
{
	int status;
	
	/* Wait for send & recv activity completion and block it. */
	status = down_interruptible(&spi_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);
	status = down_interruptible(&spi_dev.power_ctl_sem);
	TRSP_ASSERT(status == 0);

	/* After locking power_ctl_sem, locking spi shouldn't be necessary. */

	/* Going from doze to active mode 
	 * use the slow clock to wake up the target.
	*/
	statistics.wakeup++;
	status = host_send_cmd52(XMAC_DISABLE_SLEEP, 1);
	TRSP_ASSERT(status == 0);

   /* Unlock */
	up(&spi_dev.power_ctl_sem);
	up(&spi_dev.power_ctl_sem);      
}
#endif /* USE_POWER_MANAGEMENT */


#ifdef USE_WIFI
/* phth: is this used anymore? */
static int spi_control_boot(uint32_t mode, void *data)
{
   KDEBUG(TRACE, "ENTRY");
   switch(mode) {
      case NANONET_BOOT_TEST:
         return 0;
      case NANONET_BOOT_ENABLE:
         spi_dev.is_booting = 1;
         return 0;
      case NANONET_BOOT_DISABLE:
         spi_dev.is_booting = 0;
         return 0;
   }
   return -EINVAL;
}


/*!
 * @brief Callback for misc transport functions.
 *
 * @param command Control command
 * @param mode The control command mode (enable/disable)
 * @param data Optional data (not used)
 *
 * @return
 * - 0 on success
 * - appropriate negative error code on failure
 */
static int spi_control(uint32_t command, uint32_t mode, void *data)
{
	KDEBUG(TRANSPORT, "ENTRY: %u:%u", command, mode);

	switch(command) {
		case NANONET_SLEEP:
			return spi_control_sleep(mode, data);

		case NANONET_BOOT:
			return spi_control_boot(mode, data);

		case NANONET_SHUTDOWN:
			atomic_set(&spi_dev.shutdown_flag, 1);
			NRX_FLUSH_SCHEDULED_WORK();

			/* we have no outstanding tasks, so disable interrupts, and
			 * terminate target */
			spi_interrupt(IRQ_OP_DISABLE);
			host_exit();
			spi_shutdown_target();

			/* let spi_exit() complete */
			up(&spi_dev.exit_sem);
			return 0;

		default:
			break;
   }
   return -EOPNOTSUPP;
}
#endif /* USE_WIFI */


#ifndef USE_WIFI

#define LOOPBACK_SEMA_INIT				3
#define LOOPBACK_HIC_CTRL_ALIGN_HATTN	0x5a
#define LOOPBACK_FW_DOWNLOAD		spi_fw_download
#define LOOPBACK_SEND				spi_send
#define LOOPBACK_INTERRUPT(x)		spi_interrupt(x)
#define LOOPBACK_SHUTDOWN_TARGET	spi_shutdown_target
#include "../loopback/loopback.c"

#endif /* USE_WIFI */

module_init(spi_init);
module_exit(spi_exit);


/*! @} */ /* End of spi_generic group */
