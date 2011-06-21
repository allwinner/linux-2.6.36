#ifndef __DEV_DISP_H__
#define __DEV_DISP_H__

#include "drv_disp_i.h"

struct info_mm {
	void *info_base;	/* Virtual address */
	unsigned long mem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	__u32 mem_len;			/* Length of frame buffer mem */
};

typedef struct
{
	struct device   *dev;
	struct resource *mem[DISP_IO_NUM];
	void __iomem    *io[DISP_IO_NUM];

	__u32       base_ccmu;
	__u32       base_sdram;
    __u32       base_pioc;
	__u32       base_pwm;

    unsigned        b_dual_screen[FB_MAX];
    unsigned        display_mode[FB_MAX];//used when b_dual_screen==TRUE, 0:NONE; 1:screen 0; 2:screen 1; 3:both screen; 
    unsigned        screen_id[FB_MAX][2];//[fb_id][0]:b_dual_screen==FALSE || (b_dual_screen==TRUE && screen0); [fb_id][1]:(b_dual_screen==TRUE && screen1)
    unsigned        layer_hdl[FB_MAX][2];//[fb_id][0]:b_dual_screen==FALSE || (b_dual_screen==TRUE && screen0); [fb_id][1]:(b_dual_screen==TRUE && screen1)
    void *          fbinfo[FB_MAX];
    __u8            fb_num;
}fb_info_t;

typedef struct
{
    __u32         	    mid;
    __u32         	    used;
    __u32         	    status;
    __u32    		    exit_mode;//0:clean all  1:disable interrupt
    struct semaphore *  scaler_finished_sem[2];
    __bool              b_cache[2];
	__bool			    b_lcd_open[2];
	wait_queue_head_t   my_queue[2];
	__bool              b_cmd_finished[2];
}__disp_drv_t;

typedef struct
{
    __bool b_init;
    __u32 disp_mode;//0:single screen0; 1:single screen1;  2:dual diff screen; 3:dual same screen
    
    __disp_output_type_t output_type[2];
    __disp_tv_mode_t tv_mode[2];
    __disp_vga_mode_t vga_mode[2];

    __bool b_double_buffer[2];
    __u32 fb_width[2];
    __u32 fb_height[2];
    __disp_pixel_fmt_t format[2];
    __disp_pixel_seq_t seq[2];
    __bool br_swap[2];
}__disp_init_t;

int disp_open(struct inode *inode, struct file *file);
int disp_release(struct inode *inode, struct file *file);
ssize_t disp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t disp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int disp_mmap(struct file *file, struct vm_area_struct * vma);
long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

extern __s32 Display_Fb_Request(__disp_fb_create_para_t *fb_para);
extern __s32 Display_Fb_Release(__s32 hdl);

extern __s32 DRV_disp_int_process(__u32 sel);
extern void DRV_disp_wait_cmd_finish(__u32 sel);

extern __s32 DRV_DISP_Init(void);
extern __s32 DRV_DISP_Exit(void);

extern fb_info_t g_fbi;

extern __disp_drv_t    g_disp_drv;

extern __s32 DRV_lcd_open(__u32 sel);
extern __s32 DRV_lcd_close(__u32 sel);
extern __s32 Fb_Init(void);
extern __s32 Fb_Exit(void);
extern __s32 DRV_disp_print_reg(__u32 id);

#endif
