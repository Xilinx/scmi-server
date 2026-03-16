/*
 * Copyright (c) 2022, Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PM_DEFS_H
#define PM_DEFS_H

/* Power management enums and defines */

/* Node class identifiers used for SCMI ID encoding */
#define XPM_NODECLASS_CLOCK			0x2U
#define XPM_NODECLASS_RESET			0x3U
#define XPM_NODECLASS_DEVICE			0x6U

/* Query IDs used for PM_QUERY_DATA API */
#define XPM_QID_GET_NAME			0x10U
#define XPM_QID_GET_NODE_COUNT			0x11U

#if IPI_CRC_CHECK
#define PAYLOAD_ARG_CNT         8U
#define IPI_W0_TO_W6_SIZE       28U
#define PAYLOAD_CRC_POS         7U
#define CRC_INIT_VALUE          0x4F4EU
#define CRC_ORDER               16U
#define CRC_POLYNOM             0x8005U
#else
#define PAYLOAD_ARG_CNT		7U
#endif
#define RET_PAYLOAD_ARG_CNT	6U
#define PAYLOAD_ARG_SIZE	4U	/* size in bytes */

enum pm_api_reset_action {
	PM_RESET_ACTION_RELEASE = 0,
	PM_RESET_ACTION_ASSERT = 1,
	PM_RESET_ACTION_PULSE = 2,
};

/**
 * enum pm_api_ret_status - enum represents the return status codes for a PM
 *                          API operation.
 * @PM_RET_SUCCESS: success.
 * @PM_RET_ERROR_ARGS: illegal arguments provided.
 * @PM_RET_ERROR_NOTSUPPORTED: feature not supported.
 * @PM_RET_ERROR_IOCTL_NOT_SUPPORTED: IOCTL is not supported.
 * @PM_RET_ERROR_INVALID_CRC: invalid crc in IPI communication.
 * @PM_RET_ERROR_NOT_ENABLED: feature is not enabled.
 * @PM_RET_ERROR_INTERNAL: internal error.
 * @PM_RET_ERROR_CONFLICT: conflict.
 * @PM_RET_ERROR_ACCESS: access rights violation.
 * @PM_RET_ERROR_INVALID_NODE: invalid node.
 * @PM_RET_ERROR_DOUBLE_REQ: duplicate request for same node.
 * @PM_RET_ERROR_ABORT_SUSPEND: suspend procedure has been aborted.
 * @PM_RET_ERROR_TIMEOUT: timeout in communication with PMU.
 * @PM_RET_ERROR_NODE_USED: node is already in use.
 * @PM_RET_ERROR_NO_FEATURE: indicates that the requested feature is not
 *                           supported.
 */
enum pm_api_ret_status {
	PM_RET_SUCCESS = 0U,
	PM_RET_ERROR_ARGS = 1U,
	PM_RET_ERROR_NOTSUPPORTED = 4U,
	PM_RET_ERROR_IOCTL_NOT_SUPPORTED = 19U,
	PM_RET_ERROR_NOT_ENABLED = 29U,
	PM_RET_ERROR_INVALID_CRC = 301U,
	PM_RET_ERROR_INTERNAL = 2000U,
	PM_RET_ERROR_CONFLICT = 2001U,
	PM_RET_ERROR_ACCESS = 2002U,
	PM_RET_ERROR_INVALID_NODE = 2003U,
	PM_RET_ERROR_DOUBLE_REQ = 2004U,
	PM_RET_ERROR_ABORT_SUSPEND = 2005U,
	PM_RET_ERROR_TIMEOUT = 2006U,
	PM_RET_ERROR_NODE_USED = 2007U,
	PM_RET_ERROR_NO_FEATURE = 2008U
};

/* Node capabilities */
#define PM_API_CAPABILITY_ACCESS     0x1U
#define PM_API_CAPABILITY_CONTEXT    0x2U
#define PM_API_CAPABILITY_WAKEUP     0x4U
#define PM_API_CAPABILITY_UNUSABLE   0x8U

#define PM_API_MAX_QOS               100U

/* Device states - used in PM_GET_NODE_STATUS response */
#define XPM_DEVSTATE_RUNNING            1U
#define XPM_DEVSTATE_SUSPENDING         8U
#define XPM_DEVSTATE_PENDING_PWR_DWN    11U

enum pm_api_request_ack {
	PM_API_REQUEST_ACK_NO = 1,
	PM_API_REQUEST_ACK_BLOCKING = 2,
	PM_API_REQUEST_ACK_NON_BLOCKING = 3,
};

/* PM API IDs (bits 0-7) */
#define PM_API_GET_NODE_STATUS			0x03U
#define PM_API_REQUEST_NODE			0x0DU
#define PM_API_SET_REQUIREMENT			0x0FU
#define PM_API_RESET_ASSERT			0x11U
#define PM_QUERY_DATA				0x23U
#define PM_API_CLOCK_ENABLE			0x24U
#define PM_API_CLOCK_DISABLE			0x25U
#define PM_API_CLOCK_GET_RATE			0x2AU
#define PM_FEATURE_CHECK			0x3FU
#define PM_API_CLOCK_DESCRIBERATE		0x4DU
#define PM_CLOCK_PROGRAM_RATE			0x4EU

/* Module IDs */
#define PM_MODULE_ID				0x0U
#define XPM_MODULE_ID				0x2U

/* Masks for extracting module ID and API ID */
#define API_ID_MASK				0xFFU
#define MODULE_ID_MASK				0xF00U
#define MODULE_ID_SHIFT				8U

/* Helper macro for field preparation */
#define FIELD_PREP(mask, val)			((((uint32_t)(val)) << MODULE_ID_SHIFT) & (mask))

/* Helper macro to build PM API payload header (payload[0]) */
#define PM_PACK_PAYLOAD_HDR(module_id, api_id)	(FIELD_PREP(MODULE_ID_MASK, module_id) | (api_id))

#endif /* PM_DEFS_H */
