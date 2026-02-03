/*
 * Copyright (c) 2026, Advanced Micro Devices Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <lib/smccc.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform/versal2_platform.h>
#include <stdio.h>

extern void scmi_smt_fastcall_smc_entry(unsigned int agent_id);

/*
 * Stub implementations of custom package interface functions
 * SCMI server doesn't need these, but TF-A platform code requires them
 *
 * When used with a78stl-bridge, these will be overridden by the bridge's
 * implementations using weak symbols.
 */

/*
 * Early setup - called during BL31 early platform setup
 */
__attribute__((weak)) void custom_early_setup(void)
{
	/* SCMI server: No early setup required */
}

/*
 * Runtime setup - called during BL31 runtime setup
 */
__attribute__((weak)) void custom_runtime_setup(void)
{
	/* SCMI server: No runtime setup required */
}

/*
 * Add custom memory mappings
 * Returns: number of additional mmap regions added (0 for SCMI)
 */
__attribute__((weak)) int custom_mmap_add(void)
{
	/* SCMI server: No custom mmap */
	return 0;
}

/*
 * Handle custom SMC calls
 * Returns: SMC_UNK if not handled, or result value if handled
 */
__attribute__((weak)) uintptr_t custom_smc_handler(uint32_t smc_fid,
						    u_register_t x1,
						    u_register_t x2,
						    u_register_t x3,
						    u_register_t x4,
						    void *cookie,
						    void *handle,
						    u_register_t flags)
{
	/* SCMI server: No custom SMC handling */
	return SMC_OK;
}
