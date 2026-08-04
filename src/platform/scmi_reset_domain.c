/*
 * Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>

#include "platform/pm_defs.h"
#include "platform/pm_api.h"
#include "platform/scmi_plat.h"
#include "platform/scmi_reset_map.h"
#include "common.h"

/*
 * Platform SCMI reset domains
 */

const char *plat_scmi_rstd_get_name(unsigned int agent_id, unsigned int scmi_id)
{
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	static char name_buffer[16];
	enum pm_api_ret_status ret;
	const char *result;
	uint32_t encoded_id;
	uint32_t map_flags;
	(void)agent_id;

	if (scmi_reset_map_get(scmi_id, &encoded_id, &map_flags) != 0) {
		return NULL;
	}

	/* Not instantiated by the design -- see SCMI_RST_FLAG_NOT_PRESENT. */
	if ((map_flags & SCMI_RST_FLAG_NOT_PRESENT) != 0U) {
		(void)snprintf(name_buffer, sizeof(name_buffer), "rsvd%u", scmi_id);
		return name_buffer;
	}

	/* Query firmware for reset name using XPM_QID_GET_NAME */
	ret = pm_api_query_data(XPM_QID_GET_NAME, encoded_id, 0, 0, data, RET_PAYLOAD_ARG_CNT - 1U);

	if (ret == PM_RET_SUCCESS) {
		/* Name is returned in data[0] as a 32-bit value */
		(void)strlcpy(name_buffer, (char *) &data[0], sizeof(name_buffer));
		name_buffer[15] = '\0';  /* Ensure null termination */
		result = name_buffer;
	} else {
		result = NULL;
		NOTICE("%s: failed with err code %d\n", __func__, ret);
	}

	return result;
}

size_t plat_scmi_rstd_count(unsigned int agent_id)
{
	(void)agent_id;

	/*
	 * Report the count of reset domains actually exposed via SCMI
	 * (scmi_reset_map[]), not PLM's live total reset count, which
	 * is not indexable into scmi_reset_map[].
	 */
	return scmi_reset_count;
}

int32_t plat_scmi_rstd_autonomous(unsigned int agent_id, unsigned int scmi_id,
				  unsigned int state)
{
	enum pm_api_ret_status pm_ret;
	int32_t ret = SCMI_SUCCESS;
	uint32_t encoded_id;
	uint32_t map_flags;
	(void)agent_id;

	if (scmi_reset_map_get(scmi_id, &encoded_id, &map_flags) != 0) {
		return SCMI_NOT_FOUND;
	}

	/* Not instantiated by the design -- see SCMI_RST_FLAG_NOT_PRESENT. */
	if ((map_flags & SCMI_RST_FLAG_NOT_PRESENT) != 0U) {
		return SCMI_NOT_SUPPORTED;
	}

	/* Supports only reset with context loss */
	if (state != 0U) {
		ret = SCMI_NOT_SUPPORTED;
	} else {
		pm_ret = pm_api_reset_assert(encoded_id, PM_RESET_ACTION_PULSE);
		ret = pm_error_to_scmi(pm_ret);
		if (ret != SCMI_SUCCESS) {
			NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
		}
	}

	return ret;
}

int32_t plat_scmi_rstd_set_state(unsigned int agent_id, unsigned int scmi_id,
				 bool assert_not_deassert)
{
	enum pm_api_ret_status pm_ret;
	uint32_t encoded_id;
	uint32_t map_flags;
	int32_t ret;
	(void)agent_id;

	if (scmi_reset_map_get(scmi_id, &encoded_id, &map_flags) != 0) {
		return SCMI_NOT_FOUND;
	}

	/* Not instantiated by the design -- see SCMI_RST_FLAG_NOT_PRESENT. */
	if ((map_flags & SCMI_RST_FLAG_NOT_PRESENT) != 0U) {
		return SCMI_NOT_SUPPORTED;
	}

	if (assert_not_deassert) {
		pm_ret = pm_api_reset_assert(encoded_id, PM_RESET_ACTION_ASSERT);
	} else {
		pm_ret = pm_api_reset_assert(encoded_id, PM_RESET_ACTION_RELEASE);
	}

	ret = pm_error_to_scmi(pm_ret);
	if (ret != SCMI_SUCCESS) {
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
	}

	return ret;
}
