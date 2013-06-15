/*
 * Copyright (C) 2010-2013 Canonical
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
#include <stdint.h>
#include <inttypes.h>

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

static void smbios_dump_entry(fwts_framework *fw, fwts_smbios_entry *entry, fwts_smbios_type type)
{
	if (type == FWTS_SMBIOS) {
		fwts_log_info_verbatum(fw, "SMBIOS Entry Point Structure:");
		fwts_log_info_verbatum(fw, "  Anchor String          : %4.4s", entry->signature);
		fwts_log_info_verbatum(fw, "  Checksum               : 0x%2.2x", entry->checksum);
		fwts_log_info_verbatum(fw, "  Entry Point Length     : 0x%2.2x", entry->length);
		fwts_log_info_verbatum(fw, "  Major Version          : 0x%2.2x", entry->major_version);
		fwts_log_info_verbatum(fw, "  Minor Version          : 0x%2.2x", entry->minor_version);
		fwts_log_info_verbatum(fw, "  Maximum Struct Size    : 0x%2.2x", entry->max_struct_size);
		fwts_log_info_verbatum(fw, "  Entry Point Revision   : 0x%2.2x", entry->revision);
		fwts_log_info_verbatum(fw, "  Formatted Area         : 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
			entry->formatted_area[0], entry->formatted_area[1],
			entry->formatted_area[2], entry->formatted_area[3],
			entry->formatted_area[4]);
	}
	if (type == FWTS_SMBIOS_DMI_LEGACY)
		fwts_log_info_verbatum(fw, "Legacy DMI Entry Point Structure:");

	/* Common to SMBIOS and SMBIOS_DMI_LEGACY */
	fwts_log_info_verbatum(fw, "  Intermediate Anchor    : %5.5s", (char *)entry->anchor_string);
	fwts_log_info_verbatum(fw, "  Intermediate Checksum  : 0x%2.2x", entry->intermediate_checksum);
	fwts_log_info_verbatum(fw, "  Structure Table Length : 0x%4.4x", entry->struct_table_length);
	fwts_log_info_verbatum(fw, "  Structure Table Address: 0x%8.8x", entry->struct_table_address);
	fwts_log_info_verbatum(fw, "  # of SMBIOS Structures : 0x%4.4x", entry->number_smbios_structures);
	fwts_log_info_verbatum(fw, "  SBMIOS BCD Revision    : %2.2x", entry->smbios_bcd_revision);
	if (entry->smbios_bcd_revision == 0)
		fwts_log_info_verbatum(fw, "    BCD Revision 00 indicates compliance with specification stated in Major/Minor Version.");
}

static int smbios_dmi_sane(fwts_framework *fw, fwts_smbios_entry *entry)
{
	uint8_t	*table, *ptr;
	uint8_t dmi_entry_length;
	uint8_t dmi_entry_type = 0;
	uint16_t i = 0;
	uint16_t table_length = entry->struct_table_length;
	int ret = FWTS_OK;

	ptr = table = fwts_mmap((off_t)entry->struct_table_address,
			  (size_t)table_length);
	if (table == FWTS_MAP_FAILED) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SMBIOSTableAddressNotMapped",
			"Cannot mmap SMBIOS tables from "
			"%8.8" PRIx32 "..%8.8" PRIx32 ".",
			entry->struct_table_address,
			entry->struct_table_address + table_length);
			return FWTS_ERROR;
	}

	for (i = 0; i < entry->number_smbios_structures; i++) {
		if (ptr > table + table_length) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"SMBIOSTableLengthTooSmall",
				"The size indicated by the SMBIOS table length is "
				"smaller than the DMI data.");
			ret = FWTS_ERROR;
			break;
		}

		dmi_entry_type = ptr[0];
		dmi_entry_length = ptr[1];

		if (dmi_entry_length < 4) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"SMBIOSIllegalTableEntry",
				"The size of a DMI entry %" PRIu16 " is illegal, "
				"DMI data is either wrong or the SMBIOS Table "
				"Pointer is pointing to the wrong memory region.", i);
			ret = FWTS_ERROR;
			break;
		}
		ptr += dmi_entry_length;

		/* Scan for end of DMI entry, must be 2 zero bytes */
		while (((ptr - table + 1) < table_length) &&
		       ((ptr[0] != 0) || (ptr[1] != 0)))
				ptr++;
		/* Skip over the two zero bytes */
		ptr += 2;
	}

	/* We found DMI end of table, are number of entries correct? */
	if ((dmi_entry_type == 127) && (i != entry->number_smbios_structures)) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SMBIOSNumberOfStructures",
			"The end of DMI table marker structure was found "
			"but only %d DMI structures were found. The SMBIOS "
			"entry table reported that there were %" PRIu16
			" DMI structures in the DMI table.",
			i, entry->number_smbios_structures);
		/* And table length can't be correct either */
		ret = FWTS_ERROR;
	}

	(void)fwts_munmap(table, (size_t)entry->struct_table_length);

	return ret;
}

static int smbios_test1(fwts_framework *fw)
{
	void *addr = 0;
	fwts_smbios_entry entry;
	fwts_smbios_type  type;
	uint16_t	  version;
	uint8_t checksum;
	static char warning[] =
		"This field is not checked by the kernel, and so will not affect the system, "
		"however it should be fixed to conform to the latest version of the "
		"System Management BIOS (SMBIOS) Reference Specification. ";

	fwts_log_info(fw,
		"This test tries to find and sanity check the SMBIOS "
		"data structures.");

	if ((addr = fwts_smbios_find_entry(fw, &entry, &type, &version)) == NULL) {
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SMBIOSNoEntryPoint",
			"Could not find SMBIOS Table Entry Point.");
		return FWTS_OK;
	}

	fwts_passed(fw, "Found SMBIOS Table Entry Point at %p", addr);
	smbios_dump_entry(fw, &entry, type);
	fwts_log_nl(fw);

	if (type == FWTS_SMBIOS) {
		checksum = fwts_checksum((uint8_t*)&entry, sizeof(fwts_smbios_entry));
		if (checksum != 0)
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SMBIOSBadChecksum",
				"SMBIOS Table Entry Point Checksum is 0x%2.2x, should be 0x%2.2x",
					entry.checksum, (uint8_t)(entry.checksum - checksum));
		else
			fwts_passed(fw, "SMBIOS Table Entry Point Checksum is valid.");

		if (entry.length != 0x1f) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"SMBIOSBadEntryLength",
				"SMBIOS Table Entry Point Length is 0x%2.2x, should be 0x1f", entry.length);
			fwts_advice(fw, "%s Note that version 2.1 of the specification incorrectly stated that the "
				"Entry Point Length should be 0x1e when it should be 0x1f.", warning);
		} else
			fwts_passed(fw, "SMBIOS Table Entry Point Length is valid.");
	}

	if (memcmp(entry.anchor_string, "_DMI_", 5) != 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"SMBIOSBadIntermediateAnchor",
			"SMBIOS Table Entry Intermediate Anchor String was '%5.5s' and should be '_DMI_'.",
			entry.anchor_string);
		fwts_advice(fw, "%s", warning);
	} else
		fwts_passed(fw, "SMBIOS Table Entry Intermediate Anchor String _DMI_ is valid.");

	/* Intermediate checksum for legacy DMI */
	checksum = fwts_checksum(((uint8_t*)&entry)+16, 15);
	if (checksum != 0)
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"SMBIOSBadChecksum",
			"SMBIOS Table Entry Point Intermediate Checksum is 0x%2.2x, should be 0x%2.2x",
				entry.intermediate_checksum,
				(uint8_t)(entry.intermediate_checksum - checksum));
	else
		fwts_passed(fw, "SMBIOS Table Entry Point Intermediate Checksum is valid.");

	if ((entry.struct_table_length > 0) && (entry.struct_table_address == 0)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"SMBIOSBadTableAddress",
			"SMBIOS Table Entry Structure Table Address is NULL and should be defined.");
		fwts_advice(fw,
			"The address of the SMBIOS Structure Tables must be defined if the "
			"length of these tables is defined.");
	} else {
		/*
		 * Now does the DMI table look sane? If not,
		 * the SMBIOS Structure Table could be bad
		 */
		if (smbios_dmi_sane(fw, &entry) == FWTS_OK)
			fwts_passed(fw, "SMBIOS Table Entry Structure Table Address and Length looks valid.");
	}

	return FWTS_OK;
}

static fwts_framework_minor_test smbios_tests[] = {
	{ smbios_test1, "Find and Check SMBIOS Table Entry Point." },
	{ NULL, NULL }
};

static fwts_framework_ops smbios_ops = {
	.description = "Check SMBIOS.",
	.minor_tests = smbios_tests
};

FWTS_REGISTER("smbios", &smbios_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ROOT_PRIV);

#endif
