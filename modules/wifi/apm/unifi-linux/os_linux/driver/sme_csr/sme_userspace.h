/*
 * ***************************************************************************
 *  FILE:     sme_userspace.h
 * 
 *  PURPOSE:    SME related definitions.
 * 
 *  Copyright (C) 2007-2008 by Cambridge Silicon Radio Ltd.
 *
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */
#ifndef __LINUX_SME_USERSPACE_H__
#define __LINUX_SME_USERSPACE_H__ 1

#include <linux/kernel.h>

#include "sme_top_level_fsm/sme.h" 
#include "sys_sap/sys_sap_remote_sme_interface.h"
#include "mgt_sap/mgt_sap_remote_sme_interface.h"

int uf_sme_init(unifi_priv_t *priv);
void uf_sme_deinit(unifi_priv_t *priv);

int sme_queue_message(unifi_priv_t *priv, u8 *buffer, int length);

int receive_remote_sys_hip_req(FsmContext* context, CsrUint8 *buffer, CsrUint32 length);


#endif /* __LINUX_SME_USERSPACE_H__ */
