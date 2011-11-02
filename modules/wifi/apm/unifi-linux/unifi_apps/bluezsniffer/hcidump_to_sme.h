#ifndef HCIDUMP_TO_SME_H_
#define HCIDUMP_TO_SME_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "bluezsniffer_top_level_fsm/bluezsniffer_top_level_fsm.h"
#include "parser/parser.h"

/* Call when connected */
extern void onIpcConnect();
extern void onIpcDisconnect();
extern void onBtConnect();
extern void onBtDisconnect();

extern void sniff_connect_request(evt_conn_request* evt);

extern void sniff_acl_connect(create_conn_cp* evt);
extern void sniff_acl_connect_complete(evt_conn_complete* evt);

extern void sniff_role_change(evt_role_change* evt);
extern void sniff_mode_change(evt_mode_change* evt);

extern void sniff_sco_connect(setup_sync_conn_cp* evt);
extern void sniff_sco_connect_complete(evt_conn_complete* evt);
extern void sniff_sco_sync_connect_complete(evt_sync_conn_complete* evt);

extern void sniff_disconnect_complete(evt_disconn_complete* evt);

extern void sniff_a2dp_start(CsrUint16 handle);
extern void sniff_a2dp_stop(CsrUint16 handle);

extern void sniff_obex_start(CsrUint16 handle);
extern void sniff_obex_stop(CsrUint16 handle);


#endif /* HCIDUMP_TO_SME_H_ */
