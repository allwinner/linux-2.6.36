
#include "hcidump_to_sme.h"

char traceMacAddressBuffer[19];
const char *trace_unifi_MACAddress(unifi_MACAddress addr, char* macaddressBuffer)
{
    char     tmp = 0;
    CsrUint8    L1 = 0;

    for (L1 = 0; L1 < 17; L1++)
    {
        switch (L1 % 3)
        {
        case 0:
            tmp = ((addr.data[L1/3] & 0xF0) >> 4) + '0';
            break;
        case 1:
            tmp = (addr.data[L1/3] & 0x0F) + '0';
            break;
        case 2:
            macaddressBuffer[L1] = ':';
            continue;
        default:
            break;
        }
        macaddressBuffer[L1]=(tmp > '9')?(((tmp - '0') - 10) + 'A'):tmp;
    }
    macaddressBuffer[L1] = 0;
    return macaddressBuffer;
}



static int bt_hci_dev = 0;

static CsrBool btConnected = FALSE;
static CsrBool smeRunning = FALSE;

#define MAX_ACL_LINKS 1
#define MAX_SCO_LINKS 1
typedef struct ScoData {
    CsrBool inUse;
    CsrUint16 scoHandle;
    CsrUint16 periodSlots;
    CsrUint16 durationMinSlots;
    CsrUint16 durationMaxSlots;
} ScoData;

typedef struct ConnectionData {
    CsrBool inUse;
    unifi_MACAddress address;
    CsrUint16 handle;
    unifi_BtDeviceRole role;
    unifi_BtDeviceMode mode;
    CsrUint16 logicalChannelTypeMask;
    CsrUint8 numberOfGuaranteedLogicalChannels;

    int numScoLinks;
    ScoData scoData[MAX_SCO_LINKS];

} ConnectionData;

static int numConnections = 0;
static ConnectionData connectionData[MAX_ACL_LINKS];

static ConnectionData* findConnectionDataFromAddress(unifi_MACAddress address)
{
    int i;
    for (i = 0; i < MAX_ACL_LINKS; i++)
    {
        if (connectionData[i].inUse &&
            CsrMemCmp(connectionData[i].address.data, address.data, 6) == 0)
        {
            return &connectionData[i];
        }
    }
    return NULL;
}

static ConnectionData* findConnectionDataFromHandle(CsrUint16 handle)
{
    int i;
    for (i = 0; i < MAX_ACL_LINKS; i++)
    {
        if (connectionData[i].inUse && connectionData[i].handle == handle)
        {
            return &connectionData[i];
        }
    }
    return NULL;
}

static ScoData* findScoData(CsrUint16 scoHandle, ConnectionData** connection)
{
    int i, j;
    for (i = 0; i < MAX_ACL_LINKS; i++)
    {
        if (connectionData[i].inUse)
        {
            for (j = 0; j < MAX_SCO_LINKS; j++)
            {
                if (connectionData[i].scoData[j].inUse && connectionData[i].scoData[j].scoHandle == scoHandle)
                {
                    if (connection != NULL)
                    {
                        *connection = &connectionData[i];
                    }
                    return &connectionData[i].scoData[j];
                }
            }
        }
    }
    return NULL;
}

static ConnectionData* addConnectionData(unifi_MACAddress address, CsrUint16 handle, unifi_BtDeviceRole role, unifi_BtDeviceMode mode)
{
    int i;
    for (i = 0; i < MAX_ACL_LINKS; i++)
    {
        if (!connectionData[i].inUse)
        {
            numConnections++;
            connectionData[i].inUse = TRUE;
            connectionData[i].address = address;
            connectionData[i].handle = handle;
            connectionData[i].role = role;
            connectionData[i].mode = mode;
            connectionData[i].logicalChannelTypeMask = unifi_BtDeviceLogicalChannelNone;
            connectionData[i].numberOfGuaranteedLogicalChannels = 0;

            connectionData[i].numScoLinks = 0;
            CsrMemSet(connectionData[i].scoData, 0x00, sizeof(connectionData[i].scoData));

            if (smeRunning)
            {
                printf("unifi_bt_acl_start_ind(address:%s, role:%d, mode:%d)\n", trace_unifi_MACAddress(address, traceMacAddressBuffer), role, mode);
                unifi_bt_acl_start_ind(NULL, &connectionData[i].address, connectionData[i].role, connectionData[i].mode);
            }
            return &connectionData[i];
        }
    }

    printf("addConnectionData() Cannot support more connections\n");
    return NULL;
}

static ConnectionData* addScoData(unifi_MACAddress address, CsrUint16 scoHandle, CsrUint16 periodSlots, CsrUint16 durationMinSlots, CsrUint16 durationMaxSlots)
{
    int i;
    ConnectionData* connection = findConnectionDataFromAddress(address);

    if (connection == NULL)
    {
        printf("addScoData() Cannot find connection for address %s\n", trace_unifi_MACAddress(address, traceMacAddressBuffer));
        return NULL;
    }

    for (i = 0; i < MAX_SCO_LINKS; i++)
    {
        if (!connection->scoData[i].inUse)
        {
            connection->numScoLinks++;
            connection->scoData[i].inUse = TRUE;
            connection->scoData[i].scoHandle = scoHandle;
            connection->scoData[i].periodSlots = periodSlots;
            connection->scoData[i].durationMinSlots = durationMinSlots;
            connection->scoData[i].durationMaxSlots = durationMaxSlots;

            if (smeRunning)
            {
                printf("unifi_bt_sco_start_ind(address:%s, scoHandle:0x%.4X, periodSlots:%d, durationMinSlots:%d, durationMaxSlots:%d)\n",
                                            trace_unifi_MACAddress(address, traceMacAddressBuffer),
                                            scoHandle,
                                            periodSlots,
                                            durationMinSlots,
                                            durationMaxSlots);

                unifi_bt_sco_start_ind(NULL, &address, scoHandle, periodSlots, durationMinSlots, durationMaxSlots);
            }
            return connection;
        }
    }

    printf("addScoData() Cannot support more sco connections\n");
    return NULL;
}

static void removeScoData(CsrUint16 scoHandle)
{
    ConnectionData* connection;
    ScoData* scoData = findScoData(scoHandle, &connection);
    if (scoData == NULL)
    {
        printf("removeScoData() Cannot find connection for handle 0x%.4X    \n", scoHandle);
        return;
    }

    connection->numScoLinks--;
    scoData->inUse = FALSE;
    if (smeRunning)
    {
        printf("unifi_bt_sco_stop_ind(address:%s, handle:0x%.4X)\n", trace_unifi_MACAddress(connection->address, traceMacAddressBuffer), scoHandle);
        unifi_bt_sco_stop_ind(NULL, &connection->address, scoHandle);
    }
}

static void removeConnectionData(CsrUint16 handle)
{
    int i;
    ConnectionData* connection = findConnectionDataFromHandle(handle);

    if (connection == NULL)
    {
        printf("removeConnectionData() Cannot find connection for handle 0x%.4X    \n", handle);
        return;
    }

    /* Remove ALL sco links */
    for (i = 0; i < MAX_SCO_LINKS; i++)
    {
        if (connection->scoData[i].inUse)
        {
            removeScoData(connection->scoData[i].scoHandle);
        }
    }

    numConnections--;
    connection->inUse = FALSE;
    if (smeRunning)
    {
        printf("unifi_bt_acl_stop_ind(address:%s)\n", trace_unifi_MACAddress(connection->address, traceMacAddressBuffer));
        unifi_bt_acl_stop_ind(NULL, &connection->address);
    }
}

static void updateConnectionData(CsrUint16 handle, CsrUint16 logicalChannelTypeMask, CsrUint8 numberOfGuaranteedLogicalChannels)
{
    if (smeRunning)
    {
        ConnectionData* connection = findConnectionDataFromHandle(handle);

        if (connection == NULL)
        {
            printf("updateConnectionData() Cannot find connection for handle 0x%.4X    \n", handle);
            return;
        }

        connection->logicalChannelTypeMask = logicalChannelTypeMask;
        connection->numberOfGuaranteedLogicalChannels = numberOfGuaranteedLogicalChannels;

        printf("unifi_bt_acl_channel_types_ind(address:%s,logicalChannelTypeMask:%d, numberOfGuaranteedLogicalChannels:%d)\n",
                                    trace_unifi_MACAddress(connection->address, traceMacAddressBuffer),
                                    logicalChannelTypeMask,
                                    numberOfGuaranteedLogicalChannels);

        unifi_bt_acl_channel_types_ind(NULL,
                                       &connection->address,
                                       logicalChannelTypeMask,
                                       numberOfGuaranteedLogicalChannels);
    }
}

//if (smeRunning)
//{
//    printf("unifi_bt_acl_change_ind(address:%s, role:%d, mode:%d)\n",
//                                trace_unifi_MACAddress(newConnectionData.address, traceMacAddressBuffer),
//                                newConnectionData.role,
//                                newConnectionData.mode);
//    unifi_bt_acl_change_ind(NULL, &newConnectionData.address, newConnectionData.role, newConnectionData.mode);
//}

//if (smeRunning)
//{
//    printf("unifi_bt_acl_channel_types_ind(address:%s,logicalChannelTypeMask:%d, numberOfGuaranteedLogicalChannels:%d)\n",
//                                trace_unifi_MACAddress(newConnectionData.address, traceMacAddressBuffer),
//                                newConnectionData.logicalChannelTypeMask,
//                                newConnectionData.numberOfGuaranteedLogicalChannels);
//
//    unifi_bt_acl_channel_types_ind(NULL,
//                                   &newConnectionData.address,
//                                   newConnectionData.logicalChannelTypeMask,
//                                   newConnectionData.numberOfGuaranteedLogicalChannels);
//}

//if (smeRunning)
//{
//    printf("unifi_bt_sco_change_ind(address:%s, scohandle:0x%.4X, periodSlots:%d, durationMinSlots:%d, durationMaxSlots:%d)\n",
//                                    trace_unifi_MACAddress(newScoData.address, traceMacAddressBuffer),
//                                    newScoData.scoHandle,
//                                    newScoData.periodSlots,
//                                    newScoData.durationMinSlots,
//                                    newScoData.durationMaxSlots);
//
//    unifi_bt_sco_change_ind(NULL,
//                            &newScoData.address,
//                            newScoData.scoHandle,
//                            newScoData.periodSlots,
//                            newScoData.durationMinSlots,
//                            newScoData.durationMaxSlots);
//}


void sniff_connect_request(evt_conn_request* evt)
{
    char addr[18];

    // dev_class[3] - ?? this could be useful for UI icons, etc...
    // link_type - 00:SCO, 01:ACL, 02:eSCO
    ba2str(&evt->bdaddr, addr);
    printf("sniff_connect_request(): bdaddr %s link_type %d\n\n",
                                addr, evt->link_type);

}

void sniff_acl_connect(create_conn_cp* evt)
{
    unifi_MACAddress address = {{evt->bdaddr.b[0], evt->bdaddr.b[1], evt->bdaddr.b[2], evt->bdaddr.b[3], evt->bdaddr.b[4], evt->bdaddr.b[5]}};

    /* no real useful information here from standard bluez.  perhaps
     * with android or a modified bluez stack */
    printf("sniff_acl_connect(): %s role_switch %d role (master) mode (active) LCTM 0 nGLC 0\n\n", trace_unifi_MACAddress(address, traceMacAddressBuffer), evt->role_switch);

    CsrUint16 handle = 0; /* Fill in on the connect complete */
    unifi_BtDeviceRole role = unifi_BtDeviceMaster;
    unifi_BtDeviceMode mode = unifi_BtDeviceActive;
    addConnectionData(address, handle, role, mode);


}

void sniff_acl_connect_complete(evt_conn_complete* evt)
{
    unifi_MACAddress address = {{evt->bdaddr.b[0], evt->bdaddr.b[1], evt->bdaddr.b[2], evt->bdaddr.b[3], evt->bdaddr.b[4], evt->bdaddr.b[5]}};
    ConnectionData* connection = findConnectionDataFromAddress(address);

    printf("sniff_acl_connect_complete(): status %d handle 0x%.4X link_type %d encr_mode %d %s\n",
                                evt->status, btohs(evt->handle), evt->link_type, evt->encr_mode, trace_unifi_MACAddress(address, traceMacAddressBuffer));

    /* We are the master */
    if (connection)
    {
        connection->handle = btohs(evt->handle);
        if (evt->status != 0)
        {
            removeConnectionData(btohs(evt->handle));
        }
        return;
    }

    /* We are the slave */
    if (evt->status == 0)
    {
        CsrUint16 handle = btohs(evt->handle);
        unifi_BtDeviceRole role = unifi_BtDeviceSlave;
        unifi_BtDeviceMode mode = unifi_BtDeviceActive;
        addConnectionData(address, handle, role, mode);
    }
}

void sniff_sco_connect(setup_sync_conn_cp* cp)
{
    printf("sniff_sco_connect(): handle 0x%.4X max_latency %d retrans_effort %d\n",
            btohs(cp->handle), btohs(cp->max_latency), cp->retrans_effort);
}

void sniff_sco_sync_connect_complete(evt_sync_conn_complete* evt)
{
    unifi_MACAddress address = {{evt->bdaddr.b[0], evt->bdaddr.b[1], evt->bdaddr.b[2], evt->bdaddr.b[3], evt->bdaddr.b[4], evt->bdaddr.b[5]}};
    printf("sniff_sco_sync_connect_complete(): status %d %s, scoHandle 0x%.4X link_type %d  trans_interval %02x retrans_window %02x rx_pkt_len %04x tx_pkt_len %04x air_mode %02x\n",
                        evt->status,
                        trace_unifi_MACAddress(address, traceMacAddressBuffer),
                        btohs(btohs(evt->handle)),
                        evt->link_type,
                        evt->trans_interval,
                        evt->retrans_window,
                        btohs(btohs(evt->rx_pkt_len)),
                        btohs(btohs(evt->tx_pkt_len)),
                        evt->air_mode);

    if (evt->status == 0)
    {
        CsrUint16 scoHandle = btohs(btohs(evt->handle));
        CsrUint16 periodSlots = evt->trans_interval;
        CsrUint16 durationMinSlots = 2;
        CsrUint16 durationMaxSlots = 2 + evt->retrans_window;

        // Special for sco Values not filled in the evt
        if (evt->link_type == 0)
        {
            periodSlots = 6;
            durationMinSlots = 2;
            durationMaxSlots = 2;
        }

        addScoData(address, scoHandle, periodSlots, durationMinSlots, durationMaxSlots);
    }
}

void sniff_sco_connect_complete(evt_conn_complete* evt)
{
    unifi_MACAddress address = {{evt->bdaddr.b[0], evt->bdaddr.b[1], evt->bdaddr.b[2], evt->bdaddr.b[3], evt->bdaddr.b[4], evt->bdaddr.b[5]}};
    printf("sniff_sco_connect_complete(): status %d %s, scoHandle 0x%.4X\n", evt->status, trace_unifi_MACAddress(address, traceMacAddressBuffer), btohs(evt->handle));

    if (evt->status == 0)
    {
        CsrUint16 scoHandle = btohs(evt->handle);
        CsrUint16 periodSlots = 6;
        CsrUint16 durationMinSlots = 2;
        CsrUint16 durationMaxSlots = 2;
        addScoData(address, scoHandle, periodSlots, durationMinSlots, durationMaxSlots);
    }
}

void sniff_disconnect_complete(evt_disconn_complete* evt)
{
    // status - 00:success (disconnected), other:error BT core spec vol2, part d
    // handle - connection handle, status
    // reason - 00:success, other:error BT core spec vol2, part d
    printf("sniff_disconnect_complete(): status %d handle 0x%.4X reason %d\n",
                                evt->status, btohs(evt->handle), evt->reason);

    if (evt->status == 0)
    {
        if (findConnectionDataFromHandle(btohs(evt->handle)))
        {
            removeConnectionData(btohs(evt->handle));
        }
        else if (findScoData(btohs(evt->handle), NULL))
        {
            removeScoData(btohs(evt->handle));
        }
    }
}

void sniff_role_change(evt_role_change* evt)
{
    unifi_MACAddress address = {{evt->bdaddr.b[0], evt->bdaddr.b[1], evt->bdaddr.b[2], evt->bdaddr.b[3], evt->bdaddr.b[4], evt->bdaddr.b[5]}};
    printf("sniff_role_change(): status %d bdaddr %s role %d\n", evt->status, trace_unifi_MACAddress(address, traceMacAddressBuffer), evt->role);

    // status - 00:success
    if (evt->status == 0)
    {
        ConnectionData* connection = findConnectionDataFromAddress(address);
        if (connection == NULL)
        {
            printf("sniff_role_change() Cannot find connection for address %s\n", trace_unifi_MACAddress(address, traceMacAddressBuffer));
            return;
        }

        // role - 00:master, 01:slave
        connection->role = (evt->role?unifi_BtDeviceSlave:unifi_BtDeviceMaster);

        if (smeRunning)
        {
            printf("unifi_bt_acl_change_ind(address:%s, role:%d, mode:%d)\n",
                                        trace_unifi_MACAddress(address, traceMacAddressBuffer),
                                        connection->role,
                                        connection->mode);
            unifi_bt_acl_change_ind(NULL, &connection->address, connection->role, connection->mode);
        }
    }
}

void sniff_mode_change(evt_mode_change* evt)
{
    // status - 00:success
    // handle
    // mode - 00:active, 01:hold, 02:sniff, 03:park
    // interval - number of baseband slots
    printf("sniff_mode_change(): status %d handle 0x%.4X mode %d interval %d\n",
                                 evt->status, btohs(evt->handle), evt->mode, btohs(evt->interval));

    // status - 00:success
    if (evt->status == 0)
    {
        ConnectionData* connection = findConnectionDataFromHandle(btohs(evt->handle));
        if (connection == NULL)
        {
            printf("sniff_role_change() Cannot find connection for address %d\n", btohs(evt->handle));
            return;
        }

        switch(evt->mode)
        {
        case 0: connection->mode = unifi_BtDeviceActive; break;
        case 2: connection->mode = unifi_BtDeviceSniff;  break;
        case 3: connection->mode = unifi_BtDevicePark;   break;
        case 1: /* connection->mode = unifi_BtDeviceHold; break;*/
        default:
            printf("sniff_role_change() unsupported mode %d\n", evt->mode);
            return;
            break;
        }

        if (smeRunning)
        {
            printf("unifi_bt_acl_change_ind(address:%s, role:%d, mode:%d)\n",
                                        trace_unifi_MACAddress(connection->address, traceMacAddressBuffer),
                                        connection->role,
                                        connection->mode);
            unifi_bt_acl_change_ind(NULL, &connection->address, connection->role, connection->mode);
        }
    }
}


void sniff_a2dp_start(CsrUint16 handle)
{
    printf("sniff_a2dp_start(): handle 0x%.4X\n", handle);
    updateConnectionData(handle, unifi_BtDeviceLogicalChannelGarrenteed, 1);
}

void sniff_a2dp_stop(CsrUint16 handle)
{
    printf("sniff_a2dp_stop(): handle 0x%.4X\n", handle);
    updateConnectionData(handle, unifi_BtDeviceLogicalChannelNone, 0);
}

void sniff_obex_start(CsrUint16 handle)
{
    printf("sniff_obex_start(): handle 0x%.4X\n", handle);
    updateConnectionData(handle, unifi_BtDeviceLogicalChannelData, 0);
}

void sniff_obex_stop(CsrUint16 handle)
{
    printf("sniff_obex_stop(): handle 0x%.4X\n", handle);
    updateConnectionData(handle, unifi_BtDeviceLogicalChannelNone, 0);
}



static int set_afh_map(CsrUint8* map)
{
    int r;

    int dd = hci_open_dev(bt_hci_dev);
    if (dd < 0) {
        printf("mbd_set_afh_map() HCI device open failed\n");
        return -1;
    }

    /*TODO: check timeout (last parameter), will we block?? */
    r =  hci_set_afh_classification(dd, map, 1000);

    hci_close_dev(dd);

    return r;
}

static void set_channel_avoidance(CsrUint16 channelMhz, CsrUint16 bandwidthMhz)
{
#define AFH_BYTE_ARRAY_SIZE 10
#define AFH_MAP_SIZE 79
    int i;
    CsrUint8 map[AFH_BYTE_ARRAY_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F};
    printf("set_channel_avoidance(channelMhz:%d, bandwidthMhz:%d)\n", channelMhz, bandwidthMhz);

    if (channelMhz && bandwidthMhz)
    {
        bandwidthMhz /= 2;
        channelMhz = channelMhz - 2402; /* normalize */
        CsrMemSet(map, 0, sizeof(map));

        for( i = 0; i < AFH_MAP_SIZE; i++ ) {
            if( abs( i - channelMhz ) >= bandwidthMhz ) {
                map[i / 8] |= ( 1 << ( i % 8 ) );
            }
        }
    }
    set_afh_map(map);
}

void unifi_bt_wifi_active_req(FsmContext* context)
{

    printf("unifi_bt_wifi_active_req()\n");

    smeRunning = TRUE;
    int i, j;

    if (!btConnected)
    {
        return;
    }

    /* SME has reset Need to send all the current BT data to the SME */
    printf("unifi_bt_bt_active_ind()\n");
    unifi_bt_bt_active_ind(NULL);

    for (i = 0; i < MAX_ACL_LINKS; i++)
    {
        if (connectionData[i].inUse)
        {
            printf("unifi_bt_acl_start_ind(address:%s, role:%d, mode:%d)\n", trace_unifi_MACAddress(connectionData[i].address, traceMacAddressBuffer), connectionData[i].role, connectionData[i].mode);
            unifi_bt_acl_start_ind(NULL, &connectionData[i].address, connectionData[i].role, connectionData[i].mode);

            printf("unifi_bt_acl_channel_types_ind(address:%s,logicalChannelTypeMask:%d, numberOfGuaranteedLogicalChannels:%d)\n",
                                        trace_unifi_MACAddress(connectionData[i].address, traceMacAddressBuffer),
                                        connectionData[i].logicalChannelTypeMask,
                                        connectionData[i].numberOfGuaranteedLogicalChannels);

            unifi_bt_acl_channel_types_ind(NULL,
                                           &connectionData[i].address,
                                           connectionData[i].logicalChannelTypeMask,
                                           connectionData[i].numberOfGuaranteedLogicalChannels);

            for (j = 0; j < MAX_SCO_LINKS; j++)
            {
                if (connectionData[i].scoData[j].inUse)
                {
                    printf("unifi_bt_sco_start_ind(address:%s, scohandle:0x%.4X, periodSlots:%d, durationMinSlots:%d, durationMaxSlots:%d)\n",
                                                trace_unifi_MACAddress(connectionData[i].address, traceMacAddressBuffer),
                                                connectionData[i].scoData[j].scoHandle,
                                                connectionData[i].scoData[j].periodSlots,
                                                connectionData[i].scoData[j].durationMinSlots,
                                                connectionData[i].scoData[j].durationMaxSlots);

                    unifi_bt_sco_start_ind(NULL, &connectionData[i].address,
                                                  connectionData[i].scoData[j].scoHandle,
                                                  connectionData[i].scoData[j].periodSlots,
                                                  connectionData[i].scoData[j].durationMinSlots,
                                                  connectionData[i].scoData[j].durationMaxSlots);
                }
            }
        }
    }
}

void unifi_bt_wifi_inactive_req(FsmContext* context)
{

    printf("unifi_bt_wifi_inactive_req()\n");

    smeRunning = FALSE;
    if (btConnected)
    {
        set_channel_avoidance(0, 0);
    }
}

void unifi_bt_active_wifi_channel_req(FsmContext* context, CsrUint16 channelMhz, CsrUint16 bandwidthMhz)
{
    printf("unifi_bt_active_wifi_channel_req(channelMhz:%d, bandwidthMhz:%d)\n", channelMhz,bandwidthMhz);

    if (smeRunning && btConnected)
    {
        set_channel_avoidance(channelMhz, bandwidthMhz);
    }
}

void onIpcConnect()
{
    unifi_bt_wifi_active_req(NULL);
}

void onIpcDisconnect()
{
    smeRunning = FALSE;
}

void onBtConnect()
{
    btConnected = TRUE;
    CsrMemSet(connectionData, 0x00, sizeof(connectionData));
    unifi_bt_bt_active_ind(NULL);
}

void onBtDisconnect()
{
    btConnected = FALSE;
    CsrMemSet(connectionData, 0x00, sizeof(connectionData));
    unifi_bt_bt_inactive_ind(NULL);
}


