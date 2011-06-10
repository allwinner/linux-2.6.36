#include "drv_disp_i.h"
#include "drv_disp.h"
#include "dev_disp.h"

//#define FB_RESERVED_MEM
#define DO_FB_INIT


extern fb_info_t g_fbi;

struct alloc_struct_t
{
    __u32 address;                      //申请内存的地址
    __u32 size;                         //分配的内存大小，用户实际得到的内存大小
    __u32 o_size;                       //用户申请的内存大小
    struct alloc_struct_t *next;
};
#define MY_BYTE_ALIGN(x)                ( ( (x + (4*1024-1)) >> 12) << 12)             /* alloc based on 4K byte */
static struct alloc_struct_t boot_heap_head, boot_heap_tail;


#define FBHANDTOID(handle)  ((handle) - 100)
#define FBIDTOHAND(ID)  ((ID) + 100)

__s32 fb_create_heap(__u32 pHeapHead, __u32 nHeapSize)
{    
	if(pHeapHead <0xc0000000) 
	{
	    return -1;                             //检查Head地址是否合法
	}

    boot_heap_head.size    = boot_heap_tail.size = 0;
    boot_heap_head.address = pHeapHead;
    boot_heap_tail.address = pHeapHead + nHeapSize;
    boot_heap_head.next    = &boot_heap_tail;
    boot_heap_tail.next    = 0;

    __inf("head:%x,tail:%x\n" ,boot_heap_head.address, boot_heap_tail.address);
    return 0;
}

void *fb_malloc(__u32 num_bytes)
{
    struct alloc_struct_t *ptr, *newptr;
    __u32  actual_bytes;

    if (!num_bytes) 
    {
        return 0;
    }

    actual_bytes = MY_BYTE_ALIGN(num_bytes);    /* translate the byte count to size of long type       */
    
    ptr = &boot_heap_head;                      /* scan from the boot_heap_head of the heap            */

    while (ptr && ptr->next)                    /* look for enough memory for alloc                    */
    {
        if (ptr->next->address >= (ptr->address + ptr->size + (8 * 1024) + actual_bytes))
        {
            break;
        }
                                                /* find enough memory to alloc                         */
        ptr = ptr->next;
    }

    if (!ptr->next)
    {
        __wrn(" it has reached the boot_heap_tail of the heap now\n");
        return 0;                   /* it has reached the boot_heap_tail of the heap now              */
    }

    newptr = (struct alloc_struct_t *)(ptr->address + ptr->size);
                                                /* create a new node for the memory block             */
    if (!newptr)
    {
        __wrn(" create the node failed, can't manage the block\n");
        return 0;                               /* create the node failed, can't manage the block     */
    }
    
    /* set the memory block chain, insert the node to the chain */
    newptr->address = ptr->address + ptr->size + 4*1024;
    newptr->size    = actual_bytes;
    newptr->o_size  = num_bytes;
    newptr->next    = ptr->next;
    ptr->next       = newptr;

    return (void *)newptr->address;
}

void  fb_free(void *p)
{
    struct alloc_struct_t *ptr, *prev;

	if( p == NULL )
		return;

    ptr = &boot_heap_head;                /* look for the node which po__s32 this memory block                     */
    while (ptr && ptr->next)
    {
        if (ptr->next->address == (__u32)p)
            break;              /* find the node which need to be release                              */
        ptr = ptr->next;
    }

	prev = ptr;
	ptr = ptr->next;

    if (!ptr) return;           /* the node is heap boot_heap_tail                                               */

    prev->next = ptr->next;     /* delete the node which need be released from the memory block chain  */

    return ;
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
		__inf("pa=0x%08lx va=0x%p size:0x%x\n",info->fix.smem_start, info->screen_base, info->fix.smem_len);
		return 0;
	}
	else
	{
		__wrn("alloc_pages fail!\n");
		return -ENOMEM;
	}
#else        
    info->screen_base = (char __iomem *)fb_malloc(info->fix.smem_len);
    info->fix.smem_start = (unsigned long)info->screen_base - 0x40000000;
    memset(info->screen_base,0,info->fix.smem_len);

    __inf("pa=0x%08lx va=0x%p size:0x%x\n",info->fix.smem_start, info->screen_base, info->fix.smem_len);

    return 0;
#endif
}


static inline void Fb_unmap_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	
	free_pages((unsigned long)info->screen_base,get_order(map_size));
#else
    fb_free((void *)info->screen_base);
#endif
}

__s32 var_to_fb(__disp_fb_t *fb, struct fb_var_screeninfo *var, struct fb_fix_screeninfo * fix)//todo
{    
    if(var->nonstd == 0)//argb
    {
		var->reserved[0] = DISP_MOD_INTERLEAVED;

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
			}
			else if(var->red.length==5 && var->green.length==6 && var->blue.length==5)
			{
				var->reserved[1] = DISP_FORMAT_RGB565;
			}
			else if(var->red.length==5 && var->green.length==5 && var->blue.length==6)
			{
				var->reserved[1] = DISP_FORMAT_RGB556;
			}
			else if(var->transp.length==1 && var->red.length==5 && var->green.length==5 && var->blue.length==5)
			{
				var->reserved[1] = DISP_FORMAT_ARGB1555;
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
			break;
			
		case 32:
			var->transp.length  = 8;
			var->red.length		= 8;
			var->green.length	= 8;
			var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_ARGB8888;
			break;
			
		default:
		    __wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
			return -EINVAL;
		}

    	if (var->bits_per_pixel >= 16) 
    	{
    	    if(var->reserved[3] == 0)
    	    {
        		if (var->reserved[2] == DISP_SEQ_ARGB) //argb
        		{
        			var->blue.offset = 0;
        			var->green.offset = var->blue.offset + var->blue.length;
        			var->red.offset = var->green.offset + var->green.length;
        			var->transp.offset = var->red.offset + var->red.length;
        		} 
        		else //bgra
        		{
        			var->transp.offset = 0;
        			var->red.offset = var->transp.offset + var->transp.length;
        			var->green.offset = var->red.offset + var->red.length;
        			var->blue.offset = var->green.offset + var->green.length;
        		}
    		}
    		else
    		{
        		if (var->reserved[2] == DISP_SEQ_ARGB) //abgr
        		{
        			var->red.offset = 0;
        			var->green.offset = var->red.offset + var->red.length;
        			var->blue.offset = var->green.offset + var->green.length;
        			var->transp.offset = var->blue.offset + var->blue.length;
        		} 
        		else //rgba
        		{
        			var->transp.offset = 0;
        			var->blue.offset = var->transp.offset + var->transp.length;
        			var->green.offset = var->blue.offset + var->blue.length;
        			var->red.offset = var->green.offset + var->green.length;
        		}
    		}
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
	__u32 i = 0;

	__msg("Fb_pan_display\n");

    for(i = 0; i < g_fbi.b_dual_screen[info->node]+1; i++)
    {
        if((g_fbi.b_dual_screen[info->node] == 0) || (g_fbi.display_mode[info->node] & (1<<i)))
        {
            __s32 hdl = g_fbi.layer_hdl[info->node][i];
            __u32 sel = g_fbi.screen_id[info->node][i];
            __disp_layer_info_t layer_para;
            __u32 screen_num = 1;
            __u32 y_offset = 0;

            if(g_fbi.b_dual_screen[info->node] && (g_fbi.display_mode[info->node] == 3))
            {   
                screen_num = 2;
            }
            if(g_fbi.b_dual_screen[info->node] && (g_fbi.display_mode[info->node] == 3) && (i == 1))
            {
                y_offset = var->yres / 2;
            }

            screen_num = 1;
            y_offset = 0;
            
        	BSP_disp_layer_get_para(sel, hdl, &layer_para);

        	if(layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)
        	{
            	layer_para.src_win.x = var->xoffset;
            	layer_para.src_win.y = var->yoffset + y_offset;
            	layer_para.src_win.width = var->xres;
            	layer_para.src_win.height = var->yres / screen_num;
            }
            else
            {
            	layer_para.src_win.x = var->xoffset;
            	layer_para.src_win.y = var->yoffset + y_offset;
            	layer_para.src_win.width = var->xres;
            	layer_para.src_win.height = var->yres / screen_num;

            	layer_para.scn_win.width = var->xres;
            	layer_para.scn_win.height = var->yres / screen_num;
            }

            BSP_disp_layer_set_para(sel, hdl, &layer_para);
            DRV_disp_wait_cmd_finish(sel);
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
	__u32 i = 0;
    
	__msg("Fb_set_par\n"); 

    for(i = 0; i < g_fbi.b_dual_screen[info->node]+1; i++)
    {
        if((g_fbi.b_dual_screen[info->node] == 0) || (g_fbi.display_mode[info->node] & (1<<i)))
        {
            struct fb_var_screeninfo *var = &info->var;
            struct fb_fix_screeninfo * fix = &info->fix;
            __s32 hdl = g_fbi.layer_hdl[info->node][i];
            __u32 sel = g_fbi.screen_id[info->node][i];
            __disp_layer_info_t layer_para;
            __u32 screen_num = 1;
            __u32 y_offset = 0;

            if(g_fbi.b_dual_screen[info->node] && (g_fbi.display_mode[info->node] == 3))
            {   
                screen_num = 2;
            }
            if(g_fbi.b_dual_screen[info->node] && (g_fbi.display_mode[info->node] == 3) && (i == 1))
            {
                y_offset = var->yres / 2;
            }

            screen_num = 1;
            y_offset = 0;

            BSP_disp_layer_get_para(sel, hdl, &layer_para);

        	layer_para.src_win.x = var->xoffset;
        	layer_para.src_win.y = var->yoffset + y_offset;
        	layer_para.src_win.width = var->xres;
        	layer_para.src_win.height = var->yres / screen_num;

            var_to_fb(&(layer_para.fb), var, fix);

            BSP_disp_layer_set_para(sel, hdl, &layer_para);
            DRV_disp_wait_cmd_finish(sel);
        }
    }
	return 0;
}
 

static int Fb_setcolreg(unsigned regno,unsigned red, unsigned green, unsigned blue,unsigned transp, struct fb_info *info)
{
    __u32 i = 0;
    
	 __msg("Fb_setcolreg,regno=%d,a=%d,r=%d,g=%d,b=%d\n",regno, transp,red, green, blue); 

    for(i = 0; i < g_fbi.b_dual_screen[info->node]+1; i++)
    {
        if((g_fbi.b_dual_screen[info->node] == 0) || (g_fbi.display_mode[info->node] & (1<<i)))
        {
            unsigned int val;
            __u32 sel = g_fbi.screen_id[info->node][i];

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
    __u32 i = 0;
    
	__msg("Fb_setcmap\n"); 
	
    for(i = 0; i < g_fbi.b_dual_screen[info->node]+1; i++)
    {
        if((g_fbi.b_dual_screen[info->node] == 0) || (g_fbi.display_mode[info->node] & (1<<i)))
        {
            unsigned int j = 0, val = 0;
            unsigned char hred, hgreen, hblue, htransp = 0xff;
            unsigned short *red, *green, *blue, *transp;
            __u32 sel = g_fbi.screen_id[info->node][i];

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
    __u32 i = 0;
    
	__msg("Fb_blank,mode:%d\n",blank_mode); 

    for(i = 0; i < g_fbi.b_dual_screen[info->node]+1; i++)
    {
        if((g_fbi.b_dual_screen[info->node] == 0) || (g_fbi.display_mode[info->node] & (1<<i)))
        {
            __s32 hdl = g_fbi.layer_hdl[info->node][i];
            __u32 sel = g_fbi.screen_id[info->node][i];

        	if (blank_mode == FB_BLANK_POWERDOWN) 
        	{
        		BSP_disp_layer_close(sel, hdl);
        	} 
        	else 
        	{
        		BSP_disp_layer_open(sel, hdl);
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
    
    if(g_fbi.b_dual_screen[info->node])
    {
        int i = 0;

        for(i=0; i<2; i++)
        {
            if(mode & (1<<0))
            {
                ret = BSP_disp_layer_open(i, g_fbi.layer_hdl[info->node][i]);
            }
            else
            {
                ret = BSP_disp_layer_close(i, g_fbi.layer_hdl[info->node][i]);
            }
            DRV_disp_wait_cmd_finish(i);
        }

        g_fbi.display_mode[info->node] = mode;
    }

    return ret;
}


static int Fb_ioctl(struct fb_info *info, unsigned int cmd,unsigned long arg)
{
	long ret = 0;
	__u32 sel = 0;
	__u32 layer_hdl = 0;

	switch (cmd) 
	{
	case FBIOGET_LAYER_HDL:
	    if(g_fbi.b_dual_screen[info->node])
        {
    	    layer_hdl = g_fbi.layer_hdl[info->node][arg];
    	}
    	else
    	{
    	    layer_hdl = g_fbi.layer_hdl[info->node][0];
    	}
	    ret = layer_hdl;
		break; 

    case FBIO_OPEN:
	    if(g_fbi.b_dual_screen[info->node])
        {
    	    sel = g_fbi.screen_id[info->node][arg];
    	    layer_hdl = g_fbi.layer_hdl[info->node][arg];
    	}
    	else
    	{
    	    sel = g_fbi.screen_id[info->node][0];
    	    layer_hdl = g_fbi.layer_hdl[info->node][0];
    	}
		ret = BSP_disp_layer_open(sel, layer_hdl);
		DRV_disp_wait_cmd_finish(sel);
        break;

    case FBIO_CLOSE:
	    if(g_fbi.b_dual_screen[info->node])
        {
    	    sel = g_fbi.screen_id[info->node][arg];
    	    layer_hdl = g_fbi.layer_hdl[info->node][arg];
    	}
    	else
    	{
    	    sel = g_fbi.screen_id[info->node][0];
    	    layer_hdl = g_fbi.layer_hdl[info->node][0];
    	}
	    ret = BSP_disp_layer_close(sel, layer_hdl);
		DRV_disp_wait_cmd_finish(sel);
        break;

    case FBIO_DISPLAY_SCREEN0_ONLY:
        ret = Fb_dual_screen_set_mode(info, 1);
        break;

    case FBIO_DISPLAY_SCREEN1_ONLY:
        ret = Fb_dual_screen_set_mode(info, 2);
        break;

    case FBIO_DISPLAY_DUAL_SCREEN:
        ret = Fb_dual_screen_set_mode(info, 3);
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

__s32 Display_Fb_Request(__u32 sel, __disp_fb_create_para_t *fb_para)
{
	struct fb_info *fbinfo = NULL;
	__s32 hdl = 0;
	__disp_layer_info_t layer_para;
	int ret;
	int i = 0;

	__msg("Display_Fb_Request, sel=%d\n", sel);

	fbinfo = framebuffer_alloc(0, g_fbi.dev);

	fbinfo->fbops   = &dispfb_ops;
	fbinfo->flags   = 0; 
	fbinfo->device  = g_fbi.dev;
	fbinfo->par     = &g_fbi;

	fbinfo->var.xoffset         = 0;
	fbinfo->var.yoffset         = 0;
	fbinfo->var.xres            = fb_para->width;
	fbinfo->var.yres            = fb_para->height * (fb_para->b_dual_screen + 1);
	fbinfo->var.xres_virtual    = fb_para->width;
	fbinfo->var.yres_virtual    = fb_para->height * (fb_para->b_double_buffer + 1) * (fb_para->b_dual_screen + 1);
	fbinfo->var.nonstd = 0;
    fbinfo->var.bits_per_pixel = 32;
    fbinfo->var.transp.length = 8;
    fbinfo->var.red.length = 8;
    fbinfo->var.green.length = 8;
    fbinfo->var.blue.length = 8;
    fbinfo->var.reserved[0] = DISP_MOD_INTERLEAVED;
    fbinfo->var.reserved[1] = DISP_FORMAT_ARGB8888;
    fbinfo->var.reserved[2] = DISP_SEQ_ARGB;
    fbinfo->var.reserved[3] = 0;
    fbinfo->var.activate = FB_ACTIVATE_FORCE;

	fbinfo->fix.type	    = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	= 0;
	fbinfo->fix.visual 		= FB_VISUAL_TRUECOLOR;
	fbinfo->fix.xpanstep	= 1;
	fbinfo->fix.ypanstep	= 1;
	fbinfo->fix.ywrapstep	= 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;
	fbinfo->fix.line_length = fb_para->line_length;
	fbinfo->fix.smem_len 	= fb_para->smem_len;

	ret = Fb_map_video_memory(fbinfo);
	if (ret) 
	{
		__wrn("Failed to allocate video RAM: %d\n", ret);
		return DIS_FAIL;
	}
	register_framebuffer(fbinfo);
	

    for(i = 0; i < fb_para->b_dual_screen+1; i++)
    {
        __u32 sel = 0;

        if(!fb_para->b_dual_screen)
        {
            sel = sel;
        }
        else
        {
            sel = i;
        }
        
        hdl = BSP_disp_layer_request(sel, fb_para->mode);

        memset(&layer_para, 0, sizeof(__disp_layer_info_t));
        layer_para.mode = fb_para->mode;
        layer_para.pipe = 0;
        layer_para.alpha_en = 1;
        layer_para.alpha_val = 0xff;
        layer_para.ck_enable = 0;
        layer_para.src_win.x = 0;
        layer_para.src_win.y = 0;
        layer_para.src_win.width = fb_para->width;
        layer_para.src_win.height = fb_para->height;
        layer_para.scn_win.x = 0;
        layer_para.scn_win.y = 0;
        layer_para.scn_win.width = fb_para->width;
        layer_para.scn_win.height = fb_para->height;
        if(i == 0)
        {
            layer_para.fb.addr[0] = (__u32)fbinfo->fix.smem_start;
            layer_para.fb.addr[1] = (__u32)fbinfo->fix.smem_start+fb_para->ch1_offset;
            layer_para.fb.addr[2] = (__u32)fbinfo->fix.smem_start+fb_para->ch2_offset;
        }
        else
        {
            layer_para.fb.addr[0] = (__u32)fbinfo->fix.smem_start + fb_para->line_length * fb_para->height;
            layer_para.fb.addr[1] = (__u32)fbinfo->fix.smem_start + fb_para->line_length * fb_para->height + fb_para->ch1_offset;
            layer_para.fb.addr[2] = (__u32)fbinfo->fix.smem_start + fb_para->line_length * fb_para->height + fb_para->ch2_offset;
        }
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
        
    	g_fbi.screen_id[fbinfo->node][i] = sel;
    	g_fbi.layer_hdl[fbinfo->node][i] = hdl;
	}

	g_fbi.b_dual_screen[fbinfo->node] = fb_para->b_dual_screen;
	g_fbi.fbinfo[fbinfo->node] = fbinfo;
	g_fbi.fb_num++;

    return FBIDTOHAND(fbinfo->node);
}

__s32 Display_Fb_Release(__s32 fb_hdl)
{
	__s32 fb_id = FBHANDTOID(fb_hdl);

    __msg("Display_Fb_Release call\n");
    
	if(fb_id >= 0)
	{
        struct fb_info *fbinfo = g_fbi.fbinfo[fb_id];
        int i = 0;

        for(i = 0; i < g_fbi.b_dual_screen[fb_id]+1; i++)
        {
            __u32 sel = g_fbi.screen_id[fb_id][i];
            __u32 hdl = g_fbi.layer_hdl[fb_id][i];
            
            BSP_disp_layer_release(sel, hdl);

        	g_fbi.screen_id[fbinfo->node][i] = -1;
        	g_fbi.layer_hdl[fb_id][i] = -1;
    	}

    	unregister_framebuffer(fbinfo);
    	Fb_unmap_video_memory(fbinfo);
    	framebuffer_release(fbinfo);

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
    __disp_color_t color;
    __u32 i,j;
    __u32 FB_WIDTH = 800, FB_HEIGHT = 480;

#ifdef FB_RESERVED_MEM
    fb_create_heap(CONFIG_SW_SYSMEM_RESERVED_BASE + 0x40000000 + 0x8000, CONFIG_SW_SYSMEM_RESERVED_SIZE*1024 - 0x8000);
#endif

#ifdef DO_FB_INIT
    fb_para.mode = DISP_LAYER_WORK_MODE_NORMAL;
    fb_para.b_dual_screen = 0;
    fb_para.b_double_buffer = 1;
    fb_para.line_length = FB_WIDTH * 4;
    fb_para.smem_len = fb_para.line_length * FB_HEIGHT * (fb_para.b_dual_screen+1) * (fb_para.b_double_buffer + 1);
    fb_para.width = FB_WIDTH;
    fb_para.height = FB_HEIGHT;
    fb_para.ch1_offset = 0;
    fb_para.ch2_offset = 0;

    DRV_lcd_open(0);
    
    BSP_disp_lcd_set_bright(0, DISP_LCD_BRIGHT_LEVEL12);

    color.red = 0x00;
    color.green = 0xff;
    color.blue = 0xff;
    BSP_disp_set_bk_color(0, &color);

    if(fb_para.b_dual_screen)
    {
        DRV_lcd_open(1);
        
        BSP_disp_lcd_set_bright(1, DISP_LCD_BRIGHT_LEVEL12);
        
        color.red = 0x00;
        color.green = 0xff;
        color.blue = 0xff;
        BSP_disp_set_bk_color(1, &color);
    }

    Display_Fb_Request(0, &fb_para);
    
    info = g_fbi.fbinfo[0];
    info->var.xoffset= 0;
    info->var.yoffset= 0;
    info->var.xres = FB_WIDTH;
    info->var.yres = FB_HEIGHT * (fb_para.b_dual_screen+1);
    info->var.xres_virtual= FB_WIDTH;
    info->var.yres_virtual= FB_HEIGHT * (fb_para.b_dual_screen+1) * (fb_para.b_double_buffer + 1);
    info->var.nonstd = 0;
    info->var.bits_per_pixel = 32;
    info->var.transp.length = 8;
    info->var.red.length = 8;
    info->var.green.length = 8;
    info->var.blue.length = 8;
    info->var.reserved[0] = DISP_MOD_INTERLEAVED;
    info->var.reserved[1] = DISP_FORMAT_ARGB8888;
    info->var.reserved[2] = DISP_SEQ_ARGB;
    info->var.reserved[3] = 0;
    info->var.activate = FB_ACTIVATE_FORCE;
    Fb_set_par(info);

    memset((void*)info->screen_base, 0xff, fb_para.smem_len);
    for(i = 0; i<FB_HEIGHT*(fb_para.b_dual_screen+1)*(fb_para.b_double_buffer + 1); i++)
    {
        for(j = 0; j<FB_WIDTH/4; j++)
        {   
            __u32 base = (__u32)info->screen_base;
            __u32 offset = FB_WIDTH * i + j;

            sys_put_wvalue(base + offset*4, 0xff000000 | (0xff<<info->var.red.offset));
        }
    }
    for(i = 0; i<FB_HEIGHT*(fb_para.b_dual_screen+1)*(fb_para.b_double_buffer + 1); i++)
    {
        for(j = 0; j<FB_WIDTH/4; j++)
        {   
            __u32 base = (__u32)info->screen_base;
            __u32 offset = FB_WIDTH * i + j + FB_WIDTH/4;

            sys_put_wvalue(base + offset*4, 0xff000000 | (0xff<<info->var.green.offset));
        }
    }
    for(i = 0; i<FB_HEIGHT*(fb_para.b_dual_screen+1)*(fb_para.b_double_buffer + 1); i++)
    {
        for(j = 0; j<FB_WIDTH/4; j++)
        {   
            __u32 base = (__u32)info->screen_base;
            __u32 offset = FB_WIDTH * i + j + FB_WIDTH/4*2;

            sys_put_wvalue(base + offset*4, 0xff000000 | (0xff<<info->var.blue.offset));
        }
    }
    for(i = 0; i<FB_HEIGHT*(fb_para.b_dual_screen+1)*(fb_para.b_double_buffer + 1); i++)
    {
        for(j = 0; j<FB_WIDTH/4; j++)
        {   
            __u32 base = (__u32)info->screen_base;
            __u32 offset = FB_WIDTH * i + j + FB_WIDTH/4*3;

            sys_put_wvalue(base + offset*4, 0xff000000 | (0xff<<info->var.red.offset) | (0xff<<info->var.green.offset));
        }
    }

    if(0)
    {
	    __inf("lcd0:\n");
	    BSP_disp_print_reg((__u32)g_fbi.io[DISP_IO_LCDC0], 256);
	    
	    if(fb_para.b_dual_screen)
	    {
    	    __inf("lcd1:\n");
    	    BSP_disp_print_reg((__u32)g_fbi.io[DISP_IO_LCDC1], 256);
	    }
	    
	    printk("image0:\n");
	    BSP_disp_print_reg((__u32)g_fbi.io[DISP_IO_IMAGE0], 256);
	    
	    if(fb_para.b_dual_screen)
	    {
    	    __inf("image1:\n");
    	    BSP_disp_print_reg((__u32)g_fbi.io[DISP_IO_IMAGE1], 256);
	    }
	    
	    __inf("ccmu:\n");
	    BSP_disp_print_reg(g_fbi.base_ccmu, 256);
	    
	    __inf("pioc:\n");
	    BSP_disp_print_reg(g_fbi.base_pioc, 256); 
	}	
#endif

	return 0;
}

__s32 Fb_Exit(void)
{
	__u8 fb_id=0;
	__u8 i = 0;

	for(fb_id=0; fb_id<FB_MAX; fb_id++)
	{
		if(g_fbi.fbinfo[fb_id] != NULL)
		{
		    for(i = 0; i < g_fbi.b_dual_screen[fb_id]+1; i++)
		    {
			    Display_Fb_Release(g_fbi.layer_hdl[fb_id][i]);
			}
		}
	}
	
	return 0;
}

