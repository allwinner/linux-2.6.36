/*
 * ***************************************************************************
 *  FILE:     putest.c
 * 
 *  PURPOSE:    putest related functions.
 * 
 *  Copyright (C) 2008-2009 by Cambridge Silicon Radio Ltd.
 *  
 * Refer to LICENSE.txt included with this source code for details on
 * the license terms.
 *
 * ***************************************************************************
 */

#include <linux/vmalloc.h>
#include <linux/firmware.h>

#include "unifi_priv.h"


int unifi_putest_cmd52_read(unifi_priv_t *priv, unsigned char *arg)
{
    struct unifi_putest_cmd52 cmd52_params;
    CsrUint8 *arg_pos;
    unsigned int cmd_param_size;
    int r;
    CsrInt32 csr_r;
    unsigned char ret_buffer[32];
    CsrUint8 *ret_buffer_pos;

    arg_pos = (CsrUint8*)(((unifi_putest_command_t*)arg) + 1);
    if (get_user(cmd_param_size, (int*)arg_pos)) {
        unifi_error(priv,
                    "unifi_putest_cmd52_read: Failed to get the argument\n");
        return -EFAULT;
    }

    if (cmd_param_size != sizeof(struct unifi_putest_cmd52)) {
        unifi_error(priv,
                    "unifi_putest_cmd52_read: cmd52 struct mismatch\n");
        return -EINVAL;
    }

    arg_pos += sizeof(unsigned int);
    if (copy_from_user(&cmd52_params,
                       (void*)arg_pos,
                       sizeof(struct unifi_putest_cmd52))) {
        unifi_error(priv,
                    "unifi_putest_cmd52_read: Failed to get the cmd52 params\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG2, "cmd52r: func=%d addr=0x%x ",
                cmd52_params.funcnum, cmd52_params.addr);

    if (cmd52_params.funcnum == 0) {
        csr_r = CsrSdioF0Read8(priv->sdio, cmd52_params.addr, &cmd52_params.data);
    } else {
        csr_r = CsrSdioRead8(priv->sdio, cmd52_params.addr, &cmd52_params.data);
    }
    if (csr_r) {
        unifi_error(priv,
                    "\nunifi_putest_cmd52_read: unifi_sdio_readb() failed (r=0x%x)\n", csr_r);
        return convert_csr_error(csr_r);
    }
    unifi_trace(priv, UDBG2, "data=%d\n", cmd52_params.data);

    /* Copy the info to the out buffer */
    *(unifi_putest_command_t*)ret_buffer = UNIFI_PUTEST_CMD52_READ;
    ret_buffer_pos = (CsrUint8*)(((unifi_putest_command_t*)ret_buffer) + 1);
    *(unsigned int*)ret_buffer_pos = sizeof(struct unifi_putest_cmd52);
    ret_buffer_pos += sizeof(unsigned int);
    memcpy(ret_buffer_pos, &cmd52_params, sizeof(struct unifi_putest_cmd52));
    ret_buffer_pos += sizeof(struct unifi_putest_cmd52);

    r = copy_to_user((void*)arg,
                     ret_buffer,
                     ret_buffer_pos - ret_buffer);
    if (r) {
        unifi_error(priv,
                    "unifi_putest_cmd52_read: Failed to return the data\n");
        return -EFAULT;
    }

    return 0;
}


int unifi_putest_cmd52_write(unifi_priv_t *priv, unsigned char *arg)
{
    struct unifi_putest_cmd52 cmd52_params;
    CsrUint8 *arg_pos;
    unsigned int cmd_param_size;
    CsrInt32 csr_r;

    arg_pos = (CsrUint8*)(((unifi_putest_command_t*)arg) + 1);
    if (get_user(cmd_param_size, (int*)arg_pos)) {
        unifi_error(priv,
                    "unifi_putest_cmd52_write: Failed to get the argument\n");
        return -EFAULT;
    }

    if (cmd_param_size != sizeof(struct unifi_putest_cmd52)) {
        unifi_error(priv,
                    "unifi_putest_cmd52_write: cmd52 struct mismatch\n");
        return -EINVAL;
    }

    arg_pos += sizeof(unsigned int);
    if (copy_from_user(&cmd52_params,
                       (void*)(arg_pos),
                       sizeof(struct unifi_putest_cmd52))) {
        unifi_error(priv,
                    "unifi_putest_cmd52_write: Failed to get the cmd52 params\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG2, "cmd52w: func=%d addr=0x%x data=%d\n",
                cmd52_params.funcnum, cmd52_params.addr, cmd52_params.data);


    if (cmd52_params.funcnum == 0) {
        csr_r = CsrSdioF0Write8(priv->sdio, cmd52_params.addr, cmd52_params.data);
    } else {
        csr_r = CsrSdioWrite8(priv->sdio, cmd52_params.addr, cmd52_params.data);
    }
    if (csr_r) {
        unifi_error(priv,
                    "unifi_putest_cmd52_write: unifi_sdio_writeb() failed (r=0x%x)\n", csr_r);
    }

    return convert_csr_error(csr_r);
}


int unifi_putest_set_sdio_clock(unifi_priv_t *priv, unsigned char *arg)
{
    int sdio_clock_speed;
    int r;

    if (get_user(sdio_clock_speed, (int*)(((unifi_putest_command_t*)arg) + 1))) {
        unifi_error(priv,
                    "unifi_putest_set_sdio_clock: Failed to get the argument\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG2, "set sdio clock: %d KHz\n", sdio_clock_speed);

    r = csr_sdio_set_max_clock_speed(priv->sdio, sdio_clock_speed);
    if (r) {
        unifi_error(priv,
                    "unifi_putest_set_sdio_clock: Set clock failed (r=0x%x)\n", r);
        return r;
    }

    return 0;
}


int unifi_putest_start(unifi_priv_t *priv, unsigned char *arg)
{
    int r;
    CsrInt32 csr_r;
    int already_in_test = priv->ptest_mode;

    /* Ensure that sme_sys_suspend() doesn't power down the chip because:
     *  1) Power is needed anyway for ptest.
     *  2) The app code uses the START ioctl as a reset, so it gets called
     *     multiple times. If the app stops the XAPs, but the power_down/up
     *     sequence doesn't actually power down the chip, there can be problems
     *     resetting, because part of the power_up sequence disables function 1
     */
    priv->ptest_mode = 1;
    
    /* Suspend the SME and UniFi */
    if (priv->sme_cli) {
        r = sme_sys_suspend(priv);
        if (r) {
            unifi_error(priv,
                        "unifi_putest_start: failed to suspend UniFi\n");
            return r;
        }
    }

    /* Application may have stopped the XAPs, but they are needed for reset */
    if (already_in_test) {
        csr_r = unifi_start_processors(priv->card);
        if (csr_r) {
            unifi_error(priv, "Failed to start XAPs. Hard reset required.\n");
        }
    } else {
        /* Ensure chip is powered for the case where there's no unifi_helper */
        csr_r = CsrSdioPowerOn(priv->sdio);
        if (csr_r) {
            unifi_error(priv, "CsrSdioPowerOn status %d\n", csr_r);
        }
    }

    csr_r = unifi_init(priv->card);
    if (csr_r && (csr_r != 1)) {
        unifi_error(priv,
                    "unifi_putest_start: failed to init UniFi\n");
        return convert_csr_error(csr_r);
    }

    return 0;
}


int unifi_putest_stop(unifi_priv_t *priv, unsigned char *arg)
{
    int r = 0;
    CsrInt32 csr_r;

    /* Application may have stopped the XAPs, but they are needed for reset */
    csr_r = unifi_start_processors(priv->card);
    if (csr_r) {
        unifi_error(priv, "Failed to start XAPs. Hard reset required.\n");
    }

    /* At this point function 1 is enabled and the XAPs are running, so it is
     * safe to let the card power down. Power is restored later, asynchronously,
     * during the wifi_on requested by the SME.
     */
    CsrSdioPowerOff(priv->sdio);

    /* Resume the SME and UniFi */
    if (priv->sme_cli) {
        r = sme_sys_resume(priv);
        if (r) {
            unifi_error(priv,
                        "unifi_putest_stop: failed to resume UniFi\n");
        }
    }
    priv->ptest_mode = 0;

    return r;
}


int unifi_putest_dl_fw(unifi_priv_t *priv, unsigned char *arg)
{
#define UF_PUTEST_MAX_FW_FILE_NAME      16
#define UNIFI_MAX_FW_PATH_LEN           32
    unsigned int fw_name_length;
    unsigned char fw_name[UF_PUTEST_MAX_FW_FILE_NAME+1];
    unsigned char *name_buffer;
    int postfix;
    char fw_path[UNIFI_MAX_FW_PATH_LEN];
    const struct firmware *fw_entry;
    struct dlpriv temp_fw_sta;
    int r;
    CsrInt32 csr_r;

    /* Get the f/w file name length */
    if (get_user(fw_name_length, (unsigned int*)(((unifi_putest_command_t*)arg) + 1))) {
        unifi_error(priv,
                    "unifi_putest_dl_fw: Failed to get the length argument\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG2, "unifi_putest_dl_fw: file name size = %d\n", fw_name_length);

    /* Sanity check for the f/w file name length */
    if (fw_name_length > UF_PUTEST_MAX_FW_FILE_NAME) {
        unifi_error(priv,
                    "unifi_putest_dl_fw: F/W file name is too long\n");
        return -EINVAL;
    }

    /* Get the f/w file name */
    name_buffer = ((unsigned char*)arg) + sizeof(unifi_putest_command_t) + sizeof(unsigned int);
    if (copy_from_user(fw_name, (void*)name_buffer, fw_name_length)) {
        unifi_error(priv, "unifi_putest_dl_fw: Failed to get the file name\n");
        return -EFAULT;
    }
    fw_name[fw_name_length] = '\0';
    unifi_trace(priv, UDBG2, "unifi_putest_dl_fw: file = %s\n", fw_name);

    /* Keep the existing f/w to a temp, we need to restore it later */
    temp_fw_sta = priv->fw_sta;
    
    /* Get the putest f/w */
    postfix = priv->instance;
    scnprintf(fw_path, UNIFI_MAX_FW_PATH_LEN, "unifi-sdio-%d/%s",
              postfix, fw_name);
    r = request_firmware(&fw_entry, fw_path, priv->unifi_device);
    if (r == 0) {
        priv->fw_sta.fw_desc = (void *)fw_entry;
        priv->fw_sta.dl_data = fw_entry->data;
        priv->fw_sta.dl_len = fw_entry->size;
    } else {
        unifi_error(priv, "Firmware file not available\n");
        return -EINVAL;
    }

    /* Application may have stopped the XAPs, but they are needed for reset */
    csr_r = unifi_start_processors(priv->card);
    if (csr_r) {
        unifi_error(priv, "Failed to start XAPs. Hard reset required.\n");
    }

    /* Download the f/w. On UF6xxx this will cause the f/w file to convert
     * into patch format and download via the ROM boot loader
     */
    csr_r = unifi_download(priv->card, 0x0c00);
    if (csr_r < 0) {
        unifi_error(priv,
                    "unifi_putest_dl_fw: failed to download the f/w\n");
        goto free_fw;
    }

    /* Free the putest f/w... */
free_fw:
    uf_release_firmware(priv, &priv->fw_sta);
    /* ... and restore the original f/w */
    priv->fw_sta = temp_fw_sta;

    return convert_csr_error(csr_r);
}


int unifi_putest_dl_fw_buff(unifi_priv_t *priv, unsigned char *arg)
{
    unsigned int fw_length;
    unsigned char *fw_buf = NULL;
    unsigned char *fw_user_ptr;
    struct dlpriv temp_fw_sta;
    CsrInt32 csr_r;
    
    /* Get the f/w buffer length */
    if (get_user(fw_length, (unsigned int*)(((unifi_putest_command_t*)arg) + 1))) {
        unifi_error(priv,
                    "unifi_putest_dl_fw_buff: Failed to get the length arg\n");
        return -EFAULT;
    }

    unifi_trace(priv, UDBG2, "unifi_putest_dl_fw_buff: size = %d\n", fw_length);

    /* Sanity check for the buffer length */
    if (fw_length == 0 || fw_length > 0xfffffff) {
        unifi_error(priv,
                    "unifi_putest_dl_fw_buff: buffer length bad %u\n", fw_length);
        return -EINVAL;
    }

    /* Buffer for kernel copy of the f/w image */
    fw_buf = CsrPmalloc(fw_length);
    if (!fw_buf) {
        unifi_error(priv, "unifi_putest_dl_fw_buff: malloc fail\n");
        return -ENOMEM;
    }
    
    /* Get the f/w image */
    fw_user_ptr = ((unsigned char*)arg) + sizeof(unifi_putest_command_t) + sizeof(unsigned int);
    if (copy_from_user(fw_buf, (void*)fw_user_ptr, fw_length)) {
        unifi_error(priv, "unifi_putest_dl_fw_buff: Failed to get the buffer\n");
        CsrPfree(fw_buf);
        return -EFAULT;
    }
    
    /* Save the existing f/w to a temp, we need to restore it later */    
    temp_fw_sta = priv->fw_sta;

    /* Setting fw_desc NULL indicates to the core that no f/w file was loaded
     * via the kernel request_firmware() mechanism. This indicates to the core
     * that it shouldn't call release_firmware() after the download is done.
     */
    priv->fw_sta.fw_desc = NULL;            /* No OS f/w resource */
    priv->fw_sta.dl_data = fw_buf;
    priv->fw_sta.dl_len = fw_length;
    
    /* Application may have stopped the XAPs, but they are needed for reset */
    csr_r = unifi_start_processors(priv->card);
    if (csr_r) {
        unifi_error(priv, "Failed to start XAPs. Hard reset required.\n");
    }

    /* Download the f/w. On UF6xxx this will cause the f/w file to convert
     * into patch format and download via the ROM boot loader
     */
    csr_r = unifi_download(priv->card, 0x0c00);
    if (csr_r < 0) {
        unifi_error(priv,
                    "unifi_putest_dl_fw_buff: failed to download the f/w\n");
        goto free_fw;
    }

free_fw:
    /* Finished with the putest f/w, so restore the station f/w */
    priv->fw_sta = temp_fw_sta;
    CsrPfree(fw_buf);
    
    return convert_csr_error(csr_r);
}

