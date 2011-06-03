#ifndef __DRV_DISP_H__
#define __DRV_DISP_H__

#include "drv_disp_i.h"


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

extern __disp_drv_t    g_disp_drv;

extern __s32 DRV_lcd_open(__u32 sel);
extern __s32 DRV_lcd_close(__u32 sel);
extern __s32 Fb_Init(void);
extern __s32 Fb_Exit(void);
#endif

