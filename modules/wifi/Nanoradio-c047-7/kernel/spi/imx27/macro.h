// add base
#define  base_offset(adrs) 			      	((CSPI_VIRT_ADDR)+(adrs))
#define  base_offset_gpio(adrs) 			((IMX27_GPIO_BASE)+(adrs))

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SPI reg

// 32bit access
#define write_32(adrs,data)					*(volatile long *)(base_offset(adrs))  = (long)(data)
#define read_32(adrs)						(*(volatile long *)(base_offset(adrs)))
#define or_32(adrs,data)                  	*(volatile long *)(base_offset(adrs)) |= (long)(data)
#define and_32(adrs,data)                 	*(volatile long *)(base_offset(adrs)) &= (long)(data)
// 16bit access
#define write_16(adrs,data)					*(volatile short *)(base_offset(adrs))  = (short)(data)
#define read_16(adrs)						(*(volatile short *)(base_offset(adrs)))
#define or_16(adrs,data)                  	*(volatile short *)(base_offset(adrs)) |= (short)(data)
#define and_16(adrs,data)                 	*(volatile short *)(base_offset(adrs)) &= (short)(data)
// 8bit access
#define write_8(adrs,data)					*(volatile char *)(base_offset(adrs))  = (char)(data)
#define read_8(adrs)						(*(volatile char *)(base_offset(adrs)))
#define or_8(adrs,data)                  	*(volatile char *)(base_offset(adrs)) |= (char)(data)
#define and_8(adrs,data)                 	*(volatile char *)(base_offset(adrs)) &= (char)(data)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPIO reg

// 32bit access
#define write_gpio_32(adrs,data)				*(volatile long *)(base_offset_gpio(adrs))  = (long)(data)
#define read_gpio_32(adrs)						(*(volatile long *)(base_offset_gpio(adrs)))
#define or_gpio_32(adrs,data)                  	*(volatile long *)(base_offset_gpio(adrs)) |= (long)(data)
#define and_gpio_32(adrs,data)                 	*(volatile long *)(base_offset_gpio(adrs)) &= (long)(data)
// 16bit access
#define write_gpio_16(adrs,data)				*(volatile short *)(base_offset_gpio(adrs))  = (short)(data)
#define read_gpio_16(adrs)						(*(volatile short *)(base_offset_gpio(adrs)))
#define or_gpio_16(adrs,data)                  	*(volatile short *)(base_offset_gpio(adrs)) |= (short)(data)
#define and_gpio_16(adrs,data)                 	*(volatile short *)(base_offset_gpio(adrs)) &= (short)(data)
// 8bit access
#define write_gpio_8(adrs,data)					*(volatile char *)(base_offset_gpio(adrs))  = (char)(data)
#define read_gpio_8(adrs)						(*(volatile char *)(base_offset_gpio(adrs)))
#define or_gpio_8(adrs,data)                  	*(volatile char *)(base_offset_gpio(adrs)) |= (char)(data)
#define and_gpio_8(adrs,data)                 	*(volatile char *)(base_offset_gpio(adrs)) &= (char)(data)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
