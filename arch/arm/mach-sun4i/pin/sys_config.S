	.globl	sys_cofig_data
sys_cofig_data:
	.incbin	"arch/arm/mach-sun4i/pin/sys_config1.bin"
	.globl	sys_cofig_data_end
sys_cofig_data_end:
