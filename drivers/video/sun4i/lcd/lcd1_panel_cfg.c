
#include "lcd_panel_cfg.h"

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
    LCD_POWER_EN(sel, 1);
}

static void LCD_power_off(__u32 sel)
{
    LCD_POWER_EN(sel, 0);
}

////////////////////////////////////////   back light   ////////////////////////////////////////////////////////////////////
static void LCD_bl_open(__u32 sel)
{
    LCD_BL_EN(sel, 1);
    LCD_PWM_EN(sel, 1);
}

static void LCD_bl_close(__u32 sel)
{
    LCD_BL_EN(sel, 0);
    LCD_PWM_EN(sel, 0);
}


void LCD_get_panel_funs_1(__lcd_panel_fun_t * fun)
{
    fun->cfg_open_flow = LCD_open_flow;
    fun->cfg_close_flow = LCD_close_flow;
}
