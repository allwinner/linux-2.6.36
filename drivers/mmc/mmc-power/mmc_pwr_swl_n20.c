/*
 * Nanoradio sdio wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#define nano_msg(...)    do {printk("[nano_wifi]: "__VA_ARGS__);} while(0)

extern u32 mmc_pio_hdle;

int nano_sdio_powerup(u32 id, char* mname)
{
    int ret;
    
    /* enable wakeup (no use) */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 1, "swl_n20_host_wakeup");
    if (ret)
    {
        nano_msg("Failed to pullup wakeup pin !\n");
        return -1;
    }
    
    /* enable vcc */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 1, "swl_n20_vcc_en");
    if (ret)
    {
        nano_msg("Failed to enable VCC-3v3 !\n");
        gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_host_wakeup");
        return -1;
    }
    
    /* delay 5 micro seconds */
    udelay(100);
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 1, "swl_n20_shdn");
    if (ret)
    {
        nano_msg("Failed to shut down N20S!\n");
        gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_vcc_en");
        gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_host_wakeup");
        return -1;
    }
    
    /* delay 150 microseconds */
    udelay(50);
    /* enable vdd */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 1, "swl_n20_vdd_en");
    if (ret)
    {
        nano_msg("Failed to enable VDD-1v2 !\n");
        gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_shdn");
        gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_vcc_en");
        gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_host_wakeup");
        return -1;
    }
    
    nano_msg("%s Power UP !!\n", mname);

    return 0;
}
EXPORT_SYMBOL(nano_sdio_powerup);

int nano_sdio_poweroff(u32 id, char* mname)
{
    int ret;
    
    /* disable vdd */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_shdn");
    if (ret)
    {
        nano_msg("Failed to shutdown wifi card !\n");
        return -1;
    }
    
    /* disable vdd */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_vdd_en");
    if (ret)
    {
        nano_msg("Failed to disable VDD-1v2 !\n");
        return -1;
    }

    /* disable vcc */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_vcc_en");
    if (ret)
    {
        nano_msg("Failed to disable VCC-3v3 !\n");
        return -1;
    }
    
    /* disable wakeup (no use) */
    ret = gpio_write_one_pin_value(mmc_pio_hdle, 0, "swl_n20_host_wakeup");
    if (ret)
    {
        nano_msg("Failed to pulldown wakeup pin !\n");
        return -1;
    }
    
    nano_msg("%s Power Off !!\n", mname);

    return 0;
}
EXPORT_SYMBOL(nano_sdio_poweroff);

void nano_wifi_gpio_init(void)
{
    
}