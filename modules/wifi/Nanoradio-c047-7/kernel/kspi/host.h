
/** @defgroup kspi_host SPI host interface on top of kernel driver
 *
 * @brief SPI platform-specific functions.
 *
 * Declaration of SPI host interface that should be implemented for the
 * host platform.
 * 
 * See <b>include/linux/spi/spi.h</b> for linux kernel SPI driver API.
 *
 * The following defines can be used by the host to configure the
 * kspi driver (put in host-specific <b><i>host_def.h</i></b>):
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
 * <b>USE_SPI_MUTEX</b>\n
 * When defined, accesses to the SPI bus are serialized with #spi_mutex.
 * This is safer but reduces speed, so it has been made optional.
 *
 * <b>JTAG_CONNECTION</b>\n
 * Avoid reseting target on initialization. Usefull when target is under
 * debugger control through its JTAG interface.
 *
 * <b>USE_NO_MSLEEP</b>\n
 * Implement <b>mlseep()</b> instead of using linux kernel's mlseep().
 *
 * <b>USE_DMA</b>\n
 * Ask kernel SPI driver to send/receive data to/from host controller
 * by setting <b>spi_message.is_dma_mapped</b> to 1.
 *
 * <b>TARGET_IRQ_NUM <i>irq_number</i></b>\n
 * Defines target IRQ number.
 *
 * <b>USE_SDIO_IRQ</b>\n
 * Let the target use level IRQ instead of both-edges IRQ signal.
 * Experimantal, will not work with normal kernel SPI drivers.
 *
 * <b>SPI_DRIVER_NAME <i>spi_driver_name</i></b>\n
 * String used to register this driver to the linux kernel SPI API.
 * Must be equal to <b>spi_board_info.modalias</b> passed to
 * <b>spi_register_board_info()</b> during platform initialization.
 *
 * <b>SPI_SPEED_HZ <i>freq_Hz</i></b>\n
 * SPI clock frequency rate in Hz
 *
 * <b>SPI_SPEED_HZ_LOW <i>low_freq_Hz</i></b>\n
 * Must be always defined.
 * Some nanoradio devices need a slower clock frequency
 * when pulled out of power save mode with CMD52. In this case,
 * <b><i>low_freq_Hz</i></b> must be less than <b><i>freq_Hz</i></b>.
 * Else, <b><i>low_freq_Hz</i></b> can be equal to <b><i>freq_Hz</i></b>.
 *
 * <b>SPI_CHIP_SELECT <i>cs_num</i></b>\n
 * Defines which Chip Select signal must be used for addressing Nanoradio
 * target. It is platform-specific.
 *
 * <b>SPI_BUS_NUM <i>bus_num</i></b>\n
 * Must be equal to <b>spi_board_info.bus_num</b> passed to
 * <b>spi_register_board_info()</b> during platform initialization.
 *
 * <b>SPI_MODE <i>spi_mode</i></b>\n
 * Defines SPI mode of operation. Can be 0, 1, 2 or 3.
 * Nanoradio target device needs CLK phase 0 and CLK polarity 0.
 * Normally, this mode of operation is selected when <b><i>spi_mode</i></b>
 * is set to 0. However, other values may be required with some kernel
 * SPI drivers.
 *
 * <b>USE_DUMMY_RX_BUFF</b>\n
 * Kernel SPI driver should dump received data when
 * <b>spi_transfef.rx_buf</b> is NULL. But some kernel SPI drivers
 * misbehave when <b>spi_transfef.rx_buf</b> is NULL. In this case, define
 * <b>USE_DUMMY_RX_BUFF</b> to create a dummy buffer for <b>rx_buf</b>.
 *
 * <b>USE_DUMMY_TX_BUFF</b>\n
 * Nanoradio target device should be fed with 1's through MOSI line when
 * data is read from it. For this reason, we define a dummy TX buffer
 * filled with 0xff and use it as <b>spi_transfef.tx_buf</b> during reads.
 * But some kernel SPI drivers misbehave when both <b>.rx_buf</b> and
 * <b>.tx_buf</b> of the same <b>spi_transfer</b> are not NULL.
 * In this case, <b>USE_DUMMY_TX_BUFF</b> can be left undefined,
 * but correct operation will not be guaranteed in general.
 *
 * <b>CHECK_SPI</b>\n
 * Check return status whan calling SPI kernel driver
 * and also check responses to SDIO commands issued over SPI bus.
 * Does not work with all kernel SPI drivers.
 *
 * <b>MAX_TRANSFER_SIZE <i>max_payload_size</i></b>\n
 * When defined, limits CMD53 payload size to <b><i>max_payload_size</i></b>.
 *
 * <b>MAX_CMD53_PER_MSG <i>cmd53_per_msg</i></b>\n
 * We may use the same spi_message structure to deliver many CMD53.
 * The maximum number of CMD53 in the same spi_message will be
 * <b><i>cmd53_per_msg</i></b>.
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

/*! Maximum CMD53 per message */
#ifndef MAX_CMD53_PER_MSG
#define MAX_CMD53_PER_MSG 4
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
 * Enumeration of GPIO pin id's. Issued for function host_gpio().
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
 * GPIO for target IRQ  etc.
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
 *
 * This function will be called when interrupts should be enabled,
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

/** @} */ /* End of kspi_host group */
