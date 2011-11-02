/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2000-2002  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2003-2007  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "parser/parser.h"

/* ------------------------------------------------------------- */
/* Remote sme                                                    */
/* ------------------------------------------------------------- */
#ifdef IPC_IP
#include "hcidump_to_sme.h"
static ipcConnection* smeConnection;
ipcConnection* get_bt_ipc_connection(FsmContext* context)
{
    return smeConnection;
}
extern CsrBool remote_bt_signal_receive(FsmContext* context, CsrUint8* buffer, CsrUint16 size);
#endif
/* ------------------------------------------------------------- */

#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t ntoh64(uint64_t n)
{
	uint64_t h;
	uint64_t tmp = ntohl(n & 0x00000000ffffffff);
	h = ntohl(n >> 32);
	h |= tmp << 32;
	return h;
}
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ntoh64(x) (x)
#else
#error "Unknown byte order"
#endif
#define hton64(x) ntoh64(x)

#define SNAP_LEN 	HCI_MAX_FRAME_SIZE
#define DEFAULT_PORT	"10839";

/* Default options */
static int  snap_len = SNAP_LEN;

struct hcidump_hdr {
	uint16_t	len;
	uint8_t		in;
	uint8_t		pad;
	uint32_t	ts_sec;
	uint32_t	ts_usec;
} __attribute__ ((packed));
#define HCIDUMP_HDR_SIZE (sizeof(struct hcidump_hdr))

struct btsnoop_hdr {
	uint8_t		id[8];		/* Identification Pattern */
	uint32_t	version;	/* Version Number = 1 */
	uint32_t	type;		/* Datalink Type */
} __attribute__ ((packed));
#define BTSNOOP_HDR_SIZE (sizeof(struct btsnoop_hdr))

struct btsnoop_pkt {
	uint32_t	size;		/* Original Length */
	uint32_t	len;		/* Included Length */
	uint32_t	flags;		/* Packet Flags */
	uint32_t	drops;		/* Cumulative Drops */
	uint64_t	ts;		/* Timestamp microseconds */
	uint8_t		data[0];	/* Packet Data */
} __attribute__ ((packed));
#define BTSNOOP_PKT_SIZE (sizeof(struct btsnoop_pkt))

struct pktlog_hdr {
	uint32_t	len;
	uint64_t	ts;
	uint8_t		type;
} __attribute__ ((packed));
#define PKTLOG_HDR_SIZE (sizeof(struct pktlog_hdr))

static inline int read_n(int fd, char *buf, int len)
{
	int t = 0, w;

	while (len > 0) {
		if ((w = read(fd, buf, len)) < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return -1;
		}
		if (!w)
			return 0;
		len -= w; buf += w; t += w;
	}
	return t;
}

static inline int write_n(int fd, char *buf, int len)
{
	int t = 0, w;

	while (len > 0) {
		if ((w = write(fd, buf, len)) < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return -1;
		}
		if (!w)
			return 0;
		len -= w; buf += w; t += w;
	}
	return t;
}

static int open_socket(int dev, unsigned long flags)
{
    struct sockaddr_hci addr;
    struct hci_filter flt;
    struct hci_dev_info di;
    int sk, dd, opt;

    if (dev != HCI_DEV_NONE) {
        dd = hci_open_dev(dev);
        if (dd < 0) {
            //perror("Can't open device");
            return -1;
        }

        if (hci_devinfo(dev, &di) < 0) {
            //perror("Can't get device info");
            return -1;
        }

        opt = hci_test_bit(HCI_RAW, &di.flags);
        if (ioctl(dd, HCISETRAW, opt) < 0) {
            if (errno == EACCES) {
                //perror("Can't access device");
                return -1;
            }
        }

        hci_close_dev(dd);
    }

    /* Create HCI socket */
    sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (sk < 0) {
        //perror("Can't create raw socket");
        return -1;
    }

    opt = 1;
    if (setsockopt(sk, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0) {
        //perror("Can't enable data direction info");
        return -1;
    }

    opt = 1;
    if (setsockopt(sk, SOL_HCI, HCI_TIME_STAMP, &opt, sizeof(opt)) < 0) {
        //perror("Can't enable time stamp");
        return -1;
    }

    /* Setup filter */
    hci_filter_clear(&flt);
    hci_filter_all_ptypes(&flt);
    hci_filter_all_events(&flt);
    if (setsockopt(sk, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        //perror("Can't set filter");
        return -1;
    }

    /* Bind socket to the HCI device */
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = dev;
    if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        //printf("Can't attach to device hci%d. %s(%d)\n",
        //            dev, strerror(errno), errno);
        return -1;
    }

    return sk;
}

static int find_device_by_address_callback(int dd, int dev_id, long arg)
{
    bdaddr_t* searchaddress = (bdaddr_t *)arg;
    bdaddr_t deviceaddress;

    if (hci_devba(dev_id, &deviceaddress) >= 0)
    {
        char bastr[64];
        ba2str(&deviceaddress, bastr);
        printf("find_device_by_address_callback(hci%d, %s)\n", dev_id, bastr);

        if (memcmp(searchaddress->b, deviceaddress.b, sizeof(deviceaddress.b)) == 0)
        {
            return 1;
        }
    }
    return 0;
}


static int process_frames(bdaddr_t address, unsigned long flags)
{
	struct cmsghdr *cmsg;
	struct msghdr msg;
	struct iovec  iv;
	struct hcidump_hdr *dh;
	struct btsnoop_pkt *dp;
	struct frame frm;
	struct pollfd fds[2];
	int nfds = 0;
	char *buf, *ctrl;
	int len, hdr_size = HCIDUMP_HDR_SIZE;
	int polltimeout = -1;
	int sock = -1;
	int device = -1;

	if (snap_len < SNAP_LEN)
		snap_len = SNAP_LEN;

	if (flags & DUMP_BTSNOOP)
		hdr_size = BTSNOOP_PKT_SIZE;

	buf = malloc(snap_len + hdr_size);
	if (!buf) {
		perror("Can't allocate data buffer");
		return -1;
	}

	dh = (void *) buf;
	dp = (void *) buf;
	frm.data = buf + hdr_size;

	ctrl = malloc(100);
	if (!ctrl) {
		free(buf);
		perror("Can't allocate control buffer");
		return -1;
	}

	printf("snap_len: %d filter: 0x%lx\n", snap_len, parser.filter);

	memset(&msg, 0, sizeof(msg));

	while (1)
	{
	    nfds = 0;
	    if (sock == -1)
	    {
            device = hci_for_each_dev(0, find_device_by_address_callback, (long)&address);
            if (device != -1)
            {
                sock = open_socket(device, flags);
                if (sock != -1)
                {
                    printf("hci%d Connected\n", device);
                    (void)fflush(stdout);
                    onBtConnect();
                }
            }
	    }

        if (sock != -1)
        {
            fds[nfds].fd = sock;
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            nfds++;
        }

        /* ------------------------------------------------------------- */
        /* Remote sme                                                    */
        /* ------------------------------------------------------------- */
#ifdef IPC_IP
        // If we are not connected attempt to connect
        if (!smeConnection)
        {
            //printf("Sme Connect :: tcp:localhost:10101\n");
            //(void)fflush(stdout);
            smeConnection = ipc_ip_connect("tcp:localhost:10101", NULL, NULL, NULL, NULL);
            if (smeConnection)
            {
                printf("Sme Connected\n");
                (void)fflush(stdout);
                onIpcConnect();
            }
        }

        if (smeConnection)
        {
            fds[nfds].fd = ipc_getFd(smeConnection);
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            nfds++;
        }

        polltimeout = sock==-1?500:smeConnection?-1:500;
#endif
        /* ------------------------------------------------------------- */
        (void)fflush(stdout);
	    int i, n = poll(fds, nfds, polltimeout);
        (void)fflush(stdout);

	    if (n <= 0)
			continue;

		for (i = 0; i < nfds; i++) {
			if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
				if (fds[i].fd == sock)
				{
					printf("device: disconnected\n");
					sock = -1;
                    onBtDisconnect();
                    continue;
				}
#ifdef IPC_IP
                else if (smeConnection && fds[i].fd == ipc_getFd(smeConnection))
                {
                    printf("Sme Disconnected\n");
                    (void)fflush(stdout);
                    ipc_disconnect(smeConnection);
                    smeConnection = NULL;
                    continue;
                }
#endif
				else
				{
					printf("client: disconnect\n");
				}
			}
		}

        for (i = 0; i < nfds; i++)
        {
#ifdef IPC_IP
            if (smeConnection && fds[i].fd == ipc_getFd(smeConnection) && fds[i].revents & (POLLIN | POLLPRI))
            {
                CsrUint8* smeRxBuffer;
                CsrUint32 smeRxLength = 0;

                printf("Sme fd Triggered\n");
                (void)fflush(stdout);

                if (ipc_message_recv(smeConnection, 0, &smeRxBuffer, &smeRxLength))
                {
                    if (smeRxLength != 0)
                    {
                        if (!remote_bt_signal_receive(NULL, smeRxBuffer, (CsrUint16)smeRxLength))
                        {
                            printf("Sme unhandled ipc message\n");
                            (void)fflush(stdout);
                        }
                        ipc_message_free(smeConnection, smeRxBuffer);
                    }
                    continue;
                }

                /* Opps Disconnected */
                printf("Sme Disconnected\n");
                (void)fflush(stdout);
                ipc_disconnect(smeConnection);
                smeConnection = NULL;
                nfds--;
                onIpcDisconnect();
                continue;
            }
#endif

            if (sock != -1 && fds[i].fd == sock && fds[i].revents & (POLLIN | POLLPRI))
            {
                iv.iov_base = frm.data;
                iv.iov_len  = snap_len;

                msg.msg_iov = &iv;
                msg.msg_iovlen = 1;
                msg.msg_control = ctrl;
                msg.msg_controllen = 100;

                len = recvmsg(sock, &msg, MSG_DONTWAIT);
                if (len < 0) {
                    if (errno == EAGAIN || errno == EINTR)
                        continue;
                    perror("Receive failed");
                    return -1;
                }

                /* Process control message */
                frm.data_len = len;
                frm.dev_id = device;
                frm.in = 0;
                frm.pppdump_fd = parser.pppdump_fd;
                frm.audio_fd   = parser.audio_fd;

                cmsg = CMSG_FIRSTHDR(&msg);
                while (cmsg) {
                    switch (cmsg->cmsg_type) {
                    case HCI_CMSG_DIR:
                        frm.in = *((int *) CMSG_DATA(cmsg));
                        break;
                    case HCI_CMSG_TSTAMP:
                        frm.ts = *((struct timeval *) CMSG_DATA(cmsg));
                        break;
                    }
                    cmsg = CMSG_NXTHDR(&msg, cmsg);
                }

                frm.ptr = frm.data;
                frm.len = frm.data_len;

                /* Parse and print */
                parse(&frm);
            }
        }
	}

	return 0;
}


static void usage(void)
{
    printf(
    "Usage: bluezcoexsniffer [OPTION...]\n"
    "  -a address       Device Address    (-a XX:XX:XX:XX:XX:XX)\n"
    "  -h               Give this help list\n"
    );
}

int main(int argc, char *argv[])
{
    unsigned long flags = DUMP_VERBOSE;
    unsigned long filter = ~0L;
    bdaddr_t address;
    int defpsm = 0;
    int defcompid = DEFAULT_COMPID;
    int opt, pppdump_fd = -1, audio_fd = -1;

    printf("Csr Bluez Coex Sniffer\n");

	while ((opt=getopt_long(argc, argv, "a:i:", NULL, NULL)) != -1) {
		switch(opt) {
        case 'a':
            printf("Address = %s\n", optarg);
            str2ba(optarg, &address);
            break;
		case 'h':
		default:
			usage();
			exit(0);
		}
	}

    init_parser(flags, filter, defpsm, defcompid, pppdump_fd, audio_fd);
    process_frames(address, flags);

    return 0;
}
