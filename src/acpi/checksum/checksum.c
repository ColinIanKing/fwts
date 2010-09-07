/*
 * Copyright (C) 2010 Canonical
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

static int checksum_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	return FWTS_OK;
}

static char *checksum_headline(void)
{
	return "Check ACPI table checksum.";
}

static int checksum_scan_tables(fwts_framework *fw)
{
	int i;
	fwts_acpi_table_info *table;

	for (i=0; (table = fwts_acpi_get_table(i)) != NULL; i++) {
		int j;

		if (strcmp("RSDP", table->name) == 0)
			continue;
		if (strcmp("FACS", table->name) == 0)
			continue;

		fwts_acpi_table_header *hdr = (fwts_acpi_table_header*)table->data;
		uint8 *data = (uint8*) table->data;
		uint8 checksum = 0;

		for (j=0; j<hdr->length; j++) 
			checksum += data[j];
	
		if (checksum == 0)
			fwts_passed(fw, "Table %s has correct checksum 0x%x.", table->name, hdr->checksum);
		else
			fwts_failed_low(fw, "Table %s has incorrect checksum, expected 0x%x, got 0x%x.", table->name, 256-checksum, hdr->checksum);
	}
	return FWTS_OK;
}

static int checksum_test1(fwts_framework *fw)
{
	checksum_scan_tables(fw);

	return FWTS_OK;
}


static fwts_framework_tests checksum_tests[] = {
	checksum_test1,
	NULL
};

static fwts_framework_ops checksum_ops = {
	checksum_headline,
	checksum_init,
	NULL,
	checksum_tests
};

FWTS_REGISTER(checksum, &checksum_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
