#ifndef	__LINUX_USBNET_ASIX_H
#define	__LINUX_USBNET_ASIX_H

/* ASIX AX8817X based USB 2.0 Ethernet Devices */
#define AX_CMD_SET_SW_MII		0x06
#define AX_CMD_READ_MII_REG		0x07
#define AX_CMD_WRITE_MII_REG		0x08
#define AX_CMD_SET_HW_MII		0x0a
#define AX_CMD_READ_EEPROM		0x0b
#define AX_CMD_WRITE_EEPROM		0x0c
#define AX_CMD_WRITE_ENABLE		0x0d
#define AX_CMD_WRITE_DISABLE		0x0e
#define AX_CMD_READ_RX_CTL		0x0f
#define AX_CMD_WRITE_RX_CTL		0x10
#define AX_CMD_READ_IPG012		0x11
#define AX_CMD_WRITE_IPG0		0x12
#define AX_CMD_WRITE_IPG1		0x13
#define AX_CMD_READ_NODE_ID		0x13
#define AX_CMD_WRITE_NODE_ID		0x14
#define AX_CMD_WRITE_IPG2		0x14
#define AX_CMD_WRITE_MULTI_FILTER	0x16
#define AX88172_CMD_READ_NODE_ID	0x17
#define AX_CMD_READ_PHY_ID		0x19
#define AX_CMD_READ_MEDIUM_STATUS	0x1a
#define AX_CMD_WRITE_MEDIUM_MODE	0x1b
#define AX_CMD_READ_MONITOR_MODE	0x1c
#define AX_CMD_WRITE_MONITOR_MODE	0x1d
#define AX_CMD_READ_GPIOS		0x1e
#define AX_CMD_WRITE_GPIOS		0x1f
#define AX_CMD_SW_RESET			0x20
#define AX_CMD_SW_PHY_STATUS		0x21
#define AX_CMD_SW_PHY_SELECT		0x22

#define AX_PHYSEL_PSEL		(1 << 0)
#define AX_PHYSEL_ASEL		(1 << 1)
#define AX_PHYSEL_SSMII		(1 << 2)
#define AX_PHYSEL_SSRMII	(2 << 2)
#define AX_PHYSEL_SSRRMII	(3 << 2)
#define AX_PHYSEL_SSEN		(1 << 4)
#define AX88772_CMD_READ_NODE_ID	0x13
#define AX88772_CMD_WRITE_NODE_ID	0x14
#define AX_CMD_READ_RXCOE_CTL		0x2b
#define AX_CMD_WRITE_RXCOE_CTL		0x2c
#define AX_CMD_READ_TXCOE_CTL		0x2d
#define AX_CMD_WRITE_TXCOE_CTL		0x2e

#define AX_MONITOR_MODE			0x01
#define AX_MONITOR_LINK			0x02
#define AX_MONITOR_MAGIC		0x04
#define AX_MONITOR_HSFS			0x10

/* AX88172 Medium Status Register values */
#define AX88172_MEDIUM_FD		0x02
#define AX88172_MEDIUM_TX		0x04
#define AX88172_MEDIUM_FC		0x10
#define AX88172_MEDIUM_DEFAULT \
		( AX88172_MEDIUM_FD | AX88172_MEDIUM_TX | AX88172_MEDIUM_FC )

#define AX_MCAST_FILTER_SIZE		8
#define AX_MAX_MCAST			64

#define AX_SWRESET_CLEAR		0x00
#define AX_SWRESET_RR			0x01
#define AX_SWRESET_RT			0x02
#define AX_SWRESET_PRTE			0x04
#define AX_SWRESET_PRL			0x08
#define AX_SWRESET_BZ			0x10
#define AX_SWRESET_IPRL			0x20
#define AX_SWRESET_IPPD			0x40

#define AX88772_IPG0_DEFAULT		0x15
#define AX88772_IPG1_DEFAULT		0x0c
#define AX88772_IPG2_DEFAULT		0x12

#define AX88772A_IPG0_DEFAULT		0x15
#define AX88772A_IPG1_DEFAULT		0x16
#define AX88772A_IPG2_DEFAULT		0x1A

/* AX88772 & AX88178 Medium Mode Register */
#define AX_MEDIUM_PF		0x0080
#define AX_MEDIUM_JFE		0x0040
#define AX_MEDIUM_TFC		0x0020
#define AX_MEDIUM_RFC		0x0010
#define AX_MEDIUM_ENCK		0x0008
#define AX_MEDIUM_AC		0x0004
#define AX_MEDIUM_FD		0x0002
#define AX_MEDIUM_GM		0x0001
#define AX_MEDIUM_SM		0x1000
#define AX_MEDIUM_SBP		0x0800
#define AX_MEDIUM_PS		0x0200
#define AX_MEDIUM_RE		0x0100

#define AX88178_MEDIUM_DEFAULT	\
	(AX_MEDIUM_PS | AX_MEDIUM_FD | AX_MEDIUM_AC | \
	 AX_MEDIUM_RFC | AX_MEDIUM_TFC | AX_MEDIUM_JFE | \
	 AX_MEDIUM_RE)

#define AX88772_MEDIUM_DEFAULT	\
	(AX_MEDIUM_FD | AX_MEDIUM_RFC | \
	 AX_MEDIUM_TFC | AX_MEDIUM_PS | \
	 AX_MEDIUM_AC | AX_MEDIUM_RE)

/* AX88772 & AX88178 RX_CTL values */
#define AX_RX_CTL_SO		0x0080
#define AX_RX_CTL_AP		0x0020
#define AX_RX_CTL_AM		0x0010
#define AX_RX_CTL_AB		0x0008
#define AX_RX_CTL_SEP		0x0004
#define AX_RX_CTL_AMALL		0x0002
#define AX_RX_CTL_PRO		0x0001
#define AX_RX_CTL_MFB_2048	0x0000
#define AX_RX_CTL_MFB_4096	0x0100
#define AX_RX_CTL_MFB_8192	0x0200
#define AX_RX_CTL_MFB_16384	0x0300

#define AX_DEFAULT_RX_CTL	(AX_RX_CTL_SO | AX_RX_CTL_AB)

/* GPIO 0 .. 2 toggles */
#define AX_GPIO_GPO0EN		0x01	/* GPIO0 Output enable */
#define AX_GPIO_GPO_0		0x02	/* GPIO0 Output value */
#define AX_GPIO_GPO1EN		0x04	/* GPIO1 Output enable */
#define AX_GPIO_GPO_1		0x08	/* GPIO1 Output value */
#define AX_GPIO_GPO2EN		0x10	/* GPIO2 Output enable */
#define AX_GPIO_GPO_2		0x20	/* GPIO2 Output value */
#define AX_GPIO_RESERVED	0x40	/* Reserved */
#define AX_GPIO_RSE		0x80	/* Reload serial EEPROM */

#define AX_EEPROM_MAGIC		0xdeadbeef
#define AX88172_EEPROM_LEN	0x40
#define AX88772_EEPROM_LEN	0xff

#define PHY_MODE_MARVELL	0x0000
#define MII_MARVELL_LED_CTRL	0x0018
#define MII_MARVELL_STATUS	0x001b
#define MII_MARVELL_CTRL	0x0014

#define MARVELL_LED_MANUAL	0x0019

#define MARVELL_STATUS_HWCFG	0x0004

#define MARVELL_CTRL_TXDELAY	0x0002
#define MARVELL_CTRL_RXDELAY	0x0080

#define	PHY_MODE_RTL8211CL	0x0004

/* This structure cannot exceed sizeof(unsigned long [5]) AKA 20 bytes */
struct asix_data {
	u8 multi_filter[AX_MCAST_FILTER_SIZE];
	u8 mac_addr[ETH_ALEN];
	u8 phymode;
	u8 ledmode;
	u8 eeprom_len;
	u8 checksum;
};

struct ax88172_int_data {
	__le16 res1;
	u8 link;
	__le16 res2;
	u8 status;
	__le16 res3;
} __packed;

/* just for ax88772b */
#define AX88772B_MAX_BULKIN_2K		0
#define AX88772B_MAX_BULKIN_4K		1
#define AX88772B_MAX_BULKIN_6K		2
#define AX88772B_MAX_BULKIN_8K		3
#define AX88772B_MAX_BULKIN_16K		4
#define AX88772B_MAX_BULKIN_20K		5
#define AX88772B_MAX_BULKIN_24K		6
#define AX88772B_MAX_BULKIN_32K		7

#define AX_RX_CTL_RH1M			0x0100		/* Enable RX-Header mode 0 */
#define AX_RX_CTL_RH2M			0x0200		/* Enable IP header in receive buffer aligned on 32-bit aligment */
#define AX_RX_CTL_RH3M			0x0400		/* checksum value in rx header 3 */
#define AX_RX_HEADER_DEFAULT		(AX_RX_CTL_RH1M | AX_RX_CTL_RH2M)

#define AX_RXCOE_IPCE			0x0001
#define AX_RXCOE_IPVE			0x0002
#define AX_RXCOE_V6VE			0x0004
#define AX_RXCOE_TCPE			0x0008
#define AX_RXCOE_UDPE			0x0010
#define AX_RXCOE_ICMP			0x0020
#define AX_RXCOE_IGMP			0x0040
#define AX_RXCOE_ICV6			0x0080
#define AX_RXCOE_TCPV6			0x0100
#define AX_RXCOE_UDPV6			0x0200
#define AX_RXCOE_ICMV6			0x0400
#define AX_RXCOE_IGMV6			0x0800
#define AX_RXCOE_ICV6V6			0x1000
#define AX_RXCOE_FOPC			0x8000
#define AX_RXCOE_DEF_CSUM		(AX_RXCOE_IPCE | AX_RXCOE_IPVE | \
					 AX_RXCOE_V6VE | AX_RXCOE_TCPE | \
					 AX_RXCOE_UDPE |  AX_RXCOE_ICV6 | \
					 AX_RXCOE_TCPV6 | AX_RXCOE_UDPV6)
#define AX_RXCOE_64TE			0x0100
#define AX_RXCOE_PPPOE			0x0200
#define AX_RXCOE_RPCE			0x8000

#define AX_TXCOE_IP			0x0001
#define AX_TXCOE_TCP			0x0002
#define AX_TXCOE_UDP			0x0004
#define AX_TXCOE_ICMP			0x0008
#define AX_TXCOE_IGMP			0x0010
#define AX_TXCOE_ICV6			0x0020
#define AX_TXCOE_TCPV6			0x0100
#define AX_TXCOE_UDPV6			0x0200
#define AX_TXCOE_ICMV6			0x0400
#define AX_TXCOE_IGMV6			0x0800
#define AX_TXCOE_ICV6V6			0x1000
#define AX_TXCOE_DEF_CSUM		(AX_TXCOE_TCP | AX_TXCOE_UDP | \
					 AX_TXCOE_TCPV6 | AX_TXCOE_UDPV6)

#define AX_RX_CHECKSUM		1
#define AX_TX_CHECKSUM		2

struct {unsigned short size, byte_cnt,threshold;} AX88772B_BULKIN_SIZE[] =
{
	/* 2k */
	{2048, 0x8000, 0x8001},
	/* 4k */
	{4096, 0x8100, 0x8147},
	/* 6k */
	{6144, 0x8200, 0x81EB},
	/* 8k */
	{8192, 0x8300, 0x83D7},
	/* 16 */
	{16384, 0x8400, 0x851E},
	/* 20k */
	{20480, 0x8500, 0x8666},
	/* 24k */
	{24576, 0x8600, 0x87AE},
	/* 32k */
	{32768, 0x8700, 0x8A3D},
};

#define AX_RXHDR_L4_ERR		(1 << 8)
#define AX_RXHDR_L3_ERR		(1 << 9)

#define AX_RXHDR_L4_TYPE_UDP		1
#define AX_RXHDR_L4_TYPE_ICMP		2
#define AX_RXHDR_L4_TYPE_IGMP		3
#define AX_RXHDR_L4_TYPE_TCP		4
#define AX_RXHDR_L4_TYPE_TCMPV6	5
#define AX_RXHDR_L4_TYPE_MASK		7

#define AX_RXHDR_L3_TYPE_IP		1
#define AX_RXHDR_L3_TYPE_IPV6		2

struct ax88772b_rx_header {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u16	len:11,
		res1:1,
		crc:1,
		mii:1,
		runt:1,
		mc_bc:1;

	u16	len_bar:11,
		res2:5;

	u8	vlan_ind:3,
		vlan_tag_striped:1,
		pri:3,
		res3:1;

	u8	l4_csum_err:1,
		l3_csum_err:1,
		l4_type:3,
		l3_type:2,
		ce:1;
#elif defined (__BIG_ENDIAN_BITFIELD)
	u16	mc_bc:1,
		runt:1,
		mii:1,
		crc:1,
		res1:1,
		len:11;

	u16	res2:5,
		len_bar:11;

	u8	res3:1,
		pri:3,
		vlan_tag_striped:1,
		vlan_ind:3;

	u8	ce:1,
		l3_type:2,
		l4_type:3,
		l3_csum_err:1,
		l4_csum_err:1;
#else
#error	"Please fix <asm/byteorder.h>"
#endif

} __attribute__ ((packed));

#endif /* __LINUX_USBNET_ASIX_H */
