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

static void smbios_dump_entry(fwts_framework *fw, fwts_smbios_entry *entry, fwts_smbios_type type)
{	
	if (type == FWTS_SMBIOS) {
		fwts_log_info_verbatum(fw, "SMBIOS Entry Point Stucture:");
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
		fwts_log_info_verbatum(fw, "Legacy DMI Entry Point Stucture:");

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

static int smbios_test1(fwts_framework *fw)
{
	void *addr = 0;
	fwts_smbios_entry entry;
	fwts_smbios_type  type;
	uint16_t	  version;

	fwts_log_info(fw,
		"This test tries to find and sanity check the SMBIOS "
		"data structures.");
	if ((addr = fwts_smbios_find_entry(fw, &entry, &type, &version)) == NULL)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SMBIOSNoEntryPoint",
			"Could not find SMBIOS Table Entry Point.");
	else {
		uint8_t checksum;
		static char warning[] = 
			"This field is not checked by the kernel, and so will not affect the system, "
			"however it should be fixed to conform to the latest version of the "
			"System Management BIOS (SMBIOS) Reference Specification. ";

		fwts_passed(fw, "Found SMBIOS Table Entry Point at %p", addr);

		smbios_dump_entry(fw, &entry, type);
		
		if (type == FWTS_SMBIOS) {
			checksum = fwts_checksum((uint8_t*)&entry, sizeof(fwts_smbios_entry));
			if (checksum != 0) {
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"SMBIOSBadChecksum",
					"SMBIOS Table Entry Point Checksum is 0x%2.2x, should be 0x%2.2x",
						entry.checksum, (uint8_t)(entry.checksum - checksum));
			}
			if (entry.length != 0x1f) {
				fwts_failed(fw, LOG_LEVEL_LOW,
					"SMBIOSBadEntryLength",
					"SMBIOS Table Entry Point Length is 0x%2.2x, should be 0x1f", entry.length);
				fwts_advice(fw, "%s Note that version 2.1 of the specification incorrectly stated that the "
					"Entry Point Length should be 0x1e when it should be 0x1f.", warning);
			}
		}
		if (memcmp(entry.anchor_string, "_DMI_", 5) != 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SMBIOSBadIntermediateAnchor", 
				"SMBIOS Table Entry Intermediate Anchor String was '%5.5s' and should be '_DMI_'.", 
				entry.anchor_string);
			fwts_advice(fw, "%s", warning);
		}
		/* Intermediate checksum for legacy DMI */
		checksum = fwts_checksum(((uint8_t*)&entry)+16, 15);
		if (checksum != 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SMBIOSBadChecksum",
				"SMBIOS Table Entry Point Intermediate Checksum is 0x%2.2x, should be 0x%2.2x",
					entry.intermediate_checksum,
					(uint8_t)(entry.intermediate_checksum - checksum));
		}
		if ((entry.struct_table_length > 0) && (entry.struct_table_address == 0)) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SMBIOSBadTableAddress",
				"SMBIOS Table Entry Structure Table Address is NULL and should be defined.");
			fwts_advice(fw,
				"The address of the SMBIOS Structure Tables must be defined if the "
				"length of these tables is defined.");
		}
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

FWTS_REGISTER(smbios, &smbios_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
