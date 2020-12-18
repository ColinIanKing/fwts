/*
 * Copyright (C) 2016-2020 Canonical
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
#include <ctype.h>

static fwts_acpi_table_info *table;
acpi_table_init(WPBT, &table)

/*
 *  WPBT Windows Platform Binary Table
 */
static int wpbt_test1(fwts_framework *fw)
{
	fwts_acpi_table_wpbt *wpbt = (fwts_acpi_table_wpbt*) table->data;
	bool passed = true;

	fwts_log_info_verbatim(fw, "WPBT Windows Platform Binary Table:");
	fwts_log_info_verbatim(fw, "  Handoff Memory Size:      0x%8.8" PRIx32, wpbt->handoff_size);
	fwts_log_info_verbatim(fw, "  Handoff Memory Location:  0x%16.16" PRIx64, wpbt->handoff_address);
	fwts_log_info_verbatim(fw, "  Content Layout:           0x%2.2" PRIx8, wpbt->layout);
	fwts_log_info_verbatim(fw, "  Content Type:             0x%2.2" PRIx8, wpbt->type);

	if (wpbt->layout != 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"WPBTBadLayout",
			"WPBT supports Content Layout 1 only, got "
			"0x%2.2" PRIx8 " instead", wpbt->layout);
	}

	if (wpbt->type != 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"WPBTBadType",
			"WPBT supports Content Type 1 only, got "
			"0x%2.2" PRIx8 " instead", wpbt->type);

	} else {
		fwts_acpi_table_wpbt_type1 *type = (fwts_acpi_table_wpbt_type1 *) (table->data + sizeof(fwts_acpi_table_wpbt));
		fwts_log_info_verbatim(fw, "  Arguments Length:         0x%4.4" PRIx16, type->arguments_length);

		if (type->arguments_length % 2) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"WPBTBadArgumentLength",
				"WPBT arugments length must be multiple of 2, got "
				"0x%4.4" PRIx16 " instead", type->arguments_length);
		}
	}

	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in WPBT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test wpbt_tests[] = {
	{ wpbt_test1, "WPBT Windows Platform Binary Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops wpbt_ops = {
	.description = "WPBT Windows Platform Binary Table test.",
	.init        = WPBT_init,
	.minor_tests = wpbt_tests
};

FWTS_REGISTER("wpbt", &wpbt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
