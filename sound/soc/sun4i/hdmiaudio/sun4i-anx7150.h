#ifndef SUN4I_ANX7150_H_
#define SUN4I_ANX7150_H_

//#include "sound/anx7150.h"

struct sun4i_anx7150_platform_data {
	int iis_bclk;
	int iis_ws;
	int iis_data;
	void (*power)(int);
	int model;
}
#endif