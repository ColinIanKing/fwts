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

static int checksum_table(fwts_framework *fw, const char *filename, const char *basename)
{
	int fd;
	uint8 *data;
	fwts_acpi_table_header hdr;
	int i;
	uint8 checksum = 0;
	int length;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		fwts_failed_low(fw, "Cannot read table %s.", basename);
		return FWTS_ERROR;
	}

	if ((data = fwts_acpi_table_read(fd, &length)) == NULL) {
		fwts_failed_low(fw, "Cannot read table %s.", basename);
		return FWTS_ERROR;
	}
	
	fwts_acpi_table_get_header(&hdr, data);

	if (length < sizeof(fwts_acpi_table_header)) {
		fwts_failed_high(fw, "Table %s is shorter than the ACPI table header.", basename);
		return FWTS_ERROR;
	}

	for (i=0; i<hdr.length; i++) 
		checksum += data[i];

	if (checksum == 0)
		fwts_passed(fw, "Table %s has correct checksum 0x%x.", basename, hdr.checksum);
	else
		fwts_failed_low(fw, "Table %s has incorrect checksum, expected 0x%x, got 0x%x.", basename, 256-checksum, hdr.checksum);

	return FWTS_OK;
}

static int checksum_scan_tables(fwts_framework *fw, const char *path)
{
	DIR *dir;
	struct dirent *entry;

	if ((dir = opendir(path)) == NULL) 
		return FWTS_ERROR;

	while ((entry = readdir(dir)) != NULL) {
		if (strlen(entry->d_name) > 2) {
			struct stat buf;
			char pathname[PATH_MAX];

			snprintf(pathname, sizeof(pathname), "%s/%s", path, entry->d_name);
			if (stat(pathname, &buf) != 0)
				continue;
			if (S_ISDIR(buf.st_mode))
				checksum_scan_tables(fw, pathname);
			else
				checksum_table(fw, pathname, entry->d_name);
		}
	}
	return FWTS_OK;
}

static int checksum_test1(fwts_framework *fw)
{
	char *path = (fw->acpi_table_path == NULL) ? 
		FWTS_ACPI_TABLES_PATH : fw->acpi_table_path;

	checksum_scan_tables(fw, path);

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
