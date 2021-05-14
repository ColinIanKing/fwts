/*
 * Copyright (C) 2021 Canonical
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
acpi_table_init(PHAT, &table)

static void print_record_header(fwts_framework *fw, const fwts_acpi_table_phat_header *header)
{
	fwts_log_info_simp_int(fw, "    Type:                           ", header->type);
	fwts_log_info_simp_int(fw, "    Record Length:                  ", header->length);
	fwts_log_info_simp_int(fw, "    Revision:                       ", header->revision);
}

static void phat_version_test(fwts_framework *fw, fwts_acpi_table_phat_version *entry, uint32_t offset, bool *passed)
{
	uint32_t reserved, i;

	offset += sizeof(fwts_acpi_table_phat_version);
	reserved = entry->reserved[0] + (entry->reserved[1] << 8) + (entry->reserved[2] << 16);

	fwts_log_info_verbatim(fw, "  Firmware Version Data Record (Type 0):");
	print_record_header(fw, &entry->header);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", reserved);
	fwts_log_info_simp_int(fw, "    Record Count:                   ", entry->element_count);

	for (i = 0 ; i < entry->element_count; i++) {
		char guid[37];

		/* stop if element is outside the table */
		offset += sizeof(fwts_acpi_table_phat_version_elem);
		if (fwts_acpi_structure_range(fw, "PHAT", table->length, offset)) {
			*passed = false;
			break;
		}

		fwts_acpi_table_phat_version_elem *element =
			(fwts_acpi_table_phat_version_elem *) ((char *)entry + sizeof(fwts_acpi_table_phat_version));

		fwts_guid_buf_to_str(element->component_id, guid, sizeof(guid));

		fwts_log_info_verbatim(fw, "    Component ID:                   %s", guid);
		fwts_log_info_simp_int(fw, "    Version:                        ", element->version);
		fwts_log_info_simp_int(fw, "    Producer ID:                    ", element->producer_id);

	}

	fwts_acpi_reserved_zero("PHAT", "Reserved", reserved, passed);
}

static void phat_health_test(fwts_framework *fw, fwts_acpi_table_phat_health *entry, uint32_t offset, bool *passed)
{
	char *device_path;
	char guid[37];

	fwts_guid_buf_to_str(entry->data_signature, guid, sizeof(guid));
	device_path = (char *)entry + sizeof(fwts_acpi_table_phat_health);

	fwts_log_info_verbatim(fw, " Firmware Health Data Record (Type 1):");
	print_record_header(fw, &entry->header);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved);
	fwts_log_info_simp_int(fw, "    AmHealthy:                      ", entry->healthy);
	fwts_log_info_verbatim(fw, "    Device Signature:               %s", guid);
	fwts_log_info_simp_int(fw, "    Device-specific Data Offset:    ", entry->data_offset);
	fwts_log_info_verbatim(fw, "    Device Path:                    %s", device_path);

	offset = offset + sizeof(fwts_acpi_table_phat_health) + strlen(device_path);

	if (entry->data_offset != 0) {
		uint16_t i;

		for (i = entry->data_offset; i < entry->header.length; i++) {
			uint8_t *vendor_data = (uint8_t *)entry + i;

			/* stop if data is outside the table */
			offset += sizeof(uint8_t);
			if (fwts_acpi_structure_range(fw, "PHAT", table->length, offset)) {
				*passed = false;
				break;
			}

			fwts_log_info_simp_int(fw, "    Vendor Data:                    ", *vendor_data);
		}
	}

	fwts_acpi_reserved_zero("PHAT", "Reserved", entry->reserved, passed);

	if (entry->data_offset > entry->header.length) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"PHATOutOfRangeOffset",
			"PHAT Type 1's Data Offset is out of range");
		*passed = false;
	}
}

static int phat_test1(fwts_framework *fw)
{
	fwts_acpi_table_phat_header *entry;
	bool passed = true;
	uint32_t offset;

	fwts_log_info_verbatim(fw, "PHAT Platform Health Assessment Table:");

	entry = (fwts_acpi_table_phat_header *) (table->data + sizeof(fwts_acpi_table_phat));
	offset = sizeof(fwts_acpi_table_phat);
	while (offset < table->length) {
		if (fwts_acpi_structure_length_zero(fw, "PHAT", entry->length, offset)) {
			passed = false;
			break;
		}

		if (entry->type == FWTS_PHAT_VERSION) {
			phat_version_test(fw, (fwts_acpi_table_phat_version *) entry, offset, &passed);
		} else if (entry->type == FWTS_PHAT_HEALTH) {
			phat_health_test(fw, (fwts_acpi_table_phat_health *) entry, offset, &passed);
		} else {
			fwts_acpi_reserved_type(fw, "PHAT", entry->type, 0, FWTS_PHAT_RESERVED - 1, &passed);
			break;
		}

		/* stop if sub structure is outside the table */
		offset += entry->length;
		if (fwts_acpi_structure_range(fw, "PHAT", table->length, offset)) {
			passed = false;
			break;
		}

		entry = (fwts_acpi_table_phat_header *) (table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in PHAT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test phat_tests[] = {
	{ phat_test1, "Validate PHAT table." },
	{ NULL, NULL }
};

static fwts_framework_ops phat_ops = {
	.description = "PHAT Platform Health Assessment Table test.",
	.init        = PHAT_init,
	.minor_tests = phat_tests
};

FWTS_REGISTER("phat", &phat_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
