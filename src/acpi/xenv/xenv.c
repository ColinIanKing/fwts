/*
 * Copyright (C) 2010-2016 Canonical
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

#include <string.h>
#include <inttypes.h>

static fwts_acpi_table_info *table;

static int xenv_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "XENV", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI XENV table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 * Sanity check XENV table, see:
 *    http://wiki.xenproject.org/mediawiki/images/c/c4/Xen-environment-table.pdf
 */
static int xenv_test1(fwts_framework *fw)
{
	fwts_acpi_table_xenv *xenv = (fwts_acpi_table_xenv*)table->data;
	bool passed = true;

	if (table->length < sizeof(fwts_acpi_table_xenv)) {
		fwts_failed(fw, LOG_LEVEL_HIGH, "XENVAcpiTableTooSmall",
			"XENV ACPI table is %zd bytes long which is smaller "
			"than the expected size of %zd bytes.",
			table->length, sizeof(fwts_acpi_table_xenv));
		return FWTS_ERROR;
	}

	if (xenv->header.revision != 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"XENVBadRevision",
			"XENV revision is incorrect, expecting 0x01, "
			"instead got 0x%2.2" PRIx8,
			xenv->header.revision);
	}

	fwts_log_info_verbatim(fw, "XENV Table:");
	fwts_log_info_verbatim(fw, "  GNT Start Address:               0x%16.16" PRIx64, xenv->gnt_start);
	fwts_log_info_verbatim(fw, "  GNT Size:                        0x%16.16" PRIx64, xenv->gnt_size);
	fwts_log_info_verbatim(fw, "  Evtchn Intr:                     0x%8.8"   PRIx32, xenv->evtchn_intr);
	fwts_log_info_verbatim(fw, "  Evtchn Intr Flags:               0x%2.2"   PRIx8,  xenv->evtchn_intr_flags);

	if (xenv->evtchn_intr_flags & ~3) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"XENVBadEvtchnIntrFlags",
			"XENV Evtchn Intr Flags was 0x%2.2" PRIx8
			" and reserved bits [7:2] are not zero.",
			xenv->evtchn_intr_flags);
	}

	if (passed)
		fwts_passed(fw, "No issues found in XENV table.");

	return FWTS_OK;
}

static fwts_framework_minor_test xenv_tests[] = {
	{ xenv_test1, "Validate XENV table." },
	{ NULL, NULL }
};

static fwts_framework_ops xenv_check_ops = {
	.description = "XENV Xen Environment Table tests.",
	.init        = xenv_init,
	.minor_tests = xenv_tests
};

FWTS_REGISTER("xenv", &xenv_check_ops, FWTS_TEST_ANYTIME,
	FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
