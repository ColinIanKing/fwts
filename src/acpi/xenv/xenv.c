/*
 * Copyright (C) 2010-2021 Canonical
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
acpi_table_init(XENV, &table)

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

	fwts_acpi_revision_check("XENV", xenv->header.revision, 1, &passed);

	fwts_log_info_verbatim(fw, "XENV Table:");
	fwts_log_info_verbatim(fw, "  GNT Start Address:               0x%16.16" PRIx64, xenv->gnt_start);
	fwts_log_info_verbatim(fw, "  GNT Size:                        0x%16.16" PRIx64, xenv->gnt_size);
	fwts_log_info_verbatim(fw, "  Evtchn Intr:                     0x%8.8"   PRIx32, xenv->evtchn_intr);
	fwts_log_info_verbatim(fw, "  Evtchn Intr Flags:               0x%2.2"   PRIx8,  xenv->evtchn_intr_flags);

	fwts_acpi_reserved_bits_check(fw, "XENV", "Evtchn Intr Flags", xenv->evtchn_intr_flags, sizeof(xenv->evtchn_intr_flags), 2, 7, &passed);

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
	.init        = XENV_init,
	.minor_tests = xenv_tests
};

FWTS_REGISTER("xenv", &xenv_check_ops, FWTS_TEST_ANYTIME,
	FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
