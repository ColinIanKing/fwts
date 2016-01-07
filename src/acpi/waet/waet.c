/*
 * Copyright (C) 2015-2016 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <alloca.h>

#include "fwts_acpi_object_eval.h"

static fwts_acpi_table_info *table;

static int waet_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "WAET", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI WAET table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  WAET Windows ACPI Emulated Devices Table
 *   see http://msdn.microsoft.com/en-us/windows/hardware/gg487524.aspx
 */
static int waet_test1(fwts_framework *fw)
{
	bool passed = true;
	fwts_acpi_table_waet *waet = (fwts_acpi_table_waet *)table->data;

	/* Enough length for the initial waet header? */
	if (table->length < sizeof(fwts_acpi_table_waet)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"WAETTooShort",
			"WAET table too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_waet), table->length);
		goto done;
	}

	fwts_log_info_verbatum(fw, "WAET Table:");
	fwts_log_info_verbatum(fw, "  Emulated Device Flags:    0x%8.8" PRIx32, waet->flags);
	fwts_log_info_verbatum(fw, "    Bit [0] RTC Good:       %1" PRIu32, waet->flags & 1);
	fwts_log_info_verbatum(fw, "    Bit [1] PM Timer Good:  %1" PRIu32, (waet->flags >> 1) & 1);
	fwts_log_nl(fw);

	if (waet->flags & ~3) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"WAETFlagReservedNotZero",
			"WAET Emulated Device Flags was 0x%" PRIx32
			" and so some of reserved bits [31:2] are not zero "
			" as expected.", waet->flags);
	}
done:
	if (passed)
		fwts_passed(fw, "No issues found in WAET table.");

	return FWTS_OK;
}

static fwts_framework_minor_test waet_tests[] = {
	{ waet_test1, "Windows ACPI Emulated Devices Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops waet_ops = {
	.description = "WAET Windows ACPI Emulated Devices Table test.",
	.init        = waet_init,
	.minor_tests = waet_tests
};

FWTS_REGISTER("waet", &waet_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)
