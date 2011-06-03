#include "drv_disp.h"
#include "dev_disp.h"

fb_info_t g_fbi;
__disp_drv_t g_disp_drv;


__s32 DRV_lcd_open(__u32 sel)
{    
    __u32 i = 0;
    __lcd_flow_t *flow;

	if(g_disp_drv.b_lcd_open[sel] == 0)
	{	    
	    BSP_disp_lcd_open_before(sel);

	    flow = BSP_disp_lcd_get_open_flow(sel);
	    for(i=0; i<flow->func_num; i++)
	    {
	        __u32 timeout = flow->func[i].delay*HZ/1000;

	        flow->func[i].func(sel);
	    	
	    	set_current_state(TASK_INTERRUPTIBLE);
	    	schedule_timeout(timeout);    

	    }

	    BSP_disp_lcd_open_after(sel);

		g_disp_drv.b_lcd_open[sel] = 1;
	}
	
    return 0;
}

__s32 DRV_lcd_close(__u32 sel)
{    
    __u32 i = 0;
    __lcd_flow_t *flow;

	if(g_disp_drv.b_lcd_open[sel] == 1)
	{
	    BSP_disp_lcd_close_befor(sel);

	    flow = BSP_disp_lcd_get_close_flow(sel);
	    for(i=0; i<flow->func_num; i++)
	    {
	        __u32 timeout = flow->func[i].delay*HZ/1000;

	        flow->func[i].func(sel);

	    	set_current_state(TASK_INTERRUPTIBLE);
	    	schedule_timeout(timeout);    

	    }

	    BSP_disp_lcd_close_after(sel);

		g_disp_drv.b_lcd_open[sel] = 0;
	}
    return 0;
}

__s32 DRV_scaler_begin(__u32 sel)
{
    down(g_disp_drv.scaler_finished_sem[sel]);
    return 0;
}

void DRV_scaler_finish(__u32 sel)
{
    up(g_disp_drv.scaler_finished_sem[sel]);
}


__s32 disp_set_hdmi_func(__disp_hdmi_func * func)
{
    BSP_disp_set_hdmi_func(func);
    
    return 0;
}

void DRV_disp_wait_cmd_finish(__u32 sel)
{
    if(g_disp_drv.b_cache[sel] == 0 && BSP_disp_get_output_type(sel)!= DISP_OUTPUT_TYPE_NONE)
    {
        long timeout = 50 * HZ / 1000;//50ms

        g_disp_drv.b_cmd_finished[sel] = 0;
        timeout = wait_event_timeout(g_disp_drv.my_queue[sel], g_disp_drv.b_cmd_finished[sel] == 1, timeout);
        if(timeout == 0)
        {
            __wrn("wait cmd finished timeout\n");
        }
    }
}

__s32 DRV_disp_int_process(__u32 sel)
{
    if(g_disp_drv.b_cmd_finished[sel] == 0)
    {
        g_disp_drv.b_cmd_finished[sel] = 1;
        wake_up(&g_disp_drv.my_queue[sel]);
    }
    return 0;
}

__s32 DRV_DISP_Init(void)
{
    __disp_bsp_init_para para;

    para.base_image0    = (__u32)g_fbi.io[DISP_IO_IMAGE0];
    para.base_image1    = (__u32)g_fbi.io[DISP_IO_IMAGE1];
    para.base_scaler0   = (__u32)g_fbi.io[DISP_IO_SCALER0];
    para.base_scaler1   = (__u32)g_fbi.io[DISP_IO_SCALER1];
    para.base_lcdc0     = (__u32)g_fbi.io[DISP_IO_LCDC0];
    para.base_lcdc1     = (__u32)g_fbi.io[DISP_IO_LCDC1];
    para.base_tvec0      = (__u32)g_fbi.io[DISP_IO_TVEC0];
    para.base_tvec1      = (__u32)g_fbi.io[DISP_IO_TVEC1];
    para.base_ccmu      = (__u32)g_fbi.base_ccmu;
    para.base_sdram     = (__u32)g_fbi.base_sdram;
    para.base_pioc      = (__u32)g_fbi.base_pioc;
    para.base_pwm       = (__u32)g_fbi.base_pwm;
    
    para.scaler_begin   		= DRV_scaler_begin;
    para.scaler_finish  		= DRV_scaler_finish;
    para.tve_interrup   		= NULL;
	para.Hdmi_open  			= NULL;
	para.Hdmi_close  			= NULL;
	para.hdmi_set_mode  		= NULL;
	para.hdmi_mode_support		= NULL;
	para.hdmi_get_HPD_status	= NULL;
	para.disp_int_process       = DRV_disp_int_process;

	memset(&g_disp_drv, 0, sizeof(__disp_drv_t));

	g_disp_drv.scaler_finished_sem[0] = kmalloc(sizeof(struct semaphore),GFP_KERNEL | __GFP_ZERO);
    if(!g_disp_drv.scaler_finished_sem[0])
    {
        __wrn("create scaler_finished_sem[0] fail!\n");
        return -1;
    }  
	sema_init(g_disp_drv.scaler_finished_sem[0],0);


	g_disp_drv.scaler_finished_sem[1] = kmalloc(sizeof(struct semaphore),GFP_KERNEL | __GFP_ZERO);
    if(!g_disp_drv.scaler_finished_sem[1])
    {
        __wrn("create scaler_finished_sem[1] fail!\n");
        return -1;
    }    
	sema_init(g_disp_drv.scaler_finished_sem[1],0);

    init_waitqueue_head(&g_disp_drv.my_queue[0]);
    init_waitqueue_head(&g_disp_drv.my_queue[1]);

    BSP_disp_init(&para);
    BSP_disp_open();
    Fb_Init();

    return 0;        
}

__s32 DRV_DISP_Exit(void)
{        
    Fb_Exit();
    BSP_disp_close();
    BSP_disp_exit(g_disp_drv.exit_mode);

	kfree(g_disp_drv.scaler_finished_sem[0]);
	kfree(g_disp_drv.scaler_finished_sem[1]);

    return 0;
} 

EXPORT_SYMBOL(disp_set_hdmi_func);

