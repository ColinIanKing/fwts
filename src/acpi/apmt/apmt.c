/*
 * Copyright (C) 2025-2026 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

static fwts_acpi_table_info *table;
acpi_table_init(APMT, &table)

static int apmt_test1(fwts_framework *fw)
{
	fwts_acpi_apmt_node *entry;
	bool passed = true;
	uint32_t offset;

	offset = sizeof(fwts_acpi_table_apmt);
	entry = (fwts_acpi_apmt_node *)(table->data + offset);
	while (offset < table->length) {
		if (fwts_acpi_structure_length_zero(fw, "APMT", entry->length, offset)) {
			passed = false;
			break;
		}

		if ((offset + entry->length) > table->length) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"APMTOutOfRangeOffset",
			"APMT Node Offset is out of range.");
			passed = false;
			break;
		}

		fwts_log_info_verbatim(fw, "APMT node:");
		fwts_log_info_simp_int(fw, "  Length:                   ", entry->length);
		fwts_log_info_simp_int(fw, "  Node flags:               ", entry->flags);
		fwts_acpi_reserved_bits("APMT", "Node flags", entry->flags, 3, 7, &passed);
		fwts_log_info_simp_int(fw, "  Node type:                ", entry->type);
		fwts_log_info_simp_int(fw, "  Identifier:               ", entry->identifier);
		fwts_log_info_simp_int(fw, "  Node Instance primary:    ", entry->instance_prim);
		fwts_log_info_simp_int(fw, "  Node Instance secondary:  ", entry->instance_secondary);
		switch (entry->type) {
		case 0x00:
		case 0x01:
		case 0x02:
			if (entry->instance_secondary != 0) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"APMTInvalidNodeType",
					"APMT Node Type 0x%2.2" PRIx8 ", the Secondary Node "
					"instance field must be 0, but got 0x%8.8" PRIx32,
					entry->type, entry->instance_secondary);
			}
			break;
		case 0x03:
			/* nothing to check */
			break;
		case 0x04:
			if (entry->instance_prim != 0) {
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"APMTInvalidNodeType",
					"APMT Node Type 0x%2.2" PRIx8 ", the Primary Node "
					"instance field must be 0, but got 0x%" PRIx64,
					entry->type, entry->instance_prim);
			}
			break;
		default:
			/* reserved */
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"APMTNodeTypeUnknown",
				"APMT Node Type 0x%2.2" PRIx8 " is unknown.",
				entry->type);
			break;
		}
		fwts_log_info_simp_int(fw, "  Base address 0:           ", entry->base_addr_0);
		fwts_log_info_simp_int(fw, "  Base address 1:           ", entry->base_addr_1);
		fwts_log_info_simp_int(fw, "  Overflow interrupt:       ", entry->overflow_intr);
		fwts_log_info_simp_int(fw, "  Reserved1:                ", entry->reserved1);
		fwts_acpi_reserved_zero("APMT", "Reserved1", entry->reserved1, &passed);
		fwts_log_info_simp_int(fw, "  Overflow interrupt flags: ", entry->overflow_intr_flag);
		fwts_acpi_reserved_bits("APMT", "Overflow interrupt flags", entry->overflow_intr_flag, 1, 31, &passed);
		fwts_log_info_simp_int(fw, "  Processor affinity:       ", entry->processor_affinity);
		fwts_log_info_simp_int(fw, "  Implementation ID:        ", entry->implementation_id);

		offset += entry->length;
		entry = (fwts_acpi_apmt_node *)(table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in APMT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test apmt_tests[] = {
	{ apmt_test1, "Validate APMT table." },
	{ NULL, NULL }
};

static fwts_framework_ops apmt_ops = {
	.description = "APMT Arm Performance Monitoring Unit table test.",
	.init        = APMT_init,
	.minor_tests = apmt_tests
};

FWTS_REGISTER("apmt", &apmt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
