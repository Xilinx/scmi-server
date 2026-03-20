/*
 * Copyright (c) 2022, Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Platform Management API declarations */

#ifndef PM_API_H
#define PM_API_H

#include <stdint.h>
#include <stddef.h>
#include "platform/pm_defs.h"

/* Clock management APIs */
enum pm_api_ret_status pm_api_get_rate(const uint32_t node, uint32_t *rate);
enum pm_api_ret_status pm_api_set_rate(const uint32_t node, uint32_t rate);
enum pm_api_ret_status pm_api_clock_describe_rate(const uint32_t node, uint32_t index,
			       uint32_t *data, size_t count);

/* Query API */
enum pm_api_ret_status pm_api_query_data(uint32_t qid, uint32_t arg1, uint32_t arg2,
		      uint32_t arg3, uint32_t *data, size_t count);

/* Reset management APIs */
enum pm_api_ret_status pm_api_reset_assert(const uint32_t reset,
			const enum pm_api_reset_action assert_flag);

/* Power domain management APIs */
enum pm_api_ret_status pm_api_request_node(const uint32_t node, const uint32_t capabilities,
			const uint32_t qos, const enum pm_api_request_ack ack);
enum pm_api_ret_status pm_api_set_requirement(const uint32_t node, const uint32_t capabilities,
			   const uint32_t qos, const enum pm_api_request_ack ack);
enum pm_api_ret_status pm_api_get_node_status(uint32_t node, uint32_t *state);

#endif /* PM_API_H */
