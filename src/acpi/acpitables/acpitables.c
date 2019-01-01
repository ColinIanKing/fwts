/*
 * Copyright (C) 2010-2019 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>

static bool acpi_table_check_field(const char *field, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (!isascii(field[i]))
			return false;

	return true;
}

static bool acpi_table_check_field_test(
	fwts_framework *fw,
	const char *table_name,
	const char *field_name,
	const char *field,
	const size_t len)
{
	if (!acpi_table_check_field(field, len)) {
		fwts_failed(fw, LOG_LEVEL_LOW, "ACPITableHdrInfo",
			"ACPI Table %s has non-ASCII characters in "
			"header field %s", table_name, field_name);
		return false;
	}
	return true;
}

static int acpi_table_check_test1(fwts_framework *fw)
{
	int i;
	bool checked = false;

	for (i = 0; ; i++) {
		fwts_acpi_table_info *info;
		fwts_acpi_table_header *hdr;
		char name[50];
		bool passed;

		if (fwts_acpi_get_table(fw, i, &info) != FWTS_OK)
			break;
		if (info == NULL)
			continue;

		checked = true;
		/* RSDP and FACS are special cases, skip these */
		if (!strcmp(info->name, "RSDP") ||
		    !strcmp(info->name, "FACS"))
			continue;

		hdr = (fwts_acpi_table_header *)info->data;
		if (acpi_table_check_field(hdr->signature, 4)) {
			snprintf(name, sizeof(name), "%4.4s", hdr->signature);
		} else {
			/* Table name not printable, so identify it by the address */
			snprintf(name, sizeof(name), "at address 0x%" PRIx64, info->addr);
		}

		/*
		 * Tables shouldn't be short, however, they do have at
		 * least 4 bytes with their signature else they would not
		 * have been loaded by this stage.
		 */
		if (hdr->length < sizeof(fwts_acpi_table_header)) {
			fwts_failed(fw, LOG_LEVEL_HIGH, "ACPITableHdrShort",
				"ACPI Table %s is too short, only %" PRIu32 " bytes long. Further "
				"header checks will be omitted.", name, hdr->length);
			continue;
		}
		/* Warn about empty tables */
		if (hdr->length == sizeof(fwts_acpi_table_header)) {
			fwts_warning(fw,
				"ACPI Table %s is empty and just contains a table header. Further "
				"header checks will be omitted.", name);
			continue;
		}

		passed = acpi_table_check_field_test(fw, name, "Signature", hdr->signature, 4) &
			 acpi_table_check_field_test(fw, name, "OEM ID", hdr->oem_id, 6) &
			 acpi_table_check_field_test(fw, name, "OEM Table ID", hdr->oem_tbl_id, 8) &
			 acpi_table_check_field_test(fw, name, "Creator ID", hdr->creator_id, 4);
		if (passed)
			fwts_passed(fw, "Table %s has valid signature and ID strings.", name);

	}
	if (!checked)
		fwts_aborted(fw, "Cannot find any ACPI tables.");

	return FWTS_OK;
}

static fwts_framework_minor_test acpi_table_check_tests[] = {
	{ acpi_table_check_test1, "Test ACPI headers." },
	{ NULL, NULL }
};

static fwts_framework_ops acpi_table_check_ops = {
	.description = "ACPI table headers sanity tests.",
	.minor_tests = acpi_table_check_tests
};

FWTS_REGISTER("acpitables", &acpi_table_check_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI | FWTS_FLAG_TEST_SBBR)

#endif
