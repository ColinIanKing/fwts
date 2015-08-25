/*
 * Copyright (C) 2015 Canonical
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
#include <ctype.h>

static fwts_acpi_table_info *table;

static int rsdp_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "RSDP", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI RSDP table does not exist, skipping test");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 *  RSDP Root System Description Pointer
 */
static int rsdp_test1(fwts_framework *fw)
{
	fwts_acpi_table_rsdp *rsdp = (fwts_acpi_table_rsdp*)table->data;
	bool passed = true;
	size_t i;

	for (i = 0; i < 6; i++) {
		if (!isprint(rsdp->oem_id[i])) {
			passed = false;
			break;
		}
	}
	if (!passed) {
		fwts_failed(fw, LOG_LEVEL_LOW,
			"RSDPBadOEMId",
			"RSDP: oem_id contains non-printable "
			"characters.");
		fwts_advice(fw,
			"The RSDP OEM Id is non-conforming, but this "
			"will not affect the system behaviour. However "
			"this should be fixed if possible to make the "
			"firmware ACPI complaint.");
	}
	if (rsdp->revision > 2) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"RSDPBadRevisionId",
			"RSDP: revision is %" PRIu8 ", expected "
			"value less than 2.", rsdp->revision);
	}

	if (passed)
		fwts_passed(fw, "No issues found in RSDP table.");

	return FWTS_OK;
}

static fwts_framework_minor_test rsdp_tests[] = {
	{ rsdp_test1, "RSDP Root System Description Pointer test." },
	{ NULL, NULL }
};

static fwts_framework_ops rsdp_ops = {
	.description = "RSDP Root System Description Pointer test.",
	.init        = rsdp_init,
	.minor_tests = rsdp_tests
};

FWTS_REGISTER("rsdp", &rsdp_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)
