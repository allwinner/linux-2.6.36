/* driver/misc/sun4i-debugreg.c
 *
 *  Copyright (C) 2011 Allwinner Technology Co.Ltd
 *  Tom Cubie <tangliang@allwinnertech.com>
 *
 *  www.allwinnertech.com
 *
 *  User access to the registers for debugging.
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

#undef DEBUG_SUN4I

#ifdef DEBUG_SUN4I
#define sun4i_reg_dbg(x...) printk(x)
#else
#define sun4i_reg_dbg(x...)
#endif


static char readme[] = "This is a userspace interface to access the sunxi soc registers.\n"
                       "Usage:\n"
                       "\techo address > read           # Read the value at address\n"
					   "\teg: echo f1c20c14 > read\n"
                       "\techo address:value > write    # Write value to address\n"
					   "\teg: echo f1c20c14:ffff > write\n"
                       "\tcat read or cat write         # See this readme\n"
                       "Note: Always use hex and always use virtual address\n"
					   "Warnning: use at your own risk\n";

static ssize_t sun4i_debugreg_read_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long addr;
	int err, len;

	len = strlen(buf);

	/* echo will append '\n', user may use 0x */
	if( len != 9 && len != 11) {
		printk("Invalid address length, please cat read to see readme\n");
		return count;
	}

	err = strict_strtoul(buf, 16, &addr);

	if (err) {
		printk("Invalid value, please cat read to see readme\n");
		return count;
	}

	if(addr < 0xf0000000) {
		printk("Please use virtual address!!!\n");
		return count;
	}

	printk("0x%x\n", readl(addr));

	return count;
}

static ssize_t sun4i_debugreg_write_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long addr, value;
	int err, len;
	const char *s = NULL;
	char addrstr[16];

	s = strchr(buf, ':');
	if( s == NULL) {
		printk("Wrong format, no :, please cat write to see readme\n");
		return count;
	}

	len = s - buf;
	sun4i_reg_dbg("len: %d\n", len);

	if( len != 8 && len != 10) {
		printk("Invalid address length, please cat write to see readme\n");
		return count;
	}

	strncpy(addrstr, buf, len);
	addrstr[len] = '\0';

	sun4i_reg_dbg("addrstr: %s\n", addrstr);

	err = strict_strtoul(addrstr, 16, &addr);
	sun4i_reg_dbg("addr: 0x%lx\n", addr);
	if(err) {
		printk("Invalid address, please cat write to see readme\n");
		return count;
	}

	/* value starts after the : */
	len = strlen(s+1);
	sun4i_reg_dbg("s+1 length: %d\n", len);
	if( len > 11) {
		printk("Invalid value length, please cat read to see readme\n");
		return count;
	}

	err = strict_strtoul(s+1, 16, &value);
	sun4i_reg_dbg("value: 0x%lx\n", value);

	if(err) {
		printk("Invalid value, please cat write to see readme\n");
		return count;
	}

	if(addr < 0xf0000000) {
		printk("Please use virtual address!!!\n");
		return count;
	}

	writel(value, addr);

	return count;
}

static ssize_t sun4i_debugreg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, readme);
}

static DEVICE_ATTR(read, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		sun4i_debugreg_show, sun4i_debugreg_read_store);
static DEVICE_ATTR(write, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		sun4i_debugreg_show, sun4i_debugreg_write_store);

static struct attribute *sun4i_debugreg_attributes[] = {
	&dev_attr_read.attr,
	&dev_attr_write.attr,
	NULL
};

static struct attribute_group sun4i_debugreg_attribute_group = {
	.name = "rw",
	.attrs = sun4i_debugreg_attributes
};

static struct miscdevice sun4i_debugreg_dev = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		"sun4i-debugreg",
};

static int __init sun4i_debugreg_init(void) {
	int err;

	pr_info("sun4i debug register driver init\n");

	err = misc_register(&sun4i_debugreg_dev);
	if(err) {
		pr_err("%s register sun4i debug register driver as misc device error\n", __FUNCTION__);
		goto exit;
	}

	sysfs_create_group(&sun4i_debugreg_dev.this_device->kobj,
						 &sun4i_debugreg_attribute_group);
exit:
	return err;
}

static void __exit sun4i_debugreg_exit(void) {

	sun4i_reg_dbg("bye, sun4i_debugreg exit\n");
	misc_deregister(&sun4i_debugreg_dev);
	sysfs_remove_group(&sun4i_debugreg_dev.this_device->kobj,
						 &sun4i_debugreg_attribute_group);
}

module_init(sun4i_debugreg_init);
module_exit(sun4i_debugreg_exit);

MODULE_DESCRIPTION("a simple sun4i debug driver");
MODULE_AUTHOR("Tom Cubie");
MODULE_LICENSE("GPL");
