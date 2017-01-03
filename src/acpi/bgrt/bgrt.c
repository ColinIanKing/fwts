/*
 * Copyright (C) 2015-2017 Canonical
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

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

static fwts_acpi_table_info *table;

static int bgrt_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "BGRT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI BGRT table does not exist, skipping test");
		return FWTS_SKIP;
	}
	return FWTS_OK;
}

/*
 *  BGRT Boot Graphics Resource Table
 */
static int bgrt_test1(fwts_framework *fw)
{
	fwts_acpi_table_bgrt *bgrt = (fwts_acpi_table_bgrt*)table->data;
	bool passed = true;

	fwts_log_info_verbatim(fw, "BGRT Boot Graphics Resource Table:");
	fwts_log_info_verbatim(fw, "  Version:                  0x%4.4" PRIx16, bgrt->version);
	fwts_log_info_verbatim(fw, "  Status:                   0x%2.2" PRIx8, bgrt->status);
	fwts_log_info_verbatim(fw, "  Image Type:               0x%2.2" PRIx8, bgrt->image_type);
	fwts_log_info_verbatim(fw, "  Image Memory Address:     0x%16.16" PRIx64, bgrt->image_addr);
	fwts_log_info_verbatim(fw, "  Image Offset X:           0x%8.8" PRIx32, bgrt->image_offset_x);
	fwts_log_info_verbatim(fw, "  Image Offset Y:           0x%8.8" PRIx32, bgrt->image_offset_y);

	if (bgrt->version != 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"BGRTInvalidVersion",
			"BGRT: Version field is 0x%" PRIx8
			" and not the expected value of 0x01",
			bgrt->version);
	}
	if (bgrt->status & ~0x1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"BGRTStatusRersevedBits",
			"BGRT: Status field is 0x%" PRIx8
			", reserved bits [1:7] should be zero",
			bgrt->status);
	}
	if (bgrt->image_type > 0x00) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"BGRTInvalidImageType",
			"BGRT: Image Type field is 0x%" PRIx8
			", the only allowed type is 0x00",
			bgrt->image_type);
	}

	if (passed)
		fwts_passed(fw, "No issues found in BGRT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test bgrt_tests[] = {
	{ bgrt_test1, "BGRT Boot Graphics Resource Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops bgrt_ops = {
	.description = "BGRT Boot Graphics Resource Table test.",
	.init        = bgrt_init,
	.minor_tests = bgrt_tests
};

FWTS_REGISTER("bgrt", &bgrt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
