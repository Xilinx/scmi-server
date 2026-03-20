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
#include "platform/pm_api.h"
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

/**
 * pm_api_get_rate() - Get the current rate of clock for given id
 * @node: ID of the clock
 * @rate: Rate retrieved from the firmware
 *
 * This function is used by master to get the current rate of the clock
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_get_rate(const uint32_t node, uint32_t *rate)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if clock get rate API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_API_CLOCK_GET_RATE));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_API_CLOCK_GET_RATE);
		payload[1] = node;

		result = pm_ipi_send_sync(primary_proc, payload, rate, 1);
	}

	return result;
}

/**
 * pm_api_set_rate() - Set the rate of clock for given id
 * @node: ID of the clock
 * @rate: Rate to be set
 *
 * This function is used by master to set the rate of the clock
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_set_rate(const uint32_t node, uint32_t rate)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if clock set rate API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_CLOCK_PROGRAM_RATE));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_CLOCK_PROGRAM_RATE);
		payload[1] = node;
		payload[2] = rate;

		result = pm_ipi_send_sync(primary_proc, payload, NULL, 0);
	}

	return result;
}

/**
 * pm_api_query_data() - Query firmware data
 * @qid: Query ID
 * @arg1: Argument 1
 * @arg2: Argument 2
 * @arg3: Argument 3
 * @data: Returned data
 * @count: Number of return values expected
 *
 * This function is used to query firmware for various data
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_query_data(uint32_t qid, uint32_t arg1, uint32_t arg2,
		      uint32_t arg3, uint32_t *data, size_t count)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if query data API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_QUERY_DATA));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_QUERY_DATA);
		payload[1] = qid;
		payload[2] = arg1;
		payload[3] = arg2;
		payload[4] = arg3;

		result = pm_ipi_send_sync(primary_proc, payload, data, count);
	}

	return result;
}

/**
 * pm_api_clock_describe_rate() - Describe supported rates for a clock
 * @node: ID of the clock (encoded SCMI_ID)
 * @index: Starting index for rate description (for pagination)
 * @data: Returned rate data (format descriptor + rates)
 * @count: Number of return values expected
 *
 * This function queries the firmware for supported rates for a given clock.
 * Response format:
 *   data[0]: Format descriptor
 *            - Bits [0-11]: Number of rates returned
 *            - Bit [12]: Return format (0=discrete list, 1=range)
 *            - Bits [13-15]: Reserved
 *            - Bits [16-31]: Number of remaining rates
 *   data[1-N]: Rate data (format depends on bit[12] of data[0])
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_clock_describe_rate(const uint32_t node, uint32_t index,
			       uint32_t *data, size_t count)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if clock describe rate API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_API_CLOCK_DESCRIBERATE));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_API_CLOCK_DESCRIBERATE);
		payload[1] = node;
		payload[2] = index;

		result = pm_ipi_send_sync(primary_proc, payload, data, count);
	}

	return result;
}

/**
 * pm_api_reset_assert() - Request setting of reset (1 - assert, 0 - release)
 * @reset: Reset to be configured
 * @assert_flag: Flag stating should reset be asserted (1) or
 *               released (0)
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_reset_assert(const uint32_t reset,
			const enum pm_api_reset_action assert_flag)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if reset API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_API_RESET_ASSERT));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_API_RESET_ASSERT);
		payload[1] = reset;
		payload[2] = (uint32_t)assert_flag;

		result = pm_ipi_send_sync(primary_proc, payload, NULL, 0);
	}

	return result;
}

/**
 * pm_api_set_requirement() - PM call to set requirement for PM slaves
 * @node: Node ID of the slave
 * @capabilities: Requested capabilities of the slave
 * @qos: Quality of service (not supported)
 * @ack: Flag to specify whether acknowledge is requested
 *
 * This API function is to be used for slaves a PU already has requested
 * to change its capabilities.
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_set_requirement(const uint32_t node, const uint32_t capabilities,
			   const uint32_t qos,
			   const enum pm_api_request_ack ack)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if set requirement API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_API_SET_REQUIREMENT));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_API_SET_REQUIREMENT);
		payload[1] = node;
		payload[2] = capabilities;
		payload[3] = qos;
		payload[4] = (uint32_t)ack;

		result = pm_ipi_send_sync(primary_proc, payload, NULL, 0);
	}

	return result;
}

/**
 * pm_api_request_node() - Request a node with specific capabilities
 * @node: Node ID of the slave
 * @capabilities: Requested capabilities of the slave
 * @qos: Quality of service (not supported)
 * @ack: Flag to specify whether acknowledge is requested
 *
 * This function is used by master to request particular node from firmware.
 * Every master must request node before using it.
 *
 * Return: Returns status, either success or error+reason
 */

enum pm_api_ret_status pm_api_request_node(const uint32_t node, const uint32_t capabilities,
			const uint32_t qos,
			const enum pm_api_request_ack ack)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if request node API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_API_REQUEST_NODE));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_API_REQUEST_NODE);
		payload[1] = node;
		payload[2] = capabilities;
		payload[3] = qos;
		payload[4] = (uint32_t)ack;

		result = pm_ipi_send_sync(primary_proc, payload, NULL, 0);
	}

	return result;
}

/**
 * pm_api_get_node_status() - Get node status from firmware
 * @node: Node ID
 * @state: Returned state value
 *
 * This function is used to get the current status of a node
 *
 * Return: Returns status, either success or error+reason
 */
enum pm_api_ret_status pm_api_get_node_status(uint32_t node, uint32_t *state)
{
	uint32_t payload[PAYLOAD_ARG_CNT];
	enum pm_api_ret_status result;
	uint32_t feature_check_api;

	/* Check if get node status API is supported by firmware before sending the command */
	feature_check_api = pm_api_feature_check(PM_PACK_PAYLOAD_HDR(PM_MODULE_ID, PM_API_GET_NODE_STATUS));
	if (feature_check_api == (uint32_t)PM_RET_ERROR_NO_FEATURE) {
		/* API not supported by firmware */
		result = PM_RET_ERROR_NO_FEATURE;
	} else {
		zeromem(&payload[0], sizeof(payload));

		payload[0] = PM_PACK_PAYLOAD_HDR(XPM_MODULE_ID, PM_API_GET_NODE_STATUS);
		payload[1] = node;

		result = pm_ipi_send_sync(primary_proc, payload, state, 1);
	}

	return result;
}
