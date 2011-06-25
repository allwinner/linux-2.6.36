/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __ARCH_CONFIG_H__
#define __ARCH_CONFIG_H__

/* Configuration for the EB platform with ZBT memory enabled */

static _mali_osk_resource_t arch_configuration [] =
{
	{
		.type = MALI400GP,
		.description = "Mali-400 GP",
		.base = 0x01C40000,
		.irq = 69,
		.mmu_id = 1
	},
	{
		.type = MALI400PP,
		.base = 0x01C48000,
		.irq = 71,
		.description = "Mali-400 PP",
		.mmu_id = 2
	},
#if USING_MMU
	{
		.type = MMU,
		.base = 0x01C43000,
		.irq = 70,
		.description = "Mali-400 MMU for GP",
		.mmu_id = 1
	},
	{
		.type = MMU,
		.base = 0x01C44000,
		.irq = 72,
		.description = "Mali-400 MMU for PP",
		.mmu_id = 2
	},
#endif
//#if USING_ZBT
//	{
//		.type = MEMORY,
//		.description = "Mali ZBT",
//		.alloc_order = 0, /* Medium preference for this memory */
//		.base = 0x54000000,
//		.size = 64 * 1024 * 1024, /*64M*/
//		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_MMU_READABLE | _MALI_MMU_WRITEABLE
//	},
//#endif
	{
		.type = MEM_VALIDATION,
		.description = "Framebuffer",
		.base = 0x58000000,
		.size = 128 * 1024 * 1024,  /*16M*/
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_MMU_READABLE | _MALI_MMU_WRITEABLE
	},
	{
		.type = OS_MEMORY,
		.description = "OS Memory",
		.alloc_order = 10, /* Lowest preference for this memory */
		.size = 64 * 1024 * 1024, /* 96 MB */
		//.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE |_MALI_GP_READABLE | _MALI_GP_WRITEABLE
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_MMU_READABLE | _MALI_MMU_WRITEABLE
	},
	{
		.type = MALI400L2,
		.base = 0x01C41000,
		.description = "Mali-400 L2 cache"
	},
};

#endif /* __ARCH_CONFIG_H__ */
