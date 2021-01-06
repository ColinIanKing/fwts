/*
 * Copyright (C) 2015-2021 Canonical
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

#if defined(FWTS_HAS_ACPI) && !(FWTS_ARCH_AARCH64)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <alloca.h>

#include "fwts_acpi_object_eval.h"

static fwts_acpi_table_info *table;
acpi_table_init(WAET, &table)

/*
 *  WAET Windows ACPI Emulated Devices Table
 *   see http://msdn.microsoft.com/en-us/windows/hardware/gg487524.aspx
 */
static int waet_test1(fwts_framework *fw)
{
	bool passed = true;
	fwts_acpi_table_waet *waet = (fwts_acpi_table_waet *)table->data;

	/* Enough length for the initial waet header? */
	if (!fwts_acpi_table_length_check(fw, "WAET", table->length, sizeof(fwts_acpi_table_waet))) {
		passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "WAET Table:");
	fwts_log_info_verbatim(fw, "  Emulated Device Flags:    0x%8.8" PRIx32, waet->flags);
	fwts_log_info_verbatim(fw, "    Bit [0] RTC Good:       %1" PRIu32, waet->flags & 1);
	fwts_log_info_verbatim(fw, "    Bit [1] PM Timer Good:  %1" PRIu32, (waet->flags >> 1) & 1);
	fwts_log_nl(fw);

	fwts_acpi_reserved_bits_check(fw, "WAET", "Emulated Device Flags", waet->flags, sizeof(waet->flags), 2, 31, &passed);

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
	.init        = WAET_init,
	.minor_tests = waet_tests
};

FWTS_REGISTER("waet", &waet_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
