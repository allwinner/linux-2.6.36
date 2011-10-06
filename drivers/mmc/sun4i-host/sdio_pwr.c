#include "host_op.h"

extern struct sw_sdio_res sw_sdio_card;
extern struct awsmc_host* sw_host[4];
extern unsigned int smc_debug;

int sw_get_sdio_resource(void)
{
    int ret;
    char* wifi_para = "sdio_wifi_para";
    
    memset(&sw_sdio_card, 0, sizeof(sw_sdio_card));
    ret = script_parser_fetch(wifi_para, "sdio_wifi_used", &sw_sdio_card.used, sizeof(unsigned)); 
    if (ret)
    {
        awsmc_dbg_err("failed to fetch sdio card configuration!\n");
    }
    if (!sw_sdio_card.used)
    {
        awsmc_dbg_err("no sdio card used in configuration\n");
        goto fail;
    }
    
    ret = script_parser_fetch(wifi_para, "sdio_wifi_sdc_id", &sw_sdio_card.sdc_id, sizeof(unsigned));
    if (ret)
    {
        awsmc_dbg_err("failed to fetch sdio card's sdcid\n");
        goto fail;
    }

    sw_sdio_card.pio_hdle = gpio_request_ex(wifi_para, NULL);
    if (!sw_sdio_card.pio_hdle)
    {
        awsmc_msg("failed to fetch sdio card's io handler, please check it !!\n");
    }

    awsmc_msg("SDIO card reource is OK !!\n");
    return 0;
    
fail:
    memset(&sw_sdio_card, 0, sizeof(sw_sdio_card));
    return -1;
}

int sw_put_sdio_resource(void)
{
    if (sw_sdio_card.pio_hdle)
        gpio_release(sw_sdio_card.pio_hdle, 2);
    memset(&sw_sdio_card, 0, sizeof(sw_sdio_card));
    
    awsmc_msg("SDIO card resource is released !!\n");
    return 0;
}

/*
 * Nanoradio sdio wifi power management API
 */
int nano_sdio_powerup(u32 id, char* mname)
{
    int ret;
    struct awsmc_host* smc_host = NULL;
    
    BUG_ON(id > 3);
    BUG_ON(id != sw_sdio_card.sdc_id);
    BUG_ON(sw_host[id] == NULL);
    
    smc_host = sw_host[id];
    if (sw_sdio_card.poweron)
    {
        awsmc_msg("%s is already powered on !!\n", mname);
        goto detect;
    }
    
    /* enable wakeup (no use) */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 1, "sdio_wifi_host_wakeup");
    if (ret)
    {
        awsmc_dbg_err("Failed to pullup wakeup pin !\n");
        return -1;
    }
    
    /* enable vcc */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 1, "sdio_wifi_vcc_en");
    if (ret)
    {
        awsmc_dbg_err("Failed to enable VCC-3v3 !\n");
        gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_host_wakeup");
        return -1;
    }
    
    /* delay 5 micro seconds */
    udelay(100);
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 1, "sdio_wifi_shdn");
    if (ret)
    {
        awsmc_dbg_err("Failed to shut down N20S!\n");
        gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_vcc_en");
        gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_host_wakeup");
        return -1;
    }
    
    /* delay 150 microseconds */
    udelay(50);
    /* enable vdd */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 1, "sdio_wifi_vdd_en");
    if (ret)
    {
        awsmc_dbg_err("Failed to enable VDD-1v2 !\n");
        gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_shdn");
        gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_vcc_en");
        gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_host_wakeup");
        return -1;
    }
    
    sw_sdio_card.poweron = 1;
    strcpy((void*)sw_sdio_card.mname, mname);
    
    awsmc_msg("%s Power UP !!\n", mname);

detect:
    if (!smc_host->present)
    {
        awsmc_msg("card is inserted by %s\n", mname);
        smc_host->present = 1;
        mmc_detect_change(smc_host->mmc, msecs_to_jiffies(1));
    }
    
    return 0;
}

int nano_sdio_poweroff(u32 id, char* mname)
{
    int ret;
    struct awsmc_host* smc_host = NULL;
    
    BUG_ON(id > 3);
    BUG_ON(id != sw_sdio_card.sdc_id);
    BUG_ON(sw_host[id] == NULL);
    
    smc_host = sw_host[id];
    if (!sw_sdio_card.poweron)
    {
        awsmc_msg("%s is already powered off !!\n", mname);
        goto detect;
    }
    
    /* disable vdd */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_shdn");
    if (ret)
    {
        awsmc_msg("Failed to shutdown wifi card !\n");
        return -1;
    }
    
    /* disable vdd */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_vdd_en");
    if (ret)
    {
        awsmc_msg("Failed to disable VDD-1v2 !\n");
        return -1;
    }

    /* disable vcc */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_vcc_en");
    if (ret)
    {
        awsmc_msg("Failed to disable VCC-3v3 !\n");
        return -1;
    }
    
    /* disable wakeup (no use) */
    ret = gpio_write_one_pin_value(sw_sdio_card.pio_hdle, 0, "sdio_wifi_host_wakeup");
    if (ret)
    {
        awsmc_msg("Failed to pulldown wakeup pin !\n");
        return -1;
    }
    
    sw_sdio_card.poweron = 0;
    awsmc_msg("%s Power Off !!\n", mname);

detect:
    if (smc_host->present && !sw_sdio_card.suspend)
    {
        awsmc_msg("card is removed by %s\n", mname);
        smc_host->present = 0;
        mmc_detect_change(smc_host->mmc, msecs_to_jiffies(1));
    }
    return 0;
}

int sw_sdio_powerup(char* mname)
{
    return nano_sdio_powerup(3, mname);
}
EXPORT_SYMBOL_GPL(sw_sdio_powerup);

int sw_sdio_poweroff(char* mname)
{
    return nano_sdio_poweroff(3, mname);
}
EXPORT_SYMBOL_GPL(sw_sdio_poweroff);

