/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_VARIANT_H
#define PLAT_VARIANT_H

#include <stdint.h>
#include <lib/utils_def.h>

/*
 * Chip variant, one bit per variant (not a sequential ordinal) so a map
 * table entry can be present on several variants at once. Bits are grouped
 * 8 per die family; add a new bit here (and its idcode in scmi.c) for a new
 * variant -- no other code needs to change.
 */
typedef uint32_t plat_variant_t;

/* Family 0 (bits 0-7): Versal Gen2 die family */
#define PLAT_VARIANT_VERSAL_GEN2	BIT_32(0)
#define PLAT_VARIANT_2VM3654		BIT_32(1)
#define PLAT_VARIANT_2VE3288		BIT_32(2)

/* Family 1 (bits 8-15): Versal Premium Gen2 die family */
#define PLAT_VARIANT_VPG2	BIT_32(8)	/* reserved: idcode not yet assigned */

/* Families 2-3 (bits 16-30) reserved for future platform families. */

/*
 * Shorthand for map tables. Do NOT fold a future variant in here if it only
 * shares some ids with this family -- give it its own explicit mask instead.
 */
#define PLAT_VARIANT_GEN2_FAMILY \
	(PLAT_VARIANT_VERSAL_GEN2 | PLAT_VARIANT_2VM3654)

/* "Present on zero variants" sentinel. Bit 31 is reserved, never assigned to a real variant. */
#define PLAT_VARIANT_NONE	BIT_32(31)

/* Detect the current chip's platform variant from idcode. */
plat_variant_t plat_get_variant(void);

/*
 * Chip variant detected at boot; set once (see custom_early_setup()) before
 * any scmi_{clock,power,reset}_map_get() call. All three maps check the
 * same bit-per-variant encoding, so a single shared value is sufficient --
 * no need for a separate copy per map.
 */
extern plat_variant_t plat_variant;

#endif /* PLAT_VARIANT_H */
