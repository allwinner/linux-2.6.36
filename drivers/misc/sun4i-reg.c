/* driver/misc/sunxi-reg.c
 *
 *  Copyright (C) 2011 Allwinner Technology Co.Ltd
 *  Tom Cubie <tangliang@allwinnertech.com>
 *
 *  www.allwinnertech.com
 *
 *  User access to the registers driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/device.h>

#undef DEBUG_SUNXI

#ifdef DEBUG_SUNXI
#define sunxi_reg_dbg(x...) printk(x)
#else
#define sunxi_reg_dbg(x...)
#endif

#if 0
static char readme[] = "This is a userspace interface to access the sunxi soc registers.\n"
                       "Usage:\n"
                       "\techo address > read           # Read the value at address\n"
					   "\teg: echo f1c20c14 > read\n"
                       "\techo address:value > write    # Write value to address\n"
					   "\teg: echo f1c20c14:ffff > write\n"
                       "\tcat read or cat write         # See this readme\n"
                       "Note: Always use hex and always use virtual address\n"
					   "Warnning: use at your own risk\n";

#endif

static ulong sunxi_reg_value, sunxi_reg_addr;

static ssize_t sunxi_reg_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(sunxi_reg_addr > 0xF0000000 && sunxi_reg_addr < 0xFFFFFFFF) {
		sunxi_reg_value = readl(sunxi_reg_addr);
		return sprintf(buf, "0x%lx", sunxi_reg_value);
	} else {
		return sprintf(buf, "Please set valid address first\n");
	}
}

static ssize_t sunxi_reg_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(sunxi_reg_addr > 0xF0000000 && sunxi_reg_addr < 0xFFFFFFFF) {
		return sprintf(buf, "0x%lx", sunxi_reg_addr);
	} else {
		return sprintf(buf, "Invalid address\n");
	}
}

static ssize_t sunxi_reg_value_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;

	err = strict_strtoul(buf, 16, &sunxi_reg_value);

	if (err) {
		printk("Invalid value\n");
		return err;
	}

	if(sunxi_reg_addr < 0xF0000000 || sunxi_reg_addr > 0xFFFFFFFF) {
		printk("Please set valid address first\n");
		return -1;
	}

	writel(sunxi_reg_value, sunxi_reg_addr);

	return count;
}

static ssize_t sunxi_reg_addr_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;

	err = strict_strtoul(buf, 16, &sunxi_reg_addr);

	if (err) {
		printk("Invalid address\n");
		return err;
	}

	return count;
}

static DEVICE_ATTR(value, S_IRUGO|S_IWUSR|S_IWGRP,
		sunxi_reg_value_show, sunxi_reg_value_store);
static DEVICE_ATTR(address, S_IRUGO|S_IWUSR|S_IWGRP,
		sunxi_reg_addr_show, sunxi_reg_addr_store);

static struct attribute *sunxi_reg_attributes[] = {
	&dev_attr_value.attr,
	&dev_attr_address.attr,
	NULL
};

static struct attribute_group sunxi_reg_attribute_group = {
	.name = "rw",
	.attrs = sunxi_reg_attributes
};

static struct miscdevice sunxi_reg_dev = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		"sunxi-reg",
};

static int __init sunxi_reg_init(void) {
	int err;

	pr_info("sunxi debug register driver init\n");

	err = misc_register(&sunxi_reg_dev);
	if(err) {
		pr_err("%s register sunxi debug register driver as misc device error\n", __FUNCTION__);
		goto exit;
	}

	sysfs_create_group(&sunxi_reg_dev.this_device->kobj,
						 &sunxi_reg_attribute_group);
exit:
	return err;
}

static void __exit sunxi_reg_exit(void) {

	sunxi_reg_dbg("Bye, sunxi_reg exit\n");
	misc_deregister(&sunxi_reg_dev);
	sysfs_remove_group(&sunxi_reg_dev.this_device->kobj,
						 &sunxi_reg_attribute_group);
}

module_init(sunxi_reg_init);
module_exit(sunxi_reg_exit);

MODULE_DESCRIPTION("a simple sunxi register driver");
MODULE_AUTHOR("Tom Cubie");
MODULE_LICENSE("GPL");
