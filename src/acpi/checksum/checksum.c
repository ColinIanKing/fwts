/*
 * Copyright (C) 2010-2011 Canonical
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "fwts.h"

static char *checksum_headline(void)
{
	return "Check ACPI table checksum.";
}

static int checksum_scan_tables(fwts_framework *fw)
{
	int i;

	for (i=0;; i++) {
		fwts_acpi_table_info *table;
		int j;
		
		if (fwts_acpi_get_table(fw, i, &table) != FWTS_OK) {
			fwts_aborted(fw, "Cannot load ACPI tables.");
			return FWTS_ABORTED;
		}
		if (table == NULL)
			break;

		if (strcmp("RSDP", table->name) == 0)
			continue;
		if (strcmp("FACS", table->name) == 0)
			continue;

		fwts_acpi_table_header *hdr = (fwts_acpi_table_header*)table->data;
		uint8_t *data = (uint8_t*) table->data;
		uint8_t checksum = 0;

		for (j=0; j<table->length; j++)
			checksum += data[j];
	
		if (checksum == 0)
			fwts_passed(fw, "Table %s has correct checksum 0x%x.", table->name, hdr->checksum);
		else {
			fwts_failed_low(fw, "Table %s has incorrect checksum, expected 0x%x, got 0x%x.", table->name, 256-checksum, hdr->checksum);
		}	fwts_tag_failed(fw, FWTS_TAG_ACPI_TABLE_CHECKSUM);
	}
	return FWTS_OK;
}

static int checksum_test1(fwts_framework *fw)
{
	return checksum_scan_tables(fw);
}

static fwts_framework_minor_test checksum_tests[] = {
	{ checksum_test1, "Check ACPI table checksums." },
	{ NULL, NULL }
};

static fwts_framework_ops checksum_ops = {
	.headline    = checksum_headline,
	.minor_tests = checksum_tests
};

FWTS_REGISTER(checksum, &checksum_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
