// add base
#define  base_offset(adrs) 			      	(SDHC_SDMMC_BASE+adrs)
// 32bit access
#define write_32(adrs,data)					*(volatile long *)(base_offset(adrs))  = (long)(data)
#define read_32(adrs)					(*(volatile long *)(base_offset(adrs)))
#define or_32(adrs,data)                  	*(volatile long *)(base_offset(adrs)) |= (long)(data)
#define and_32(adrs,data)                 	*(volatile long *)(base_offset(adrs)) &= (long)(data)
// 16bit access */
#define write_16(adrs,data)					*(volatile short *)(base_offset(adrs))  = (short)(data)
#define read_16(adrs)					(*(volatile short *)(base_offset(adrs)))
#define or_16(adrs,data)                  	*(volatile short *)(base_offset(adrs)) |= (short)(data)
#define and_16(adrs,data)                 	*(volatile short *)(base_offset(adrs)) &= (short)(data)
// 8bit access */
#define write_8(adrs,data)					*(volatile char *)(base_offset(adrs))  = (char)(data)
#define read_8(adrs)					(*(volatile char *)(base_offset(adrs)))
#define or_8(adrs,data)                  	*(volatile char *)(base_offset(adrs)) |= (char)(data)
#define and_8(adrs,data)                 	*(volatile char *)(base_offset(adrs)) &= (char)(data)
