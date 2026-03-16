/*
 * Copyright (c) 2022, Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * System level PM-API functions and communication with PMC via
 * IPI interrupts
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <lib/utils.h>
#include "platform/pm_defs.h"
#include "pm_client.h"

extern enum pm_api_ret_status pm_ipi_send_sync(const struct pm_proc *proc,
					       uint32_t payload[PAYLOAD_ARG_CNT],
					       uint32_t *value, size_t count);

/**
 * pm_api_feature_check() - Check if a PM API is supported by firmware
 * @api_id: API ID to check
 *
 * Verifies if a specific PM API is supported by the firmware.
 * Extracts the module ID, determines the appropriate feature check method,
 * and queries the firmware via IPI.
 *
 * Return: Returns version if supported, or PM_RET_ERROR_NO_FEATURE if not
 */
static uint32_t pm_api_feature_check(uint32_t api_id)
{
	uint32_t result[RET_PAYLOAD_ARG_CNT];
	uint32_t payload[PAYLOAD_ARG_CNT];
	uint32_t return_value;
	uint32_t ret;

	zeromem(&payload[0], sizeof(payload));
	zeromem(&result[0], sizeof(result));

	/*
	 * Frame the payload for feature check:
	 * payload[0] = XPM_MODULE_ID | PM_FEATURE_CHECK
	 * payload[1] = api_id (without module bits)
	 */
	payload[0] = FIELD_PREP(MODULE_ID_MASK, XPM_MODULE_ID) | PM_FEATURE_CHECK;
	payload[1] = api_id & API_ID_MASK;

	ret = (uint32_t)pm_ipi_send_sync(primary_proc, payload, result, 2);

	/* If firmware call failed, API is not supported */
	if (ret != (uint32_t)PM_RET_SUCCESS) {
		return_value = (uint32_t)PM_RET_ERROR_NO_FEATURE;
	} else {
		/* Return the feature version from result[0] */
		return_value = result[0];
	}

	return return_value;
}
