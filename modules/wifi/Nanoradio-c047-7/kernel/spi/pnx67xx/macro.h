

void __iomem	*spi_base_regs;

// add base
#define  base_offset(adrs) 			      	((spi_base_regs)+(adrs))
#define  SPI_BASE_ADDR 						SPI1_BASE_ADDR
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SPI reg

// 32bit access
#define write_32(adrs,data)					__raw_writel((long)(data), base_offset(adrs))
#define read_32(adrs)						(long)(__raw_readl(base_offset(adrs)))
#define or_32(adrs,data)                  	__raw_writel(read_32(adrs)|(long)(data), base_offset(adrs))
#define and_32(adrs,data)                 	__raw_writel(read_32(adrs)&(long)(data), base_offset(adrs))

// 16bit access
#define write_16(adrs,data)					__raw_writew((long)(data), base_offset(adrs))
#define read_16(adrs)						(long)(__raw_readw(base_offset(adrs)))
#define or_16(adrs,data)                  	__raw_writew(read_16(adrs)|(long)(data), base_offset(adrs))
#define and_16(adrs,data)                 	__raw_writew(read_16(adrs)&(long)(data), base_offset(adrs))

// 8bit access
#define write_8(adrs,data)					__raw_writeb((long)(data), base_offset(adrs))
#define read_8(adrs)						(long)(__raw_readb(base_offset(adrs)))
#define or_8(adrs,data)                  	__raw_writeb(read_8(adrs)|(long)(data), base_offset(adrs))
#define and_8(adrs,data)                 	__raw_writeb(read_8(adrs)&(long)(data), base_offset(adrs))


