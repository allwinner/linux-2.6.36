/*! @defgroup kspi_kernel_api SDIO-to-SPI wrapper on top of kernel SPI driver
 *
 * @brief SDIO-to-SPI wrapper on top of kernel SPI driver
 *
 * This file implements a wrapper around linux kernel SPI driver.
 * This module uses the linux kernel SPI driver to implement
 * SDIO-over-SPI bus. It is used by the generic SPI driver
 * and it does not depend on any particular host.
 *
 * See definitions of spi_transfer, spi_message, etc. in header file
 * <b>include/linux/spi/spi.h</b>
 *
 *  @{
 */

#include "host.h"

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include "nanoutil.h"


/******************************************************************************
D E F I N E S
******************************************************************************/

/* Use 8-bit words */
#define SPI_BIT_WIDTH 8

#ifndef SPI_DRIVER_NAME
#error Please define SPI_DRIVER_NAME
#endif

#if !defined(SPI_SPEED_HZ) || !defined(SPI_SPEED_HZ_SLOW)
#error Please define SPI_SPEED_HZ and SPI_SPEED_HZ_SLOW
#endif

#ifndef SPI_CHIP_SELECT
#error Please define SPI_CHIP_SELECT
#endif

#ifndef SPI_BUS_NUM
#error Please define SPI_BUS_NUM
#endif

#ifndef SPI_MODE
#error Please define SPI_MODE
#endif


/******************************************************************************
G L O B A L S
******************************************************************************/
/*! Represents R5 response of SDIO */
typedef unsigned char r5_resp_t[2];

/*! Represents the target device through kernel SPI API. */
static struct spi_device *nano_spi_dev = NULL;

/*! Used by nano_spi_register() to wait for nano_spi_probe() completion. */
extern struct completion probe_completion; /* just for doxygen */
DECLARE_COMPLETION(probe_completion);


/******************************************************************************
S D I O   C O M M A N D S
******************************************************************************/
/*
 * SDIO is emulated over SPI. So, we need SDIO command fields etc.
 * See SDIO specification.
 * As we may pack multiple CMD53 in the same spi_message,
 * the CMD53 starts with a CRC for the data phase of the previous CMD53.
 * This is harmless for the 1st CMD53 in the spi_message.
 */

/* contents of #buff_cmd53_write */
const unsigned char cmd53_write[] =
{
	0xff, 0xff, /*** CRC for the data phase of the previous CMD53 */
	0xff,		/*** command preamble */
	0x75,		/* StartBit = 0, Dir = 1, cmd index = 0b110101 */
	0x90,		/* R/W = 1, Func# = 1, BlockMode = 0, OpCode = 0 */
	0x00,
	0x00,   	/* size msb */
	0x00,   	/* size lsb */
	0x01,		/* CRC = 0b0000000, EndBit = 1 */
	0xff,		/*** response preamble */
	0xff, 0xfe	/* receive R5 response (16 bits),
				   also send StartBit = 0 for data phase */
};

/* contents of #buff_cmd53_read */
const unsigned char cmd53_read[] =
{
	0xff, 0xff, /*** CRC for the data phase of the previous CMD53 */
	0xff,		/*** command preamble */
	0x75,		/* StartBit = 0, Dir = 1, cmd index = 0b110101 */
	0x10,		/* R/W = 0, Func# = 1, BlockMode = 0, OpCode = 0 */
	0x00,
	0x00,   	/* size msb */
	0x00,   	/* size lsb */
	0x01,		/* CRC = 0b0000000, EndBit = 1 */
	0xff,		/*** response preamble */
	0xff, 0xff,	/* receive R5 response (16 bits) */
	0xff, 0xff	/*** data phase preamble, receive StartBit from target */
};

/* contents of #buff_cmd53_crc */
const unsigned char cmd53_crc[] =
{
	0xff, 0xff
};

/* contents of #buff_cmd52 */
const unsigned char cmd52[] =
{
	0xff,	/*** command preamble */
	0x74,	/* StartBit = 0, Dir = 1, cmd index = 0b110100 */
	0x00,
	0x00,
	0x00,
	0x00,
	0xff,	/* CRC = 0b1111111, EndBit = 1 */
	0xff,	/*** response preamble */
	0xff, 0xff	/* receive R5 response (16 bits) */
};


/******************************************************************************
D M A - S A F E   B U F F E R   D E S C R I P T O R S
******************************************************************************/
/*! @brief Represents a buffer with data to / from SPI bus. */
struct spi_buff
{
	unsigned char*	ptr;	/*!< pointer to buffer contents */
	dma_addr_t		phys;	/*!< physical address for DMA */
	unsigned int	size;	/*!< buffer size in bytes */
};

/*!
 * SDIO CMD53 for sending data to the target.
 * Response reception and data phase preamble included.
 */
static struct spi_buff buff_cmd53_write[MAX_CMD53_PER_MSG];

/*!
 * SDIO CMD53 for receiving data from the target.
 * Response reception and data phase preamble included.
 */
static struct spi_buff buff_cmd53_read[MAX_CMD53_PER_MSG];

/*! CRC at the end of CMD53 data phase. */
static struct spi_buff buff_cmd53_crc;

/*! SDIO CMD52, response reception included. */
static struct spi_buff buff_cmd52;

/*! Data output to MOSI pin during reception. */
#ifdef USE_DUMMY_TX_BUFF
static struct spi_buff buff_dummy_tx;
#endif

/*! Data input from MISO pin during transmission. */
#ifdef USE_DUMMY_RX_BUFF
static struct spi_buff buff_dummy_rx;
#endif

/*!
 * Data input from target during CMD53 and CMD52.
 * Contains response to SDIO command.
 */
#ifdef CHECK_SPI
static struct spi_buff buff_cmd_feedback;
#endif


/*!
 * @brief Creates an spi_buff object and allocates kernel memory for it.
 *
 * Allocated memory is DMA-safe if <b>USE_DMA</b> is defined.
 *
 * @param	p_spi_buff	pointer to the created object
 * @param	size		size of memory to be allocated
 *
 * @return
 * - 0 on success 
 * - -ENOMEM if memory allocation fails.
 */
static int spi_buff_alloc(struct spi_buff *p_spi_buff, unsigned int size)
{
#ifdef USE_DMA
	struct device *dev = &nano_spi_dev->dev;
	p_spi_buff->ptr = dma_alloc_coherent(dev,
										 size, &p_spi_buff->phys,
										 GFP_KERNEL);
	TRSP_MSG("After dma_alloc_coherent(), virt:0x%p phys:0x%08x\n",
			p_spi_buff->ptr, p_spi_buff->phys);
#else /* !USE_DMA */
	p_spi_buff->ptr = kmalloc(size, GFP_KERNEL);
	p_spi_buff->phys = 0;
#endif /* USE_DMA */
	p_spi_buff->size = size;
	return (p_spi_buff->ptr) ? 0 : -ENOMEM;
}


/*!
 * @brief Creates an spi_buff object from an existing buffer.
 *
 * If <b>USE_DMA</b> is defined and direction is not <b>DMA_NONE</b>,
 * memory assigned to this buffer must be DMA-safe
 * and <b>dma_map_single</b> will be called.
 *
 * @param	p_spi_buff	pointer to the created object
 * @param	ptr			pointer to the existing buffer
 * @param	size		size of the existing buffer
 * @param	direction	DMA data direction
 */
static void spi_buff_from_ptr(struct spi_buff *p_spi_buff, unsigned char *ptr,
							  unsigned int size, enum dma_data_direction direction)
{
#ifndef USE_DMA
	direction = DMA_NONE;
#endif

	if (direction != DMA_NONE) {
		struct device *dev = &nano_spi_dev->dev;
		p_spi_buff->phys = dma_map_single(dev, ptr, size, direction);
	}
	else
		p_spi_buff->phys = 0;

	p_spi_buff->ptr = ptr;
	p_spi_buff->size = size;
}


/*!
 * @brief Copies data to the memory of an spi_buff object.
 *
 * Copies from a given source pointer
 * to the memory assigned to a previously created spi_buff object.
 *
 * @param	p_spi_buff	pointer to the spi_buff object
 * @param	src			source pointer
 */
static inline void spi_buff_copy(struct spi_buff *p_spi_buff, const unsigned char *src)
{
	memcpy(p_spi_buff->ptr, src, p_spi_buff->size);
}


/*!
 * @brief Fills the memory of an spi_buff object.
 *
 * Fills the memory assigned to a previously created spi_buff object with
 * a given byte.
 *
 * @param	p_spi_buff	pointer to the spi_buff object
 * @param	fill		byte filler
 */
static inline void spi_buff_fill(struct spi_buff *p_spi_buff, unsigned char fill)
{
	memset(p_spi_buff->ptr, fill, p_spi_buff->size);
}


/*!
 * @brief Frees the kernel memory assigned to an spi_buff object.
 *
 * @param	p_spi_buff	pointer to the spi_buff object
 */
static void spi_buff_free(struct spi_buff *p_spi_buff)
{
#ifdef USE_DMA
	if (!p_spi_buff->ptr) {
		struct device *dev = &nano_spi_dev->dev;
		if (p_spi_buff->ptr)
			dma_free_coherent(dev, p_spi_buff->size,
							  p_spi_buff->ptr, p_spi_buff->phys);
	}
#else /* !USE_DMA */
	if (!p_spi_buff->ptr)
		kfree(p_spi_buff->ptr);
#endif /* USE_DMA */
}


/*!
 * @brief Creates and initializes all spi_buff objects
 * needed during driver operation.
 *
 * @return
 * - 0 on success 
 * - -ENOMEM if any memory allocation fails.
 */
static int nano_spi_buff_init(void)
{
	unsigned int failed = 0;
	int i;
	
	for (i = 0; i < MAX_CMD53_PER_MSG; i++) {
		failed |= spi_buff_alloc(&buff_cmd53_write[i],	sizeof(cmd53_write));
		failed |= spi_buff_alloc(&buff_cmd53_read[i],  sizeof(cmd53_read));
	}
	failed |= spi_buff_alloc(&buff_cmd53_crc,   sizeof(cmd53_crc));
	failed |= spi_buff_alloc(&buff_cmd52,		sizeof(cmd52));
#ifdef USE_DUMMY_TX_BUFF
	failed |= spi_buff_alloc(&buff_dummy_tx, MAX_TRANSFER_SIZE);
#endif
#ifdef USE_DUMMY_RX_BUFF
	failed |= spi_buff_alloc(&buff_dummy_rx, MAX_TRANSFER_SIZE);
#endif
#ifdef CHECK_SPI
	failed |= spi_buff_alloc(&buff_cmd_feedback, max(sizeof(cmd52),
												 max(sizeof(cmd53_write),
													 sizeof(cmd53_read))));
#endif
	if (failed)
		return -ENOMEM;
		
	for (i = 0; i < MAX_CMD53_PER_MSG; i++) {
		spi_buff_copy(&buff_cmd53_write[i], cmd53_write);
		spi_buff_copy(&buff_cmd53_read[i],  cmd53_read);
	}
	spi_buff_copy(&buff_cmd53_crc,	 cmd53_crc);
	spi_buff_copy(&buff_cmd52,		 cmd52);

#ifdef USE_DUMMY_TX_BUFF
	/* This is necessary to keep SPI_MOSI at high during reception */
	spi_buff_fill(&buff_dummy_tx, 0xff);
#endif
	
#ifdef USE_DUMMY_RX_BUFF
	/* Just fill buff_dummy_rx with a known pattern to aid debugging */
	spi_buff_fill(&buff_dummy_rx, 0xab);
#endif

	return 0;
}


/*!
 * @brief Frees all kernel memory allocated by nano_spi_buff_init().
 */
static void nano_spi_buff_exit(void)
{
	int i;
	
	for (i = 0; i < MAX_CMD53_PER_MSG; i++) {
		spi_buff_free(&buff_cmd53_write[i]);
		spi_buff_free(&buff_cmd53_read[i]);
	}
	spi_buff_free(&buff_cmd53_crc);
#ifdef USE_DUMMY_TX_BUFF
	spi_buff_free(&buff_dummy_tx);
#endif
#ifdef USE_DUMMY_RX_BUFF
	spi_buff_free(&buff_dummy_rx);
#endif
#ifdef CHECK_SPI
	spi_buff_free(&buff_cmd_feedback);
#endif
}


/******************************************************************************
S P I   T R A N S F E R   D E S C R I P T O R S
******************************************************************************/
/*!
 * SDIO CMD53 for sending data to the target.
 * Response reception and data phase preamble included.
 * See also #buff_cmd53_write.
 */
static struct spi_transfer xfer_cmd53_write[MAX_CMD53_PER_MSG];

/*!
 * SDIO CMD53 for reading data from the target.
 * Response reception and data phase preamble included.
 * See also #buff_cmd53_read.
 */
static struct spi_transfer xfer_cmd53_read[MAX_CMD53_PER_MSG];

/*! Data phase of CMD53 for sending data to the target */
static struct spi_transfer xfer_data_write[MAX_CMD53_PER_MSG];

/*! Data phase of CMD53 for reading data from the target */
static struct spi_transfer xfer_data_read[MAX_CMD53_PER_MSG];

/*! CRC ending data phase of CMD53. See also #buff_cmd53_crc */
static struct spi_transfer xfer_cmd53_crc_write;
static struct spi_transfer xfer_cmd53_crc_read;

/*! SDIO CMD52, response reception included. See also #buff_cmd52 */
static struct spi_transfer xfer_cmd52;


/*!
 * @brief Zero-initializes an <b>spi_transfer</b> structure.
 *
 * @param	p_spi_transfer	pointer to the spi_transfer structure
 */
static void spi_xfer_init(struct spi_transfer *p_spi_transfer)
{
	memset(p_spi_transfer, 0, sizeof(struct spi_transfer));
}


/*!
 * @brief Sets TX parameters for an <b>spi_transfer</b> structure
 * from a given spi_buff object.
 *
 * If <b>tx_buff</b> is NULL, TX pointers of the spi_transfer structure
 * will be set to zero.
 * If <b>len</b> is zero, <b>spi_transfer.len</b> will not be changed.
 *
 * @param	p_spi_transfer	pointer to the spi_transfer structure
 * @param	tx_buff			pointer to spi_buff object
 * @param	len				size of the transfer in bytes
 */
static void spi_xfer_tx_setup(struct spi_transfer *p_spi_transfer,
							  const struct spi_buff *tx_buff,
							  unsigned int len)
{
	TRSP_ASSERT(p_spi_transfer);

	if (len)
		p_spi_transfer->len = len;

	if (tx_buff) {
		p_spi_transfer->tx_buf = tx_buff->ptr;
		p_spi_transfer->tx_dma = tx_buff->phys;
		TRSP_ASSERT(len <= tx_buff->size);
	}
	else {
		p_spi_transfer->tx_buf = NULL;
		p_spi_transfer->tx_dma = 0;
	}
}

/*!
 * @brief Sets RX parameters for an <b>spi_transfer</b> structure
 * from a given spi_buff object.
 *
 * If <b>rx_buff</b> is NULL, RX pointers of the spi_transfer structure
 * will be set to zero.
 * If <b>len</b> is zero, <b>spi_transfer.len</b> will not be changed.
 *
 * @param	p_spi_transfer	pointer to the spi_transfer structure
 * @param	rx_buff			pointer to spi_buff object
 * @param	len				size of the transfer in bytes
 */
static void spi_xfer_rx_setup(struct spi_transfer *p_spi_transfer,
							  const struct spi_buff *rx_buff,
							  unsigned int len)
{
	TRSP_ASSERT(p_spi_transfer);

	if (len)
		p_spi_transfer->len = len;

	if (rx_buff) {
		p_spi_transfer->rx_buf = rx_buff->ptr;
		p_spi_transfer->rx_dma = rx_buff->phys;
		TRSP_ASSERT(len <= rx_buff->size);
	}
	else {
		p_spi_transfer->rx_buf = NULL;
		p_spi_transfer->rx_dma = 0;
	}
}

/*!
 * @brief Initializes all <b>spi_transfer</b> structures needed
 * during driver operation.
 */
static void nano_spi_xfer_init(void)
{
	struct spi_buff *pbuff_default_tx = NULL;
	struct spi_buff *pbuff_default_rx = NULL;
	struct spi_buff *pbuff_cmd_feedback;
	int i;

#ifdef USE_DUMMY_TX_BUFF
	pbuff_default_tx = &buff_dummy_tx;
#endif

#ifdef USE_DUMMY_RX_BUFF
	pbuff_default_rx = &buff_dummy_rx;
#endif

#ifdef CHECK_SPI
	pbuff_cmd_feedback = &buff_cmd_feedback;
#else
	pbuff_cmd_feedback = pbuff_default_rx;
#endif

	spi_xfer_init(&xfer_cmd52);
	spi_xfer_tx_setup(&xfer_cmd52,		 &buff_cmd52, 			buff_cmd52.size);
	spi_xfer_rx_setup(&xfer_cmd52,		 pbuff_cmd_feedback, 	0);

	for (i = 0; i < MAX_CMD53_PER_MSG; i++) {
		spi_xfer_init(&xfer_cmd53_write[i]);
		spi_xfer_tx_setup(&xfer_cmd53_write[i], &buff_cmd53_write[i],	buff_cmd53_write[i].size);
		spi_xfer_rx_setup(&xfer_cmd53_write[i], pbuff_cmd_feedback, 	0);

		spi_xfer_init(&xfer_cmd53_read[i]);
		spi_xfer_tx_setup(&xfer_cmd53_read[i],  &buff_cmd53_read[i],  	buff_cmd53_read[i].size);
		spi_xfer_rx_setup(&xfer_cmd53_read[i],  pbuff_cmd_feedback, 	0);

		spi_xfer_init(&xfer_data_write[i]);
		/* spi_xfer_tx_setup(&xfer_data_write,...) called in nano_spi_send_cmd53 */
		spi_xfer_rx_setup(&xfer_data_write[i],  pbuff_default_rx, 0);
	
		spi_xfer_init(&xfer_data_read[i]);
		spi_xfer_tx_setup(&xfer_data_read[i],   pbuff_default_tx, 0);
		/* spi_xfer_rx_setup(&xfer_data_read,...) called in nano_spi_read_cmd53 */
	}

	spi_xfer_init(&xfer_cmd53_crc_write);
	spi_xfer_tx_setup(&xfer_cmd53_crc_write, &buff_cmd53_crc,   buff_cmd53_crc.size);
	spi_xfer_rx_setup(&xfer_cmd53_crc_write, pbuff_default_rx, 	0);

	spi_xfer_init(&xfer_cmd53_crc_read);
	spi_xfer_tx_setup(&xfer_cmd53_crc_read,  &buff_cmd53_crc,   buff_cmd53_crc.size);
	spi_xfer_rx_setup(&xfer_cmd53_crc_read,  pbuff_default_rx, 	0);
}


/******************************************************************************
S P I   M E S S A G E   D E S C R I P T O R S
******************************************************************************/
/*!
 * One or many SDIO CMD53 for sending data to the target.
 * Consists of (possibly many) #xfer_cmd53_write and #xfer_data_write pairs,
 * finalized by one #xfer_cmd53_crc_write.
 */
static struct spi_message msg_cmd53_write;

/*!
 * One or many SDIO CMD53 for receiving data from the target.
 * Consists of (possibly many) #msg_cmd53_read and #xfer_data_read pairs,
 * finalized by one #xfer_cmd53_crc_read.
 */
static struct spi_message msg_cmd53_read;

/*!
 * SDIO CMD52 for sending data to the target. Contains only #xfer_cmd52.
 */
static struct spi_message msg_cmd52;

/*!
 * @brief Initializes all <b>spi_message</b> structures
 * needed during driver operation.
 */
static void nano_spi_message_init(void)
{
	/* Setup CMD52 */
	spi_message_init(&msg_cmd52);
	spi_message_add_tail(&xfer_cmd52, &msg_cmd52);

#ifdef USE_DMA
	/* Configure SPI messages representing CMD53 and CMD52 as DMA mapped. */
	msg_cmd53_read.is_dma_mapped = 1;
	msg_cmd53_write.is_dma_mapped = 1;
	msg_cmd52.is_dma_mapped = 1;
#endif
}


/******************************************************************************
S D I O   O V E R   S P I   O P E R A T I O N S
******************************************************************************/
/*!
 * @brief Raw read from SPI bus.
 *
 * Does not use DMA even if <b>USE_DMA</b> is defined.
 *
 * @param	buf		buffer to hold read data
 * @param	len		number of bytes to be read,
 *					must not exceed <b>MAX_TRANSFER_SIZE</b>
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
static int nano_spi_read(uint8_t *buf, size_t len)
{
	struct spi_transfer	t;
	struct spi_message	m;
	int status;

	spi_message_init(&m);
	spi_xfer_init(&t);
#ifdef USE_DUMMY_TX_BUFF
	t.tx_buf = buff_dummy_tx.ptr;
#endif
	t.rx_buf = buf;
	t.len = len;
	spi_message_add_tail(&t, &m);
	status = spi_sync(nano_spi_dev, &m);
#ifdef CHECK_SPI
	TRSP_ASSERT(status == 0);
#if 0 /* Check spi_message.status only if kernel driver sane... */
	if (status == 0) {

		status = m.status;
		TRSP_ASSERT(status == 0);
	}
#endif
#endif /* CHECK_SPI */

#if 0
	printk("FX==> [%s]:[%d]\n", __func__, __LINE__);
	nano_util_printbuf(buff_dummy_tx.ptr, len, "tx");
	nano_util_printbuf(buf, len, "rx");
#endif
	return status;
}


/*!
 * @brief Raw write to SPI bus.
 *
 * Does not use DMA even if <b>USE_DMA</b> is defined.
 *
 * @param	buf		buffer holding data to be written
 * @param	len		number of bytes to be written,
 *					must not exceed <b>MAX_TRANSFER_SIZE</b>
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
static int nano_spi_write(uint8_t *buf, size_t len)
{
	struct spi_transfer	t;
	struct spi_message	m;
	int status;

	spi_message_init(&m);
	spi_xfer_init(&t);
	t.tx_buf = buf;
#ifdef USE_DUMMY_RX_BUFF
	t.rx_buf = buff_dummy_rx.ptr;
#endif
	t.len = len;
	spi_message_add_tail(&t, &m);
	status = spi_sync(nano_spi_dev, &m);
		
#ifdef CHECK_SPI
	TRSP_ASSERT(status == 0);
#if 0 /* Check spi_message.status only if kernel driver sane... */
	if (status == 0) {

		status = m.status;
		TRSP_ASSERT(status == 0);
	}
#endif
#endif /* CHECK_SPI */

#if 0
	printk("FX==> [%s]:[%d]\n", __func__, __LINE__);
	nano_util_printbuf(buf, len, "tx");
	if (t.rx_buf)
		nano_util_printbuf(t.rx_buf, len, "rx");
#endif
	return status;
}


/*!
 * @brief Sends SDIO CMD0 to put the target in SPI mode.
 *
 * After restart, target interface will be in SDIO mode.
 * To put the interface in SPI mode, we send a string of zeros
 * and then CMD0. The target must respond with 0x01.
 * Uses nano_spi_write() and nano_spi_read().
 *
 * @return
 * - 0 on success 
 * - error code reported by nano_spi_write() or nano_spi_read()
 *		if any of them fails.
 * - -EIO if target response is not 0x01.
 */
int nano_spi_send_cmd0(void)
{
    unsigned char zeros[] = {   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    unsigned char cmd0[] = {	0xff,0xff,0x40,0x00,0x00,0x00,0x00,0x01,0xff};
    unsigned char response = 0xff;
	int status;

    status = nano_spi_write(zeros, sizeof(zeros));
	TRSP_ASSERT(status == 0);
	if (status) return status;

    status = nano_spi_write(cmd0, sizeof(cmd0));
	TRSP_ASSERT(status == 0);
	if (status) return status;

    status = nano_spi_read(&response, 1);
	TRSP_ASSERT(status == 0);
	if (status) return status;

	TRSP_MSG("Response to CMD0: 0x%02x\n", response);
	return (response == 0x01) ? 0 : -EIO;
}


/*!
 * @brief Delivers CMD52 or CMD53.
 *
 * If multiple commands are pasted in the pspi_message, then presponse will
 * get the response to the last command.
 *
 * @param	pspi_message	ptr to struct spi_message representing the command
 * @param	presponse		ptr returning R5 response
 * @param   resp_offset		offset of the R5 response in the #buff_cmd_feedback
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
#ifdef CHECK_SPI

static int spi_issue_cmd(struct spi_message *pspi_message,
						 r5_resp_t *presponse, unsigned int resp_offset)
{
	int status;

	spi_buff_fill(&buff_cmd_feedback, 0xff);

	status = spi_sync(nano_spi_dev, pspi_message);
	TRSP_ASSERT(status == 0);

#if 0 /* Check spi_message.status only if kernel driver sane... */
	if (status == 0) {

		status = pspi_message->status;
		TRSP_ASSERT(status == 0);
	}
#endif

	(*presponse)[0] = buff_cmd_feedback.ptr[resp_offset];
	(*presponse)[1] = buff_cmd_feedback.ptr[resp_offset + 1];
	return status;
}

#else /* !CHECK_SPI */

inline int spi_issue_cmd(struct spi_message *pspi_message,
						 r5_resp_t *presponse, unsigned int resp_offset)
{
	return spi_sync(nano_spi_dev, pspi_message);
}

#endif /* CHECK_SPI */


/*!
 * @brief Issues one CMD52 to the target.
 *
 * @param	content			CMD52 contents
 * @param	use_slow_clock	if non-zero, sets SPI clock frequency
 *							at <b>SPI_SPEED_HZ_SLOW</b>
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
int nano_spi_send_cmd52(uint32_t content, int use_slow_clock)
{
	int status;
	r5_resp_t response;

    buff_cmd52.ptr[2] = (unsigned char) (content >> 24) & 0xff;
    buff_cmd52.ptr[3] = (unsigned char) (content >> 16) & 0xff;
    buff_cmd52.ptr[4] = (unsigned char) (content >>  8) & 0xff;
    buff_cmd52.ptr[5] = (unsigned char) (content      ) & 0xff;

#if SPI_SPEED_HZ_SLOW < SPI_SPEED_HZ
	xfer_cmd52.speed_hz = use_slow_clock ? SPI_SPEED_HZ_SLOW : 0;
#else
	(void) use_slow_clock; /* mute compiler warning */
#endif /* SPI_SPEED_HZ_SLOW < SPI_SPEED_HZ */

	status = spi_issue_cmd(&msg_cmd52, &response, 8);
#ifdef CHECK_SPI
	TRSP_ASSERT(status == 0);
	if (status == 0) {
		TRSP_ASSERT(response[0] == 0x01);
		TRSP_ASSERT(response[1] == (content & 0xff));
		if (response[0] != 0x01 || response[1] != (content & 0xff))
			return -EIO;
	}
#endif /* CHECK_SPI */
	return status;
}


/* 
 * Split byte/block count for CMD53.
 * High part is count[8], low part is count[7:0]
 * If count == 512, both high and low part will be zero, to comply with spec.
 */
#define CMD53_COUNT_HI(count) ((unsigned char) ((count & 0x1ff) >> 8))
#define CMD53_COUNT_LO(count) ((unsigned char) (count & 0xff))


/*!
 * @brief Issues one or many CMD53 to read data from the target.
 *
 * @param	data		buffer to hold data from the target
 * @param	size		number of bytes to be read
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
int nano_spi_read_cmd53(unsigned char* data, unsigned int size)
{
	struct spi_buff buff_data_read[MAX_CMD53_PER_MSG];
	int status;
	r5_resp_t response;
	int i = 0;
	
	spi_message_init(&msg_cmd53_read);
	while (size > MAX_TRANSFER_SIZE) {

		/* Write the size to the CMD53 header */
		BUG_ON(i >= MAX_CMD53_PER_MSG);
		buff_cmd53_read[i].ptr[6] = CMD53_COUNT_HI(MAX_TRANSFER_SIZE);
		buff_cmd53_read[i].ptr[7] = CMD53_COUNT_LO(MAX_TRANSFER_SIZE);

		/* Prepare SPI buffer and transfer descriptor for data phase */
		spi_buff_from_ptr(&buff_data_read[i], data, MAX_TRANSFER_SIZE, DMA_FROM_DEVICE);
		spi_xfer_rx_setup(&xfer_data_read[i], &buff_data_read[i], MAX_TRANSFER_SIZE);

		spi_message_add_tail(&xfer_cmd53_read[i], &msg_cmd53_read);
		spi_message_add_tail(&xfer_data_read[i], &msg_cmd53_read);

		data += MAX_TRANSFER_SIZE;
		size -= MAX_TRANSFER_SIZE;
		i++;
	}

	/* Write the size to the CMD53 header */
	BUG_ON(i >= MAX_CMD53_PER_MSG);
	buff_cmd53_read[i].ptr[6] = CMD53_COUNT_HI(size);
	buff_cmd53_read[i].ptr[7] = CMD53_COUNT_LO(size);

	/* Prepare SPI buffer and transfer descriptor for data phase */
	spi_buff_from_ptr(&buff_data_read[i], data, size, DMA_FROM_DEVICE);
	spi_xfer_rx_setup(&xfer_data_read[i], &buff_data_read[i], size);

	spi_message_add_tail(&xfer_cmd53_read[i], &msg_cmd53_read);
	spi_message_add_tail(&xfer_data_read[i], &msg_cmd53_read);
	spi_message_add_tail(&xfer_cmd53_crc_read, &msg_cmd53_read);

	status = spi_issue_cmd(&msg_cmd53_read, &response, 10);
#ifdef CHECK_SPI
	TRSP_ASSERT(status == 0);
	if (status == 0) {
		if (response[0] != 0x01 || response[1] != 0x00) {
			TRSP_ASSERT(0);
			return -EIO;
		}
	}
#endif /* CHECK_SPI */

	return status;
}


/*!
 * @brief Issues one or many CMD53 to write data to the target.
 *
 * @param	data		buffer holding data to the target
 * @param	size		number of bytes to be written
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
int nano_spi_send_cmd53(unsigned char* data, unsigned int size)
{
	struct spi_buff buff_data_write[MAX_CMD53_PER_MSG];
	int status;
	r5_resp_t response;
	int i = 0;

	spi_message_init(&msg_cmd53_write);
	while (size > MAX_TRANSFER_SIZE) {

	/* Write the size to the CMD53 header */
		BUG_ON(i >= MAX_CMD53_PER_MSG);
		buff_cmd53_write[i].ptr[6] = CMD53_COUNT_HI(MAX_TRANSFER_SIZE);
		buff_cmd53_write[i].ptr[7] = CMD53_COUNT_LO(MAX_TRANSFER_SIZE);

		/* Prepare SPI buffer and transfer descriptor for data phase */
		spi_buff_from_ptr(&buff_data_write[i], data, MAX_TRANSFER_SIZE, DMA_TO_DEVICE);
		spi_xfer_tx_setup(&xfer_data_write[i], &buff_data_write[i], MAX_TRANSFER_SIZE);

		spi_message_add_tail(&xfer_cmd53_write[i], &msg_cmd53_write);
		spi_message_add_tail(&xfer_data_write[i], &msg_cmd53_write);

		data += MAX_TRANSFER_SIZE;
		size -= MAX_TRANSFER_SIZE;
		i++;
	}

	/* Write the size to the CMD53 header */
	BUG_ON(i >= MAX_CMD53_PER_MSG);
	buff_cmd53_write[i].ptr[6] = CMD53_COUNT_HI(size);
	buff_cmd53_write[i].ptr[7] = CMD53_COUNT_LO(size);

	/* Prepare SPI transfer descriptor for data phase */
	spi_buff_from_ptr(&buff_data_write[i], data, size, DMA_TO_DEVICE);
	spi_xfer_tx_setup(&xfer_data_write[i], &buff_data_write[i], size);

	spi_message_add_tail(&xfer_cmd53_write[i], &msg_cmd53_write);
	spi_message_add_tail(&xfer_data_write[i], &msg_cmd53_write);
	spi_message_add_tail(&xfer_cmd53_crc_write, &msg_cmd53_write);

	status = spi_issue_cmd(&msg_cmd53_write, &response, 10);
#ifdef CHECK_SPI
	TRSP_ASSERT(status == 0);
	if (status == 0) {
		if (response[0] != 0x01 || response[1] != 0x00) {
			TRSP_ASSERT(0);
			return -EIO;
		}
	}
#endif
	return status;
}



/******************************************************************************
S P I   D R I V E R   C A L L B A C K S
******************************************************************************/
/*!
 * @brief Callback registered to kernel SPI API.
 * Called whenever the SPI slave device is detected.
 *
 * It sets global pointer #nano_spi_dev only on successful completion.
 * See also function nano_spi_register()
 *
 * @param	spi	the SPI slave device
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
static int __devinit nano_spi_probe(struct spi_device *spi)
{
	int status = -EINVAL;

	TRSP_MSG("Device %s probed\n", spi->dev.bus_id);

	/* Setup SPI slave device parameters */
	if (!spi) goto exit;
	spi->mode = SPI_MODE;
	spi->bits_per_word = SPI_BIT_WIDTH;
	spi->chip_select = SPI_CHIP_SELECT;
	spi->max_speed_hz = SPI_SPEED_HZ;
		
	/* Check SPI slave bus number */
	if (!spi->master) goto exit;
	if (spi->master->bus_num != SPI_BUS_NUM) goto exit;

	/* Register SPI slave device */
	status = spi_setup(spi);
	if (status) goto exit;

	/* Successful completion */
	nano_spi_dev = spi;
	status = 0;

exit:
	complete(&probe_completion);
	return status;
}

/*!
 * @brief Callback registered to kernel SPI API.
 * Called whenever the SPI slave device is removed.
 *
 * @param	spi	the SPI slave device
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
static int __devexit nano_spi_remove(struct spi_device *spi)
{
	TRSP_MSG("Device %s removed\n", spi->dev.bus_id);
	return 0;
}

/*!
 * @brief Callback registered to kernel SPI API.
 * Should put the SPI slave device in low-power mode/state.
 *
 * @param	spi	the SPI slave device
 * @param	message	the power state to enter
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
static int nano_spi_suspend(struct spi_device *spi, pm_message_t message)
{
	return 0;
}

/*!
 * @brief Callback registered to kernel SPI API.
 * Should bring the SPI slave device back from low-power mode/state.
 *
 * @param	spi	the SPI slave device
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
static int nano_spi_resume(struct spi_device *spi)
{
	return 0;
}


/******************************************************************************
S P I   A P I   R E G I S T R A T I O N
******************************************************************************/
/*
 * Used to register this driver to the linux SPI API.
 * Contains pointers to the power management callback functions.
 */
struct spi_driver nano_spi_driver = {
	.driver = {
		   .name = SPI_DRIVER_NAME,
		   .bus = &spi_bus_type,
		   .owner = THIS_MODULE,
		   },
	.probe = nano_spi_probe,
	.remove = __devexit_p(nano_spi_remove),
	.suspend = nano_spi_suspend,
	.resume = nano_spi_resume,
};

/*!
 * @brief Registers the driver to kernel SPI API.
 *
 * This function is called by spi_init() when the driver is loaded.
 * After succesful registration, it creates all objects needed during
 * driver operation. If it fails at any point, driver loading will fail.
 *
 * @return
 * - 0 on success 
 * - apropriate negative error code on failure.
 */
int nano_spi_register(void)
{
	int status;

	/* During registration, nano_spi_probe will be called,
	   perhaps implicitly. */
	status = spi_register_driver(&nano_spi_driver);
	if (status) {
		TRSP_MSG("spi_register_driver failed\n");
		return status;
	}

	/* Wait for nano_spi_probe to complete and check nano_spi_dev
	   If nano_spi_dev is still NULL, driver loading must fail.
	   This trick is necessary because spi_register_driver may have returned 0
	   even if nano_spi_probe has failed. */
	status = wait_for_completion_interruptible_timeout(&probe_completion, 
													msecs_to_jiffies(1000));
	if (status >= 0 && !nano_spi_dev) {
		/* probe_completion has been signaled, but nano_spi_dev still NULL */
		status = -ENODEV; 
	}
	if (status < 0) {
		spi_unregister_driver(&nano_spi_driver);
		TRSP_MSG("nano_spi_probe failed (or never called)\n");
		return status;
	}

	/* Initialize buffers, memory allocation may fail here. */ 
	status = nano_spi_buff_init();
	if (status) {
		spi_unregister_driver(&nano_spi_driver);
		TRSP_MSG("nano_spi_buff_init failed\n");
		return status;
	}
	
	/* Initialize SPI transfer descriptors and SPI messages. */
	nano_spi_xfer_init();
	nano_spi_message_init();
	return 0;
}


/*!
 * @brief Unregisters the driver from kernel SPI API.
 *
 * This function is called by spi_exit() when the driver is unloaded.
 * It destroys all objects created by nano_spi_register().
 */
void nano_spi_unregister(void)
{
	nano_spi_buff_exit();
	spi_unregister_driver(&nano_spi_driver);
}

/*! @} */ /* End of kspi_kernel_api group */
