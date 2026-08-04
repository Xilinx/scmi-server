/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_RESET_MAP_H
#define SCMI_RESET_MAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * SCMI reset domain id -> PLM/EEMI reset id translation.
 *
 * The SCMI reset id is the array index (key) into scmi_reset_map[] and forms a
 * compact 0..scmi_reset_count-1 space. Each entry describes the PLM/EEMI reset
 * id to use when communicating with the PLM, plus a flags field.
 */
struct scmi_reset_map_entry {
	uint32_t plm_id;	/* PLM/EEMI reset id used in PLM communication */
	uint32_t flags;		/* SCMI_RST_FLAG_* bitmask, 0 for a plain entry */
};

/*
 * The reset line exists in the Versal Gen2 node-id namespace (xpm_nodeid.h)
 * but is not instantiated in this platform's PLM device topology.
 */
/* TODO: Detect the platform and enable the specific clock/reset/pd ids meant for ve-p-a1225 */
#define SCMI_RST_FLAG_NOT_PRESENT	(1U << 0)

/*
 * The map is not const: each entry carries a per-agent permission bitmap
 * (entry.perms) that is populated at agent-init time. A zero bitmap means no
 * agent may access the reset. Use the accessors below instead of touching
 * entry.perms directly.
 */
extern struct scmi_reset_map_entry scmi_reset_map[];

/* Number of reset domains in scmi_reset_map[] (derived from the table). */
extern const size_t scmi_reset_count;

/*
 * Look up the PLM/EEMI reset id and flags for a given SCMI reset id.
 *
 * @scmi_id: SCMI reset id (array index / key)
 * @plm_id:  optional, receives the PLM/EEMI reset id on success
 * @flags:   optional, receives the entry flags on success
 *
 * Returns 0 on success, or -1 if @scmi_id is out of range.
 */
static inline int32_t scmi_reset_map_get(unsigned int scmi_id,
					 uint32_t *plm_id, uint32_t *flags)
{
	if (scmi_id >= scmi_reset_count) {
		return -1;
	}

	if (plm_id != NULL) {
		*plm_id = scmi_reset_map[scmi_id].plm_id;
	}

	if (flags != NULL) {
		*flags = scmi_reset_map[scmi_id].flags;
	}

	return 0;
}

#endif /* SCMI_RESET_MAP_H */
