/*
 * Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>

#include "platform/pm_defs.h"
#include "platform/pm_api.h"
#include "platform/scmi_plat.h"
#include "common.h"

/*
 * Platform SCMI reset domains
 */

const char *plat_scmi_rstd_get_name(unsigned int agent_id, unsigned int scmi_id)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_RESET, scmi_id);
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	static char name_buffer[16];
	enum pm_api_ret_status ret;
	const char *result;
	(void)agent_id;

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
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	enum pm_api_ret_status ret;
	size_t count;
	(void)agent_id;

	/* Query firmware for reset count using XPM_QID_GET_NODE_COUNT with RESET class */
	ret = pm_api_query_data(XPM_QID_GET_NODE_COUNT, XPM_NODECLASS_RESET, 0, 0, data, 1U);

	if (ret == PM_RET_SUCCESS) {
		count = (size_t)data[0];
	} else {
		count = 0U;
		NOTICE("%s: failed with err code %d\n", __func__, ret);
	}

	return count;
}

int32_t plat_scmi_rstd_autonomous(unsigned int agent_id, unsigned int scmi_id,
				  unsigned int state)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_RESET, scmi_id);
	enum pm_api_ret_status pm_ret;
	int32_t ret = SCMI_SUCCESS;
	(void)agent_id;

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
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_RESET, scmi_id);
	enum pm_api_ret_status pm_ret;
	int32_t ret;
	(void)agent_id;

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
