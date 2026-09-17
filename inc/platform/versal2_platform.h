/*
 * Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VERSAL2_PLATFORM_H
#define VERSAL2_PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <lib/smccc.h>

#include "platform/plat_variant.h"

/* SMC fast-call FID assigned to the nonsecure (Linux) SCMI agent */
#define SCMI_NS_AGENT_SMC_FID	0xC2002001U

void init_scmi_server(void);

/* Custom platform initialization hooks */
void custom_early_setup(void);
void custom_runtime_setup(void);
void custom_mmap_add(void);
uint64_t custom_smc_handler(uint32_t smc_fid,
			    uint64_t x1,
			    uint64_t x2,
			    uint64_t x3,
			    uint64_t x4,
			    void *cookie,
			    void *handle,
			    uint64_t flags);

size_t plat_scmi_pd_count(unsigned int agent_id);
const char *plat_scmi_pd_get_name(unsigned int agent_id, unsigned int pd_id);
unsigned int plat_scmi_pd_statistics(unsigned int agent_id, unsigned long *pd_id);
unsigned int plat_scmi_pd_get_attributes(unsigned int agent_id, unsigned int pd_id);
unsigned int plat_scmi_pd_get_state(unsigned int agent_id, unsigned int pd_id);
int32_t plat_scmi_pd_set_state(unsigned int agent_id, unsigned int flags, unsigned int pd_id,
				unsigned int state);

#define SCMI_VENDOR	"AMD"
#define SCMI_PRODUCT	"Versal Gen 2"


#endif /* VERSAL2_PLATFORM_H */
