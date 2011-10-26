#include <linux/module.h>
#include <linux/init.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/gpio_v2.h>
#include <mach/script_v2.h>

extern int sw_sdio_powerup(char* mname);
extern int sw_sdio_poweroff(char* mname);

static int rfkill_set_power(void *data, bool blocked)
{
    printk("rfkill set power %d\n", blocked);
    /*
     * bluetooth power control, pullup control pins here
     * example:
     * bt config in sys_config1.fex like this:
     * [bt_para]
     * bt_used                  = 1
     * bt_uart_id               = 2
     * bt_rst                   = port:PB05<1><default><default><default>
     *
     * firstly, we should get the handle for gpio control
     * u32 bt_pio = gpio_request_ex(wifi_para, NULL);
     * if (!bt_pio)
     * {
     *     ....
     * }
     * then if we want pullup the "bt_rst" pin, we can do this:
     * ret = gpio_write_one_pin_value(bt_pio, 1, "bt_rst");
     * if (ret)
     * {
     *     ...;
     * }
     */
     
    if (!blocked) {
        sw_sdio_powerup("USI-BM01A-BT");
        sw_sdio_powerup("USI-BM01A-BTRST");
    } else {
        sw_sdio_poweroff("USI-BM01A-BT");
        sw_sdio_poweroff("USI-BM01A-BTRST");
    }
    mdelay(200);
    return 0;
}

static const char bt_name[] = "bcm4329";
static struct rfkill *sw_rfkill;
static struct rfkill_ops sw_rfkill_ops = {
    .set_block = rfkill_set_power,
};

static int sw_rfkill_probe(struct platform_device *pdev)
{
    int ret = 0;
//    bool default_state = true; /* off */

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

