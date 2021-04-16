/*
 * Copyright (C) 2016-2021 Canonical
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

static void pmtt_memory_device(fwts_framework *fw, fwts_acpi_table_pmtt_header *entry, uint32_t offset, bool *passed);

static fwts_acpi_table_info *table;
acpi_table_init(PMTT, &table)

static void pmtt_subtable_header_test(
	fwts_framework *fw,
	fwts_acpi_table_pmtt_header *entry,
	bool *passed)
{
	fwts_log_info_verbatim(fw, "  PMTT Subtable:");
	fwts_log_info_simp_int(fw, "    Type:                           ", entry->type);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved1);
	fwts_log_info_simp_int(fw, "    Length:                         ", entry->length);
	fwts_log_info_simp_int(fw, "    Flags:                          ", entry->flags);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved2);

	fwts_acpi_reserved_zero_check("PMTT", "Reserved1", entry->reserved1, passed);
	fwts_acpi_reserved_bits_check("PMTT", "Flags", entry->flags, 4, 15, passed);

	if ((entry->flags & 0x0C) == 0x0C) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PMTTBadFlags",
			"PMTT Flags's Bits[3..2] must not be 11b");
	}

	fwts_acpi_reserved_zero_check("PMTT", "Reserved2", entry->reserved2, passed);
}

static void pmtt_physical_component_test(
	fwts_framework *fw,
	fwts_acpi_table_pmtt_physical_component *entry,
	bool *passed)
{
	pmtt_subtable_header_test(fw, &entry->header, passed);
	fwts_log_info_simp_int(fw, "    SMBIOS Handle:                  ", entry->bios_handle);

	if ((entry->bios_handle & 0xFFFF0000) != 0 && entry->bios_handle != 0xFFFFFFFF) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PMTTBADSMBIOSHandle",
			"PMTT upper 2 bytes must be zero, got "
			"0x%8.8" PRIx32 " instead", entry->bios_handle);
	}
}

static void pmtt_controller_test(
	fwts_framework *fw,
	fwts_acpi_table_pmtt_controller *entry,
	uint32_t entry_offset,
	bool *passed)
{
	fwts_acpi_table_pmtt_header *header;
	uint32_t offset = 0;

	pmtt_subtable_header_test(fw, &entry->header, passed);
	fwts_log_info_simp_int(fw, "    Memory Controller ID            ", entry->memory_controller_id);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved);

	fwts_acpi_reserved_zero_check("PMTT", "Reserved", entry->reserved, passed);

	offset = sizeof(fwts_acpi_table_pmtt_controller);
	header = (fwts_acpi_table_pmtt_header *) (((char *) entry) + offset);
	while (offset < entry->header.length) {
		/* stop if sub-structure is outside the table */
		if (fwts_acpi_structure_range_check(fw, "PMTT", table->length, entry_offset + offset)) {
			*passed = false;
			break;
		}

		if (header->length == 0) {
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"PMTTBadSubtableLength",
				"PMTT Controller has a subtable with zero length");
			break;
		}

		pmtt_memory_device(fw, header, entry_offset + offset, passed);

		offset += header->length;
		header = (fwts_acpi_table_pmtt_header *)(((char *) entry) + offset);
	}
}

static void pmtt_socket_test(
	fwts_framework *fw,
	fwts_acpi_table_pmtt_socket *entry,
	uint32_t entry_offset,
	bool *passed)
{
	fwts_acpi_table_pmtt_header *header;
	uint32_t offset;

	pmtt_subtable_header_test(fw, &entry->header, passed);
	fwts_log_info_simp_int(fw, "    Socket Identifier:              ", entry->socket_id);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved);

	fwts_acpi_reserved_zero_check("PMTT", "Reserved", entry->reserved, passed);

	offset = sizeof(fwts_acpi_table_pmtt_socket);
	header = (fwts_acpi_table_pmtt_header *) (((char *) entry) + offset);
	while (offset < entry->header.length) {
		/* stop if sub-structure is outside the table */
		if (fwts_acpi_structure_range_check(fw, "PMTT", table->length, entry_offset + offset)) {
			*passed = false;
			break;
		}

		if (header->length == 0) {
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"PMTTBadSubtableLength",
				"PMTT Socket has a subtable with zero length");
			break;
		}

		pmtt_memory_device(fw, header, entry_offset + offset, passed);

		offset += header->length;
		header = (fwts_acpi_table_pmtt_header *)(((char *) entry) + offset);
	}
}

static void pmtt_memory_device(
	fwts_framework *fw,
	fwts_acpi_table_pmtt_header *entry,
	uint32_t offset,
	bool *passed)
{
	switch(entry->type) {
		case FWTS_ACPI_PMTT_TYPE_SOCKET:
			pmtt_socket_test(fw, (fwts_acpi_table_pmtt_socket *) entry, offset, passed);
			break;
		case FWTS_ACPI_PMTT_TYPE_CONTROLLER:
			pmtt_controller_test(fw, (fwts_acpi_table_pmtt_controller *) entry, offset, passed);
			break;
		case FWTS_ACPI_PMTT_TYPE_DIMM:
			pmtt_physical_component_test(fw, (fwts_acpi_table_pmtt_physical_component *) entry, passed);
			break;
		case FWTS_ACPI_PMTT_TYPE_VENDOR_SPECIFIC:
			/* no tests for vendor-specific type */
			break;
		default:
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"PMTTBadSubtableType",
				"PMTT must have subtable with Type 1..2 or 0xFF, got "
				"0x%4.4" PRIx16 " instead", entry->type);
			break;
	}
}

static int pmtt_test1(fwts_framework *fw)
{
	fwts_acpi_table_pmtt *pmtt = (fwts_acpi_table_pmtt*) table->data;
	fwts_acpi_table_pmtt_header *entry;
	uint32_t offset;
	bool passed = true;

	if (pmtt->header.revision < 2) {
		fwts_failed(fw, LOG_LEVEL_CRITICAL, "PMTTDeprecatedRevision",
			"PMTT Revision 1 has been deprecated in ACPI 6.4");
		return FWTS_OK;
	}

	fwts_log_info_verbatim(fw, "PMTT Table:");
	fwts_log_info_simp_int(fw, "  Number of Memory Devices:         ", pmtt->num_devices);

	entry = (fwts_acpi_table_pmtt_header *) (table->data + sizeof(fwts_acpi_table_pmtt));
	offset = sizeof(fwts_acpi_table_pmtt);
	while (offset < table->length) {

		if (entry->length == 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH, "PMTTLengthZero",
				    "PMTT Subtable (offset 0x%4.4" PRIx32 ") "
				    "length cannot be 0", offset);
			break;
		}

		pmtt_memory_device(fw, entry, offset, &passed);

		offset += entry->length;
		entry = (fwts_acpi_table_pmtt_header *) (table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in PMTT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test pmtt_tests[] = {
	{ pmtt_test1, "Validate PMTT table." },
	{ NULL, NULL }
};

static fwts_framework_ops pmtt_ops = {
	.description = "PMTT Memory Topology Table test.",
	.init        = PMTT_init,
	.minor_tests = pmtt_tests
};

FWTS_REGISTER("pmtt", &pmtt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
