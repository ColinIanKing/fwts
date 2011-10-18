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

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

/*
 *  dmi_str()
 *	return the string at the specified offset on the table
 *	offset is the Nth field in the hdr, and this contains
 *	the index to the Nth string at the end of the DMI entry
 */
static const char* dmi_str(fwts_dmi_header *hdr, uint8_t offset)
{
	char *data = (char *)hdr;
	uint8_t	i = data[offset];

	if (i > 0) {
		data += hdr->length;
		while (i > 1 && *data) {
			data += strlen(data) + 1;
			i--;
		}
		return (char*)data;
	}
	return "";
}

/*
 *  dmi_get_bios_information()
 *	Scan for DMI entry "BIOS Information", type 0
 *	return NULL if we cannot find the entry, otherwise return
 *	the entry starting from its header.
 */
static fwts_dmi_header *dmi_get_bios_information(fwts_framework *fw,
	fwts_smbios_entry *entry,
	uint8_t  *table)
{
	uint8_t *entry_data = table;
	uint16_t table_length = entry->struct_table_length;
	uint16_t struct_count = entry->number_smbios_structures;
	int i = 0;

	while ((i < struct_count) &&
	       (entry_data <= (table + table_length - 4))) {
		fwts_dmi_header *hdr = (fwts_dmi_header *)entry_data;
		uint8_t *next_entry = entry_data + hdr->length;

		if (hdr->length < 4)
			break;

		/* Look for structure terminator, ends in two zero bytes */
		while (((next_entry - table + 1) < table_length) &&
		       ((next_entry[0] != 0) || (next_entry[1] != 0))) {
			next_entry++;
		}
		next_entry += 2;

		/* Got valid table type 0? */
		if (((next_entry - table) <= table_length) && (hdr->type == 0x00))
			return hdr;

		i++;
		entry_data = next_entry;
	}
	return NULL;
}

static int crs_get_bios_date(fwts_framework *fw, int *day, int *mon, int *year)
{
	void *addr;
	const char *date;
	fwts_smbios_entry entry;
	fwts_smbios_type  type;
	uint16_t version = 0;
	uint8_t  *table;
	fwts_dmi_header *hdr;

	addr = fwts_smbios_find_entry(fw, &entry, &type, &version);
	if (addr == NULL) {
		fwts_log_error(fw, "Cannot find SMBIOS or DMI tables.");
		return FWTS_ERROR;
	}

	table = fwts_mmap((off_t)entry.struct_table_address,
		         (size_t)entry.struct_table_length);
	if (table == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap SMBIOS tables from %8.8x..%8.8x.",
			entry.struct_table_address,
			entry.struct_table_address + entry.struct_table_length);
		return FWTS_ERROR;
	}

	/* Search for BIOS Information table */
	if ((hdr = dmi_get_bios_information(fw, &entry, table)) == NULL) {
		(void)fwts_munmap(table, (size_t)entry.struct_table_length);
		return FWTS_ERROR;
	}

	/*
	 * Offset 8 contains the BIOS release date, see System Management
	 * BIOS (SMBIOS) Reference Specification, section 7.1
	 */
	date = dmi_str(hdr, 0x8);
	*mon = *day = *year = 0;
	/* Assume mon/day/year, but we only care about the year anyway */
	sscanf(date, "%d/%d/%d", mon, day, year);

	(void)fwts_munmap(table, (size_t)entry.struct_table_length);

	return FWTS_OK;
}

static int crs_test1(fwts_framework *fw)
{
	fwts_list *klog;
	int day, mon, year;
	char *cmdline;

	if ((cmdline = fwts_get("/proc/cmdline")) == NULL) {
		fwts_log_error(fw, "Cannot read /proc/cmdline");
		return FWTS_ERROR;
	}

	if (crs_get_bios_date(fw, &day, &mon, &year) != FWTS_OK) {
		fwts_log_error(fw, "Cannot determine age of BIOS.");
		return FWTS_ERROR;
	}

	if ((klog = fwts_klog_read()) == NULL) {
		fwts_log_error(fw, "Cannot read kernel log.");
		return FWTS_ERROR;
	}

        if (fwts_klog_regex_find(fw, klog,
		"PCI: Ignoring host bridge windows from ACPI;") > 0) {
		if (strstr(cmdline, "pci=nocrs") != NULL) {
			fwts_skipped(fw, "Kernel was booted with pci=nocrs, Ignoring host bridge windows _CRS settings from ACPI, skipping test.");
		}
		else {
			if (year == 0) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"BIOSTooOld",
					"The kernel could not determine the BIOS age "
					"and has assumed that your BIOS is too old to correctly "
					"specify the host bridge MMIO aperture using _CRS.");
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
				fwts_log_advice(fw, "You can override this by booting with \"pci=use_crs\".");

			} else if (year < 2008) {
				fwts_passed(fw,
					"The kernel has detected an old BIOS (%d/%d/%d) "
					"and has assumed that your BIOS is too old to correctly "
					"specify the host bridge MMIO aperture using _CRS.", mon, day, year);
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
				fwts_log_advice(fw, "You can override this by booting with \"pci=use_crs\".");
			} else {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"HostBridgeWindows",
					"The kernel is ignoring host bridge windows from ACPI for some unknown reason. "
					"pci=nocrs has not been used as a boot parameter and the BIOS may be recent enough "
					"to support this (%d/%d/%d)", mon, day, year);
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
			}
		}
	} else if (fwts_klog_regex_find(fw, klog, "PCI: Using host bridge windows from ACPI;") > 0) {
		if (strstr(cmdline, "pci=use_crs") != NULL) {
			if (year == 0)  {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					"BIOSNoReleaseDate",
					"The BIOS does not seem to have release date, hence pci=use_crs was required.");
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
			} else if (year < 2008) {
				fwts_passed(fw,
					"The BIOS is relatively old (%d/%d/%d) and hence pci=use_crs was required to "
					"enable host bridge windows _CRS settings from ACPI.", mon, day, year);
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
			} else {
				fwts_failed(fw, LOG_LEVEL_LOW,
					"BIOSSupportBridgeWindows",
					"Kernel was booted with pci=use_crs but this may be uncessary as "
					"the BIOS is new enough to support automatic bridge windows configuring using _CRS from ACPI. "
					"However, the workaround may be necessary because _CRS is incorrect or not implemented in the "
					"DSDT.");
				fwts_tag_failed(fw, FWTS_TAG_BIOS);
			}
		}
		else {
			fwts_passed(fw,
				"The kernel has detected a BIOS newer than the end of 2007 (%d/%d/%d) "
				"and has assumed that your BIOS can correctly "
				"specify the host bridge MMIO aperture using _CRS.  If this does not work "
				"correctly you can override this by booting with \"pci=nocrs\".", mon, day, year);
		}
	} else {
		fwts_skipped(fw, "Cannot find host bridge message in kernel log, skipping test.");
	}

	fwts_list_free(klog, free);
	free(cmdline);

	return FWTS_OK;
}

static fwts_framework_minor_test crs_tests[] = {
	{ crs_test1, "Check PCI host bridge configuration using _CRS." },
	{ NULL, NULL },
};

static fwts_framework_ops crs_ops = {
	.description = "Check PCI host bridge configuration using _CRS.",
	.minor_tests = crs_tests
};

FWTS_REGISTER(crs, &crs_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
