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
#  define cdebug(fmt, args...) pr_debug( KERN_DEBUG "[cedar]: " fmt, ## args)
#else
#  define cdebug(fmt, args...)
#endif

int g_dev_major = CEDARDEV_MAJOR;
int g_dev_minor = CEDARDEV_MINOR;
module_param(g_dev_major, int, S_IRUGO);//S_IRUGO represent that g_dev_major can be read,but canot be write
module_param(g_dev_minor, int, S_IRUGO);

#ifdef CHIP_VERSION_F23
#define VE_IRQ_NO (53)
#else
#define VE_IRQ_NO (48)
#endif
#ifdef CHIP_VERSION_F23
struct clk *ve_moduleclk, *ve_pll4clk, *ahb_veclk, *dram_veclk, *avs_moduleclk, *hosc_clk;
static unsigned long suspend_pll4clk = 0;
static unsigned long gpu_pll4clk = 0;
static unsigned int cedar_count = 0;
#else
#define CCMU_VE_PLL_REG      (0xf1c20000)
#define CCMU_VE_CLK_REG      (0xf1c20028)
#endif
void *reserved_mem = (void *)( (CONFIG_SW_SYSMEM_RESERVED_BASE | 0xC0000000) + 64*1024);
int cedarmem_size = CONFIG_SW_SYSMEM_RESERVED_SIZE * 1024 - 64*1024;

struct iomap_para{
	volatile char* regs_macc;
	#ifdef CHIP_VERSION_F23
	volatile char* regs_avs;
	#else
	volatile char* regs_ccmu;
	#endif
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
        case 0xa: //isp            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0xa00 + 0x08);
            break;
        case 0xb: //avc enc            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0xb00 + 0x14);
            break; 
        default:            
            ve_int_ctrl_reg = (int *)(addrs.regs_macc + 0x100 + 0x14);
            pr_debug("macc modual sel not defined!\n");
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

#ifdef CHIP_VERSION_F23
static unsigned int g_ctx_reg0;
static void save_context()
{
	g_ctx_reg0 = readl(0xf1c20e00);
}

static void restore_context()
{
	writel(g_ctx_reg0, 0xf1c20e00);
}
#else
	#define save_context() 
	#define restore_context() 
#endif

#ifdef CHIP_VERSION_F23
short VEPLLTable[][6] = 
{
	//set, actual, Nb, Kb, Mb, Pb
	{ 60,  60,  5,  2,  2,  1},
	{ 90,  90,  5,  2,  0,  2},
	{120, 120,  5,  2,  2,  0},
	{150, 150, 25,  0,  0,  2},
	{180, 180,  5,  2,  0,  1},
	{216, 216,  6,  2,  0,  1},
	{240, 240,  5,  3,  0,  1},
	{270, 270, 15,  2,  0,  2},
	{300, 300, 25,  0,  0,  1},
	{330, 336,  7,  1,  0,  0},
	{360, 360,  5,  2,  0,  0},
	{384, 384,  4,  3,  0,  0},
	{402, 400, 25,  1,  2,  0},
	{420, 416, 13,  3,  2,  0},
	{444, 448, 14,  3,  2,  0},
	{456, 456, 19,  0,  0,  0},
	{468, 468, 13,  2,  0,  1},
	{480, 480,  5,  3,  0,  0},
	{492, 496, 31,  1,  2,  0},
};
#endif

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
			pr_debug("get mem for IOCTL_ENGINE_REQ\n");
			return PTR_ERR(task_ptr);
		}
		#ifdef USE_TASK
		if(copy_from_user((void*)task_ptr,(void __user*)arg, sizeof(struct __cedarv_task))){
			pr_debug("IOCTL_ENGINE_REQ copy_from_user fail\n");
			return -EFAULT;
		}
		#endif
		gpu_pll4clk = clk_get_rate(ve_pll4clk);
		if(cedar_count == 0){			
			cedar_count++;
			ve_pll4clk = clk_get(NULL,"ve_pll");
			if (-1 == clk_enable(ve_pll4clk)) {
				pr_debug("ve_pll4clk failed; \n");
			}
			clk_set_rate(ve_pll4clk, 240000000);	
			
			/* getting ahb clk for ve!(macc) */
			ahb_veclk = clk_get(NULL,"ahb_ve");		
			if (-1 == clk_enable(ahb_veclk)) {
				pr_debug("ahb_veclk failed; \n");
			}
		
			ve_moduleclk = clk_get(NULL,"ve");	
			if (clk_set_parent(ve_moduleclk, ve_pll4clk)) {
				pr_debug("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
			}
			if (clk_set_parent(ve_moduleclk, ve_pll4clk)) {
				pr_debug("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
			}
			if (-1 == clk_enable(ve_moduleclk)) {
				pr_debug("ve_moduleclk failed; \n");
			}	
			if(clk_reset(ve_moduleclk, 1)){//先处于复位状态（不工作状态）
				pr_debug("reset ve_moduleclk failed!!!\n");
			}
			if(clk_reset(ve_moduleclk, 0)){//再设置处于非复位状态（工作状态）
				pr_debug("reset ve_moduleclk failed!!!\n");
			}				
			/*geting dram clk for ve!*/
			dram_veclk = clk_get(NULL, "sdram_ve");
		
			if (-1 == clk_enable(dram_veclk)) {
				pr_debug("dram_veclk failed; \n");
			}
			
		    hosc_clk = clk_get(NULL,"hosc");	
		    avs_moduleclk = clk_get(NULL,"avs");	
			if (clk_set_parent(avs_moduleclk, hosc_clk)) {
				pr_debug("set parent of avs_moduleclk to hosc_clk failed!\n");		
			}
			if (-1 == clk_enable(avs_moduleclk)) {
				pr_debug("ve_moduleclk failed; \n");
			}
			/*for clk test*/
			pr_debug("[cedar request reg]\n");
			pr_debug("PLL4 CLK:0xf1c20018 is:%x\n", *(volatile int *)0xf1c20018);
			pr_debug("AHB CLK:0xf1c20064 is:%x\n", *(volatile int *)0xf1c20064);
			pr_debug("VE CLK:0xf1c2013c is:%x\n", *(volatile int *)0xf1c2013c);
			pr_debug("SDRAM CLK:0xf1c20100 is:%x\n", *(volatile int *)0xf1c20100);		
			pr_debug("AVS CLK:0xf1c20144 is:%x\n", *(volatile int *)0xf1c20144);	
			pr_debug("[cedar request reg]\n");
		}else{
			cedar_count++;
		}
		#ifdef USE_TASK
		printk("sbsbsbsbbb%s,%d\n", __func__,__LINE__);
		init_MUTEX_LOCKED(&task_ptr->mutex_lock);

		cedardev_insert_task(task_ptr);

		down(&task_ptr->mutex_lock);
		#endif
		return task_done_flag ? 0 : 1;
		break;

    	case IOCTL_ENGINE_REL:
    		#ifdef USE_TASK
		if (copy_from_user(&karg, (void *)arg, sizeof(unsigned long))){
			pr_debug("IOCTL_ENGINE_REL,copy_from_user fail\n");
			return -EFAULT;
		}		
		ret = cedardev_del_task(karg);//karg是传递过来的id号
		#endif
		if(cedar_count==1){			
			clk_disable(ve_moduleclk);	
			clk_put(ve_moduleclk);	
			
			clk_disable(dram_veclk);
			clk_put(dram_veclk);
			
			clk_disable(ahb_veclk);
			clk_put(ahb_veclk);
			
			clk_put(ve_pll4clk);
			
			clk_disable(avs_moduleclk);	
			clk_put(avs_moduleclk);
			cedar_count--;
						/*for clk test*/
			pr_debug("[cedar release reg]\n");
			pr_debug("PLL4 CLK:0xf1c20018 is:%x\n", *(volatile int *)0xf1c20018);
			pr_debug("AHB CLK:0xf1c20064 is:%x\n", *(volatile int *)0xf1c20064);
			pr_debug("VE CLK:0xf1c2013c is:%x\n", *(volatile int *)0xf1c2013c);
			pr_debug("SDRAM CLK:0xf1c20100 is:%x\n", *(volatile int *)0xf1c20100);	
			pr_debug("AVS CLK:0xf1c20144 is:%x\n", *(volatile int *)0xf1c20144);	
			pr_debug("[cedar release reg]\n");
		}else if(cedar_count > 1){
			cedar_count--;
		}
		clk_set_rate(ve_pll4clk,gpu_pll4clk);
		return ret;
		break;
		
		case IOCTL_ENGINE_CHECK_DELAY:
		if(copy_from_user(&karg,(void *)arg, sizeof(unsigned long))){
			pr_debug("IOCTL_ENGINE_CHECK_DELAY copy_from_user fail\n");
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
#ifdef CHIP_VERSION_F20
            v = readl(CCMU_VE_CLK_REG);   
			v |= (0x1 << 7);              // Do not gate the speccial clock for DE
			writel(v, CCMU_VE_CLK_REG);	
#else
            clk_enable(ve_moduleclk);
//			v = readl(0xf1c2013c);
//			v |= (1<<31 | 1<<0);
//			writel(v,0xf1c2013c);
#endif			
			break;
			
		case IOCTL_DISABLE_VE: 
#ifdef CHIP_VERSION_F20			
            v = readl(CCMU_VE_CLK_REG);   
			v &= ~(0x1 << 7);             // Gate the speccial clock for DE
			writel(v, CCMU_VE_CLK_REG);	
#else
//			v = readl(0xf1c2013c);
//			v &= ~(1<<31 | 1<<0);
//			writel(v,0xf1c2013c);
			clk_disable(ve_moduleclk);
#endif			
			break;
			
		case IOCTL_RESET_VE:				
#ifdef CHIP_VERSION_F20
			v = readl(CCMU_SDRAM_PLL_REG);
			v &= ~(0x1 << 24);            
			writel(v, CCMU_SDRAM_PLL_REG);
			//----------------------------
			v = readl(CCMU_VE_CLK_REG);   
			v &= ~(0x1 << 5);             // Reset VE
			writel(v, CCMU_VE_CLK_REG);
			v |= (0x1 << 5);              // Reset VE
			writel(v, CCMU_VE_CLK_REG);
			//-----------------------------
			v = readl(CCMU_SDRAM_PLL_REG);
			v |= (0x1 << 24);            
			writel(v, CCMU_SDRAM_PLL_REG);
#else		            
            clk_disable(dram_veclk);
            clk_reset(ve_moduleclk, 1);
            clk_reset(ve_moduleclk, 0);
            clk_enable(dram_veclk);
#endif		
		break;
		
		case IOCTL_SET_VE_FREQ:
#ifdef CHIP_VERSION_F20
			v = readl(CCMU_VE_PLL_REG);
			v |= (0x1 << 15);             // Enable VE PLL;
			v &= ~(0x1 << 12);            // Disable ByPass;
			v |= (arg - 30) / 6;                
			writel(v, CCMU_VE_PLL_REG);
#else
			{
				int freq = (int)arg;
				int fq;
				int ret = 0;
				if(freq > 492 || freq < 60){
					return ret;
				}
				for(fq=0;;fq++){
					if(freq <= VEPLLTable[fq][0])
						break;
				}
				pr_debug("Attention: set ve freq to %d MHz\n", VEPLLTable[fq][0]);				
                clk_set_rate(ve_pll4clk, 1000000 * VEPLLTable[fq][0]);   
                /*clk_get_rate used to test the ve_pll4clk rate*/  
                ret = clk_get_rate(ve_pll4clk);           
                pr_debug("After set rate, the ve_pll4clk rate is:%d\n", ret);
			}

#endif			
			break;
		
        case IOCTL_GETVALUE_AVS2:
			/* Return AVS1 counter value */
            return readl(cedar_devp->iomap_addrs.regs_avs + 0x88);

        case IOCTL_ADJUST_AVS2:	    
        {	        
            int arg_s = (int)arg;		
            int temp;	        
            save_context();
            v = readl(cedar_devp->iomap_addrs.regs_avs + 0x8c);				        
            temp = v & 0xffff0000;		
            temp =temp + temp*arg_s/100;                
            v = (temp & 0xffff0000) | (v&0x0000ffff);   
            pr_debug("Kernel AVS ADJUST Print: 0x%x\n", v);             
            writel(v, cedar_devp->iomap_addrs.regs_avs + 0x8c);
            restore_context();
            break;
        }
        
        case IOCTL_CONFIG_AVS2:
        	save_context();
			/* Set AVS counter divisor */
            v = readl(cedar_devp->iomap_addrs.regs_avs + 0x8c);
            v = 239 << 16 | (v & 0xffff);
            writel(v, cedar_devp->iomap_addrs.regs_avs + 0x8c);
			
			/* Enable AVS_CNT1 and Pause it */
            v = readl(cedar_devp->iomap_addrs.regs_avs + 0x80);
            v |= 1 << 9 | 1 << 1;
            writel(v, cedar_devp->iomap_addrs.regs_avs + 0x80);

			/* Set AVS_CNT1 init value as zero  */
            writel(0, cedar_devp->iomap_addrs.regs_avs + 0x88);

#ifdef CHIP_VERSION_F20
            v = readl(devp->iomap_addrs.regs_ccmu + 0x040);
            v |= 1<<8;
            writel(v, devp->iomap_addrs.regs_ccmu + 0x040);
#endif
			restore_context();
            break;
            
        case IOCTL_RESET_AVS2:
            /* Set AVS_CNT1 init value as zero */
            save_context();
            writel(0, cedar_devp->iomap_addrs.regs_avs + 0x88);
            restore_context();
            break;
            
        case IOCTL_PAUSE_AVS2:
            /* Pause AVS_CNT1 */
            save_context();
            v = readl(cedar_devp->iomap_addrs.regs_avs + 0x80);
            v |= 1 << 9;
            writel(v, cedar_devp->iomap_addrs.regs_avs + 0x80);
            restore_context();
            break;
            
        case IOCTL_START_AVS2:
		    /* Start AVS_CNT1 : do not pause */
		    save_context();
            v = readl(cedar_devp->iomap_addrs.regs_avs + 0x80);
            v &= ~(1 << 9);
            writel(v, cedar_devp->iomap_addrs.regs_avs + 0x80);
            restore_context();
            break;

        case IOCTL_GET_ENV_INFO:
        {
            struct cedarv_env_infomation env_info;
            env_info.phymem_start = (unsigned int)reserved_mem;
            env_info.phymem_total_size = cedarmem_size;
	        env_info.address_macc = (unsigned int)cedar_devp->iomap_addrs.regs_macc;
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

static int snd_sw_cedar_suspend(struct platform_device *pdev,pm_message_t state)
{	
	pr_debug("enter snd_sw_cedar_suspend:%s,%d\n",__func__,__LINE__);
	if(cedar_count){
	suspend_pll4clk = clk_get_rate(ve_pll4clk);
	clk_disable(ve_moduleclk);	
	clk_put(ve_moduleclk);	
	clk_put(dram_veclk);
	clk_put(ahb_veclk);
	clk_put(ve_pll4clk);
	clk_disable(avs_moduleclk);	
	clk_put(avs_moduleclk);	
	}
		/*for clk test*/
	pr_debug("[cedar suspend reg]\n");
	pr_debug("PLL4 CLK:0xf1c20018 is:%x\n", *(volatile int *)0xf1c20018);
	pr_debug("AHB CLK:0xf1c20064 is:%x\n", *(volatile int *)0xf1c20064);
	pr_debug("VE CLK:0xf1c2013c is:%x\n", *(volatile int *)0xf1c2013c);
	pr_debug("SDRAM CLK:0xf1c20100 is:%x\n", *(volatile int *)0xf1c20100);
	pr_debug("AVS CLK:0xf1c20144 is:%x\n", *(volatile int *)0xf1c20144);	
	pr_debug("[cedar suspend reg]\n");
	return 0;
}

static int snd_sw_cedar_resume(struct platform_device *pdev)
{
	pr_debug("enter snd_sw_cedar_resume:%s,%d\n",__func__,__LINE__);
	if(cedar_count){
		ve_pll4clk = clk_get(NULL,"ve_pll");
		if (-1 == clk_enable(ve_pll4clk)) {
			pr_debug("ve_pll4clk failed; \n");
		}
		clk_set_rate(ve_pll4clk, suspend_pll4clk);	
		
		/* getting ahb clk for ve!(macc) */
		ahb_veclk = clk_get(NULL,"ahb_ve");		
		if (-1 == clk_enable(ahb_veclk)) {
			pr_debug("ahb_veclk failed; \n");
		}
	
		ve_moduleclk = clk_get(NULL,"ve");	
		if (clk_set_parent(ve_moduleclk, ve_pll4clk)) {
			pr_debug("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
		}
		if (clk_set_parent(ve_moduleclk, ve_pll4clk)) {
			pr_debug("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
		}
		if (-1 == clk_enable(ve_moduleclk)) {
			pr_debug("ve_moduleclk failed; \n");
		}	
		if(clk_reset(ve_moduleclk, 1)){
			pr_debug("reset ve_moduleclk failed!!!\n");
		}
		if(clk_reset(ve_moduleclk, 0)){
			pr_debug("reset ve_moduleclk failed!!!\n");
		}
		/*geting dram clk for ve!*/
		dram_veclk = clk_get(NULL, "sdram_ve");
	
		if (-1 == clk_enable(dram_veclk)) {
			pr_debug("dram_veclk failed; \n");
		}
		
	    hosc_clk = clk_get(NULL,"hosc");	
	    avs_moduleclk = clk_get(NULL,"avs");	
		if (clk_set_parent(avs_moduleclk, hosc_clk)) {
			pr_debug("set parent of avs_moduleclk to hosc_clk failed!\n");		
		}
		if (-1 == clk_enable(avs_moduleclk)) {
			pr_debug("ve_moduleclk failed; \n");
		}		
	}
		/*for clk test*/
	pr_debug("PLL4 CLK:0xf1c20018 is:%x\n", *(volatile int *)0xf1c20018);
	pr_debug("AHB CLK:0xf1c20064 is:%x\n", *(volatile int *)0xf1c20064);
	pr_debug("VE CLK:0xf1c2013c is:%x\n", *(volatile int *)0xf1c2013c);
	pr_debug("SDRAM CLK:0xf1c20100 is:%x\n", *(volatile int *)0xf1c20100);
	pr_debug("SRAM:0xf1c00000 is:%x\n", *(volatile int *)0xf1c00000);
	pr_debug("AVS CLK:0xf1c20144 is:%x\n", *(volatile int *)0xf1c20144);	
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

/*data relating*/
static struct platform_device sw_device_cedar = {
	.name = "sun4i-cedar",   	   
};

/*method relating*/
static struct platform_driver sw_cedar_driver = {
#ifdef CONFIG_PM
	.suspend	= snd_sw_cedar_suspend,
	.resume		= snd_sw_cedar_resume,
#endif
	.driver		= {
		.name	= "sun4i-cedar",
	},
};

static int __init cedardev_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	unsigned int val;
	dev_t dev = 0;
	
	printk("[cedar dev]: install start!!!\n");
	if((platform_device_register(&sw_device_cedar))<0)
		return err;

	if ((err = platform_driver_register(&sw_cedar_driver)) < 0)
		return err;
	/*register or alloc the device number.*/
	if (g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);	
		ret = register_chrdev_region(dev, 1, "cedar_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "cedar_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		pr_debug(KERN_WARNING "cedar_dev: can't get major %d\n", g_dev_major);
		return ret;
	}
	
	cedar_devp = kmalloc(sizeof(struct cedar_dev), GFP_KERNEL);
	if (cedar_devp == NULL) {
		pr_debug("malloc mem for cedar device err\n");
		return -ENOMEM;
	}		
	memset(cedar_devp, 0, sizeof(struct cedar_dev));
	cedar_devp->irq = VE_IRQ_NO;

	init_MUTEX(&cedar_devp->sem);
	init_waitqueue_head(&cedar_devp->wq);	

	memset(&cedar_devp->iomap_addrs, 0, sizeof(struct iomap_para));

    ret = request_irq(VE_IRQ_NO, VideoEngineInterupt, 0, "cedar_dev", NULL);
    if (ret < 0) {
        pr_debug("request irq err\n");
        return -EINVAL;
    }
	/* map for macc io space */
    cedar_devp->iomap_addrs.regs_macc = ioremap(MACC_REGS_BASE, 4096);
    if (!cedar_devp->iomap_addrs.regs_macc){
        pr_debug("cannot map region for macc");
    }
	/* map for ccmu io space */
	#ifdef CHIP_VERSION_F20
	cedar_devp->iomap_addrs.regs_ccmu = ioremap(CCMU_REGS_BASE, 1024); //1024 ?
	if (!cedar_devp->iomap_addrs.regs_ccmu){        
		pr_debug("cannot map region for ccmu");        
	} 
	pr_debug("%s %d\n", __FUNCTION__, __LINE__);
	writel(0x00000309, 0xf1c01060); 
	writel(0x00000309, 0xf1c01064); 
	writel(0x00000301, 0xf1c01068); 
	writel(0x00000305, 0xf1c0106c); 
	writel(0x00030701, 0xf1c01070); 
	writel(0x00031001, 0xf1c01074); 
	writel(0x00031001, 0xf1c01078); 
	writel(0x00030701, 0xf1c0107c); 
	writel(0x00031001, 0xf1c01080); 
	writel(0x00030701, 0xf1c01084); 
	writel(0x00031001, 0xf1c01088); 
	writel(0x00030701, 0xf1c0108c); 
	writel(0x00000000, 0xf1c01090); 
	writel(0x00031001, 0xf1c01094); 
	writel(0x00031001, 0xf1c01098); 
	writel(0x00000301, 0xf1c0109c); 

	/* VE Clock Setting */
	val = readl(CCMU_VE_PLL_REG);
	val |= (0x1 << 15);             // Enable VE PLL;
	val &= ~(0x1 << 12);            // Disable ByPass;
	//VE freq: 0x16->162; 0x17->168; 0x18->174; 0x19->180
	//         0x1a->186; 0x1b->192; 0x1c->198; 0x1d->204
	//         0x1e->210;
	val |= 0x19;                
	writel(val, CCMU_VE_PLL_REG);
	pr_debug("-----[cedar_dev] set ve clock -> %#x\n", val);

	val = readl(CCMU_AHB_GATE_REG); 
	val |= (0x1 << 15);             // Disable VE AHB clock gating
	writel(val, CCMU_AHB_GATE_REG);

	val = readl(CCMU_VE_CLK_REG);   
	val |= (0x1 << 7);              // Do not gate the speccial clock for DE
	val |= (0x1 << 5);              // Reset VE
	writel(val, CCMU_VE_CLK_REG);

	val = readl(CCMU_SDRAM_PLL_REG);
	val |= (0x1 << 24);             // Do not gate DRAM clock for VD
	writel(val, CCMU_SDRAM_PLL_REG);

#else
    cedar_devp->iomap_addrs.regs_avs = ioremap(AVS_REGS_BASE, 1024);	
    #if 0
	ve_pll4clk = clk_get(NULL,"ve_pll");
	if (-1 == clk_enable(ve_pll4clk)) {
		pr_debug("ve_pll4clk failed; \n");
	}
	clk_set_rate(ve_pll4clk, 288000000);	
	
	/*clk_get_rate used to testing the freq!!!*/
	ret = clk_get_rate(ve_pll4clk);      
	printk("After set rate, the ve_pll4clk rate is:%d\n", ret);
	/* getting ahb clk for ve!(macc) */
	ahb_veclk = clk_get(NULL,"ahb_ve");		
	if (-1 == clk_enable(ahb_veclk)) {
		pr_debug("ahb_veclk failed; \n");
	}

	ve_moduleclk = clk_get(NULL,"ve");	
	if (clk_set_parent(ve_moduleclk, ve_pll4clk)) {
		pr_debug("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
	}
	if (clk_set_parent(ve_moduleclk, ve_pll4clk)) {
		pr_debug("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
	}
	if (-1 == clk_enable(ve_moduleclk)) {
		pr_debug("ve_moduleclk failed; \n");
	}	
	if(clk_reset(ve_moduleclk, 1)){
		pr_debug("reset ve_moduleclk failed!!!\n");
	}
		
	/*geting dram clk for ve!*/
	dram_veclk = clk_get(NULL, "sdram_ve");

	if (-1 == clk_enable(dram_veclk)) {
		pr_debug("dram_veclk failed; \n");
	}
	
    hosc_clk = clk_get(NULL,"hosc");	
    avs_moduleclk = clk_get(NULL,"avs");	
	if (clk_set_parent(avs_moduleclk, hosc_clk)) {
		pr_debug("set parent of avs_moduleclk to hosc_clk failed!\n");		
	}
	if (-1 == clk_enable(avs_moduleclk)) {
		pr_debug("ve_moduleclk failed; \n");
	}
	#endif
	//VE_SRAM mapping to AC320
	val = readl(0xf1c00000);
	val &= 0x80000000;
	writel(val,0xf1c00000);	
	//remapping SRAM to MACC for codec test
	val = readl(0xf1c00000);
	val |= 0x7fffffff;
	writel(val,0xf1c00000);
	
	/*for clk test*/
	pr_debug("PLL4 CLK:0xf1c20018 is:%x\n", *(volatile int *)0xf1c20018);
	pr_debug("AHB CLK:0xf1c20064 is:%x\n", *(volatile int *)0xf1c20064);
	pr_debug("VE CLK:0xf1c2013c is:%x\n", *(volatile int *)0xf1c2013c);
	pr_debug("SDRAM CLK:0xf1c20100 is:%x\n", *(volatile int *)0xf1c20100);
	pr_debug("SRAM:0xf1c00000 is:%x\n", *(volatile int *)0xf1c00000);
#endif	
	/* Create char device */
	devno = MKDEV(g_dev_major, g_dev_minor);
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	cedar_devp->cdev.ops = &cedardev_fops;
	ret = cdev_add(&cedar_devp->cdev, devno, 1);
	if (ret) {
		pr_debug(KERN_NOTICE "Err:%d add cedardev", ret);
	}   
    cedar_devp->class = class_create(THIS_MODULE, "cedar_dev");
    cedar_devp->dev   = device_create(cedar_devp->class, NULL, devno, NULL, "cedar_dev");

	cedarv_tsk = kthread_create(cedarv_tread, NULL, "cedarv_tread");
	if (IS_ERR(cedarv_tsk)) {
		ret = PTR_ERR(cedarv_tsk);
		cedarv_tsk = NULL;
		pr_debug("create_thread for the cedar_engine error\n");
		return ret;
	}
	wake_up_process(cedarv_tsk);
	pr_debug("[cedar dev]: install end!!!\n");
	return 0;
}
module_init(cedardev_init);

static void __exit cedardev_exit(void)
{
	dev_t dev;
	dev = MKDEV(g_dev_major, g_dev_minor);

    free_irq(VE_IRQ_NO, NULL);
	iounmap(cedar_devp->iomap_addrs.regs_macc);
	#ifdef CHIP_VERSION_F20
	iounmap(cedar_devp->iomap_addrs.regs_ccmu);
	#else
	iounmap(cedar_devp->iomap_addrs.regs_avs);
	#if 0
	clk_disable(ve_moduleclk);	
	clk_put(ve_moduleclk);	
	clk_put(dram_veclk);
	clk_put(ahb_veclk);
	clk_put(ve_pll4clk);
	clk_disable(avs_moduleclk);	
	clk_put(avs_moduleclk);
	#endif
	#endif
	/* Destroy char device */
	if (cedar_devp) {
		cdev_del(&cedar_devp->cdev);
		device_destroy(cedar_devp->class, dev);
		class_destroy(cedar_devp->class);
	}

	unregister_chrdev_region(dev, 1);	
  	platform_driver_unregister(&sw_cedar_driver);
	if (cedar_devp) {
		kfree(cedar_devp);
	}
}
module_exit(cedardev_exit);

MODULE_AUTHOR("Soft-Allwinner");
MODULE_DESCRIPTION("User mode CEDAR device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
