
#include "lcd_panel_cfg.h"


static void LCD_cfg_panel_info(__panel_para_t * info)
{
    memset(info,0,sizeof(__panel_para_t));

//屏的基本信息
    info->lcd_x                   = 800;
    info->lcd_y                   = 480;
    info->lcd_dclk_freq           = 33;  //33MHz
    info->lcd_pwm_freq            = 1;  //KHz
    info->lcd_srgb                = 0x00202020;
    info->lcd_swap                = 0;

//屏的接口配置信息
    info->lcd_if                  = 0;//0:HV , 1:8080 I/F, 2:TTL I/F, 3:LVDS

//屏的HV模块相关信息
    info->lcd_hv_if               = 0;
    info->lcd_hv_smode            = 0;
    info->lcd_hv_syuv_if          = 0;
    info->lcd_hv_hspw             = 0;
    info->lcd_hv_vspw             = 0;

//屏的HV配置信息
    info->lcd_hbp           = 215;
    info->lcd_ht            = 1055;
    info->lcd_vbp           = 34;
    info->lcd_vt            = (2 * 525);

//屏的IO配置信息
    info->lcd_io_cfg0             = 0x10000000;
    info->lcd_io_cfg1             = 0x00000000;
    info->lcd_io_strength         = 0;
}

static __s32 LCD_open_flow(__u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 10); //打开LCD供电,并延迟10ms
	LCD_OPEN_FUNC(sel, TCON_open, 200); //打开LCD控制器,并延迟200ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0); //打开背光,并延迟0ms

	return 0;
}

static __s32 LCD_close_flow(__u32 sel)
{	
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0); //关闭背光,并延迟0ms
	LCD_CLOSE_FUNC(sel, TCON_close, 0); //关闭LCD 控制器,并延迟0ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 20); //关闭LCD供电,并延迟20ms

	return 0;
}

////////////////////////////////////////   POWER   ////////////////////////////////////////////////////////////////////
static void LCD_power_on(__u32 sel)
{
	  __hdle hdl;
    user_gpio_set_t  gpio_set[1];
    
    gpio_set->port = 8;
    gpio_set->port_num = 8;
    gpio_set->mul_sel = 1;
    gpio_set->pull = 1;
    gpio_set->drv_level = 1;
    gpio_set->data = 0;

    hdl = OSAL_GPIO_Request(gpio_set, 1);
    OSAL_GPIO_Release(hdl, 2);
}

static void LCD_power_off(__u32 sel)
{
    __hdle hdl;
    user_gpio_set_t  gpio_set[1];
    
    gpio_set->port = 8;
    gpio_set->port_num = 8;
    gpio_set->mul_sel = 1;
    gpio_set->pull = 1;
    gpio_set->drv_level = 1;
    gpio_set->data = 1;

    hdl = OSAL_GPIO_Request(gpio_set, 1);
    OSAL_GPIO_Release(hdl, 2);
}

////////////////////////////////////////   back light   ////////////////////////////////////////////////////////////////////
static void LCD_bl_open(__u32 sel)
{
    LCD_PWM_EN(sel, 1);
}

static void LCD_bl_close(__u32 sel)
{
    LCD_PWM_EN(sel, 0);
}


void LCD_get_panel_funs_0(__lcd_panel_fun_t * fun)
{
    fun->cfg_panel_info = LCD_cfg_panel_info;
    fun->cfg_open_flow = LCD_open_flow;
    fun->cfg_close_flow = LCD_close_flow;
}
