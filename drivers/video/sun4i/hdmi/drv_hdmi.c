
#include "drv_hdmi_i.h"
#include "hdmi_hal.h"
#include "dev_hdmi.h"

static struct cdev *my_cdev;
static dev_t devid ;
static struct class *hdmi_class;


typedef struct
{
	__bool bopen;
	__disp_tv_mode_t mode;
}hdmi_info_t;
static hdmi_info_t ghdmi;

static struct semaphore *run_sem = NULL;
static struct task_struct * HDMI_task;

__u32 cfg_change = 0;

void delay_ms(__u32 t)
{
    volatile __u32 time;

    if(t <= 10)
    {
        for(time = 0; time < (t*300*1000/40);time++);             //assume cpu runs at 300Mhz,10 clock one cycle
    }
    else
    {
        __u32 timeout = t*HZ/1000;
        
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(timeout);
    }
}

__s32 Hdmi_set_display_mode(__disp_tv_mode_t mode)
{
	__u8 hdmi_mode;

	__inf("----hdmi_ioctrl(Hdmi_set_display_mode)mode:%d\n",mode);
	
	switch(mode)
	{
	case DISP_TV_MOD_480I:
		hdmi_mode = HDMI_V720x480i_60Hz_4x3;
		break;
		
	case DISP_TV_MOD_576I:
		hdmi_mode = HDMI_V720x576i_50Hz_4x3;
		break;
		
	case DISP_TV_MOD_480P:
		hdmi_mode = HDMI_V720x480p_60Hz_4x3;
		break;
		
	case DISP_TV_MOD_576P:
		hdmi_mode = HDMI_V720x576p_50Hz_4x3;
		break;  
		
	case DISP_TV_MOD_720P_50HZ:
		hdmi_mode = HDMI_V1280x720p_50Hz;
		break;
		
	case DISP_TV_MOD_720P_60HZ:
		hdmi_mode = HDMI_V1280x720p_60Hz;
		break;
		
	case DISP_TV_MOD_1080I_50HZ:
		hdmi_mode = HDMI_V1920x1080i_50Hz;
		break;
		
	case DISP_TV_MOD_1080I_60HZ:
		hdmi_mode = HDMI_V1920x1080i_60Hz;
		break;         
		
	case DISP_TV_MOD_1080P_24HZ:
		hdmi_mode = HDMI_V1920x1080p_24Hz;
		break;    
		
	case DISP_TV_MOD_1080P_50HZ:
		hdmi_mode = HDMI_V1920x1080p_50Hz;
		break;
		
	case DISP_TV_MOD_1080P_60HZ:
		hdmi_mode = HDMI_V1920x1080p_60Hz;
		break;  
		
	default:
	    __wrn("unsupported video mode %d when set display mode\n", mode);
		return -1;
	}

	ghdmi.mode = mode;
	cfg_change = 1;
	return Hdmi_hal_set_display_mode(hdmi_mode);
}

__s32 Hdmi_mode_support(__u8 mode)
{
	__u8 hdmi_mode;
	
	switch(mode)
	{
	case DISP_TV_MOD_480I:
		hdmi_mode = HDMI_V720x480i_60Hz_4x3;
		break;
		
	case DISP_TV_MOD_576I:
		hdmi_mode = HDMI_V720x576i_50Hz_4x3;
		break;
		
	case DISP_TV_MOD_480P:
		hdmi_mode = HDMI_V720x480p_60Hz_4x3;
		break;
		
	case DISP_TV_MOD_576P:
		hdmi_mode = HDMI_V720x576p_50Hz_4x3;
		break;  
		
	case DISP_TV_MOD_720P_50HZ:
		hdmi_mode = HDMI_V1280x720p_50Hz;
		break;
		
	case DISP_TV_MOD_720P_60HZ:
		hdmi_mode = HDMI_V1280x720p_60Hz;
		break;
		
	case DISP_TV_MOD_1080I_50HZ:
		hdmi_mode = HDMI_V1920x1080i_50Hz;
		break;
		
	case DISP_TV_MOD_1080I_60HZ:
		hdmi_mode = HDMI_V1920x1080i_60Hz;
		break;         
		
	case DISP_TV_MOD_1080P_24HZ:
		hdmi_mode = HDMI_V1920x1080p_24Hz;
		break;    
		
	case DISP_TV_MOD_1080P_50HZ:
		hdmi_mode = HDMI_V1920x1080p_50Hz;
		break;
		
	case DISP_TV_MOD_1080P_60HZ:
		hdmi_mode = HDMI_V1920x1080p_60Hz;
		break;  
		
	default:
		hdmi_mode = HDMI_V1280x720p_50Hz;
		break;
	}

	return Hdmi_hal_mode_support(hdmi_mode);
}

__s32 Hdmi_get_HPD_status(void)
{
	return Hdmi_hal_get_HPD_status();
}

__s32 Hdmi_open(void)
{
    __inf("----hdmi_ioctrl(Hdmi_open)\n");
    
	if(ghdmi.bopen == 0)
	{
		up(run_sem);
	}
	ghdmi.bopen = 1;

	return 0;
}

__s32 Hdmi_close(void)
{
    __inf("----hdmi_ioctrl(Hdmi_close)\n");
    
	Hdmi_hal_standby_exit(); 
	ghdmi.bopen = 0;

	return 0;
}


__s32 Hdmi_Audio_Enable(__u8 mode, __u8 channel)
{
    __inf("----hdmi_ioctrl(Hdmi_Audio_Enable),ch:%d\n",channel);
    
    cfg_change = 1;
	return Hdmi_hal_audio_enable(mode, channel);
}

__s32 Hdmi_Set_Audio_Para(hdmi_audio_t * audio_para)
{
    __inf("----hdmi_ioctrl(Hdmi_Set_Audio_Para)\n");
    
    cfg_change = 1;
	return Hdmi_hal_set_audio_para(audio_para);
}

int Hdmi_run_thread(void *parg)
{
	while (1)
	{
		if(ghdmi.bopen == 0)
		{
			down(run_sem);
		}
		
        if(cfg_change || (Hdmi_hal_get_connection_status() != HDMI_STATE_PLAY_BACK))
        {
		    Hdmi_hal_main_task();
		}

		if(Hdmi_hal_get_connection_status() == HDMI_STATE_PLAY_BACK)
		{
		    cfg_change = 0;
		    
			delay_ms(100);
		}
		else
		{
			delay_ms(30);   
		}
	}

	return 0;
}

__s32 Hdmi_init(void)
{	    
	memset(&ghdmi, 0, sizeof(hdmi_info_t));

	run_sem = kmalloc(sizeof(struct semaphore),GFP_KERNEL | __GFP_ZERO);
	sema_init((struct semaphore*)run_sem,0);
	
	HDMI_task = kthread_create(Hdmi_run_thread, (void*)0, "hdmi proc");
	if(IS_ERR(HDMI_task))
	{
	    __s32 err = 0;
	    
		printk("Unable to start kernel thread %s.\n","hdmi proc");
		err = PTR_ERR(HDMI_task);
		HDMI_task = NULL;
		return err;
	}
	wake_up_process(HDMI_task);

	Hdmi_hal_init();
	
	return 0;
}

__s32 Hdmi_exit(void)
{
	Hdmi_hal_exit();

	if(run_sem)
	{
		kfree(run_sem);
		run_sem = 0;
	}
	
	if(HDMI_task)
	{
		kthread_stop(HDMI_task);
		HDMI_task = 0;
	}
	return 0;
}

EXPORT_SYMBOL(Hdmi_open);
EXPORT_SYMBOL(Hdmi_close);
EXPORT_SYMBOL(Hdmi_set_display_mode);
EXPORT_SYMBOL(Hdmi_mode_support);
EXPORT_SYMBOL(Hdmi_get_HPD_status);
EXPORT_SYMBOL(Hdmi_Audio_Enable);
EXPORT_SYMBOL(Hdmi_Set_Audio_Para);




static const struct file_operations hdmi_fops = 
{
	.owner		= THIS_MODULE,
	.open		= hdmi_open,
	.release    = hdmi_release,
	.write      = hdmi_write,
	.read		= hdmi_read,
	.unlocked_ioctl	= hdmi_ioctl,
	.mmap       = hdmi_mmap,
};



int __init hdmi_module_init(void)
{
	int ret = 0, err;
	
	__inf("----------- hdmi_module_init call --------------\n");

	 alloc_chrdev_region(&devid, 0, 1, "hdmi");
	 my_cdev = cdev_alloc();
	 cdev_init(my_cdev, &hdmi_fops);
	 my_cdev->owner = THIS_MODULE;
	 err = cdev_add(my_cdev, devid, 1);
	 if (err)
	 {
		  __wrn("cdev_add fail.\n");
		  return -1;
	 }

    hdmi_class = class_create(THIS_MODULE, "hdmi");
    if (IS_ERR(hdmi_class))
    {
        __wrn("class_create fail\n");
        return -1;
    }
    
	ret |= hdmi_i2c_add_driver();
	
	DRV_HDMI_MInit();

	return ret;
}

static void __exit hdmi_module_exit(void)
{
	__inf("hdmi_module_exit\n");
	
	hdmi_i2c_del_driver();

    class_destroy(hdmi_class);

    cdev_del(my_cdev);
}



//module_init(hdmi_module_init);
late_initcall(hdmi_module_init);  //[tt]
module_exit(hdmi_module_exit);

MODULE_AUTHOR("danling_xiao");
MODULE_DESCRIPTION("hdmi driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hdmi");
