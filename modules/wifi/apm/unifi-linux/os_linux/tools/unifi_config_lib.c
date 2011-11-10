/*
 * ***************************************************************************
 *
 *  FILE: unifi_config_lib.c
 *
 *      Get/Set configuration to the UniFi driver library.
 *
 * Copyright (C) 2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <stddef.h>

#include "csr_types.h"
#include "hostio/hip_fsm_types.h"
#include "smeio/smeio_fsm_types.h"
#include "unifiio.h"
#include "unifi_config_lib.h"

#if 0
static void
dump(unsigned char* in_buffer, unsigned int count)
{
    unsigned int i;
    for (i=0; i<count; i++) {
        printf("%02X ", in_buffer[i]);
        if ((i % 16) == 15) {
            printf("\n");
        }
    }
    printf("\n");
}
#endif

static void TcpIp4Tclas(tclas_t* tclas)
{
    memset(tclas, 0, sizeof(tclas_t));
    tclas->element_id = 14;
    tclas->length = sizeof(tcpip_clsfr_t) + 1;
    tclas->user_priority = 0;
    tclas->tcp_ip_cls_fr.cls_fr_type = 1;
    tclas->tcp_ip_cls_fr.version = 4;
}

static CsrUint32
FilterCups(tclas_t* tclas)
{
    TcpIp4Tclas(tclas);
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.source_port))[0] = 0x02;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.source_port))[1] = 0x77;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.dest_port))[0] = 0x02;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.dest_port))[1] = 0x77;
    tclas->tcp_ip_cls_fr.protocol = 0x11;
    tclas->tcp_ip_cls_fr.cls_fr_mask = 0x58; //bits: 3,4,6
    return sizeof(tclas_t);
}

static CsrUint32
FilterNbns(tclas_t* tclas)
{
    TcpIp4Tclas(tclas);
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.source_port))[0] = 0x00;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.source_port))[1] = 0x89;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.dest_port))[0] = 0x00;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.dest_port))[1] = 0x89;
    tclas->tcp_ip_cls_fr.protocol = 0x11;
    tclas->tcp_ip_cls_fr.cls_fr_mask = 0x58; //bits: 3,4,6

    return sizeof(tclas_t);
}

static CsrUint32
FilterNbds(tclas_t* tclas)
{
    TcpIp4Tclas(tclas);
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.source_port))[0] = 0x00;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.source_port))[1] = 0x8A;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.dest_port))[0] = 0x00;
    ((CsrUint8*)(&tclas->tcp_ip_cls_fr.dest_port))[1] = 0x8A;
    tclas->tcp_ip_cls_fr.protocol = 0x11;
    tclas->tcp_ip_cls_fr.cls_fr_mask = 0x58; //bits: 3,4,6
    return sizeof(tclas_t);
}

static int
_add_filter(unsigned char* in_buffer, unsigned int filter)
{
    uf_cfg_bcast_packet_filter_t *bcast_packet_filter;
    unsigned char *tclas;
    unsigned int bcast_packet_filter_size;
    unsigned int tclas_ies_length;
    CsrUint32 added_tclas_length;

    bcast_packet_filter = (uf_cfg_bcast_packet_filter_t*)in_buffer;

    bcast_packet_filter->filter_mode = 0;
    tclas = bcast_packet_filter->tclas_ies;
    bcast_packet_filter_size = sizeof(uf_cfg_bcast_packet_filter_t);
    tclas_ies_length = 0;

    bcast_packet_filter->arp_filter = filter & UNIFI_CFG_FILTER_ARP;
    bcast_packet_filter->dhcp_filter = filter & UNIFI_CFG_FILTER_DHCP;

    if(filter & UNIFI_CFG_FILTER_CUPS) {
        added_tclas_length = FilterCups((tclas_t*)(tclas + tclas_ies_length));
        tclas_ies_length += added_tclas_length;
        bcast_packet_filter_size += added_tclas_length;
    }

    if(filter & UNIFI_CFG_FILTER_NBNS) {
        added_tclas_length = FilterNbns((tclas_t*)(tclas + tclas_ies_length));
        tclas_ies_length += added_tclas_length;
        bcast_packet_filter_size += added_tclas_length;
    }

    if(filter & UNIFI_CFG_FILTER_NBDS) {
        added_tclas_length = FilterNbds((tclas_t*)(tclas + tclas_ies_length));
        tclas_ies_length += added_tclas_length;
        bcast_packet_filter_size += added_tclas_length;
    }

    bcast_packet_filter->tclas_ies_length = tclas_ies_length;

    /*
    dump(bcast_packet_filter, sizeof(uf_cfg_bcast_packet_filter_t));
    dump(tclas, bcast_packet_filter->tclas_ies_length);
    */

    return (tclas_ies_length) ? (bcast_packet_filter_size - 1) : bcast_packet_filter_size;
}


int unifi_cfg_init(char *device)
{
    int fd;

    /* Start communication with driver. */
    fd = open(device, O_RDWR);
    if (fd < 0) {
        printf("Failed to open device\n");
    }
    return fd;
}

int unifi_cfg_deinit(int fd)
{
    /* Kill communication with driver. */
    close(fd);

    return 0;
}

int unifi_cfg_set_filters(int fd, unsigned int filter_type)
{
    int r, filter_size;
    unsigned char buffer[128];

    *((unifi_cfg_command_t*)buffer) = UNIFI_CFG_FILTER;

    filter_size = _add_filter(buffer + sizeof(unsigned int) + sizeof(unifi_cfg_command_t),
                              filter_type);
    *((unsigned int*)(buffer + sizeof(unifi_cfg_command_t))) = filter_size;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("UNIFI_CFG: failed\n");
        return r;
    }

    return 0;
}


int unifi_cfg_set_wmm_qos_info(int fd, CsrUint8 wmm_qos_info)
{
    int r;
    unsigned char buffer[8];

    *((unifi_cfg_command_t*)buffer) = UNIFI_CFG_WMM_QOSINFO;
    *((CsrUint8*)(buffer + sizeof(unifi_cfg_command_t))) = wmm_qos_info;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_set_wmm_qos_info: failed\n");
        return r;
    }

    return 0;
}


int unifi_cfg_set_power_save(int fd, unifi_cfg_powersave_t power_mode)
{
    int r;
    unsigned char buffer[16];

    *((unifi_cfg_command_t*)buffer) = UNIFI_CFG_POWERSAVE;
    *((unifi_cfg_powersave_t*)(buffer + sizeof(unifi_cfg_command_t))) = power_mode;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_set_power_save: failed\n");
        return r;
    }

    return 0;
}

int unifi_cfg_set_power(int fd, unifi_cfg_power_t power)
{
    int r;
    unsigned char buffer[16];

    *((unifi_cfg_command_t*)buffer) = UNIFI_CFG_POWER;
    *((unifi_cfg_power_t*)(buffer + sizeof(unifi_cfg_command_t))) = power;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_set_power: failed\n");
        return r;
    }

    return 0;
}

int unifi_cfg_set_power_supply(int fd, unifi_cfg_powersupply_t power_supply)
{
    int r;
    unsigned char buffer[16];

    *((unifi_cfg_command_t*)buffer) = UNIFI_CFG_POWERSUPPLY;
    *((unifi_cfg_powersupply_t*)(buffer + sizeof(unifi_cfg_command_t))) = power_supply;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_set_power_supply: failed\n");
        return r;
    }

    return 0;
}

int unifi_cfg_set_strict_draft_n(int fd, CsrBool strict_draft_n)
{
    int r;
    unsigned char buffer[8];

    *((unifi_cfg_command_t*)buffer) = UNIFI_CFG_STRICT_DRAFT_N;
    *((CsrBool*)(buffer + sizeof(unifi_cfg_command_t))) = strict_draft_n;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_set_strict_draft_n: failed\n");
        return r;
    }

    return 0;
}

int unifi_cfg_set_enable_okc(int fd, CsrBool enable_okc)
{
    int r;
    unsigned char buffer[8];

    *((unifi_cfg_command_t *)buffer) = UNIFI_CFG_ENABLE_OKC;
    *((CsrBool *)(buffer + sizeof(unifi_cfg_command_t))) = enable_okc;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0)
    {
        printf("unifi_cfg_set_enable_okc: failed\n");
        return r;
    }

    return 0;
}

int unifi_cfg_wmm_addts(int fd, CsrUint32 addts_tid,
                        CsrUint8 *addts_ie, CsrUint8 addts_ie_length)
{
    int r;
    static unsigned char buffer[UNIFI_CFG_MAX_ADDTS_IE_LENGTH + 16];
    CsrUint8 *buf_pos;

    if (addts_ie_length > UNIFI_CFG_MAX_ADDTS_IE_LENGTH) {
        printf("unifi_cfg_wmm_addts: IE exceeds UNIFI_CFG_MAX_ADDTS_IE_LENGTH\n");
        return -EINVAL;
    }

    buf_pos = buffer;
    *((unifi_cfg_command_t*)buf_pos) = UNIFI_CFG_WMM_ADDTS;

    buf_pos += sizeof(unifi_cfg_command_t);
    *((CsrUint32*)buf_pos) = addts_tid;

    buf_pos += sizeof(CsrUint32);
    *((CsrUint8*)buf_pos) = addts_ie_length;

    buf_pos += sizeof(CsrUint8);
    memcpy(buf_pos, addts_ie, addts_ie_length);

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_wmm_addts: failed\n");
        return r;
    }

    return 0;
}


int unifi_cfg_wmm_delts(int fd, CsrUint32 delts_tid)
{
    int r;
    static unsigned char buffer[8];
    CsrUint8 *buf_pos;

    buf_pos = buffer;
    *((unifi_cfg_command_t*)buf_pos) = UNIFI_CFG_WMM_DELTS;

    buf_pos += sizeof(unifi_cfg_command_t);
    *((CsrUint32*)buf_pos) = delts_tid;

    r = ioctl(fd, UNIFI_CFG, buffer);
    if (r < 0) {
        printf("unifi_cfg_wmm_delts: failed\n");
        return r;
    }

    return 0;
}


int unifi_cfg_get_info(int fd, unifi_cfg_get_t type,
                       unsigned char *out_buffer)
{
    int r;

    if (out_buffer == NULL) {
        return -1;
    }

    *((unifi_cfg_command_t*)out_buffer) = UNIFI_CFG_GET;
    *((unifi_cfg_get_t*)(out_buffer + sizeof(unifi_cfg_command_t))) = type;

    r = ioctl(fd, UNIFI_CFG, out_buffer);
    if (r < 0) {
        printf("unifi_cfg_get_info: failed\n");
        return r;
    }

    return 0;
}

