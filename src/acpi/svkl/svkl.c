/*
 * Copyright (C) 2022-2026 Canonical
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
acpi_table_init(SVKL, &table)

static int svkl_test1(fwts_framework *fw)
{
	fwts_acpi_table_svkl *svkl = (fwts_acpi_table_svkl *) table->data;
	fwts_acpi_table_svkl_key_structure *key_structure;
	uint32_t offset, count, i;
	bool passed = true;

	if (table->length < (size_t)svkl->header.length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"SVKLTooShort",
			"SVKL table incorrectly sized, SVKL "
			"header reports it is %" PRIu32 " bytes, "
			"instead got %zu bytes",
			svkl->header.length, table->length);
		return FWTS_OK;
	}

	if (table->length < sizeof(fwts_acpi_table_svkl)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"SVKLTooShort",
			"SVKL table too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_svkl), table->length);
		return FWTS_OK;
	}

	fwts_log_info_verbatim(fw, "SVKL Storage Volume Key Data Table:");
	fwts_log_info_simp_int(fw, "  Key Count:         ", svkl->key_count);

	offset = sizeof(fwts_acpi_table_svkl);
	key_structure = (fwts_acpi_table_svkl_key_structure *) (table->data + offset);

	count = (svkl->header.length - offset) / sizeof(fwts_acpi_table_svkl_key_structure);
	if (count != svkl->key_count) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_CRITICAL,
			"SVKLBadStructureCount",
			"SVKL should have %" PRId32 " key structures, got %" PRId32,
			svkl->key_count, count);
		return FWTS_OK;
	}

	for (i = 0; i < svkl->key_count; i++) {
		fwts_log_info_verbatim(fw, "  Storage Volume Key Structure %" PRIu8, (i + 1));
		fwts_log_info_simp_int(fw, "    Key Type:        ", key_structure->key_type);
		if (key_structure->key_type != 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SVKLBadKeyType",
				"SVKL key type not zero, 1~0xFFFF reserved.");
		}
		fwts_log_info_simp_int(fw, "    Key Format:      ", key_structure->key_format);
		if (key_structure->key_format != 0) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SVKLBadKeyFormat",
				"SVKL key format not zero, 1~0xFFFF reserved.");
		}
		fwts_log_info_simp_int(fw, "    Key Size:        ", key_structure->key_size);
		fwts_log_info_simp_int(fw, "    Key Address:     ", key_structure->key_addr);

		if ((offset += sizeof(fwts_acpi_table_svkl_key_structure)) > table->length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_CRITICAL,
				"SVKLBadTableLength",
				"SVKL has more key structures than its size can handle");
			break;
		}

		key_structure = (fwts_acpi_table_svkl_key_structure *) (table->data + offset);
	}

	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in SVKL table.");

	return FWTS_OK;
}

static fwts_framework_minor_test svkl_tests[] = {
	{ svkl_test1, "Validate SVKL table." },
	{ NULL, NULL }
};

static fwts_framework_ops svkl_ops = {
	.description = "SVKL Storage Volume Key Data table test.",
	.init        = SVKL_init,
	.minor_tests = svkl_tests
};

FWTS_REGISTER("svkl", &svkl_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
