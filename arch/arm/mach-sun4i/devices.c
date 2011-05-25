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
	.name = "android_pmem_adsp",
	.id = 1,
	.dev = {.platform_data = &android_pmem_adsp_pdata },
};

static int __init init_pmem_devs(void)
{
	void *pmem_base = NULL;
	unsigned long size = CONFIG_ANDROID_PMEM_SIZE;

	pmem_base = alloc_bootmem(size * 2 * 1024 * 1024);

	if (!pmem_base) {
		pr_err("[pmem] alloc bootmem failed, size=%lu\n",
		       size*2*1024*1024);
		return -ENOMEM;
	}

	android_pmem_pdata.start = (unsigned long)pmem_base;
	android_pmem_pdata.size = size;
	android_pmem_adsp_pdata.start = (unsigned long)
		(((char *)pmem_base) + size * 1024 * 1024);
	android_pmem_adsp_pdata.size = size;

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

