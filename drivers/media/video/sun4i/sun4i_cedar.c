/*
**************************************************************************************************************
*											         eLDK
*						            the Easy Portable/Player Develop Kits
*									           desktop system 
*
*						        	 (c) Copyright 2009-2012, ,HUANGXIN China
*											 All Rights Reserved
*
* File    	: sun4i_cedar.c
* By      	: HUANGXIN
* Func		: 
* Version	: v1.0
* ============================================================================================================
* 2011-5-25 9:56:37  HUANGXIN create this file, implements the fundemental interface;
**************************************************************************************************************
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/preempt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <asm/system.h>

#include "sun4i_cedar.h"

#define DRV_VERSION "0.01alpha"

#define CHIP_VERSION_F23

#ifndef CEDARDEV_MAJOR
#define CEDARDEV_MAJOR (150)
#endif
#ifndef CEDARDEV_MINOR
#define CEDARDEV_MINOR (0)
#endif

#undef cdebug
#ifdef CEDAR_DEBUG
#  define cdebug(fmt, args...) printk( KERN_DEBUG "[cedar]: " fmt, ## args)
#else
#  define cdebug(fmt, args...)
#endif

int g_dev_major = CEDARDEV_MAJOR;
int g_dev_minor = CEDARDEV_MINOR;
module_param(g_dev_major, int, S_IRUGO);//S_IRUGO represent that g_dev_major can be read,but canot be write
module_param(g_dev_minor, int, S_IRUGO);

#define VE_IRQ_NO (53)

#define CCMU_VE_PLL_REG      (0xf1c20000)
#define CCMU_AHB_GATE_REG    (0xf1c2000c)
#define CCMU_SDRAM_PLL_REG   (0xf1c20020)
#define CCMU_VE_CLK_REG      (0xf1c20028)
#define CCMU_TS_SS_CLK_REG   (0xf1c2003c)
#define CCMU_AHB_APB_CFG_REG (0xf1c20008)

#define SRAM_MASTER_CFG_REG  (0xf1c01060)
#define CPU_SRAM_PRIORITY_REG    (SRAM_MASTER_CFG_REG + 0x00)
#define VE_SRAM_PRIORITY_REG     (SRAM_MASTER_CFG_REG + 0x20)
#define IMAGE0_SRAM_PRIORITY_REG (SRAM_MASTER_CFG_REG + 0x10)
#define SCALE0_SRAM_PRIORITY_REG (SRAM_MASTER_CFG_REG + 0x14)
#define IMAGE1_SRAM_PRIORITY_REG (SRAM_MASTER_CFG_REG + 0x24)
#define SCALE1_SRAM_PRIORITY_REG (SRAM_MASTER_CFG_REG + 0x28)

void *reserved_mem = (void *)(CONFIG_SW_SYSMEM_RESERVED_BASE|0xc0000000 + 64*1024);//lys modify
int cedarmem_size = CONFIG_SW_SYSMEM_RESERVED_SIZE * 1024 - 64*1024;

struct iomap_para{
	volatile char* regs_macc;
	volatile char* regs_ccmu;
};

static DECLARE_WAIT_QUEUE_HEAD(wait_ve);
struct cedar_dev {
	struct cdev cdev;	             /* char device struct                 */
	struct device *dev;              /* ptr to class device struct         */
	struct class  *class;            /* class for auto create device node  */

	struct semaphore sem;            /* mutual exclusion semaphore         */
	spinlock_t  lock;                /* spinlock to pretect ioctl access   */

	wait_queue_head_t wq;            /* wait queue for poll ops            */

	struct iomap_para iomap_addrs;   /* io remap addrs                     */
	
	u32 irq;                         /* cedar video engine irq number      */
	u32 irq_flag;                    /* flag of video engine irq generated */
	u32 irq_value;                   /* value of video engine irq          */
	u32 irq_has_enable;
};
struct cedar_dev *cedar_devp;

u32 int_sta=0,int_value;

/*
 * Video engine interrupt service routine
 * To wake up ve wait queue
 */
static irqreturn_t VideoEngineInterupt(int irq, void *dev)
{
    volatile int* ve_int_ctrl_reg;    
    volatile int* modual_sel_reg;
    int modual_sel;
   
	struct iomap_para addrs = cedar_devp->iomap_addrs;

    modual_sel_reg = (int *)(addrs.regs_macc + 0);

    modual_sel = *modual_sel_reg;
    modual_sel &= 0xf;
    
	/* estimate Which video format */
    switch (modual_sel)
    {
        case 0: //mpeg124            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0x100 + 0x14);
            break;
        case 1: //h264            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0x200 + 0x20);
            break;
        case 2: //vc1           
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0x300 + 0x24);
            break;
        case 3: //rmvb            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0x400 + 0x14);
            break;
        default:            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0x100 + 0x14);
            printk("macc modual sel not defined!\n");
            break;
    }

    //disable interrupt
    if(modual_sel == 0)
        *ve_int_ctrl_reg = *ve_int_ctrl_reg & (~0x7C);
    else
        *ve_int_ctrl_reg = *ve_int_ctrl_reg & (~0xF);
	
	cedar_devp->irq_value = 0;	
	cedar_devp->irq_flag = 1;	
		
    //any interrupt will wake up wait queue
    wake_up_interruptible(&wait_ve);        //ioctl
	
    return IRQ_HANDLED;
}

/*
 * poll operateion for wait for ve irq
 */
unsigned int cedardev_poll(struct file *filp, struct poll_table_struct *wait) 
{
	int mask = 0;
	struct cedar_dev *devp = filp->private_data;	
    
	poll_wait(filp, &devp->wq, wait);

	if (devp->irq_flag == 1) {
		devp->irq_flag = 0;
		mask |= POLLIN | POLLRDNORM;
	}

	return mask;
}



static DECLARE_WAIT_QUEUE_HEAD(cedarv_wait);
static DEFINE_SPINLOCK(cedarv_queue_lock);

static LIST_HEAD(cedarv_task_list);
struct cedarv_engine_task * cedarv_run_task = NULL;
static struct task_struct *cedarv_tsk;
static int task_done_flag = 0;
static int cedarv_tread(void *arg)
{
	do {
		wait_event_interruptible(cedarv_wait,	!list_empty(&cedarv_task_list));

		if (kthread_should_stop())
			break;

		spin_lock_irq(&cedarv_queue_lock);
		if (!list_empty(&cedarv_task_list)){
			cedarv_run_task = list_entry(cedarv_task_list.next, struct cedarv_engine_task, list);
			list_del(cedarv_task_list.next);
		}
		spin_unlock_irq(&cedarv_queue_lock);

		if ( cedarv_run_task != NULL ) {

			up(&cedarv_run_task->mutex_lock);
			
			task_done_flag = sleep_on_timeout(&cedarv_run_task->wait,
					cedarv_run_task->t.timeout);

			kfree(cedarv_run_task);
			cedarv_run_task = NULL;
		}
	} while (1);

	return 0;
}

void cedardev_insert_task(struct cedarv_engine_task* new_task)
{	
	struct cedarv_engine_task *task_entry;

	list_for_each_entry(task_entry, &cedarv_task_list, list) {
		if (task_entry->t.task_prio < new_task->t.task_prio) {
			break;
		}
	}
	list_add(&new_task->list, task_entry->list.prev);
}

int cedardev_del_task(int task_id)
{
	if(1 == task_done_flag)
		return 1;
	
	if (task_id == cedarv_run_task->t.task_id) {
		wake_up(&cedarv_run_task->wait);
		return 0;
	}
	return -1;
}
int cedardev_check_delay(int check_prio)
{
	struct cedarv_engine_task *task_entry;
	int timeout_total = 0;
	list_for_each_entry(task_entry, &cedarv_task_list, list) {
	if (task_entry->t.task_prio < check_prio)
		break;		
	timeout_total = timeout_total + task_entry->t.frametime;		
	}
	return timeout_total;
}

static unsigned int g_ctx_reg0;
static void save_context()
{
	g_ctx_reg0 = readl(0xf1c20e00);
}

static void restore_context()
{
	writel(g_ctx_reg0, 0xf1c20e00);
}
//#define dbg_print printk

/*
 * ioctl function
 * including : wait video engine done,
 *             AVS Counter control,
 *             Physical memory control,
 *             module clock/freq control.
 *				cedar engine
 */ 
long cedardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long   ret = 0;
    unsigned int v; 
    unsigned long karg;	
	spinlock_t *lock;
	struct cedar_dev *devp;
	struct cedarv_engine_task *task_ptr;
	ret = 0;
	devp = filp->private_data;
	lock = &devp->lock;
	
    switch (cmd)
    {
    	case IOCTL_ENGINE_REQ:
		task_ptr = kmalloc(sizeof(struct cedarv_engine_task), GFP_KERNEL);
		if(!task_ptr){
			printk("get mem for IOCTL_ENGINE_REQ\n");
			return PTR_ERR(task_ptr);
		}
		if(copy_from_user((void*)task_ptr,(void __user*)arg, sizeof(struct __cedarv_task))){
			printk("IOCTL_ENGINE_REQ copy_from_user fail\n");
			return -EFAULT;
		}

		init_MUTEX_LOCKED(&task_ptr->mutex_lock);

		cedardev_insert_task(task_ptr);

		down(&task_ptr->mutex_lock);
		return task_done_flag ? 0 : 1;
		break;

    	case IOCTL_ENGINE_REL:
		if (copy_from_user(&karg, (void *)arg, sizeof(unsigned long))){
			printk("IOCTL_ENGINE_REL,copy_from_user fail\n");
			return -EFAULT;
		}		
		ret = cedardev_del_task(karg);//karg是传递过来的id号
		return ret;
		break;
		
		case IOCTL_ENGINE_CHECK_DELAY:
		if(copy_from_user(&karg,(void *)arg, sizeof(unsigned long))){
			printk("IOCTL_ENGINE_CHECK_DELAY copy_from_user fail\n");
			return -EFAULT;
		}
		ret = cedardev_check_delay(karg);//karg是传递过来的优先级
		return ret;
		break;
		
        case IOCTL_WAIT_VE: 
            wait_event_interruptible(wait_ve, cedar_devp->irq_flag);
	        cedar_devp->irq_flag = 0;	
			return cedar_devp->irq_value;
		
		case IOCTL_ENABLE_VE: 
            v = readl(CCMU_VE_CLK_REG);   
			v |= (0x1 << 7);              // Do not gate the speccial clock for DE
			writel(v, CCMU_VE_CLK_REG);	
			break;
			
		case IOCTL_DISABLE_VE: 
            v = readl(CCMU_VE_CLK_REG);   
			v &= ~(0x1 << 7);             // Gate the speccial clock for DE
			writel(v, CCMU_VE_CLK_REG);	
			break;
		case IOCTL_RESET_VE:				
		{			
			/*reset ve*/
				v = readl(0xf1c2013c);
				v &= ~(1<<0);
				writel(v,0xf1c2013c);

				v = readl(0xf1c2013c);
				v |= (1<<0);
				writel(v,0xf1c2013c);		
		}
		break;
		case IOCTL_SET_VE_FREQ:
			/* VE Clock Setting */
			v = readl(CCMU_VE_PLL_REG);
			v |= (0x1 << 15);             // Enable VE PLL;
			v &= ~(0x1 << 12);            // Disable ByPass;
			v |= (arg - 30) / 6;                
			writel(v, CCMU_VE_PLL_REG);
			break;
		
        case IOCTL_GETVALUE_AVS2:
			/* Return AVS1 counter value */
            return readl(devp->iomap_addrs.regs_ccmu + 0xc88);

        case IOCTL_ADJUST_AVS2:	    
        {	        
            int arg_s = (int)arg;		
            int temp;	        
            save_context();
            v = readl(devp->iomap_addrs.regs_ccmu + 0xc44);				        
            temp = v & 0xffff0000;		
            temp =temp + temp*arg_s/100;                
            v = (temp & 0xffff0000) | (v&0x0000ffff);                
            writel(v, devp->iomap_addrs.regs_ccmu + 0xc44);
            restore_context();
            break;
        }
        
        case IOCTL_CONFIG_AVS2:
        	save_context();
			/* Set AVS counter divisor */
            v = readl(devp->iomap_addrs.regs_ccmu + 0xc8c);
            v = 239 << 16 | (v & 0xffff);
            writel(v, devp->iomap_addrs.regs_ccmu + 0xc8c);
			
			/* Enable AVS_CNT1 and Pause it */
            v = readl(devp->iomap_addrs.regs_ccmu + 0xc80);
            v |= 1 << 9 | 1 << 1;
            writel(v, devp->iomap_addrs.regs_ccmu + 0xc80);

			/* Set AVS_CNT1 init value as zero  */
            writel(0, devp->iomap_addrs.regs_ccmu + 0xc88);

			v = readl(devp->iomap_addrs.regs_ccmu + 0x144);
            v |= 1<<31;
            writel(v, devp->iomap_addrs.regs_ccmu + 0x144);

			restore_context();
            break;
            
        case IOCTL_RESET_AVS2:
            /* Set AVS_CNT1 init value as zero */
            save_context();
            writel(0, devp->iomap_addrs.regs_ccmu + 0xc88);
            restore_context();
            break;
            
        case IOCTL_PAUSE_AVS2:
            /* Pause AVS_CNT1 */
            save_context();
            v = readl(devp->iomap_addrs.regs_ccmu + 0xc80);
            v |= 1 << 9;
            writel(v, devp->iomap_addrs.regs_ccmu + 0xc80);
            restore_context();
            break;
            
        case IOCTL_START_AVS2:
		    /* Start AVS_CNT1 : do not pause */
		    save_context();
            v = readl(devp->iomap_addrs.regs_ccmu + 0xc80);
            v &= ~(1 << 9);
            writel(v, devp->iomap_addrs.regs_ccmu + 0xc80);
            restore_context();
            break;

        case IOCTL_GET_ENV_INFO:
        {
            struct cedarv_env_infomation env_info;
            env_info.phymem_start = (unsigned int)0xc8000000;//reserved_mem;
	        env_info.address_macc = (unsigned int)cedar_devp->iomap_addrs.regs_macc;
            env_info.phymem_total_size = 0x4000000;//cedarmem_size;
            if (copy_to_user((char *)arg, &env_info, sizeof(struct cedarv_env_infomation)))                
                return -EFAULT;                           
        }
        break;
        default:
        break;
    }    
    return ret;
}

static int cedardev_open(struct inode *inode, struct file *filp)
{   
	struct cedar_dev *devp;
	devp = container_of(inode->i_cdev, struct cedar_dev, cdev);
	filp->private_data = devp;
	
	if (down_interruptible(&devp->sem)) {
		return -ERESTARTSYS;
	}
	
	/* init other resource here */
    devp->irq_flag = 0;

	up(&devp->sem);

	nonseekable_open(inode, filp);
	return 0;
}

static int cedardev_release(struct inode *inode, struct file *filp)
{   
	struct cedar_dev *devp;
	
	devp = filp->private_data;

	if (down_interruptible(&devp->sem)) {
		return -ERESTARTSYS;
	}
	
	/* release other resource here */
    devp->irq_flag = 1;

	up(&devp->sem);
	return 0;
}

void cedardev_vma_open(struct vm_area_struct *vma)
{
	return;
} 

void cedardev_vma_close(struct vm_area_struct *vma)
{
	return;
}

static struct vm_operations_struct cedardev_remap_vm_ops = {
    .open  = cedardev_vma_open,
    .close = cedardev_vma_close,
};

static int cedardev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long temp_pfn;
    unsigned int  VAddr;
	struct iomap_para addrs;

	unsigned int io_ram = 0;
    VAddr = vma->vm_pgoff << 12;

	addrs = cedar_devp->iomap_addrs;

    if(VAddr == (unsigned int)addrs.regs_macc) {
        temp_pfn = MACC_REGS_BASE >> 12;
        io_ram = 1;
    } else {
        temp_pfn = (__pa(vma->vm_pgoff << 12))>>12;
        io_ram = 0;
    }

    if (io_ram == 0) {   
        /* Set reserved and I/O flag for the area. */
        vma->vm_flags |= VM_RESERVED | VM_IO;

        /* Select uncached access. */
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

        if (remap_pfn_range(vma, vma->vm_start, temp_pfn,
                            vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
            return -EAGAIN;
        }
    } else {
        /* Set reserved and I/O flag for the area. */
        vma->vm_flags |= VM_RESERVED | VM_IO;
        /* Select uncached access. */
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

        if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
                               vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
            return -EAGAIN;
        }
    }
    
    vma->vm_ops = &cedardev_remap_vm_ops;
    cedardev_vma_open(vma);
    
    return 0; 
} 

static struct file_operations cedardev_fops = {
    .owner   = THIS_MODULE,
    .mmap    = cedardev_mmap,
	.poll    = cedardev_poll,
    .open    = cedardev_open,
    .release = cedardev_release,
	.llseek  = no_llseek,
    .unlocked_ioctl   = cedardev_ioctl,
};

static int __init cedardev_init(void)
{
	int ret;
	int devno;
	unsigned int val;
	dev_t dev = 0;
	
	printk("the func: %s, the Line: %d\n", __func__, __LINE__);
	/*register or alloc the device number.*/
	if (g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);	
		ret = register_chrdev_region(dev, 1, "cedar_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "cedar_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}

	 printk("[cedar dev]: install start..1. \n");

	if (ret < 0) {
		printk(KERN_WARNING "cedar_dev: can't get major %d\n", g_dev_major);
		return ret;
	}
	
	printk("the func: %s, the Line: %d\n", __func__, __LINE__);
	cedar_devp = kmalloc(sizeof(struct cedar_dev), GFP_KERNEL);
	if (cedar_devp == NULL) {
		printk("malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	
	printk("[cedar dev]: install start..2. \n");
	memset(cedar_devp, 0, sizeof(struct cedar_dev));
	cedar_devp->irq = VE_IRQ_NO;

	init_MUTEX(&cedar_devp->sem);
	init_waitqueue_head(&cedar_devp->wq);	

	memset(&cedar_devp->iomap_addrs, 0, sizeof(struct iomap_para));

    ret = request_irq(VE_IRQ_NO, VideoEngineInterupt, 0, "cedar_dev", NULL);
    if (ret < 0) {
        printk("request irq err\n");
        return -EINVAL;
    }
	
	printk("the func: %s, the Line: %d\n", __func__, __LINE__);    	
    printk("[cedar dev]: install start..3. \n");
    /* map for macc io space */
    cedar_devp->iomap_addrs.regs_macc = ioremap(MACC_REGS_BASE, 4096);
    if (!cedar_devp->iomap_addrs.regs_macc){
        printk("cannot map region for macc");
    }
	/* map for ccmu io space */
	cedar_devp->iomap_addrs.regs_ccmu = ioremap(CCMU_REGS_BASE, 4096); //1024 ?
	if (!cedar_devp->iomap_addrs.regs_ccmu){        
		printk("cannot map region for ccmu");        
	} 
	printk("%s %d\n", __FUNCTION__, __LINE__);

	//macc PLL
	val = readl(0xf1c20018);
	val &= 0x7ffc0000;
	val |= 1<<31;
	val |= (0x0)<<16; //Pb
	val |= (0x3)<<8; //Nb
	val |= (0x3)<<4; //Kb
	val |= (0x0)<<0; //Mb
	writel(val,0xf1c20018);
	
	//Active AHB bus to MACC
	val = readl(0xf1c20064);
	val |= (1<<0);
	writel(val,0xf1c20064);
	
	//Power on and release reset ve
	val = readl(0xf1c2013c);
	val &= ~(1<<0); //reset ve
	writel(val,0xf1c2013c);

	val = readl(0xf1c2013c);
	val |= (1<<31);
	val |= (1<<0);
	writel(val,0xf1c2013c);
	
	//gate on the bus to SDRAM
	val = readl(0xf1c20100);
	val |= (1<<29);
	val |= (1<<0);
	writel(val,0xf1c20100);
	
	//VE_SRAM mapping to AC320
	val = readl(0xf1c00000);
	val &= 0x80000000;
	writel(val,0xf1c00000);
	
	//remapping SRAM to MACC for codec test
	val = readl(0xf1c00000);
	val |= 0x7fffffff;
	writel(val,0xf1c00000);
	
	/* Create char device */
	printk("the func: %s, the Line: %d\n", __func__, __LINE__);
	devno = MKDEV(g_dev_major, g_dev_minor);
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	cedar_devp->cdev.ops = &cedardev_fops;
	ret = cdev_add(&cedar_devp->cdev, devno, 1);
	if (ret) {
		printk(KERN_NOTICE "Err:%d add cedardev", ret);
	}
    printk("[cedar dev]: dev install successfully \n");
    
    cedar_devp->class = class_create(THIS_MODULE, "cedar_dev");
    cedar_devp->dev   = device_create(cedar_devp->class, NULL, devno, NULL, "cedar_dev");
	printk("the func: %s, the Line: %d\n", __func__, __LINE__);    	

	cedarv_tsk = kthread_create(cedarv_tread, NULL, "cedarv_tread");
	if (IS_ERR(cedarv_tsk)) {
		ret = PTR_ERR(cedarv_tsk);
		cedarv_tsk = NULL;
		printk("create_thread for the cedar_engine error\n");
		return ret;
	}
	wake_up_process(cedarv_tsk);
	return 0;
}
module_init(cedardev_init);


static void __exit cedardev_exit(void)
{
	dev_t dev;
	dev = MKDEV(g_dev_major, g_dev_minor);

    free_irq(VE_IRQ_NO, NULL);
	iounmap(cedar_devp->iomap_addrs.regs_macc);
	iounmap(cedar_devp->iomap_addrs.regs_ccmu);

	/* Destroy char device */
	if (cedar_devp) {
		cdev_del(&cedar_devp->cdev);
		device_destroy(cedar_devp->class, dev);
		class_destroy(cedar_devp->class);
	}

	unregister_chrdev_region(dev, 1);	

	if (cedar_devp) {
		kfree(cedar_devp);
	}
}
module_exit(cedardev_exit);

MODULE_AUTHOR("Soft-Allwinner");
MODULE_DESCRIPTION("User mode CEDAR device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
