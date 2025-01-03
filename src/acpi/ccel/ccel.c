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
acpi_table_init(CCEL, &table)

static int ccel_test1(fwts_framework *fw)
{

	bool passed = true;
	fwts_acpi_table_ccel *ccel = (fwts_acpi_table_ccel *)table->data;

	if (ccel->header.length != sizeof(fwts_acpi_table_ccel)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"CCELBadTableLength",
			"CCEL table length should be %" PRIu32 ", got %" PRIu32
			 " instead.", (uint32_t)sizeof(fwts_acpi_table_ccel),
			 ccel->header.length);
		return FWTS_OK;
	}

	fwts_log_info_verbatim(fw, "CC Event Log ACPI Table:");
	fwts_log_info_simp_int(fw, "  CC Type:                       ", ccel->cc_type);
	if (ccel->cc_type == 0 || ccel->cc_type >= 3) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"CCELBadCCType",
			"CCEL CC types must not have the reaserved value 0, 0x03..0xff, "
			"got 0x%2.2" PRIx8 " instead.", ccel->cc_type);
		passed = false;
	}
	fwts_log_info_simp_int(fw, "  CC Subtype:                    ", ccel->cc_subtype);
	fwts_log_info_simp_int(fw, "  Reserved:                      ", ccel->reserved);
	fwts_acpi_reserved_zero("CCEL", "Reserved", ccel->reserved, &passed);
	fwts_log_info_simp_int(fw, "  Log Area Minimum Length(LAML): ", ccel->laml);
	fwts_log_info_simp_int(fw, "  Log Area Start Address(LASA):  ", ccel->lasa);
	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in CCEL table.");

	return FWTS_OK;
}

static fwts_framework_minor_test ccel_tests[] = {
	{ ccel_test1, "Validate CCEL table." },
	{ NULL, NULL }
};

static fwts_framework_ops ccel_ops = {
	.description = "CCEL CC Event Log ACPI Table test.",
	.init        = CCEL_init,
	.minor_tests = ccel_tests
};

FWTS_REGISTER("ccel", &ccel_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
