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

#include <unistd.h> 
#include <sys/io.h> 

#include "fwts.h"

static int acpidump_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	return FWTS_OK;
}


static char *acpidump_headline(void)
{
	return "Check ACPI table acpidump.";
}

static void acpidump_hdr(fwts_framework *fw, fwts_acpi_table_header *hdr)
{
	fwts_log_info_verbatum(fw, "Signature:        \"%4.4s\"", hdr->signature);
	fwts_log_info_verbatum(fw, "Length:           0x%lx (%lu)", hdr->length, hdr->length);
	fwts_log_info_verbatum(fw, "Revision:         0x%x (%d)", hdr->revision, hdr->revision);
	fwts_log_info_verbatum(fw, "Checksum:         0x%x (%d)", hdr->checksum, hdr->checksum);
	fwts_log_info_verbatum(fw, "OEM ID:           \"%6.6s\"", hdr->oem_id);
	fwts_log_info_verbatum(fw, "OEM Table ID:     \"%6.6s\"", hdr->oem_tbl_id);
	fwts_log_info_verbatum(fw, "OEM Revision:     0x%lx (%lu)", hdr->oem_revision, hdr->oem_revision);
	fwts_log_info_verbatum(fw, "Creator ID:       \"%4.4s\"", hdr->creator_id);
	fwts_log_info_verbatum(fw, "Creator Revision: 0x%lx (%lu)", hdr->creator_revision, hdr->creator_revision);
}

static void acpidump_boot(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_boot *boot;
	uint8 cmos_data;

	if (length < (sizeof(fwts_acpi_table_header) + 4)) {
		fwts_log_info(fw, "Boot table too short\n");
		return;
	}
	boot = (fwts_acpi_table_boot*)(data + sizeof(fwts_acpi_table_header));

	fwts_log_info_verbatum(fw, "CMOS offset: 0x%lx", boot->cmos_index);

	/* TODO: Locate CMOS (ESCD), index into it by offsef cmos_index  and dump the flags as 
	  defined in ftws_acpi_cmos_boot_register */

	/*
	  The CMOS memory exists outside of the normal address space and cannot
	  contain directly executable code. It is reachable through IN and OUT
	  commands at port number 70h (112d) and 71h (113d). To read a CMOS byte,
	  an OUT to port 70h is executed with the address of the byte to be read and
	  an IN from port 71h will then retrieve the requested information. 
	*/

	cmos_data = fwts_cmos_read(boot->cmos_index);

	fwts_log_info_verbatum(fw, "Boot Register: 0x%x", cmos_data);
	fwts_log_info_verbatum(fw, "  PNP-OS:   %x", (cmos_data & FTWS_BOOT_REGISTER_PNPOS) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Booting:  %x", (cmos_data & FWTS_BOOT_REGISTER_BOOTING) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Diag:     %x", (cmos_data & FWTS_BOOT_REGISTER_DIAG) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Suppress: %x", (cmos_data & FWTS_BOOT_REGISTER_SUPPRESS_BOOT_DISPLAY) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Parity:   %x", (cmos_data & FWTS_BOOT_REGISTER_PARITY) ? 1 : 0);
}

static int acpidump_table(fwts_framework *fw, const char *filename, const char *basename)
{
	int fd;
	uint8 *data;
	fwts_acpi_table_header hdr;
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
	acpidump_hdr(fw, &hdr);
	fwts_log_info(fw, "");

	if (strncmp(hdr.signature, "BOOT", 4) == 0)
		acpidump_boot(fw, data, length);

	fwts_log_info(fw, "");

	return FWTS_OK;
}

static int acpidump_all_tables(fwts_framework *fw, const char *path)
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
				acpidump_all_tables(fw, pathname);
			else
				acpidump_table(fw, pathname, entry->d_name);
		}
	}
	return FWTS_OK;
}

static int acpidump_test1(fwts_framework *fw)
{
	char *path = (fw->acpi_table_path == NULL) ? 
		FWTS_ACPI_TABLES_PATH : fw->acpi_table_path;

	acpidump_all_tables(fw, path);

	return FWTS_OK;
}


static fwts_framework_tests acpidump_tests[] = {
	acpidump_test1,
	NULL
};

static fwts_framework_ops acpidump_ops = {
	acpidump_headline,
	acpidump_init,
	NULL,
	acpidump_tests
};

FWTS_REGISTER(acpidump, &acpidump_ops, FWTS_TEST_ANYTIME, FWTS_BATCH_EXPERIMENTAL);
