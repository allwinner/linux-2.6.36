
#include "disp_video.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_scaler.h"
#include "disp_de.h"

frame_para_t g_video[2];


static __inline __s32 Hal_Set_Frame(__u32 sel, __u32 tcon_index, __u32 scaler_index)
{	
	__scal_buf_addr_t scal_addr;
    __scal_src_size_t in_size;
    __scal_out_size_t out_size;
    __scal_src_type_t in_type;
    __scal_out_type_t out_type;
    __scal_scan_mod_t in_scan;
    __scal_scan_mod_t out_scan;
    __disp_scaler_t * scaler;
    __disp_video_fb_t * video_fb;
    __u32 pre_frame_addr = 0;
    __u32 maf_flag_addr = 0;
    __u32 maf_linestride = 0;
    __u32 cur_line = 0, start_delay = 0;
    
    cur_line = LCDC_get_cur_line(sel, tcon_index);
    start_delay = LCDC_get_start_delay(0, tcon_index);
	if(cur_line > start_delay-5)
	{
	    //DE_INF("cur_line(%d) >= start_delay(%d)-3 in Hal_Set_Frame\n", cur_line, start_delay);
		return DIS_FAIL;
	}

    memcpy(&g_video[scaler_index].video_cur, &g_video[scaler_index].video_new, sizeof(__disp_video_fb_t));
    scaler = &(gdisp.scaler[scaler_index]);
    video_fb = &(g_video[scaler_index].video_cur);

	if(g_video[scaler_index].dit_enable == FALSE)
	{
	    g_video[scaler_index].fetch_field = FALSE;
	    g_video[scaler_index].fetch_bot = FALSE;
	    g_video[scaler_index].dit_mode = DIT_MODE_WEAVE;
	    g_video[scaler_index].tempdiff_en = FALSE;
	    g_video[scaler_index].diagintp_en = FALSE;
    }
    else
    {
    	if(g_video[scaler_index].field_cnt != 0)
    	{
    		g_video[scaler_index].fetch_bot = (g_video[scaler_index].fetch_bot == 0)?1:0;
    		g_video[scaler_index].field_cnt++;
    	}

    	pre_frame_addr = (__u32)OSAL_VAtoPA((void*)g_video[scaler_index].pre_frame_addr0);
    	maf_flag_addr = (__u32)OSAL_VAtoPA((void*)g_video[scaler_index].video_cur.flag_addr);
    	
		maf_linestride =  g_video[scaler_index].video_cur.flag_stride;
    }
    
	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.format);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.seq);
	in_type.byte_seq = 0;

	scal_addr.ch0_addr= (__u32)OSAL_VAtoPA((void*)(video_fb->addr[0]));
	scal_addr.ch1_addr= (__u32)OSAL_VAtoPA((void*)(video_fb->addr[1]));
	scal_addr.ch2_addr= (__u32)OSAL_VAtoPA((void*)(video_fb->addr[2]));

	in_size.src_width = scaler->in_fb.size.width;
	in_size.x_off =  scaler->src_win.x;
	in_size.y_off =  scaler->src_win.y;
	in_size.scal_height=  scaler->src_win.height;
	in_size.scal_width=  scaler->src_win.width;

	out_type.byte_seq =  scaler->out_fb.seq;
	out_type.fmt =  scaler->out_fb.format;

	out_size.width =  scaler->out_size.width;
	out_size.height =  scaler->out_size.height;

	in_scan.field = g_video[scaler_index].fetch_field;
	in_scan.bottom = g_video[scaler_index].fetch_bot;

	out_scan.field = (gdisp.screen[sel].de_flicker_status == DE_FLICKER_USED)?0: gdisp.screen[sel].b_out_interlace;
    
	if(scaler->out_fb.cs_mode > DISP_VXYCC)
	{
		scaler->out_fb.cs_mode = DISP_BT601;
	}	

    if(scaler->in_fb.b_trd_src)
    {
        __scal_3d_inmode_t inmode;
        __scal_3d_outmode_t outmode = 0;
        __scal_buf_addr_t scal_addr_right;

        inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
        outmode = Scaler_3d_sw_para_to_reg(1, gdisp.screen[sel].trd_out_mode,gdisp.screen[sel].b_out_interlace);
        
        DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
        if(gdisp.screen[sel].b_trd_out)
        {
            DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
        }
    	scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(video_fb->addr[0]));
    	scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(video_fb->addr[1]));
    	scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(video_fb->addr[2]));

        DE_SCAL_Set_3D_Ctrl(scaler_index, gdisp.screen[sel].b_trd_out, inmode, outmode);
        DE_SCAL_Config_3D_Src(scaler_index, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
    }
    else
    {
	    DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
	}

	if(g_video[scaler_index].dit_enable == TRUE && gdisp.screen[sel].de_flicker_status == DE_FLICKER_USED)
	{	
		Disp_de_flicker_enable(sel, FALSE);
	}
	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, FALSE);
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type,  scaler->smooth_mode);
	DE_SCAL_Set_Di_Ctrl(scaler_index,g_video[scaler_index].dit_enable,g_video[scaler_index].dit_mode,g_video[scaler_index].diagintp_en,g_video[scaler_index].tempdiff_en);
	DE_SCAL_Set_Di_PreFrame_Addr(scaler_index, pre_frame_addr);
	DE_SCAL_Set_Di_MafFlag_Src(scaler_index, maf_flag_addr, maf_linestride);
    DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);

    gdisp.scaler[scaler_index].b_reg_change = TRUE;
    
	return DIS_SUCCESS;
}


__s32 Video_Operation_In_Vblanking(__u32 sel, __u32 tcon_index)
{	
    __u32 scaler_index = 0;

    for(scaler_index = 0; scaler_index<2; scaler_index++)
    {
        if((gdisp.scaler[scaler_index].status & SCALER_USED) && 
            (g_video[scaler_index].enable == TRUE) && 
            (g_video[scaler_index].have_got_frame == TRUE) &&
            (gdisp.scaler[scaler_index].screen_index == sel)) 
        {
    		Hal_Set_Frame(sel, tcon_index, scaler_index);
    	}
    }
    
	return DIS_SUCCESS;
}

__s32 BSP_disp_video_set_fb(__u32 sel, __u32 hid, __disp_video_fb_t *in_addr)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(gdisp.screen[sel].layer_manage[hid].status & LAYER_USED && 
        gdisp.screen[sel].layer_manage[hid].para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        __u32 scaler_index = 0;

        scaler_index = gdisp.screen[sel].layer_manage[hid].scaler_index;
    	memcpy(&g_video[scaler_index].video_new, in_addr, sizeof(__disp_video_fb_t));
    	g_video[scaler_index].have_got_frame = TRUE;

		if(g_video[scaler_index].video_new.interlace == TRUE)
		{
			g_video[scaler_index].dit_enable = FALSE;

			if(g_video[scaler_index].dit_enable == TRUE)
			{
    			if(g_video[scaler_index].video_new.maf_valid == TRUE)
    			{
    				g_video[scaler_index].dit_mode = DIT_MODE_MAF;
    			}
    			else
    			{
    				g_video[scaler_index].dit_mode = DIT_MODE_MAF_BOB;
    			}

    			if(g_video[scaler_index].video_new.pre_frame_valid == TRUE)
    			{
    				g_video[scaler_index].tempdiff_en = TRUE;
    			}
    			else
    			{
    				g_video[scaler_index].tempdiff_en = FALSE;
    			}
    			g_video[scaler_index].diagintp_en = TRUE;
			}

			if(g_video[scaler_index].video_new.top_field_first == TRUE)
			{
				g_video[scaler_index].fetch_bot = FALSE;
			}
			else
			{
				g_video[scaler_index].fetch_bot = TRUE;
			}
			g_video[scaler_index].field_cnt = 0;
			
		}
		else
		{
			g_video[scaler_index].dit_enable = FALSE;
		}
		
		g_video[scaler_index].pre_frame_addr0 = g_video[scaler_index].video_cur.addr[0];
    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
}


__s32 BSP_disp_video_get_frame_id(__u32 sel, __u32 hid)//get the current displaying frame id
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(gdisp.screen[sel].layer_manage[hid].status & LAYER_USED && 
        gdisp.screen[sel].layer_manage[hid].para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        __u32 scaler_index = 0;

        scaler_index = gdisp.screen[sel].layer_manage[hid].scaler_index;
        if(g_video[scaler_index].have_got_frame == TRUE)
        {
            return g_video[scaler_index].video_cur.id;
        }
        else
        {
            return DIS_FAIL;
        }
    }
    else
    {
        return DIS_FAIL;
    }
}

__s32 BSP_disp_video_get_dit_info(__u32 sel, __u32 hid, __disp_dit_info_t * dit_info)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if((gdisp.screen[sel].layer_manage[hid].status & LAYER_USED) && 
        (gdisp.screen[sel].layer_manage[hid].para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        __u32 scaler_index = 0;

    	dit_info->maf_enable = FALSE;
    	dit_info->pre_frame_enable = FALSE;
    	
    	if(g_video[scaler_index].dit_enable)
    	{
    		if(g_video[scaler_index].dit_mode == DIT_MODE_MAF)
    		{
    			dit_info->maf_enable = TRUE;
    		}
    		if(g_video[scaler_index].tempdiff_en)
    		{	
    			dit_info->pre_frame_enable = TRUE;
    		}
    	}
	}
	return DIS_SUCCESS;
}

__s32 BSP_disp_video_start(__u32 sel, __u32 hid)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if((gdisp.screen[sel].layer_manage[hid].status & LAYER_USED) && 
        (gdisp.screen[sel].layer_manage[hid].para.mode == DISP_LAYER_WORK_MODE_SCALER))
    {
        __u32 scaler_index = 0;

        scaler_index = gdisp.screen[sel].layer_manage[hid].scaler_index;
        memset(&g_video[scaler_index], 0, sizeof(frame_para_t));
        g_video[scaler_index].video_cur.id = -1;
        g_video[scaler_index].enable = TRUE;

    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
}

__s32 BSP_disp_video_stop(__u32 sel, __u32 hid)
{
    hid = HANDTOID(hid);
    HLID_ASSERT(hid, gdisp.screen[sel].max_layers);

    if(gdisp.screen[sel].layer_manage[hid].status & LAYER_USED && 
        gdisp.screen[sel].layer_manage[hid].para.mode == DISP_LAYER_WORK_MODE_SCALER)
    {
        __u32 scaler_index = 0;
        
        scaler_index = gdisp.screen[sel].layer_manage[hid].scaler_index;
        memset(&g_video[scaler_index], 0, sizeof(frame_para_t));

    	return DIS_SUCCESS;
    }
    else
    {
        return DIS_FAIL;
    }
    
}

