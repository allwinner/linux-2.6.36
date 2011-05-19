#include "drv_disp_i.h"
#include "drv_disp.h"
#include "dev_disp.h"

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

//#define FB_RESERVED_MEM

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

__s32 layer_hdl_to_fb_id(__u32 sel, __u32 hdl)
{
	__s32 i = 0;
	
	for(i = 0; i<FB_MAX; i++)
	{
	    if(g_fbi.fb_screen_id[i] == sel && g_fbi.layer_hdl[i] == hdl)
	    {
	        return i;
	    }
	}
	return -1;
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
	__s32 hdl = g_fbi.layer_hdl[info->node];
	__u32 sel = g_fbi.fb_screen_id[info->node];
	__disp_layer_info_t layer_para;

	__inf("Fb_pan_display\n");

	BSP_disp_layer_get_para(sel, hdl, &layer_para);

	if(layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)
	{
    	layer_para.src_win.x = var->xoffset;
    	layer_para.src_win.y = var->yoffset;
    	layer_para.src_win.width = var->xres;
    	layer_para.src_win.height = var->yres;
    }
    else
    {
    	layer_para.src_win.x = var->xoffset;
    	layer_para.src_win.y = var->yoffset;
    	layer_para.src_win.width = var->xres;
    	layer_para.src_win.height = var->yres;

    	layer_para.scn_win.width = var->xres;
    	layer_para.scn_win.height = var->yres;
    }

    BSP_disp_layer_set_para(sel, hdl, &layer_para);
    disp_wait_cmd_finish(sel);
    
	return 0;
}

static int Fb_ioctl(struct fb_info *info, unsigned int cmd,unsigned long arg)
{
	__s32 karg = 0;
	void __user *uarg = (void __user *)arg;
	long ret = 0;

	switch (cmd) 
	{
	case FBIOGET_LAYER_HDL:
		karg = g_fbi.layer_hdl[info->node];
		if(copy_to_user(uarg, &karg, sizeof(karg)))
		{
			return -EFAULT;
		}
		break; 

	default:
		break;
	}
	return ret;
}

static int Fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)//todo
{
	__inf("Fb_check_var\n");

	return 0;
}

static int Fb_set_par(struct fb_info *info)//todo
{
	struct fb_var_screeninfo *var = &info->var;
	struct fb_fix_screeninfo * fix = &info->fix;
	__disp_layer_info_t layer_para;
	__s32 hdl = g_fbi.layer_hdl[info->node];
	__u32 sel = g_fbi.fb_screen_id[info->node];

	__inf("Fb_set_par\n"); 

    BSP_disp_layer_get_para(sel, hdl, &layer_para);

	layer_para.src_win.x = var->xoffset;
	layer_para.src_win.y = var->yoffset;
	layer_para.src_win.width = var->xres;
	layer_para.src_win.height = var->yres;

    var_to_fb(&(layer_para.fb), var, fix);

    BSP_disp_layer_set_para(sel, hdl, &layer_para);
    disp_wait_cmd_finish(sel);

	return 0;
}
 

static int Fb_setcolreg(unsigned regno,unsigned red, unsigned green, unsigned blue,unsigned transp, struct fb_info *info)
{
	unsigned int val;
	__u32 sel = g_fbi.fb_screen_id[info->node];
	
	 __inf("Fb_setcolreg,regno=%d,a=%d,r=%d,g=%d,b=%d\n",regno, transp,red, green, blue); 

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

	return 0;
}

static int Fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	unsigned int i = 0, val = 0;
	unsigned char hred, hgreen, hblue, htransp = 0xff;
	unsigned short *red, *green, *blue, *transp;
	__u32 sel = g_fbi.fb_screen_id[info->node];

	__inf("Fb_setcmap\n"); 

    red = cmap->red;
    green = cmap->green;
    blue = cmap->blue;
    transp = cmap->transp;
    
	for (i = 0; i < cmap->len; i++) 
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
		BSP_disp_set_palette_table(sel, &val, (cmap->start + i) * 4, 4);
	}
	return 0;
}

int Fb_blank(int blank_mode, struct fb_info *info)
{    
    __u32 sel = g_fbi.fb_screen_id[info->node];
    __s32 hdl = g_fbi.layer_hdl[info->node];
    
	__inf("Fb_blank,mode:%d\n",blank_mode); 

	if (blank_mode == FB_BLANK_POWERDOWN) 
	{
		BSP_disp_layer_close(sel, hdl);
	} 
	else 
	{
		BSP_disp_layer_open(sel, hdl);
	}
	disp_wait_cmd_finish(sel);

	return 0;
}

static int Fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    __inf("Fb_cursor\n"); 

    return 0;
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
	fb_info_t * fbi = &g_fbi;
	__s32 hdl = 0;
	__disp_layer_info_t layer_para;
	int ret;

	__inf("Display_Fb_Request, sel=%d\n", sel);

	fbinfo = framebuffer_alloc(0, fbi->dev);

	fbinfo->fbops   = &dispfb_ops;
	fbinfo->flags   = 0; 
	fbinfo->device  = fbi->dev;
	fbinfo->par     = fbi;

	fbinfo->var.xoffset         = 0;
	fbinfo->var.yoffset         = 0;
	fbinfo->var.xres            = 1;
	fbinfo->var.yres            = 1;
	fbinfo->var.xres_virtual    = 1;
	fbinfo->var.yres_virtual    = 1;

	fbinfo->fix.type	    = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	= 0;
	fbinfo->fix.visual 		= FB_VISUAL_TRUECOLOR;
	fbinfo->fix.xpanstep	= 1;
	fbinfo->fix.ypanstep	= 1;
	fbinfo->fix.ywrapstep	= 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;
	fbinfo->fix.line_length = 0;
	fbinfo->fix.smem_len 	= fb_para->smem_len;

	ret = Fb_map_video_memory(fbinfo);
	if (ret) 
	{
		__wrn("Fb_map_video_memory fail\n");
		return DIS_FAIL;
	}

    hdl = BSP_disp_layer_request(sel, fb_para->mode);

    memset(&layer_para, 0, sizeof(__disp_layer_info_t));
    layer_para.mode = fb_para->mode;
    layer_para.b_from_screen = 0;
    layer_para.pipe = 0;
    layer_para.alpha_en = 0;
    layer_para.alpha_val = 0xff;
    layer_para.ck_enable = 0;
    layer_para.src_win.x = 0;
    layer_para.src_win.y = 0;
    layer_para.src_win.width = 1;
    layer_para.src_win.height = 1;
    layer_para.scn_win.x = 0;
    layer_para.scn_win.y = 0;
    layer_para.scn_win.width = 1;
    layer_para.scn_win.height = 1;
    layer_para.fb.addr[0] = (__u32)fbinfo->fix.smem_start;
    layer_para.fb.addr[1] = (__u32)fbinfo->fix.smem_start+fb_para->ch1_offset;
    layer_para.fb.addr[2] = (__u32)fbinfo->fix.smem_start+fb_para->ch2_offset;
    layer_para.fb.size.width = 1;
    layer_para.fb.size.height = 1;
    layer_para.fb.format = DISP_FORMAT_ARGB8888;
    layer_para.fb.seq = DISP_SEQ_ARGB;
    layer_para.fb.mode = DISP_MOD_INTERLEAVED;
    layer_para.fb.br_swap = 0;
    layer_para.fb.cs_mode = DISP_BT601;
    BSP_disp_layer_set_para(sel, hdl, &layer_para);

    BSP_disp_layer_open(sel, hdl);
    
	register_framebuffer(fbinfo);

	fbi->fb_screen_id[fbinfo->node] = sel;
	fbi->layer_hdl[fbinfo->node] = hdl;
	fbi->fbinfo[fbinfo->node] = fbinfo;
	fbi->fb_num++;

    return hdl;
}

__s32 Display_Fb_Release(__u32 sel, __s32 hdl)
{
	__s32 fb_id = layer_hdl_to_fb_id(sel, hdl);

    __inf("Display_Fb_Release call\n");
    
	if(fb_id >= 0)
	{
	    fb_info_t * fbi = &g_fbi;
        struct fb_info *fbinfo = fbi->fbinfo[fb_id];

        BSP_disp_layer_release(sel, hdl);
        
    	unregister_framebuffer(fbinfo);
    	Fb_unmap_video_memory(fbinfo);
    	framebuffer_release(fbinfo);

    	fbi->fb_screen_id[fbinfo->node] = -1;
    	fbi->layer_hdl[fb_id] = 0;
    	fbi->fbinfo[fb_id] = NULL;
    	fbi->fb_num--;

	    return DIS_SUCCESS;
	}
	else
	{
	    __wrn("invalid paras (sel:%d,hdl:%d)\n", sel, hdl);
	    return DIS_FAIL;
	}

}

__s32 Fb_Init(void)
{    
#if 0
    __disp_fb_create_para_t fb_para;
    __disp_layer_info_t layer_para;
    __u32 layer_hdl;
    __u32 FB_WIDTH = 800;
    __u32 FB_HEIGHT = 480;
    __u32 DOUBLE_BUFFER = 1;
    
#ifdef FB_RESERVED_MEM
    fb_create_heap(CONFIG_SW_SYSMEM_RESERVED_BASE + 0x40000000 + 0x8000, CONFIG_SW_SYSMEM_RESERVED_SIZE*1024 - 0x8000);
#endif

    DRV_lcd_open(0);

    fb_para.mode = DISP_LAYER_WORK_MODE_NORMAL;
    fb_para.smem_len = FB_WIDTH * FB_HEIGHT * 4/*32bpp*/ * (DOUBLE_BUFFER+1);
    fb_para.ch1_offset = 0;
    fb_para.ch2_offset = 0;
    layer_hdl = Display_Fb_Request(0, &fb_para);

    BSP_disp_layer_get_para(0, layer_hdl, &layer_para);
    layer_para.src_win.x = 0;
    layer_para.src_win.y = 0;
    layer_para.src_win.width = FB_WIDTH;
    layer_para.src_win.height = FB_HEIGHT;
    layer_para.scn_win.x = 0;
    layer_para.scn_win.y = 0;
    layer_para.scn_win.width = FB_WIDTH;
    layer_para.scn_win.height = FB_HEIGHT;
    layer_para.fb.size.width = FB_WIDTH;
    layer_para.fb.size.height = FB_HEIGHT * (DOUBLE_BUFFER+1);
    BSP_disp_layer_set_para(0, layer_hdl, &layer_para);
#endif

    return 0;
}

__s32 Fb_Exit(void)
{
	__u8 sel = 0;
	__u8 fb_id=0;

	for(sel = 0; sel<2; sel++)
	{
		for(fb_id=0; fb_id<FB_MAX; fb_id++)
		{
			if(g_fbi.fb_screen_id[fb_id] == sel && g_fbi.fbinfo[fb_id] != NULL)
			{
				Display_Fb_Release(sel, g_fbi.layer_hdl[fb_id]);
			}
		}
	}
	
	return 0;
}

