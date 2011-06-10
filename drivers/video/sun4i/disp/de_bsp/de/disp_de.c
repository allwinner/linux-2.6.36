#include "disp_de.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_scaler.h"
#include "disp_clk.h"

__s32 Image_init(__u32 sel)
{
    
    image_clk_init(sel);
	image_clk_on(sel);	//when access image registers, must open MODULE CLOCK of image
	DE_BE_Reg_Init(sel);
	
    BSP_disp_sprite_init(sel);
    BSP_disp_set_yuv_output(sel, FALSE);
    
    Image_open(sel);
	
    return DIS_SUCCESS;
}
      
__s32 Image_exit(__u32 sel)
{    
    DE_BE_DisableINT(sel, DE_IMG_IRDY_IE);
    BSP_disp_sprite_exit(sel);
    image_clk_exit(sel);
        
    return DIS_SUCCESS;
}

__s32 Image_open(__u32  sel)
{
   DE_BE_Enable(sel);
      
   return DIS_SUCCESS;
}
      

__s32 Image_close(__u32 sel)
{
   DE_BE_Disable(sel);
   
   gdisp.screen[sel].status &= IMAGE_USED_MASK;
   
   return DIS_SUCCESS;
}


__s32 BSP_disp_set_bright(__u32 sel, __u32 bright)
{
    gdisp.screen[sel].bright = bright;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_bright(__u32 sel)
{
    return gdisp.screen[sel].bright;
}

__s32 BSP_disp_set_contrast(__u32 sel, __u32 contrast)
{
    gdisp.screen[sel].contrast = contrast;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_contrast(__u32 sel)
{
    return gdisp.screen[sel].contrast;
}

__s32 BSP_disp_set_saturation(__u32 sel, __u32 saturation)
{
    gdisp.screen[sel].saturation = saturation;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_saturation(__u32 sel)
{
    return gdisp.screen[sel].saturation;
}

__s32 BSP_disp_enhance_enable(__u32 sel, __bool enable)
{
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation);
    DE_BE_enhance_enable(sel, enable);
    gdisp.screen[sel].enhance_en = enable;

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_enhance_enable(__u32 sel)
{
    return gdisp.screen[sel].enhance_en;
}


__s32 BSP_disp_set_screen_size(__u32 sel, __disp_rectsz_t * size)
{    
    DE_BE_set_display_size(sel, size->width, size->height);

    gdisp.screen[sel].screen_width = size->width;
    gdisp.screen[sel].screen_height= size->height;

    return DIS_SUCCESS;
}


__s32 BSP_disp_set_yuv_output(__u32 sel, __bool bout_yuv)
{
    DE_BE_Output_Cfg_Csc_Coeff(sel, gdisp.screen[sel].bout_yuv);

    gdisp.screen[sel].bout_yuv = bout_yuv;

    return DIS_SUCCESS;
}

