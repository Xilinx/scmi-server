/*
 * Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include "platform/pm_defs.h"
#include "platform/pm_api.h"
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

#define HIGH (1)
#define LOW (0)

#define XPM_NODESUBCLASS_SCMI		0xAU
#define XPM_NODETYPE_SCMI		0x0U

#define XPM_NODEID_CLASS_SHIFT		26
#define XPM_NODEID_SUBCLASS_SHIFT	20
#define XPM_NODEID_TYPE_SHIFT		14
#define XPM_NODEID_INDEX_MASK		0x3FFF

/* Format descriptor bit field definitions */
#define FORMAT_DESC_NUM_RATES_MASK       0xFFF
#define FORMAT_DESC_IS_RANGE_SHIFT       12
#define FORMAT_DESC_IS_RANGE_MASK        0x1
#define FORMAT_DESC_NUM_REMAINING_SHIFT  16
#define FORMAT_DESC_NUM_REMAINING_MASK   0xFFFF

#define PD_STATE_ON  0
#define PD_STATE_OFF 0x40000000

static inline uint32_t scmi_encode_node_id(uint32_t node_class, uint32_t scmi_id)
{
	return ((node_class & 0x3FU) << XPM_NODEID_CLASS_SHIFT) |
	       (((uint32_t)XPM_NODESUBCLASS_SCMI & 0x3FU) << XPM_NODEID_SUBCLASS_SHIFT) |
	       (((uint32_t)XPM_NODETYPE_SCMI & 0x3FU) << XPM_NODEID_TYPE_SHIFT) |
	       (scmi_id & (uint32_t)XPM_NODEID_INDEX_MASK);
}

/*
 * Generic PM API error to SCMI error mapper
 */
static int32_t pm_error_to_scmi(enum pm_api_ret_status pm_ret)
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

/*
 * Platform SCMI power domains
 */

size_t plat_scmi_pd_count(unsigned int agent_id)
{
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	enum pm_api_ret_status ret;
	size_t count;
	(void)agent_id;

	/* Query firmware for power domain count using XPM_QID_GET_NODE_COUNT with POWER class */
	ret = pm_api_query_data(XPM_QID_GET_NODE_COUNT, XPM_NODECLASS_DEVICE, 0, 0, data, 1U);

	if (ret == PM_RET_SUCCESS) {
		count = (size_t)data[0];
	} else {
		count = 0U;
		NOTICE("%s: failed with err code %d\n", __func__, ret);
	}

	return count;
}

const char *plat_scmi_pd_get_name(unsigned int agent_id, unsigned int pd_id)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_DEVICE, pd_id);
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	static char name_buffer[16];
	enum pm_api_ret_status ret;
	const char *result;
	(void)agent_id;

	/* Query firmware for power domain name using XPM_QID_GET_NAME */
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

unsigned int plat_scmi_pd_statistics(unsigned int agent_id, unsigned long *pd_id)
{
	(void)agent_id;
	(void)pd_id;
	/* We don't support stats */
	return 0U;
}

unsigned int plat_scmi_pd_get_attributes(unsigned int agent_id, unsigned int pd_id)
{
	(void)agent_id;
	(void)pd_id;
	/* We don't support notification and async set */
	return 0U;
}

unsigned int plat_scmi_pd_get_state(unsigned int agent_id, unsigned int pd_id)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_DEVICE, pd_id);
	enum pm_api_ret_status pm_ret;
	unsigned int scmi_state;
	uint32_t dev_state = 0;
	(void)agent_id;

	/* Query firmware for node status using PM_GET_NODE_STATUS */
	pm_ret = pm_api_get_node_status(encoded_id, &dev_state);

	if (pm_ret != PM_RET_SUCCESS) {
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
		scmi_state = PD_STATE_OFF;
	} else {
		/*
		 * XST_SUCCESS case - Map device state to SCMI power state:
		 * ON states: XPM_DEVSTATE_RUNNING (1), XPM_DEVSTATE_SUSPENDING (8),
		 *            XPM_DEVSTATE_PENDING_PWR_DWN (11)
		 * OFF states: Any other value
		 */
		if ((dev_state == XPM_DEVSTATE_RUNNING) ||
		    (dev_state == XPM_DEVSTATE_SUSPENDING) ||
		    (dev_state == XPM_DEVSTATE_PENDING_PWR_DWN)) {
			scmi_state = PD_STATE_ON;
		} else {
			scmi_state = PD_STATE_OFF;
		}
	}

	return scmi_state;
}

int32_t plat_scmi_pd_set_state(unsigned int agent_id, unsigned int flags, unsigned int pd_id,
			       unsigned int state)
{
	uint32_t encoded_id = scmi_encode_node_id(XPM_NODECLASS_DEVICE, pd_id);
	enum pm_api_ret_status pm_ret;
	int32_t ret;
	(void)agent_id;
	(void)flags;

	if (state == 0U) {
		/* Power ON: request node and set requirement */
		pm_ret = pm_api_request_node(encoded_id, 0, 0,
					     PM_API_REQUEST_ACK_BLOCKING);

		if (pm_ret != PM_RET_SUCCESS) {
			ret = pm_error_to_scmi(pm_ret);
		} else {
			pm_ret = pm_api_set_requirement(encoded_id,
							PM_API_CAPABILITY_ACCESS,
							PM_API_MAX_QOS,
							PM_API_REQUEST_ACK_BLOCKING);
			/* Common error handling for set_requirement */
			ret = pm_error_to_scmi(pm_ret);
		}
	} else {
		/* Power OFF: set requirement to unusable */
		pm_ret = pm_api_set_requirement(encoded_id,
						PM_API_CAPABILITY_UNUSABLE, 0,
						PM_API_REQUEST_ACK_NO);
		/* Common error handling for set_requirement */
		ret = pm_error_to_scmi(pm_ret);
	}

	if (ret != SCMI_SUCCESS) {
		NOTICE("%s: failed with err code %d\n", __func__, pm_ret);
	}

	return ret;
}

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
