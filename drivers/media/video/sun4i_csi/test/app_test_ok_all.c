/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>
#include <time.h>
#include <linux/fb.h>
#include <linux/kernel.h>

#include "./../../../../../include/linux/drv_display_sun4i.h"//modify this
#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define DISPLAY


struct size{
	int width;
	int height;
};


struct buffer {
        void *                  start;
        size_t                  length;
};

static char *           dev_name        = NULL;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;
int disphd;
unsigned int hlay;
int sel = 0;//which screen 0/1
__disp_layer_info_t layer_para;
__disp_video_fb_t video_fb;
__u32 arg[4];

struct timeval time_test;   
struct timezone tz; 

struct size input_size;
struct size disp_size;
int  csi_format;
__disp_pixel_fmt_t  disp_format;
__disp_pixel_mod_t  disp_mode;
int	 read_num=100;
int  test_num=10;
int  req_frame_num;
int	 fps=0;
int	 invalid_ops=0;

struct test_case{
		int 							  input_width;
		int									input_height;
		int 							  disp_width;
		int									disp_height;
		int 								csi_format;
		__disp_pixel_fmt_t 	disp_format;
		__disp_pixel_mod_t	disp_mode;
}; 

struct test_case test_case_set[]={
	{
		.input_width  = 320,
		.input_height = 240,
		.disp_width   = 320,
		.disp_height  = 240,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 328,
		.input_height = 240,
		.disp_width   = 320,
		.disp_height  = 240,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 320,
		.input_height = 248,
		.disp_width   = 320,
		.disp_height  = 240,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 312,
		.input_height = 240,
		.disp_width   = 176,
		.disp_height  = 144,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 320,
		.input_height = 232,
		.disp_width   = 176,
		.disp_height  = 144,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 640,
		.input_height = 480,
		.disp_width   = 640,
		.disp_height  = 480,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 648,
		.input_height = 480,
		.disp_width   = 640,
		.disp_height  = 480,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 640,
		.input_height = 488,
		.disp_width   = 640,
		.disp_height  = 480,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 632,
		.input_height = 480,
		.disp_width   = 352,
		.disp_height  = 288,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 640,
		.input_height = 472,
		.disp_width   = 352,
		.disp_height  = 288,
		.csi_format   = V4L2_PIX_FMT_YUV422P,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 320,
		.input_height = 240,
		.disp_width   = 320,
		.disp_height  = 240,
		.csi_format   = V4L2_PIX_FMT_YUV420,
		.disp_format	= DISP_FORMAT_YUV420,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 320,
		.input_height = 240,
		.disp_width   = 320,
		.disp_height  = 240,
		.csi_format   = V4L2_PIX_FMT_NV16,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_UV_COMBINED,
	},
	{
		.input_width  = 320,
		.input_height = 240,
		.disp_width   = 320,
		.disp_height  = 240,
		.csi_format   = V4L2_PIX_FMT_NV12,
		.disp_format	= DISP_FORMAT_YUV420,
		.disp_mode		= DISP_MOD_NON_MB_UV_COMBINED,
	},
	{
		.input_width  = 640,
		.input_height = 480,
		.disp_width   = 640,
		.disp_height  = 480,
		.csi_format   = V4L2_PIX_FMT_YUV420,
		.disp_format	= DISP_FORMAT_YUV420,
		.disp_mode		= DISP_MOD_NON_MB_PLANAR,
	},
	{
		.input_width  = 640,
		.input_height = 480,
		.disp_width   = 640,
		.disp_height  = 480,
		.csi_format   = V4L2_PIX_FMT_NV16,
		.disp_format	= DISP_FORMAT_YUV422,
		.disp_mode		= DISP_MOD_NON_MB_UV_COMBINED,
	},
	{
		.input_width  = 640,
		.input_height = 480,
		.disp_width   = 640,
		.disp_height  = 480,
		.csi_format   = V4L2_PIX_FMT_NV12,
		.disp_format	= DISP_FORMAT_YUV420,
		.disp_mode		= DISP_MOD_NON_MB_UV_COMBINED,
	},
};
    
static void
errno_exit                      (const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));

        exit (EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
        int r;

        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}

static void
process_image                   (const void *           p)
{
//        fputc ('.', stdout);
//        fflush (stdout);
	
	disp_set_addr(disp_size.width,disp_size.height,p);
	
}

static int
read_frame			(void)
{
        struct v4l2_buffer buf;
	unsigned int i;

	
		CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

    	if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) 
    	{
            printf("VIDIOC_DQBUF Error\n");
        
        	switch (errno) {
            case EAGAIN:
                 printf("DQBUF EAGAIN!\n");
                 return 0;

						case EIO:
						/* Could ignore EIO, see spec. */

						/* fall through */

						default:
								printf("DQBUF EIO!\n");
								errno_exit ("VIDIOC_DQBUF");
			}
		}
		
		
      assert (buf.index < n_buffers);

	    printf ("buf.index dq is %d,\n",buf.index);
			printf ("buf.m.offset = 0x%x\n",buf.m.offset);	
	    process_image (&(buf.m.offset));
	    
	    if(read_num==1)
	    	{
	    		printf("press ENTER key to continue!\n");
	    		getchar();
	    	}
	       
		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			{
				printf("VIDIOC_QBUF Error!\n");
				errno_exit ("VIDIOC_QBUF");
			}

	return 1;
}

static void
mainloop                        (void)
{
	unsigned int count;

        count = read_num;
//
        while (count-- > 0) {
                	
                gettimeofday(&time_test,&tz);
	        			
	        			if(fps)
	        				printf("process image %d sec %d usec\n",time_test.tv_sec,time_test.tv_usec);
                
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO (&fds);
                        FD_SET (fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
				
                        r = select (fd + 1, &fds, NULL, NULL, &tv);
						
						
                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;

                                errno_exit ("select");
                        }
						
                        if (0 == r) {
                                fprintf (stderr, "select timeout\n");
                                exit (EXIT_FAILURE);
                        }
#ifdef DISPLAY      
      if(count==read_num-1)
      	disp_on();                  
#endif			
			if (read_frame ())
                    		break;
				
			/* EAGAIN - continue select loop. */
                }
        }
}

static void
stop_capturing                  (void)
{
        enum v4l2_buf_type type;

	
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
			errno_exit ("VIDIOC_STREAMOFF");

	
}

static void
start_capturing                 (void)
{
        unsigned int i;
        enum v4l2_buf_type type;

	

	
		for (i = 0; i < n_buffers; ++i) {
            		struct v4l2_buffer buf;

        		CLEAR (buf);

        		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_MMAP;
        		buf.index       = i;

        		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    		errno_exit ("VIDIOC_QBUF");
		}
		
		
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");
		
	
}

static void
uninit_device                   (void)
{
        unsigned int i;

	
		for (i = 0; i < n_buffers; ++i)
			{
				
				if (-1 == munmap (buffers[i].start, buffers[i].length))
				errno_exit ("munmap");
			}
	
	free (buffers);
}

static void
init_read			(unsigned int		buffer_size)
{
        buffers = calloc (1, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

	buffers[0].length = buffer_size;
	buffers[0].start = malloc (buffer_size);

	if (!buffers[0].start) {
    		fprintf (stderr, "Out of memory\n");
            	exit (EXIT_FAILURE);
	}
}

static void
init_mmap			(void)
{
	struct v4l2_requestbuffers req;

        CLEAR (req);

        req.count               = req_frame_num;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_MMAP;
		
		if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

//        if (req.count < 2) {
//                fprintf (stderr, "Insufficient buffer memory on %s\n",
//                         dev_name);
//                exit (EXIT_FAILURE);
//        }

        buffers = calloc (req.count, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit ("VIDIOC_QUERYBUF");

                
                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap (NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        {
                        	printf("app mmap failed\n");
                        	errno_exit ("mmap");
                        }
        }
}


static int
init_device                     (void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
				unsigned int min;

        if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf (stderr, "%s is no video capture device\n",
                         dev_name);
                exit (EXIT_FAILURE);
        }


		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf (stderr, "%s does not support streaming i/o\n",
				 dev_name);
			exit (EXIT_FAILURE);
		}

			
			
			if(invalid_ops)
				{
					if(-1==xioctl (fd, 0xff, &cropcap))
						printf("invalid_ops return error\n");
				}
			

        /* Select video input, video standard and tune here. */


				CLEAR (cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {	
                /* Errors ignored. */
        }


        CLEAR (fmt);

        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = input_size.width; 
        fmt.fmt.pix.height      = input_size.height;
        fmt.fmt.pix.pixelformat = csi_format;//V4L2_PIX_FMT_YUV422P;//V4L2_PIX_FMT_YUV420;//V4L2_PIX_FMT_NV16;//V4L2_PIX_FMT_NV12;
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;//V4L2_FIELD_ANY;//V4L2_FIELD_INTERLACED;

        if (-1 == xioctl (fd, VIDIOC_TRY_FMT, &fmt))
        {
         		printf("[app]try format error!\n");       
         		
        }
        
        if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
         {
         		printf("[app]set format error!,return\n");       
         		return -1;
         }
                //errno_exit ("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;


		init_mmap ();

		return 0;
	
}

static void
close_device                    (void)
{
        if (-1 == close (fd))
	        errno_exit ("close");

        fd = -1;
}

static void
open_device                     (void)
{
        struct stat st; 

        if (-1 == stat (dev_name, &st)) {
                fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }

        if (!S_ISCHR (st.st_mode)) {
                fprintf (stderr, "%s is no device\n", dev_name);
                exit (EXIT_FAILURE);
        }

        fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf (stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }
}


int disp_int(int w,int h)
{
	/*display start*/ 
    //unsigned int h,w;
    __u32 id = 0;
	
    //h= 480;
    //w= 640;

	if((disphd = open("/dev/disp",O_RDWR)) == -1)
	{
		printf("open file /dev/disp fail. \n");
		return 0;
	}

   
    //layer0
    arg[0] = 0;
    arg[1] = DISP_LAYER_WORK_MODE_SCALER;
    hlay = ioctl(disphd, DISP_CMD_LAYER_REQUEST, (void*)arg);
    if(hlay == NULL)
    {
        printf("request layer0 fail\n");
        return 0;
    }

    layer_para.mode = DISP_LAYER_WORK_MODE_SCALER; 
    layer_para.pipe = 0; 
    //layer_para.fb.addr[0]       = 0;//your Y address,modify this 
    //layer_para.fb.addr[1]       = 0; //your C address,modify this 
    //layer_para.fb.addr[2]       = 0; 
    layer_para.fb.size.width    = w;
    layer_para.fb.size.height   = h;
    layer_para.fb.mode          = disp_mode;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_UV_COMBINED;
    layer_para.fb.format        = disp_format;//DISP_FORMAT_YUV422;//DISP_FORMAT_YUV420;
    layer_para.fb.br_swap       = 0;
    layer_para.fb.seq           = DISP_SEQ_UVUV;
    layer_para.ck_enable        = 0;
    layer_para.alpha_en         = 1; 
    layer_para.alpha_val        = 0xff;
    layer_para.src_win.x        = 0;
    layer_para.src_win.y        = 0;
    layer_para.src_win.width    = w;
    layer_para.src_win.height   = h;
    layer_para.scn_win.x        = 0;
    layer_para.scn_win.y        = 0;
    layer_para.scn_win.width    = 800;
    layer_para.scn_win.height   = 480;
		arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);

    

}

int disp_on()
{
		arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_ON, (void*)arg);
}

int disp_start()
{
		arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_OPEN,(void*)arg);
}

int disp_set_addr(int w,int h,int *addr)
{
		layer_para.fb.addr[0]       = *addr;//your Y address,modify this 
		
		switch(csi_format){
		case V4L2_PIX_FMT_YUV422P:
    	layer_para.fb.addr[1]       = *addr+w*h; //your C address,modify this 
    	layer_para.fb.addr[2]       = *addr+w*h*3/2; 
    	break;
    case V4L2_PIX_FMT_YUV420:
    	layer_para.fb.addr[1]       = *addr+w*h; //your C address,modify this 
    	layer_para.fb.addr[2]       = *addr+w*h*5/4;
    	break;
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV12:	
    case V4L2_PIX_FMT_HM12:	
    	layer_para.fb.addr[1]       = *addr+w*h; //your C address,modify this 
    	layer_para.fb.addr[2]       = layer_para.fb.addr[1];
    	break;
    
    default:
    	printf("csi_format is not found!\n");
    	break;
    
  	}
  	
    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
}

int disp_quit()
{
	__u32 arg[4];

		//arg[0] = sel;
    //arg[1] = hlay;
    //ioctl(disphd, DISP_CMD_VIDEO_STOP, (void*)arg);
		arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_OFF, (void*)arg);
		
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_RELEASE,  (void*)arg);
    close (disphd);
}

int
main_test (void)
{
        dev_name = "/dev/video0";
    
 
        open_device ();
		
        if(init_device())
        	{
        		printf("[app]init_device error!return\n");
        		return -1;
        	}
        
				start_capturing ();
				
#ifdef DISPLAY				
				disp_int(disp_size.width,disp_size.height);
				disp_start();
#endif				
       	mainloop ();

        stop_capturing ();

        uninit_device ();

        close_device ();
        
        disp_quit();
        
      return 0;
}

int 
main(void)
{
		int i;
		struct test_case *test_ptr;
		
		test_num=1;
		read_num=200;
		
		req_frame_num = 5;
		input_size.width = 640;
		input_size.height = 480;
		disp_size.width = 640;
		disp_size.height = 480;
		csi_format=V4L2_PIX_FMT_YUV422P;
		disp_format=DISP_FORMAT_YUV422;
		disp_mode=DISP_MOD_NON_MB_PLANAR;
		
		invalid_ops=1;
printf("IOCTL invalid test start,press to continue\n");		
		getchar();
		main_test();
		
		
printf("Read one frame test start,capture 1 frame,press to continue\n");
		getchar();
		
		invalid_ops=0;
		read_num=1;
		main_test();
		
printf("Read stream test start,capture 1000 frames,press to continue\n");
		getchar();
		
		read_num = 1000;
		main_test();
		
printf("Req buffer test start,press to continue\n");
		getchar();
		
		read_num = 100;
		for(i=1;i<7;i++)
		{
			printf("Req buffer count = %d, capture 100 frames\n",i);
			req_frame_num = i;
			main_test();
			printf("press to continue\n");
			getchar();
		}

printf("Try and set invalid format test start,press to continue\n");
		getchar();
		
		printf("Try V4L2_PIX_FMT_YUV410\n");
		csi_format=V4L2_PIX_FMT_YUV410;
		main_test();
		
		printf("Try V4L2_PIX_FMT_YVU420\n");
		csi_format=V4L2_PIX_FMT_YVU420;
		main_test();
		
printf("resolution and format test start,press to continue\n");
		getchar();
		
		read_num=200;
		
		for(i=0;i<16;i++)
		{
			test_ptr = &test_case_set[i];
			input_size.width = test_ptr->input_width;
			input_size.height = test_ptr->input_height;
			disp_size.width = test_ptr->disp_width;
			disp_size.height = test_ptr->disp_height;
			csi_format = test_ptr->csi_format;
			disp_format = test_ptr->disp_format;
			disp_mode=test_ptr->disp_mode;
		
			printf("***************************************************************************************\ninput size:%dx%d\n",
			input_size.width,input_size.height);
			
			switch(csi_format){
			case V4L2_PIX_FMT_YUV422P:
    		printf("format: V4L2_PIX_FMT_YUV422P\n");
    		break;
    	case V4L2_PIX_FMT_YUV420:
    		printf("format: V4L2_PIX_FMT_YUV420\n");
    		break;
    	case V4L2_PIX_FMT_NV16:
    		printf("format: V4L2_PIX_FMT_NV16\n");
    		break;
    	case V4L2_PIX_FMT_NV12:	
    		printf("format: V4L2_PIX_FMT_NV12\n");
    		break;
    	case V4L2_PIX_FMT_HM12:
    		printf("format: V4L2_PIX_FMT_HM12\n");
    		break;
    	default:
    		printf("format: error\n");
    		break;
    	}
    	
			printf("***************************************************************************************\n");			
			main_test();
			printf("press to continue\n");	
			getchar();
		}
		
printf("fps test start,press to continue\n");
		getchar();		
		fps=1;
		read_num=30;
		req_frame_num = 4;
		input_size.width = 640;
		input_size.height = 480;
		disp_size.width = 640;
		disp_size.height = 480;
		csi_format=V4L2_PIX_FMT_YUV422P;
		disp_format=DISP_FORMAT_YUV422;
		disp_mode=DISP_MOD_NON_MB_PLANAR;
		main_test();
		
		
printf("test done,press to end\n");	
		getchar();
					
			
	exit (EXIT_SUCCESS);
	return 0;
}