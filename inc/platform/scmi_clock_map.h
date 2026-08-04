/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_CLOCK_MAP_H
#define SCMI_CLOCK_MAP_H

#include <stddef.h>
#include <stdint.h>

#include "platform/scmi_plat.h"

/*
 * SCMI clock id -> PLM/EEMI clock id translation.
 *
 * The SCMI clock id is the array index (key) into scmi_clock_map[] and forms a
 * compact 0..scmi_clock_count-1 space. Each entry describes the PLM/EEMI clock
 * id to use when communicating with the PLM, plus a flags field.
 */
struct scmi_clock_map_entry {
	uint32_t plm_id;	/* PLM/EEMI clock id used in PLM communication */
	uint32_t flags;		/* SCMI_CLK_FLAG_* bitmask, 0 for a plain entry */
};

/*
 * The clock exists in the Versal Gen2 node-id namespace (xpm_nodeid.h) but is
 * not instantiated in this platform's PLM device topology.
 */
/* TODO: Detect the platform and enable the specific clock/reset/pd ids meant for ve-p-a1225 */
#define SCMI_CLK_FLAG_NOT_PRESENT	(1U << 0)

extern struct scmi_clock_map_entry scmi_clock_map[];

/* Number of clock domains in scmi_clock_map[] (derived from the table). */
extern const size_t scmi_clock_count;

/*
 * Look up the PLM/EEMI clock id and flags for a given SCMI clock id.
 *
 * @scmi_id: SCMI clock id (array index / key)
 * @plm_id:  optional, receives the PLM/EEMI clock id on success
 * @flags:   optional, receives the entry flags on success
 *
 * Returns 0 on success, or -1 if @scmi_id is out of range.
 */
static inline int32_t scmi_clock_map_get(unsigned int scmi_id,
					 uint32_t *plm_id, uint32_t *flags)
{
	if (scmi_id >= scmi_clock_count) {
		return -1;
	}

	if (plm_id != NULL) {
		*plm_id = scmi_clock_map[scmi_id].plm_id;
	}

	if (flags != NULL) {
		*flags = scmi_clock_map[scmi_id].flags;
	}

	return 0;
}

#endif /* SCMI_CLOCK_MAP_H */
