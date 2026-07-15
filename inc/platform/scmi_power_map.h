/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_POWER_MAP_H
#define SCMI_POWER_MAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * SCMI power domain id -> PLM/EEMI device (power) id translation.
 *
 * The SCMI power domain id is the array index (key) into scmi_power_map[] and
 * forms a compact 0..scmi_power_count-1 space. Each entry describes the
 * PLM/EEMI device id to use when communicating with the PLM, plus a flags field
 * (reserved, 0 for now).
 */
struct scmi_power_map_entry {
	uint32_t plm_id;	/* PLM/EEMI device (power) id used in PLM communication */
	uint32_t flags;		/* reserved for future use, 0 for now */
};

extern struct scmi_power_map_entry scmi_power_map[];

/* Number of reset domains in scmi_power_map[] (derived from the table). */
extern const size_t scmi_power_count;

/*
 * Look up the PLM/EEMI device id and flags for a given SCMI power domain id.
 *
 * @scmi_id: SCMI power domain id (array index / key)
 * @plm_id:  optional, receives the PLM/EEMI device id on success
 * @flags:   optional, receives the entry flags on success
 *
 * Returns 0 on success, or -1 if @scmi_id is out of range.
 */
static inline int32_t scmi_power_map_get(unsigned int scmi_id,
					 uint32_t *plm_id, uint32_t *flags)
{
	if (scmi_id >= scmi_power_count) {
		return -1;
	}

	if (plm_id != NULL) {
		*plm_id = scmi_power_map[scmi_id].plm_id;
	}

	if (flags != NULL) {
		*flags = scmi_power_map[scmi_id].flags;
	}

	return 0;
}

#endif /* SCMI_POWER_MAP_H */
