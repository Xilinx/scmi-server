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
#include "platform/scmi_power_map.h"
#include "common.h"

#define PD_STATE_ON  0
#define PD_STATE_OFF 0x40000000

/*
 * Platform SCMI power domains
 */

size_t plat_scmi_pd_count(unsigned int agent_id)
{
	(void)agent_id;

	/*
	 * Report the count of power domains actually exposed via SCMI
	 * (scmi_power_map[]), not PLM's live total device count, which
	 * includes every device on the SoC and is not indexable into
	 * scmi_power_map[].
	 */
	return scmi_power_count;
}

const char *plat_scmi_pd_get_name(unsigned int agent_id, unsigned int pd_id)
{
	uint32_t data[RET_PAYLOAD_ARG_CNT];
	static char name_buffer[16];
	enum pm_api_ret_status ret;
	const char *result;
	uint32_t encoded_id;
	uint32_t map_flags;
	(void)agent_id;

	if (scmi_power_map_get(pd_id, &encoded_id, &map_flags) != 0) {
		return NULL;
	}

	/* Gated at runtime for restricted variants. */
	if ((map_flags & SCMI_PWR_FLAG_NOT_PRESENT) != 0U) {
		(void)snprintf(name_buffer, sizeof(name_buffer), "rsvd%u", pd_id);
		return name_buffer;
	}

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
	enum pm_api_ret_status pm_ret;
	unsigned int scmi_state;
	uint32_t dev_state = 0;
	uint32_t encoded_id;
	uint32_t map_flags;
	(void)agent_id;

	if (scmi_power_map_get(pd_id, &encoded_id, &map_flags) != 0) {
		return PD_STATE_OFF;
	}

	/* Gated at runtime for restricted variants. */
	if ((map_flags & SCMI_PWR_FLAG_NOT_PRESENT) != 0U) {
		return PD_STATE_OFF;
	}

	/* Query firmware for node status using PM_GET_NODE_STATUS */
	pm_ret = pm_api_get_node_status(encoded_id, &dev_state);

	if (pm_ret != PM_RET_SUCCESS) {
		NOTICE("%s: pd_id=%u (0x%x) failed with err code %d\n",
		       __func__, pd_id, encoded_id, pm_ret);
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
	enum pm_api_ret_status pm_ret;
	uint32_t encoded_id;
	uint32_t map_flags;
	int32_t ret;
	(void)agent_id;
	(void)flags;

	if (scmi_power_map_get(pd_id, &encoded_id, &map_flags) != 0) {
		return SCMI_NOT_FOUND;
	}

	/* Gated at runtime for restricted variants. */
	if ((map_flags & SCMI_PWR_FLAG_NOT_PRESENT) != 0U) {
		return SCMI_NOT_SUPPORTED;
	}

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
