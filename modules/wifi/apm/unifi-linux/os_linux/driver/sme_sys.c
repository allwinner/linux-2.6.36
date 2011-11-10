/*
 * ---------------------------------------------------------------------------
 * FILE:     sme_sys.c
 *
 * PURPOSE:
 *      Driver specific implementation of the SME SYS SAP.
 *      It is part of the porting exercise.
 *
 * Copyright (C) 2008-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ---------------------------------------------------------------------------
 */

#include "driver/unifiversion.h"
#include "unifi_priv.h"
#include "driver/conversions.h"


/*
 * This file implements the SME SYS API. It contains the following functions:
 * unifi_sys_media_status_req()
 * unifi_sys_hip_req()
 * unifi_sys_port_configure_req()
 * unifi_sys_wifi_on_req()
 * unifi_sys_wifi_off_req()
 * unifi_sys_suspend_rsp()
 * unifi_sys_resume_rsp()
 * unifi_sys_qos_control_req()
 * unifi_sys_configure_power_mode_req()
 * unifi_sys_wifi_on_rsp()
 * unifi_sys_wifi_off_rsp()
 * unifi_sys_multicast_address_rsp()
 * unifi_sys_traffic_config_req()
 * unifi_sys_traffic_classification_req()
 * unifi_sys_tclas_add_req()
 * unifi_sys_tclas_del_req()
 */


void unifi_sys_media_status_req(void* drvpriv, unifi_MediaStatus mediaStatus, CsrUint32 mediaTypeMask)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_media_status_req: invalid smepriv\n");
        return;
    }

    if (mediaTypeMask&unifi_MediaType80211)
    {
        if (mediaStatus == unifi_MediaConnected) {
            unifi_sys_ip_configured_ind(priv->smepriv, (priv->sta_ip_address != 0xFFFFFFFF));

            priv->m4_monitor_state = m4_trap;
            if (priv->m4_bulk_data.data_length > 0)
            {
                unifi_trace(priv, UDBG5, "unifi_sys_media_status_req: free M4 \n\n");
                unifi_net_data_free(priv, &priv->m4_bulk_data);
            }

            netif_carrier_on(priv->netdev);
            UF_NETIF_TX_WAKE_ALL_QUEUES(priv->netdev);

            priv->connected = UnifiConnected;
        } else  {
            netif_carrier_off(priv->netdev);
            priv->connected = UnifiNotConnected;

            priv->m4_monitor_state = m4_idle;
            if (priv->m4_bulk_data.data_length > 0)
            {
                unifi_trace(priv, UDBG5, "unifi_sys_media_status_req: free M4\n\n");
                unifi_net_data_free(priv, &priv->m4_bulk_data);
            }
        }
    }
}


void unifi_sys_hip_req(void* drvpriv,
                       CsrUint16 mlmeCommandLength, const CsrUint8 *mlmeCommand,
                       CsrUint16 dataRef1Length,    const CsrUint8 *dataRef1,
                       CsrUint16 dataRef2Length,    const CsrUint8 *dataRef2)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    bulk_data_param_t bulkdata;
    u8 *signal_ptr;
    int signal_length;
    int r;
    void *dest;

    if (priv == NULL) {
        return;
    }

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_hip_req: invalid smepriv\n");
        return;
    }

    unifi_trace(priv, UDBG4, "unifi_sys_hip_req: 0x04%X ---->\n",
                *((CsrUint16*)mlmeCommand));

    /* Construct the signal. */
    signal_ptr = (u8*)mlmeCommand;
    signal_length = mlmeCommandLength;

    /*
     * The MSB of the sender ID needs to be set to the client ID.
     * The LSB is controlled by the SME.
     */
    signal_ptr[5] = (priv->sme_cli->sender_id >> 8) & 0xff;

    /* Allocate buffers for the bulk data. */
    if (dataRef1Length) {
        r = unifi_net_data_malloc(priv, &bulkdata.d[0], dataRef1Length);
        if (r == 0) {
            dest = (void*)bulkdata.d[0].os_data_ptr;
            memcpy(dest, dataRef1, dataRef1Length);
            bulkdata.d[0].data_length = dataRef1Length;
        } else {
        }
    } else {
        bulkdata.d[0].data_length = 0;
    }
    if (dataRef2Length) {
        r = unifi_net_data_malloc(priv, &bulkdata.d[1], dataRef2Length);
        if (r == 0) {
            dest = (void*)bulkdata.d[1].os_data_ptr;
            memcpy(dest, dataRef2, dataRef2Length);
            bulkdata.d[1].data_length = dataRef2Length;
        } else {
        }
    } else {
        bulkdata.d[1].data_length = 0;
    }

    unifi_trace(priv, UDBG3, "SME SEND: Signal %s \n",
                lookup_signal_name(*((CsrUint16*)signal_ptr)));

    r = ul_send_signal_raw(priv, (const unsigned char*)signal_ptr,
                           signal_length, &bulkdata);
    if (r) {
        unifi_error(priv, "unifi_sys_hip_req: Failed to send signal\n");
        unifi_sys_wifi_off_ind(priv->smepriv, unifi_ControlError);
    }

    unifi_trace(priv, UDBG4, "unifi_sys_hip_req: <----\n");
}




/*
 * ---------------------------------------------------------------------------
 * configure_data_port
 *
 *      Store the new controlled port configuration.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *      port_cfg        Pointer to the port configuration
 *
 * Returns:
 *      An unifi_ControlledPortAction value.
 * ---------------------------------------------------------------------------
 */
static int
configure_data_port(unifi_priv_t *priv,
                    unifi_PortAction port_action,
                    const unifi_MACAddress *macAddress,
                    const int queue)
{
    const CsrUint8 broadcast_mac_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    unifi_port_config_t *port;

    if (queue == UF_CONTROLLED_PORT_Q) {
        port = &priv->controlled_data_port;
    } else {
        port = &priv->uncontrolled_data_port;
    }

    /*
     * If the new configuration has the broadcast MAC address
     * we invalidate all entries in the list and store the new
     * configuration in index 0.
     */
    if (memcmp(macAddress->data, broadcast_mac_address, ETH_ALEN) == 0) {

        port->port_cfg[0].port_action = port_action;
        port->port_cfg[0].mac_address = *macAddress;
        port->entries_in_use = 1;
        port->overide_action = UF_DATA_PORT_OVERIDE;

        if (port_action == unifi_8021xPortOpen) {
            unifi_trace(priv, UDBG1, "%s port broadcast set to open.\n",
                        (queue == UF_CONTROLLED_PORT_Q) ? "Controlled" : "Uncontrolled");

            /*
             * Ask stack to schedule for transmission any packets queued
             * while controlled port was not open.
             * Use netif_schedule() instead of netif_wake_queue() because
             * transmission should be already enabled at this point. If it
             * is not, probably the interface is down and should remain as is.
             */
            uf_resume_data_plane(priv, queue, *macAddress);
        } else {
            unifi_trace(priv, UDBG1, "%s port broadcast set to %s.\n",
                        (queue == UF_CONTROLLED_PORT_Q) ? "Controlled" : "Uncontrolled",
                        (port_action == unifi_8021xPortClosedDiscard) ? "discard": "closed");

            /* If port is closed, discard all the pending Rx packets */
            if (port_action == unifi_8021xPortClosedDiscard) {
                uf_free_pending_rx_packets(priv, queue, *macAddress);
            }
        }
    } else {
        /*
         * If the new configuration is for Ad-Hoc we clear the overide action,
         * find the first available index and store the new configuration.
         */
        if (port->entries_in_use < UNIFI_MAX_CONNECTIONS) {
            unifi_port_cfg_t *next_port_cfg;

            /* If the broadcast configuration is still set, reset it. */
            if (port->overide_action == UF_DATA_PORT_OVERIDE) {
                port->entries_in_use = 0;
                port->overide_action = UF_DATA_PORT_NOT_OVERIDE;
            }

            /* Use the next available index. */
            next_port_cfg = &port->port_cfg[port->entries_in_use];
            next_port_cfg->port_action = port_action;
            next_port_cfg->mac_address = *macAddress;
            port->entries_in_use ++;

            if (port_action == unifi_8021xPortOpen) {
                /*
                 * Ask stack to schedule for transmission any packets queued
                 * while controlled port was not open.
                 * Use netif_schedule() instead of netif_wake_queue() because
                 * transmission should be already enabled at this point. If it
                 * is not, probably the interface is down and should remain as is.
                 */
                uf_resume_data_plane(priv, queue, *macAddress);
            }

            /*
             * If port is closed, discard all the pending Rx packets
             * coming from the peer station.
             */
            if (port_action == unifi_8021xPortClosedDiscard) {
                uf_free_pending_rx_packets(priv, queue, *macAddress);
            }

            unifi_trace(priv, UDBG1, "add a new port config (%d).\n",
                        port_action);
        } else {
            unifi_error(priv, "controlled_port_cfg is full.\n");
            return -EFAULT;
        }
    }
    return 0;
} /* configure_data_port() */


void unifi_sys_port_configure_req(void* drvpriv,
                                  unifi_PortAction uncontrolledPortAction,
                                  unifi_PortAction controlledPortAction,
                                  const unifi_MACAddress* macAddress)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_port_configure_req: invalid smepriv\n");
        return;
    }

    configure_data_port(priv, uncontrolledPortAction, macAddress, UF_UNCONTROLLED_PORT_Q);
    configure_data_port(priv, controlledPortAction, macAddress, UF_CONTROLLED_PORT_Q);

    unifi_sys_port_configure_cfm(priv->smepriv, unifi_Success, macAddress);
}


void unifi_sys_wifi_on_req(void* drvpriv, void* appHandle)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    unifi_DriverVersions versions;
    int r;
    CsrInt32 csr_r;

    if (priv == NULL) {
        return;
    }

    unifi_trace(priv, UDBG1, "unifi_sys_wifi_on_req\n");

    /*
     * The request to initialise UniFi might come while UniFi is running.
     * We need to block all I/O activity until the reset completes, otherwise
     * an SDIO error might occur resulting an indication to the SME which
     * makes it think that the initialisation has failed.
     */
    priv->bh_thread.block_thread = 1;

    /* Update the wifi_on state */
    priv->wifi_on_state = wifi_on_in_progress;

    r = uf_request_firmware_files(priv, UNIFI_FW_STA);
    if (r) {
        unifi_error(priv, "unifi_sys_wifi_on_req: Failed to get f/w\n");
        unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Error);
        return;
    }

    /* Power on UniFi (which may not necessarily have been off) */
    csr_r = CsrSdioPowerOn(priv->sdio);
    if (csr_r < 0) {
        unifi_error(priv, "unifi_sys_wifi_on_req: Failed to power on UniFi\n");
        unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Error);
        return;
    }

    /* If CsrSdioPowerOn() returns 0, it means that we need to initialise UniFi */
    if (csr_r == CSR_SDIO_RESULT_DEVICE_WAS_RESET && !priv->wol_suspend) {
        /* Initialise UniFi hardware */
        r = uf_init_hw(priv);
        if (r) {
            unifi_error(priv, "unifi_sys_wifi_on_req: Failed to initialise h/w, error %d\n", r);
            unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Error);
            return;
        }
    } else {
        unifi_trace(priv, UDBG1, "UniFi already initialised\n");
    }

    /* Completed handling of wake up from suspend with UniFi powered */
    priv->wol_suspend = FALSE;

    /* Re-enable the I/O thread */
    priv->bh_thread.block_thread = 0;

    /*
     * Start the I/O thread. The thread might be already running.
     * This fine, just carry on with the request.
     */
    r = uf_init_bh(priv);
    if (r) {
        CsrSdioPowerOff(priv->sdio);
        unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Error);
        return;
    }

    /* Get the version information from the core */
    unifi_card_info(priv->card, &priv->card_info);

    /* Copy to the unifiio_card_info structure. */
    versions.chipId = priv->card_info.chip_id;
    versions.chipVersion = priv->card_info.chip_version;
    versions.firmwareBuild = priv->card_info.fw_build;
    versions.firmwareHip = priv->card_info.fw_hip_version;
    versions.driverBuild = UNIFI_DRIVER_BUILD_ID;
    versions.driverHip = (UNIFI_HIP_MAJOR_VERSION << 8) | UNIFI_HIP_MINOR_VERSION;

    unifi_sys_wifi_on_ind(priv->smepriv, unifi_Success, &versions);

    /* Update the wifi_on state */
    priv->wifi_on_state = wifi_on_done;
}


/*
 * wifi_off:
 *      Common code for unifi_sys_wifi_off_req() and
 *      unifi_sys_wifi_off_rsp().
 */
static void
wifi_off(unifi_priv_t *priv)
{
    int power_off;
    int priv_instance;
    CsrInt32 csr_r;

    /* Destroy the Traffic Analysis Module */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
    cancel_work_sync(&priv->ta_ind_work.task);
    cancel_work_sync(&priv->ta_sample_ind_work.task);
#ifdef CSR_SUPPORT_WEXT
    cancel_work_sync(&priv->sme_config_task);
#endif
#endif
    flush_workqueue(priv->unifi_workqueue);

    /* fw_init parameter can prevent power off UniFi, for debugging */
    priv_instance = uf_find_priv(priv);
    if (priv_instance == -1) {
        unifi_warning(priv,
                      "unifi_sys_stop_req: Unknown priv instance, will power off card.\n");
        power_off = 1;
    } else {
        power_off = (fw_init[priv_instance] > 0) ? 0 : 1;
    }

    /* Production test mode requires power to the chip, too */
    if (priv->ptest_mode) {
        power_off = 0;
    }

    /* Stop the bh_thread */
    uf_stop_thread(priv, &priv->bh_thread);

    /* Unregister the interrupt handler */
    if (csr_sdio_linux_remove_irq(priv->sdio)) {
        unifi_notice(priv,
                     "csr_sdio_linux_remove_irq failed to talk to card.\n");
    }

    if (power_off) {
        unifi_trace(priv, UDBG2,
                    "Force low power and try to power off\n");
        /* Put UniFi to deep sleep, in case we can not power it off */
        csr_r = unifi_force_low_power_mode(priv->card);

        CsrSdioPowerOff(priv->sdio);
    }

    /* Consider UniFi to be uninitialised */
    priv->init_progress = UNIFI_INIT_NONE;

} /* wifi_off() */


void unifi_sys_wifi_off_req(void* drvpriv, void* appHandle)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        return;
    }

    unifi_trace(priv, UDBG1, "unifi_sys_wifi_off_req:\n");


    /* Stop the network traffic before freeing the core. */
    if (priv->netdev_registered == 1) {
        netif_carrier_off(priv->netdev);
        UF_NETIF_TX_STOP_ALL_QUEUES(priv->netdev);
    }

    wifi_off(priv);

    unifi_sys_wifi_off_cfm(priv->smepriv);
}


void unifi_sys_qos_control_req(void* drvpriv, unifi_QoSControl control)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_qos_control_req: invalid smepriv\n");
        return;
    }

    unifi_trace(priv, UDBG4,
                "unifi_sys_qos_control_req: control = %d", control);

    if (control == unifi_QoSWMMOn) {
        priv->sta_wmm_capabilities |= QOS_CAPABILITY_WMM_ENABLED;
    } else {
        priv->sta_wmm_capabilities = 0;
    }

    unifi_sys_qos_control_cfm(priv->smepriv, unifi_Success);
}


void unifi_sys_tclas_add_req(void* drvpriv, CsrUint16 tclasLength, const CsrUint8 *tclas)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_tclas_add_req: invalid smepriv\n");
        return;
    }

    unifi_sys_tclas_add_cfm(priv->smepriv, unifi_Success);
}

void unifi_sys_tclas_del_req(void* drvpriv, CsrUint16 tclasLength, const CsrUint8 *tclas)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_tclas_del_req: invalid smepriv\n");
        return;
    }

    unifi_sys_tclas_del_cfm(priv->smepriv, unifi_Success);
}


void unifi_sys_configure_power_mode_req(void* drvpriv, unifi_LowPowerMode mode, CsrBool wakeHost)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    enum unifi_low_power_mode pm;
    CsrInt32 csr_r;

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_hw_check_req: invalid smepriv\n");
        return;
    }

    if (mode == unifi_LowPowerDisabled) {
        pm = UNIFI_LOW_POWER_DISABLED;
    } else {
        pm = UNIFI_LOW_POWER_ENABLED;
    }

    unifi_trace(priv, UDBG2,
                "unifi_sys_configure_power_mode_req (mode=%d, wake=%d)\n",
                mode, wakeHost);
    csr_r = unifi_configure_low_power_mode(priv->card, pm,
                                           (wakeHost ? UNIFI_PERIODIC_WAKE_HOST_ENABLED : UNIFI_PERIODIC_WAKE_HOST_DISABLED));
}


void unifi_sys_wifi_on_rsp(void* drvpriv, unifi_Status status,
                           const unifi_MACAddress *stationMacAddress,
                           const unifi_SmeVersions * versions,
                           CsrBool scheduledInterrupt)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(NULL, "unifi_sys_wifi_on_rsp: Invalid ospriv.\n");
        return;
    }

    unifi_trace(priv, UDBG1,
                "unifi_sys_wifi_on_rsp: status %d (patch %u)\n", status, versions->firmwarePatch);

    /* UniFi is now initialised, complete the init. */
    if (status == unifi_Success)
    {
        CsrUint32 intmode = CSR_WIFI_INTMODE_DEFAULT;
        
        /* Register the UniFi device with the OS network manager */
        unifi_trace(priv, UDBG3, "Card Init Completed Successfully\n");

        /* Store the MAC address in the netdev */
        memcpy(priv->netdev->dev_addr, stationMacAddress->data, ETH_ALEN);

        /* Copy version structure into the private versions field */
        priv->sme_versions = *versions;

        if (!priv->netdev_registered)
        {
            int r;
            unifi_trace(priv, UDBG1, "registering net device\n");
            r = uf_register_netdev(priv);
            if (r) {
                unifi_error(priv, "Failed to register the network device.\n");
                unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Error);
                return;
            }
        }

        /* If the MIB has selected f/w scheduled interrupt mode, apply it now
         * but let module param override.
         */
        if (run_bh_once != -1) {
            intmode = (CsrUint32)run_bh_once;
        } else if (scheduledInterrupt) {
            intmode = CSR_WIFI_INTMODE_RUN_BH_ONCE;
        }
        unifi_set_interrupt_mode(priv->card, intmode);
        
        priv->init_progress = UNIFI_INIT_COMPLETED;

        /* Acknowledge the unifi_sys_wifi_on_req() now */
        unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Success);

        unifi_info(priv, "UniFi ready\n");

        /* Firmware initialisation is complete, so let the SDIO bus
         * clock be raised when convienent to the core.
         */
        unifi_request_max_sdio_clock(priv->card);

#ifdef CSR_SUPPORT_WEXT
        /* Notify the Android wpa_supplicant that we are ready */
        wext_send_started_event(priv);

        queue_work(priv->unifi_workqueue, &priv->sme_config_task);
#endif

    } else {
        /* Acknowledge the unifi_sys_wifi_on_req() now */
        unifi_sys_wifi_on_cfm(priv->smepriv, unifi_Error);
    }
}


void unifi_sys_wifi_off_rsp(void* drvpriv)
{
}


void unifi_sys_multicast_address_rsp(void* drvpriv, unifi_Status status,
                                     unifi_ListAction action,
                                     CsrUint8 getAddressesCount,
                                     const unifi_MACAddress *getAddresses)
{
}


void unifi_sys_ma_unitdata_subscribe_req(void* drvpriv,
                                         void* appHandle,
                                         unifi_Encapsulation encapsulation,
                                         CsrUint16 protocol,
                                         CsrUint32 oui)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    unifi_SubscriptionResult result;
    CsrUint8 i;

    if (priv == NULL) {
        unifi_error(priv,
                    "unifi_sys_ma_unitdata_subscribe_req: invalid priv\n");
        return;
    }

    /* Look for an unused filter */
    result = unifi_SubscriptionResultErrorProtocolTableFull;
    for (i = 0; i < MAX_MA_UNIDATA_IND_FILTERS; i++) {

        if (!priv->sme_unidata_ind_filters[i].in_use) {

            priv->sme_unidata_ind_filters[i].in_use = 1;
            priv->sme_unidata_ind_filters[i].appHandle = appHandle;
            priv->sme_unidata_ind_filters[i].encapsulation = encapsulation;
            priv->sme_unidata_ind_filters[i].protocol = protocol;

            priv->sme_unidata_ind_filters[i].oui[2] = (CsrUint8)  (oui        & 0xFF);
            priv->sme_unidata_ind_filters[i].oui[1] = (CsrUint8) ((oui >>  8) & 0xFF);
            priv->sme_unidata_ind_filters[i].oui[0] = (CsrUint8) ((oui >> 16) & 0xFF);

            result = unifi_SubscriptionResultSuccess;
            break;
        }
    }

    unifi_trace(priv, UDBG1,
                "subscribe_req: encap=%d, handle=%d, result=%d\n",
                encapsulation, i, result);
    unifi_sys_ma_unitdata_subscribe_cfm(priv->smepriv, appHandle,
                                        i, result, 0);
}


void unifi_sys_ma_unitdata_unsubscribe_req(void* drvpriv,
                                           void* appHandle,
                                           CsrUint8 subscriptionHandle)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    unifi_SubscriptionResult result;

    if (priv == NULL) {
        unifi_error(priv,
                    "unifi_sys_ma_unitdata_unsubscribe_req: invalid priv\n");
        return;
    }

    result = unifi_SubscriptionResultErrorUndefinedEntry;

    if (subscriptionHandle < MAX_MA_UNIDATA_IND_FILTERS) {
        if (priv->sme_unidata_ind_filters[subscriptionHandle].in_use) {
            priv->sme_unidata_ind_filters[subscriptionHandle].in_use = 0;
            result = unifi_SubscriptionResultSuccess;
        } else {
            result = unifi_SubscriptionResultErrorCouldNotFindEntry;
        }
    }

    unifi_trace(priv, UDBG1,
                "unsubscribe_req: handle=%d, result=%d\n",
                subscriptionHandle, result);
    unifi_sys_ma_unitdata_unsubscribe_cfm(priv->smepriv, appHandle, result);
}


void unifi_sys_capabilities_req(void* drvpriv, void* appHandle)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_capabilities_req: invalid priv\n");
        return;
    }

    unifi_sys_capabilities_cfm(priv->smepriv, appHandle,
                               UNIFI_SOFT_COMMAND_Q_LENGTH - 1,
                               UNIFI_SOFT_TRAFFIC_Q_LENGTH - 1);
}


void unifi_sys_suspend_rsp(void* drvpriv, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_suspend_rsp: invalid priv\n");
        return;
    }

    sme_complete_request(priv, status);
}


void unifi_sys_resume_rsp(void* drvpriv, unifi_Status status)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_resume_rsp: invalid priv\n");
        return;
    }

    /*
     * Unless we are in ptest mode, nothing is waiting for the response.
     * Do not call sme_complete_request(), otherwise the driver
     * and the SME will be out of step.
     */
    if (priv->ptest_mode == 1) {
        sme_complete_request(priv, status);
    }

}


void unifi_sys_traffic_config_req(void* drvpriv, unifi_TrafficConfigType type, const unifi_TrafficConfig *config)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    CsrInt32 csr_r;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_traffic_config_req: invalid smepriv\n");
        return;
    }

    csr_r = unifi_ta_configure(priv->card, type, config);
}

void unifi_sys_traffic_classification_req(void* drvpriv, unifi_TrafficType traffic_type, CsrUint16 period)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_traffic_classification_req: invalid smepriv\n");
        return;
    }

    unifi_ta_classification(priv->card, traffic_type, period);
}

static int
_sys_packet_req(unifi_priv_t *priv, const CSR_SIGNAL *signal,
                CsrUint8 subscriptionHandle,
                CsrUint16 frameLength, const CsrUint8 *frame,
                int proto, int hold_m4, void* appHandle)
{
    const sme_ma_unidata_ind_filter_t *subs;
    bulk_data_param_t bulkdata;
    int r;
    struct sk_buff *skb;

    if (!priv->sme_unidata_ind_filters[subscriptionHandle].in_use) {
        unifi_error(priv, "_sys_packet_req: unknown subscription.\n");
        return -EINVAL;
    }

    subs = &priv->sme_unidata_ind_filters[subscriptionHandle];
    unifi_trace(priv, UDBG1,
                "_sys_packet_req: handle=%d, subs=%p, encap=%d\n",
                subscriptionHandle, subs, subs->encapsulation);

    r = unifi_net_data_malloc(priv, &bulkdata.d[0], frameLength);
    if (r) {
        unifi_error(priv, "_sys_packet_req: failed to allocate bulkdata.\n");
        return r;
    }

    /* Determine if we need to add encapsulation header */
    if (subs->encapsulation == unifi_Ethernet) {
        memcpy((void*)bulkdata.d[0].os_data_ptr, frame, frameLength);

        /* The translation is performed on the skb */
        skb = (struct sk_buff*)bulkdata.d[0].os_net_buf_ptr;

        unifi_trace(priv, UDBG1,
                    "_sys_packet_req: skb_ether_to_80211 -->\n");
        r = skb_ether_to_80211(priv->netdev, skb, proto);
        unifi_trace(priv, UDBG1,
                    "_sys_packet_req: skb_ether_to_80211 <--\n");
        if (r) {
            unifi_error(priv,
                        "_sys_packet_req: failed to translate eth frame.\n");
            return r;
        }

        bulkdata.d[0].data_length = skb->len;
    } else {
        /* Crop the MAC addresses from the packet */
        memcpy((void*)bulkdata.d[0].os_data_ptr, frame + 2*ETH_ALEN, frameLength - 2*ETH_ALEN);
        bulkdata.d[0].data_length = frameLength - 2*ETH_ALEN;
    }

    bulkdata.d[1].os_data_ptr = NULL;
    bulkdata.d[1].os_net_buf_ptr = NULL;
    bulkdata.d[1].data_length = 0;

    if (hold_m4 && (priv->m4_monitor_state == m4_trap) &&
        (0 == uf_verify_m4(priv, bulkdata.d[0].os_data_ptr, bulkdata.d[0].data_length))) {
        /* Store the EAPOL M4 packet for later */
        priv->m4_signal = *signal;
        priv->m4_bulk_data.net_buf_length = bulkdata.d[0].net_buf_length;
        priv->m4_bulk_data.data_length = bulkdata.d[0].data_length;
        priv->m4_bulk_data.os_data_ptr = bulkdata.d[0].os_data_ptr;
        priv->m4_bulk_data.os_net_buf_ptr = bulkdata.d[0].os_net_buf_ptr;

        priv->m4_monitor_state = m4_trapped;
    } else {

        /* Send the signal to UniFi */
        unifi_trace(priv, UDBG1,
                    "_sys_packet_req: ul_send_signal_unpacked -->\n");
        r = ul_send_signal_unpacked(priv, signal, &bulkdata);
        unifi_trace(priv, UDBG1,
                    "_sys_packet_req: ul_send_signal_unpacked <--\n");
        if (r) {
            unifi_error(priv,
                        "_sys_packet_req: failed to send signal.\n");
            return r;
        }
    }
    /* The final unifi_sys_ma_unitdata_cfm() will called when the actual MA-UNITDATA.cfm is received from the chip */

    return 0;
}

void unifi_sys_ma_unitdata_req(void* drvpriv,
                               void* appHandle,
                               CsrUint8 subscriptionHandle,
                               CsrUint16 frameLength,
                               const CsrUint8 *frame,
                               unifi_FrameFreeFunction freeFunction,
                               unifi_Priority priority,
                               unifi_ServiceClass serviceClass,
                               CsrUint32 reqIdentifier)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    int r;
    struct ethhdr *ehdr;
    CSR_SIGNAL signal;
    CSR_MA_UNITDATA_REQUEST *req = &signal.u.MaUnitdataRequest;
    unifi_PortAction cp_action;


    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_ma_unitdata_req: invalid smepriv\n");
        return;
    }

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_ma_unitdata_req: invalid smepriv\n");
        return;
    }

    /*
     * If the source frame isn't a 802.11, add encapsulation before the
     * frame is sent to UniFi.
     */
    ehdr = (struct ethhdr*)frame;

    /* Controlled port restrictions apply to the packets */
    cp_action = uf_sme_port_state(priv, ehdr->h_dest, UF_CONTROLLED_PORT_Q);
    if (cp_action != unifi_8021xPortOpen) {
        unifi_error(priv, "unifi_sys_ma_unitdata_req: controlled port is closed.\n");
        unifi_sys_ma_unitdata_cfm(priv->smepriv,  (void*)appHandle, unifi_Error,
                                  unifi_TransmissionStatusNoBss,
                                  priority, serviceClass, reqIdentifier);
        return;
    }

    /* Fill in the MA-UNITDATA.req signal */
    memcpy(req->Da.x, ehdr->h_dest, ETH_ALEN);
    memcpy(req->Sa.x, ehdr->h_source, ETH_ALEN);
    req->Priority = priority;
    req->ServiceClass = serviceClass;
    req->RoutingInformation = CSR_NULL_RT;
    req->HostTag = reqIdentifier;

    signal.SignalPrimitiveHeader.SignalId = CSR_MA_UNITDATA_REQUEST_ID;
    /* Store the appHandle in the LSB of the SenderId. */
    COAL_COPY_UINT16_TO_LITTLE_ENDIAN(((priv->sme_cli->sender_id & 0xff00) | (unsigned int)appHandle),
                                      (u8*)&signal.SignalPrimitiveHeader.SenderProcessId);
    signal.SignalPrimitiveHeader.ReceiverProcessId = 0;

    r = _sys_packet_req(priv, &signal, subscriptionHandle,
                        frameLength, frame, ntohs(ehdr->h_proto), 0, appHandle);
    if (r) {
        unifi_sys_ma_unitdata_cfm(priv->smepriv, (void*)appHandle, unifi_Error,
                                  unifi_TransmissionStatusNoBss,
                                  priority, serviceClass, reqIdentifier);
    }

    /* The final unifi_sys_ma_unitdata_cfm() will called when the actual MA-UNITDATA.cfm is received from the chip */
    return;
}

void unifi_sys_ma_unitdata_rsp(void* drvpriv, CsrUint8 subscriptionHandle, unifi_Status result)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_ma_unitdata_rsp: invalid smepriv\n");
        return;
    }
}

void unifi_sys_eapol_req(void* drvpriv,
                         void* appHandle,
                         CsrUint8 subscriptionHandle,
                         CsrUint16 frameLength,
                         const CsrUint8 *frame,
                         unifi_FrameFreeFunction freeFunction)
{
    unifi_priv_t *priv = (unifi_priv_t*)drvpriv;
    int r;
    struct ethhdr *ehdr;
    CSR_SIGNAL signal;
    CSR_MLME_EAPOL_REQUEST *req = &signal.u.MlmeEapolRequest;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_eapol_req: invalid smepriv\n");
        return;
    }

    if (priv->smepriv == NULL) {
        unifi_error(priv, "unifi_sys_eapol_req: invalid smepriv\n");
        return;
    }

    /* If frame isn't 802.11 add headers before copying it to output */
    ehdr = (struct ethhdr*)frame;

    memcpy(req->Da.x, ehdr->h_dest, ETH_ALEN);
    memcpy(req->Sa.x, ehdr->h_source, ETH_ALEN);

    signal.SignalPrimitiveHeader.SignalId = CSR_MLME_EAPOL_REQUEST_ID;
    /* Store the appHandle in the LSB of the SenderId. */
    COAL_COPY_UINT16_TO_LITTLE_ENDIAN(((priv->sme_cli->sender_id & 0xff00) | (unsigned int)appHandle),
                                      (u8*)&signal.SignalPrimitiveHeader.SenderProcessId);
    signal.SignalPrimitiveHeader.ReceiverProcessId = 0;

    r = _sys_packet_req(priv, &signal, subscriptionHandle,
                        frameLength, frame, ntohs(ehdr->h_proto), 1, appHandle);
    if (r) {
        unifi_sys_eapol_cfm(priv->smepriv, (void*)appHandle, unifi_EapolRcFailure);
    }

    return;
}


void unifi_sys_m4_transmit_req(void* context)
{
    unifi_priv_t *priv = (unifi_priv_t*)context;
    int r;
    bulk_data_param_t bulkdata;

    if (priv == NULL) {
        unifi_error(priv, "unifi_sys_m4_transmit_req: invalid smepriv\n");
        return;
    }

    if (priv->m4_monitor_state != m4_trapped)
    {
        /* If m4 is not trapped yet, possibly the setkey/protection is quick */
        priv->m4_monitor_state = m4_wait_eapol_confirm;
        unifi_error(priv, "unifi_sys_m4_transmit_req: m4_monitor_state is incorrect %d\n", priv->m4_monitor_state);
        return;
    }

    if (priv->m4_bulk_data.data_length == 0)
    {
        priv->m4_monitor_state = m4_wait_eapol_confirm;
        unifi_error(priv, "unifi_sys_m4_transmit_req: invalid buffer\n");
        return;
    }

    priv->m4_monitor_state = m4_wait_eapol_confirm;

    memcpy(&bulkdata.d[0], &priv->m4_bulk_data, sizeof(bulk_data_desc_t));

    priv->m4_bulk_data.net_buf_length = 0;
    priv->m4_bulk_data.data_length = 0;
    priv->m4_bulk_data.os_data_ptr = priv->m4_bulk_data.os_net_buf_ptr = NULL;

    bulkdata.d[1].data_length = 0;


    r = ul_send_signal_unpacked(priv, &priv->m4_signal, &bulkdata);
    unifi_trace(priv, UDBG1,
                "unifi_sys_m4_transmit_req: sent\n");
    if (r) {
        unifi_error(priv,
                    "unifi_sys_m4_transmit_req: failed to send signal.\n");
    }
}
