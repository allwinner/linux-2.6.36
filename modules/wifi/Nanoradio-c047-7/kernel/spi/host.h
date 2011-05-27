
/** @defgroup spi_host SPI host controller interface
 *
 * @brief SPI platform-specific functions.
 *
 * Declaration of SPI host interface that should be implemented for the
 * host platform.
 * 
 * The following defines can be used by the host to configure the
 * spi driver (put in host-specific <b><i>host_def.h</i></b>):
 *
 * <b>USE_WIFI</b>\n
 * When defined, connect to upper layers (i.e. the WiFiEngine).\n
 * When not defined, perform self test using target loopback mode.
 * 
 * <b>USE_POWER_MANAGEMENT</b>\n
 * Control target's power save mode, see spi_control_sleep().
 *
 * <b>USE_GPIO_CPU_MEASURE</b>\n
 * Use a host GPIO to measure CPU load caused by the driver,
 * see #GPIO_ID_CPU_LOAD.
 *
 * <b>USE_FW_DOWNLOAD</b>\n
 * Allow firmware download after driver initialization.
 * See also #nrx_fw_download and spi_fw_download().
 *
 * <b>USE_TARGET_PARAMS</b>\n
 * Allows custom target configuration for each platform.
 * When defined, host_get_target_params() must be defined in host.c
 * and will be called to get the target configuration data.
 *
 * <b>JTAG_CONNECTION</b>\n
 * Avoid reseting target on initialization. Usefull when target is under
 * debugger control through its JTAG interface.
 *
 * <b>USE_NO_MSLEEP</b>\n
 * Implement <b>mlseep()</b> instead of using linux kernel's mlseep().
 *
 * <b>USE_DMA</b>\n
 * Enable DMA (applicable if the host hardware architecture supports it).
 *
 * <b>TARGET_IRQ_NUM <i>irq_number</i></b>\n
 * Defines target IRQ number.
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

#ifndef TARGET_IRQ_NUM
#error Please define TARGET_IRQ_NUM
#endif

#ifndef USE_WIFI
#undef USE_POWER_MANAGEMENT
#endif

/*! Maximum CMD53 payload size */
#ifndef MAX_TRANSFER_SIZE
#define MAX_TRANSFER_SIZE   512
#endif

/*! Initial state of debug pins (all LOW by default) */
#ifndef INIT_DEBUG_PINS
#define INIT_DEBUG_PINS  0x00000000
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
 * Interrupt line operations. Issued for function host_interrupt().
 */
typedef enum {

	/*! Enable interrupt */
	IRQ_OP_ENABLE,

	/*! Disable interrupt */
	IRQ_OP_DISABLE,

	/*! Get interrupt status, used to determine if interrupt is
	 *  currently active.
	 */
	IRQ_OP_STATUS,

	/*! Acknowledge interrupt */
	IRQ_OP_ACKNOWLEDGE

} irq_op_t;


/*!
 * Enumeration of GPIO pin id's. Issued for host_gpio function.
 */
typedef enum {

	/*!
	 * GPIO pin for measuring CPU load.
	 * Kept high while this driver utilizes the CPU.
	 */
	GPIO_ID_CPU_LOAD,

	/*! GPIO pin for target reset. */
	GPIO_ID_RESET,

	/*! GPIO pin for target power. */
	GPIO_ID_POWER,

} gpio_id_t;


/*!
 * Enumeration of operations that can be performed on GPIO pin id's.
 * Issued for functions host_gpio() and host_debug_pins().
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
 * @brief Initialize host.
 *
 * Will only be called once. This function should typically allocate memory,
 * GPIO for target IRQ  etc. It must also setup the SPI controller
 * and the DMA controller hardware (if DMA is supported and enabled).
 *
 *
 * @return
 * - 0 on SUCCESS
 * - appropriate negative error code on FAILURE
 */
int host_init(void);


/*!
 * @brief Cleanup host.
 *
 * Hook for performing any neccesary cleanup for host code. Will only be called
 * when driver is unloaded.
 *
 */
void host_exit(void);


/*!
 * @brief Control function for misc GPIO pins.
 *
 * This function will be used to control optional GPIO-pins used for target
 * reset or shutdown. If reset or shutdown (or both) connections are missing,
 * those cases can of course be left unimplemented.
 * 
 * @param gpio_op Defines an operation to be performed on a GPIO pin.
 *				  #GPIO_OP_ENABLE will be issued before any other operation
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
 * @param gpio_op	Defines an operation to be performed on debug pin(s).
 *          	  	#GPIO_OP_ENABLE will be issued before any other operation
 *					for each GPIO pin id.
 * @param mask		Defines which debug pin(s) to perform the operation on.
 *
 */
#ifdef USE_DEBUG_PINS
void host_debug_pins(gpio_op_t gpio_op, uint32_t mask);
#else
inline static void host_debug_pins(gpio_op_t gpio_op, uint32_t mask) {}
#endif


/*! 
 * @brief Control function for target interrupt.
 *
 * This function will be used to control interrupt signals from the target.
 * It is not supposed to handle interrupts from the host SPI controller.
 * Host SPI controller IRQs, if enabled, should be handled inside
 * <b><i>host.c</i></b> for each particular host.
 *
 * This function will be called when target interrupts should be enabled,
 * disabled, acknowledged or checked.
 * - After enabling interrupt with #IRQ_OP_ENABLE, the target can generate
 *	 interrupt request on line <b>TARGET_IRQ_NUM</b>.
 * - After an interrupt has been generated, status check with #IRQ_OP_STATUS
 *   should return non-zero values.
 * - New interrupts from the target should not be generated
 *   until the pending interrupt request has been acknowledged.
 *   After pending request acknowledgement with #IRQ_OP_ACKNOWLEDGE,
 *   status check must return 0 until a new interrupt request is generated.
 * - If interrupt request has been disabled with #IRQ_OP_DISABLE,
 *   the target should not generate an interrupt request before re-enable.
 *
 * @param irq_op Defines the operation to be performed.
 *
 * @return
 * - If <b>irq_op</b> was #IRQ_OP_ENABLE, #IRQ_OP_ACKNOWLEDGE or
 *						  #IRQ_OP_DISABLE:
 *		0 on SUCCESS,
 *		appropriate negative error code on FAILURE
 * - If <b>irq_op</b> was #IRQ_OP_STATUS:
 *		non-zero if and only if target interrupt is pending
 */
int host_interrupt(irq_op_t irq_op);


/*!
 * @brief Sends SDIO CMD0 to put the target in SPI mode.
 *
 * After restart, target interface will be in SDIO mode.
 * To put the interface in SPI mode, this function must send CMD0,
 * possibly after a string of zeros.
 * The target is expected to respond to CMD0 with 0x01,
 * else this function must return a negative error code.
 *
 * @return
 * - 0 on SUCCESS,
 * - appropriate negative error code on FAILURE
 */
int host_send_cmd0(void);


/*!
 * @brief Issues one CMD52 to the target.
 *
 * Some nanoradio devices need a slower clock frequency
 * when pulled out of power save mode with CMD52.
 * If such a device is supported, this function must use an apropriate
 * low frequency clock when <b>use_slow_clock</b> is non-zero.
 *
 * @param	content			CMD52 contents
 * @param	use_slow_clock	if non-zero, low frequency SPI clock
 *                          must be used
 *
 * @return
 * - 0 on success 
 * - appropriate negative error code on FAILURE
 */
int host_send_cmd52(uint32_t content, int use_slow_clock);


/*!
 * @brief Issues one CMD53 to read data from the target.
 *
 * @param	data		buffer to hold data from the target
 * @param	size		number of bytes to be read
 *
 * @return
 * - 0 on success 
 * - appropriate negative error code on FAILURE
 */
int host_send_cmd53(unsigned char* data, unsigned int size);


/*!
 * @brief Issues one CMD53 to write data to the target.
 *
 * @param	data		buffer holding data to the target
 * @param	size		number of bytes to be written
 *
 * @return
 * - 0 on success 
 * - appropriate negative error code on FAILURE
 */
int host_read_cmd53(unsigned char* data, unsigned int size);


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

/** @} */ /* End of spi_host group */
