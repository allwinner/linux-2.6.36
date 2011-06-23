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


#ifdef CONFIG_ANDROID_PMEM
/*
 *  Android pmem devices
 */
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	//.start = CONFIG_ANDROID_PMEM_BASE,
	//.size = CONFIG_ANDROID_PMEM_SIZE*1024,
	.no_allocator = 1,
	.cached = 1,
	.buffered = 0,
};


static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	//.start = CONFIG_ANDROID_PMEM_ADSP_BASE,
	//.size = CONFIG_ANDROID_PMEM_ADSP_SIZE*1024,
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
	unsigned long size = CONFIG_ANDROID_PMEM_SIZE * 1024 * 1024  - 4 * 1024 * 1024;

	pmem_base = (void *)CONFIG_ANDROID_PMEM_BASE;
	android_pmem_pdata.start = (unsigned long)pmem_base;
	android_pmem_pdata.size = size;
	android_pmem_adsp_pdata.start = (unsigned long)(((char *)pmem_base) + size);
	android_pmem_adsp_pdata.size = 4*1024*1024; /* 4MB for ADSP */

	pr_info("pmem: base=0x%x, size=0x%x\n", (unsigned int)android_pmem_pdata.start,
		(unsigned int)android_pmem_pdata.size);
	pr_info("pmem_adsp: base=0x%x, size=0x%x\n", (unsigned int)android_pmem_adsp_pdata.start,
		(unsigned int)android_pmem_adsp_pdata.size);

	return 0;
}

#endif /* CONFIG_ANDROID_PMEM */

/*
 * All platform devices
 */
static struct platform_device *aw_pdevs[] __initdata = {
#ifdef CONFIG_ANDROID_PMEM
	&android_pmem_device0,
	&android_pmem_device1,
#endif /* CONFIG_ANDROID_PMEM */
};

int __init aw_pdevs_init(void)
{
	pr_info("Platform devices init\n");

#ifdef CONFIG_ANDROID_PMEM
	if (init_pmem_devs()) {
		return -ENOMEM;
	}
#endif
	platform_add_devices(aw_pdevs, ARRAY_SIZE(aw_pdevs));

	return 0;
}
arch_initcall(aw_pdevs_init);

static int read_chip_sid(char* page, char** start, off_t off, int count,
	int* eof, void* data)
{

	return sprintf(page, "%08x-%08x-%08x-%08x\n", 
		readl(SW_VA_SID_IO_BASE + 0x0),
		readl(SW_VA_SID_IO_BASE + 0x4),
		readl(SW_VA_SID_IO_BASE + 0x8),
		readl(SW_VA_SID_IO_BASE + 0xc));
}

static int __init platform_proc_init(void)
{
	struct proc_dir_entry *sid_entry = NULL;

	sid_entry = create_proc_read_entry("sid", 0400,
			NULL, read_chip_sid, NULL);

	if (!sid_entry) {
		pr_err("Create sid at /proc failed\n");
	}

	return 0;
}
module_init(platform_proc_init);


