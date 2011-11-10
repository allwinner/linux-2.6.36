/*
 * ---------------------------------------------------------------------------
 * FILE:     sme_drv.c
 * 
 * PURPOSE:
 *      This file provides the implementation of the connection establishment
 *      between the SME SHIM layer and the unifi driver.
 *
 * Copyright (C) 2006-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */
    
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "csr_types.h"
#include "sme_drv.h"
#include "unifiio.h"
#include "driver/conversions.h"
#include "csr_util.h"

#define MIB_TAG_INTEGER     0x02
#define MIB_TAG_OCTETSTRING 0x04
#define MIB_TAG_OID         0x06
#define MIB_TAG_SEQUENCE    0x30

#define MAX_VARBIND_LENGTH 127
#define dot11MACAddress_oid "1.2.840.10036.2.1.1.1.1"

int
wait_for_event(MANAGER_PRIV_DATA *priv, int signal_id)
{
    static unsigned char buffer[4096];
    fd_set readfds;
    int reply_len;
    CSR_SIGNAL *reply_signal;

    while (1) {
        int r;
        udi_msg_t *msg;
        unsigned char *sigptr;
        int signal_and_data;
        
        FD_ZERO(&readfds);
        FD_SET(priv->Fd, &readfds);

        /* This will poll in the char device for an event. */
        r = select(priv->Fd+1, &readfds, NULL, NULL, NULL);
        if (r < 0) {
            perror("Error from select");
            return ERROR_IO_ERROR;
        }

        /* This read will obtain the indication. */
        r = read(priv->Fd, buffer, 4096);
        if (r < 0) {
            perror("Error reading UDI msg");
            return ERROR_IO_ERROR;
        }

        /* We get a udi_msg_t, and the signal is appended right after the structure. */
        msg = (udi_msg_t *)buffer;
        sigptr = (unsigned char *)(msg + 1);

        /* Total length of signal body + any bulk data */
        signal_and_data = COAL_GET_UINT32_FROM_LITTLE_ENDIAN((buffer + offsetof(udi_msg_t, length))) - sizeof(udi_msg_t);

        /*
        * Check if the indication is actual a config indication, 
        * rather than a signal..
        */
        if (COAL_GET_UINT32_FROM_LITTLE_ENDIAN((buffer + offsetof(udi_msg_t, direction))) == UDI_CONFIG_IND) {
            /* .. in this case the information is just one byte. */
            return (int)(*sigptr);
        }
        
        reply_len = signal_and_data;
        if (reply_len > 0) {
            reply_signal = (CSR_SIGNAL*)sigptr;
            if (signal_id == GET_SIGNAL_ID(sigptr)) {
                if (signal_id == CSR_MLME_GET_CONFIRM_ID) {
                    unsigned char varbind[32];
                    unsigned char *ptr;
                    int oid_len;
                    const int oid_offset = offsetof(struct CSR_SIGNAL_PRIMITIVE, u) + sizeof(CSR_MLME_GET_CONFIRM) + 2;
        
                    /* We are looking for this pattern. */
                    oid_len = MibEncodeOID(dot11MACAddress_oid, varbind);
                    /* Length of the signal with bulk data should be at least.. */
                    if (reply_len >= oid_offset + oid_len + 2 + 6) {
                        /* .. and the pattern should match. */
                        if (memcmp(sigptr + oid_offset, varbind, oid_len) == 0) {
                            ptr = sigptr + oid_offset + oid_len;
                            if ((ptr[0] == MIB_TAG_OCTETSTRING) && (ptr[1] == 6)) {
                                /* Store the MAC address, skip two bytes of the NULL decode. */
                                memcpy(priv->unifi_macaddress, ptr + 2, 6);
                            }
                        }
                    }
                }
                /* This is what we are expecting for, return success. */
                return 0;
            }
        }
    }
    
    return 0;
} /* wait_for_event() */


static int
send_mlme_set_req(MANAGER_PRIV_DATA *priv, unsigned char *req_data, unsigned int req_len)
{
    unsigned char *sig_ptr;
    CSR_SIGNAL *sig;
    CSR_DATAREF *datarefptr;
    int r = 0;
    unsigned int signal_len = offsetof(struct CSR_SIGNAL_PRIMITIVE, u) + sizeof(CSR_MLME_SET_REQUEST);

    sig_ptr = malloc(signal_len + req_len);
    if (sig_ptr == NULL) {
        return ERROR_IO_ERROR;
    }
    memset(sig_ptr, 0, signal_len + req_len);
    sig = (CSR_SIGNAL *)sig_ptr;

    sig->SignalPrimitiveHeader.SignalId = CSR_MLME_SET_REQUEST_ID;
    sig->SignalPrimitiveHeader.ReceiverProcessId = 0;
    sig->SignalPrimitiveHeader.SenderProcessId = 0xC013;
    datarefptr = (CSR_DATAREF*)&sig->u;
    datarefptr->DataLength = req_len;

    memcpy(sig_ptr + signal_len, req_data, req_len);
    signal_len += req_len;
    
    /* Send the command to the unifi. */
    if (write(priv->Fd, sig_ptr, signal_len) != signal_len) {
        r = ERROR_IO_ERROR;
    }

    free(sig_ptr);
    return r;
} /* send_mlme_set_req() */

static int
send_mlme_get_req(MANAGER_PRIV_DATA *priv, unsigned char *req_data, unsigned int req_len)
{
    unsigned char *sig_ptr;
    CSR_SIGNAL *sig;
    CSR_DATAREF *datarefptr;
    int r = 0;
    unsigned int signal_len = offsetof(struct CSR_SIGNAL_PRIMITIVE, u) + sizeof(CSR_MLME_GET_REQUEST);

    sig_ptr = malloc(signal_len + req_len);
    if (sig_ptr == NULL) {
        return ERROR_IO_ERROR;
    }
    memset(sig_ptr, 0, signal_len + req_len);
    sig = (CSR_SIGNAL *)sig_ptr;

    sig->SignalPrimitiveHeader.SignalId = CSR_MLME_GET_REQUEST_ID;
    sig->SignalPrimitiveHeader.ReceiverProcessId = 0;
    sig->SignalPrimitiveHeader.SenderProcessId = 0xC013;
    datarefptr = (CSR_DATAREF*)&sig->u;
    datarefptr->DataLength = req_len;

    memcpy(sig_ptr + signal_len, req_data, req_len);
    signal_len += req_len;

    /* Send the command to the unifi. */
    if (write(priv->Fd, sig_ptr, signal_len) != signal_len) {
        r =  ERROR_IO_ERROR;
    }

    free(sig_ptr);
    return r;
} /* send_mlme_get_req() */



int
drv_set_mac_address(MANAGER_PRIV_DATA *priv)
{
    unsigned char varbind[MAX_VARBIND_LENGTH];
    unsigned char *ptr;
    int len, r;
    unsigned char invalid_macaddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    /* Build a varbind of OID and INTEGER */
    varbind[0] = MIB_TAG_SEQUENCE;
    ptr = varbind + 2;          /* assumes length will always be one octet */

    /* Build the MAC address MLME-GET.req */
    len = MibEncodeOID(dot11MACAddress_oid, ptr);
    len += MibEncodeNull(ptr+len);
    varbind[1] = len;
    
    /* Send the MLME-GET.req */
    r = send_mlme_get_req(priv, varbind, len+2);
    if (r != 0) {
        return r;
    }
    
    /* Wait for the MLME-GET.cfm */
    r = wait_for_event(priv, CSR_MLME_GET_CONFIRM_ID);
    if (r != 0) {
        return r;
    }
    
    /* If the MAC passed from the SME a new valid .. */
    if ((memcmp(priv->macaddress, invalid_macaddr, 6) != 0) &&
        (memcmp(priv->macaddress, priv->unifi_macaddress, 6) != 0)) {
        /* .. update the existing address. */
        len = 0;
        /* Build the MAC address MLME-SET.req */
        len = MibEncodeOID(dot11MACAddress_oid, ptr);
        len += MibEncodeOctetString(priv->macaddress, 6, ptr+len);
        varbind[1] = len;
        
        /* Send the request. */
        r = send_mlme_set_req(priv, varbind, len+2);
        if (r != 0) {
            return r;
        }
        
        /* Wait for MAC address confirm. */
        r = wait_for_event(priv, CSR_MLME_SET_CONFIRM_ID);
        if (r != 0) {
            return r;
        }
    } else {
        memcpy(priv->macaddress, priv->unifi_macaddress, 6);
    }
    
    return 0;
} /* drv_set_mac_address() */


int
drv_set_reset_state(MANAGER_PRIV_DATA *priv)
{
    CSR_SIGNAL signal;
    CSR_MLME_RESET_REQUEST *req;
    int signal_len;
    
    memset((void*)&signal, 0, sizeof(CSR_SIGNAL));

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_RESET_REQUEST_ID;
    signal.SignalPrimitiveHeader.ReceiverProcessId = 0;
    signal.SignalPrimitiveHeader.SenderProcessId = 0xC013;

    /* Set up MLME-RESET request */
    req = &signal.u.MlmeResetRequest;
    memcpy(req->StaAddress.x, priv->macaddress, 6);
    req->SetDefaultMib = 1;
    
    signal_len = offsetof(struct CSR_SIGNAL_PRIMITIVE, u) + sizeof(CSR_MLME_RESET_REQUEST);

    /* Send the command to the unifi. */
    if (write(priv->Fd, (unsigned char *)&signal, signal_len) != signal_len) {
        return ERROR_IO_ERROR;
    }

    return 0;
} /* drv_set_reset_state() */


static int
_dl_mibs(MANAGER_PRIV_DATA *priv, unsigned char *mibdata, int miblen)
{
    unsigned char *header;
    int version;
    int filelen;
    unsigned char *varbind;
    int r;

    /* Check mibdata contains at least the file header (10) and one MIB (2) */
    if (miblen < 12) {
        perror("Invalid MIB file");
        return ERROR_IO_ERROR;
    }

    /* 
     * Read file header.
     */
    header = mibdata;
    mibdata += 10;

    if ((header[0] != 'U') || (header[1] != 'D') ||
        (header[2] != 'M') || (header[3] != 'I'))
    {
        perror("Bad header in MIB file");
        return ERROR_IO_ERROR;
    }
    version = header[4] + (header[5] << 8);
    filelen = header[6] + (header[7] << 8) +
        (header[8] << 16) + (header[9] << 24);

    /* Check file length from file against given file size */
    if ((filelen + 10) != miblen) {
        perror("Length mismatch for MIB file.\n");

        /* Truncate filelen if necessary */
        if ((filelen + 10) > miblen) {
            filelen = miblen - 10;
        }
    }

    /* 
     * Read and set MIBs
     */
    while (filelen > 2)
    {
        int datalen;
        unsigned char *vblen;

        /* Read length field */
        vblen = mibdata;
        mibdata += 2;
        filelen -= 2;

        datalen = (vblen[1] << 8) + vblen[0];
        if (datalen > filelen) {
            perror("End of file reached reading MIB\n");
            return ERROR_IO_ERROR;
        }

        /* Now read the varbind itself */
        varbind = mibdata;
        mibdata += datalen;
        filelen -= datalen;

        /* Check */
        if ((varbind[1] + 2) != datalen) {
            perror("Malformed MIB entry, bad length.\n");
            return ERROR_IO_ERROR;
        }

        r = send_mlme_set_req(priv, varbind, varbind[1] + 2);
        if (r < 0) {
            /* Exit immediately */
            return r;
        }
        r = wait_for_event(priv, CSR_MLME_SET_CONFIRM_ID);
        if (r != 0) {
            /* Exit immediately */
            return r;
        }
    }

    return 0;

} /* _dl_mibs() */


int
drv_download_mib(MANAGER_PRIV_DATA *priv, char *mib_file)
{
    FILE *fp;
    struct stat st;
    int r;
    unsigned char *img_data;
    unsigned int img_len;

    if (stat(mib_file, &st)) {
        perror("Failed to stat img file");
        return ERROR_IO_ERROR;
    }

    img_len = st.st_size;
    fp = fopen(mib_file, "r");
    if (!fp) {
        perror("Failed to open firmware file");
        return ERROR_IO_ERROR;
    }
    img_data = (unsigned char *)malloc(img_len);
    if (img_data == NULL) {
        perror("Failed to allocate memory for firmware");
        fclose(fp);
        return ERROR_IO_ERROR;
    }
    /* NB This assumes fread() will successfully read whole file in one call
     */
    if (fread(img_data, 1, img_len, fp) != img_len) {
        perror("Failed to read MIB file");
        free(img_data);
        fclose(fp);
        return ERROR_IO_ERROR;
    }

    fclose(fp);
    
    r = _dl_mibs(priv, img_data, img_len);
    if (r) {
        perror("Failed to download UniFi MIBs");
    }
    
    free(img_data);
    return r;
} /* drv_download_mib() */

