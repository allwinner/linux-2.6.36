#include "drv_disp_i.h"
#include "dev_disp.h"


extern fb_info_t g_fbi;


#define FBHANDTOID(handle)  ((handle) - 100)
#define FBIDTOHAND(ID)  ((ID) + 100)


// CONFIG_LYCHEE_DISPLAY_SCREEN0_FB_PIXEL_SEQUENCE
//              0:ARGB    1:BRGA    2:ABGR    3:RGBA
//seq           ARGB        BRGA       ARGB       BRGA
//br_swqp    0              0            1              1      

__s32 parser_disp_init_para(__disp_init_t * init_para)
{
    __u32 i = 0;

    memset(init_para, 0, sizeof(__disp_init_t));
    
#ifdef CONFIG_LYCHEE_DISPLAY_INIT_CONFIG_SUN4I
    init_para->b_init = 1;

#ifdef CONFIG_LYCHEE_DISPLAY_SINGLE_SCREEN0
    init_para->disp_mode = DISP_INIT_MODE_SCREEN0;
#endif

#ifdef CONFIG_LYCHEE_DISPLAY_SINGLE_SCREEN1
    init_para->disp_mode = DISP_INIT_MODE_SCREEN1;
#endif

#ifdef CONFIG_LYCHEE_DISPLAY_DUAL_DIFF_SCREEN
    init_para->disp_mode = DISP_INIT_MODE_TWO_DIFF_SCREEN;
#endif

#ifdef CONFIG_LYCHEE_DISPLAY_DUAL_SAME_SCREEN
    init_para->disp_mode = DISP_INIT_MODE_TWO_SAME_SCREEN;
#endif

#ifdef CONFIG_LYCHEE_DISPLAY_DUAL_DIFF_SCREEN_SAME_CONTENTS
    init_para->disp_mode = DISP_INIT_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS;
#endif


#ifndef CONFIG_LYCHEE_DISPLAY_SINGLE_SCREEN1
    switch (CONFIG_LYCHEE_DISPLAY_SCREEN0_OUTPUT_TYPE)
    {
        case 0:
            init_para->output_type[0] = DISP_OUTPUT_TYPE_NONE;
            break;
        case 1:
            init_para->output_type[0] = DISP_OUTPUT_TYPE_LCD;
            break;
        case 2:
            init_para->output_type[0] = DISP_OUTPUT_TYPE_TV;
            break;
        case 3:
            init_para->output_type[0] = DISP_OUTPUT_TYPE_HDMI;
            break;
        case 4:
            init_para->output_type[0] = DISP_OUTPUT_TYPE_VGA;
            break;
        default:
            init_para->output_type[0] = DISP_OUTPUT_TYPE_LCD;
            break;
    }
    init_para->tv_mode[0] = (__disp_tv_mode_t)CONFIG_LYCHEE_DISPLAY_SCREEN0_TV_MODE;
    init_para->vga_mode[0] = (__disp_vga_mode_t)CONFIG_LYCHEE_DISPLAY_SCREEN0_VGA_MODE;
#endif

#ifndef CONFIG_LYCHEE_DISPLAY_SINGLE_SCREEN0
    switch (CONFIG_LYCHEE_DISPLAY_SCREEN1_OUTPUT_TYPE)
    {
        case 0:
            init_para->output_type[1] = DISP_OUTPUT_TYPE_NONE;
            break;
        case 1:
            init_para->output_type[1] = DISP_OUTPUT_TYPE_LCD;
            break;
        case 2:
            init_para->output_type[1] = DISP_OUTPUT_TYPE_TV;
            break;
        case 3:
            init_para->output_type[1] = DISP_OUTPUT_TYPE_HDMI;
            break;
        case 4:
            init_para->output_type[1] = DISP_OUTPUT_TYPE_VGA;
            break;
        default:
            init_para->output_type[1] = DISP_OUTPUT_TYPE_LCD;
            break;
    }
    init_para->tv_mode[1] = (__disp_tv_mode_t)CONFIG_LYCHEE_DISPLAY_SCREEN1_TV_MODE;
    init_para->vga_mode[1] = (__disp_vga_mode_t)CONFIG_LYCHEE_DISPLAY_SCREEN1_VGA_MODE;
#endif

    if(init_para->disp_mode == DISP_INIT_MODE_TWO_SAME_SCREEN)
    {
        init_para->output_type[1] = init_para->output_type[0];
        init_para->tv_mode[1] = init_para->tv_mode[0];
        init_para->vga_mode[1] = init_para->vga_mode[0];
    }


    #ifdef CONFIG_LYCHEE_DISPLAY_FB0_DOUBLE_BUFFER
    init_para->b_double_buffer[0] = 1;  
    #else
    init_para->b_double_buffer[0] = 0;
    #endif
    init_para->fb_width[0] = CONFIG_LYCHEE_DISPLAY_FB0_WIDTH;
    init_para->fb_height[0] = CONFIG_LYCHEE_DISPLAY_FB0_HEIGHT;
    init_para->format[0] = CONFIG_LYCHEE_DISPLAY_FB0_FORMAT;
    init_para->seq[0] = (CONFIG_LYCHEE_DISPLAY_FB0_PIXEL_SEQUENCE==0 || CONFIG_LYCHEE_DISPLAY_FB0_PIXEL_SEQUENCE==2)?DISP_SEQ_ARGB:DISP_SEQ_BGRA;
    init_para->br_swap[0] = (CONFIG_LYCHEE_DISPLAY_FB0_PIXEL_SEQUENCE==0 || CONFIG_LYCHEE_DISPLAY_FB0_PIXEL_SEQUENCE==1)?0:1;

#ifdef CONFIG_LYCHEE_DISPLAY_DUAL_DIFF_SCREEN
    #ifdef CONFIG_LYCHEE_DISPLAY_FB1_UBLE_BUFFER
    init_para->b_double_buffer[1] = 1;
    #else
    init_para->b_double_buffer[1] = 0;
    #endif
    init_para->fb_width[1] = CONFIG_LYCHEE_DISPLAY_FB1_WIDTH;
    init_para->fb_height[1] = CONFIG_LYCHEE_DISPLAY_FB1_HEIGHT;
    init_para->format[1] = CONFIG_LYCHEE_DISPLAY_FB1_FORMAT;
    init_para->seq[1] = (CONFIG_LYCHEE_DISPLAY_FB1_PIXEL_SEQUENCE==0 || CONFIG_LYCHEE_DISPLAY_FB1_PIXEL_SEQUENCE==2)?DISP_SEQ_ARGB:DISP_SEQ_BGRA;
    init_para->br_swap[1] = (CONFIG_LYCHEE_DISPLAY_FB1_PIXEL_SEQUENCE==0 || CONFIG_LYCHEE_DISPLAY_FB1_PIXEL_SEQUENCE==1)?0:1;
#endif


#else
    init_para->b_init = 0;
#endif

    __inf("====display init para begin====\n");
    __inf("b_init:%d\n", init_para->b_init);
    __inf("disp_mode:%d\n\n", init_para->disp_mode);
    for(i=0; i<2; i++)
    {
        __inf("output_type[%d]:%d\n", i, init_para->output_type[i]);
        __inf("tv_mode[%d]:%d\n", i, init_para->tv_mode[i]);
        __inf("vga_mode[%d]:%d\n\n", i, init_para->vga_mode[i]);
    }
    for(i=0; i<2; i++)
    {
        __inf("b_double_buffer[%d]:%d\n", i, init_para->b_double_buffer[i]);
        __inf("fb_width[%d]:%d\n", i, init_para->fb_width[i]);
        __inf("fb_height[%d]:%d\n", i, init_para->fb_height[i]);
        __inf("format[%d]:%d\n", i, init_para->format[i]);
        __inf("seq[%d]:%d\n", i, init_para->seq[i]);
        __inf("br_swap[%d]:%d\n\n", i, init_para->br_swap[i]);
    }
    __inf("====display init para end====\n");
    return 0;
}

__s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var)
{
    __u32 i=0, j=0;
    
    for(i = 0; i<height; i++)
    {
        for(j = 0; j<width/4; j++)
        {   
            __u32 offset = 0;

            if(var->bits_per_pixel == 32)
            {
                offset = width * i + j;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset));

                offset = width * i + j + width/4;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->green.length)-1)<<var->green.offset));

                offset = width * i + j + width/4*2;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->blue.length)-1)<<var->blue.offset));

                offset = width * i + j + width/4*3;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset) | (((1<<var->green.length)-1)<<var->green.offset));
            }
            else if(var->bits_per_pixel == 16)
            {
                offset = width * i + j;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset));

                offset = width * i + j + width/4;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->green.length)-1)<<var->green.offset));

                offset = width * i + j + width/4*2;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->blue.length)-1)<<var->blue.offset));

                offset = width * i + j + width/4*3;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset) | (((1<<var->green.length)-1)<<var->green.offset));
            }
        }
    }

    return 0;
}



static int __init Fb_map_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	struct page *page;
	
	page = alloc_pages(GFP_KERNEL,get_order(map_size));
	if(page != NULL)
	{
		info->screen_base = page_address(page);
		info->fix.smem_start = virt_to_phys(info->screen_base);
		memset(info->screen_base,0,info->fix.smem_len);
		__inf("Fb_map_video_memory, pa=0x%08lx size:0x%x\n",info->fix.smem_start, info->fix.smem_len);
		return 0;
	}
	else
	{
		__wrn("alloc_pages fail!\n");
		return -ENOMEM;
	}
#else        
    info->screen_base = (char __iomem *)disp_malloc(info->fix.smem_len);
    info->fix.smem_start = (unsigned long)info->screen_base - 0x80000000;
    memset(info->screen_base,0,info->fix.smem_len);

    __inf("Fb_map_video_memory, pa=0x%08lx size:0x%x\n",info->fix.smem_start, info->fix.smem_len);

    return 0;
#endif
}


static inline void Fb_unmap_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	
	free_pages((unsigned long)info->screen_base,get_order(map_size));
#else
    disp_free((void *)info->screen_base);
#endif
}

__s32 disp_fb_to_var(__disp_pixel_fmt_t format, __disp_pixel_seq_t seq, __bool br_swap, struct fb_var_screeninfo *var)//todo
{
    if(format==DISP_FORMAT_ARGB8888)
    {
        var->bits_per_pixel = 32;
        var->transp.length = 8;
        var->red.length = 8;
        var->green.length = 8;
        var->blue.length = 8;
        if(seq == DISP_SEQ_ARGB && br_swap == 0)//argb
        {
            var->transp.offset = var->red.offset + var->red.length;
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else if(seq == DISP_SEQ_BGRA && br_swap == 0)//bgra
        {
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = var->transp.offset + var->transp.length;
            var->transp.offset = 0;
        }
        else if(seq == DISP_SEQ_ARGB && br_swap == 1)//abgr
        {
            var->transp.offset = var->blue.offset + var->blue.length;
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
        else if(seq == DISP_SEQ_BGRA && br_swap == 1)//rgba
        {
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = var->transp.offset + var->transp.length;
            var->transp.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_RGB888)
    {
        var->bits_per_pixel = 24;
        var->transp.length = 0;
        var->red.length = 8;
        var->green.length = 8;
        var->blue.length = 8;
        if(br_swap == 0)//rgb
        {
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else//bgr
        {
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_RGB655)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 0;
        var->red.length = 6;
        var->green.length = 5;
        var->blue.length = 5;
        if(br_swap == 0)//rgb
        {
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else//bgr
        {
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_RGB565)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 0;
        var->red.length = 5;
        var->green.length = 6;
        var->blue.length = 5;
        if(br_swap == 0)//rgb
        {
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else//bgr
        {
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_RGB556)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 0;
        var->red.length = 5;
        var->green.length = 5;
        var->blue.length = 6;
        if(br_swap == 0)//rgb
        {
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else//bgr
        {
            var->blue.offset = var->blue.offset + var->blue.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_ARGB1555)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 1;
        var->red.length = 5;
        var->green.length = 5;
        var->blue.length = 5;
        if(br_swap == 0)//rgb
        {
            var->transp.offset = var->red.offset + var->red.length;
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else//bgr
        {
            var->transp.offset = var->blue.offset + var->blue.length;
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_RGBA5551)
    {
        var->bits_per_pixel = 16;
        var->red.length = 5;
        var->green.length = 5;
        var->blue.length = 5;
        var->transp.length = 1;
        if(br_swap == 0)//rgba
        {
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = var->transp.offset + var->transp.length;
            var->transp.offset = 0;
        }
        else//bgra
        {
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = var->transp.offset + var->transp.length;
            var->transp.offset = 0;
        }
    }
    else if(format==DISP_FORMAT_ARGB4444)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 4;
        var->red.length = 4;
        var->green.length = 4;
        var->blue.length = 4;
        if(br_swap == 0)//argb
        {
            var->transp.offset = var->red.offset + var->red.length;
            var->red.offset = var->green.offset + var->green.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->blue.offset = 0;
        }
        else//abgr
        {
            var->transp.offset = var->blue.offset + var->blue.length;
            var->blue.offset = var->green.offset + var->green.length;
            var->green.offset = var->red.offset + var->red.length;
            var->red.offset = 0;
        }
    }

    return 0;
}

__s32 var_to_disp_fb(__disp_fb_t *fb, struct fb_var_screeninfo *var, struct fb_fix_screeninfo * fix)//todo
{    
    if(var->nonstd == 0)//argb
    {
		var->reserved[0] = DISP_MOD_INTERLEAVED;
		var->reserved[1] = DISP_FORMAT_ARGB8888;
		var->reserved[2] = DISP_SEQ_ARGB;
		var->reserved[3] = 0;

		switch (var->bits_per_pixel) 
		{
		case 1:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 1;
			var->reserved[1] = DISP_FORMAT_1BPP;
			break;

		case 2:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 2;
			var->reserved[1] = DISP_FORMAT_2BPP;
			break;

		case 4:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 4;
			var->reserved[1] = DISP_FORMAT_4BPP;
			break;
			
		case 8:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_8BPP;
			break;
						
		case 16:
			if(var->red.length==6 && var->green.length==5 && var->blue.length==5)
			{
			    var->reserved[1] = DISP_FORMAT_RGB655;
			    if(var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0)//rgb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->blue.offset == 11 && var->green.offset == 6 && var->red.offset == 0)//bgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid RGB655 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
				
			}
			else if(var->red.length==5 && var->green.length==6 && var->blue.length==5)
			{
				var->reserved[1] = DISP_FORMAT_RGB565;
				if(var->red.offset == 11 && var->green.offset == 5 && var->blue.offset == 0)//rgb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->blue.offset == 11 && var->green.offset == 5 && var->red.offset == 0)//bgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid RGB565 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else if(var->red.length==5 && var->green.length==5 && var->blue.length==6)
			{
				var->reserved[1] = DISP_FORMAT_RGB556;
				if(var->red.offset == 11 && var->green.offset == 6 && var->blue.offset == 0)//rgb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->blue.offset == 10 && var->green.offset == 5 && var->red.offset == 0)//bgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid RGB556 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else if(var->transp.length==1 && var->red.length==5 && var->green.length==5 && var->blue.length==5)
			{
				var->reserved[1] = DISP_FORMAT_ARGB1555;
				if(var->transp.offset == 15 && var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0)//argb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->transp.offset == 15 && var->blue.offset == 10 && var->green.offset == 5 && var->red.offset == 0)//abgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid ARGB1555 format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else if(var->transp.length==4 && var->red.length==4 && var->green.length==4 && var->blue.length==4)
			{
				var->reserved[1] = DISP_FORMAT_ARGB4444;
				if(var->transp.offset == 12 && var->red.offset == 8 && var->green.offset == 4 && var->blue.offset == 0)//argb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->transp.offset == 12 && var->blue.offset == 8 && var->green.offset == 4 && var->red.offset == 0)//abgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid ARGB4444 format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else
			{
			    __wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
				return -EINVAL;
			}
			break;
			
		case 24:
			var->red.length		= 8;
			var->green.length	= 8;
			var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_RGB888;
			if(var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0)//rgb
		    {
		        var->reserved[2] = DISP_SEQ_ARGB;
		        var->reserved[3] = 0;
		    }
		    else if(var->blue.offset == 16 && var->green.offset == 8&& var->red.offset == 0)//bgr
		    {
		        var->reserved[2] = DISP_SEQ_ARGB;
		        var->reserved[3] = 1;
		    }
		    else
		    {
		        __wrn("invalid RGB888 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
		        var->reserved[2] = DISP_SEQ_ARGB;
		        var->reserved[3] = 0;
		    }
			break;
			
		case 32:
			var->transp.length  = 8;
			var->red.length		= 8;
			var->green.length	= 8;
			var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_ARGB8888;

			if(var->transp.offset == 24 && var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0)//argb
			{
			    var->reserved[2] = DISP_SEQ_ARGB;
			    var->reserved[3] = 0;
 			}
			else if(var->blue.offset == 24 && var->green.offset == 16 && var->red.offset == 8 && var->transp.offset == 0)//bgra
			{
			    var->reserved[2] = DISP_SEQ_BGRA;
			    var->reserved[3] = 0;
			}
			else if(var->transp.offset == 24 && var->blue.offset == 16 && var->green.offset == 8 && var->red.offset == 0)//abgr
			{
			    var->reserved[2] = DISP_SEQ_ARGB;
			    var->reserved[3] = 1;
			}
			else if(var->red.offset == 24 && var->green.offset == 16 && var->blue.offset == 8 && var->transp.offset == 0)//rgba
			{
			    var->reserved[2] = DISP_SEQ_BGRA;
			    var->reserved[3] = 1;
			}
			else
			{
			    __wrn("invalid argb format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
			    var->reserved[2] = DISP_SEQ_ARGB;
			    var->reserved[3] = 0;
			}
			break;
			
		default:
		    __wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
			return -EINVAL;
		}
	}

    fb->mode = var->reserved[0];
    fb->format = var->reserved[1];
    fb->seq = var->reserved[2];
    fb->br_swap = var->reserved[3];
    fb->size.width = var->xres_virtual;
    
    fix->line_length = (var->xres_virtual * var->bits_per_pixel) / 8;
	
	return 0;
}


static int Fb_open(struct fb_info *info, int user)
{
	return 0;
}
static int Fb_release(struct fb_info *info, int user)
{
	return 0;
}

static int Fb_pan_display(struct fb_var_screeninfo *var,struct fb_info *info)
{
	__u32 sel = 0;

	__msg("Fb_pan_display\n");

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0 || g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
        || ((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1|| g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
        {
            __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
            __disp_layer_info_t layer_para;
            __u32 buffer_num = 1;
            __u32 y_offset = 0;

            if(g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)
            {
                layer_hdl = g_fbi.layer_hdl[info->node][0];
            }
            if(g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB)
            {   
                buffer_num = 2;
            }
            if((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB))
            {
                y_offset = var->yres / 2;
            }
            
        	BSP_disp_layer_get_para(sel, layer_hdl, &layer_para);

        	if(layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)
        	{
            	layer_para.src_win.x = var->xoffset;
            	layer_para.src_win.y = var->yoffset + y_offset;
            	layer_para.src_win.width = var->xres;
            	layer_para.src_win.height = var->yres / buffer_num;
            }
            else
            {
            	layer_para.src_win.x = var->xoffset;
            	layer_para.src_win.y = var->yoffset + y_offset;
            	layer_para.src_win.width = var->xres;
            	layer_para.src_win.height = var->yres / buffer_num;

            	layer_para.scn_win.width = var->xres;
            	layer_para.scn_win.height = var->yres / buffer_num;
            }

            BSP_disp_layer_set_para(sel, layer_hdl, &layer_para);
            //DRV_disp_wait_cmd_finish(sel);
        }
    }
    
	return 0;
}

static int Fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)//todo
{
	__msg("Fb_check_var\n");

	return 0;
}

static int Fb_set_par(struct fb_info *info)//todo
{
	__u32 sel = 0;
    
	__msg("Fb_set_par\n"); 

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0 || g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
        || ((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1|| g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
        {
            struct fb_var_screeninfo *var = &info->var;
            struct fb_fix_screeninfo * fix = &info->fix;
            __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
            __disp_layer_info_t layer_para;
            __u32 buffer_num = 1;
            __u32 y_offset = 0;

            if(g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)
            {
                layer_hdl = g_fbi.layer_hdl[info->node][0];
            }
            if(g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB)
            {   
                buffer_num = 2;
            }
            if((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB))
            {
                y_offset = var->yres / 2;
            }
            BSP_disp_layer_get_para(sel, layer_hdl, &layer_para);

        	layer_para.src_win.x = var->xoffset;
        	layer_para.src_win.y = var->yoffset + y_offset;
        	layer_para.src_win.width = var->xres;
        	layer_para.src_win.height = var->yres / buffer_num;

            var_to_disp_fb(&(layer_para.fb), var, fix);

            BSP_disp_layer_set_para(sel, layer_hdl, &layer_para);
            DRV_disp_wait_cmd_finish(sel);
        }
    }
	return 0;
}
 

static int Fb_setcolreg(unsigned regno,unsigned red, unsigned green, unsigned blue,unsigned transp, struct fb_info *info)
{
    __u32 sel = 0;
    
	 __msg("Fb_setcolreg,regno=%d,a=%d,r=%d,g=%d,b=%d\n",regno, transp,red, green, blue); 

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0 || g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
        || ((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1|| g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
        {
            unsigned int val;

        	switch (info->fix.visual) 
        	{
        	case FB_VISUAL_PSEUDOCOLOR:
        		if (regno < 256) 
        		{
        			val = (transp<<24) | (red<<16) | (green<<8) | blue;
        			BSP_disp_set_palette_table(sel, &val, regno*4, 4);
        		}
        		break;

        	default:
        		break;
        	}
    	}
	}

	return 0;
}

static int Fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
    __u32 sel = 0;
    
	__msg("Fb_setcmap\n"); 
	
    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0 || g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
        || ((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1|| g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
        {
            unsigned int j = 0, val = 0;
            unsigned char hred, hgreen, hblue, htransp = 0xff;
            unsigned short *red, *green, *blue, *transp;

            red = cmap->red;
            green = cmap->green;
            blue = cmap->blue;
            transp = cmap->transp;
            
        	for (j = 0; j < cmap->len; j++) 
        	{
        		hred = (*red++)&0xff;
        		hgreen = (*green++)&0xff;
        		hblue = (*blue++)&0xff;
        		if (transp)
        		{
        			htransp = (*transp++)&0xff;
        		}
        		else
        		{
        		    htransp = 0xff;
        		}

        		val = (htransp<<24) | (hred<<16) | (hgreen<<8) |hblue;
        		BSP_disp_set_palette_table(sel, &val, (cmap->start + j) * 4, 4);
        	}
    	}
	}
	return 0;
}

int Fb_blank(int blank_mode, struct fb_info *info)
{    
    __u32 sel = 0;
    
	__msg("Fb_blank,mode:%d\n",blank_mode); 

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0 || g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
        || ((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1|| g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
        {
            __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];

            if(g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)
            {
                layer_hdl = g_fbi.layer_hdl[info->node][0];
            }
        	if (blank_mode == FB_BLANK_POWERDOWN) 
        	{
        		BSP_disp_layer_close(sel, layer_hdl);
        	} 
        	else 
        	{
        		BSP_disp_layer_open(sel, layer_hdl);
        	}
            DRV_disp_wait_cmd_finish(sel);
        }
    }
	return 0;
}

static int Fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    __msg("Fb_cursor\n"); 

    return 0;
}

static int Fb_dual_screen_set_mode(struct fb_info *info, unsigned int mode)
{
    int ret = 0;
    
    //todo 
    
    g_fbi.fb_mode[info->node] = mode;

    return ret;
}


static int Fb_ioctl(struct fb_info *info, unsigned int cmd,unsigned long arg)
{
	long ret = 0;
	__u32 sel = 0;
	unsigned long layer_hdl = 0;

	switch (cmd) 
	{
    case FBIOGET_LAYER_HDL_0:
        layer_hdl = g_fbi.layer_hdl[info->node][0];
        copy_to_user((void __user *)arg, &layer_hdl, sizeof(unsigned long));
        break; 

    case FBIOGET_LAYER_HDL_1:
        if(g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)
        {
            layer_hdl = g_fbi.layer_hdl[info->node][1];
            copy_to_user((void __user *)arg, &layer_hdl, sizeof(unsigned long));
        }
        else
        {
            ret = -1;
        }
        break; 

    case FBIO_OPEN:
        BSP_disp_layer_open(sel, g_fbi.layer_hdl[info->node][0]);
	    DRV_disp_wait_cmd_finish(sel);

	    if(g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)
	    {
            BSP_disp_layer_open(sel, g_fbi.layer_hdl[info->node][1]);
    	    DRV_disp_wait_cmd_finish(sel);
	    }
        break;

    case FBIO_CLOSE:
        BSP_disp_layer_close(sel, g_fbi.layer_hdl[info->node][0]);
	    DRV_disp_wait_cmd_finish(sel);

	    if(g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)
	    {
            BSP_disp_layer_close(sel, g_fbi.layer_hdl[info->node][1]);
    	    DRV_disp_wait_cmd_finish(sel);
	    }
        break;

    case FBIO_DISPLAY_SCREEN0_ONLY:
        ret = Fb_dual_screen_set_mode(info, FB_MODE_SCREEN0);
        break;

    case FBIO_DISPLAY_SCREEN1_ONLY:
        ret = Fb_dual_screen_set_mode(info, FB_MODE_SCREEN1);
        break;

    case FBIO_DISPLAY_TWO_SAME_SCREEN_TB:
        ret = Fb_dual_screen_set_mode(info, FB_MODE_TWO_SAME_SCREEN_TB);
        break;

    case FBIO_DISPLAY_TWO_DIFF_SCREEN_SAME_CONTENTS:
        ret = Fb_dual_screen_set_mode(info, FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS);
        break;

   	default:
		break;
	}
	return ret;
}

static struct fb_ops dispfb_ops = 
{
	.owner		    = THIS_MODULE,
	.fb_open        = Fb_open,
	.fb_release     = Fb_release,
	.fb_pan_display	= Fb_pan_display,
	.fb_ioctl       = Fb_ioctl,
	.fb_check_var   = Fb_check_var,
	.fb_set_par     = Fb_set_par,
	.fb_setcolreg   = Fb_setcolreg,
	.fb_setcmap     = Fb_setcmap,
	.fb_blank       = Fb_blank,
	.fb_cursor      = Fb_cursor,
};

__s32 Display_Fb_Request(__disp_fb_create_para_t *fb_para)
{
	struct fb_info *info = NULL;
	__s32 hdl = 0;
	__disp_layer_info_t layer_para;
	__u32 buffer_num = 1, sel;
	int ret;

	__msg("Display_Fb_Request\n");


    if(fb_para->fb_mode == FB_MODE_TWO_SAME_SCREEN_TB)
    {
        buffer_num = 2;
    }
    
	info = framebuffer_alloc(0, g_fbi.dev);

	info->fbops   = &dispfb_ops;
	info->flags   = 0; 
	info->device  = g_fbi.dev;
	info->par     = &g_fbi;

	info->var.xoffset         = 0;
	info->var.yoffset         = 0;
	info->var.xres            = fb_para->width;
	info->var.yres            = fb_para->height * buffer_num;
	info->var.xres_virtual    = fb_para->width;
	info->var.yres_virtual    = fb_para->height * (fb_para->b_double_buffer + 1) * buffer_num;
	info->var.nonstd = 0;
    info->var.bits_per_pixel = 32;
    info->var.transp.length = 8;
    info->var.red.length = 8;
    info->var.green.length = 8;
    info->var.blue.length = 8;
    info->var.transp.offset = 24;
    info->var.red.offset = 16;
    info->var.green.offset = 8;
    info->var.blue.offset = 0;
    info->var.activate = FB_ACTIVATE_FORCE;

	info->fix.type	    = FB_TYPE_PACKED_PIXELS;
	info->fix.type_aux	= 0;
	info->fix.visual 		= FB_VISUAL_TRUECOLOR;
	info->fix.xpanstep	= 1;
	info->fix.ypanstep	= 1;
	info->fix.ywrapstep	= 0;
	info->fix.accel	    = FB_ACCEL_NONE;
	info->fix.line_length = fb_para->line_length;
	info->fix.smem_len 	= fb_para->smem_len;

	ret = Fb_map_video_memory(info);
	if (ret) 
	{
		__wrn("Failed to allocate video RAM: %d\n", ret);
		return DIS_FAIL;
	}
	register_framebuffer(info);
	
    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (fb_para->fb_mode == FB_MODE_SCREEN0 || fb_para->fb_mode == FB_MODE_TWO_SAME_SCREEN_TB || fb_para->fb_mode == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
        || ((sel==1) && (fb_para->fb_mode == FB_MODE_SCREEN1|| fb_para->fb_mode == FB_MODE_TWO_SAME_SCREEN_TB || fb_para->fb_mode == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
        {
    	    __u32 screen_width, screen_height;
    	    __u32 y_offset = 0, width = fb_para->width, height = fb_para->height;

    	    screen_width = BSP_disp_get_screen_width(sel);
    	    screen_height = BSP_disp_get_screen_height(sel);

            hdl = BSP_disp_layer_request(sel, fb_para->mode);

            if(fb_para->fb_mode == FB_MODE_TWO_SAME_SCREEN_TB)
            {
                height = fb_para->height / 2;
                if(sel == 1)
                {
                    y_offset = fb_para->height / 2;
                }
            }
            if(width > screen_width)
            {
                width = screen_width;
            }
            if(height > screen_height)
            {
                height = screen_height;
            }

            memset(&layer_para, 0, sizeof(__disp_layer_info_t));
            layer_para.mode = fb_para->mode;
            layer_para.pipe = 0;
            layer_para.alpha_en = 1;
            layer_para.alpha_val = 0xff;
            layer_para.ck_enable = 0;
            layer_para.src_win.x = 0;
            layer_para.src_win.y = y_offset;
            layer_para.src_win.width = width;
            layer_para.src_win.height = height;
            layer_para.scn_win.x = 0;
            layer_para.scn_win.y = 0;
            layer_para.scn_win.width = width;
            layer_para.scn_win.height = height;
            
            layer_para.fb.addr[0] = (__u32)info->fix.smem_start;
            layer_para.fb.addr[1] = (__u32)info->fix.smem_start+fb_para->ch1_offset;
            layer_para.fb.addr[2] = (__u32)info->fix.smem_start+fb_para->ch2_offset;
            layer_para.fb.size.width = fb_para->width;
            layer_para.fb.size.height = fb_para->height;
            layer_para.fb.format = DISP_FORMAT_ARGB8888;
            layer_para.fb.seq = DISP_SEQ_ARGB;
            layer_para.fb.mode = DISP_MOD_INTERLEAVED;
            layer_para.fb.br_swap = 0;
            layer_para.fb.cs_mode = DISP_BT601;
            layer_para.b_from_screen = 0;
            BSP_disp_layer_set_para(sel, hdl, &layer_para);

            BSP_disp_layer_open(sel, hdl);

            if(fb_para->fb_mode == FB_MODE_SCREEN1)
            {
        	    g_fbi.layer_hdl[info->node][0] = hdl;
            }
            else
            {
        	    g_fbi.layer_hdl[info->node][sel] = hdl;
            }
    	}
	}

	g_fbi.fb_mode[info->node] = fb_para->fb_mode;
	g_fbi.fbinfo[info->node] = info;
	g_fbi.fb_num++;

    return FBIDTOHAND(info->node);
}

__s32 Display_Fb_Release(__s32 fb_hdl)
{
	__s32 fb_id = FBHANDTOID(fb_hdl);

    __msg("Display_Fb_Release, fb_hdl:%d\n", fb_hdl);
    
	if(fb_id >= 0)
	{
        struct fb_info *info = g_fbi.fbinfo[fb_id];
        __u32 sel = 0;

        for(sel = 0; sel < 2; sel++)
        {
            if(((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0 || g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS))
            || ((sel==1) && (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1|| g_fbi.fb_mode[info->node] == FB_MODE_TWO_SAME_SCREEN_TB || g_fbi.fb_mode[info->node] == FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)))
            {
                __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
                
                if(g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)
                {
                    layer_hdl = g_fbi.layer_hdl[info->node][0];
                }

                BSP_disp_layer_release(1, layer_hdl);
            }
        }
        g_fbi.layer_hdl[info->node][0] = 0;
        g_fbi.layer_hdl[info->node][1] = 0;
        g_fbi.fb_mode[info->node] = FB_MODE_SCREEN0;
        
    	unregister_framebuffer(info);
    	Fb_unmap_video_memory(info);
    	framebuffer_release(info);

    	g_fbi.fbinfo[fb_id] = NULL;
    	g_fbi.fb_num--;

	    return DIS_SUCCESS;
	}
	else
	{
	    __wrn("invalid paras fb_hdl:%d in Display_Fb_Release\n", fb_hdl);
	    return DIS_FAIL;
	}

}

__s32 Fb_Init(void)
{    
    __disp_fb_create_para_t fb_para;
    struct fb_info *info;
    __disp_init_t disp_init;
    
#ifdef FB_RESERVED_MEM
    __inf("CONFIG_SW_SYSMEM_RESERVED_BASE:%x,CONFIG_SW_SYSMEM_RESERVED_SIZE:%d K\n", CONFIG_SW_SYSMEM_RESERVED_BASE, CONFIG_SW_SYSMEM_RESERVED_SIZE);
    
    disp_create_heap(CONFIG_ANDROID_PMEM_BASE + 0x80000000 - 0x2000000, 0x2000000);
#endif

    parser_disp_init_para(&disp_init);

    if(disp_init.b_init)
    {
        __u32 fb_num = 0, i = 0, sel = 0;

        for(sel = 0; sel<2; sel++)
        {
            if(((sel==0) && (disp_init.disp_mode!=DISP_INIT_MODE_SCREEN1)) || 
                ((sel==1) && (disp_init.disp_mode!=DISP_INIT_MODE_SCREEN0)))
            {
                if(disp_init.output_type[sel] == DISP_OUTPUT_TYPE_LCD)
                {
                    DRV_lcd_open(sel);
                }
                else if(disp_init.output_type[sel] == DISP_OUTPUT_TYPE_TV)
                {
                    BSP_disp_tv_set_mode(sel, disp_init.tv_mode[sel]);
                    BSP_disp_tv_open(sel);
                }
                 else if(disp_init.output_type[sel] == DISP_OUTPUT_TYPE_HDMI)
                {
                    BSP_disp_hdmi_set_mode(sel, disp_init.tv_mode[sel]);
                    BSP_disp_hdmi_open(sel);
                }
                 else if(disp_init.output_type[sel] == DISP_OUTPUT_TYPE_VGA)
                {
                    BSP_disp_vga_set_mode(sel, disp_init.vga_mode[sel]);
                    BSP_disp_vga_open(sel);
                }
            }
        }

        fb_num = (disp_init.disp_mode==DISP_INIT_MODE_TWO_DIFF_SCREEN)?2:1;
        for(i = 0; i<fb_num; i++)
        {
            __u32 screen_id = i;
            __u32 buffer_num = 1;

            if(disp_init.disp_mode == DISP_INIT_MODE_SCREEN1)
            {
                screen_id = 1;
            }
            
            fb_para.mode = DISP_LAYER_WORK_MODE_NORMAL;
            if(disp_init.disp_mode == DISP_INIT_MODE_SCREEN0)
            {
                fb_para.fb_mode = FB_MODE_SCREEN0;
            }
            else if(disp_init.disp_mode == DISP_INIT_MODE_SCREEN1)
            {
                fb_para.fb_mode = FB_MODE_SCREEN1;
            }
            else if(disp_init.disp_mode == DISP_INIT_MODE_TWO_DIFF_SCREEN)
            {
                if(i == 0)
                {
                    fb_para.fb_mode = FB_MODE_SCREEN0;
                }
                else
                {
                    fb_para.fb_mode = FB_MODE_SCREEN1;
                }
            }
            else if(disp_init.disp_mode == DISP_INIT_MODE_TWO_SAME_SCREEN)
            {
                fb_para.fb_mode = FB_MODE_TWO_SAME_SCREEN_TB;
                buffer_num = 2;
            }
            else if(disp_init.disp_mode == DISP_INIT_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)
            {
                fb_para.fb_mode = FB_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS;
            }
            fb_para.b_double_buffer = disp_init.b_double_buffer[i];
            fb_para.line_length = disp_init.fb_width[i] * ((disp_init.format[i]==DISP_FORMAT_ARGB8888)?4:((disp_init.format[i]==DISP_FORMAT_RGB888)?3:2));
            fb_para.smem_len = fb_para.line_length * disp_init.fb_height[i] * buffer_num * (fb_para.b_double_buffer + 1);
            fb_para.width = disp_init.fb_width[i];
            fb_para.height = disp_init.fb_height[i];
            fb_para.ch1_offset = 0;
            fb_para.ch2_offset = 0;
            Display_Fb_Request(&fb_para);

            info = g_fbi.fbinfo[i];
            disp_fb_to_var(disp_init.format[i], disp_init.seq[i], disp_init.br_swap[i], &(info->var));
            info->var.xoffset= 0;
            info->var.yoffset= 0;
            info->var.xres = disp_init.fb_width[i];
            info->var.yres = disp_init.fb_height[i] * buffer_num;
            info->var.xres_virtual= disp_init.fb_width[i];
            info->var.yres_virtual= disp_init.fb_height[i] * buffer_num * (fb_para.b_double_buffer + 1);
            info->var.nonstd = 0;
            Fb_set_par(info);

            fb_draw_colorbar((__u32)info->screen_base, disp_init.fb_width[i], disp_init.fb_height[i]*buffer_num*(fb_para.b_double_buffer + 1), &info->var);
        }

    	if(disp_init.disp_mode != DISP_INIT_MODE_SCREEN1)
    	{
            BSP_disp_print_reg(0, DISP_REG_IMAGE0);
            BSP_disp_print_reg(0, DISP_REG_LCDC0);
            BSP_disp_print_reg(0, DISP_REG_TVEC0); 
        }
        if(disp_init.disp_mode != DISP_INIT_MODE_SCREEN0)
        {
    	    BSP_disp_print_reg(0, DISP_REG_IMAGE1);
    	    BSP_disp_print_reg(0, DISP_REG_LCDC1); 
    	    BSP_disp_print_reg(0, DISP_REG_TVEC1); 
        }
        BSP_disp_print_reg(0, DISP_REG_CCMU); 
        BSP_disp_print_reg(0, DISP_REG_PWM);
        BSP_disp_print_reg(0, DISP_REG_PIOC); 
    }

	return 0;
}

__s32 Fb_Exit(void)
{
	__u8 fb_id=0;

	for(fb_id=0; fb_id<FB_MAX; fb_id++)
	{
		if(g_fbi.fbinfo[fb_id] != NULL)
		{
			Display_Fb_Release(FBIDTOHAND(fb_id));
		}
	}
	
	return 0;
}
