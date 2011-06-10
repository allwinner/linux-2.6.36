#include "hdmi_core.h"


static __u32 HDMI_BASE = 0xf1c16000;

#define get_wvalue(n)   (*((volatile __u32 *)(n)))          /* word input */
#define put_wvalue(n,c) (*((volatile __u32 *)(n))  = (c))   /* word output */
#define get_value(n)    (*((volatile __u8 *)(n)))          /* byte input */
#define put_value(n,c)  (*((volatile __u8 *)(n))  = (c))   /* byte output */
#define get_hvalue(n)   (*((volatile __u16 *)(n)))         /* half word input */
#define put_hvalue(n,c) (*((volatile __u16 *)(n)) = (c))   /* half word output */

void HDMI_set_reg_base(__u32 base)
{
    HDMI_BASE = base;
}

void HDMI_SET(void)
{
	int reg_val;
    //hdmi setting
    put_wvalue(HDMI_BASE + 0x004,0x00000000); 
    put_wvalue(HDMI_BASE + 0x008,0xffff0000);		//interrupt mask
    //video setting
    put_wvalue(HDMI_BASE + 0x010,0xc0000000);
    
    #ifdef HDMI1080I_50
    put_wvalue(HDMI_BASE + 0x010,0xc0000010);              //video enable and hdmi mode
    #endif
    #ifdef HDMI1080I_60
    put_wvalue(HDMI_BASE + 0x010,0xc0000010);              //video enable and hdmi mode
    #endif
    
    #ifdef PIX_13M5
        reg_val = get_wvalue(HDMI_BASE + 0x010); 
     	put_wvalue(HDMI_BASE + 0x010,reg_val|0x11);				//interlace and repeat
     	put_hvalue(HDMI_BASE + 0x014,(INPUTX<<1) -1);              //active H
     	put_hvalue(HDMI_BASE + 0x018,(HBP<<1)    -1);                //active HBP
     	put_hvalue(HDMI_BASE + 0x01c,(HFP<<1)    -1);                //active HFP
     	put_hvalue(HDMI_BASE + 0x020,(HPSW<<1)   -1);               //active HSPW
    #else
     	put_hvalue(HDMI_BASE + 0x014,(INPUTX<<0) -1);              //active H
     	put_hvalue(HDMI_BASE + 0x018,(HBP<<0)    -1);                //active HBP
     	put_hvalue(HDMI_BASE + 0x01c,(HFP<<0)    -1);                //active HFP
     	put_hvalue(HDMI_BASE + 0x020,(HPSW<<0)   -1);               //active HSPW
    #endif    
    put_hvalue(HDMI_BASE + 0x016,INPUTY   -1);             //active V
    put_hvalue(HDMI_BASE + 0x01a,VBP    -1);               //active VBP
    put_hvalue(HDMI_BASE + 0x01e,VFP   -1);                //active VFP
    put_hvalue(HDMI_BASE + 0x022,VPSW   -1);              	//active VSPW
    
    put_hvalue(HDMI_BASE + 0x024,0x00   );                	//Vsync/Hsync pol
    
    #ifdef HDMI1080P_50    
    put_hvalue(HDMI_BASE + 0x024,0x03   );                	//Vsync/Hsync pol
    #endif
    #ifdef HDMI1080P_60    
    put_hvalue(HDMI_BASE + 0x024,0x03   );                	//Vsync/Hsync pol
    #endif
    #ifdef HDMI1080I_50    
    put_hvalue(HDMI_BASE + 0x024,0x03   );                	//Vsync/Hsync pol
    #endif
    #ifdef HDMI1080I_60    
    put_hvalue(HDMI_BASE + 0x024,0x03   );                	//Vsync/Hsync pol
    #endif    
    #ifdef HDMI720P_50    
    put_hvalue(HDMI_BASE + 0x024,0x03   );                	//Vsync/Hsync pol
    #endif
    #ifdef HDMI720P_60    
    put_hvalue(HDMI_BASE + 0x024,0x03   );                	//Vsync/Hsync pol
    #endif    
    
        
    put_hvalue(HDMI_BASE + 0x026,0x03e0 );                	//TX clock sequence
    //audio setting
    #ifdef AUDIO_DDMA
       #ifdef AUDIO_2CH
          put_wvalue(HDMI_BASE + 0x044,0x00000000);             	//audio fifo rst and select ddma, 2 ch 16bit pcm 
          put_wvalue(HDMI_BASE + 0x048,0x00000001);					//ddma,pcm layout0 2ch
          
          put_wvalue(HDMI_BASE + 0x0A0,0x710a0184);             	//audio infoframe head 
          put_wvalue(HDMI_BASE + 0x0A4,0x00000000);             	//CA = 0X1F 
          put_wvalue(HDMI_BASE + 0x0A8,0x00000000); 
          put_wvalue(HDMI_BASE + 0x0Ac,0x00000000);           
       #else
          put_wvalue(HDMI_BASE + 0x044,0x00000000);             	//audio fifo rst and select ddma, 2 ch 16bit pcm 
          put_wvalue(HDMI_BASE + 0x048,0x0000000f);					//ddma,pcm layout1 8ch
          
          put_wvalue(HDMI_BASE + 0x0A0,0x520a0184);             	//audio infoframe head 
          put_wvalue(HDMI_BASE + 0x0A4,0x1F000000);             	//CA = 0X1F 
          put_wvalue(HDMI_BASE + 0x0A8,0x00000000); 
          put_wvalue(HDMI_BASE + 0x0Ac,0x00000000); 
       #endif      
    #endif
    #ifdef AUDIO_NDMA
       #ifdef AUDIO_2CH
          put_wvalue(HDMI_BASE + 0x044,0x80000000);             	//audio fifo rst and select ndma, 2 ch 16bit pcm 
          put_wvalue(HDMI_BASE + 0x048,0x00000001);					//ddma,pcm layout0 2ch
          
          put_wvalue(HDMI_BASE + 0x0A0,0x710a0184);             	//audio infoframe head 
          put_wvalue(HDMI_BASE + 0x0A4,0x00000000);             	//CA = 0X1F 
          put_wvalue(HDMI_BASE + 0x0A8,0x00000000); 
          put_wvalue(HDMI_BASE + 0x0Ac,0x00000000);          
       #else
          put_wvalue(HDMI_BASE + 0x044,0x80000000);             	//audio fifo rst and select ndma, 2 ch 16bit pcm 
          put_wvalue(HDMI_BASE + 0x048,0x0000000f);					//ddma,pcm layout1 8ch
          
          put_wvalue(HDMI_BASE + 0x0A0,0x520a0184);             	//audio infoframe head
          put_wvalue(HDMI_BASE + 0x0A4,0x1F000000);             	//CA = 0X1F 
          put_wvalue(HDMI_BASE + 0x0A8,0x00000000); 
          put_wvalue(HDMI_BASE + 0x0Ac,0x00000000); 
       #endif          
    #endif
    put_wvalue(HDMI_BASE + 0x04c,0x76543210);
    put_wvalue(HDMI_BASE + 0x050,CTS);                   	//CTS and N 
    put_wvalue(HDMI_BASE + 0x054,ACRN);
    put_wvalue(HDMI_BASE + 0x058,CH_STATUS0 );
    put_wvalue(HDMI_BASE + 0x05c,CH_STATUS1 );
    

    put_wvalue(HDMI_BASE + 0x040,0x80000000);

    
    
    //avi packet
    put_value(HDMI_BASE + 0x080,0x82);
    put_value(HDMI_BASE + 0x081,0x02);
    put_value(HDMI_BASE + 0x082,0x0d);
    put_value(HDMI_BASE + 0x083,0xF7);
    
    put_value(HDMI_BASE + 0x084,0x1E);
    put_value(HDMI_BASE + 0x085,0x58);
    put_value(HDMI_BASE + 0x086,0x00); 
    put_value(HDMI_BASE + 0x087,VIC	);
    put_value(HDMI_BASE + 0x088,AVI_PR);
    
    put_value(HDMI_BASE + 0x089,0x00);
    put_value(HDMI_BASE + 0x08a,0x00);
    put_value(HDMI_BASE + 0x08b,0x00);
    put_value(HDMI_BASE + 0x08c,0x00);
    put_value(HDMI_BASE + 0x08d,0x00);
    put_value(HDMI_BASE + 0x08e,0x00);
    put_value(HDMI_BASE + 0x08f,0x00);
    put_value(HDMI_BASE + 0x090,0x00);    
    //gcp packet
    put_wvalue(HDMI_BASE + 0x0e0,0x00000003);
    put_wvalue(HDMI_BASE + 0x0e4,0x00000000);
    
    //vendor infoframe
    put_value(HDMI_BASE + 0x240,0x81);
    put_value(HDMI_BASE + 0x241,0x01);
    put_value(HDMI_BASE + 0x242,15  );	//length
    
    put_value(HDMI_BASE + 0x243,0x21);	//pb0:checksum
    put_value(HDMI_BASE + 0x244,0x03);	//pb1-3:24bit ieee id
    put_value(HDMI_BASE + 0x245,0x0c);  //length
    put_value(HDMI_BASE + 0x246,0x00);
    
    put_value(HDMI_BASE + 0x247,0x40);   //pb4
    put_value(HDMI_BASE + 0x248,0x88);	//pb5:3d present, side by side half
    put_value(HDMI_BASE + 0x249,0x00);  //pb6:extra data for 3d
    put_value(HDMI_BASE + 0x24a,0x08);  //pb7: matadata type=0,len=8
    
    put_value(HDMI_BASE + 0x24b,0x00);
    put_value(HDMI_BASE + 0x24c,0x00);
    put_value(HDMI_BASE + 0x24d,0x00);
    put_value(HDMI_BASE + 0x24e,0x00);
    put_value(HDMI_BASE + 0x24f,0x00);
    put_value(HDMI_BASE + 0x250,0x00);
    put_value(HDMI_BASE + 0x251,0x00);
    put_value(HDMI_BASE + 0x252,0x00);
    
    //packet config
    put_wvalue(HDMI_BASE + 0x2f0,0x00005321);
    put_wvalue(HDMI_BASE + 0x2f4,0x0000000f);    
    
    //////////////////////    
    //hdmi pll setting 
    put_wvalue(HDMI_BASE + 0x208,(1<<31)+ (1<<30)+ (1<<29)+ (3<<27)+ (0<<26)+ 
    		                     (0<<25)+ (0<<24)+ (0<<23)+ (4<<20)+ (7<<17)+
    		                     (15<<12)+ (7<<8)+ (clkdiv<<4)+(8<<0) );	
    // tx driver setting
    put_wvalue(HDMI_BASE + 0x200,0xfe800000);   	//txen enable
    
    put_wvalue(HDMI_BASE + 0x204,0x00C0C020);   			//
    /*
    //cec hpd setting
    put_wvalue(HDMI_BASE + 0x214,0x00000808);   	//en
    set_wbit(HDMI_BASE + 0x214,BIT9);				//cec output 1
    set_wbit(HDMI_BASE + 0x214,BIT1);				//hpd output 1
    //while((get_wvalue(HDMI_BASE + 0x214)& BIT8) ==0);
    get_wvalue(HDMI_BASE + 0x214);
    get_wvalue(HDMI_BASE + 0x00c);
    clr_wbit(HDMI_BASE + 0x214,BIT9);				//cec output 0
    clr_wbit(HDMI_BASE + 0x214,BIT1);				//cec output 0
    //while((get_wvalue(HDMI_BASE + 0x214)& BIT8) ==1);
    get_wvalue(HDMI_BASE + 0x214);
    get_wvalue(HDMI_BASE + 0x00c);*/
    
    /*
    //dedicated dma setting  aw1623 env  
    put_wvalue(0x01c023a4,0x40c00000);	//ddma ch5 seting from addr =0x40c00000
    put_wvalue(0x01c023a8,0x00000000);	//des =0
    put_wvalue(0x01c023ac,0x01f00000);	//byte to trans
    put_wvalue(0x01c023b8,(31<<24) +(7<<16) + (31<<8) +(7<<0));	//data block and wait cycle
    put_wvalue(0x01c023a0,0xa4b80481); 	//from src0 to des1,continous mode
*/
    reg_val = get_wvalue(HDMI_BASE + 0x300); 
    reg_val |= (1<<27);
    put_wvalue(HDMI_BASE + 0x300,reg_val);
    //set_wbit(HDMI_BASE + 0x300,BIT27);

    put_wvalue(HDMI_BASE + 0x004,0x80000000);	//start hdmi controller	

    //for test only
    //reg_val = get_wvalue(HDMI_BASE + 0x010) & (~(1<<5)); 
 	//put_wvalue(HDMI_BASE + 0x010,reg_val| (1<<5));				//interlace and repeat
}









