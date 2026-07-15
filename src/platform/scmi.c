/*
 * Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include "platform/versal2_platform.h"
#include "common.h"

/* =============================================================================
 * Package Version Information (provided by Makefile)
 * Note: SCMI_VERSION is the protocol version (0x20000U) defined in common.h
 *       SCMI_PKG_VERSION is the package/git version string
 * =============================================================================
 */

#ifndef SCMI_GIT_COMMIT
#define SCMI_GIT_COMMIT "unknown"
#endif

#ifndef SCMI_GIT_COMMIT_LONG
#define SCMI_GIT_COMMIT_LONG "unknown"
#endif

#ifndef SCMI_GIT_BRANCH
#define SCMI_GIT_BRANCH "unknown"
#endif

#ifndef SCMI_GIT_DATE
#define SCMI_GIT_DATE "unknown"
#endif

#ifndef SCMI_PKG_VERSION
#define SCMI_PKG_VERSION "unknown"  // ← RENAMED from SCMI_VERSION
#endif

#ifndef SCMI_BUILD_DATE
#define SCMI_BUILD_DATE "unknown"
#endif

#ifndef SCMI_BUILD_TIME
#define SCMI_BUILD_TIME "unknown"
#endif

#ifndef SCMI_BUILD_TIMESTAMP
#define SCMI_BUILD_TIMESTAMP "unknown"
#endif

/* Currently only one channel is supported. Expectation is that channel 0 is used by NS SW */
static struct scmi_msg_channel scmi_channel[] = {
	[0] = {
		.shm_addr = SMT_BUFFER_BASE,
		.shm_size = SMT_BUF_SLOT_SIZE,
	},
};

struct scmi_msg_channel *plat_scmi_get_channel(unsigned int agent_id)
{
	assert(agent_id < ARRAY_SIZE(scmi_channel));

	VERBOSE("%d: SCMI asking for channel\n", agent_id);

	/* Just in case that code is reused */
	return &scmi_channel[agent_id];
}

/* Base protocol implementations */
const char *plat_scmi_vendor_name(void)
{
	return SCMI_VENDOR;
}

const char *plat_scmi_sub_vendor_name(void)
{
	return SCMI_PRODUCT;
}

/* Currently supporting Clocks and Reset Domains */
static const uint8_t plat_protocol_list[] = {
	SCMI_PROTOCOL_ID_BASE,
	SCMI_PROTOCOL_ID_CLOCK,
	SCMI_PROTOCOL_ID_RESET_DOMAIN,
	SCMI_PROTOCOL_ID_POWER_DOMAIN,
	/* SCMI_PROTOCOL_ID_SENSOR, */
	0U /* Null termination */
};

size_t plat_scmi_protocol_count(void)
{
	const size_t count = ARRAY_SIZE(plat_protocol_list) - 1U;

	VERBOSE("SCMI: Protocol count: %d\n", (int32_t)count);

	return count;
}

const uint8_t *plat_scmi_protocol_list(unsigned int agent_id __unused)
{
	return plat_protocol_list;
}

void init_scmi_server(void)
{
	size_t i;

	for (i = 0U; i < ARRAY_SIZE(scmi_channel); i++) {
		scmi_smt_init_agent_channel(&scmi_channel[i]);
	}

	NOTICE("SCMI: Server initialized %s %s %s\n",
	       SCMI_PKG_VERSION, SCMI_BUILD_DATE, SCMI_BUILD_TIME);
}
