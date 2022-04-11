/*
 * Copyright (C) 2016-2022 Canonical
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
acpi_table_init(ASPT, &table)

/*
 *  ASPT Table
 *    (reverse engineered, table is common on AMD machines)
 */
static int aspt_test1(fwts_framework *fw)
{
	bool passed = true;
	fwts_acpi_table_aspt *aspt = (fwts_acpi_table_aspt *)table->data;

	if (!fwts_acpi_table_length(fw, "ASPT", table->length, sizeof(fwts_acpi_table_aspt))) {
		passed = false;
		goto done;
	}

	fwts_log_info_verbatim(fw, "ASPT Table:");
	fwts_log_info_simp_int(fw, "  SPTT Start Address: ", aspt->sptt_addr_start);
	fwts_log_info_simp_int(fw, "  SPTT End Address:   ", aspt->sptt_addr_end);
	fwts_log_info_simp_int(fw, "  AMRT Start Address: ", aspt->amrt_addr_start);
	fwts_log_info_simp_int(fw, "  AMRT End Address:   ", aspt->amrt_addr_end);
	fwts_log_nl(fw);

	/*
	 * Without a specification to work with there is very
	 * little we can do to validate this apart from the
	 * simplest sanity check
	 */
	if (aspt->sptt_addr_end < aspt->sptt_addr_start) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"ASPTSpttEndError",
			"ASPT SPTT end address is less than the APTT start "
			"address.");
		passed = false;
	}
	if (aspt->amrt_addr_end < aspt->amrt_addr_start) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"ASPTAmrtEndError",
			"ASPT AMRT end address is less than the AMRT start "
			"address.");
		passed = false;
	}
done:
	if (passed)
		fwts_passed(fw, "No issues found in ASPT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test aspt_tests[] = {
	{ aspt_test1, "ASPT Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops aspt_ops = {
	.description = "ASPT Table test.",
	.init        = ASPT_init,
	.minor_tests = aspt_tests
};

FWTS_REGISTER("aspt", &aspt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
