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

static int pptt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "PPTT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI PPTT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

static void pptt_processor_test(fwts_framework *fw, const fwts_acpi_table_pptt_processor *entry, bool *passed)
{
	uint32_t i;

	fwts_log_info_verbatim(fw, "  Processor hierarchy node structure (Type 0):");
	fwts_log_info_verbatim(fw, "    Type:                           0x%2.2" PRIx8, entry->header.type);
	fwts_log_info_verbatim(fw, "    Length:                         0x%2.2" PRIx8, entry->header.length);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved);
	fwts_log_info_verbatim(fw, "    Flags:                          0x%8.8" PRIx32, entry->flags);
	fwts_log_info_verbatim(fw, "    Parent:                         0x%8.8" PRIx32, entry->parent);
	fwts_log_info_verbatim(fw, "    ACPI Processor ID:              0x%8.8" PRIx32, entry->acpi_processor_id);
	fwts_log_info_verbatim(fw, "    Number of Private Resources:    0x%8.8" PRIx32, entry->number_priv_resources);

	if ((entry->header.length - sizeof(fwts_acpi_table_pptt_processor)) / 4 == entry->number_priv_resources) {
		for (i = 0; i < entry->number_priv_resources; i++)
			fwts_log_info_verbatim(fw, "    Private Resources[%d]:           0x%8.8" PRIx32, i, entry->private_resource[i]);
	} else {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"PPTTBadProcessorLength",
			"PPTT Processor length does not match "
			"0x%8.8" PRIx32 " private resources",
			entry->number_priv_resources);
	}

	fwts_acpi_reserved_zero_check(fw, "PPTT", "Reserved", entry->reserved, sizeof(entry->reserved), passed);
	fwts_acpi_reserved_bits_check(fw, "PPTT", "Flags", entry->flags, sizeof(entry->flags), 2, 31, passed);
}

static void pptt_cache_test(fwts_framework *fw, const fwts_acpi_table_pptt_cache *entry, bool *passed)
{

	fwts_log_info_verbatim(fw, "  Cache Type Structure (Type 1):");
	fwts_log_info_verbatim(fw, "    Type:                           0x%2.2" PRIx8, entry->header.type);
	fwts_log_info_verbatim(fw, "    Length:                         0x%2.2" PRIx8, entry->header.length);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved);
	fwts_log_info_verbatim(fw, "    Flags:                          0x%8.8" PRIx32, entry->flags);
	fwts_log_info_verbatim(fw, "    Next Level of Cache:            0x%8.8" PRIx32, entry->next_level_cache);
	fwts_log_info_verbatim(fw, "    Size:                           0x%8.8" PRIx32, entry->size);
	fwts_log_info_verbatim(fw, "    Number of sets:                 0x%8.8" PRIx32, entry->number_sets);
	fwts_log_info_verbatim(fw, "    Associativity:                  0x%2.2" PRIx8, entry->associativity);
	fwts_log_info_verbatim(fw, "    Attributes:                     0x%2.2" PRIx8, entry->attributes);
	fwts_log_info_verbatim(fw, "    Line size:                      0x%4.4" PRIx16, entry->line_size);

	fwts_acpi_reserved_zero_check(fw, "PPTT", "Reserved", entry->reserved, sizeof(entry->reserved), passed);
	fwts_acpi_reserved_bits_check(fw, "PPTT", "Flags", entry->flags, sizeof(entry->flags), 7, 31, passed);

	if (entry->attributes & ~0x1f) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PPTTBadAttributes",
			"PPTT attributes's Bits[7..5] must be zero, got "
			"0x%2.2" PRIx8 " instead", entry->attributes);
	}
}

static void pptt_id_test(fwts_framework *fw, const fwts_acpi_table_pptt_id *entry, bool *passed)
{
	char vendor_id[5];

	memcpy(vendor_id, &entry->vendor_id, sizeof(uint32_t));
	vendor_id[4] = 0;

	fwts_log_info_verbatim(fw, "  ID structure (Type 2):");
	fwts_log_info_verbatim(fw, "    Type:                           0x%2.2" PRIx8, entry->header.type);
	fwts_log_info_verbatim(fw, "    Length:                         0x%2.2" PRIx8, entry->header.length);
	fwts_log_info_verbatim(fw, "    Reserved:                       0x%4.4" PRIx16, entry->reserved);
	fwts_log_info_verbatim(fw, "    VENDOR_ID:                      %4.4s", vendor_id);
	fwts_log_info_verbatim(fw, "    LEVEL_1_ID:                     0x%16.16" PRIx64, entry->level1_id);
	fwts_log_info_verbatim(fw, "    LEVEL_2_ID:                     0x%16.16" PRIx64, entry->level2_id);
	fwts_log_info_verbatim(fw, "    MAJOR_REV:                      0x%4.4" PRIx16, entry->major_rev);
	fwts_log_info_verbatim(fw, "    MINOR_REV:                      0x%4.4" PRIx16, entry->minor_rev);
	fwts_log_info_verbatim(fw, "    SPIN_REV:                       0x%4.4" PRIx16, entry->spin_rev);

	fwts_acpi_reserved_zero_check(fw, "PPTT", "Reserved", entry->reserved, sizeof(entry->reserved), passed);
}

static int pptt_test1(fwts_framework *fw)
{
	fwts_acpi_table_pptt_header *entry;
	uint32_t offset;
	bool passed = true;

	fwts_log_info_verbatim(fw, "PPTT Processor Properties Topology Table:");

	entry = (fwts_acpi_table_pptt_header *) (table->data + sizeof(fwts_acpi_table_pptt));
	offset = sizeof(fwts_acpi_table_pptt);
	while (offset < table->length) {
		uint32_t type_length;

		if (entry->length == 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH, "PPTTStructLengthZero",
				    "PPTT structure (offset 0x%4.4" PRIx32 ") "
				    "length cannot be 0", offset);
			break;
		}

		if (entry->type == FWTS_ACPI_PPTT_PROCESSOR) {
			pptt_processor_test(fw, (fwts_acpi_table_pptt_processor *) entry, &passed);
			type_length = sizeof(fwts_acpi_table_pptt_processor) +
				      ((fwts_acpi_table_pptt_processor *) entry)->number_priv_resources * 4;
		} else if (entry->type == FWTS_ACPI_PPTT_CACHE) {
			pptt_cache_test(fw, (fwts_acpi_table_pptt_cache *) entry, &passed);
			type_length = sizeof(fwts_acpi_table_pptt_cache);
		} else if (entry->type == FWTS_ACPI_PPTT_ID) {
			pptt_id_test(fw, (fwts_acpi_table_pptt_id *) entry, &passed);
			type_length = sizeof(fwts_acpi_table_pptt_id);
		} else {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"PPTTBadSubtableType",
				"PPTT must have subtable with Type 0..2, got "
				"0x%2.2" PRIx8 " instead", entry->type);
			break;
		}

		if (entry->length != type_length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"PPTTBadSubtableLength",
				"PPTT subtable Type 0x%2.2" PRIx8 " should have "
				"length 0x%2.2" PRIx8 ", got 0x%2.2" PRIx8,
				entry->type, entry->length, type_length);
			break;
		}

		if ((offset += entry->length) > table->length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"PPTTBadTableLength",
				"PPTT has more subtypes than its size can handle");
			break;
		}
		entry = (fwts_acpi_table_pptt_header *) (table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in PPTT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test pptt_tests[] = {
	{ pptt_test1, "Validate PPTT table." },
	{ NULL, NULL }
};

static fwts_framework_ops pptt_ops = {
	.description = "PPTT Processor Properties Topology Table test.",
	.init        = pptt_init,
	.minor_tests = pptt_tests
};

FWTS_REGISTER("pptt", &pptt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
