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

#define HIGH (1)
#define LOW (0)

/* Format descriptor bit field definitions */
#define FORMAT_DESC_NUM_RATES_MASK       0xFFF
#define FORMAT_DESC_IS_RANGE_SHIFT       12
#define FORMAT_DESC_IS_RANGE_MASK        0x1
#define FORMAT_DESC_NUM_REMAINING_SHIFT  16
#define FORMAT_DESC_NUM_REMAINING_MASK   0xFFFF

/*
 * Platform SCMI clock
 */
size_t plat_scmi_clock_count(unsigned int agent_id)
{
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	enum pm_api_ret_status ret;
	size_t count;
	(void)agent_id;

	/* Query firmware for clock count using XPM_QID_GET_NODE_COUNT with CLOCK class */
	ret = pm_api_query_data(XPM_QID_GET_NODE_COUNT, XPM_NODECLASS_CLOCK, 0, 0, data, 1U);

	if (ret == PM_RET_SUCCESS) {
		count = (size_t)data[0];
	} else {
		count = 0U;
		NOTICE("%s: failed with err code %d\n", __func__, ret);
	}

	return count;
}

const char *plat_scmi_clock_get_name(unsigned int agent_id, unsigned int scmi_id)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_CLOCK, scmi_id);
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	static char name_buffer[16];
	enum pm_api_ret_status ret;
	const char *result;
	(void)agent_id;

	/* Query firmware for clock name using XPM_QID_GET_NAME */
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

int32_t plat_scmi_clock_rates_array(unsigned int agent_id, unsigned int scmi_id,
				    unsigned long *rates, size_t *nb_elts,
				    uint32_t start_idx)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_CLOCK, scmi_id);
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	enum pm_api_ret_status pm_ret;
	uint32_t num_rates_remaining;
	uint32_t num_rates_returned;
	int32_t ret = SCMI_SUCCESS;
	uint32_t format_desc;
	uint32_t is_range;
	(void)agent_id;

	/* Call PLM to describe the rates for this clock at the specified index */
	pm_ret = pm_api_clock_describe_rate(encoded_id, start_idx, data, RET_PAYLOAD_ARG_CNT - 1U);

	if (pm_ret != PM_RET_SUCCESS) {
		ret = pm_error_to_scmi(pm_ret);
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
		goto exit_label;
	}

	/* Parse format descriptor */
	format_desc = data[0];
	num_rates_returned = format_desc & (uint32_t)FORMAT_DESC_NUM_RATES_MASK;
	is_range = (format_desc >> FORMAT_DESC_IS_RANGE_SHIFT) & (uint32_t)FORMAT_DESC_IS_RANGE_MASK;
	num_rates_remaining = (format_desc >> FORMAT_DESC_NUM_REMAINING_SHIFT) &
			      (uint32_t)FORMAT_DESC_NUM_REMAINING_MASK;

	/*
	 * This function handles ONLY discrete rate arrays.
	 * If PLM returns range format, signal NOT_SUPPORTED so caller uses
	 * plat_scmi_clock_rates_by_step.
	 */
	if (is_range != 0U) {
		ret = SCMI_NOT_SUPPORTED;
		goto exit_label;
	}

	/*
	 * Enforce the platform limit on discrete clock-rate entries.
	 *
	 * The implementation with IPI can handle at most 3 discrete rates in a single
	 * response.
	 */
	if (num_rates_returned > 3U) {
		ret = SCMI_NOT_SUPPORTED;
		goto exit_label;
	}

	if (rates == NULL) {
		/*
		 * First call to get total count for discrete rates.
		 * Total = rates returned in this call + remaining rates
		 */
		*nb_elts = (size_t)num_rates_returned + (size_t)num_rates_remaining;
	} else {
		/*
		 * Discrete list - fill in what was returned.
		 */
		size_t count = (*nb_elts < num_rates_returned) ? *nb_elts : num_rates_returned;

		for (size_t i = 0; i < count; i++) {
			rates[i] = data[i + 1U];  /* Skip data[0] (format) */
		}
		*nb_elts = count;
	}

exit_label:
	return ret;
}

unsigned long plat_scmi_clock_get_rate(unsigned int agent_id, unsigned int scmi_id)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_CLOCK, scmi_id);
	enum pm_api_ret_status pm_ret;
	unsigned long ret;
	uint32_t rate = 0;
	(void)agent_id;

	/* Call PLM to get the current rate */
	pm_ret = pm_api_get_rate(encoded_id, &rate);

	if (pm_ret == PM_RET_SUCCESS) {
		ret = (unsigned long) rate;
	} else {
		ret = 0UL;
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
	}

	return ret;
}

int32_t plat_scmi_clock_set_rate(unsigned int agent_id, unsigned int scmi_id,
				 unsigned long rate)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_CLOCK, scmi_id);
	enum pm_api_ret_status pm_ret;
	int32_t ret = SCMI_SUCCESS;
	(void)agent_id;

	/* Call PLM to set the rate */
	pm_ret = pm_api_set_rate(encoded_id, (uint32_t)rate);

	ret = pm_error_to_scmi(pm_ret);
	if (ret != SCMI_SUCCESS) {
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
	}

	return ret;
}

int32_t plat_scmi_clock_get_state(unsigned int agent_id, unsigned int scmi_id)
{
	(void)agent_id;
	(void)scmi_id;
	/*
	 * Clock is always enabled in PLM and we don't have support to enable/disable
	 * the same, so return enabled as default for compatibility
	 */
	return HIGH;
}

int32_t plat_scmi_clock_set_state(unsigned int agent_id, unsigned int scmi_id,
				  bool enable_not_disable)
{
	(void)enable_not_disable;
	(void)agent_id;
	(void)scmi_id;
	/*
	 * Clock is always enabled in PLM and we don't have support to enable/disable
	 * the same, so return SUCCESS as default for compatibility
	 */
	return SCMI_SUCCESS;
}

int32_t plat_scmi_clock_rates_by_step(unsigned int agent_id, unsigned int scmi_id,
				      unsigned long *min_max_step)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_CLOCK, scmi_id);
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	enum pm_api_ret_status pm_ret;
	uint32_t num_rates_returned;
	int32_t ret = SCMI_SUCCESS;
	uint32_t format_desc;
	uint32_t is_range;
	(void)agent_id;

	/* Call PLM to describe the rates for this clock (index 0 for range format) */
	pm_ret = pm_api_clock_describe_rate(encoded_id, 0, data, RET_PAYLOAD_ARG_CNT - 1U);

	if (pm_ret != PM_RET_SUCCESS) {
		ret = pm_error_to_scmi(pm_ret);
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
		goto exit_label;
	}

	/* Parse format descriptor */
	format_desc = data[0];
	num_rates_returned = format_desc & (uint32_t)FORMAT_DESC_NUM_RATES_MASK;
	is_range = (format_desc >> FORMAT_DESC_IS_RANGE_SHIFT) & (uint32_t)FORMAT_DESC_IS_RANGE_MASK;

	/*
	 * Accept this call only if we have exactly 3 rates (min, max, step) and
	 * the format is range.
	 */
	if ((is_range == 0U) || (num_rates_returned != 3U)) {
		ret = SCMI_NOT_SUPPORTED;
		goto exit_label;
	}

	/* Range format: extract min, max, step */
	min_max_step[0] = data[1];  /* Min rate */
	min_max_step[1] = data[2];  /* Max rate */
	min_max_step[2] = data[3];  /* Step */

exit_label:
	return ret;
}
