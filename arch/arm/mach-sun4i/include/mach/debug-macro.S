/* arch/arm/mach-sun4i/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 *  Copyright (C) 1994-1999 Russell King
 *  Copyright (C) 2005 Simtec Electronics
 *
 *  Moved from linux/arch/arm/kernel/debug.S by Ben Dooks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/



/* Todo: implement the following function to the actual sun4i uart*/

	.macro	addruart, rd, rx
		tst	\rd,	#0
		tst	\rx,	#0
	.endm
	
	.macro	senduart, rd, rx
		tst	\rd,	#0
		tst	\rx,	#0
		nop
	.endm
	
	.macro	busyuart, rd, rx
		tst	\rd,	#0
		tst	\rx,	#0
		nop
	.endm
	
	.macro	waituart, rd, rx
		tst	\rd,	#0
		tst	\rx,	#0
		nop
	.endm
