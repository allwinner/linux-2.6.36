#ifndef __DEV_DISP_H__
#define __DEV_DISP_H__

#include "drv_disp_i.h"

struct info_mm {
	void *info_base;	/* Virtual address */
	unsigned long mem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	__u32 mem_len;			/* Length of frame buffer mem */
};

int disp_open(struct inode *inode, struct file *file);
int disp_release(struct inode *inode, struct file *file);
ssize_t disp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t disp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int disp_mmap(struct file *file, struct vm_area_struct * vma);
long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

extern __s32 Display_Fb_Request(__u32 sel, __disp_fb_create_para_t *fb_para);
extern __s32 Display_Fb_Release(__u32 sel, __s32 hdl);
extern __s32 disp_int_process(__u32 sel);
extern void disp_wait_cmd_finish(__u32 sel);

#endif
