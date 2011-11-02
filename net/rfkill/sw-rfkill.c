#include <linux/module.h>
#include <linux/init.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

#if CONFIG_BT_HCIUART_DEBUG
#define RF_MSG(...)     do {printk("[rfkill]: "__VA_ARGS__);} while(0)
#else
#define RF_MSG(...)
#endif

#if (defined CONFIG_ARCH_SUN4I || defined CONFIG_ARCH_SUN5I)
extern unsigned int get_sdio_wifi_module_select(void);
extern int usi_bm01a_gpio_ctrl(const char* name, int level);
extern int usi_bm01a_get_gpio_value(const char* name);
extern int swbb23_gpio_ctrl(const char* name, int level);
extern int swbb23_get_gpio_value(const char* name);
#else
#define get_sdio_wifi_module_select
#define usi_bm01a_gpio_ctrl
#define usi_bm01a_get_gpio_value
#define swbb23_gpio_ctrl
#define swbb23_get_gpio_value
#endif

static DEFINE_SPINLOCK(bt_power_lock);
static const char bt_name[] = "bcm4329";
static struct rfkill *sw_rfkill;
static int rfkill_set_power(void *data, bool blocked)
{
    unsigned int mod_sel = get_sdio_wifi_module_select();
    
    RF_MSG("rfkill set power %d\n", blocked);
    
    spin_lock(&bt_power_lock);
    switch (mod_sel)
    {
        case 2: /* usi bm01a */
            if (!blocked) {
                usi_bm01a_gpio_ctrl("usi_bm01a_bt_regon", 1);
                usi_bm01a_gpio_ctrl("usi_bm01a_bt_rst", 1);
            } else {
                usi_bm01a_gpio_ctrl("usi_bm01a_bt_regon", 0);
                usi_bm01a_gpio_ctrl("usi_bm01a_bt_rst", 0);
            }
            break;
        case 5: /* swb b23 */
            if (!blocked) {
                swbb23_gpio_ctrl("swbb23_wl_shdn", 1);
            } else {
                swbb23_gpio_ctrl("swbb23_wl_shdn", 0);
            }
            break;
        default:
            RF_MSG("no bt module matched !!\n");
    }
    
    spin_unlock(&bt_power_lock);
    mdelay(100);
    return 0;
}

static struct rfkill_ops sw_rfkill_ops = {
    .set_block = rfkill_set_power,
};

static int sw_rfkill_probe(struct platform_device *pdev)
{
    int ret = 0;

    sw_rfkill = rfkill_alloc(bt_name, &pdev->dev, 
                        RFKILL_TYPE_BLUETOOTH, &sw_rfkill_ops, NULL);
    if (unlikely(!sw_rfkill))
        return -ENOMEM;

    ret = rfkill_register(sw_rfkill);
    if (unlikely(ret)) {
        rfkill_destroy(sw_rfkill);
    }
    return ret;
}

static int sw_rfkill_remove(struct platform_device *pdev)
{
    if (likely(sw_rfkill)) {
        rfkill_unregister(sw_rfkill);
        rfkill_destroy(sw_rfkill);
    }
    return 0;
}

static struct platform_driver sw_rfkill_driver = {
    .probe = sw_rfkill_probe,
    .remove = sw_rfkill_remove,
    .driver = { 
        .name = "sw-rfkill",
        .owner = THIS_MODULE,
    },
};

static struct platform_device sw_rfkill_dev = {
    .name = "sw-rfkill",
};

static int __init sw_rfkill_init(void)
{
    platform_device_register(&sw_rfkill_dev);
    return platform_driver_register(&sw_rfkill_driver);
}

static void __exit sw_rfkill_exit(void)
{
    platform_device_unregister(&sw_rfkill_dev);
    platform_driver_unregister(&sw_rfkill_driver);
}

module_init(sw_rfkill_init);
module_exit(sw_rfkill_exit);

MODULE_DESCRIPTION("sw-rfkill driver");
MODULE_AUTHOR("Aaron.yemao<leafy.myeh@allwinnertech.com>");
MODULE_LICENSE(GPL);

