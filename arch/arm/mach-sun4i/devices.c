#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/amba/pl061.h>
#include <linux/amba/mmci.h>
#include <linux/amba/pl022.h>
#include <linux/io.h>
#include <linux/gfp.h>
#include <linux/clockchips.h>
#include <linux/bootmem.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/android_pmem.h>

#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/sw_sys.h>
#include <asm/uaccess.h>

#include <asm/clkdev.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/icst.h>
#include <asm/hardware/vic.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <mach/clkdev.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/script_v2.h>
#include <mach/gpio_v2.h>
#include <mach/system.h>

#ifdef CONFIG_ANDROID_PMEM
/*
 *  Android pmem devices
 */
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
	.cached = 1,
	.buffered = 0,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.no_allocator = 0,
	.cached = 0,
	.buffered = 0,
};

static struct platform_device android_pmem_device0 = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_device1 = {
	.name = "android_pmem",
	.id = 1,
	.dev = {.platform_data = &android_pmem_adsp_pdata },
};

static int __init init_pmem_devs(void)
{
	void *pmem_base = NULL;
	unsigned long size = CONFIG_ANDROID_PMEM_SIZE * 1024 * 1024;

	pmem_base = (void *)CONFIG_ANDROID_PMEM_BASE;
	android_pmem_pdata.start = (unsigned long)pmem_base;
	android_pmem_pdata.size = size;

	pr_info("pmem: base=0x%x, size=0x%x\n", (unsigned int)android_pmem_pdata.start,
		(unsigned int)android_pmem_pdata.size);

	return 0;
}

/*
 * All platform devices
 */
static struct platform_device *aw_pdevs[] __initdata = {
	&android_pmem_device0,
};


int __init aw_pdevs_init(void)
{
	pr_info("Platform devices init\n");

	if (init_pmem_devs()) {
		return -ENOMEM;
	}

	platform_add_devices(aw_pdevs, ARRAY_SIZE(aw_pdevs));

	return 0;
}
arch_initcall(aw_pdevs_init);

#endif /* CONFIG_ANDROID_PMEM */

static atomic_t sw_sys_status = ATOMIC_INIT(0);

static struct sw_script_para script_para;

static long sw_sys_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int err = -EFAULT;

	pr_debug("sw_sys: ioctl\n");
	if (copy_from_user(&script_para, argp, sizeof(script_para))) {
		return -EFAULT;
	}

	switch (cmd) {
	case SW_SYS_IOC_GET_TOTAL_MAINKEY:
		break;
	case SW_SYS_IOC_GET_TOTAL_SUBKEY:
		break;
	case SW_SYS_IOC_GET_KEY_VALUE:
		pr_debug("sw_sys_ioctl: SW_SYS_IOC_GET_KEY_VALUE\n");
		err = script_parser_fetch_ex(script_para.main_name, script_para.sub_name, script_para.value, &(script_para.value_type), SW_SCRIPT_PARA_VALUE_BUF_SIZE);
	case SW_SYS_IOC_GET_TOTAL_GPIO:
		break;
	case SW_SYS_IOC_GET_GPIO_CFG:
		break;
	default:
		break;
	}

	if (!err) {
		if (copy_to_user(argp, &script_para, sizeof(script_para))) {
			pr_err("%s: copy to user failed\n", __func__);
			return -EFAULT;
		}
	}

	return err;
}

static int sw_sys_open(struct inode *inode, struct file *file)
{
	pr_debug("sw_sys: open\n");
	if (atomic_read(&sw_sys_status) > 0) {
		pr_info("sw_sys busy\n");
		return -EBUSY;
	}

	atomic_set(&sw_sys_status, 1);
	return 0;
}

static int sw_sys_release(struct inode *inode, struct file *file)
{
	pr_debug("sw_sys: release\n");
	atomic_set(&sw_sys_status, 0);
	return 0;
}

static const struct file_operations sw_sys_fops = {
	.unlocked_ioctl	= sw_sys_ioctl,
	.open		= sw_sys_open,
	.release	= sw_sys_release,
};

static struct miscdevice sw_sys_dev = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		"sw_ctl",
	.fops =		&sw_sys_fops
};

static int __init sw_sys_init(void)
{
	pr_info("sw_sys: init\n");
	return misc_register(&sw_sys_dev);
}

/* This module will not be remove */
module_init(sw_sys_init);

extern void sw_get_sid(int *a, int *b, int *c, int *d);

int sid_readl(const volatile void __iomem *addr)
{
	return readl(addr);
}

static int read_chip_sid(char* page, char** start, off_t off, int count,
	int* eof, void* data)
{
	int a, b, c, d;
	sw_get_sid(&a, &b, &c, &d);
	return sprintf(page, "%08x-%08x-%08x-%08x\n", a, b, c, d);
}

static int read_chip_version(char* page, char** start, off_t off, int count,
	int* eof, void* data)
{
	enum sw_ic_ver ver = sw_get_ic_ver();

	switch (ver) {
	case MAGIC_VER_A:
		return sprintf(page, "A\n");
	case MAGIC_VER_B:
		return sprintf(page, "B\n");
	case MAGIC_VER_C:
		return sprintf(page, "C\n");
	default:
		return sprintf(page, "%d\n", ver);
	}

	return 0;
}


static int __init platform_proc_init(void)
{
	struct proc_dir_entry *sid_entry = NULL;
	struct proc_dir_entry *version_entry = NULL;

	sid_entry = create_proc_read_entry("sid", 0400,
			NULL, read_chip_sid, NULL);

	if (!sid_entry) {
		pr_err("Create sid at /proc failed\n");
	}

	version_entry = create_proc_read_entry("sw_ver", 0400,
			NULL, read_chip_version, NULL);

	if (!version_entry) {
		pr_err("Create version at /proc failed\n");
	}

	return 0;
}
module_init(platform_proc_init);


