/*
 * Copyright (C) 2015-2017 Canonical
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

static fwts_acpi_table_info *table;

static int facs_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "FACS", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI FACS table does not exist, skipping test");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 *  FACS Firmware ACPI Control Structure test
 */
static int facs_test1(fwts_framework *fw)
{
	fwts_acpi_table_facs *facs = (fwts_acpi_table_facs*)table->data;
	bool passed = true;
	int i;

	if (table->length < 64) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"FACSInvalidLength",
			"FACS: Length field is %" PRIu32
			" and should be at least 64",
			facs->length);
		goto done;
	}

	fwts_log_info_verbatim(fw, "FACS Firmware ACPI Control Structure:");
	fwts_log_info_verbatim(fw, "  Signature:                '%4.4s'", facs->signature);
	fwts_log_info_verbatim(fw, "  Length:                   0x%8.8" PRIx32, facs->length);
	fwts_log_info_verbatim(fw, "  Hardware Signature:       0x%8.8" PRIx32, facs->hardware_signature);
	fwts_log_info_verbatim(fw, "  Firmware Waking Vector:   0x%8.8" PRIx32, facs->firmware_waking_vector);
	fwts_log_info_verbatim(fw, "  Global Lock:              0x%8.8" PRIx32, facs->global_lock);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, facs->flags);
	fwts_log_info_verbatim(fw, "  X-Firmware Waking Vector: 0x%16.16" PRIx64, facs->x_firmware_waking_vector);
	fwts_log_info_verbatim(fw, "  Version:                  0x%2.2" PRIx8, facs->version);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8 " 0x%2.2" PRIx8 " 0x%2.2" PRIx8,
		facs->reserved[0], facs->reserved[1], facs->reserved[2]);
	fwts_log_info_verbatim(fw, "  OSPM Flags:               0x%8.8" PRIx32, facs->ospm_flags);
	for (i = 0; i < 24; i+= 4) {
		fwts_log_info_verbatim(fw, "  Reserved:                 "
			"0x%2.2" PRIx8 " 0x%2.2" PRIx8 " 0x%2.2" PRIx8 " 0x%2.2" PRIx8,
			facs->reserved2[i + 0], facs->reserved2[i + 1],
			facs->reserved2[i + 2], facs->reserved2[i + 3]);
	}

	/*
	 * If we've loaded the table from memory we can do some extra checks
	 */
	if (table->provenance == FWTS_ACPI_TABLE_FROM_FIRMWARE) {
		fwts_list *memory_map;

		if (table->addr & 0x3f) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FACSInvalidAlignment",
				"FACS: The FACS address is 0x%" PRIx64
				" which is not aligned on a 64 byte boundary",
				table->addr);
		}
		memory_map = fwts_memory_map_table_load(fw);
		if (fwts_memory_map_is_reserved(memory_map, table->addr) == FWTS_FALSE) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"FACSNotReserved",
				"FACS: The FACS does not seem to be in a reserved "
				"memory region, check that the BIOS memory map is "
				"correctly reserving the address space 0x%" PRIx64
				"-0x%" PRIx64,
				table->addr, table->addr + table->length);
		}
		fwts_memory_map_table_free(memory_map);
	}

	/*
	 * Check for incorrectly set facs length field, we know that
	 * the table that has been loaded is large enough because of
	 * the first check.  We don't need to bail if the following
	 * test fails.
	 */
	if (facs->length < 64) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"FACSInvalidLength",
			"FACS: Length field is %" PRIu32
			" and should be at least 64",
			facs->length);
	}
	if (facs->reserved[0] |
	    facs->reserved[1] |
	    facs->reserved[2]) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"FACSInvalidReserved1",
			"FACS: 1st Reserved field is non-zero");
	}

	fwts_acpi_reserved_bits_check(fw, "FACS", "Flags", facs->flags, sizeof(facs->flags), 2, 31, &passed);
	fwts_acpi_reserved_bits_check(fw, "FACS", "OSPM Flags", facs->ospm_flags, sizeof(facs->ospm_flags), 1, 31, &passed);

	for (i = 0; i < 24; i++) {
		if (facs->reserved2[i]) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_LOW,
				"FACSInvalidReserved1",
				"FACS: 2nd Reserved field is non-zero");
			break;
		}
	}
done:
	if (passed)
		fwts_passed(fw, "No issues found in FACS table.");

	return FWTS_OK;
}

static fwts_framework_minor_test facs_tests[] = {
	{ facs_test1, "FACS Firmware ACPI Control Structure test." },
	{ NULL, NULL }
};

static fwts_framework_ops facs_ops = {
	.description = "FACS Firmware ACPI Control Structure test.",
	.init        = facs_init,
	.minor_tests = facs_tests
};

FWTS_REGISTER("facs", &facs_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
