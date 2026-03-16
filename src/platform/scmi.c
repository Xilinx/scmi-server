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

/*
 * struct scmi_reset - Data for the exposed reset controller
 * @reset_id: Reset identifier in RCC reset driver
 * @name: Reset string ID exposed to agent
 */
struct scmi_reset {
	unsigned long reset_id;
	const char *name;
};

#define RESET_CELL(_scmi_id, _id, _name) \
	[_scmi_id] = { \
		.reset_id = (_id), \
		.name = (_name), \
	}

static struct scmi_reset scmi0_reset[] = {
	RESET_CELL(RESET_GEM0_0, RESET_GEM0_0, "gem0"),
	RESET_CELL(RESET_GEM1_0, RESET_GEM1_0, "gem1"),
	RESET_CELL(RESET_SERIAL0_0, RESET_SERIAL0_0, "serial0"),
	RESET_CELL(RESET_SERIAL1_0, RESET_SERIAL1_0, "serial1"),
	RESET_CELL(RESET_UFS0_0, RESET_UFS0_0, "ufs0"),
	RESET_CELL(RESET_I2C0_0, RESET_I2C0_0, "i2c0"),
	RESET_CELL(RESET_I2C1_0, RESET_I2C1_0, "i2c1"),
	RESET_CELL(RESET_I2C2_0, RESET_I2C2_0, "i2c2"),
	RESET_CELL(RESET_I2C3_0, RESET_I2C3_0, "i2c3"),
	RESET_CELL(RESET_I2C4_0, RESET_I2C4_0, "i2c4"),
	RESET_CELL(RESET_I2C5_0, RESET_I2C5_0, "i2c5"),
	RESET_CELL(RESET_I2C6_0, RESET_I2C6_0, "i2c6"),
	RESET_CELL(RESET_I2C7_0, RESET_I2C7_0, "i2c7"),
	RESET_CELL(RESET_I2C8_0, RESET_I2C8_0, "i2c8"),
	RESET_CELL(RESET_OSPI0_0, RESET_OSPI0_0, "ospi"),
	RESET_CELL(RESET_USB0_0, RESET_USB0_0, "usb0_0"),
	RESET_CELL(RESET_USB0_1, RESET_USB0_1, "usb0_1"),
	RESET_CELL(RESET_USB0_2, RESET_USB0_2, "usb0_2"),
	RESET_CELL(RESET_USB1_0, RESET_USB1_0, "usb1_0"),
	RESET_CELL(RESET_USB1_1, RESET_USB1_1, "usb1_1"),
	RESET_CELL(RESET_USB1_2, RESET_USB1_2, "usb1_2"),
	RESET_CELL(RESET_MMC0_0, RESET_MMC0_0, "mmc0"),
	RESET_CELL(RESET_MMC1_0, RESET_MMC1_0, "mmc1"),
	RESET_CELL(RESET_SPI0_0, RESET_SPI0_0, "spi0"),
	RESET_CELL(RESET_SPI1_0, RESET_SPI1_0, "spi1"),
	RESET_CELL(RESET_QSPI0_0, RESET_QSPI0_0, "qspi"),
	RESET_CELL(RESET_I3C0_0, RESET_I3C0_0, "i3c0"),
	RESET_CELL(RESET_I3C1_0, RESET_I3C1_0, "i3c1"),
	RESET_CELL(RESET_I3C2_0, RESET_I3C2_0, "i3c2"),
	RESET_CELL(RESET_I3C3_0, RESET_I3C3_0, "i3c3"),
	RESET_CELL(RESET_I3C4_0, RESET_I3C4_0, "i3c4"),
	RESET_CELL(RESET_I3C5_0, RESET_I3C5_0, "i3c5"),
	RESET_CELL(RESET_I3C6_0, RESET_I3C6_0, "i3c6"),
	RESET_CELL(RESET_I3C7_0, RESET_I3C7_0, "i3c7"),
	RESET_CELL(RESET_I3C8_0, RESET_I3C8_0, "i3c8"),
	RESET_CELL(RESET_UFSPHY_0, RESET_UFSPHY_0, "ufsphy0"),
};

/**
 * struct scmi_pd - Data for the exposed power domain controller
 * @pd_id: pd identifier in RCC reset driver
 * @name: pd string ID exposed to agent
 * @state: keep state setting
 */
struct scmi_pd {
	unsigned long pd_id;
	const char *name;
	unsigned int state;
};

#define PD_CELL(_scmi_id, _id, _name, _state) \
	[_scmi_id] = { \
		.pd_id = _id, \
		.name = _name, \
		.state = _state, \
	}

static struct scmi_pd scmi0_pd[] = {
	PD_CELL(PD_USB0, PD_USB0, "usb0", 0),
	PD_CELL(PD_USB1, PD_USB1, "usb1", 0),
};

struct scmi_resources {
	struct scmi_reset *reset;
	size_t reset_count;
	struct scmi_pd *pd;
	size_t pd_count;
};

static const struct scmi_resources resources[] = {
	[0] = {
		.reset = scmi0_reset,
		.reset_count = ARRAY_SIZE(scmi0_reset),
		.pd = scmi0_pd,
		.pd_count = ARRAY_SIZE(scmi0_pd),
	},
};

static const struct scmi_resources *find_resource(unsigned int agent_id)
{
	assert(agent_id < ARRAY_SIZE(resources));

	return &resources[agent_id];
}

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
static struct scmi_reset *find_reset(unsigned int agent_id,
					 unsigned int scmi_id)
{
	const struct scmi_resources *resource = find_resource(agent_id);
	size_t n;

	if (resource != NULL) {
		for (n = 0U; n < resource->reset_count; n++) {
			if (n == scmi_id) {
				return &resource->reset[n];
			}
		}
	}

	return NULL;
}

const char *plat_scmi_rstd_get_name(unsigned int agent_id, unsigned int scmi_id)
{
	const struct scmi_reset *reset = find_reset(agent_id, scmi_id);

	if (reset == NULL) {
		return NULL;
	}

	return reset->name;
}

size_t plat_scmi_rstd_count(unsigned int agent_id)
{
	const struct scmi_resources *resource = find_resource(agent_id);

	if (resource == NULL) {
		return 0U;
	}

	return resource->reset_count;
}

int32_t plat_scmi_rstd_autonomous(unsigned int agent_id, unsigned int scmi_id,
				uint32_t state)
{
	const struct scmi_reset *reset = find_reset(agent_id, scmi_id);

	if (reset == NULL) {
		return SCMI_NOT_FOUND;
	}

	/* Supports only reset with context loss */
	if (state != 0U) {
		return SCMI_NOT_SUPPORTED;
	}

	NOTICE("SCMI reset on ID %lu/%s\n",
	       reset->reset_id, plat_scmi_rstd_get_name(agent_id, scmi_id));

	return SCMI_SUCCESS;
}

int32_t plat_scmi_rstd_set_state(unsigned int agent_id, unsigned int scmi_id,
				 bool assert_not_deassert)
{
	const struct scmi_reset *reset = find_reset(agent_id, scmi_id);

	if (reset == NULL) {
		return SCMI_NOT_FOUND;
	}

	if (assert_not_deassert) {
		NOTICE("SCMI reset %lu/%s set\n",
		       reset->reset_id, plat_scmi_rstd_get_name(agent_id, scmi_id));

		switch (scmi_id) {
		case RESET_UFS0_0:
			mmio_write_32(PMXC_CRP_RST_UFS, 1);
			break;
		case RESET_UFSPHY_0:
			mmio_write_32(PMXC_IOU_SLCR_PHY_RESET, 1);
			break;
		default:
			break;
		}
	} else {
		NOTICE("SCMI reset %lu/%s release\n",
		       reset->reset_id, plat_scmi_rstd_get_name(agent_id, scmi_id));

		switch (scmi_id) {
		case RESET_UFS0_0:
			mmio_write_32(PMXC_CRP_RST_UFS, 0);
			break;
		case RESET_UFSPHY_0:
			mmio_write_32(PMXC_IOU_SLCR_PHY_RESET, 0);
			break;
		default:
			break;
		}
	}

	return SCMI_SUCCESS;
}

/*
 * Platform SCMI reset domains
 */
static struct scmi_pd *find_pd(unsigned int agent_id, unsigned int pd_id)
{
	const struct scmi_resources *resource = find_resource(agent_id);
	size_t n;

	if (resource != NULL) {
		for (n = 0U; n < resource->pd_count; n++) {
			if (n == pd_id) {
				return &resource->pd[n];
			}
		}
	}

	return NULL;
}

size_t plat_scmi_pd_count(unsigned int agent_id)
{
	const struct scmi_resources *resource = find_resource(agent_id);
	size_t ret;

	if (resource == NULL) {
		ret = 0U;
	} else {
		ret = resource->pd_count;

		NOTICE("SCMI: PD: %d\n", (unsigned int)ret);
	}
	return ret;
}

const char *plat_scmi_pd_get_name(unsigned int agent_id, unsigned int pd_id)
{
	const struct scmi_pd *pd = find_pd(agent_id, pd_id);
	const char *ret = NULL;

	if (pd != NULL) {
		ret = pd->name;
	}

	return ret;
}

unsigned int plat_scmi_pd_statistics(unsigned int agent_id, unsigned long *pd_id)
{
	return 0U;
}

unsigned int plat_scmi_pd_get_attributes(unsigned int agent_id, unsigned int pd_id)
{
	return 0U;
}

unsigned int plat_scmi_pd_get_state(unsigned int agent_id, unsigned int pd_id)
{
	const struct scmi_pd *pd = find_pd(agent_id, pd_id);
	uint32_t ret = SCMI_NOT_SUPPORTED;

	if (pd != NULL) {
		NOTICE("SCMI: PD: get id: %d, state: %x\n", pd_id, pd->state);

		ret = pd->state;
	}

	return ret;
}

int32_t plat_scmi_pd_set_state(unsigned int agent_id, unsigned int flags, unsigned int pd_id,
			       unsigned int state)
{
	struct scmi_pd *pd = find_pd(agent_id, pd_id);
	int32_t ret = SCMI_SUCCESS;

	if (pd == NULL) {
		ret = SCMI_NOT_SUPPORTED;
		goto exit_label;
	}

	NOTICE("SCMI: PD: set id: %d, orig state: %x, new state: %x,  flags: %x\n",
			pd_id, pd->state, state, flags);

	pd->state = state;

exit_label:
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
	int32_t ret;

	for (i = 0U; i < ARRAY_SIZE(scmi_channel); i++) {
		scmi_smt_init_agent_channel(&scmi_channel[i]);
	}

	NOTICE("SCMI: Server initialized %s %s %s\n",
	       SCMI_PKG_VERSION, SCMI_BUILD_DATE, SCMI_BUILD_TIME);

	if (platform_id == QEMU) {
		/* default setting is for QEMU */
	} else if (platform_id == SPP) {
		for (i = 0U; i < ARRAY_SIZE(scmi0_clock); i++) {

			/* Keep i2c on 100MHz to calculate rates properly */
			if ((i >= CLK_I2C0_0) && (i <= CLK_I2C7_0)) {
				continue;
			}

			/* Keep UFS clocks to default values to get the expected rates */
			if ((i >= CLK_UFS0_0) && (i <= CLK_UFS0_2)) {
				continue;
			}

			/*
			 * SPP supports multiple versions.
			 * The cpu_clock value is set to corresponding SPP
			 * version in early platform setup, resuse the same
			 * value here.
			 */
			ret = plat_scmi_clock_set_rate(0, i, cpu_clock);
			if (ret < 0) {
				NOTICE("Failed to set clock rate for SPP scmi_id=%ld\n", i);
			}
		}
	} else {
		/* Making MISRA C 2012 15.7 compliant */
	}
}
