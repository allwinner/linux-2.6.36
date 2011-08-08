/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"
#include "sw_usb_platform.h" /* by Cesc */

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by platform data */
#define VENDOR_ID		0x18D1
#define PRODUCT_ID		0x0001

struct android_dev {
	struct usb_composite_dev *cdev;
	struct usb_configuration *config;
	int num_products;
	struct android_usb_product *products;
	int num_functions;
	char **functions;

	int vendor_id;
	int product_id;
	int version;
};

static struct android_dev *_android_dev;

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "Android",
	[STRING_PRODUCT_IDX].s = "Android",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct list_head _functions = LIST_HEAD_INIT(_functions);
static bool _are_functions_bound;

static struct android_usb_function *get_function(const char *name)
{
	struct android_usb_function	*f;
	list_for_each_entry(f, &_functions, list) {
		if (!strcmp(name, f->name))
			return f;
	}
	return 0;
}

static bool are_functions_registered(struct android_dev *dev)
{
	char **functions = dev->functions;
	int i;

	/* Look only for functions required by the board config */
	for (i = 0; i < dev->num_functions; i++) {
		char *name = *functions++;
		bool is_match = false;
		/* Could reuse get_function() here, but a reverse search
		 * should yield less comparisons overall */
		struct android_usb_function *f;
		list_for_each_entry_reverse(f, &_functions, list) {
			if (!strcmp(name, f->name)) {
				is_match = true;
				break;
			}
		}
		if (is_match)
			continue;
		else
			return false;
	}

	return true;
}

static bool should_bind_functions(struct android_dev *dev)
{
	/* Don't waste time if the main driver hasn't bound */
	if (!dev->config)
		return false;

	/* Don't waste time if we've already bound the functions */
	if (_are_functions_bound)
		return false;

	/* This call is the most costly, so call it last */
	if (!are_functions_registered(dev))
		return false;

	return true;
}

static void bind_functions(struct android_dev *dev)
{
	struct android_usb_function	*f;
	char **functions = dev->functions;
	int i;

	for (i = 0; i < dev->num_functions; i++) {
		char *name = *functions++;
		f = get_function(name);
		if (f)
			f->bind_config(dev->config);
		else
			printk(KERN_ERR "function %s not found in bind_functions\n", name);
	}

	_are_functions_bound = true;
}

static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_DEBUG "android_bind_config\n");
	dev->config = c;

	if (should_bind_functions(dev))
		bind_functions(dev);

	return 0;
}

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl);

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.bind		= android_bind_config,
	.setup		= android_setup_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0xFA, /* 500ma */
};

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl)
{
	int i;
	int ret = -EOPNOTSUPP;

	for (i = 0; i < android_config_driver.next_interface_id; i++) {
		if (android_config_driver.interface[i]->setup) {
			ret = android_config_driver.interface[i]->setup(
				android_config_driver.interface[i], ctrl);
			if (ret >= 0)
				return ret;
		}
	}
	return ret;
}

static int product_has_function(struct android_usb_product *p,
		struct usb_function *f)
{
	char **functions = p->functions;
	int count = p->num_functions;
	const char *name = f->name;
	int i;

	for (i = 0; i < count; i++) {
		/* For functions with multiple instances, usb_function.name
		 * will have an index appended to the core name (ex: acm0),
		 * while android_usb_product.functions[i] will only have the
		 * core name (ex: acm). So, only compare up to the length of
		 * android_usb_product.functions[i].
		 */
		if (!strncmp(name, functions[i], strlen(functions[i])))
			return 1;
	}
	return 0;
}

static int product_matches_functions(struct android_usb_product *p)
{
	struct usb_function		*f;
	list_for_each_entry(f, &android_config_driver.functions, list) {
		if (product_has_function(p, f) == !!f->disabled)
			return 0;
	}
	return 1;
}

static int get_vendor_id(struct android_dev *dev)
{
	struct android_usb_product *p = dev->products;
	int count = dev->num_products;
	int i;

	if (p) {
		for (i = 0; i < count; i++, p++) {
			if (p->vendor_id && product_matches_functions(p))
				return p->vendor_id;
		}
	}
	/* use default vendor ID */
	return dev->vendor_id;
}

static int get_product_id(struct android_dev *dev)
{
	struct android_usb_product *p = dev->products;
	int count = dev->num_products;
	int i;

	if (p) {
		for (i = 0; i < count; i++, p++) {
			if (product_matches_functions(p))
				return p->product_id;
		}
	}
	/* use default product ID */
	return dev->product_id;
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, ret;

	printk(KERN_INFO "android_bind\n");

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	/* register our configuration */
	ret = usb_add_config(cdev, &android_config_driver);
	if (ret) {
		printk(KERN_ERR "usb_add_config failed\n");
		return ret;
	}

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;
	device_desc.idVendor = __constant_cpu_to_le16(get_vendor_id(dev));
	device_desc.idProduct = __constant_cpu_to_le16(get_product_id(dev));
	cdev->desc.idVendor = device_desc.idVendor;
	cdev->desc.idProduct = device_desc.idProduct;

	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= android_bind,
	.enable_function = android_enable_function,
};

void android_register_function(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_INFO "android_register_function %s\n", f->name);
	list_add_tail(&f->list, &_functions);

	if (dev && should_bind_functions(dev))
		bind_functions(dev);
}

void update_dev_desc(struct android_dev *dev)
{
	struct usb_function *f;
	struct usb_function *last_enabled_f = NULL;
	int num_enabled = 0;
	int has_iad = 0;

	dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
	dev->cdev->desc.bDeviceSubClass = 0x00;
	dev->cdev->desc.bDeviceProtocol = 0x00;

	list_for_each_entry(f, &android_config_driver.functions, list) {
		if (!f->disabled) {
			num_enabled++;
			last_enabled_f = f;
			if (f->descriptors[0]->bDescriptorType ==
					USB_DT_INTERFACE_ASSOCIATION)
				has_iad = 1;
		}
		if (num_enabled > 1 && has_iad) {
			dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
			dev->cdev->desc.bDeviceSubClass = 0x02;
			dev->cdev->desc.bDeviceProtocol = 0x01;
			break;
		}
	}

	if (num_enabled == 1) {
#ifdef CONFIG_USB_ANDROID_RNDIS
		if (!strcmp(last_enabled_f->name, "rndis")) {
#ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
			dev->cdev->desc.bDeviceClass =
					USB_CLASS_WIRELESS_CONTROLLER;
#else
			dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
#endif
		}
#endif
	}
}

void android_enable_function(struct usb_function *f, int enable)
{
	struct android_dev *dev = _android_dev;
	int disable = !enable;

	if (!!f->disabled != disable) {
		usb_function_set_enabled(f, !disable);

#ifdef CONFIG_USB_ANDROID_RNDIS
		if (!strcmp(f->name, "rndis")) {
			struct usb_function		*func;
			/* Windows does not support other interfaces when RNDIS is enabled,
			 * so we disable UMS and MTP when RNDIS is on.
			 */
			list_for_each_entry(func, &android_config_driver.functions, list) {
				if (!strcmp(func->name, "usb_mass_storage")
					|| !strcmp(func->name, "mtp")) {
					usb_function_set_enabled(func, !enable);
				}
			}
		}
#endif

		update_dev_desc(dev);

		device_desc.idVendor = __constant_cpu_to_le16(get_vendor_id(dev));
		device_desc.idProduct = __constant_cpu_to_le16(get_product_id(dev));
		if (dev->cdev) {
			dev->cdev->desc.idVendor = device_desc.idVendor;
			dev->cdev->desc.idProduct = device_desc.idProduct;
		}
		usb_composite_force_reset(dev->cdev);
	}
}

static int android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *dev = _android_dev;

	printk(KERN_INFO "android_probe pdata: %p\n", pdata);

	if (pdata) {
		dev->products = pdata->products;
		dev->num_products = pdata->num_products;
		dev->functions = pdata->functions;
		dev->num_functions = pdata->num_functions;
		if (pdata->vendor_id) {
			dev->vendor_id = pdata->vendor_id;
			device_desc.idVendor =
				__constant_cpu_to_le16(pdata->vendor_id);
		}
		if (pdata->product_id) {
			dev->product_id = pdata->product_id;
			device_desc.idProduct =
				__constant_cpu_to_le16(pdata->product_id);
		}
		if (pdata->version)
			dev->version = pdata->version;

		if (pdata->product_name)
			strings_dev[STRING_PRODUCT_IDX].s = pdata->product_name;
		if (pdata->manufacturer_name)
			strings_dev[STRING_MANUFACTURER_IDX].s =
					pdata->manufacturer_name;
		if (pdata->serial_number)
			strings_dev[STRING_SERIAL_IDX].s = pdata->serial_number;
	}

	return usb_composite_register(&android_usb_driver);
}
/*
** add platform_device for android device. by Cesc -begin
*/

static char *sw_usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *sw_usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *sw_usb_functions_all[] = {
	"usb_mass_storage",
	"adb",
};

static struct android_usb_product sw_usb_products[] = {
	{  /* usb_mass_storage */
    	.vendor_id      = SW_USB_VENDOR_ID,
    	.product_id     = SW_USB_UMS_PRODUCT_ID,
    	.num_functions  = ARRAY_SIZE(sw_usb_functions_ums),
    	.functions   	= sw_usb_functions_ums,
	},

	{  /* adb */
    	.vendor_id      = SW_USB_VENDOR_ID,
    	.product_id     = SW_USB_ADB_PRODUCT_ID,
        .num_functions  = ARRAY_SIZE(sw_usb_functions_ums_adb),
        .functions      = sw_usb_functions_ums_adb,
	},
};

static struct android_usb_platform_data sw_usb_android_pdata = {
	.vendor_id          = SW_USB_VENDOR_ID,
	.product_id         = SW_USB_UMS_PRODUCT_ID,
	.version            = SW_USB_VERSION,

	.product_name       = SW_USB_PRODUCT_NAME,
	.manufacturer_name  = SW_USB_MANUFACTURER_NAME,
	.serial_number      = SW_USB_SERIAL_NUMBER,

	.num_products       = ARRAY_SIZE(sw_usb_products),
	.products	        = sw_usb_products,

	.num_functions      = ARRAY_SIZE(sw_usb_functions_all),
	.functions          = sw_usb_functions_all,
};

static struct platform_device sw_usb_android_device = {
	.name	= "android_usb",
	.id     = -1,
	.dev	= {
		.platform_data	= &sw_usb_android_pdata,
	},
};

//---------------------------------------------------------------
//  usb_mass_storage
//---------------------------------------------------------------
static struct usb_mass_storage_platform_data sw_usb_ums_pdata = {
	.vendor     = SW_USB_MASS_STORAGE_VENDOR_NAME,
	.product    = SW_USB_MASS_STORAGE_PRODUCT_NAME,
	.release    = SW_USB_MASS_STORAGE_RELEASE,
	.nluns      = SW_USB_NLUNS,
};

static struct platform_device sw_usb_ums_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &sw_usb_ums_pdata,
	},
};

struct usb_msc_config{
    /* usb feature */
    u32 vendor_id;
    u32 mass_storage_id;
    u32 adb_id;

    char usb_manufacturer_name[64];
    char usb_product_name[64];
    char usb_serial_number[64];

    /* usb_mass_storage feature */
    char msc_vendor_name[64];
    char msc_product_name[64];
    u32 msc_release;
    u32 luns;
};

#include <mach/script_v2.h>

static s32 get_msc_config(struct usb_msc_config *config)
{
    s32 ret = 0;

    //----------------------------------------
    //  usb_feature
    //----------------------------------------

    /* vendor_id */
    ret = script_parser_fetch("usb_feature", "vendor_id", (int *)&(config->vendor_id), 64);
	if(ret != 0){
	    printk("ERR: get usb_feature vendor_id failed\n");
	}

    /* mass_storage_id */
    ret = script_parser_fetch("usb_feature", "mass_storage_id", (int *)&(config->mass_storage_id), 64);
	if(ret != 0){
	    printk("ERR: get usb_feature mass_storage_id failed\n");
	}

    /* adb_id */
    ret = script_parser_fetch("usb_feature", "adb_id", (int *)&(config->adb_id), 64);
	if(ret != 0){
	    printk("ERR: get usb_feature adb_id failed\n");
	}

	/* manufacturer_name */
    ret = script_parser_fetch("usb_feature", "manufacturer_name", (int *)config->usb_manufacturer_name, 64);
	if(ret != 0){
	    printk("ERR: get usb_feature manufacturer_name failed\n");
	}

	/* product_name */
    ret = script_parser_fetch("usb_feature", "product_name", (int *)config->usb_product_name, 64);
	if(ret != 0){
	    printk("ERR: get usb_feature product_name failed\n");
	}

	/* serial_number */
    ret = script_parser_fetch("usb_feature", "serial_number", (int *)config->usb_serial_number, 64);
	if(ret != 0){
	    printk("ERR: get usb_feature serial_number failed\n");
	}

    //----------------------------------------
    //  msc_feature
    //----------------------------------------

	/* vendor_name */
    ret = script_parser_fetch("msc_feature", "vendor_name", (int *)config->msc_vendor_name, 64);
	if(ret != 0){
	    printk("ERR: get msc_feature vendor_name failed\n");
	}

	/* product_name */
    ret = script_parser_fetch("msc_feature", "product_name", (int *)config->msc_product_name, 64);
	if(ret != 0){
	    printk("ERR: get msc_feature product_name failed\n");
	}

	/* release */
    ret = script_parser_fetch("msc_feature", "release", (int *)&(config->msc_release), 64);
	if(ret != 0){
	    printk("ERR: get msc_feature release failed\n");
	}

	/* luns */
    ret = script_parser_fetch("msc_feature", "luns", (int *)&(config->luns), 64);
	if(ret != 0){
	    printk("ERR: get msc_feature luns failed\n");
	}

    return 0;
}

static void print_msc_config(struct usb_msc_config *config)
{
    printk("------print_msc_config-----\n");
    printk("vendor_id             = 0x%x\n", config->vendor_id);
    printk("mass_storage_id       = 0x%x\n", config->mass_storage_id);
    printk("adb_id                = 0x%x\n", config->adb_id);

    printk("usb_manufacturer_name = %s\n", config->usb_manufacturer_name);
    printk("usb_product_name      = %s\n", config->usb_product_name);
    printk("usb_serial_number     = %s\n", config->usb_serial_number);

    printk("msc_vendor_name       = %s\n", config->msc_vendor_name);
    printk("msc_product_name      = %s\n", config->msc_product_name);
    printk("msc_release           = %d\n", config->msc_release);
    printk("luns                  = %d\n", config->luns);
    printk("---------------------------\n");
}

static struct usb_msc_config g_usb_msc_config;

static s32 modify_device_data(struct android_usb_platform_data *android_pdata,
                       struct usb_mass_storage_platform_data *ums_pdata)
{
    struct usb_msc_config *config = &g_usb_msc_config;

    memset(config, 0, sizeof(struct usb_msc_config));

    get_msc_config(config);

    print_msc_config(config);

    //----------------------------------------
    //  android
    //----------------------------------------

    if(config->vendor_id){
        android_pdata->vendor_id = config->vendor_id;
        android_pdata->products[0].vendor_id = config->vendor_id;
        android_pdata->products[1].vendor_id = config->vendor_id;
    }

    if(config->mass_storage_id){
        android_pdata->product_id = config->mass_storage_id;
        android_pdata->products[0].product_id = config->mass_storage_id;
    }

    if(config->adb_id){
        android_pdata->products[1].product_id = config->adb_id;
    }

    if(config->usb_manufacturer_name[0] != 0){
        android_pdata->manufacturer_name = config->usb_manufacturer_name;
    }

    if(config->usb_product_name[0] != 0){
        android_pdata->product_name = config->usb_product_name;
    }

    if(config->usb_serial_number[0] != 0){
        android_pdata->serial_number = config->usb_serial_number;
    }

    //----------------------------------------
    //  usb_mass_storage
    //----------------------------------------

    if(config->msc_vendor_name[0] != 0){
        ums_pdata->vendor = config->msc_vendor_name;
    }

    if(config->msc_product_name[0] != 0){
        ums_pdata->product = config->msc_product_name;
    }

    if(config->msc_release){
        ums_pdata->release = config->msc_release;
    }

    if(config->luns){
        ums_pdata->nluns = config->luns;
    }

    return 0;
}

/* by Cesc - end */

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", },
	.probe = android_probe,
};

static int __init init(void)
{
	struct android_dev *dev;

	printk(KERN_INFO "android init\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* device register */
	printk(KERN_INFO "android-platform_device_register\n");

	modify_device_data(&sw_usb_android_pdata, &sw_usb_ums_pdata);

	platform_device_register(&sw_usb_android_device);
	platform_device_register(&sw_usb_ums_device);

	/* set default values, which should be overridden by platform data */
	dev->product_id = PRODUCT_ID;
	_android_dev = dev;

	return platform_driver_register(&android_platform_driver);
}
module_init(init);

static void __exit cleanup(void)
{
	printk(KERN_INFO "android cleanup\n");
	usb_composite_unregister(&android_usb_driver);
	platform_driver_unregister(&android_platform_driver);
	kfree(_android_dev);
	_android_dev = NULL;
}
module_exit(cleanup);
