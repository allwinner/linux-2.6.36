
/*
 * USI wm-bn-bm-01-5(bcm4329) sdio wifi power management API
 * gpio define
 * apm_6981_vcc_en         = port:PA09<1><default><default><0>
 * apm_6981_vdd_en         = port:PA10<1><default><default><0>
 * apm_6981_wakeup         = port:PA11<1><default><default><0>
 * apm_6981_rst_n          = port:PA12<1><default><default><0>
 * apm_6981_pwd_n          = port:PA13<1><default><default><0>
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#define apm_msg(...)    do {printk("[apm_wifi]: "__VA_ARGS__);} while(0)

extern u32 mmc_pio_hdle;

int apm_6xxx_gpio_ctrl(const char* name, int level)
{
    const char* gpio_cmd[5] = {"apm_6981_vcc_en", "apm_6981_vdd_en", "apm_6981_wakeup", 
                               "apm_6981_rst_n", "apm_6981_pwd_n"};
    int i = 0;
    int ret = 0;
    
    for (i=0; i<5; i++) {
        if (strcmp(name, gpio_cmd[i])==0)
            break;
    }
    if (i==5) {
        apm_msg("No gpio %s for APM 6XXX module\n", name);
        return -1;
    }
    
    ret = gpio_write_one_pin_value(mmc_pio_hdle, level, name);
    if (ret) {
        apm_msg("Failed to set gpio %s to %d !\n", name, level);
        return -1;
    }
    apm_msg("Set gpio %s to %d !\n", name, level);
    return 0;
}
EXPORT_SYMBOL(apm_6xxx_gpio_ctrl);


int apm_6xxx_get_gpio_value(const char* name)
{
    return -1;
}
EXPORT_SYMBOL(apm_6xxx_get_gpio_value);

void apm_6xxx_gpio_init(void)
{
    apm_6xxx_gpio_ctrl("apm_6981_wakeup", 1);
    apm_6xxx_gpio_ctrl("apm_6981_pwd_n", 1);
    apm_6xxx_gpio_ctrl("apm_6981_rst_n", 1);
}