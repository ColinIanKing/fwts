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
#include <ctype.h>

static fwts_acpi_table_info *table;

static int wsmt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "WSMT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI WSMT table does not exist, skipping test");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 *  WSMT Windows Platform Binary Table
 */
static int wsmt_test1(fwts_framework *fw)
{
	fwts_acpi_table_wsmt *wsmt = (fwts_acpi_table_wsmt*) table->data;
	bool passed = true;

	fwts_log_info_verbatim(fw, "WSMT Windows SMM Security Mitigations Table:");
	fwts_log_info_verbatim(fw, "  Protection Flags:      0x%8.8" PRIx32, wsmt->protection_flags);

	if (wsmt->protection_flags & ~0x7) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"WSMTFlagsReserved",
			"WSMT Protection Flags reserved bits "
			"[3:31] must be zero, instead got 0x%" PRIx32,
			wsmt->protection_flags);
	}

	if ((wsmt->protection_flags & 0x2) && !(wsmt->protection_flags & 0x1)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"WSMTBadFlagsValue",
			"WSMT Protection Flags bit[1] must be "
			"set when bit[2] is set");
	}

	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in WSMT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test wsmt_tests[] = {
	{ wsmt_test1, "WSMT Windows SMM Security Mitigations Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops wsmt_ops = {
	.description = "WSMT Windows SMM Security Mitigations Table test.",
	.init        = wsmt_init,
	.minor_tests = wsmt_tests
};

FWTS_REGISTER("wsmt", &wsmt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
