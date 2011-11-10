
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>
#include "mmc_pm.h"

#define mmc_pm_msg(...)    do {printk("[mmc_pm]: "__VA_ARGS__);} while(0)


struct mmc_pm_ops mmc_card_pm_ops;
static char* wifi_para = "sdio_wifi_para";
static char* wifi_mod[] = {" ", "swl-n20", "usi-bm01a", "ar6302qfn", "apm6xxx", "swb-b23"};

int mmc_pm_get_mod_type(void)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    if (ops->sdio_card_used)
        return ops->module_sel;
    else {
        mmc_pm_msg("No sdio card, please check your config !!\n");
        return 0;
    }
}
EXPORT_SYMBOL(mmc_pm_get_mod_type);

int mmc_pm_gpio_ctrl(char* name, int level)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    if (ops->sdio_card_used)
        return ops->gpio_ctrl(name, level);
    else {
        mmc_pm_msg("No sdio card, please check your config !!\n");
        return -1;
    }
}
EXPORT_SYMBOL(mmc_pm_gpio_ctrl);

int mmc_pm_get_io_val(char* name)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    if (ops->sdio_card_used)
        return ops->get_io_val(name);
    else {
        mmc_pm_msg("No sdio card, please check your config !!\n");
        return -1;
    }
}
EXPORT_SYMBOL(mmc_pm_get_io_val);

static int mmc_pm_get_res(void)
{
    int ret = 0;
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    
    ret = script_parser_fetch(wifi_para, "sdio_wifi_used", &ops->sdio_card_used, sizeof(unsigned)); 
    if (ret) {
        mmc_pm_msg("failed to fetch sdio card configuration!\n");
        return -1;
    }
    if (!ops->sdio_card_used) {
        mmc_pm_msg("no sdio card used in configuration\n");
        return -1;
    }
    
    ret = script_parser_fetch(wifi_para, "sdio_wifi_sdc_id", &ops->sdio_cardid, sizeof(unsigned));
    if (ret) {
        mmc_pm_msg("failed to fetch sdio card's sdcid\n");
        return -1;
    }

    ret = script_parser_fetch(wifi_para, "sdio_wifi_mod_sel", &ops->module_sel, sizeof(unsigned));
    if (ret) {
        mmc_pm_msg("failed to fetch sdio module select\n");
        return -1;
    }
    ops->mod_name = wifi_mod[ops->module_sel];
    printk("[wifi]: Select sdio wifi: %s !!\n", wifi_mod[ops->module_sel]);
    
    ops->pio_hdle = gpio_request_ex(wifi_para, NULL);
    if (!ops->pio_hdle) {
        mmc_pm_msg("failed to fetch sdio card's io handler, please check it !!\n");
        return -1;
    }
    
    return 0;
}

static int __devinit mmc_pm_probe(struct platform_device *pdev)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    
    switch (ops->module_sel) {
        case 1: /* nano wifi */
            nano_wifi_gpio_init();
            break;
        case 2: /* usi bm01a */
            usi_bm01a_gpio_init();
            break;
        case 3: /* ar6302qfn */
            //ar6302qfn_gpio_init();
            break;
        case 4: /* apm 6xxx */
            apm_6xxx_gpio_init();
            break;
        case 5: /* swb b23 */
            swbb23_gpio_init();
            break;
        default:
            mmc_pm_msg("Wrong sdio module select %d !!\n", ops->module_sel);
    }
    
    mmc_pm_msg("SDIO card gpio init is OK !!\n");
    return 0;
}

static int __devexit mmc_pm_remove(struct platform_device *pdev)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    
    switch (ops->module_sel) {
        case 1: /* nano wifi */
            nano_wifi_gpio_init();
            break;
        case 2: /* usi bm01a */
            usi_bm01a_gpio_init();
            break;
        case 3: /* ar6302qfn */
            //ar6302qfn_gpio_init();
            break;
        case 4: /* usi bm01a */
            apm_6xxx_gpio_init();
        case 5: /* swb b23 */
            swbb23_gpio_init();
            break;
        default:
            mmc_pm_msg("Wrong sdio module select %d !!\n", ops->module_sel);
    }
    
    mmc_pm_msg("SDIO card gpio is released !!\n");
    return 0;
}

#ifdef CONFIG_PM
static int mmc_pm_suspend(struct device *dev)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    
    if (ops->standby)
        ops->standby(1);
    return 0;
}
static int mmc_pm_resume(struct device *dev)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    
    if (ops->standby)
        ops->standby(0);
    return 0;
}

static struct dev_pm_ops mmc_pm_ops = {
    .suspend	= mmc_pm_suspend,
    .resume		= mmc_pm_resume,
};
#endif

static struct platform_device mmc_pm_dev = {
    .name           = "mmc_pm",
};

static struct platform_driver mmc_pm_driver = {
    .driver.name    = "mmc_pm",
    .driver.owner   = THIS_MODULE,
#ifdef CONFIG_PM
    .driver.pm	    = &mmc_pm_ops,
#endif
    .probe          = mmc_pm_probe,
    .remove         = __devexit_p(mmc_pm_remove),
};

static int __init mmc_pm_init(void)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    
    memset(ops, 0, sizeof(struct mmc_pm_ops));
    mmc_pm_get_res();
    if (!ops->sdio_card_used)
        return 0;
        
    platform_device_register(&mmc_pm_dev);
    return platform_driver_register(&mmc_pm_driver);
}

static void __exit mmc_pm_exit(void)
{
    struct mmc_pm_ops *ops = &mmc_card_pm_ops;
    if (!ops->sdio_card_used)
        return;
        
    if (ops->pio_hdle)
        gpio_release(ops->pio_hdle, 2);
    
    memset(ops, 0, sizeof(struct mmc_pm_ops));
    platform_driver_unregister(&mmc_pm_driver);
}

module_init(mmc_pm_init);
module_exit(mmc_pm_exit);

