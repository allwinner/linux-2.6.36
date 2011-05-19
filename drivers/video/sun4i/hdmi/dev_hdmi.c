
#include "drv_hdmi_i.h"

__s32 DRV_HDMI_MInit(void)
{    
    if(Hdmi_init() == -1)
    {
        __inf("Hdmi_Init() fail!\n");
   	    return -1;
    }

    return 0;

}

__s32 DRV_HDMI_MExit(void)
{
    __s32 ret;
    
    ret = Hdmi_exit();   /*hdmi module exit*/

    return ret;
} 

int hdmi_open(struct inode *inode, struct file *file)
{
	return 0;
}

int hdmi_release(struct inode *inode, struct file *file)
{
	return 0;
}


ssize_t hdmi_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

ssize_t hdmi_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    return -EINVAL;
}

int hdmi_mmap(struct file *file, struct vm_area_struct * vma)
{
	return 0;
}

long hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{ 
	return 0;
}
