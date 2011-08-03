#include "disp_lcd.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_de.h"
#include "disp_clk.h"

static __lcd_flow_t         open_flow[2];
static __lcd_flow_t         close_flow[2];
__panel_para_t              gpanel_info[2];
static __lcd_panel_fun_t    lcd_panel_fun[2];
static __hdle gpio_hdl[4];

void LCD_get_reg_bases(__reg_bases_t *para)
{
	para->base_lcdc0 = gdisp.init_para.base_lcdc0;
	para->base_lcdc1 = gdisp.init_para.base_lcdc1;
	para->base_pioc = gdisp.init_para.base_pioc;
	para->base_ccmu = gdisp.init_para.base_ccmu;
	para->base_pwm  = gdisp.init_para.base_pwm;
}

void LCD_delay_ms(__u32 ms) 
{
#ifdef __LINUX_OSAL__
    __u32 timeout = ms*HZ/1000;
    
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(timeout);
#else
    volatile __u32 time;
    
    for(time = 0; time < (ms*700*1000/10);time++);//assume cpu runs at 700Mhz,10 clock one cycle
#endif
}

void LCD_OPEN_FUNC(__u32 sel, LCD_FUNC func, __u32 delay)
{
    open_flow[sel].func[open_flow[sel].func_num].func = func;
    open_flow[sel].func[open_flow[sel].func_num].delay = delay;
    open_flow[sel].func_num++;
}


void LCD_CLOSE_FUNC(__u32 sel, LCD_FUNC func, __u32 delay)
{
    close_flow[sel].func[close_flow[sel].func_num].func = func;
    close_flow[sel].func[close_flow[sel].func_num].delay = delay;
    close_flow[sel].func_num++;
}

void TCON_open(__u32 sel)
{    
    if(gpanel_info[sel].tcon_index == 0)
    {
        TCON0_open(sel);
        gdisp.screen[sel].lcdc_status |= LCDC_TCON0_USED;
    }
    else
    {
        TCON1_open(sel);
        gdisp.screen[sel].lcdc_status |= LCDC_TCON1_USED;
    }

    if(gpanel_info[sel].lcd_if == 3)
    {
        LCD_LVDS_open(sel);
    }
}

void TCON_close(__u32 sel)
{    
    if(gpanel_info[sel].lcd_if == 3)
    {
        LCD_LVDS_close(sel);
    }

    if(gpanel_info[sel].tcon_index == 0)
    {
        TCON0_close(sel);
        gdisp.screen[sel].lcdc_status &= LCDC_TCON0_USED_MASK;
    }
    else
    {
        TCON1_close(sel);
        gdisp.screen[sel].lcdc_status &= LCDC_TCON1_USED_MASK;
    }
}

__s32 LCD_PWM_EN(__u32 sel, __bool b_en)
{
/*
    __u32 tmp = 0;

    if(b_en)
    {
        tmp = sys_get_wvalue(gdisp.init_para.base_pwm+0x200);
        tmp |= (1<<4);
        sys_put_wvalue(gdisp.init_para.base_pwm+0x200,tmp);
    }
    else
    {
        tmp = sys_get_wvalue(gdisp.init_para.base_pwm+0x200);
        tmp &= (~(1<<4));
        sys_put_wvalue(gdisp.init_para.base_pwm+0x200,tmp);    
    }
*/
    
    if(b_en)
    {
        __hdle hdl;
        user_gpio_set_t  gpio_set[1];
        
        gpio_set->port = 2;
        gpio_set->port_num = 2;
        gpio_set->mul_sel = 1;
        gpio_set->pull = 0;
        gpio_set->drv_level = 1;
        gpio_set->data = 1;

        hdl = OSAL_GPIO_Request(gpio_set, 1);
        OSAL_GPIO_Release(hdl, 2);
    }
    else
    {
        __hdle hdl;
        user_gpio_set_t  gpio_set[1];
        
        gpio_set->port = 2;
        gpio_set->port_num = 2;
        gpio_set->mul_sel = 1;
        gpio_set->pull = 0;
        gpio_set->drv_level = 1;
        gpio_set->data = 0;

        hdl = OSAL_GPIO_Request(gpio_set, 1);
        OSAL_GPIO_Release(hdl, 2);
    }
    return 0;
}

__s32 LCD_BL_EN(__u32 sel, __bool b_en)
{
    user_gpio_set_t  gpio_info[1];
    __hdle hdl;
    int  value;
    int  ret;
    char primary_key[20];

    sprintf(primary_key, "lcd%d_para", sel);
    
    ret = OSAL_script_parser_fetch(primary_key, "lcd_bl_en_used", &value, 1);
    if(ret < 0)
    {
        DE_WRN("fetch script data %s.lcd_bl_en_used fail\n", primary_key);
        return -1;
    }
    else
    {
        DE_INF("%s.lcd_bl_en_used=%d\n",primary_key,value);
    }

    if(value == 1)
    {
        ret = OSAL_script_parser_fetch(primary_key,"lcd_bl_en", (int *)gpio_info, sizeof(user_gpio_set_t)/sizeof(int));
        if(ret < 0)
        {
            DE_WRN("fetch script data %s.lcd_bl_en fail\n", primary_key);
            return -1;
        }
        else
        {
            DE_INF("%s.lcd_bl_en gpio_port=%d,gpio_port_num:%d, data:%d\n",primary_key, gpio_info->port, gpio_info->port_num, gpio_info->data);
        }

        if(!b_en)
        {
            gpio_info->data = (gpio_info->data==0)?1:0;
        }

        hdl = OSAL_GPIO_Request(gpio_info, 1);
        OSAL_GPIO_Release(hdl, 2);
    }
    
    return 0;
}

__s32 LCD_POWER_EN(__u32 sel, __bool b_en)
{
    user_gpio_set_t  gpio_info[1];
    __hdle hdl;
    int  value;
    int  ret;
    char primary_key[20];

    sprintf(primary_key, "lcd%d_para", sel);

    ret = OSAL_script_parser_fetch(primary_key, "lcd_power_used", &value, 1);
    if(ret < 0)
    {
        DE_WRN("fetch script data %s.lcd_power_used fail\n", primary_key);
        return -1;
    }
    else
    {
        DE_INF("%s.lcd_power_used=%d\n",primary_key,value);
    }

    if(value == 1)
    {
        ret = OSAL_script_parser_fetch(primary_key,"lcd_power", (int *)gpio_info, sizeof(user_gpio_set_t)/sizeof(int));
        if(ret < 0)
        {
            DE_WRN("fetch script data %s.lcd_power fail\n", primary_key);
            return -1;
        }
        else
        {
            DE_INF("%s.lcd_power gpio_port=%d,gpio_port_num:%d, data:%d\n", primary_key, gpio_info->port, gpio_info->port_num, gpio_info->data);
        }

        if(!b_en)
        {
            gpio_info->data = (gpio_info->data==0)?1:0;
        }

        hdl = OSAL_GPIO_Request(gpio_info, 1);
        OSAL_GPIO_Release(hdl, 2);
    }
    
    return 0;
}


__s32 LCD_GPIO_request(__u32 sel, __u32 io_index)
{
    user_gpio_set_t  gpio_info[1];
    int  ret;
    char primary_key[20],gpio_name[20];

    sprintf(primary_key, "lcd%d_para", sel);
    sprintf(gpio_name, "lcd_gpio_%d", io_index);

    ret = OSAL_script_parser_fetch(primary_key,gpio_name, (int *)gpio_info, sizeof(user_gpio_set_t)/sizeof(int));
    if(ret < 0)
    {
        DE_WRN("fetch script data %s.%s fail\n",primary_key, gpio_name);
        return -1;
    }
    else
    {
        DE_INF("%s.%s gpio_port=%d,gpio_port_num:%d, data:%d\n",primary_key, gpio_name,gpio_info->port, gpio_info->port_num, gpio_info->data);
    }

    gpio_info->mul_sel = 0;
    gpio_hdl[io_index] = OSAL_GPIO_Request(gpio_info, 1);
    
    return 0;
}

__s32 LCD_GPIO_release(__u32 sel,__u32 io_index)
{
    user_gpio_set_t  gpio_info[1];
    int  ret;
    char primary_key[20],gpio_name[20];

    sprintf(primary_key, "lcd%d_para", sel);
    sprintf(gpio_name, "lcd_gpio_%d", io_index);

    ret = OSAL_script_parser_fetch(primary_key,gpio_name, (int *)gpio_info, sizeof(user_gpio_set_t)/sizeof(int));
    if(ret < 0)
    {
        DE_WRN("fetch script data %s.%s fail\n",primary_key, gpio_name);
        return -1;
    }
    else
    {
        DE_INF("%s.%s gpio_port=%d,gpio_port_num:%d, data:%d\n",primary_key, gpio_name,gpio_info->port, gpio_info->port_num, gpio_info->data);
    }

    OSAL_GPIO_Release(gpio_hdl[io_index], 2);
    
    return 0;
}

__s32 LCD_GPIO_set_attr(__u32 sel,__u32 io_index, __bool b_output)
{
    char gpio_name[20];

    sprintf(gpio_name, "lcd_gpio_%d", io_index);
    return  OSAL_GPIO_DevSetONEPIN_IO_STATUS(gpio_hdl[io_index], b_output, gpio_name);
}

__s32 LCD_GPIO_read(__u32 sel,__u32 io_index)
{
    char gpio_name[20];

    sprintf(gpio_name, "lcd_gpio_%d", io_index);
    return OSAL_GPIO_DevREAD_ONEPIN_DATA(gpio_hdl[io_index], gpio_name);
}

__s32 LCD_GPIO_write(__u32 sel,__u32 io_index, __u32 data)
{
    char gpio_name[20];

    sprintf(gpio_name, "lcd_gpio_%d", io_index);
    return OSAL_GPIO_DevWRITE_ONEPIN_DATA(gpio_hdl[io_index], data, gpio_name);
}

//lcd_pwm_div = log2(24000/(16*pwm_freq));
__s32 Disp_pwm_cfg(__u32 sel)
{
/*
    __u32 tmp;
    __u32 pwm_div;
    __u8 lcd_pwm_div = 0x04;
    __u32 i;
    __u32 mul_val = 1;
    __u32 pwm_freq = gpanel_info[sel].lcd_pwm_freq;
    __u32 PWM_SOUCE_FREQ = 24000;//24M
    __u32 PWM_LEVEL = 16;
    __u32 cycle_num = 0;
	
    if(pwm_freq)
    {
        pwm_div = (PWM_SOUCE_FREQ/(PWM_LEVEL * pwm_freq));
    }
    else
    {
        pwm_div = 16;
    }

    for(i = 0;i <= 12;i++)
    {
        if(pwm_div <= mul_val)
        {
            lcd_pwm_div = i;

            break;
        }

        mul_val = mul_val * 2;
    }

    if(gpanel_info[sel].lcd_pwm_pol == 0)
    {
        cycle_num = 0xc * PWM_LEVEL / 16;
    }
    else
    {
        cycle_num = 0x4 * PWM_LEVEL / 16;
    }

    if(gpanel_info[sel].port_index == 0)
    {
        sys_put_wvalue(gdisp.init_para.base_pwm+0x204, ((PWM_LEVEL - 1)<< 16) + cycle_num);
        
        tmp = sys_get_wvalue(gdisp.init_para.base_pwm+0x200);
        tmp &= 0xffffff00;
        tmp |= ((1<<6) | (1<<5) | lcd_pwm_div);//bit6:gatting the special clock for pwm0; bit5:pwm0  active state is high level
        sys_put_wvalue(gdisp.init_para.base_pwm+0x200,tmp);

        tmp = sys_get_wvalue(gdisp.init_para.base_pioc+0x24);
        tmp &= 0xfffff8ff;
        sys_put_wvalue(gdisp.init_para.base_pioc+0x24,tmp | (2<<8));//pwm io,PB2, bit10:8
    }
    else
    {
        sys_put_wvalue(gdisp.init_para.base_pwm+0x208, ((PWM_LEVEL - 1)<< 16) + cycle_num);
        
        tmp = sys_get_wvalue(gdisp.init_para.base_pwm+0x200);
        tmp &= 0xff807fff;
        tmp |= ((1<<21) | (1<<20) | (lcd_pwm_div<<15));//bit21:gatting the special clock for pwm1; bit20:pwm1  active state is high level
        sys_put_wvalue(gdisp.init_para.base_pwm+0x200,tmp);

        tmp = sys_get_wvalue(gdisp.init_para.base_pioc+0x120);
        tmp &= 0xffff8fff;
        sys_put_wvalue(gdisp.init_para.base_pioc+0x120,tmp | (2<<12));//pwm io,PI3, bit14:12
    }

    */
	return DIS_SUCCESS;
}

//lcd pin:
	//pD0~pD27; 2:lcd0, 4:lcd1
	//pH0~pH27; 2:lcd1

//pwm pin
	//pB2: 2:pwm0
	//pI3:  2:pwm1
__s32 Disp_lcdc_pin_cfg(__u32 sel, __disp_output_type_t out_type, __u32 bon)
{   
    __u32 tmp = 0;
    __u32 reg_start = 0;

    if(sel == 0)
    {
        reg_start = gdisp.init_para.base_pioc+0x6c;
    }
    else
    {
        reg_start = gdisp.init_para.base_pioc+0xfc;
    }

    if(bon)
    {
        switch(out_type)
        {
            case DISP_OUTPUT_TYPE_LCD:
                if(gpanel_info[sel].lcd_if == 3)
                {
                	if(gpanel_info[sel].lcd_lvds_ch == 0 && gpanel_info[sel].lcd_lvds_bitwidth == 0)
                	{
                        sys_put_wvalue(reg_start,0x33333333);
                        tmp = sys_get_wvalue(reg_start + 0x04) & 0xffffff00;
                        sys_put_wvalue(reg_start + 0x04,tmp | 0x00000033);
	                }                    			
                	else if(gpanel_info[sel].lcd_lvds_ch == 0 && gpanel_info[sel].lcd_lvds_bitwidth == 1)	
                	{            			
                        sys_put_wvalue(reg_start,0x33333333);
	                }
                	else if(gpanel_info[sel].lcd_lvds_ch == 1 && gpanel_info[sel].lcd_lvds_bitwidth == 0)
                	{
                        sys_put_wvalue(reg_start,0x33333333);
                        sys_put_wvalue(reg_start + 0x04,0x33333333);
                    	tmp = sys_get_wvalue(reg_start + 0x08) & 0xffff0000;
                    	sys_put_wvalue(reg_start + 0x08,tmp | 0x00003333); 
                    }                    			
                	else
                	{						     			
                        sys_put_wvalue(reg_start,0x33333333);
                    	tmp = sys_get_wvalue(reg_start + 0x04) & 0x000000ff;
                    	sys_put_wvalue(reg_start + 0x04,tmp | 0x33333300);
                    	tmp = sys_get_wvalue(reg_start + 0x08) & 0xffffff00;
                    	sys_put_wvalue(reg_start + 0x08,tmp | 0x00000033); 
	                }             		                
                }
                else
                {
                    sys_put_wvalue(reg_start,0x22222222);
                    sys_put_wvalue(reg_start + 0x04,0x22222222);
                    sys_put_wvalue(reg_start + 0x08,0x22222222);
                    tmp = sys_get_wvalue(reg_start + 0x0c) & 0xffff0000;
                    sys_put_wvalue(reg_start + 0x0c,tmp | 0x00002222);
                }
                break;
            case DISP_OUTPUT_TYPE_VGA:
                tmp = sys_get_wvalue(reg_start + 0x0c) & 0xffff00ff;
                sys_put_wvalue(reg_start + 0x0c,tmp | 0x00002200);
                break;
            default:
                break;
        }
    }
    else
    {
        switch(out_type)
        {
            case DISP_OUTPUT_TYPE_LCD:
                if(gpanel_info[sel].lcd_if == 3)
                {
                	if(gpanel_info[sel].lcd_lvds_ch == 0 && gpanel_info[sel].lcd_lvds_bitwidth == 0)
                	{
                        sys_put_wvalue(reg_start,0x00000000);
                        tmp = sys_get_wvalue(reg_start + 0x04) & 0xffffff00;
                        sys_put_wvalue(reg_start + 0x04,tmp);
	                }                    			
                	else if(gpanel_info[sel].lcd_lvds_ch == 0 && gpanel_info[sel].lcd_lvds_bitwidth == 1)	
                	{            			
                        sys_put_wvalue(reg_start,0x00000000);
	                }
                	else if(gpanel_info[sel].lcd_lvds_ch == 1 && gpanel_info[sel].lcd_lvds_bitwidth == 0)
                	{
                        sys_put_wvalue(reg_start,0x00000000);
                        sys_put_wvalue(reg_start + 0x04,0x00000000);
                    	tmp = sys_get_wvalue(reg_start + 0x08) & 0xffff0000;
                    	sys_put_wvalue(reg_start + 0x08,tmp); 
                    }                    			
                	else
                	{						     			
                        sys_put_wvalue(reg_start,0x00000000);
                    	tmp = sys_get_wvalue(reg_start + 0x04) & 0x000000ff;
                    	sys_put_wvalue(reg_start + 0x04,tmp);
                    	tmp = sys_get_wvalue(reg_start + 0x08) & 0xffffff00;
                    	sys_put_wvalue(reg_start + 0x08,tmp); 
	                }             		                
                }
                else
                {
                    sys_put_wvalue(reg_start,0x00000000);
                    sys_put_wvalue(reg_start + 0x04,0x00000000);
                    sys_put_wvalue(reg_start + 0x08,0x00000000);
                    tmp = sys_get_wvalue(reg_start + 0x0c) & 0xffff0000;
                    sys_put_wvalue(reg_start + 0x0c,tmp);
                }
                break;
            case DISP_OUTPUT_TYPE_VGA:
                tmp = sys_get_wvalue(reg_start + 0x0c) & 0xffff00ff;
                sys_put_wvalue(reg_start + 0x0c,tmp);
                break;
            default:
                break;
        }
    }

	return DIS_SUCCESS;
}


#ifdef __LINUX_OSAL__
__s32 Disp_lcdc_event_proc(int irq, void *parg)
#else
__s32 Disp_lcdc_event_proc(void *parg)
#endif
{
    __u32  lcdc_flags;
    __u32 sel = (__u32)parg;

    lcdc_flags=LCDC_query_int(sel);  

    if(lcdc_flags & LCDC_VBI_LCD)
    {
        LCDC_clear_int(sel,LCDC_VBI_LCD);
        LCD_vbi_event_proc(sel, 0);
    }
    if(lcdc_flags & LCDC_VBI_HD)
    {
        LCDC_clear_int(sel,LCDC_VBI_HD);
        LCD_vbi_event_proc(sel, 1);
    }
    if(lcdc_flags & LCDC_LTI_LCD_FLAG)
    {
        LCDC_clear_int(sel,LCDC_LTI_LCD_FLAG);
        LCD_line_event_proc(sel, 0);
    }
    if(lcdc_flags & LCDC_LTI_HD_FLAG)
    {
        LCDC_clear_int(sel,LCDC_LTI_HD_FLAG);
        LCD_line_event_proc(sel, 1); 
    }

    return OSAL_IRQ_RETURN;
}

__s32 Disp_lcdc_init(__u32 sel)
{
    lcdc_clk_init(sel);
    lvds_clk_init();
    lcdc_clk_on(sel);	//??need to be open
    LCDC_init(sel);
    lcdc_clk_off(sel);
    
    if(sel == 0)
    {
        LCD_get_panel_funs_0(&lcd_panel_fun[sel]);
        
        OSAL_RegISR(INTC_IRQNO_LCDC0,0,Disp_lcdc_event_proc,(void*)sel,0,0);
#ifdef __MELIS_OSAL__
        OSAL_InterruptEnable(INTC_IRQNO_LCDC0);
#endif
    }
    else
    {
        LCD_get_panel_funs_1(&lcd_panel_fun[sel]);
        
        OSAL_RegISR(INTC_IRQNO_LCDC1,0,Disp_lcdc_event_proc,(void*)sel,0,0);
#ifdef __MELIS_OSAL__
        OSAL_InterruptEnable(INTC_IRQNO_LCDC1);
#endif
    }

    lcd_panel_fun[sel].cfg_panel_info(&gpanel_info[sel]);
    Disp_pwm_cfg(sel);
    gpanel_info[sel].tcon_index = 0;

    return DIS_SUCCESS;
}


__s32 Disp_lcdc_exit(__u32 sel)
{	
    if(sel == 0)
    {
        OSAL_InterruptDisable(INTC_IRQNO_LCDC0);
        OSAL_UnRegISR(INTC_IRQNO_LCDC0,Disp_lcdc_event_proc,(void*)sel);
    }
    else if(sel == 1)
    {
        OSAL_InterruptDisable(INTC_IRQNO_LCDC1);
        OSAL_UnRegISR(INTC_IRQNO_LCDC1,Disp_lcdc_event_proc,(void*)sel);
    }

    LCDC_exit(sel);

    lcdc_clk_exit(sel);

    return DIS_SUCCESS;
}

__u32 tv_mode_to_width(__disp_tv_mode_t mode)
{
    __u32 width = 0;

    switch(mode)
    {
        case DISP_TV_MOD_480I:
        case DISP_TV_MOD_576I:
        case DISP_TV_MOD_480P:
        case DISP_TV_MOD_576P:
        case DISP_TV_MOD_PAL:
        case DISP_TV_MOD_NTSC:
        case DISP_TV_MOD_PAL_SVIDEO:
        case DISP_TV_MOD_NTSC_SVIDEO:
        case DISP_TV_MOD_PAL_CVBS_SVIDEO:
        case DISP_TV_MOD_NTSC_CVBS_SVIDEO:
        case DISP_TV_MOD_PAL_M:
        case DISP_TV_MOD_PAL_M_SVIDEO:
        case DISP_TV_MOD_PAL_M_CVBS_SVIDEO:
        case DISP_TV_MOD_PAL_NC:
        case DISP_TV_MOD_PAL_NC_SVIDEO:
        case DISP_TV_MOD_PAL_NC_CVBS_SVIDEO:
            width = 720;
            break;
        case DISP_TV_MOD_720P_50HZ:
        case DISP_TV_MOD_720P_60HZ:
            width = 1280;
            break;
        case DISP_TV_MOD_1080I_50HZ:
        case DISP_TV_MOD_1080I_60HZ:
        case DISP_TV_MOD_1080P_24HZ:
        case DISP_TV_MOD_1080P_50HZ:
        case DISP_TV_MOD_1080P_60HZ:
            width = 1920;
            break;
        default:
            width = 0;
            break;
    }

    return width;
}


__u32 tv_mode_to_height(__disp_tv_mode_t mode)
{
    __u32 height = 0;

    switch(mode)
    {
        case DISP_TV_MOD_480I:
        case DISP_TV_MOD_480P:
        case DISP_TV_MOD_NTSC:
        case DISP_TV_MOD_NTSC_SVIDEO:
        case DISP_TV_MOD_NTSC_CVBS_SVIDEO:
        case DISP_TV_MOD_PAL_M:
        case DISP_TV_MOD_PAL_M_SVIDEO:
        case DISP_TV_MOD_PAL_M_CVBS_SVIDEO:
            height = 480;
            break;
        case DISP_TV_MOD_576I:
        case DISP_TV_MOD_576P:
        case DISP_TV_MOD_PAL:
        case DISP_TV_MOD_PAL_SVIDEO:
        case DISP_TV_MOD_PAL_CVBS_SVIDEO:
        case DISP_TV_MOD_PAL_NC:
        case DISP_TV_MOD_PAL_NC_SVIDEO:
        case DISP_TV_MOD_PAL_NC_CVBS_SVIDEO:
            height = 576;
            break;
        case DISP_TV_MOD_720P_50HZ:
        case DISP_TV_MOD_720P_60HZ:
            height = 720;
            break;
        case DISP_TV_MOD_1080I_50HZ:
        case DISP_TV_MOD_1080I_60HZ:
        case DISP_TV_MOD_1080P_24HZ:
        case DISP_TV_MOD_1080P_50HZ:
        case DISP_TV_MOD_1080P_60HZ:
            height = 1080;
            break;
        default:
            height = 0;
            break;
    }

    return height;
}

__u32 vga_mode_to_width(__disp_vga_mode_t mode)
{
    __u32 width = 0;

    switch(mode)
    {
    	case DISP_VGA_H1680_V1050:
    		width = 1680;
            break;
    	case DISP_VGA_H1440_V900:
    		width = 1440;
            break;
    	case DISP_VGA_H1360_V768:
    		width = 1360;
            break;
    	case DISP_VGA_H1280_V1024:
    		width = 1280;
            break;
    	case DISP_VGA_H1024_V768:
    		width = 1024;
            break;
    	case DISP_VGA_H800_V600:
    		width = 800;
            break;
    	case DISP_VGA_H640_V480:
    		width = 640;
            break;
    	case DISP_VGA_H1440_V900_RB:
    		width = 1440;
            break;
    	case DISP_VGA_H1680_V1050_RB:
    		width = 1680;
            break;
    	case DISP_VGA_H1920_V1080_RB:
    	case DISP_VGA_H1920_V1080:
    		width = 1920;
            break;
    	default:
    		width = 0;
            break;
    }

    return width;
}


__u32 vga_mode_to_height(__disp_vga_mode_t mode)
{
    __u32 height = 0;

    switch(mode)
    {
    case DISP_VGA_H1680_V1050:
        height = 1050;
        break;
    case DISP_VGA_H1440_V900:
        height = 900;
        break;
    case DISP_VGA_H1360_V768:
        height = 768;
        break;
    case DISP_VGA_H1280_V1024:
        height = 1024;
        break;
    case DISP_VGA_H1024_V768:
        height = 768;
        break;
    case DISP_VGA_H800_V600:
        height = 600;
        break;
    case DISP_VGA_H640_V480:
        height = 480;
        break;
    case DISP_VGA_H1440_V900_RB:
        height = 1440;
        break;
    case DISP_VGA_H1680_V1050_RB:
        height = 1050;
        break;
    case DISP_VGA_H1920_V1080_RB:
    case DISP_VGA_H1920_V1080:
        height = 1080;
        break;
    default:
        height = 0;
        break;
    }

    return height;
}

// return 0: progressive scan mode; return 1: interlace scan mode
__u32 Disp_get_screen_scan_mode(__disp_tv_mode_t tv_mode)
{
	__u32 ret = 0;
	
	switch(tv_mode)
	{
		case DISP_TV_MOD_480I:
		case DISP_TV_MOD_NTSC:
		case DISP_TV_MOD_NTSC_SVIDEO:
		case DISP_TV_MOD_NTSC_CVBS_SVIDEO:
		case DISP_TV_MOD_PAL_M:
		case DISP_TV_MOD_PAL_M_SVIDEO:
		case DISP_TV_MOD_PAL_M_CVBS_SVIDEO:
		case DISP_TV_MOD_576I:
		case DISP_TV_MOD_PAL:
		case DISP_TV_MOD_PAL_SVIDEO:
		case DISP_TV_MOD_PAL_CVBS_SVIDEO:
		case DISP_TV_MOD_PAL_NC:
		case DISP_TV_MOD_PAL_NC_SVIDEO:
		case DISP_TV_MOD_PAL_NC_CVBS_SVIDEO:
		case DISP_TV_MOD_1080I_50HZ:	
		case DISP_TV_MOD_1080I_60HZ:
		    ret = 1;
		default:
		    break;
	}

	return ret;
}

__s32 BSP_disp_get_screen_width(__u32 sel)
{    
	__u32 width = 0;

    if((gdisp.screen[sel].status & LCD_ON) || (gdisp.screen[sel].status & TV_ON) || (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON))
    {
        width = DE_BE_get_display_width(sel);
    }
    else
    {
        width = gpanel_info[sel].lcd_x;
    }

    return width;
}
 
__s32 BSP_disp_get_screen_height(__u32 sel)
{    
	__u32 height = 0;
	
    if((gdisp.screen[sel].status & LCD_ON) || (gdisp.screen[sel].status & TV_ON) || (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON))
    {
        height = DE_BE_get_display_height(sel);
    }
    else
    {
        height = gpanel_info[sel].lcd_y;
    }

    return height;
}

__s32 BSP_disp_get_output_type(__u32 sel)
{
	if(gdisp.screen[sel].status & TV_ON)
	{
	    return (__s32)DISP_OUTPUT_TYPE_TV;
	}

	if(gdisp.screen[sel].status & LCD_ON)
	{
		return (__s32)DISP_OUTPUT_TYPE_LCD;
	}

	if(gdisp.screen[sel].status & HDMI_ON)
	{
		return (__s32)DISP_OUTPUT_TYPE_HDMI;
	}

	if(gdisp.screen[sel].status & VGA_ON)
	{
		return (__s32)DISP_OUTPUT_TYPE_VGA;
	}

	return (__s32)DISP_OUTPUT_TYPE_NONE;
}


__s32 BSP_disp_get_frame_rate(__u32 sel)
{
    __s32 frame_rate = -1;

    if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_LCD)
    {
        frame_rate = (gpanel_info[sel].lcd_dclk_freq * 1000000) / (gpanel_info[sel].lcd_ht * (gpanel_info[sel].lcd_vt / 2)) ;
    }
    else if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_TV)
    {
        switch(gdisp.screen[sel].tv_mode)
        {
            case DISP_TV_MOD_480I:
            case DISP_TV_MOD_480P:
            case DISP_TV_MOD_NTSC:
            case DISP_TV_MOD_NTSC_SVIDEO:
            case DISP_TV_MOD_NTSC_CVBS_SVIDEO:
            case DISP_TV_MOD_PAL_M:
            case DISP_TV_MOD_PAL_M_SVIDEO:
            case DISP_TV_MOD_PAL_M_CVBS_SVIDEO:
            case DISP_TV_MOD_720P_60HZ:
            case DISP_TV_MOD_1080I_60HZ:
            case DISP_TV_MOD_1080P_60HZ:
                frame_rate = 60;
                break;
            case DISP_TV_MOD_576I:
            case DISP_TV_MOD_576P:
            case DISP_TV_MOD_PAL:
            case DISP_TV_MOD_PAL_SVIDEO:
            case DISP_TV_MOD_PAL_CVBS_SVIDEO:
            case DISP_TV_MOD_PAL_NC:
            case DISP_TV_MOD_PAL_NC_SVIDEO:
            case DISP_TV_MOD_PAL_NC_CVBS_SVIDEO:
            case DISP_TV_MOD_720P_50HZ:
            case DISP_TV_MOD_1080I_50HZ:
            case DISP_TV_MOD_1080P_50HZ:
                frame_rate = 50;
                break;
            default:
                break;
        }
    }
    else if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_HDMI)
    {
        switch(gdisp.screen[sel].hdmi_mode)
        {
            case DISP_TV_MOD_480I:
            case DISP_TV_MOD_480P:
            case DISP_TV_MOD_720P_60HZ:
            case DISP_TV_MOD_1080I_60HZ:
            case DISP_TV_MOD_1080P_60HZ:
                frame_rate = 60;
                break;
            case DISP_TV_MOD_576I:
            case DISP_TV_MOD_576P:
            case DISP_TV_MOD_720P_50HZ:
            case DISP_TV_MOD_1080I_50HZ:
            case DISP_TV_MOD_1080P_50HZ:
                frame_rate = 50;
                break;
            case DISP_TV_MOD_1080P_24HZ:
                frame_rate = 24;
                break;
            default:
                break;
        }
    }
    else if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_VGA)
    {
        frame_rate = 60;
    }


    return frame_rate;
}

__s32 BSP_disp_lcd_open_before(__u32 sel)
{    
    disp_clk_cfg(sel, DISP_OUTPUT_TYPE_LCD, DIS_NULL);
    lcdc_clk_on(sel);
    image_clk_on(sel);
    Image_open(sel);//set image normal channel start bit , because every de_clk_off( )will reset this bit
    Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_LCD, 1);

    if(gpanel_info[sel].tcon_index == 0)
    {
        TCON0_cfg(sel,(__panel_para_t*)&gpanel_info[sel]);
    }
    else
    {
        TCON1_cfg_ex(sel,(__panel_para_t*)&gpanel_info[sel]);
    }
    BSP_disp_set_yuv_output(sel, FALSE);
    DE_BE_set_display_size(sel, gpanel_info[sel].lcd_x, gpanel_info[sel].lcd_y);
    DE_BE_Output_Select(sel, sel);

    open_flow[sel].func_num = 0;
    lcd_panel_fun[sel].cfg_open_flow(sel);

    return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_open_after(__u32 sel)
{	    
    //esMEM_SwitchDramWorkMode(DRAM_WORK_MODE_LCD);
    gdisp.screen[sel].b_out_interlace = 0;
    gdisp.screen[sel].status |= LCD_ON;
    gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_LCD;

    return DIS_SUCCESS;
}

__lcd_flow_t * BSP_disp_lcd_get_open_flow(__u32 sel)
{    
    return (&open_flow[sel]);
}

__s32 BSP_disp_lcd_close_befor(__u32 sel)
{    
	close_flow[sel].func_num = 0;
	lcd_panel_fun[sel].cfg_close_flow(sel);

	Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_LCD, 0);
	
	return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_close_after(__u32 sel)
{    
    Image_close(sel);
    
	image_clk_off(sel);
	lcdc_clk_off(sel);

	gdisp.screen[sel].status &= LCD_OFF;
	gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_NONE;
	gdisp.screen[sel].pll_use_status &= ((gdisp.screen[sel].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);
	
	return DIS_SUCCESS;
}

__lcd_flow_t * BSP_disp_lcd_get_close_flow(__u32 sel)
{    
    return (&close_flow[sel]);
}

__s32 BSP_disp_lcd_xy_switch(__u32 sel, __s32 mode)
{       
    if(gdisp.screen[sel].LCD_CPUIF_XY_Swap != NULL)
    {
       LCD_CPU_AUTO_FLUSH(sel,0);
       LCD_XY_SWAP(sel);
       (*gdisp.screen[sel].LCD_CPUIF_XY_Swap)(mode);
       LCD_CPU_AUTO_FLUSH(sel,1);
    }      	 
    
    return DIS_SUCCESS;
}
 //setting:  0,       1,      2,....  14,   15
//pol==0:  1,       2,      3,....  15,   0
//pol==1:  0,     15,    14, ...    2,   1
__s32 BSP_disp_lcd_set_bright(__u32 sel, __disp_lcd_bright_t  bright)
{	    
    __u32 value = 0;
    __u32    tmp;

    if(gpanel_info[sel].lcd_pwm_pol == 0)
    {
        if(bright == DISP_LCD_BRIGHT_LEVEL15)
        {
            value = 0;
        }
        else
        {
            value = bright + 1;
        }
    }
    else
    {
        if(bright == DISP_LCD_BRIGHT_LEVEL0)
        {
            value = 0;
        }
        else
        {
            value = 16 - bright;
        }
    }
    
    if(gpanel_info[sel].port_index== 0)
    {
	    tmp = sys_get_wvalue(gdisp.init_para.base_pwm+0x204);
        sys_put_wvalue(gdisp.init_para.base_pwm+0x204,(tmp & 0xffff0000) | value);
    }
    else
    {
	    tmp = sys_get_wvalue(gdisp.init_para.base_pwm+0x208);
        sys_put_wvalue(gdisp.init_para.base_pwm+0x208,(tmp & 0xffff0000) | value);
    }

    return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_get_bright(__u32 sel)
{
    __u32    value;
    __s32	bright = 0;

    if(gpanel_info[sel].port_index== 0)
    {
        value = sys_get_wvalue(gdisp.init_para.base_pwm+0x204);
        value = value & 0x0000ffff;
    }
    else
    {
        value = sys_get_wvalue(gdisp.init_para.base_pwm+0x208);
        value = value & 0x0000ffff;
    }

    if(gpanel_info[sel].lcd_pwm_pol == 0)
    {
        if(value == 0)
        {
            bright = DISP_LCD_BRIGHT_LEVEL15;
        }
        else
        {
            bright = value - 1;
        }
    }
    else
    {
        if(value == 0)
        {
            bright = DISP_LCD_BRIGHT_LEVEL0;
        }
        else
        {
            bright = 16 - value;
        }
    }

    return bright;	
}

__s32 BSP_disp_set_gamma_table(__u32 sel, __u32 *gamtbl_addr,__u32 gamtbl_size)
{    
    if((gamtbl_addr == NULL) || (gamtbl_size>1024))
    {
        DE_WRN("para invalid in BSP_disp_set_gamma_table\n");
        return DIS_FAIL;
    }
    
    TCON1_set_gamma_table(sel,(__u32)(gamtbl_addr),gamtbl_size);

    return DIS_SUCCESS;
}

__s32 BSP_disp_gamma_correction_enable(__u32 sel)
{    
	TCON1_set_gamma_Enable(sel,TRUE);
	
	return DIS_SUCCESS;
}

__s32 BSP_disp_gamma_correction_disable(__u32 sel)
{    
	TCON1_set_gamma_Enable(sel,FALSE);
	
	return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_set_src(__u32 sel, __disp_lcdc_src_t src)
{
    switch (src)
    {
        case DISP_LCDC_SRC_DE_CH1:
            TCON0_select_src(sel, LCDC_SRC_DE1);
            break;

        case DISP_LCDC_SRC_DE_CH2:
            TCON0_select_src(sel, LCDC_SRC_DE2);
            break;

        case DISP_LCDC_SRC_DMA:
            TCON0_select_src(sel, LCDC_SRC_DMA);
            break;

        case DISP_LCDC_SRC_WHITE:
            TCON0_select_src(sel, LCDC_SRC_WHITE);
            break;
            
        case DISP_LCDC_SRC_BLACK:
            TCON0_select_src(sel, LCDC_SRC_BLACK);
            break;

        default:
            DE_WRN("not supported lcdc src:%d in BSP_disp_tv_set_src\n", src);
            return DIS_NOT_SUPPORT;
    }
    return DIS_SUCCESS;
}

