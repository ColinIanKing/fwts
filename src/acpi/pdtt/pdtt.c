/*
 * Copyright (C) 2017 Canonical
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
#include <stdbool.h>

static fwts_acpi_table_info *table;

static int pdtt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "PDTT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI PDTT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static int pdtt_test1(fwts_framework *fw)
{
	fwts_acpi_table_pdtt *pdtt = (fwts_acpi_table_pdtt*) table->data;
	fwts_acpi_table_pdtt_channel *entry;
	uint32_t offset, count, i;
	uint32_t reserved;
	bool passed = true;

	reserved = (uint32_t) pdtt->reserved[0] + ((uint32_t) pdtt->reserved[1] << 8) +
		   ((uint32_t) pdtt->reserved[2] << 16);

	fwts_log_info_verbatim(fw, "PDTT Platform Debug Trigger Table:");
	fwts_log_info_verbatim(fw, "  Trigger Count:           0x%2.2" PRIx8, pdtt->trigger_count);
	fwts_log_info_verbatim(fw, "  Reserved[3]:             0x%6.6" PRIx32, reserved);
	fwts_log_info_verbatim(fw, "  Trigger ID Array Offset: 0x%2.2" PRIx8, pdtt->array_offset);

	fwts_acpi_reserved_zero_check(fw, "PDTT", "Reserved", reserved, sizeof(reserved), &passed);

	offset = pdtt->array_offset;
	entry = (fwts_acpi_table_pdtt_channel *) (table->data + offset);

	count = (pdtt->header.length - pdtt->array_offset) / sizeof(fwts_acpi_table_pdtt_channel);
	if (count != pdtt->trigger_count) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"PDTTBadIDCount",
			"PDTT should have %" PRId8 " ids, got %" PRId8,
			pdtt->trigger_count, count);
		return FWTS_OK;
	}

	fwts_log_info_verbatim(fw, "  Platform Communication Channel IDs");

	for (i = 0; i < pdtt->trigger_count; i++) {
		fwts_log_info_verbatim(fw, "    Sub channel ID:          0x%2.2" PRIx8, entry->sub_channel_id);
		fwts_log_info_verbatim(fw, "    Flags:                   0x%2.2" PRIx8, entry->flags);
		fwts_acpi_reserved_bits_check(fw, "PDTT", "Flags", entry->flags, sizeof(entry->flags), 2, 7, &passed);

		if ((offset += sizeof(fwts_acpi_table_pdtt_channel)) > table->length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"PDTTBadTableLength",
				"PDTT has more channel IDs than its size can handle");
			break;
		}

		entry = (fwts_acpi_table_pdtt_channel *) (table->data + offset);
	}

	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in PDTT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test pdtt_tests[] = {
	{ pdtt_test1, "Validate PDTT table." },
	{ NULL, NULL }
};

static fwts_framework_ops pdtt_ops = {
	.description = "PDTT Platform Debug Trigger Table test.",
	.init        = pdtt_init,
	.minor_tests = pdtt_tests
};

FWTS_REGISTER("pdtt", &pdtt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
