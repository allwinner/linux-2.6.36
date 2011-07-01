#ifndef SUN4I_ANX7150_H_
#define SUN4I_ANX7150_H_

//#include "sound/anx7150.h"

struct sun4i_anx7150_platform_data {
	int hdmiaudio_bclk;
	int hdmiaudio_ws;
	int hdmiaudio_data;
	void (*power)(int);
	int model;
}
#endif