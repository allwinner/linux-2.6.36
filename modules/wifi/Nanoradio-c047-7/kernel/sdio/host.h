
/** @defgroup sdio_host SDIO host controller interface
 *
 * @brief SDIO host controller functions.
 *
 * Declaration of SDIO host interface that should be implemented for the
 * host platform.
 * 
 * The following defines can be used by the host to configure the
 * sdio driver (put in host-specific <b><i>host_def.h</i></b>):
 *
 * <b>USE_WIFI</b>\n
 * When defined, connect to upper layers (i.e. the WiFiEngine).\n
 * When not defined, perform self test using target loopback mode.
 * 
 * <b>USE_POWER_MANAGEMENT</b>\n
 * Control target's power save mode, see sdio_control_sleep().
 *
 * <b>USE_GPIO_CPU_MEASURE</b>\n
 * Use a host GPIO to measure CPU load caused by the driver,
 * see #GPIO_ID_CPU_LOAD.
 *
 * <b>USE_NO_CLOCK</b>\n
 * Disable SDIO bus clock when the bus is inactive.
 * 
 * <b>USE_FW_DOWNLOAD</b>\n
 * Allow firmware download after driver initialization.
 * See also #nrx_fw_download and sdio_fw_download().
 *
 * <b>USE_TARGET_PARAMS</b>\n
 * Allows custom target configuration for each platform.
 * When defined, host_get_target_params() must be defined in host.c
 * and will be called to get the target configuration data.
 *
 * <b>USE_4BIT_SDIO</b>\n
 * Allow 4-bit SDIO mode. Not applicable to all host controllers.
 * See also #nrx_4bit_sdio and host_init().
 * 
 * <b>USE_CMD3_TEST</b>\n
 * During sdio_init_target() execution, CMD3 is sent twice for self test.
 * Not applicable to all host controllers.
 *
 * <b>USE_IF_REINIT</b>\n
 * Support SDIO interface power down (after an inactivity period)
 * and reinitialization.
 *
 * <b>USE_FAST_BOOT</b>\n
 * Issue short (~1ms) reset pulses instead of standard reset pulses
 * during sdio_reset_target() execution.
 *
 * <b>USE_NO_MSLEEP</b>\n
 * Implement <b>mlseep()</b> instead of using linux kernel's mlseep().
 *
 * <b>USE_DMA</b>\n
 * Use DMA to send/receive data to/from host controller. Affects only
 * host-specific code (host.c), not applicable to all host controllers.
 *
 * <b>USE_NO_CACHE_COHERENCY</b>\n
 * Do not map <b>data</b> to <b>data_phys</b> before calling host_fifo().
 *   
 * <b>USE_IRQ_ID_RESPONSE_COMPLETION</b>\n
 * <b>USE_IRQ_ID_FIFO_READ_READY</b>\n
 * <b>USE_IRQ_ID_FIFO_WRITE_READY</b>\n
 * <b>USE_IRQ_ID_RW_ACCESS_COMPLETION</b>\n
 * <b>USE_IRQ_ID_IF_REINIT</b>\n
 * Using each host controller IRQ source is optional. See #irq_id_t.
 *
 * <b>HOST_SDIO_IRQ_NUM <i>irq_number</i></b>\n
 * Defines SDIO host controller IRQ number.
 *
 * <b>MAX_TRANSFER_SIZE <i>max_payload_size</i></b>\n
 * When defined, limits CMD53 payload size to <b><i>max_payload_size</i></b>.
 *
 * <b>USE_DEBUG_PINS</b>\n
 * When defined, host_debug_pins() must be implemented, as it will be called
 * when driver is loaded / unloaded.\n
 * When not defined, host_debug_pins() degenerates to an empty inline.
 *
 * <b>INIT_DEBUG_PINS <i>init_state_mask</i></b>\n
 * When defined, it will be used as a mask of debug ping to be set to high
 * on driver loading.
 *
 *  @{
 */

#include <linux/types.h>
#include <host_def.h>


/******************************************************************************
D E F I N E S
******************************************************************************/

#ifndef HOST_SDIO_IRQ_NUM
#error Please define HOST_SDIO_IRQ_NUM
#endif

#ifndef USE_WIFI
#undef USE_POWER_MANAGEMENT
#undef USE_IF_REINIT
#endif

#ifdef USE_IF_REINIT

/*! Customer implemented functions for turning the power to the SDIO
 *  interface on/off.  The function will be called while holding a
 *  semaphore so it does not have to (and cannot) lock.
 */
void host_if_power_on(void);
void host_if_power_off(void);

#endif

/*! Maximum CMD53 payload size */
#ifndef MAX_TRANSFER_SIZE
#define MAX_TRANSFER_SIZE   512
#endif

/*! Initial state of debug pins (all LOW by default) */
#ifndef INIT_DEBUG_PINS
#define INIT_DEBUG_PINS  0x00000000
#endif

/*! Max number of loops during busy wait */
#ifndef BUSYWAIT_LOOP_MAX
#define BUSYWAIT_LOOP_MAX	1000
#endif


/******************************************************************************
C O N S T A N T S / M A C R O S / T Y P E S
******************************************************************************/

#ifdef WIFI_DEBUG_ON
#define TRSP_ASSERT(EXPR)  do { if(!(EXPR)) { printk("%s:%d: ASSERTION FAILED\n", __func__, __LINE__); } } while(0);
#define TRSP_MSG(msg, ...) do { printk("%s:%d:" msg, __func__, __LINE__ , ##__VA_ARGS__); } while(0);
#else
#define TRSP_ASSERT(EXPR)  {}
#define TRSP_MSG(msg, ...) {}
#endif


/*!
 * Transfer mode (1 or 4 bit SDIO).
 *
 */
typedef enum {
	TRANSFER_MODE_1BIT_SDIO = 0,/*!< */
	TRANSFER_MODE_4BIT_SDIO		/*!< */
} transfer_mode_t;

/*!
 * SDIO Commands
 *
 */
typedef enum {
	SD_SEND_RELATIVE_ADDR    = 0x3,	/*!< */
	SD_SELECT_DESELECT_CARD  = 0x7,	/*!< */
	SDIO_RW_DIRECT           = 0x34,/*!< */
	SDIO_RW_EXTENDED         = 0x35	/*!< */
} cmd_t;


/*!
 * Flags for data sending. Issued for function host_fifo().
 *
 */
enum {

	/*! Data should be transferred from target to host (read). */
	FIFO_FLAG_TO_HOST = 0x01    
};


/*!
 * Flags for command setup. Issued for function host_cmd().
 *
 */
enum {

	/*! This flag is set if multi-block transfer should be used instead of
	 *  single-block transfer. Multi-block transfer is currently not
	 *  supported by target.
	 */
	CMD_FLAG_MULTI_BLOCK_TRANSFER = 0x01,

	/*! Set if data should be read from target to host. */
	CMD_FLAG_DIR_DATA_TO_HOST = 0x02,

	/*! Set if data port is active, e.g if the command will result in
	 *  reading or writing data.
	 */
	CMD_FLAG_DATA_EXISTS = 0x04,

	/*! Set for security (SDIO) commands. */
	CMD_FLAG_SECURITY = 0x08,

	/*! Set busy flag */
	CMD_FLAG_BUSY = 0x10
};




/*!
 * Command response types. Indicates what kind of response that is expected for
 * a command. Issued for function host_cmd().
 *
 */
typedef enum {

	/*! No response */
	CMD_RESP_NONE,

	/*! R1 */
	CMD_RESP_R1,

	/*! R2 */
	CMD_RESP_R2,

	/*! R3 */
	CMD_RESP_R3,

	/*! R4 */
	CMD_RESP_R4,

	/*! R5 */
	CMD_RESP_R5,

	/*! R6 */
	CMD_RESP_R6

} cmd_resp_t;



/*!
 * Command types.
 *
 */
typedef enum {

	/*! Broadcast command with response */
	CMD_TYPE_BCR,

	/*! Addressed command */
	CMD_TYPE_AC,

	/*! Addressed with data */
	CMD_TYPE_ADTC

} cmd_type_t;


/*!
 * Interrupt line operations. Issued for function host_interrupt().
 *
 */
typedef enum {
	
	/*! Enable interrupt */
	IRQ_OP_ENABLE,

	/*! Disable interrupt */
	IRQ_OP_DISABLE,

	/*! Get interrupt status, used to determine which interrupts that are
	 * currently pending.
	 */
	IRQ_OP_STATUS,

	/*! Acknowledge interrupt */
	IRQ_OP_ACKNOWLEDGE
	
} irq_op_t;


/*!
 * Interrupt line id's. Issued for function host_interrupt().
 *
 */
typedef enum {

	/*! This interrupt should be generated when a command response have
	 *  been received, i.e. any command issuing can be considered finished.
	 *  Will only be used if USE_IRQ_ID_RESPONSE_COMPLETION is defined.
	 */
	IRQ_ID_RESPONSE_COMPLETION,

	/*! This interrupt should be generated when the fifo holds data which
	 *  can be read out by the host, i.e. this must happen before any
	 *  CPU or DMA read transfer starts.
	 *  Will only be used if USE_IRQ_ID_FIFO_READ_READY is defined.
	 */
	IRQ_ID_FIFO_READ_READY,

	/*! This interrupt should be generated when the fifo is ready to
	 *  receive data which will be written from the host, i.e. this must
	 *  happen before any CPU or DMA write transfer starts.
	 *  Will only be used if USE_IRQ_ID_FIFO_WRITE_READY is defined.
	 */        
	IRQ_ID_FIFO_WRITE_READY,

	/*! This interrupt should be generated when the target has any data
	 *  which can be read.
	 */        
	IRQ_ID_TARGET,               

#ifdef USE_IF_REINIT
	/*! This interrupt should be generated when the SDIO interface
	 *  has been powered on by external activity.
	 */
	IRQ_ID_IF_REINIT,
#endif

	/*! This interrupt should be generated when fifo read/write operation
	 *  is finished. This interrupt should be generated after the DMA or
	 *  CPU transfer is finished.
	 *  Will only be used if USE_IRQ_ID_RW_ACCESS_COMPLETION is defined.
	 */        
	IRQ_ID_RW_ACCESS_COMPLETION
	
} irq_id_t;



/*!
 * Interrupt handler return codes.
 * Return type for functions host_interrupt(), sdio_interrupt() etc.\n
 * <b>CAUTION:</b> Do not change values of IRQ_STATUS_FAIL, IRQ_STATUS_SUCCESS,
 * IRQ_STATUS_INACTIVE or IRQ_STATUS_ACTIVE.
 */
typedef enum {

	/*! Returned if function has been called to enable, disable or clear
	 *  an interrupt and this operation has failed.
	 */
	IRQ_STATUS_FAIL = -1,

	/*! Returned if function has been called to enable, disable or clear
	 *  an interrupt and this operation has been successful.
	 */
	IRQ_STATUS_SUCCESS = 0,

	/*! Returned if function has been called to check interrupt status
	 *  and the interrupt has been found not pending.
	 *  Is equal to #IRQ_STATUS_SUCCESS.
	 */
	 IRQ_STATUS_INACTIVE = 0,

	/*! Returned if function has been called to check interrupt status,
	 *  the interrupt is pending
	 *  and no error has occured at the SDIO controller.
	 */
	 IRQ_STATUS_ACTIVE,
	
	/*! Returned if function has been called to check interrupt status,
	 *  the interrupt is pending
	 *  and the SDIO controller indicates bus CRC error.
	 */
	 IRQ_STATUS_CRC,
	 
	/*! Returned if function has been called to check interrupt status,
	 *  the interrupt is pending
	 *  and the SDIO controller indicates bus timeout error.
	 */
	 IRQ_STATUS_TIMEOUT,
	 
	/*! Returned if function has been called to check interrupt status,
	 *  the interrupt is pending
	 *  and the SDIO controller indicates bus conflict error.
	 */
	 IRQ_STATUS_CONFLICT,

	/*! Returned if function has been called to check interrupt status,
	 *  the interrupt is pending
	 *  and the SDIO controller indicates fifo overrun or underrun.
	 */
	 IRQ_STATUS_FIFOERR

} irq_status_t;



/*!
 * Enumeration of different misc operations on SDIO host.
 * Issued for function host_control().
 *
 */
typedef enum {

	/*! Set lengthof next data transfer, usually issued before CMD53. */
	CTL_ID_LENGTH,

	/*! Wait until command line is idle. */
	CTL_ID_WAIT_CMD_READY,

	/*! Read command response. The host should block until the response is
	 * available.
	 */
	CTL_ID_RESPONSE

} ctl_id_t;


/*!
 * Enumeration of clock modes. Issued for function host_clock().
 *
 */
typedef enum {

	/*! Turn off clock. */
	CLK_MODE_OFF,

	/*! Set clock speed to slow (~300KHz). */
	CLK_MODE_SLOW,

	/*! Set clock speed to fast (MHz region). */
	CLK_MODE_FAST

} clk_mode_t;


/*!
 * Enumeration of GPIO pin id's. Issued for function host_gpio().
 *
 */
typedef enum {

	/*!
	 * GPIO pin for measuring CPU load.
	 * Kept high while this driver utilizes the CPU.
	 */
	GPIO_ID_CPU_LOAD,

	/*! GPIO pin for target analog reset. */
	GPIO_ID_RESET_A,

	/*! GPIO pin for target digital reset. */
	GPIO_ID_RESET_D,

	/*! GPIO pin for target power. */
	GPIO_ID_POWER,

} gpio_id_t;


/*!
 * Enumeration of operations that can be performed on GPIO pin id's.
 * Issued for functions host_gpio() and host_debug_pins().
 *
 */
typedef enum {

	/*! Initialie the GPIO pin to an output pin. */
	GPIO_OP_ENABLE,

	/*! Set the GPIO pin to high. */
	GPIO_OP_HIGH,

	/*! Set the GPIO pin to low. */
	GPIO_OP_LOW,

	/*! Disable/Unregister the GPIO pin */
	GPIO_OP_DISABLE

} gpio_op_t;





/******************************************************************************
F U N C T I O N  P R O T O Y P E S
******************************************************************************/


/*!
 * @brief This function should be called when data transfer
 * (either read or write) is completed.
 *
 * It should be called in both DMA and CPU modes.
 *
 */
extern void sdio_fifo_callback(void);


/*!
 * @brief Initialize host.
 *
 * Will only be called once. This function should typically do memory
 * allocations etc.
 *
 * In most cases, most of the intialization of the SDIO controller itslef will
 * be performed by host_reset().
 *
 * @param transfer_mode Defines transfer mode to use, see enum #transfer_mode_t
 *
 * @return
 * - 0 on SUCCESS
 * - appropriate negative error code on FAILURE
 */
int host_init(transfer_mode_t transfer_mode);


/*!
 * @brief Reset SDIO host controller.
 *
 * Will be called whenever the SDIO controller on the host should reset, e.g.
 * before and after firmware is downloaded to the target.
 *
 * Note that this function can be called several times.
 *
 * When the function returns the following must hold:
 * - All interrupts should be disabled.
 * - Any internal pull-ups on the SDIO port should be enabled or pull-ups
 *   should be physically connected on the target or cable.
 * - Transfer width should be set according to transfer_mode parameter
 *   passed during host_init() execution.
 * - In practice, the SDIO controller should be ready to send commands and
 *   data.
 * - If clock toggling is performed by the SDIO controller hardware, then the
 *   SDIO clock can be initialized and started here.
 *
 * No CMD3 etc should be sent by this function, that will be handled by the
 * generic SDIO driver code.
 *
 * @return
 * - 0 on SUCCESS
 * - appropriate negative error code on FAILURE
 */
int host_reset(void);


/*!
 * @brief Cleanup SDIO controller.
 *
 * Hook for performing any neccesary cleanup for host code. Will only be called
 * when driver is unloaded
 *
 */
void host_exit(void);


/*!
 * @brief Control function for SDIO clock.
 *
 * This function is called when the clock speed should change or if the clock
 * should be disabled.
 *
 * Clock speed changes are performed by the generic SDIO driver to lower the
 * power consumption during power save mode. An intermediate, low, clock
 * frequency is used to avoid the direct step from disabled clock to full
 * speed clock.
 *
 * If the SDIO controller hardware handles clock enable/disable automatically
 * this function can be left empty and the clock can be initialized and started
 * in host_reset() instead.
 *
 * @param clk_mode Defines the new clock mode.
 *
 */
void host_clock(clk_mode_t clk_mode);


/*!
 * @brief Control function for misc GPIO pins.
 *
 * This function will be used to control optional GPIO-pins used for target
 * reset or shutdown. If reset or shutdown (or both) connections are missing,
 * those cases can of course be left unimplemented.
 * 
 * @param gpio_op Defines an operation to be performed on a GPIO pin.
 *          	  #GPIO_OP_ENABLE will be issued before any other operation
 *				  for each GPIO pin id.
 * @param gpio_id Defines which GPIO pin to perform the operation on.
 *
 */
void host_gpio(gpio_op_t gpio_op, gpio_id_t gpio_id);


/*!
 * @brief Control function for debug pins.
 *
 * This function will be used to control optional GPIO-pins used to
 * generate debug signals. 
 * 
 * @param gpio_op Defines an operation to be performed on a debug pin.
 *          	  #GPIO_OP_ENABLE will be issued before any other operation
 *				  for each GPIO pin id.
 * @param mask	  Defines which debug pin(s) to perform the operation on.
 *
 */
#ifdef USE_DEBUG_PINS
void host_debug_pins(gpio_op_t gpio_op, uint32_t mask);
#else
inline static void host_debug_pins(gpio_op_t gpio_op, uint32_t mask) {}
#endif


/*! 
 * @brief Control function for interrupts.
 *
 * This function will be used to control interrupt signals generated by
 * the SDIO host controller. Only IRQ_IDs defined in <b><i>host_def.h</i></b>
 * will be used.
 *
 * This function will be called when interrupts should be enabled,
 * disabled, acknowledged or checked.
 * - After enabling interrupt source <b>irq_id</b> with #IRQ_OP_ENABLE,
 *   this source can generate interrupt request on line
 *   <b>HOST_SDIO_IRQ_NUM</b>.
 * - After an interrupt has been generated, status check with #IRQ_OP_STATUS
 *   should return values greater than #IRQ_STATUS_INACTIVE.
 * - New interrupts from the same source should not be generated
 *   until the pending interrupt request has been acknowledged.
 *   After pending request acknowledgement with #IRQ_OP_ACKNOWLEDGE,
 *   status check must return 0 until a new interrupt request is generated.
 * - If an interrupt source has been disabled with #IRQ_OP_DISABLE,
 *   it should not generate an interrupt request before re-enable.
 *
 * @return See definition of enum #irq_status_t 
 *
 * @param irq_op Defines the operation to be performed on an interrupt source.
 * @param irq_id Defines which interrupt source to perform the operation on.
 */
irq_status_t host_interrupt(irq_op_t irq_op, irq_id_t irq_id);

/*!
 * @brief Control function for data transfer.
 *
 * Transfer data between data buffer and SDIO controller FIFO.
 * This function can decide how data transfer should be done, e.g using CPU or
 * DMA. This decision will usually be based on the alignment and size of data.
 *
 * In case of DMA transfers the function can return before the data transfer is
 * finished.
 *
 * When the transfer is done, function sdio_fifo_callback() must be called. In
 * the case of a DMA transfer this function will typically be called from a DMA
 * soft interrupt. In the case of CPU transfer, the function should be called
 * explicitly before the function returns, hence the data transfer will be
 * blocking.
 *
 * @param data The data buffer which holds the data to be written to the
 *             FIFO (in case the flag #FIFO_FLAG_TO_HOST is not set).
 *             The data buffer which will hold the data read from the FIFO (in
 *             case the flag #FIFO_FLAG_TO_HOST is set).
 * @param data_phys The physical address of the data buffer (To be used in DMA 
 *                  transfer system calls). Value passed will be valid only if
 *					USE_NO_CACHE_COHERENCY is not defined.
 * @param len Number of bytes to read or write.
 * @param flags If the bit #FIFO_FLAG_TO_HOST is set then data should be read,
 *              otherwise written.
 *
 */
int host_fifo(void *data, unsigned long data_phys, uint16_t len, uint32_t flags);

/*!
 * @brief Issue a SDIO command
 *        and wait for response, if IRQ_ID_RESPONSE_COMPLETION is not used.
 * 
 * @param cmd The command number.
 * @param cmd_type The command type.
 * @param cmd_resp The command response type.
 * @param flags Command flags.
 * @param arg Command argument value.
 * @param data Pointer to data that will be transferred (in case of CMD53).
 *             This pointer can be used to determine if DMA should be set up
 *             or not before host_fifo() is actually called.
 *
 * @return 
 * - #IRQ_STATUS_FAIL if command issuing failed
 * - If IRQ_ID_RESPONSE_COMPLETION is used:
 *	 	#IRQ_STATUS_SUCCESS if command has been issued normally
 * - If IRQ_ID_RESPONSE_COMPLETION is not used:
 *		#IRQ_STATUS_SUCCESS if reponse has been received successfully,
 *		other #irq_status_t values in case of bus error (see enum definition)
 *
 */
irq_status_t host_cmd(cmd_t cmd, cmd_type_t cmd_type, cmd_resp_t cmd_resp,
					  uint32_t flags, uint32_t arg, void* data);


/*!
 * @brief Control function for misc SDIO operations.
 *
 * @param ctl_id The function id.
 * @param param  Optional argument which is function id-specific.
 *
 * @return		 Depends on <b>ctl_id</b>
 */
uint32_t host_control(ctl_id_t ctl_id, uint32_t param);


/*! 
 * @brief Passes target configuration parameters for the platform.
 *
 * This function must be defined only when USE_TARGET_PARAMS is defined.
 *
 * @param params_buf Used to pass pointer to the configuration data.
 *
 * @return
 * - positive size of the configuration data block on SUCCESS
 * - 0 on FAILURE
 */
size_t host_get_target_params(const void** params_buf);



/** @} */ /* End of sdio_host group */
