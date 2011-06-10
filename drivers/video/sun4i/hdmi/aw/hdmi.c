
#include "common.h"
#include "type.h"
#include "system.h"
#include "aw_math.h"
#include "irq.h"
#include "hdmi.h"
#include "lcd_tv.h"


void TCON1_SET()
{
    put_wvalue( TCON_BASE + 0x000,0x00000000);
    put_wvalue( TCON_BASE + 0x090,0x00000000);
    
    put_hvalue( TCON_BASE + 0x094,INPUTY  -1);            	//input Y
    put_hvalue( TCON_BASE + 0x096,INPUTX  -1);            	//input X               
    put_hvalue( TCON_BASE + 0x098,INPUTY  -1);            	//line sacle output Y: 
    put_hvalue( TCON_BASE + 0x09a,INPUTX  -1);            	//line sacle output X: 
    put_hvalue( TCON_BASE + 0x09c,INPUTY  -1);            	//output size Y 
    put_hvalue( TCON_BASE + 0x09e,INPUTX  -1);            	//output size X 
    put_hvalue( TCON_BASE + 0x0a0,HBP     -1);            	//HBP 192
    put_hvalue( TCON_BASE + 0x0a2,HT  	  -1);            	//HT  2200 
    put_hvalue( TCON_BASE + 0x0a4,VBP     -1);            	//VBP 41
    put_hvalue( TCON_BASE + 0x0a6,VT        );            	//VT  1125 line
    put_hvalue( TCON_BASE + 0x0a8,HPSW    -1);        	 	//HSPW 44
    put_hvalue( TCON_BASE + 0x0aa,VPSW    -1);            	//VSPW 5 
    
 	put_wvalue( TCON_BASE + 0x100, 0x00000000);				//tcon1_enable
 	
  	put_wvalue( TCON_BASE + 0x090, 0x80000020);				//tcon1_enable
	put_wvalue( TCON_BASE + 0x000, 0x80000000);				//tcon_enable     
    
//    put_wvalue( TCON_BASE + 0x090,0x8a200008);         	//enable ,remap ,gamma,RGB444
//    put_wvalue( TCON_BASE + 0x000,0x80000000);         	//enable the whole module
//    put_wvalue( TCON_BASE + 0x0f0,0x00000000);         	//enable 
//    put_wvalue( TCON_BASE + 0x0f4,0x00000000);         	//enable
}


void HDMI_SET()
{
    int i,reg_val;
    TCON1_SET();

    //hdmi setting
    put_wvalue(HDMI_BASE + 0x004,0x00000000);                  
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
    
    put_wvalue(HDMI_BASE + 0x004,0x80000000);
    
    
    //hdmi pll setting 
    #ifdef PIX_13M5
       put_wvalue(HDMI_BASE + 0x20c,0);		//fpga for 480/576i
    #else
       put_wvalue(HDMI_BASE + 0x20c,1);     //fpga for 480/576p
    #endif
    put_wvalue(HDMI_BASE + 0x200,0x01000000);   //tx en
    

    //dedicated dma setting  aw1620 env  
    put_wvalue(0x01c023a4,0x80800000);	//ddma ch4 seting from addr =0
    put_wvalue(0x01c023a8,0x00000000);	//des =0
    put_wvalue(0x01c023ac,0x01f00000);	//byte to trans
    put_wvalue(0x01c023a0,0xa2820281); 	//from src0 to des1,continous mode
    //while(get_wvalue(0x01c020a0)&0x80000000);//wait for finish


}

//############################################################################################
void send_ini_sequence()
{
    int i,j;
    bit_1(HDMI_BASE + 0x524,BIT3);
    for(i=0;i<9;i++)
    {
       for(j=0;j<200;j++)
          bit_0(HDMI_BASE + 0x524,BIT2);	
       
       for(j=0;j<200;j++)
          bit_1(HDMI_BASE + 0x524,BIT2);   
    	
    }
    bit_0(HDMI_BASE + 0x524,BIT3);
    
    return;
	bit_1(HDMI_BASE + 0x540,BIT24);			//enable 9 bit sequence
	put_value (HDMI_BASE + 0x520,1);		//set and cmd
	bit_1(HDMI_BASE + 0x500,BIT30);
	
	while(get_wvalue(HDMI_BASE + 0x500)&0x40000000);
	bit_0(HDMI_BASE + 0x540,BIT24);			//disable 9 bit sequence	
	
	
}
void DDC_Init()
{
	put_wvalue(HDMI_BASE + 0x500,0x80000001);
	while(get_wvalue(HDMI_BASE + 0x500) &0x01);	
	
	put_value(HDMI_BASE + 0x528,0x0c   );		//N = 4,M=1 Fscl= Ftmds/2/10/2^N/(M+1)
	
	put_value(HDMI_BASE + 0x506,0x60   );		//ddc address  0x60
	put_value(HDMI_BASE + 0x504,0xa0>>1);		//slave address  0xa0
	send_ini_sequence();
	
}

void DDC_Read(char cmd,char pointer,char offset,int nbyte,char * pbuf)
{
   char i=0;
   char n=0;
   char off = offset;
   while(nbyte >0)
   {
      if(nbyte > 16)
        n = 16;
      else
        n = nbyte;
      nbyte = nbyte -n;
      
      
      bit_0 (HDMI_BASE + 0x500,BIT8    		);		//set FIFO read
      put_value(HDMI_BASE + 0x507,pointer	);		//segment pointer
      put_value(HDMI_BASE + 0x505,off  		);		//offset address 
      bit_1 (HDMI_BASE + 0x510,BIT31  		);			//FIFO address clear
      put_wvalue(HDMI_BASE + 0x51c,n 		);		//nbyte to access
      put_value (HDMI_BASE + 0x520,cmd		);		//start and cmd
      bit_1(HDMI_BASE + 0x500,BIT30			);
      
      off   += n; 
      while(get_wvalue(HDMI_BASE + 0x500)&0x40000000);

      i=0;
      while(i<n)
      {
         put_wvalue(0x80000000,get_wvalue(HDMI_BASE + 0x514));
   	     * pbuf ++ = get_value(HDMI_BASE + 0x518);
   	     i++;
      }
   }
      	
}


void DDC_Abort_test()
{

      bit_0 (HDMI_BASE + 0x500,BIT8    		);		//set FIFO read
      put_value(HDMI_BASE + 0x507,0			);		//segment pointer
      put_value(HDMI_BASE + 0x505,0 		);		//offset address 
      bit_1 (HDMI_BASE + 0x510,BIT31  		);		//FIFO address clear
      put_wvalue(HDMI_BASE + 0x51c,100 		);		//nbyte to access
      put_value (HDMI_BASE + 0x520,4		);		//start and cmd
      bit_1(HDMI_BASE + 0x500,BIT30			);
   
      while((get_value(HDMI_BASE + 0x514)&0x40)==0);		//wait fifo full
      put_value(0x80000000,get_value((HDMI_BASE + 0x514)));
      put_value (HDMI_BASE + 0x520,0x00		);		//abort cmd
   
      get_value(HDMI_BASE + 0x518);
      while(get_wvalue(HDMI_BASE + 0x500)&0x40000000);
}

void NDMA_set()
{
   
   
   put_wvalue(0x01c02144,0x01c16518);	//source	
   put_wvalue(0x01c02148,0x80000000);	//dest
   put_wvalue(0x01c0214c,0x00000100);	//128 byte
   put_wvalue(0x01c02140,0x8c110024);	//config 

}

void DDC_Read_NDMA(char cmd,char pointer,char offset,int nbyte)
{
   char i=0;
   char n=0;
   char off = offset;
   
   bit_0 (HDMI_BASE + 0x500,BIT8    		);		//set FIFO read
   put_value(HDMI_BASE + 0x507,pointer	);		//segment pointer
   put_value(HDMI_BASE + 0x505,off  		);		//offset address 
   bit_1 (HDMI_BASE + 0x510,BIT31  		);		//FIFO address clear
   put_wvalue(HDMI_BASE + 0x51c,nbyte	);		//nbyte to access
      
   bit_1(HDMI_BASE + 0x510,BIT8);				//drq enable
      
   NDMA_set();
      
   put_value (HDMI_BASE + 0x520,cmd		);		//cmd
   bit_1(HDMI_BASE + 0x500,BIT30			);		//start

   while(get_wvalue(HDMI_BASE + 0x500)&0x40000000);
   bit_0(HDMI_BASE + 0x510,BIT8);
      	
}

void DDC_Write_NDMA(char cmd,char pointer,char offset,int nbyte,char * pbuf)
{
   char i=0;
   char n=0;
   char off = offset;
   

      bit_1 (HDMI_BASE + 0x500,BIT8    		);		//set FIFO read
      put_value(HDMI_BASE + 0x507,pointer	);		//segment pointer
      put_value(HDMI_BASE + 0x505,off  		);		//offset address 
      bit_1 (HDMI_BASE + 0x510,BIT31  		);			//FIFO address clear
      put_wvalue(HDMI_BASE + 0x51c,nbyte	);		//nbyte to access
      
      bit_1(HDMI_BASE + 0x510,BIT8);				//drq enable
      bit_1(HDMI_BASE + 0x510,BIT0);				//1 level
   	  
   	  put_wvalue(0x01c02144,0x80000000);	//source	
      put_wvalue(0x01c02148,0x01c16518);	//dest
      put_wvalue(0x01c0214c,0x000000ff);	//256 byte
      put_wvalue(0x01c02140,0x8c030031);	//config 
         
      
      put_value (HDMI_BASE + 0x520,cmd		);		// and cmd
      //bit_1(HDMI_BASE + 0x500,BIT30			);	//start

   while(get_wvalue(HDMI_BASE + 0x500)&0x40000000);
   bit_0(HDMI_BASE + 0x510,BIT8);
      	
}




























