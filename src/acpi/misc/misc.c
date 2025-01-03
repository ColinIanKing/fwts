/*
 * Copyright (C) 2024-2025 Canonical
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
acpi_table_init(MISC, &table)

static int misc_test1(fwts_framework *fw)
{
	fwts_acpi_misc_guided_entry *entry;
	bool passed = true;
	uint32_t offset;

	offset = sizeof(fwts_acpi_table_misc);
	entry = (fwts_acpi_misc_guided_entry *)(table->data + offset);
	while (offset < table->length) {
		char guid_str[37];

		if (fwts_acpi_structure_length_zero(fw, "MISC", entry->entry_len, offset)) {
			passed = false;
			break;
		}

		if ((offset + entry->entry_len) > table->length) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"MISCOutOfRangeOffset",
			"MISC GUIDed Entry Offset is out of range.");
			passed = false;
			break;
		}

		fwts_log_info_verbatim(fw, "Miscellaneous GUIDed Table Entries:");
		fwts_guid_buf_to_str(entry->entry_guid, guid_str, sizeof(guid_str));
		fwts_log_info_verbatim(fw, "  Entry GUID ID:     %s", guid_str);
		fwts_log_info_simp_int(fw, "  Entry Length:      ", entry->entry_len);
		fwts_log_info_simp_int(fw, "  Revision:          ", entry->revision);
		fwts_log_info_simp_int(fw, "  Producer ID:       ", entry->producer_id);
		fwts_log_info_verbatim(fw, "  Data:");
		fwts_hexdump_data_prefix_all(fw, entry->data, "    ",
				(entry->entry_len - sizeof(fwts_acpi_misc_guided_entry)));

		/* Nothing else need to be checked currently */

		offset += entry->entry_len;
		entry = (fwts_acpi_misc_guided_entry *)(table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in MISC table.");

	return FWTS_OK;
}

static fwts_framework_minor_test misc_tests[] = {
	{ misc_test1, "Validate MISC table." },
	{ NULL, NULL }
};

static fwts_framework_ops misc_ops = {
	.description = "MISC Miscellaneous GUIDed Table Entries test.",
	.init        = MISC_init,
	.minor_tests = misc_tests
};

FWTS_REGISTER("misc", &misc_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
