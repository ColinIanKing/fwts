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
acpi_table_init(SKVL, &table)

static int skvl_test1(fwts_framework *fw)
{
	fwts_acpi_skvl_key_struct *key_struct;
	bool passed = true;
	uint32_t offset;
	fwts_acpi_table_skvl *skvl = (fwts_acpi_table_skvl *)table->data;

	fwts_log_info_verbatim(fw, "Storage Volume Key Location Table:");
	fwts_log_info_simp_int(fw, "  Key Count:    ", skvl->key_count);
	fwts_log_nl(fw);

	offset = sizeof(fwts_acpi_table_skvl);

	for (uint32_t i = 0; i < skvl->key_count; i++) {
		if ((offset + sizeof(fwts_acpi_skvl_key_struct)) > table->length) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"SKVLOutOfRangeOffset",
			"SKVL key structure offset is out of range.");
			return FWTS_OK;
		}
		key_struct = (fwts_acpi_skvl_key_struct *)(table->data + offset);
		fwts_log_info_verbatim(fw, "  Storage Volume Key Structure:     ");
		fwts_log_info_simp_int(fw, "  Key Type:    ", key_struct->key_type);
		fwts_acpi_fixed_value(fw, LOG_LEVEL_HIGH, "SKVL", "Key Type", key_struct->key_type, 0, &passed);
		fwts_log_info_simp_int(fw, "  Key Format:  ", key_struct->key_format);
		fwts_acpi_fixed_value(fw, LOG_LEVEL_HIGH, "SKVL", "Key Format", key_struct->key_format, 0, &passed);
		fwts_log_info_simp_int(fw, "  Key Size:    ", key_struct->key_size);
		fwts_log_info_simp_int(fw, "  Key Address: ", key_struct->key_address);

		offset += sizeof(fwts_acpi_skvl_key_struct);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in SKVL table.");

	return FWTS_OK;
}

static fwts_framework_minor_test skvl_tests[] = {
	{ skvl_test1, "Validate SKVL table." },
	{ NULL, NULL }
};

static fwts_framework_ops skvl_ops = {
	.description = "SKVL Storage Volume Key Location Table test.",
	.init        = SKVL_init,
	.minor_tests = skvl_tests
};

FWTS_REGISTER("skvl", &skvl_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
