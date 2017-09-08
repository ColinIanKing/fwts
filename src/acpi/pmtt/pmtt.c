/*
 * Copyright (C) 2016-2017 Canonical
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

static int pmtt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "PMTT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI PMTT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static void pmtt_subtable_header_test(fwts_framework *fw, fwts_acpi_table_pmtt_header *entry, bool *passed)
{
	fwts_log_info_verbatim(fw, "  PMTT Subtable:");
	fwts_log_info_verbatim(fw, "    Type:                           0x%2.2" PRIx8, entry->type);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%2.2" PRIx8, entry->reserved1);
	fwts_log_info_verbatim(fw, "    Length:                         0x%4.4" PRIx16, entry->length);
	fwts_log_info_verbatim(fw, "    Flags:                          0x%4.4" PRIx16, entry->flags);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved2);

	fwts_acpi_reserved_zero_check(fw, "PMTT", "Reserved1", entry->reserved1, sizeof(entry->reserved1), passed);

	if (entry->flags & ~0x0F) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PMTTBadFlags",
			"PMTT Flags's Bits[15..4] must be zero, got "
			"0x%4.4" PRIx16 " instead", entry->flags);
	}

	if ((entry->flags & 0x0C) == 0x0C) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PMTTBadFlags",
			"PMTT Flags's Bits[3..2] must not be 11b");
	}

	fwts_acpi_reserved_zero_check(fw, "PMTT", "Reserved2", entry->reserved2, sizeof(entry->reserved2), passed);
}

static void pmtt_physical_component_test(fwts_framework *fw, fwts_acpi_table_pmtt_physical_component *entry, bool *passed)
{
	pmtt_subtable_header_test(fw, &entry->header, passed);
	fwts_log_info_verbatim(fw, "    Physical Component Identifier:  0x%4.4" PRIx16, entry->component_id);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved);
	fwts_log_info_verbatim(fw, "    Size of DIMM:                   0x%8.8" PRIx32, entry->memory_size);
	fwts_log_info_verbatim(fw, "    SMBIOS Handle:                  0x%8.8" PRIx32, entry->bios_handle);

	fwts_acpi_reserved_zero_check(fw, "PMTT", "Reserved", entry->reserved, sizeof(entry->reserved), passed);

	if ((entry->bios_handle & 0xFFFF0000) != 0 && entry->bios_handle != 0xFFFFFFFF) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PMTTBADSMBIOSHandle",
			"PMTT upper 2 bytes must be zero, got "
			"0x%8.8" PRIx32 " instead", entry->bios_handle);
	}
}

static void pmtt_controller_test(fwts_framework *fw, fwts_acpi_table_pmtt_controller *entry, bool *passed)
{
	fwts_acpi_table_pmtt_header *header;
	uint32_t offset = 0;
	size_t i;

	pmtt_subtable_header_test(fw, &entry->header, passed);
	fwts_log_info_verbatim(fw, "    Read Latency:                   0x%8.8" PRIx32, entry->read_latency);
	fwts_log_info_verbatim(fw, "    Write latency:                  0x%8.8" PRIx32, entry->write_latency);
	fwts_log_info_verbatim(fw, "    Read Bandwidth:                 0x%8.8" PRIx32, entry->read_bandwidth);
	fwts_log_info_verbatim(fw, "    Write Bandwidth:                0x%8.8" PRIx32, entry->write_bandwidth);
	fwts_log_info_verbatim(fw, "    Optimal Access Unit:            0x%4.4" PRIx16, entry->access_width);
	fwts_log_info_verbatim(fw, "    Optimal Access Alignment:       0x%4.4" PRIx16, entry->alignment);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved);
	fwts_log_info_verbatim(fw, "    Number of Proximity Domains:    0x%4.4" PRIx16, entry->domain_count);

	fwts_acpi_reserved_zero_check(fw, "PMTT", "Reserved", entry->reserved, sizeof(entry->reserved), passed);

	offset = sizeof(fwts_acpi_table_pmtt_controller);
	if (entry->header.length < offset + sizeof(fwts_acpi_table_pmtt_domain) * entry->domain_count) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PMTTOutOfBound",
			"PMTT's length is too small to contain all fields");
		return;
	}

	fwts_acpi_table_pmtt_domain *domain = (fwts_acpi_table_pmtt_domain *)(((char *) entry) + offset);
	for (i = 0; i < entry->domain_count; i++) {
		fwts_log_info_verbatim(fw, "    Proximity Domain:               0x%8.8" PRIx32, domain->proximity_domain);
		domain++;
		/* TODO cross check proximity domain with SRAT table*/
	}

	offset += sizeof(fwts_acpi_table_pmtt_domain) * entry->domain_count;
	header = (fwts_acpi_table_pmtt_header *) (((char *) entry) + offset);
	while (offset < entry->header.length) {
		if (header->type == FWTS_ACPI_PMTT_TYPE_DIMM) {
			pmtt_physical_component_test(fw, (fwts_acpi_table_pmtt_physical_component *) header, passed);
		} else {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"PMTTBadSubtableType",
				"PMTT Controller must have subtable with Type 2, got "
				"0x%4.4" PRIx16 " instead", header->type);
		}

		offset += header->length;
		header = (fwts_acpi_table_pmtt_header *)(((char *) entry) + offset);
	}
}

static void pmtt_socket_test(fwts_framework *fw, fwts_acpi_table_pmtt_socket *entry, bool *passed)
{
	fwts_acpi_table_pmtt_header *header;
	uint32_t offset;

	pmtt_subtable_header_test(fw, &entry->header, passed);
	fwts_log_info_verbatim(fw, "    Socket Identifier:              0x%4.4" PRIx16, entry->socket_id);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved);

	fwts_acpi_reserved_zero_check(fw, "PMTT", "Reserved", entry->reserved, sizeof(entry->reserved), passed);

	offset = sizeof(fwts_acpi_table_pmtt_socket);
	header = (fwts_acpi_table_pmtt_header *) (((char *) entry) + offset);
	while (offset < entry->header.length) {
		if (header->type == FWTS_ACPI_PMTT_TYPE_CONTROLLER) {
			pmtt_controller_test(fw, (fwts_acpi_table_pmtt_controller *) header, passed);
		} else {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"PMTTBadSubtableType",
				"PMTT Socket must have subtable with Type 1, got "
				"0x%4.4" PRIx16 " instead", header->type);
		}

		offset += header->length;
		header = (fwts_acpi_table_pmtt_header *)(((char *) entry) + offset);
	}
}

static int pmtt_test1(fwts_framework *fw)
{
	fwts_acpi_table_pmtt *pmtt = (fwts_acpi_table_pmtt*) table->data;
	fwts_acpi_table_pmtt_header *entry;
	uint32_t offset;
	bool passed = true;

	fwts_log_info_verbatim(fw, "PMTT Table:");
	fwts_log_info_verbatim(fw, "  Reserved:                         0x%8.8" PRIx32, pmtt->reserved);

	fwts_acpi_reserved_zero_check(fw, "PMTT", "Reserved", pmtt->reserved, sizeof(pmtt->reserved), &passed);

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

		switch(entry->type) {
			case FWTS_ACPI_PMTT_TYPE_SOCKET:
				pmtt_socket_test(fw, (fwts_acpi_table_pmtt_socket *) entry, &passed);
				break;
			case FWTS_ACPI_PMTT_TYPE_CONTROLLER:
				pmtt_controller_test(fw, (fwts_acpi_table_pmtt_controller *) entry, &passed);
				break;
			case FWTS_ACPI_PMTT_TYPE_DIMM:
				pmtt_physical_component_test(fw, (fwts_acpi_table_pmtt_physical_component *) entry, &passed);
				break;
			default:
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"PMTTBadSubtableType",
					"PMTT must have subtable with Type 1..2, got "
					"0x%4.4" PRIx16 " instead", entry->type);
				break;
		}

		offset += entry->length;
		entry = (fwts_acpi_table_pmtt_header *) (table->data + offset);
	}
	fwts_log_nl(fw);

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
	.init        = pmtt_init,
	.minor_tests = pmtt_tests
};

FWTS_REGISTER("pmtt", &pmtt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
