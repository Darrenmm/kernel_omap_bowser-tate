/*
 * arch/arm/mach-omap2/board-blaze.h
 *
 * Copyright (C) 2011 Texas Instruments
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _OMAP4_ION_H
#define _OMAP4_ION_H

/*Change 1D ION from 90MB to 10MB*/
#define OMAP4_ION_HEAP_SECURE_INPUT_SIZE	(SZ_1M * 10)
/*Change 2D TILER from 96MB to 70MB*/
#define OMAP4_ION_HEAP_TILER_SIZE		(SZ_1M * 70)
#define OMAP4_ION_HEAP_NONSECURE_TILER_SIZE	SZ_32M

#define PHYS_ADDR_SMC_SIZE	(SZ_1M * 3)
#define PHYS_ADDR_SMC_MEM	(0x80000000 + SZ_1G - PHYS_ADDR_SMC_SIZE)
#define PHYS_ADDR_DUCATI_SIZE	(SZ_1M * 105)
#define PHYS_ADDR_DUCATI_MEM	(PHYS_ADDR_SMC_MEM - PHYS_ADDR_DUCATI_SIZE - \
				OMAP4_ION_HEAP_SECURE_INPUT_SIZE)

#ifdef CONFIG_ION_OMAP
void omap_ion_init(void);
void omap4_register_ion(void);
#else
static inline void omap_ion_init(void) { return; }
static inline void omap4_register_ion(void) { return; }
#endif

#endif
