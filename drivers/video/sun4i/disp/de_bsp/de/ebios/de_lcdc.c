
#include "ebios_lcdc_tve.h"
#include "de_lcdc_i.h"

__u32 lcdc_reg_base0 = 0;
__u32 lcdc_reg_base1 = 0;


#define ____SEPARATOR_LCDC____

__s32 LCDC_set_reg_base(__u32 sel, __u32 address)
{
    if(sel == 0)
    {
	    lcdc_reg_base0 = address;
	}
	else if(sel == 1)
	{
	    lcdc_reg_base1 = address;
	}
	return 0;
}

__u32 LCDC_get_reg_base(__u32 sel)
{
    if(sel == 0)
    {
	    return lcdc_reg_base0;
	}
	else if(sel == 1)
	{
	    return lcdc_reg_base1;
	}
	return 0;
}

__s32 LCDC_init(__u32 sel)
{
	TCON0_close(sel);
	TCON1_close(sel);

	LCDC_enable_int(sel, LCDC_VBI_LCD_EN);
	LCDC_enable_int(sel, LCDC_VBI_HD_EN);
	LCDC_enable_int(sel, LCDC_LTI_LCD_EN);
	LCDC_enable_int(sel, LCDC_LTI_HD_EN);

	TCON0_select_src(sel,0);
	TCON1_select_src(sel,0);

	LCDC_open(sel);

	return 0;
}
   
__s32 LCDC_exit(__u32 sel)
{
	LCDC_disable_int(sel, LCDC_VBI_LCD_EN | LCDC_VBI_HD_EN | LCDC_LTI_LCD_EN | LCDC_LTI_HD_EN);
	LCDC_close(sel);
	return 0;
}

void LCDC_open(__u32 sel)
{
	LCDC_SET_BIT(sel,LCDC_DCLK_OFF,LCDC_BIT31 | LCDC_BIT30 | LCDC_BIT29 | LCDC_BIT28);
	LCDC_SET_BIT(sel,LCDC_GCTL_OFF,LCDC_BIT31);
}

void LCDC_close(__u32 sel)
{
	LCDC_CLR_BIT(sel,LCDC_DCLK_OFF,LCDC_BIT31 | LCDC_BIT30 | LCDC_BIT29 | LCDC_BIT28);
	LCDC_CLR_BIT(sel,LCDC_GCTL_OFF,LCDC_BIT31);
}

__s32 LCDC_set_start_delay(__u32 sel, __u32 tcon_index, __u8 delay)
{
	__u32 tmp;

    if(tcon_index == 0)
    {
	    tmp = LCDC_RUINT32(sel, LCDC_CTL_OFF)&0xfffffe0f;//clear bit8:4
	    tmp |= ((delay&0x1f)<<4);
	    LCDC_WUINT32(sel, LCDC_CTL_OFF,tmp);
	}
	else if(tcon_index == 1)
	{
		tmp = LCDC_RUINT32(sel, LCDC_HDTVIF_OFF)&0xfffffe0f;//clear bit8:4
	    tmp |= ((delay&0x1f)<<4);
	    LCDC_WUINT32(sel, LCDC_HDTVIF_OFF,tmp);
	}
    return 0;
}

__s32 LCDC_get_start_delay(__u32 sel,__u32 tcon_index)
{
	__u32 tmp;

	if(tcon_index == 0)
	{
	    tmp = LCDC_RUINT32(sel, LCDC_CTL_OFF)&0x000001f0;
	    tmp >>= 4;
	    return tmp;	
	}
	else if(tcon_index == 1)
	{
	    tmp = LCDC_RUINT32(sel, LCDC_HDTVIF_OFF)&0x000001f0;
	    tmp >>= 4;
	    return tmp;		
	}
    
    return 0;
}

__u32 LCDC_get_cur_line(__u32 sel, __u32 tcon_index)
{
	__u32 tmp;

    if(tcon_index == 0)
    {
        tmp = LCDC_RUINT32(sel, LCDC_DUBUG_OFF)&0x03ff0000;
        tmp >>= 16;
    }
    else
    {
        tmp = LCDC_RUINT32(sel, LCDC_DUBUG_OFF)&0x00000fff;
    }
    
    return tmp;
}

__s32 LCDC_set_int_line(__u32 sel,__u32 tcon_index, __u32 num)
{
	if(tcon_index==0)
		LCDC_INIT_BIT(sel,LCDC_GINT1_OFF,0x7ff<<16,num<<16);	
	else
		LCDC_INIT_BIT(sel,LCDC_GINT1_OFF,0x7ff,num);		
	return 0;
}

__s32 LCDC_enable_int(__u32 sel, __u32 irqsrc)
{
	LCDC_SET_BIT(sel,LCDC_GINT0_OFF,irqsrc);
    return 0;
}

__s32 LCDC_disable_int(__u32 sel,__u32 irqsrc)
{
	LCDC_CLR_BIT(sel,LCDC_GINT0_OFF,irqsrc);
    return 0;
}

__u32 LCDC_query_int(__u32 sel)
{
    __u32 tmp;

    tmp = LCDC_RUINT32(sel, LCDC_GINT0_OFF) & 0x0000f000;

    return  tmp ;
}

__s32 LCDC_clear_int(__u32 sel,__u32 irqsrc)
{
	LCDC_CLR_BIT(sel,LCDC_GINT0_OFF,irqsrc);
	return 0;
}

#define ____SEPARATOR_TCON0____


__s32 TCON0_open(__u32 sel)
{
	LCDC_SET_BIT(sel, LCDC_CTL_OFF,LCDC_BIT31);
    return 0;
}

__s32 TCON0_close(__u32 sel)
{
	LCDC_CLR_BIT(sel, LCDC_CTL_OFF,LCDC_BIT31);
	LCDC_WUINT32(sel, LCDC_IOCTL1_OFF, 0xffffffff);//?
	return 0;
}

void TCON0_cfg(__u32 sel, __panel_para_t * info)
{
    __u32 tmp;
	__u32 vblank_len;

    vblank_len = info->lcd_vt/2 - info->lcd_y;
	if(vblank_len > 30)
	{
		info->start_delay	= 30;
	}
	else
	{
		info->start_delay	= vblank_len - 1;
	}

    tmp = LCDC_RUINT32(sel, LCDC_CTL_OFF);
    
    LCDC_WUINT32(sel, LCDC_CTL_OFF,tmp | (info->lcd_if <<24) | (info->lcd_swap<< 23) | (0<<20) | (info->start_delay<<4) );

	tmp = LCDC_RUINT32(sel, LCDC_DCLK_OFF);
	LCDC_WUINT32(sel, LCDC_DCLK_OFF, tmp |((__u32)(1<<31/*lcd clk enable*/)));

	LCDC_WUINT32(sel, LCDC_BASIC0_OFF,((info->lcd_x - 1)<<16) | (info->lcd_y - 1) );
	
	LCDC_WUINT32(sel, LCDC_BASIC1_OFF,(info->lcd_uf <<31) | (info->lcd_ht <<16) | info->lcd_hbp);	

	LCDC_WUINT32(sel, LCDC_BASIC2_OFF,(info->lcd_vt <<16) | info->lcd_vbp);

	if(info->lcd_if == LCDC_LCDIF_HV)
	{
		LCDC_WUINT32(sel, LCDC_BASIC3_OFF,(info->lcd_hv_hspw <<16) | info->lcd_hv_vspw);
		LCDC_WUINT32(sel, LCDC_HVIF_OFF,(info->lcd_hv_if<<31) | (info->lcd_hv_smode<<30) |(info->lcd_hv_s888_if<<24) |
                                    (info->lcd_hv_syuv_if<<20));		
	}
	else if(info->lcd_if == LCDC_LCDIF_TTL)
	{
	    LCDC_WUINT32(sel, LCDC_TTL0_OFF,(info->lcd_ttl_stvh<<20) | (info->lcd_ttl_stvdl<<10) |(info->lcd_ttl_stvdp));
	    
		LCDC_WUINT32(sel, LCDC_TTL1_OFF,(info->lcd_ttl_ckvt<<30) |(info->lcd_ttl_ckvh<<10) | (info->lcd_ttl_ckvd<<0));
	
		LCDC_WUINT32(sel, LCDC_TTL2_OFF,(info->lcd_ttl_oevt<<30) |(info->lcd_ttl_oevh<<10) | (info->lcd_ttl_oevd<<0));
	
		LCDC_WUINT32(sel, LCDC_TTL3_OFF,(info->lcd_ttl_sthh<<26) |(info->lcd_ttl_sthd<<16) | (info->lcd_ttl_oehh<<10) |
		                    (info->lcd_ttl_oehd<<0));
	
		LCDC_WUINT32(sel, LCDC_TTL4_OFF,(info->lcd_ttl_datarate<<23) |(info->lcd_ttl_revsel<<22) | 
							(info->lcd_ttl_datainv_en<<21) | (info->lcd_ttl_datainv_sel<<20) |info->lcd_ttl_revd);

	}
	else if(info->lcd_if == LCDC_LCDIF_CPU)
	{
		LCDC_WUINT32(sel, LCDC_CPUIF_OFF,(info->lcd_cpu_if<<29) |(1<<26));
	}
	else
	{
	   ;
	}
	
	if(info->lcd_frm == LCDC_FRM_RGB666)
	{
		LCDC_CLR_BIT(sel,LCDC_FRM0_OFF,(__u32)0x7<<4);
	}
	else if(info->lcd_frm == LCDC_FRM_RGB656)
	{
		LCDC_INIT_BIT(sel,LCDC_FRM0_OFF,0x7<<4,0x5<<4);			
	}
	else
	{
		LCDC_CLR_BIT(sel,LCDC_FRM0_OFF,LCDC_BIT31); 
	}

	if(info->lcd_frm == LCDC_FRM_RGB666 || info->lcd_frm == LCDC_FRM_RGB656)
	{
    	LCDC_WUINT32(sel, LCDC_FRM1_OFF+0x00,0x11111111);	
    	LCDC_WUINT32(sel, LCDC_FRM1_OFF+0x04,0x11111111);	
       	LCDC_WUINT32(sel, LCDC_FRM1_OFF+0x08,0x11111111);	
    	LCDC_WUINT32(sel, LCDC_FRM1_OFF+0x0c,0x11111111);	
       	LCDC_WUINT32(sel, LCDC_FRM1_OFF+0x10,0x11111111);	
    	LCDC_WUINT32(sel, LCDC_FRM1_OFF+0x14,0x11111111);		    
	    LCDC_WUINT32(sel, LCDC_FRM2_OFF+0x00,0x01010000);	
	 	LCDC_WUINT32(sel, LCDC_FRM2_OFF+0x04,0x15151111);	
	 	LCDC_WUINT32(sel, LCDC_FRM2_OFF+0x08,0x57575555);	
		LCDC_WUINT32(sel, LCDC_FRM2_OFF+0x0c,0x7f7f7777);
		LCDC_SET_BIT(sel,LCDC_FRM0_OFF,LCDC_BIT31);
	}

	LCDC_WUINT32(sel, LCDC_IOCTL0_OFF,info->lcd_io_cfg0);
    LCDC_WUINT32(sel, LCDC_IOCTL1_OFF,info->lcd_io_cfg1);

    LCDC_set_int_line(sel, 0,vblank_len+2);    
}


__s32 TCON0_select_src(__u32 sel, __u8 src)
{
    __u32 tmp; 

    tmp = LCDC_RUINT32(sel, LCDC_CTL_OFF);
    tmp = tmp&0xffbffffc; 
    switch(src)
    {
        case LCDC_SRC_DE1:
             tmp = tmp|0x00;
             break;

        case LCDC_SRC_DE2:
             tmp = tmp|0x01;
             break;
             
        case LCDC_SRC_DMA:
             tmp = tmp|0x02;
             break;

        case LCDC_SRC_WHITE:
             tmp = tmp|0x00400003;
             break;

        case LCDC_SRC_BLACK:
             tmp = tmp|0x03;
             break;
    }
	LCDC_WUINT32(sel,LCDC_CTL_OFF,tmp);
    return 0;
}


__s32 TCON0_get_width(__u32 sel)
{
    return -1;
}
      
__s32 TCON0_get_height(__u32 sel)
{
    return -1;
}

__s32 TCON0_set_dclk_div(__u32 sel, __u8 div)
{
	LCDC_INIT_BIT(sel, LCDC_DCLK_OFF, 0xff, div);
	return 0;
}


#define ____SEPARATOR_TCON1____

__u32 TCON1_open(__u32 sel)
{
	LCDC_SET_BIT(sel, LCDC_HDTVIF_OFF, LCDC_BIT31);
	return 0;
}

__u32 TCON1_close(__u32 sel)
{
	__u32  tmp;

	LCDC_CLR_BIT(sel, LCDC_HDTVIF_OFF, LCDC_BIT31);
	
	tmp = LCDC_RUINT32(sel, LCDC_GCTL_OFF);//?
	tmp &= (~(1 << 0));//disable hdif
	LCDC_WUINT32(sel, LCDC_GCTL_OFF,tmp);

	LCDC_WUINT32(sel, LCDC_IOCTL3_OFF, 0xffffffff);//?

	return 0;
}

__u32  TCON1_cfg(__u32 sel, __tcon1_cfg_t *cfg)
{   
	__u32 vblank_len;
    __u32 reg_val;

    vblank_len = cfg->vt/2 - cfg->src_y - 2;
	if(vblank_len > 30)
	{
		cfg->start_delay	= 29;
	}
	else
	{
		cfg->start_delay	= vblank_len - 2;//23 modify//old:cfg->start_delay	= vblank_len - 1
	}

    if (cfg->b_remap_if)
    {
		LCDC_SET_BIT(sel,LCDC_GCTL_OFF,LCDC_BIT0);
    }
    else
    {
 		LCDC_CLR_BIT(sel,LCDC_GCTL_OFF,LCDC_BIT0);   	
	}
    
    reg_val = LCDC_RUINT32(sel, LCDC_HDTVIF_OFF);
    reg_val &= 0xffeffe0f;
    if (cfg->b_interlace)
    {
        reg_val |= (1<<20);
    }


    reg_val |= ((cfg->start_delay&0x1f)<<4);
    
    LCDC_WUINT32(sel, LCDC_HDTVIF_OFF,reg_val);

    LCDC_WUINT32(sel, LCDC_HDTV0_OFF,(((cfg->src_x - 1)&0xfff)<<16)|((cfg->src_y - 1)&0xfff));
    LCDC_WUINT32(sel, LCDC_HDTV1_OFF,(((cfg->scl_x - 1)&0xfff)<<16)|((cfg->scl_y - 1)&0xfff));
    LCDC_WUINT32(sel, LCDC_HDTV2_OFF,(((cfg->out_x - 1)&0xfff)<<16)|((cfg->out_y - 1)&0xfff));
    LCDC_WUINT32(sel, LCDC_HDTV3_OFF,(((cfg->ht)&0xfff)<<16)|((cfg->hbp)&0xfff));
    LCDC_WUINT32(sel, LCDC_HDTV4_OFF,(((cfg->vt)&0xfff)<<16)|((cfg->vbp - 1)&0xfff));
    LCDC_WUINT32(sel, LCDC_HDTV5_OFF,(((cfg->hspw)&0x3ff)<<16)|((cfg->vspw)&0x3ff));
    LCDC_WUINT32(sel, LCDC_IOCTL2_OFF,cfg->io_pol);//add
    LCDC_WUINT32(sel, LCDC_IOCTL3_OFF,cfg->io_out);//add

	
	LCDC_set_int_line(sel,1, vblank_len+2);

	
    return 0;
}

__u32 TCON1_cfg_ex(__u32 sel, __panel_para_t * info)
{
    __tcon1_cfg_t tcon1_cfg;

    tcon1_cfg.b_interlace = 0;
    tcon1_cfg.b_rgb_internal_hd = 0;
    tcon1_cfg.b_rgb_remap_io = 1;//rgb
    tcon1_cfg.b_remap_if = 1;	//remap tcon1 to io
    tcon1_cfg.src_x = info->lcd_x;
    tcon1_cfg.src_y = info->lcd_y;
    tcon1_cfg.scl_x = info->lcd_x;
    tcon1_cfg.scl_y = info->lcd_y;
    tcon1_cfg.out_x = info->lcd_x;
    tcon1_cfg.out_y = info->lcd_y;
    tcon1_cfg.ht = info->lcd_ht;
    tcon1_cfg.hbp = info->lcd_hbp;
    tcon1_cfg.vt = info->lcd_vt;
    tcon1_cfg.vbp = info->lcd_vbp;
    tcon1_cfg.vspw = info->lcd_hv_vspw;
    tcon1_cfg.hspw = info->lcd_hv_hspw;
    tcon1_cfg.io_pol = info->lcd_io_cfg0;
    tcon1_cfg.io_out = info->lcd_io_cfg1;

    TCON1_cfg(sel, &tcon1_cfg);

    return 0;
}

__u32 TCON1_set_hdmi_mode(__u32 sel, __u8 mode)
{
	__tcon1_cfg_t cfg;
	
	switch(mode)
	{
        case DISP_TV_MOD_480I:
        cfg.b_interlace   = 1;
        cfg.src_x       = 720;
        cfg.src_y       = 240;
        cfg.scl_x       = 720;
        cfg.scl_y       = 240;
        cfg.out_x       = 720;
        cfg.out_y       = 240;
        cfg.ht       = 857;
        cfg.hbp      = 118;
        cfg.vt       = 525;
        cfg.vbp      = 18;
        cfg.vspw     = 2;
        cfg.hspw     = 61; 
        cfg.io_pol      = 0x04000000;
        break;
     case DISP_TV_MOD_576I:
        cfg.b_interlace   = 1;
        cfg.src_x       = 720;
        cfg.src_y       = 288;
        cfg.scl_x       = 720;
        cfg.scl_y       = 288;
        cfg.out_x       = 720;
        cfg.out_y       = 288;
        cfg.ht       = 863;
        cfg.hbp      = 131;
        cfg.vt       = 625;
        cfg.vbp      = 22;
        cfg.vspw     = 2;
        cfg.hspw     = 62; 
        cfg.io_pol      = 0x04000000;
        break;
     case DISP_TV_MOD_480P:
        cfg.b_interlace   = 0;
        cfg.src_x       = 720;
        cfg.src_y       = 480;
        cfg.scl_x       = 720;
        cfg.scl_y       = 480;
        cfg.out_x       = 720;
        cfg.out_y       = 480;
        cfg.ht       = 857;
        cfg.hbp      = 121;
        cfg.vt       = 1050;
        cfg.vbp      = 42 - 6;
        cfg.vspw     = 5;
        cfg.hspw     = 61; 
        cfg.io_pol      = 0x04000000;
        break;
     case DISP_TV_MOD_576P:
        cfg.b_interlace   = 0;
        cfg.src_x       = 720;
        cfg.src_y       = 576;
        cfg.scl_x       = 720;
        cfg.scl_y       = 576;
        cfg.out_x       = 720;
        cfg.out_y       = 576;
        cfg.ht       = 863;
        cfg.hbp      = 131;
        cfg.vt       = 1250;
        cfg.vbp      = 44;
        cfg.vspw     = 4;
        cfg.hspw     = 63; 
        cfg.io_pol      = 0x04000000;
        break;       
    
    case DISP_TV_MOD_720P_50HZ:
        cfg.b_interlace   = 0;
        cfg.src_x      = 1280;
        cfg.src_y      = 720;
        cfg.scl_x      = 1280;
        cfg.scl_y      = 720;
        cfg.out_x      = 1280;
        cfg.out_y      = 720;
        cfg.ht       = 1979;
        cfg.hbp      = 259;
        cfg.vt       = 1500;
        cfg.vbp      = 25;
        cfg.vspw     = 4;
        cfg.hspw     = 39; 
        cfg.io_pol      = 0x07000000;
        break;            
    case DISP_TV_MOD_720P_60HZ:
        cfg.b_interlace   = 0;
        cfg.src_x       = 1280;
        cfg.src_y       = 720;
        cfg.scl_x       = 1280;
        cfg.scl_y       = 720;
        cfg.out_x       = 1280;
        cfg.out_y       = 720;
        cfg.ht       = 1649;
        cfg.hbp      = 259;
        cfg.vt       = 1500;
        cfg.vbp      = 25;
        cfg.vspw     = 4;
        cfg.hspw     = 39; 
        cfg.io_pol      = 0x07000000;
        break;    
    case DISP_TV_MOD_1080I_50HZ:
        cfg.b_interlace   = 1;
        cfg.src_x       = 1920;
        cfg.src_y       = 540;
        cfg.scl_x       = 1920;
        cfg.scl_y       = 540;
        cfg.out_x       = 1920;
        cfg.out_y       = 540;
        cfg.ht       = 2639;
        cfg.hbp      = 191;
        cfg.vt       = 1125;
        cfg.vbp      = 20;
        cfg.vspw     = 4;
        cfg.hspw     = 43;
        cfg.io_pol      = 0x07000000;
        break;
    case DISP_TV_MOD_1080I_60HZ:
        cfg.b_interlace   = 1;
        cfg.src_x       = 1920;
        cfg.src_y       = 540;
        cfg.scl_x       = 1920;
        cfg.scl_y       = 540;
        cfg.out_x       = 1920;
        cfg.out_y       = 540;
        cfg.ht       = 2199;
        cfg.hbp      = 191;
        cfg.vt       = 1125;
        cfg.vbp      = 20;
        cfg.vspw     = 4;
        cfg.hspw     = 43;
        cfg.io_pol      = 0x07000000;
        break;    
    case DISP_TV_MOD_1080P_24HZ:
		cfg.b_interlace   = 0;
        cfg.src_x       = 1920;
        cfg.src_y       = 1080;
        cfg.scl_x       = 1920;
        cfg.scl_y       = 1080;
        cfg.out_x       = 1920;
        cfg.out_y       = 1080;
        cfg.ht       = 2749;
        cfg.hbp      = 191;
        cfg.vt       = 2250;
        cfg.vbp      = 41;
        cfg.vspw     = 4;
        cfg.hspw     = 43;
        cfg.io_pol      = 0x07000000;
        break;
     case DISP_TV_MOD_1080P_50HZ: 
        cfg.b_interlace   = 0;
        cfg.src_x       = 1920;
        cfg.src_y       = 1080;
        cfg.scl_x       = 1920;
        cfg.scl_y       = 1080;
        cfg.out_x       = 1920;
        cfg.out_y       = 1080;
        cfg.ht       = 2639;
        cfg.hbp      = 191;
        cfg.vt       = 2250;
        cfg.vbp      = 41;
        cfg.vspw     = 4;
        cfg.hspw     = 43;
        cfg.io_pol      = 0x07000000; 
        break;  
     case DISP_TV_MOD_1080P_60HZ: 
        cfg.b_interlace   = 0;
        cfg.src_x       = 1920;
        cfg.src_y       = 1080;
        cfg.scl_x       = 1920;
        cfg.scl_y       = 1080;
        cfg.out_x       = 1920;
        cfg.out_y       = 1080;
        cfg.ht       = 2199;
        cfg.hbp      = 191;
        cfg.vt       = 2250;
        cfg.vbp      = 41;
        cfg.vspw     = 4;
        cfg.hspw     = 43;
        cfg.io_pol      = 0x07000000;
        break;
    default:
        return 0;
    }
	cfg.io_out      = 0x00000000;
	cfg.b_rgb_internal_hd = 0;
	cfg.b_rgb_remap_io = 1;//rgb
	cfg.b_remap_if      = 1;
	TCON1_cfg(sel, &cfg);

    return 0;
}

__u32 TCON1_set_tv_mode(__u32 sel, __u8 mode)
{
    __tcon1_cfg_t          cfg;

    switch(mode)
    {
        case DISP_TV_MOD_576I:
        case DISP_TV_MOD_PAL:
       	case DISP_TV_MOD_PAL_SVIDEO:
        case DISP_TV_MOD_PAL_CVBS_SVIDEO:
        case DISP_TV_MOD_PAL_NC:
        case DISP_TV_MOD_PAL_NC_SVIDEO:
        case DISP_TV_MOD_PAL_NC_CVBS_SVIDEO:
            cfg.b_interlace   = 1;
            cfg.src_x       = 720;
            cfg.src_y       = 288;
            cfg.scl_x       = 720;
            cfg.scl_y       = 288;
            cfg.out_x       = 720;
            cfg.out_y       = 288;
            cfg.ht       = 863;
            cfg.hbp      = 138;
            cfg.vt       = 625;
            cfg.vbp      = 22;
            cfg.vspw     = 1;
            cfg.hspw     = 1;  
            break;
            
        case DISP_TV_MOD_480I:
        case DISP_TV_MOD_NTSC:
        case DISP_TV_MOD_NTSC_SVIDEO:
        case DISP_TV_MOD_NTSC_CVBS_SVIDEO:
        case DISP_TV_MOD_PAL_M:
        case DISP_TV_MOD_PAL_M_SVIDEO:
        case DISP_TV_MOD_PAL_M_CVBS_SVIDEO:
            cfg.b_interlace   = 1;
            cfg.src_x       = 720;
            cfg.src_y       = 240;
            cfg.scl_x       = 720;
            cfg.scl_y       = 240;
            cfg.out_x       = 720;
            cfg.out_y       = 240;
            cfg.ht       = 857;
            cfg.hbp      = 117;
            cfg.vt       = 525;
            cfg.vbp      = 18;
            cfg.vspw     = 1;
            cfg.hspw     = 1;
            break;

        case DISP_TV_MOD_480P:
        	cfg.b_interlace   = 0;
            cfg.src_x       = 720;
            cfg.src_y       = 480;
            cfg.scl_x       = 720;
            cfg.scl_y       = 480;
            cfg.out_x       = 720;
            cfg.out_y       = 480;
            cfg.ht       = 857;
            cfg.hbp      = 117;
            cfg.vt       = 1050;
            cfg.vbp      = 22;
            cfg.vspw     = 1;
            cfg.hspw     = 1; 
            break;
            
        case DISP_TV_MOD_576P:
        	cfg.b_interlace   = 0;
            cfg.src_x       = 720;
            cfg.src_y       = 576;
            cfg.scl_x       = 720;
            cfg.scl_y       = 576;
            cfg.out_x       = 720;
            cfg.out_y       = 576;
            cfg.ht       = 863;
            cfg.hbp      = 138;
            cfg.vt       = 1250;
            cfg.vbp      = 22;
            cfg.vspw     = 1;
            cfg.hspw     = 1; 
            break;
			
        case DISP_TV_MOD_720P_50HZ:
       	 	cfg.b_interlace   = 0;
            cfg.src_x       = 1280;
            cfg.src_y       = 720;
            cfg.scl_x       = 1280;
            cfg.scl_y       = 720;
            cfg.out_x       = 1280;
            cfg.out_y       = 720;
            cfg.ht       = 1979;
            cfg.hbp      = 259;
            cfg.vt       = 1500;
            cfg.vbp      = 24;
            cfg.vspw     = 1;
            cfg.hspw     = 1; 
            break;
            
        case DISP_TV_MOD_720P_60HZ:
        	cfg.b_interlace   = 0;
            cfg.src_x       = 1280;
            cfg.src_y       = 720;
            cfg.scl_x       = 1280;
            cfg.scl_y       = 720;
            cfg.out_x       = 1280;
            cfg.out_y       = 720;
            cfg.ht       = 1649;
            cfg.hbp      = 259;
            cfg.vt       = 1500;
            cfg.vbp      = 24;
            cfg.vspw     = 1;
            cfg.hspw     = 1;
            break;

        case DISP_TV_MOD_1080I_50HZ:
            cfg.b_interlace   = 0;
            cfg.src_x       = 1920;
            cfg.src_y       = 540;
            cfg.scl_x       = 1920;
            cfg.scl_y       = 540;
            cfg.out_x       = 1920;
            cfg.out_y       = 540;
            cfg.ht       = 2639;
            cfg.hbp      = 191;
            cfg.vt       = 1125;
            cfg.vbp      = 16;
            cfg.vspw     = 1;
            cfg.hspw     = 1;	
            break;
            
        case DISP_TV_MOD_1080I_60HZ:
            cfg.b_interlace   = 1;
            cfg.src_x       = 1920;
            cfg.src_y       = 540;
            cfg.scl_x       = 1920;
            cfg.scl_y       = 540;
            cfg.out_x       = 1920;
            cfg.out_y       = 540;
            cfg.ht       = 2199;
            cfg.hbp      = 191;
            cfg.vt       = 1125;
            cfg.vbp      = 16;
            cfg.vspw     = 1;
            cfg.hspw     = 1;
            break;
            
        case DISP_TV_MOD_1080P_50HZ:
            cfg.b_interlace   = 0;
            cfg.src_x       = 1920;
            cfg.src_y       = 1080;
            cfg.scl_x       = 1920;
            cfg.scl_y       = 1080;
            cfg.out_x       = 1920;
            cfg.out_y       = 1080;
            cfg.ht       = 2639;
            cfg.hbp      = 191;
            cfg.vt       = 2250;
            cfg.vbp      = 44;
            cfg.vspw     = 1;
            cfg.hspw     = 1;
            break;
            
        case DISP_TV_MOD_1080P_60HZ:
            cfg.b_interlace   = 0;
            cfg.src_x       = 1920;
            cfg.src_y       = 1080;
            cfg.scl_x       = 1920;
            cfg.scl_y       = 1080;
            cfg.out_x       = 1920;
            cfg.out_y       = 1080;
            cfg.ht       = 2199;
            cfg.hbp      = 191;
            cfg.vt       = 2250;
            cfg.vbp      = 44;
            cfg.vspw     = 1;
            cfg.hspw     = 1;
            break;
            
        default:
            return 0;
    }
    cfg.io_pol      = 0x00000000;
    cfg.io_out      = 0x0fffffff;
    cfg.b_rgb_internal_hd = 0;//yuv
    cfg.b_rgb_remap_io = 0;
    cfg.b_remap_if      = 0;
    TCON1_cfg(sel, &cfg);

    return 0;
}


// set mode
////////////////////////////////////////////////////////////////////////////////
__s32 TCON1_set_vga_mode(__u32 sel, __u8 mode)
{
    __tcon1_cfg_t          cfg;

	switch(mode)
	{
	case DISP_VGA_H640_V480:
      cfg.src_x = cfg.scl_x = cfg.out_x = 640;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 480;//VA
      cfg.ht       = 0x31f;//HT-1=-1
      cfg.hbp      = 0x8f;//HS+HBP-1=+-1
      cfg.vt       = 0x41a;//VT*2=*2
      cfg.vbp      = 0x22;//VS+VBP-1=+-1
      cfg.vspw     = 0x1;//VS-1=-1
      cfg.hspw     = 0x5f;//HS-1=-1
		break;
	case DISP_VGA_H800_V600:
      cfg.src_x = cfg.scl_x = cfg.out_x = 800;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 600;//VA
      cfg.ht       = 0x41f;//HT-1=-1
      cfg.hbp      = 0xd7;//HS+HBP-1=+-1
      cfg.vt       = 0x4e8;//VT*2=*2
      cfg.vbp      = 0x1a;//VS+VBP-1=+-1
      cfg.vspw     = 0x3;//VS-1=-1
      cfg.hspw     = 0x7f;//HS-1=-1
		break;
	case  DISP_VGA_H1024_V768:
      cfg.src_x = cfg.scl_x = cfg.out_x = 1024;
      cfg.src_y = cfg.scl_y = cfg.out_y = 768;
      cfg.ht       = 1343;//HT-1=1344-1
      cfg.hbp      = 295;//HS+HBP-1=136+160-1
      cfg.vt       = 1612;//VT*2=806*2
      cfg.vbp      = 34;//VS+VBP-1=6+29-1
      cfg.vspw     = 5;//VS-1=6-1
      cfg.hspw     = 135;//HS-1=136-1
		break;
	case  DISP_VGA_H1280_V1024:
      cfg.src_x = cfg.scl_x = cfg.out_x = 1280;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 1024;//VA
      cfg.ht       = 0x697;//HT-1=-1
      cfg.hbp      = 0x167;//HS+HBP-1=+-1
      cfg.vt       = 0x854;//VT*2=*2
      cfg.vbp      = 0x28;//VS+VBP-1=+-1
      cfg.vspw     = 0x2;//VS-1=-1
      cfg.hspw     = 0x6f;//HS-1=-1
		break;
	case  DISP_VGA_H1360_V768:
      cfg.src_x = cfg.scl_x = cfg.out_x = 1360;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 768;//VA
      cfg.ht       = 0x6ff;//HT-1=-1
      cfg.hbp      = 0x16f;//HS+HBP-1=+-1
      cfg.vt       = 0x636;//VT*2=*2
      cfg.vbp      = 0x17;//VS+VBP-1=+-1
      cfg.vspw     = 0x5;//VS-1=-1
      cfg.hspw     = 0x6f;//HS-1=-1
		break;
	case  DISP_VGA_H1440_V900:
      cfg.src_x = cfg.scl_x = cfg.out_x = 1440;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 900;//VA
      cfg.ht       = 0x76f;//HT-1=-1
      cfg.hbp      = 0x17f;//HS+HBP-1=+-1
      cfg.vt       = 0x74c;//VT*2=*2
      cfg.vbp      = 0x1e;//VS+VBP-1=+-1
      cfg.vspw     = 0x5;//VS-1=-1
      cfg.hspw     = 0x97;//HS-1=-1
		break;
	case  DISP_VGA_H1680_V1050:
      cfg.src_x = cfg.scl_x = cfg.out_x = 1680;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 1050;//VA
      cfg.ht       = 2239;//HT-1=-1
      cfg.hbp      = 463;//HS+HBP-1=+-1
      cfg.vt       = 2178;//VT*2=*2
      cfg.vbp      = 35;//VS+VBP-1=+-1
      cfg.vspw     = 5;//VS-1=-1
      cfg.hspw     = 175;//HS-1=-1
		break;
	case  DISP_VGA_H1920_V1080_RB:
      cfg.src_x = cfg.scl_x = cfg.out_x = 1920;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 1080;//VA
      cfg.ht       = 2016;//HT-1=-1
      cfg.hbp      = 62;//HS+HBP-1=+-1
      cfg.vt       = 2222;//VT*2=*2
      cfg.vbp      = 27;//VS+VBP-1=+-1
      cfg.vspw     = 4;//VS-1=-1
      cfg.hspw     = 31;//HS-1=-1
		break;
	case  DISP_VGA_H1920_V1080://TBD
      cfg.src_x = cfg.scl_x = cfg.out_x = 1920;//HA
      cfg.src_y = cfg.scl_y = cfg.out_y = 1080;//VA
      cfg.ht       = 2200-1;//HT-1=-1
      cfg.hbp      = 148+44-1;//HS+HBP-1=+-1
      cfg.vt       = 1125*2;//VT*2=*2
      cfg.vbp      = 36+5;//VS+VBP-1=+-1
      cfg.vspw     = 5-1;//VS-1=-1
      cfg.hspw     = 44-1;//HS-1=-1	
      cfg.io_pol   = 0x03000000;	
		break;
	default:
		return 0;
	}
    cfg.b_interlace   = 0;
    cfg.io_pol      = 0x00000000;
    cfg.io_out      = 0x0cffffff;//hs vs is use
    cfg.b_rgb_internal_hd = 1;//rgb
    cfg.b_rgb_remap_io = 0;
    cfg.b_remap_if      = 1;
    TCON1_cfg(sel, &cfg);

    return 0;
}


__s32 TCON1_select_src(__u32 sel, __u8 src)
{
    __u32 tv_tmp; 

	tv_tmp = LCDC_RUINT32(sel, LCDC_HDTVIF_OFF);

    tv_tmp = tv_tmp&0xfffffffc; 
	if(src == LCDC_SRC_DE1)
	{
		tv_tmp = tv_tmp|0x00; 
	}
	else if(src == LCDC_SRC_DE2)
	{
		tv_tmp = tv_tmp|0x01; 
	}
	else if(src == LCDC_SRC_BLUE)
	{
		tv_tmp = tv_tmp|0x02;
	}
	
	LCDC_WUINT32(sel, LCDC_HDTVIF_OFF,tv_tmp);

	return 0;
}


__bool TCON1_in_valid_regn(__u32 sel, __u32 juststd)			//???
{
   __u32         readval;
   __u32         SY2;
   __u32         VT;

   readval      = LCDC_RUINT32(sel, LCDC_HDTV4_OFF);
   VT           = (readval & 0xffff0000)>>17;
   
   readval      = LCDC_RUINT32(sel, LCDC_DUBUG_OFF);
   SY2          = (readval)&0xfff;

   if((SY2 < juststd) ||(SY2 > VT))
   {
       return 1;
   }
   else
   {
       return 0;
   }  
}     

__s32 TCON1_get_width(__u32 sel)
{
    return -1;
}
      
__s32 TCON1_get_height(__u32 sel)
{
    return -1;
}

__s32 TCON1_set_gamma_table(__u32 sel, __u32 address,__u32 size)	//add next time
{
	return -1;	
}

__s32 TCON1_set_gamma_Enable(__u32 sel, __bool enable)	//add next time
{
	return -1;
}

#define ____SEPARATOR_CPU____

//__asm void my_stmia(int addr,int data1,int data2)
//{
//    stmia r0!, {r1,r2}
//    BX    lr
//}

void LCD_CPU_Burst_Write(__u32 sel, int addr,int data1,int data2)
{    
	//my_stmia(LCDC_GET_REG_BASE(sel) + addr,data1,data2);
}

//from 19
__u32 LCD_CPU_IO_extend(__u32 value)
{
	return
		  ((value & 0xfc00)<<8)
		| ((value & 0x0300)<<6)
		| ((value & 0x00e0)<<5)	
		| ((value & 0x001f)<<3);
}

__u32 LCD_CPU_IO_shrink(__u32 value)
{
	return
		  ((value & 0xfc0000)>>8)
		| ((value & 0x00c000)>>6)
		| ((value & 0x001c00)>>5)	
		| ((value & 0x0000f8)>>3);
}

void LCD_CPU_WR_24b(__u32 sel, __u32 index, __u32 data)
{
	__u32 lcd_cpu;

	LCDC_CLR_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT25);         		//ca =0
	LCD_CPU_Burst_Write(sel, LCDC_CPUWR_OFF, index, index);		// write data on 8080 bus

	do{
		lcd_cpu = LCDC_RUINT32(sel, LCDC_CPUIF_OFF);
	} while(lcd_cpu&LCDC_BIT23);                             	//check wr finish


	LCDC_SET_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT25);     			//ca =1
	LCD_CPU_Burst_Write(sel, LCDC_CPUWR_OFF, data,data);

	do{
		lcd_cpu = LCDC_RUINT32(sel, LCDC_CPUIF_OFF);
	} while(lcd_cpu&LCDC_BIT23);                             //check wr finish
}

void LCD_CPU_WR_INDEX_24b(__u32 sel, __u32 index)
{
	__u32 lcd_cpu;

	LCDC_CLR_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT25);         		//ca =0
	LCD_CPU_Burst_Write(sel, LCDC_CPUWR_OFF, index,index);		// write data on 8080 bus

	do{
		lcd_cpu = LCDC_RUINT32(sel, LCDC_CPUIF_OFF);
	} while(lcd_cpu&LCDC_BIT23);                             	//check wr finish
}

void LCD_CPU_WR_DATA_24b(__u32 sel, __u32 data)
{
	__u32 lcd_cpu;

	LCDC_SET_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT25);     			//ca =1
	LCD_CPU_Burst_Write(sel, LCDC_CPUWR_OFF, data,data);

	do{
		lcd_cpu = LCDC_RUINT32(sel, LCDC_CPUIF_OFF);
	} while(lcd_cpu&LCDC_BIT23);                             	//check wr finish
}

void LCD_CPU_RD_24b(__u32 sel, __u32 index, __u32 *data)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////

void LCD_CPU_WR(__u32 sel, __u32 index, __u32 data)
{
	LCD_CPU_WR_24b(sel,LCD_CPU_IO_extend(index),LCD_CPU_IO_extend(data));
}

void LCD_CPU_WR_INDEX(__u32 sel, __u32 index)
{
    LCD_CPU_WR_INDEX_24b(sel,LCD_CPU_IO_extend(index));
}

void LCD_CPU_WR_DATA(__u32 sel, __u32 data)
{
	LCD_CPU_WR_DATA_24b(sel,LCD_CPU_IO_extend(data));
}

void LCD_CPU_RD(__u32 sel, __u32 index, __u32 *data)
{
	
}

void LCD_CPU_AUTO_FLUSH(__u32 sel, __u8 en)
{
	if(en ==0)
		LCDC_CLR_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT28);
	else
		LCDC_SET_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT28);   
}

void LCD_CPU_DMA_FLUSH(__u32 sel, __u8 en)
{
	if(en ==0)
		LCDC_CLR_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT27);
	else
		LCDC_SET_BIT(sel, LCDC_CPUIF_OFF,LCDC_BIT27);
}

void LCD_XY_SWAP(__u32 sel)	
{
	__u32 reg,x,y;
	reg = LCDC_RUINT32(sel, LCDC_BASIC0_OFF);
	y   = reg & 0x7ff;
	x   = (reg>>16) & 0x7ff;
	LCDC_WUINT32(sel, LCDC_BASIC0_OFF,(y<<16) | x);
}


#define ____TCON_MUX_CTL____

__u8 TCON_mux_init(void)
{
	LCDC_CLR_BIT(0,LCDC_MUX_CTRL,LCDC_BIT31); 
	LCDC_INIT_BIT(0,LCDC_MUX_CTRL,0xf<<4,0<<4);		
	LCDC_INIT_BIT(0,LCDC_MUX_CTRL,0xf,1);
	return 0;
}

__u8 TCON_hdmi_src(__u8 src)
{
	if(src>2)
		src = 2;
	LCDC_INIT_BIT(0,LCDC_MUX_CTRL,0xf<<8,src<<8);	//hdmi output select
	return 0;	
}
