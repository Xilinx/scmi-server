/*
 * Copyright (c) 2026, Advanced Micro Devices Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <common.h>
#include <lib/smccc.h>
#include <smccc_helpers.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <drivers/scmi-msg.h>
#include <platform/versal2_platform.h>
#include <stdio.h>

/**
 * custom_early_setup() - Early initialization called during BL31 early platform setup
 *
 * This function is called during BL31 early platform setup phase. For the SCMI
 * server implementation, it initializes the SCMI server.
 *
 * Return: void
 */
__attribute__((weak)) void custom_early_setup(void)
{
	/* SCMI server: No early setup required */
	init_scmi_server();
}

/**
 * custom_runtime_setup() - Runtime initialization called during BL31 runtime setup
 *
 * This function is called during BL31 runtime setup phase. For the SCMI server
 * implementation, no runtime setup is required.
 *
 * Return: void
 */
__attribute__((weak)) void custom_runtime_setup(void)
{
	/* SCMI server: No runtime setup required */
}

/**
 * custom_mmap_add() - Add custom memory mappings for the platform
 *
 * This function is responsible for adding platform-specific memory mappings.
 * For the SCMI server, it maps the SMT (Shared Memory Transport) buffer region
 * used for communication between normal world and secure world.
 *
 * Return: void
 */
__attribute__((weak)) void custom_mmap_add(void)
{
	/* SCMI server: Add SMT buffer memory mapping */
	static const mmap_region_t scmi_mmap[] = {
		MAP_REGION_FLAT(SMT_BUFFER_BASE, 0x1000,
		MT_DEVICE | MT_RW | MT_NON_CACHEABLE | MT_EXECUTE_NEVER | MT_NS),
		{ 0 }
	};

	mmap_add(scmi_mmap);
}

/**
 * custom_smc_handler() - Handle custom SMC (Secure Monitor Call) requests
 * @smc_fid: SMC function identifier
 * @x1: SMC argument register x1
 * @x2: SMC argument register x2
 * @x3: SMC argument register x3
 * @x4: SMC argument register x4
 * @cookie: Unused parameter (reserved for future use)
 * @handle: Pointer to the context handle
 * @flags: SMC call flags
 *
 * This function handles custom SMC calls from non-secure world. For the SCMI
 * server implementation, it processes SCMI fast-call requests by invoking
 * the SCMI SMT (Shared Memory Transport) entry point. Only the FID assigned
 * to the nonsecure SCMI agent (SCMI_NS_AGENT_SMC_FID) is accepted; any other
 * FID is rejected and reported as an incident.
 *
 * Context: Called from SMC exception handler in secure monitor context.
 * Return: SMC_OK on success, or SMC_UNK if the SMC is not handled
 */
__attribute__((weak)) uint64_t custom_smc_handler(uint32_t smc_fid,
						   uint64_t x1,
						   uint64_t x2,
						   uint64_t x3,
						   uint64_t x4,
						   void *cookie,
						   void *handle,
						   uint64_t flags)
{
	/* Mark unused parameters to satisfy MISRA C-2012 Rule 2.7 */
	(void)x1;
	(void)x2;
	(void)x3;
	(void)x4;
	(void)cookie;
	(void)flags;

	if (smc_fid != SCMI_NS_AGENT_SMC_FID) {
		plat_scmi_report_incident(0, SCMI_INCIDENT_UNKNOWN_FID);
		SMC_RET1(handle, SMC_UNK);
	}

	scmi_smt_fastcall_smc_entry(0);
	SMC_RET1(handle, SMC_OK);
}
