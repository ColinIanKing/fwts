/*
 * Copyright (C) 2017-2020 Canonical
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

static fwts_acpi_table_info *table;
acpi_table_init(SDEV, &table)

static void sdev_acpi_namespace_device_test(fwts_framework *fw, const fwts_acpi_table_sdev_acpi *entry, bool *passed)
{
	fwts_log_info_verbatim(fw, "  ACPI Integrated Device (Type 0):");
	fwts_log_info_verbatim(fw, "    Type:                           0x%2.2" PRIx8, entry->header.type);
	fwts_log_info_verbatim(fw, "    Flags:                          0x%2.2" PRIx8, entry->header.flags);
	fwts_log_info_verbatim(fw, "    Length:                         0x%4.4" PRIx16, entry->header.length);
	fwts_log_info_verbatim(fw, "    Device Id Offset:               0x%4.4" PRIx16, entry->device_id_offset);
	fwts_log_info_verbatim(fw, "    Device Id Length:               0x%4.4" PRIx16, entry->device_id_length);
	fwts_log_info_verbatim(fw, "    Vendor Specific Data Offset:    0x%4.4" PRIx16, entry->vendor_offset);
	fwts_log_info_verbatim(fw, "    Vendor Specific Data Length:    0x%4.4" PRIx16, entry->vendor_length);

	fwts_acpi_reserved_bits_check(fw, "SDEV", "Flags", entry->header.flags, sizeof(entry->header.flags), 1, 15, passed);
}

static void sdev_pcie_endpoint_device_test(fwts_framework *fw, const fwts_acpi_table_sdev_pcie *entry, bool *passed)
{
	fwts_log_info_verbatim(fw, "  PCIe Endpoint Device (Type 1):");
	fwts_log_info_verbatim(fw, "    Type:                           0x%2.2" PRIx8, entry->header.type);
	fwts_log_info_verbatim(fw, "    Flags:                          0x%2.2" PRIx8, entry->header.flags);
	fwts_log_info_verbatim(fw, "    Length:                         0x%4.4" PRIx16, entry->header.length);
	fwts_log_info_verbatim(fw, "    PCI Segment Number:             0x%4.4" PRIx16, entry->segment);
	fwts_log_info_verbatim(fw, "    Start Bus Number:               0x%4.4" PRIx16, entry->start_bus);
	fwts_log_info_verbatim(fw, "    PCI Path Offset:                0x%4.4" PRIx16, entry->path_offset);
	fwts_log_info_verbatim(fw, "    PCI Path Length:                0x%4.4" PRIx16, entry->path_length);
	fwts_log_info_verbatim(fw, "    Vendor Specific Data Offset:    0x%4.4" PRIx16, entry->vendor_offset);
	fwts_log_info_verbatim(fw, "    Vendor Specific Data Length:    0x%4.4" PRIx16, entry->vendor_length);

	fwts_acpi_reserved_bits_check(fw, "SDEV", "Flags", entry->header.flags, sizeof(entry->header.flags), 1, 15, passed);
}

static int sdev_test1(fwts_framework *fw)
{
	fwts_acpi_table_sdev_header *entry;
	bool passed = true;
	uint32_t offset;

	fwts_log_info_verbatim(fw, "SDEV Secure Devices Table:");

	entry = (fwts_acpi_table_sdev_header *) (table->data + sizeof(fwts_acpi_table_sdev));
	offset = sizeof(fwts_acpi_table_sdev);
	while (offset < table->length) {
		uint32_t type_length;

		if (entry->length == 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH, "SDEVStructLengthZero",
				    "SDEV structure (offset 0x%4.4" PRIx32 ") "
				    "length cannot be 0", offset);
			break;
		}

		if (entry->type == FWTS_ACPI_SDEV_TYPE_ACPI_NAMESPACE) {
			fwts_acpi_table_sdev_acpi *acpi = (fwts_acpi_table_sdev_acpi *) entry;
			sdev_acpi_namespace_device_test(fw, acpi, &passed);
			type_length = sizeof(fwts_acpi_table_sdev_acpi) + acpi->device_id_length + acpi->vendor_length;
		} else if (entry->type == FWTS_ACPI_SDEV_TYPE_PCIE_ENDPOINT) {
			fwts_acpi_table_sdev_pcie *pcie = (fwts_acpi_table_sdev_pcie *) entry;
			sdev_pcie_endpoint_device_test(fw, pcie, &passed);
			type_length = sizeof(fwts_acpi_table_sdev_pcie) + pcie->path_length + pcie->vendor_length;
		} else {
			fwts_acpi_reserved_type_check(fw, "SDEV", entry->type, 0, FWTS_ACPI_SDEV_TYPE_RESERVED, &passed);
			break;
		}

		if (!fwts_acpi_structure_length_check(fw, "SDEV", entry->type, entry->length, type_length)) {
			passed = false;
			break;
		}

		if ((offset += entry->length) > table->length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"SDEVBadTableLength",
				"SDEV has more subtypes than its size can handle");
			break;
		}

		entry = (fwts_acpi_table_sdev_header *) (table->data + offset);
		fwts_log_nl(fw);
	}

	fwts_log_nl(fw);
	if (passed)
		fwts_passed(fw, "No issues found in SDEV table.");

	return FWTS_OK;
}

static fwts_framework_minor_test sdev_tests[] = {
	{ sdev_test1, "Validate SDEV table." },
	{ NULL, NULL }
};

static fwts_framework_ops sdev_ops = {
	.description = "SDEV Secure Devices Table test",
	.init        = SDEV_init,
	.minor_tests = sdev_tests
};

FWTS_REGISTER("sdev", &sdev_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
