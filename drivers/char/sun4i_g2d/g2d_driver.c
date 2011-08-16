
#include"g2d_driver_i.h"
#include<linux/g2d_driver.h>
#include"g2d.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

static struct info_mem g2d_mem[MAX_G2D_MEM_INDEX];
static int	g2d_mem_sel = 0;

static struct class	*g2d_class;
static struct cdev	*g2d_cdev;
static dev_t		 devid ;
__g2d_drv_t			 g2d_ext_hd;
__g2d_info_t		 para;

static struct resource g2d_resource[2] =
{
	[0] = {
		.start	= 0x01e80000,
		.end	= 0x01e8ffff,
		.flags	= IORESOURCE_MEM,
	},

	[1] = {
		.start	= INTC_IRQNO_DE_MIX,
		.end	= INTC_IRQNO_DE_MIX,
		.flags	= IORESOURCE_IRQ,
	},

};

struct platform_device g2d_device =
{
	.name           = "g2d",
	.id		        = -1,
	.num_resources  = ARRAY_SIZE(g2d_resource),
	.resource	    = g2d_resource,
	.dev            = {}
};

int drv_g2d_begin(void)
{
	int result = 0;

	result = down_interruptible(g2d_ext_hd.g2d_finished_sem);
	return result;
}

int drv_g2d_finish(void)
{
	int result = 0;

	up(g2d_ext_hd.g2d_finished_sem);

	return result;

}

__s32 drv_g2d_init(void)
{
    g2d_init_para init_para;
    __u32 i = 0;

    DBG("drv_g2d_init\n");
    init_para.g2d_base		= (__u32)para.io;
    init_para.g2d_begin		= drv_g2d_begin;
    init_para.g2d_finish	= drv_g2d_finish;
    memset(&g2d_ext_hd, 0, sizeof(__g2d_drv_t));
    g2d_ext_hd.g2d_finished_sem = kmalloc(sizeof(struct semaphore),GFP_KERNEL | __GFP_ZERO);
    if(!g2d_ext_hd.g2d_finished_sem)
    {
        WARNING("create g2d_finished_sem fail!\n");
        return -1;
    }
    sema_init(g2d_ext_hd.g2d_finished_sem, 0);
    for(i = 0; i<MAX_EVENT_SEM; i++)
    {
    	g2d_ext_hd.event_sem[i] = NULL;
    }
    init_waitqueue_head(&g2d_ext_hd.queue);
	g2d_init(&init_para);

    return 0;
}

__s32 g2d_get_free_mem_index(void)
{
    __u32 i = 0;

    for(i=0; i<MAX_G2D_MEM_INDEX; i++)
    {
        if(g2d_mem[i].b_used == 0)
        {
            return i;
        }
    }
    return -1;
}

int g2d_mem_request(__u32 size)
{
	__s32		 sel;
	struct page	*page;
	unsigned	 map_size = 0;

    sel = g2d_get_free_mem_index();
    if(sel < 0)
    {
        ERR("g2d_get_free_mem_index fail!\n");
        return -EINVAL;
    }

	map_size = (size + 4095) & 0xfffff000;//4k ¶ÔÆë
	page = alloc_pages(GFP_KERNEL,get_order(map_size));

	if(page != NULL)
	{
		g2d_mem[sel].virt_addr = page_address(page);
		if(g2d_mem[sel].virt_addr == 0)
		{
			free_pages((unsigned long)(page),get_order(map_size));
			ERR("line %d:fail to alloc memory!\n",__LINE__);
			return -ENOMEM;
		}
		memset(g2d_mem[sel].virt_addr,0,size);
		g2d_mem[sel].phy_addr = virt_to_phys(g2d_mem[sel].virt_addr);
	    g2d_mem[sel].mem_len = size;
		g2d_mem[sel].b_used = 1;

		INFO("map_mixer_memory[%d]: pa=%08lx va=%p size:%x\n",sel,g2d_mem[sel].phy_addr, g2d_mem[sel].virt_addr, size);
		return sel;
	}
	else
	{
		ERR("fail to alloc memory!\n");
		return -ENOMEM;
	}
}

int g2d_mem_release(__u32 sel)
{
	unsigned map_size = PAGE_ALIGN(g2d_mem[sel].mem_len);
	unsigned page_size = map_size;

	if(g2d_mem[sel].b_used == 0)
	{
	    ERR("mem not used in g2d_mem_release,%d\n",sel);
		return -EINVAL;
    }

	free_pages((unsigned long)(g2d_mem[sel].virt_addr),get_order(page_size));
	memset(&g2d_mem[sel],0,sizeof(struct info_mem));

	return 0;
}

int g2d_mmap(struct file *file, struct vm_area_struct * vma)
{
	unsigned long physics;
	unsigned long mypfn;
	unsigned long vmsize = vma->vm_end-vma->vm_start;

    if(g2d_mem[g2d_mem_sel].b_used == 0)
    {
        ERR("mem not used in g2d_mmap,%d\n",g2d_mem_sel);
        return -EINVAL;
    }

	physics =  g2d_mem[g2d_mem_sel].phy_addr;
	mypfn = physics >> PAGE_SHIFT;

	if(remap_pfn_range(vma,vma->vm_start,mypfn,vmsize,vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int g2d_open(struct inode *inode, struct file *file)
{
	g2d_clk_on();
	return 0;
}

static int g2d_release(struct inode *inode, struct file *file)
{
	g2d_clk_off();
	return 0;
}

long g2d_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void		 *kbuffer[1];
	unsigned long ubuffer[1];
	unsigned long karg[2];
	unsigned long aux = 0;
	__s32		  ret = 0;

	kbuffer[0] = 0;
	if(copy_from_user((void*)karg, (void __user*)arg, 2*sizeof(unsigned long)))
	{
		printk("copy_from_user fail\n");
		return -EFAULT;
	}
	ubuffer[0]	= *(unsigned long*)karg;
	aux	= (*(unsigned long*)(karg+1));

	switch (cmd) {

	/* Proceed to the operation */
	case G2D_CMD_BITBLT:
		kbuffer[0] = kmalloc(sizeof(g2d_blt),GFP_KERNEL);
		if(copy_from_user(kbuffer[0], (void __user *)ubuffer[0],sizeof(g2d_blt)))
		{
			kfree(kbuffer[0]);
			return  -EFAULT;
		}
	    ret = g2d_blit((g2d_blt *) kbuffer[0]);
    	break;

	case G2D_CMD_FILLRECT:
		kbuffer[0] = kmalloc(sizeof(g2d_fillrect),GFP_KERNEL);
		if(copy_from_user(kbuffer[0], (void __user *)ubuffer[0],sizeof(g2d_fillrect)))
		{
			kfree(kbuffer[0]);
			return  -EFAULT;
		}
	    ret = g2d_fill((g2d_fillrect *) kbuffer[0]);
    	break;

	case G2D_CMD_STRETCHBLT:
		kbuffer[0] = kmalloc(sizeof(g2d_stretchblt),GFP_KERNEL);
		if(copy_from_user(kbuffer[0], (void __user *)ubuffer[0],sizeof(g2d_stretchblt)))
		{
			kfree(kbuffer[0]);
			return  -EFAULT;
		}
	    ret = g2d_stretchblit((g2d_stretchblt *) kbuffer[0]);
    	break;

	case G2D_CMD_PALETTE_TBL:
		kbuffer[0] = kmalloc(sizeof(g2d_palette),GFP_KERNEL);
		if(copy_from_user(kbuffer[0], (void __user *)ubuffer[0],sizeof(g2d_palette)))
		{
			kfree(kbuffer[0]);
			return  -EFAULT;
		}
	    ret = g2d_set_palette_table((g2d_palette *) kbuffer[0]);
    	break;

	/* just management memory for test */
	case G2D_CMD_MEM_REQUEST:
		ret =  g2d_mem_request(ubuffer[0]);
		break;

	case G2D_CMD_MEM_RELEASE:
		ret =  g2d_mem_release(ubuffer[0]);
		break;

	case G2D_CMD_MEM_SELIDX:
		g2d_mem_sel = ubuffer[0];
		break;

	case G2D_CMD_MEM_GETADR:
	    if(g2d_mem[ubuffer[0]].b_used)
	    {
		    ret = g2d_mem[ubuffer[0]].phy_addr;
		}
		else
		{
			ERR("mem not used in G2D_CMD_MEM_GETADR\n");
		    ret = -1;
		}
		break;

	/* Invalid IOCTL call */
	default:
		return -EINVAL;
	}

	if(kbuffer[0])
	{
		kfree(kbuffer[0]);
		kbuffer[0] = 0;
	}

	return ret;
}

static struct file_operations g2d_fops = {
	.owner				= THIS_MODULE,
	.open				= g2d_open,
	.release			= g2d_release,
	.unlocked_ioctl		= g2d_ioctl,
	.mmap				= g2d_mmap,
};

static int g2d_probe(struct platform_device *pdev)
{
	int size;
	int	ret = 0;
	struct resource	*res;
	__g2d_info_t	*info = NULL;

	info = &para;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev,info);

	/* get the clk */
	g2d_openclk();
//	g2d_clk_on();

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL)
		{
			ERR("failed to get memory register\n");
			ret = -ENXIO;
			goto  dealloc_fb;
		}

	/* reserve the memory */
	size = (res->end - res->start) + 1;
	info->mem = request_mem_region(res->start, size, pdev->name);
	if(info->mem == NULL)
		{
			ERR("failed to get memory region\n");
			ret = -ENOENT;
			goto  relaese_regs;
		}

	/* map the memory */
	info->io = ioremap(res->start, size);
	if(info->io == NULL)
		{
			ERR("iormap() of register failed\n");
			ret = -ENXIO;
			goto  release_mem;
		}

	/* get the irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if(res == NULL)
		{
			ERR("failed to get irq resource\n");
			ret = -ENXIO;
			goto relaese_regs;
		}

	/* request the irq */
	info->irq = res->start;
	ret = request_irq(info->irq,g2d_handle_irq,0,g2d_device.name,NULL);
	if(ret)
		{
			ERR("failed to install irq resource\n");
			goto relaese_regs;
		}

	drv_g2d_init();

	return 0;

	relaese_regs:
		iounmap(info->io);
	release_mem:
		release_resource(info->mem);
		kfree(info->mem);
	dealloc_fb:
		platform_set_drvdata(pdev, NULL);
		kfree(info);

	return ret;
}

static int g2d_remove(struct platform_device *pdev)
{
	__g2d_info_t *info = platform_get_drvdata(pdev);

	/* power down */
	g2d_closeclk();

	free_irq(info->irq, info);
	iounmap(info->io);
	release_resource(info->mem);
	kfree(info->mem);

	platform_set_drvdata(pdev, NULL);

	INFO("Driver unloaded succesfully.\n");
	return 0;
}

static int g2d_suspend(struct platform_device *pdev, pm_message_t state)
{
	g2d_clk_off();
	INFO("g2d_suspend succesfully.\n");

	return 0;
}

static int g2d_resume(struct platform_device *pdev)
{	
	g2d_clk_on();
	INFO("g2d_resume succesfully.\n");

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void g2d_early_suspend(struct early_suspend *h)
{
//    g2d_suspend(NULL, PMSG_SUSPEND);
}

void g2d_late_resume(struct early_suspend *h)
{
//    g2d_resume(NULL);
}

static struct early_suspend g2d_early_suspend_handler =
{
    .level   = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = g2d_early_suspend,
	.resume = g2d_late_resume,
};
#endif


static struct platform_driver g2d_driver = {
	.probe          = g2d_probe,
	.remove         = g2d_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = g2d_suspend,
	.resume         = g2d_resume,
#else
	.suspend        = NULL,
	.resume         = NULL,
#endif
	.driver			=
	{
		.owner		= THIS_MODULE,
		.name		= "g2d",
	},
};

int __init g2d_module_init(void)
{
	int ret, err;

    alloc_chrdev_region(&devid, 0, 1, "g2d_chrdev");
    g2d_cdev = cdev_alloc();
    cdev_init(g2d_cdev, &g2d_fops);
    g2d_cdev->owner = THIS_MODULE;
    err = cdev_add(g2d_cdev, devid, 1);
    if (err)
    {
        ERR("I was assigned major number %d.\n", MAJOR(devid));
        return -1;
    }

    g2d_class = class_create(THIS_MODULE, "g2d_class");
    if (IS_ERR(g2d_class))
    {
        ERR("create class error\n");
        return -1;
    }

	device_create(g2d_class, NULL, devid, NULL, "g2d");
	ret = platform_device_register(&g2d_device);
	if (ret == 0)
	{
		ret = platform_driver_register(&g2d_driver);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
    register_early_suspend(&g2d_early_suspend_handler);
#endif
	INFO("Module initialized.major:%d\n", MAJOR(devid));
	return ret;
}

static void __exit g2d_module_exit(void)
{
	INFO("g2d_module_exit\n");
	kfree(g2d_ext_hd.g2d_finished_sem);

#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&g2d_early_suspend_handler);
#endif

	platform_driver_unregister(&g2d_driver);
	platform_device_unregister(&g2d_device);

    device_destroy(g2d_class,  devid);
    class_destroy(g2d_class);

    cdev_del(g2d_cdev);
}

module_init(g2d_module_init);
module_exit(g2d_module_exit);

MODULE_AUTHOR("yupu_tang");
MODULE_DESCRIPTION("g2d driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:g2d");

