/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_PLAT_H
#define SCMI_PLAT_H

#include <stdint.h>

#include <drivers/scmi.h>

#include "platform/pm_defs.h"
#include "platform/pm_api.h"

/* SCMI node-id encoding fields (shared by clock/reset/power-domain backends) */
#define XPM_NODESUBCLASS_SCMI		0xAU
#define XPM_NODETYPE_SCMI		0x0U

#define XPM_NODEID_CLASS_SHIFT		26
#define XPM_NODEID_SUBCLASS_SHIFT	20
#define XPM_NODEID_TYPE_SHIFT		14
#define XPM_NODEID_INDEX_MASK		0x3FFF

/*
 * Encode an SCMI resource index into an XPM node id for the given node class.
 */
static inline uint32_t scmi_encode_node_id(uint32_t node_class, uint32_t scmi_id)
{
	return ((node_class & 0x3FU) << XPM_NODEID_CLASS_SHIFT) |
	       (((uint32_t)XPM_NODESUBCLASS_SCMI & 0x3FU) << XPM_NODEID_SUBCLASS_SHIFT) |
	       (((uint32_t)XPM_NODETYPE_SCMI & 0x3FU) << XPM_NODEID_TYPE_SHIFT) |
	       (scmi_id & (uint32_t)XPM_NODEID_INDEX_MASK);
}

/*
 * Generic PM API error to SCMI error mapper.
 */
static inline int32_t pm_error_to_scmi(enum pm_api_ret_status pm_ret)
{
	int32_t scmi_ret;

	if (pm_ret == PM_RET_SUCCESS) {
		scmi_ret = SCMI_SUCCESS;
	} else if (pm_ret == PM_RET_ERROR_NO_FEATURE) {
		scmi_ret = SCMI_NOT_SUPPORTED;
	} else if (pm_ret == PM_RET_ERROR_INVALID_NODE) {
		scmi_ret = SCMI_NOT_FOUND;
	} else if (pm_ret == PM_RET_ERROR_ACCESS) {
		scmi_ret = SCMI_DENIED;
	} else if (pm_ret == PM_RET_ERROR_ARGS) {
		scmi_ret = SCMI_INVALID_PARAMETERS;
	} else {
		scmi_ret = SCMI_HARDWARE_ERROR;
	}

	return scmi_ret;
}

#endif /* SCMI_PLAT_H */
