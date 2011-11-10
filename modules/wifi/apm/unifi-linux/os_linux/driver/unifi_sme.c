/*
 * ***************************************************************************
 *  FILE:     unifi_sme.c
 *
 *  PURPOSE:    SME related functions.
 *
 *  Copyright (C) 2007-2009 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */

#include "unifi_priv.h"
#include "driver/unifi.h"
#include "driver/conversions.h"



int
convert_sme_error(enum unifi_Status error)
{
    switch (error) {
      case unifi_Success:
        return 0;
      case unifi_Error:
      case unifi_NotFound:
      case unifi_TimedOut:
      case unifi_Cancelled:
      case unifi_Unavailable:
        return -EIO;
      case unifi_NoRoom:
        return -EBUSY;
      case unifi_InvalidParameter:
        return -EINVAL;
      case unifi_Unsupported:
        return -EOPNOTSUPP;
      default:
        return -EIO;
    }
}


/*
 * ---------------------------------------------------------------------------
 *  sme_log_event
 *
 *      Callback function to be registered as the SME event callback.
 *      Copies the signal content into a new udi_log_t struct and adds
 *      it to the read queue for the SME client.
 *
 *  Arguments:
 *      arg             This is the value given to unifi_add_udi_hook, in
 *                      this case a pointer to the client instance.
 *      signal          Pointer to the received signal.
 *      signal_len      Size of the signal structure in bytes.
 *      bulkdata        Pointers to any associated bulk data.
 *      dir             Direction of the signal. Zero means from host,
 *                      non-zero means to host.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
sme_log_event(ul_client_t *pcli,
              const u8 *signal, int signal_len,
              const bulk_data_param_t *bulkdata,
              int dir)
{
    unifi_priv_t *priv;
    CSR_SIGNAL unpacked_signal;
    unifi_DataBlock mlmeCommand;
    unifi_DataBlock dataref1;
    unifi_DataBlock dataref2;
    int r;

    func_enter();

    /* Just a sanity check */
    if ((signal == NULL) || (signal_len <= 0)) {
        func_exit();
        return;
    }

    priv = uf_find_instance(pcli->instance);
    if (!priv) {
        unifi_error(priv, "sme_log_event: invalid priv\n");
        func_exit();
        return;
    }

    if (priv->smepriv == NULL) {
        unifi_error(priv, "sme_log_event: invalid smepriv\n");
        func_exit();
        return;
    }

    unifi_trace(priv, UDBG3,
                "sme_log_event: Process signal 0x%X %s\n",
                *((CsrUint16*)signal),
                lookup_signal_name(*((CsrUint16*)signal)));


    /*
     * Indicate CSR_MLME_EAPOL_CONFIRM_ID and CSR_MA_UNITDATA_CONFIRM_ID
     * using the respective SYS API.
     */
    r = read_unpack_signal(signal, &unpacked_signal);
    if (r) {
        unifi_error(priv, "sme_log_event: Received unknown or corrupted signal.\n");
        func_exit();
        return;
    }

    if (unpacked_signal.SignalPrimitiveHeader.SignalId == CSR_MLME_EAPOL_CONFIRM_ID) {
        CSR_MLME_EAPOL_CONFIRM *cfm = &unpacked_signal.u.MlmeEapolConfirm;

        /* Retrieve the appHandle from the LSB of the ReceiverId. */
        unifi_sys_eapol_cfm(priv->smepriv, (void*)((unsigned int)signal[2]),
                            (cfm->ResultCode) ? unifi_EapolRcFailure : unifi_EapolRcSuccess);
        func_exit();
        return;
    }

    if (unpacked_signal.SignalPrimitiveHeader.SignalId == CSR_MA_UNITDATA_CONFIRM_ID) {
        CSR_MA_UNITDATA_CONFIRM *cfm = &unpacked_signal.u.MaUnitdataConfirm;

        /* Retrieve the appHandle from the LSB of the ReceiverId. */
        unifi_sys_ma_unitdata_cfm(priv->smepriv,
                                  (void*)((unsigned int)signal[2]),
                                  (cfm->TransmissionStatus) ? unifi_Error : unifi_Success,
                                  cfm->TransmissionStatus,
                                  cfm->ProvidedPriority,
                                  cfm->ProvidedServiceClass,
                                  cfm->ProvidedHostTag);

        func_exit();
        return;
    }

    mlmeCommand.length = signal_len;
    mlmeCommand.data = (CsrUint8*)signal;

    dataref1.length = bulkdata->d[0].data_length;
    if (dataref1.length > 0) {
        dataref1.data = (CsrUint8 *) bulkdata->d[0].os_data_ptr;
    } else
    {
        dataref1.data = NULL;
    }

    dataref2.length = bulkdata->d[1].data_length;
    if (dataref2.length > 0) {
        dataref2.data = (CsrUint8 *) bulkdata->d[1].os_data_ptr;
    } else
    {
        dataref2.data = NULL;
    }

    unifi_sys_hip_ind(priv->smepriv, mlmeCommand.length, mlmeCommand.data,
                                     dataref1.length, dataref1.data,
                                     dataref2.length, dataref2.data);

    func_exit();
} /* sme_log_event() */


/*
 * ---------------------------------------------------------------------------
 * uf_sme_port_state
 *
 *      Return the state of the controlled port.
 *
 * Arguments:
 *      priv            Pointer to device private context struct
 *      address    Pointer to the destination for tx or sender for rx address
 *      queue           Controlled or uncontrolled queue
 *
 * Returns:
 *      An unifi_ControlledPortAction value.
 * ---------------------------------------------------------------------------
 */
unifi_PortAction
uf_sme_port_state(unifi_priv_t *priv, unsigned char *address, int queue)
{
    int i;
    unifi_port_config_t *port;

    if (queue == UF_CONTROLLED_PORT_Q) {
        port = &priv->controlled_data_port;
    } else {
        port = &priv->uncontrolled_data_port;
    }

    if (!port->entries_in_use) {
        unifi_trace(priv, UDBG5, "No port configurations, return Discard.\n");
        return unifi_8021xPortClosedDiscard;
    }
    /* If the port configuration is common for all destinations, return it. */
    if (port->overide_action == UF_DATA_PORT_OVERIDE) {
        unifi_trace(priv, UDBG5, "Single port configuration (%d).\n",
                    port->port_cfg[0].port_action);
        return port->port_cfg[0].port_action;
    }

    unifi_trace(priv, UDBG5, "Multiple port configurations.\n");
    /* If multiple configurations exist.. */
    for (i = 0; i < port->entries_in_use; i++) {
        /* .. go through the list and match the destination address. */
        if (memcmp(address, port->port_cfg[i].mac_address.data, ETH_ALEN) == 0) {
            /* Return the desired action. */
            return port->port_cfg[i].port_action;
        }
    }

    /* Could not find any information, return Open. */
    unifi_trace(priv, UDBG5, "port configuration not found, return Open.\n");
    return unifi_8021xPortOpen;
} /* uf_sme_port_state() */


void
uf_multicast_list_wq(struct work_struct *work)
{
    unifi_priv_t *priv = container_of(work, unifi_priv_t,
                                      multicast_list_task);
    int i;
    unifi_MACAddress* multicast_address_list = NULL;
    int mc_count;
    u8 *mc_list;

    unifi_trace(priv, UDBG5,
                "uf_multicast_list_wq: list count = %d\n",
                priv->mc_list_count);

    /* Flush the current list */
    unifi_sys_multicast_address_ind(priv->smepriv, unifi_ListActionFlush, 0, NULL);

    mc_count = priv->mc_list_count;
    mc_list = priv->mc_list;
    /*
     * Allocate a new list, need to free it later
     * in unifi_mgt_multicast_address_cfm().
     */
    multicast_address_list = CsrPmalloc(mc_count * sizeof(unifi_MACAddress));

    if (multicast_address_list == NULL) {
        return;
    }

    for (i = 0; i < mc_count; i++) {
        memcpy(multicast_address_list[i].data, mc_list, ETH_ALEN);
        mc_list += ETH_ALEN;
    }

    if (priv->smepriv == NULL) {
        CsrPfree(multicast_address_list);
        return;
    }

    unifi_sys_multicast_address_ind(priv->smepriv,
                                    unifi_ListActionAdd,
                                    mc_count, multicast_address_list);

    /* The SME will take a copy of the addreses*/
    CsrPfree(multicast_address_list);
}


#ifdef CSR_SUPPORT_WEXT
int unifi_cfg_power(unifi_priv_t *priv, unsigned char *arg)
{
    unifi_cfg_power_t cfg_power;
    int rc;

    if (get_user(cfg_power, (unifi_cfg_power_t*)(((unifi_cfg_command_t*)arg) + 1))) {
        unifi_error(priv, "UNIFI_CFG: Failed to get the argument\n");
        return -EFAULT;
    }

    switch (cfg_power) {
      case UNIFI_CFG_POWER_OFF:
        rc = sme_sys_suspend(priv);
        if (rc) {
            return rc;
        }
        break;
      case UNIFI_CFG_POWER_ON:
        rc = sme_sys_resume(priv);
        if (rc) {
            return rc;
        }
        break;
      default:
        unifi_error(priv, "WIFI POWER: Unknown value.\n");
        return -EINVAL;
    }

    return 0;
}


int unifi_cfg_power_save(unifi_priv_t *priv, unsigned char *arg)
{
    unifi_cfg_powersave_t cfg_power_save;
    unifi_AppValue sme_app_value;
    int rc;

    if (get_user(cfg_power_save, (unifi_cfg_powersave_t*)(((unifi_cfg_command_t*)arg) + 1))) {
        unifi_error(priv, "UNIFI_CFG: Failed to get the argument\n");
        return -EFAULT;
    }

    /* Get the coex info from the SME */
    sme_app_value.id = unifi_PowerConfigValue;
    rc = sme_mgt_get_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Get unifi_PowerConfigValue failed.\n");
        return rc;
    }

    switch (cfg_power_save) {
      case UNIFI_CFG_POWERSAVE_NONE:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveLow;
        break;
      case UNIFI_CFG_POWERSAVE_FAST:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveMed;
        break;
      case UNIFI_CFG_POWERSAVE_FULL:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveHigh;
        break;
      case UNIFI_CFG_POWERSAVE_AUTO:
        sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel = unifi_PowerSaveAuto;
        break;
      default:
        unifi_error(priv, "POWERSAVE: Unknown value.\n");
        return -EINVAL;
    }

    sme_app_value.id = unifi_PowerConfigValue;
    rc = sme_mgt_set_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Set unifi_PowerConfigValue failed.\n");
    }

    return rc;
}


int unifi_cfg_power_supply(unifi_priv_t *priv, unsigned char *arg)
{
    unifi_cfg_powersupply_t cfg_power_supply;
    unifi_AppValue sme_app_value;
    int rc;

    if (get_user(cfg_power_supply, (unifi_cfg_powersupply_t*)(((unifi_cfg_command_t*)arg) + 1))) {
        unifi_error(priv, "UNIFI_CFG: Failed to get the argument\n");
        return -EFAULT;
    }

    /* Get the coex info from the SME */
    sme_app_value.id = unifi_HostConfigValue;
    rc = sme_mgt_get_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Get unifi_HostConfigValue failed.\n");
        return rc;
    }

    switch (cfg_power_supply) {
      case UNIFI_CFG_POWERSUPPLY_MAINS:
        sme_app_value.unifi_Value_union.hostConfig.powerMode = unifi_HostActive;
        break;
      case UNIFI_CFG_POWERSUPPLY_BATTERIES:
        sme_app_value.unifi_Value_union.hostConfig.powerMode = unifi_HostPowersave;
        break;
      default:
        unifi_error(priv, "POWERSUPPLY: Unknown value.\n");
        return -EINVAL;
    }

    sme_app_value.id = unifi_HostConfigValue;
    rc = sme_mgt_set_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Set unifi_HostConfigValue failed.\n");
    }

    return rc;
}


int unifi_cfg_packet_filters(unifi_priv_t *priv, unsigned char *arg)
{
    unsigned char *tclas_buffer;
    unsigned int tclas_buffer_length;
    tclas_t *dhcp_tclas;
    int rc;

    /* Free any TCLASs previously allocated */
    if (priv->packet_filters.tclas_ies_length) {
        CsrPfree(priv->filter_tclas_ies);
        priv->filter_tclas_ies = NULL;
    }

    tclas_buffer = ((unsigned char*)arg) + sizeof(unifi_cfg_command_t) + sizeof(unsigned int);
    if (copy_from_user(&priv->packet_filters, (void*)tclas_buffer,
                        sizeof(uf_cfg_bcast_packet_filter_t))) {
        unifi_error(priv, "UNIFI_CFG: Failed to get the filter struct\n");
        return -EFAULT;
    }

    tclas_buffer_length = priv->packet_filters.tclas_ies_length;

    /* Allocate TCLASs if necessary */
    if (priv->packet_filters.dhcp_filter) {
        priv->packet_filters.tclas_ies_length += sizeof(tclas_t);
    }
    if (priv->packet_filters.tclas_ies_length > 0) {
        priv->filter_tclas_ies = CsrPmalloc(priv->packet_filters.tclas_ies_length);
        if (priv->filter_tclas_ies == NULL) {
            return -ENOMEM;
        }
        if (tclas_buffer_length) {
            tclas_buffer += sizeof(uf_cfg_bcast_packet_filter_t) - sizeof(unsigned char*);
            if (copy_from_user(priv->filter_tclas_ies,
                            tclas_buffer,
                            tclas_buffer_length)) {
                unifi_error(priv, "UNIFI_CFG: Failed to get the TCLAS buffer\n");
                return -EFAULT;
            }
        }
    }

    if(priv->packet_filters.dhcp_filter)
    {
        /* Append the DHCP tclas IE */
        dhcp_tclas = (tclas_t*)(priv->filter_tclas_ies + tclas_buffer_length);
        memset(dhcp_tclas, 0, sizeof(tclas_t));
        dhcp_tclas->element_id = 14;
        dhcp_tclas->length = sizeof(tcpip_clsfr_t) + 1;
        dhcp_tclas->user_priority = 0;
        dhcp_tclas->tcp_ip_cls_fr.cls_fr_type = 1;
        dhcp_tclas->tcp_ip_cls_fr.version = 4;
        ((CsrUint8*)(&dhcp_tclas->tcp_ip_cls_fr.source_port))[0] = 0x00;
        ((CsrUint8*)(&dhcp_tclas->tcp_ip_cls_fr.source_port))[1] = 0x44;
        ((CsrUint8*)(&dhcp_tclas->tcp_ip_cls_fr.dest_port))[0] = 0x00;
        ((CsrUint8*)(&dhcp_tclas->tcp_ip_cls_fr.dest_port))[1] = 0x43;
        dhcp_tclas->tcp_ip_cls_fr.protocol = 0x11;
        dhcp_tclas->tcp_ip_cls_fr.cls_fr_mask = 0x58; //bits: 3,4,6
    }

    rc = sme_mgt_packet_filter_set(priv);

    return rc;
}


int unifi_cfg_wmm_qos_info(unifi_priv_t *priv, unsigned char *arg)
{
    CsrUint8 wmm_qos_info;
    int rc = 0;

    if (get_user(wmm_qos_info, (CsrUint8*)(((unifi_cfg_command_t*)arg) + 1))) {
        unifi_error(priv, "UNIFI_CFG: Failed to get the argument\n");
        return -EFAULT;
    }

    /* Store the value in the connection info */
    priv->connection_config.wmmQosInfo = wmm_qos_info;

    return rc;
}


int unifi_cfg_wmm_addts(unifi_priv_t *priv, unsigned char *arg)
{
    CsrUint32 addts_tid;
    CsrUint8 addts_ie_length;
    CsrUint8 *addts_ie;
    CsrUint8 *addts_params;
    unifi_DataBlock tspec;
    unifi_DataBlock tclas;
    int rc;

    addts_params = (CsrUint8*)(((unifi_cfg_command_t*)arg) + 1);
    if (get_user(addts_tid, (CsrUint32*)addts_params)) {
        unifi_error(priv, "unifi_cfg_wmm_addts: Failed to get the argument\n");
        return -EFAULT;
    }

    addts_params += sizeof(CsrUint32);
    if (get_user(addts_ie_length, (CsrUint8*)addts_params)) {
        unifi_error(priv, "unifi_cfg_wmm_addts: Failed to get the argument\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG4, "addts: tid = 0x%x ie_length = %d\n",
                addts_tid, addts_ie_length);

    addts_ie = CsrPmalloc(addts_ie_length);
    if (addts_ie == NULL) {
        unifi_error(priv,
                    "unifi_cfg_wmm_addts: Failed to malloc %d bytes for addts_ie buffer\n",
                    addts_ie_length);
        return -ENOMEM;
    }

    addts_params += sizeof(CsrUint8);
    rc = copy_from_user(addts_ie, addts_params, addts_ie_length);
    if (rc) {
        unifi_error(priv, "unifi_cfg_wmm_addts: Failed to get the addts buffer\n");
        CsrPfree(addts_ie);
        return -EFAULT;
    }

    tspec.data = addts_ie;
    tspec.length = addts_ie_length;
    tclas.data = NULL;
    tclas.length = 0;

    rc = sme_mgt_tspec(priv, unifi_ListActionAdd, addts_tid,
                       &tspec, &tclas);

    CsrPfree(addts_ie);
    return rc;
}


int unifi_cfg_wmm_delts(unifi_priv_t *priv, unsigned char *arg)
{
    CsrUint32 delts_tid;
    CsrUint8 *delts_params;
    unifi_DataBlock tspec;
    unifi_DataBlock tclas;
    int rc;

    delts_params = (CsrUint8*)(((unifi_cfg_command_t*)arg) + 1);
    if (get_user(delts_tid, (CsrUint32*)delts_params)) {
        unifi_error(priv, "unifi_cfg_wmm_delts: Failed to get the argument\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG4, "delts: tid = 0x%x\n", delts_tid);

    tspec.data = tclas.data = NULL;
    tspec.length = tclas.length = 0;

    rc = sme_mgt_tspec(priv, unifi_ListActionRemove, delts_tid,
                       &tspec, &tclas);

    return rc;
}


int unifi_cfg_strict_draft_n(unifi_priv_t *priv, unsigned char *arg)
{
    unifi_AppValue  sme_app_value;
    CsrBool         strict_draft_n;
    CsrUint8       *strict_draft_n_params;
    int             rc;

    strict_draft_n_params = (CsrUint8*)(((unifi_cfg_command_t*)arg) + 1);
    if (get_user(strict_draft_n, (CsrBool*)strict_draft_n_params))
    {
        unifi_error(priv, "unifi_cfg_strict_draft_n: Failed to get the argument\n");
        return -EFAULT;
    }

     unifi_trace(priv, UDBG4, "strict_draft_n: = %s\n", ((strict_draft_n) ? "yes":"no"));

    sme_app_value.id = unifi_SmeConfigValue;
    rc = sme_mgt_get_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Get unifi_SmeConfigValue failed.\n");
        return rc;
    }

    sme_app_value.unifi_Value_union.smeConfig.enableStrictDraftN = strict_draft_n;
    sme_app_value.id = unifi_SmeConfigValue;

    rc = sme_mgt_set_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Set unifi_SmeConfigValue failed.\n");
        return rc;
    }

    return rc;
}

int unifi_cfg_enable_okc(unifi_priv_t *priv, unsigned char *arg)
{
    unifi_AppValue sme_app_value;
    CsrBool        enable_okc;
    CsrUint8      *enable_okc_params;
    int            rc;

    enable_okc_params = (CsrUint8 *)(((unifi_cfg_command_t *)arg) + 1);
    if (get_user(enable_okc, (CsrBool*)enable_okc_params)) {
        unifi_error(priv, "unifi_cfg_enable_okc: Failed to get the argument\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG4, "enable_okc: %s\n", ((enable_okc) ? "yes":"no"));

    sme_app_value.id = unifi_SmeConfigValue;
    rc = sme_mgt_get_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Get unifi_SmeConfigValue failed.\n");
        return rc;
    }

    sme_app_value.unifi_Value_union.smeConfig.enableOpportunisticKeyCaching = enable_okc;
    sme_app_value.id = unifi_SmeConfigValue;

    rc = sme_mgt_set_value(priv, &sme_app_value);
    if (rc) {
        unifi_error(priv, "UNIFI_CFG: Set unifi_SmeConfigValue failed.\n");
        return rc;
    }

    return rc;
}


int unifi_cfg_get_info(unifi_priv_t *priv, unsigned char *arg)
{
    unifi_AppValue sme_app_value;
    unifi_cfg_get_t get_cmd;
    char inst_name[IFNAMSIZ];
    int rc;

    if (get_user(get_cmd, (unifi_cfg_get_t*)(((unifi_cfg_command_t*)arg) + 1))) {
        unifi_error(priv, "UNIFI_CFG: Failed to get the argument\n");
        return -EFAULT;
    }

    switch (get_cmd) {
      case UNIFI_CFG_GET_COEX:
        /* Get the coex info from the SME */
        sme_app_value.id = unifi_CoexInfoValue;
        rc = sme_mgt_get_value(priv, &sme_app_value);
        if (rc) {
            unifi_error(priv, "UNIFI_CFG: Get unifi_CoexInfoValue failed.\n");
            return rc;
        }

        /* Copy the info to the out buffer */
        if (copy_to_user((void*)arg,
                        &sme_app_value.unifi_Value_union.coexInfo,
                        sizeof(unifi_CoexInfo))) {
            unifi_error(priv, "UNIFI_CFG: Failed to copy the coex info\n");
            return -EFAULT;
        }
        break;
      case UNIFI_CFG_GET_POWER_MODE:
        sme_app_value.id = unifi_PowerConfigValue;
        rc = sme_mgt_get_value(priv, &sme_app_value);
        if (rc) {
            unifi_error(priv, "UNIFI_CFG: Get unifi_PowerConfigValue failed.\n");
            return rc;
        }

        /* Copy the info to the out buffer */
        if (copy_to_user((void*)arg,
                        &sme_app_value.unifi_Value_union.powerConfig.powerSaveLevel,
                        sizeof(unifi_PowerSaveLevel))) {
            unifi_error(priv, "UNIFI_CFG: Failed to copy the power save info\n");
            return -EFAULT;
        }
        break;
      case UNIFI_CFG_GET_POWER_SUPPLY:
        sme_app_value.id = unifi_HostConfigValue;
        rc = sme_mgt_get_value(priv, &sme_app_value);
        if (rc) {
            unifi_error(priv, "UNIFI_CFG: Get unifi_HostConfigValue failed.\n");
            return rc;
        }

        /* Copy the info to the out buffer */
        if (copy_to_user((void*)arg,
                        &sme_app_value.unifi_Value_union.hostConfig.powerMode,
                        sizeof(unifi_HostPowerMode))) {
            unifi_error(priv, "UNIFI_CFG: Failed to copy the host power mode\n");
            return -EFAULT;
        }
        break;
      case UNIFI_CFG_GET_VERSIONS:
        break;
      case UNIFI_CFG_GET_INSTANCE:
        uf_net_get_name(priv->netdev, &inst_name[0], sizeof(inst_name));

        /* Copy the info to the out buffer */
        if (copy_to_user((void*)arg,
                         &inst_name[0],
                         sizeof(inst_name))) {
            unifi_error(priv, "UNIFI_CFG: Failed to copy the instance name\n");
            return -EFAULT;
        }
        break;        
      default:
        unifi_error(priv, "unifi_cfg_get_info: Unknown value.\n");
        return -EINVAL;
    }

    return 0;
}


void
uf_sme_config_wq(struct work_struct *work)
{
    unifi_priv_t *priv = container_of(work, unifi_priv_t, sme_config_task);
    unifi_AppValue sme_app_value;

    /* Register to receive indications from the SME */
    unifi_mgt_event_mask_set_req(priv->smepriv, NULL,
                                 unifi_IndWifiOff | unifi_IndConnectionQuality |
                                 unifi_IndMediaStatus | unifi_IndMicFailure);

    sme_app_value.id = unifi_SmeConfigValue;
    if (sme_mgt_get_value(priv, &sme_app_value)) {
        unifi_warning(priv, "uf_sme_config_wq: Get unifi_SMEConfigValue failed.\n");
        return;
    }

    if (priv->if_index == CSR_INDEX_5G) {
        sme_app_value.unifi_Value_union.smeConfig.ifIndex = unifi_GHZ_5_0;
    } else {
        sme_app_value.unifi_Value_union.smeConfig.ifIndex = unifi_GHZ_2_4;
    }

    sme_app_value.unifi_Value_union.smeConfig.trustLevel = (unifi_80211dTrustLevel)tl_80211d;
    if (sme_mgt_set_value(priv, &sme_app_value)) {
        unifi_warning(priv,
                      "SME config for 802.11d Trust Level and Radio Band failed.\n");
        return;
    }

} /* uf_sme_config_wq() */

#endif /* CSR_SUPPORT_WEXT */


/*
 * ---------------------------------------------------------------------------
 *  uf_ta_ind_wq
 *
 *      Deferred work queue function to send Traffic Analysis protocols
 *      indications to the SME.
 *      These are done in a deferred work queue for two reasons:
 *       - the unifi_sys_.._ind() functions are not safe for atomic context
 *       - we want to load the main driver data path as lightly as possible
 *
 *      The TA classifications already come from a workqueue.
 *
 *  Arguments:
 *      work    Pointer to work queue item.
 *
 *  Returns:
 *      None.
 * ---------------------------------------------------------------------------
 */
void
uf_ta_ind_wq(struct work_struct *work)
{
    struct ta_ind *ind = container_of(work, struct ta_ind, task);
    unifi_priv_t *priv = container_of(ind, unifi_priv_t, ta_ind_work);


    unifi_sys_traffic_protocol_ind(priv->smepriv,
                                   ind->packet_type,
                                   ind->direction,
                                   &ind->src_addr);
    ind->in_use = 0;

} /* uf_ta_ind_wq() */

void
uf_ta_sample_ind_wq(struct work_struct *work)
{
    struct ta_sample_ind *ind = container_of(work, struct ta_sample_ind, task);
    unifi_priv_t *priv = container_of(ind, unifi_priv_t, ta_sample_ind_work);


    unifi_sys_traffic_sample_ind(priv->smepriv, &ind->stats);

    ind->in_use = 0;

} /* uf_ta_sample_ind_wq() */

