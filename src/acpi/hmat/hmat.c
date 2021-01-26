/*
 * Copyright (C) 2017-2021 Canonical
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
acpi_table_init(HMAT, &table)

static void hmat_proximity_domain_test(fwts_framework *fw, const fwts_acpi_table_hmat_proximity_domain *entry, bool *passed)
{
	fwts_log_info_verbatim(fw, "  Memory Proximity Domain Attributes (Type 0):");
	fwts_log_info_simp_int(fw, "    Type:                           ", entry->header.type);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->header.reserved);
	fwts_log_info_simp_int(fw, "    Length:                         ", entry->header.length);
	fwts_log_info_simp_int(fw, "    Flags:                          ", entry->flags);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved1);
	fwts_log_info_simp_int(fw, "    Proximity Domain for Initiator: ", entry->initiator_proximity_domain);
	fwts_log_info_simp_int(fw, "    Proximity Domain for Memory:    ", entry->memory_proximity_domain);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved2);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved3);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved4);

	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->header.reserved, sizeof(entry->header.reserved), passed);
	fwts_acpi_reserved_bits_check(fw, "HMAT", "Flags", entry->flags, sizeof(entry->flags), 1, 15, passed);
	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved1, sizeof(entry->reserved1), passed);
	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved2, sizeof(entry->reserved2), passed);
	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved3, sizeof(entry->reserved3), passed);
	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved4, sizeof(entry->reserved4), passed);
}

static void hmat_locality_test(fwts_framework *fw, const fwts_acpi_table_hmat_locality *entry, bool *passed)
{
	uint32_t pd_size;
	uint16_t reserved1 = (entry->reserved1 << 8) + entry->min_transfer_size;

	fwts_log_info_verbatim(fw, "  System Locality Latency and Bandwidth Information (Type 1):");
	fwts_log_info_simp_int(fw, "    Type:                           ", entry->header.type);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->header.reserved);
	fwts_log_info_simp_int(fw, "    Length:                         ", entry->header.length);
	fwts_log_info_simp_int(fw, "    Flags:                          ", entry->flags);
	fwts_log_info_simp_int(fw, "    Data Type:                      ", entry->data_type);
	if (fwts_get_acpi_version(fw) >= FWTS_ACPI_VERSION_64) {
		fwts_log_info_simp_int(fw, "    MinTransferSize:                ", entry->min_transfer_size);
		fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved1);
	} else
		fwts_log_info_simp_int(fw, "    Reserved:                       ", reserved1);
	fwts_log_info_simp_int(fw, "    Number of Initiator PDs:        ", entry->num_initiator);
	fwts_log_info_simp_int(fw, "    Number of Target PDs:           ", entry->num_target);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved2);
	fwts_log_info_simp_int(fw, "    Entry Base Unit:                ", entry->entry_base_unit);

	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->header.reserved, sizeof(entry->header.reserved), passed);
	fwts_acpi_reserved_bits_check(fw, "HMAT", "Flags", entry->flags, sizeof(entry->flags), 6, 7, passed);

	if (entry->data_type > 5) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"HMATBadFDataType",
			"HMAT Data Type must be 0..5, got "
			"0x%2.2" PRIx8 " instead", entry->data_type);
	}

	if (fwts_get_acpi_version(fw) >= FWTS_ACPI_VERSION_64)
		fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved1, sizeof(entry->reserved1), passed);
	else
		fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", reserved1, sizeof(reserved1), passed);
	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved2, sizeof(entry->reserved2), passed);

	pd_size = (entry->num_initiator + entry->num_target) * 4 +
	          (entry->num_initiator * entry->num_target * 2);
	if ((entry->header.length - sizeof(fwts_acpi_table_hmat_locality)) != pd_size) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"HMATBadNumProximityDomain",
			"HMAT length does not match to the number of Proximity Domains ");
	}

	if (!entry->entry_base_unit) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"HMATBadBaseUnit",
			"HMAT Type 1 Entry Base Unit must be non-zero");
	}
}

static void hmat_cache_test(fwts_framework *fw, const fwts_acpi_table_hmat_cache *entry, bool *passed)
{
	fwts_log_info_verbatim(fw, "  Memory Side Cache Information (Type 2):");
	fwts_log_info_simp_int(fw, "    Type:                           ", entry->header.type);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->header.reserved);
	fwts_log_info_simp_int(fw, "    Length:                         ", entry->header.length);
	fwts_log_info_simp_int(fw, "    Proximity Domain for Memory:    ", entry->memory_proximity_domain);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved1);
	fwts_log_info_simp_int(fw, "    Memory Side Cache Size:         ", entry->cache_size);
	fwts_log_info_simp_int(fw, "    Cache Attributes:               ", entry->cache_attr);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved2);
	fwts_log_info_simp_int(fw, "    Number of SMBIOS Handles:       ", entry->num_smbios);

	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->header.reserved, sizeof(entry->header.reserved), passed);

	if ((entry->cache_attr & 0xf) > 3 ||
	   ((entry->cache_attr >> 4) & 0xf) > 3 ||
	   ((entry->cache_attr >> 8) & 0xf) > 2 ||
	   ((entry->cache_attr >> 12) & 0xf) > 2) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"HMATBadCacheAttributeReserved",
			"HMAT Cache Attribute reserved values are used, got "
			"0x%8.8" PRIx32 " instead", entry->cache_attr);
	}

	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved1, sizeof(entry->reserved1), passed);
	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", entry->reserved2, sizeof(entry->reserved2), passed);

	if ((entry->header.length - sizeof(fwts_acpi_table_hmat_cache)) / 2 != entry->num_smbios) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"HMATBadNumberOfSMBIOS",
			"HMAT length does not match to "
			"the number of SMBIOS handles");
	}
}

static int hmat_test1(fwts_framework *fw)
{
	fwts_acpi_table_hmat *hmat = (fwts_acpi_table_hmat*) table->data;
	fwts_acpi_table_hmat_header *entry;
	bool passed = true;
	uint32_t offset;

	fwts_log_info_verbatim(fw, "HMAT Heterogeneous Memory Attribute Table:");
	fwts_log_info_simp_int(fw, "  Reserved:        ", hmat->reserved);


	fwts_acpi_reserved_zero_check(fw, "HMAT", "Reserved", hmat->reserved, sizeof(hmat->reserved), &passed);

	entry = (fwts_acpi_table_hmat_header *) (table->data + sizeof(fwts_acpi_table_hmat));
	offset = sizeof(fwts_acpi_table_hmat);
	while (offset < table->length) {
		uint32_t type_length;

		if (entry->length == 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH, "HMATStructLengthZero",
				    "HMAT structure (offset 0x%4.4" PRIx32 ") "
				    "length cannot be 0", offset);
			break;
		}

		if (entry->type == FWTS_ACPI_HMAT_TYPE_PROXIMITY_DOMAIN) {
			hmat_proximity_domain_test(fw, (fwts_acpi_table_hmat_proximity_domain *) entry, &passed);
			type_length = sizeof(fwts_acpi_table_hmat_proximity_domain);
		} else if (entry->type == FWTS_ACPI_HMAT_TYPE_LOCALITY) {
			fwts_acpi_table_hmat_locality *locality = (fwts_acpi_table_hmat_locality *) entry;
			hmat_locality_test(fw, (fwts_acpi_table_hmat_locality *) entry, &passed);
			type_length = sizeof(fwts_acpi_table_hmat_locality) +
			              (locality->num_initiator + locality->num_target) * 4 +
			              (locality->num_initiator * locality->num_target * 2);
		} else if (entry->type == FWTS_ACPI_HMAT_TYPE_CACHE) {
			hmat_cache_test(fw, (fwts_acpi_table_hmat_cache *) entry, &passed);
			type_length = sizeof(fwts_acpi_table_hmat_cache) +
			              ((fwts_acpi_table_hmat_cache *) entry)->num_smbios * 2;
		} else {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"HMATBadSubtableType",
				"HMAT must have subtable with Type 0..2, got "
				"0x%2.2" PRIx8 " instead", entry->type);
			break;
		}

		if (entry->length != type_length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"HMATBadSubtableLength",
				"HMAT subtable Type 0x%2.2" PRIx8 " should have "
				"length 0x%2.2" PRIx8 ", got 0x%2.2" PRIx8,
				entry->type, entry->length, type_length);
			break;
		}

		if ((offset += entry->length) > table->length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"HMATBadTableLength",
				"HMAT has more subtypes than its size can handle");
			break;
		}
		entry = (fwts_acpi_table_hmat_header *) (table->data + offset);
		fwts_log_nl(fw);
	}


	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in HMAT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test hmat_tests[] = {
	{ hmat_test1, "Validate HMAT table." },
	{ NULL, NULL }
};

static fwts_framework_ops hmat_ops = {
	.description = "HMAT Heterogeneous Memory Attribute Table test.",
	.init        = HMAT_init,
	.minor_tests = hmat_tests
};

FWTS_REGISTER("hmat", &hmat_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
