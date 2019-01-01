/*
 * Copyright (C) 2015-2019 Canonical
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

static int boot_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "BOOT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI BOOT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  BOOT Table
 *   see https://msdn.microsoft.com/en-us/windows/hardware/gg463443.aspx
 */
static int boot_test1(fwts_framework *fw)
{
	bool passed = true;
	fwts_acpi_table_boot *boot = (fwts_acpi_table_boot *)table->data;

	if (table->length < sizeof(fwts_acpi_table_boot)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"BOOTTooShort",
			"BOOT table too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_boot), table->length);
		goto done;
	}

	fwts_log_info_verbatim(fw, "BOOT Table:");
	fwts_log_info_verbatim(fw, "  CMOS Index:               0x%2.2" PRIx8, boot->cmos_index);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, boot->reserved[0]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, boot->reserved[1]);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, boot->reserved[2]);
	fwts_log_nl(fw);

	/*
	 * If the tables directly from the machine we're running on
	 * then we can check out the error region too.
	 */
	if (table->provenance != FWTS_ACPI_TABLE_FROM_FIRMWARE) {
		fwts_log_info(fw, "ACPI table loaded from file so fwts will "
			"skip the read of the CMOS boot register");
	} else {
		uint8_t val;

		fwts_cmos_read(boot->cmos_index, &val);

		fwts_log_info_verbatim(fw, "CMOS value:             Bit 0x%" PRIx8, val);
		fwts_log_info_verbatim(fw, "  PnP OS                [0] 0x%" PRIx8, (val >> 0) & 1);
		fwts_log_info_verbatim(fw, "  Booting Bit           [1] 0x%" PRIx8, (val >> 1) & 1);
		fwts_log_info_verbatim(fw, "  DIAG Bit              [2] 0x%" PRIx8, (val >> 2) & 1);
		fwts_log_info_verbatim(fw, "  SUPPRESSBOOTDISPLAY   [3] 0x%" PRIx8, (val >> 3) & 1);
		fwts_log_info_verbatim(fw, "  RESERVED            [4-6] 0x%" PRIx8, (val >> 4) & 7);
		fwts_log_info_verbatim(fw, "  PARITY                [7] 0x%" PRIx8, (val >> 7) & 1);
		/* Ignore doing parity check sum */
	}
done:
	if (passed)
		fwts_passed(fw, "No issues found in BOOT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test boot_tests[] = {
	{ boot_test1, "BOOT Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops boot_ops = {
	.description = "BOOT Table test.",
	.init        = boot_init,
	.minor_tests = boot_tests
};

FWTS_REGISTER("boot", &boot_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
