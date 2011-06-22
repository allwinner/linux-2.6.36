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
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <asm/system.h>
#include <linux/rmap.h>
#include <linux/string.h>
#include "sun4i_drv_ace.h"
#include "sun4i_ace_i.h"

//static int drv_is_open = 0;
static DECLARE_WAIT_QUEUE_HEAD(wait_ae);
__u32 ae_interrupt_sta = 0, ae_interrupt_value = 0;
void *       ccmu_hsram;
//ae、ace、ce共享中断号
#define ACE_IRQ_NO (60)

static int ace_dev_open(struct inode *inode, struct file *filp){   
    int  status = 0;
    return status;
}

static int ace_dev_release(struct inode *inode, struct file *filp){
    int         status = 0;
    
    return status;
}


/*
 * Audio engine interrupt service routine
 * To wake up ae wait queue
 */
static irqreturn_t ace_interrupt(int irq, void *dev)
{
	 volatile int ae_out_mode_reg = 0;
	 
	 
	 
	//status 0x24
	ae_out_mode_reg = readReg(AE_STATUS_REG);
	ae_interrupt_value = ae_out_mode_reg;
	if(ae_out_mode_reg &0x04)
	{
	  writeReg(AE_INT_EN_REG,readReg(AE_INT_EN_REG)&(~0x0f));
	}

	ae_out_mode_reg &= 0x0f;
	writeReg(AE_STATUS_REG,ae_out_mode_reg);	
	ae_interrupt_sta = 1;

	ae_out_mode_reg = readReg(AE_STATUS_REG);//没用  测试是否可以del
	wake_up_interruptible(&wait_ae);

	
    return IRQ_HANDLED;
}

static long
ace_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){	

	int 		ret_val = 0;
	unsigned long        test_arg;
	__ace_req_e mpara;	
	switch (cmd){
		
		case ACE_DEV_HWREQ:
			printk("%s, %d\n", __FILE__, __LINE__);
			copy_from_user(&mpara, (__ace_req_e *)arg,
                       sizeof(__ace_req_e));
			printk("%s, %d\n", __FILE__, __LINE__);
			ret_val = ACE_HwReq(mpara.module, mpara.mode, mpara.timeout);
			printk("%s, %d\n", __FILE__, __LINE__);
			break;
			
		case ACE_DEV_HWREL:
			printk("%s, %d\n", __FILE__, __LINE__);
			copy_from_user(&mpara, (__ace_req_e *)arg,
                       sizeof(__ace_req_e));
			printk("%s, %d\n", __FILE__, __LINE__);
			ret_val = ACE_HwRel(mpara.module);
			printk("%s, %d\n", __FILE__, __LINE__);
			break;
			
		case ACE_DEV_GETCLKFREQ:
			test_arg = ACE_GetClk();
			//*(unsigned long *)arg = test_arg;
			put_user(test_arg, (unsigned long *)arg);
			ret_val = 1;
			break;
		case ACE_DEV_GET_ADDR:
			put_user((int)ace_hsram, (int *)arg);
			break;
		case ACE_DEV_INS_ISR:
#if 0
			if(drv_is_open == 0){
				/* request ACE irq */
				ret_val = request_irq(ACE_IRQ_NO, ace_interrupt, 0, "ace_dev", NULL);
				if (ret_val < 0) {
				   printk("request ace irq err\n");
				   return -EINVAL;
				}
				drv_is_open = 1;
			}
#endif
			break;
		
		case ACE_DEV_UNINS_ISR:
#if 0
		   if(drv_is_open){
			   printk("!!!!release irq");
			   free_irq(ACE_IRQ_NO, NULL);
			   drv_is_open = 0;
		   }
#endif
		   break;

		case ACE_DEV_WAIT_AE:
			wait_event_interruptible(wait_ae, 
			    ae_interrupt_sta);
			ae_interrupt_sta = 0;
			return ae_interrupt_value;

		default:
			break;
	}
	
	return ret_val;
}

void acedev_vma_open(struct vm_area_struct *vma)
{
    //printk(KERN_NOTICE "cedardev VMA open, virt %lx, phys %lx\n", vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
} 

void acedev_vma_close(struct vm_area_struct *vma)
{
    //printk(KERN_NOTICE "cedardev VMA close.\n");
}

static struct vm_operations_struct acedev_remap_vm_ops = {
    .open = acedev_vma_open,
    .close = acedev_vma_close,
};


static int acedev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long temp_pfn;

    temp_pfn = ACE_REGS_pBASE >> 12;


    /* Set reserved and I/O flag for the area. */
    vma->vm_flags |= VM_RESERVED | VM_IO;
	
    /* Select uncached access. */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
                    vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }
    vma->vm_ops = &acedev_remap_vm_ops;
    acedev_vma_open(vma);
    
    return 0; 
} 

static struct file_operations ace_dev_fops = {
    .owner =    THIS_MODULE,
    .unlocked_ioctl = ace_dev_ioctl,
    .mmap           = acedev_mmap,
    .open           = ace_dev_open,
    .release        = ace_dev_release,
};

static struct class *ace_dev_class;
static struct cdev *ace_dev;
static dev_t dev_num ;
struct clk *ace_moduleclk,*dram_aceclk,*ahb_aceclk,*ace_pll5_pclk;

static int __init ace_dev_init(void)
{
    int status=0, err;
	int ret = 0;   	
	unsigned long rate;
	printk("[ace_drv] start!!!\n");
	ret = request_irq(ACE_IRQ_NO, ace_interrupt, 0, "ace_dev", NULL);
	if (ret < 0) {
	   printk("request ace irq err\n");
	   return -EINVAL;
	}
  	/* ace_moduleclk */
	ace_moduleclk = clk_get(NULL,"ace");
	ace_pll5_pclk = clk_get(NULL, "sdram_pll_p");
	if (clk_set_parent(ace_moduleclk, ace_pll5_pclk)) {
		printk("try to set parent of ace_moduleclk to ace_pll5clk failed!\n");		
	}
	rate = clk_get_rate(ace_pll5_pclk);
	if(clk_set_rate(ace_moduleclk, rate/2)) {
		printk("try to set ace_moduleclk rate failed!!!\n");
	}
	if(clk_reset(ace_moduleclk, 0)){
		printk("try to reset ace_moduleclkfailed!!!\n");
	}
	if (-1 == clk_enable(ace_moduleclk)) {
		printk("ace_moduleclk failed; \n");
	}
	
	/*geting dram clk for ace!*/
	dram_aceclk = clk_get(NULL, "sdram_ace");

	if (-1 == clk_enable(dram_aceclk)) {
		printk("dram_moduleclk failed; \n");
	}
	/* getting ahb clk for ace! */
	ahb_aceclk = clk_get(NULL,"ahb_ace");		
	if (-1 == clk_enable(ahb_aceclk)) {
		printk("ahb_aceclk failed; \n");
	}

    alloc_chrdev_region(&dev_num, 0, 1, "ace_chrdev");
    ace_dev = cdev_alloc();
    cdev_init(ace_dev, &ace_dev_fops);
    ace_dev->owner = THIS_MODULE;
    err = cdev_add(ace_dev, dev_num, 1);	
    if (err){
    	printk(KERN_NOTICE"Error %d adding ace_dev!\n", err);    
        return -1;
    }
    ace_dev_class = class_create(THIS_MODULE, "ace_cls");
    device_create(ace_dev_class, NULL,
                  dev_num, NULL, "ace_dev");
    ACE_Init();
    printk("[ace_drv] init end!!!\n");
    return status;
}
module_init(ace_dev_init);

static void __exit ace_dev_exit(void)
{
	free_irq(ACE_IRQ_NO, NULL);
	ACE_Exit();
	clk_disable(ace_moduleclk);
	//释放ace_moduleclk时钟句柄
	clk_put(ace_moduleclk);
	//释放ace_pll5_pclk时钟句柄
	clk_put(ace_pll5_pclk);

	clk_disable(dram_aceclk);
	//释放dram_aceclk时钟句柄
	clk_put(dram_aceclk);	

	clk_disable(ahb_aceclk);
	//释放ahb_aceclk时钟句柄
	clk_put(ahb_aceclk);

    device_destroy(ace_dev_class,  dev_num);
    class_destroy(ace_dev_class);
}
module_exit(ace_dev_exit);

MODULE_AUTHOR("young");
MODULE_DESCRIPTION("User mode encrypt device interface");
MODULE_LICENSE("GPL");

