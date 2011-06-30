#ifndef SUN4I_codecchip_H_
#define SUN4I_codecchip_H_

//#include "sound/codecchip.h"

struct sun4i_codecchip_platform_data {
	int iis_bclk;
	int iis_ws;
	int iis_data;
	void (*power)(int);
	int model;
}
#endif