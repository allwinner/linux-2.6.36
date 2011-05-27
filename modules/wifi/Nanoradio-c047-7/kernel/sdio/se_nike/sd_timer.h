#include <linux/delay.h>

int sd_Timer_TimeOut=0;

void sd_start_timer(long time)
{
	sd_Timer_TimeOut = time;
}

int sd_check_timer(void)
{
	// check if we have time out to the cmd line
	if (!sd_Timer_TimeOut) {
		//mporo na grapo minima lathous
		return SDR_ERR;
	}
	sd_Timer_TimeOut--;
	mdelay(1);

    return SDR_OK;
}

void sd_wait(long time)
{
//	s5c73xx_husleep(time*10);
    mdelay(time);
}


