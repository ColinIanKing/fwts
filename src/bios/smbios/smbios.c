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

#define SMBIOS_REGION_START	(0x000e0000)
#define SMBIOS_REGION_END  	(0x000fffff)
#define SMBIOS_REGION_SIZE	(SMBIOS_REGION_END - SMBIOS_REGION_START)

typedef struct {
	uint8_t		signature[4];
	uint8_t		checksum;
	uint8_t		length;
	uint8_t		major_version;
	uint8_t		minor_version;
	uint16_t	max_struct_size;
	uint8_t		revision;
	uint8_t		formatted_area[5];
	uint8_t		anchor_string[5];
	uint8_t		intermediate_checksum;
	uint16_t	struct_table_length;
	uint32_t	struct_table_address;
	uint16_t	number_smbios_structures;
	uint8_t		smbios_bcd_revision;
}  __attribute__ ((packed)) fwts_smbios_entry;

void *smbios_find_entry_in_uefi(fwts_framework *fw, fwts_smbios_entry *entry)
{
	void *addr;
	fwts_smbios_entry *mapped_entry;

	if ((addr = fwts_scan_efi_systab("SMBIOS")) != NULL) {
		if ((mapped_entry = fwts_mmap((off_t)addr, sizeof(fwts_smbios_entry))) == FWTS_MAP_FAILED) {
			fwts_log_error(fw, "Cannot mmap SMBIOS entry at %p\n", addr);
			return NULL;
		}
		*entry = *mapped_entry;
		(void)fwts_munmap(mapped_entry, sizeof(fwts_smbios_entry));
	}
	return addr;
}

void *smbios_find_entry_in_bios(fwts_framework *fw, fwts_smbios_entry *entry)
{
	uint8_t *mem;
	void 	*addr = NULL;
	int 	i;

        if ((mem = fwts_mmap(SMBIOS_REGION_START, SMBIOS_REGION_SIZE)) == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap SMBIOS region.");
		return NULL;
	}

	for (i=0; i<SMBIOS_REGION_SIZE; i+= 16) {
		if ((*(mem+i)   == '_') &&
		    (*(mem+i+1) == 'S') &&
		    (*(mem+i+2) == 'M') &&
		    (*(mem+i+3) == '_') && 
		    (fwts_checksum(mem + i, 16) == 0)) {
			addr = (void*)SMBIOS_REGION_START + i;
			memcpy(entry, (void*)(mem + i), sizeof(fwts_smbios_entry));
			break;
		}
	}

        (void)fwts_munmap(mem, SMBIOS_REGION_SIZE);

	return addr;
}

static void smbios_dump_entry(fwts_framework *fw, fwts_smbios_entry *entry)
{
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
	fwts_log_info_verbatum(fw, "  Intermediate Anchor    : %5.5s", (char *)entry->anchor_string);
	fwts_log_info_verbatum(fw, "  Intermediate Checksum  : 0x%2.2x", entry->intermediate_checksum);
	fwts_log_info_verbatum(fw, "  Structure Table Length : 0x%4.4x", entry->struct_table_length);
	fwts_log_info_verbatum(fw, "  Structure Table Address: 0x%8.8x", entry->struct_table_address);
	fwts_log_info_verbatum(fw, "  # of SMBIOS Structures : 0x%4.4x", entry->number_smbios_structures);
	fwts_log_info_verbatum(fw, "  SBMIOS BCD Revision    : %2.2x", entry->smbios_bcd_revision);
	if (entry->smbios_bcd_revision == 0) {
		fwts_log_info_verbatum(fw, "    BCD Revision 00 indicates compliance with specification stated in Major/Minor Version.");
	}
}

static void *smbios_find_entry(fwts_framework *fw, fwts_smbios_entry *entry)
{
	switch (fw->firmware_type) {
	case FWTS_FIRMWARE_BIOS:
		return smbios_find_entry_in_bios(fw, entry);
		break;
	case FWTS_FIRMWARE_UEFI:
		return smbios_find_entry_in_uefi(fw, entry);
		break;
	default:
		return NULL;
		break;
	}
}

static int smbios_test1(fwts_framework *fw)
{
	void *addr = 0;
	fwts_smbios_entry entry;

	fwts_log_info(fw,
		"This test tries to find and sanity check the SMBIOS "
		"data structures.");
	if ((addr = smbios_find_entry(fw, &entry)) == NULL)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SMBIOSNoEntryPoint",
			"Could not find SMBIOS Table Entry Point.");
	else {
		static char warning[] = 
			"This field is not checked by the kernel, and so will not affect the system, "
			"however it should be fixed to conform to the latest version of the "
			"System Management BIOS (SMBIOS) Reference Specification. ";

		fwts_passed(fw, "Found SMBIOS Table Entry Point at %p", addr);

		smbios_dump_entry(fw, &entry);
		
		if (entry.length != 0x1f) {
			fwts_failed(fw, LOG_LEVEL_LOW,
				"SMBIOSBadEntryLength",
				"SMBIOS Table Entry Point Length is 0x%2.2x, should be 0x1f", entry.length);
			fwts_advice(fw, "%s Note that version 2.1 of the specification incorrectly stated that the "
				"Entry Point Length should be 0x1e when it should be 0x1f.", warning);
		}
		if (memcmp(entry.anchor_string, "_DMI_", 5) != 0) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"SMBIOSBadIntermediateAnchor", 
				"SMBIOS Table Entry Intermediate Anchor String was '%5.5s' and should be '_DMI_'.", 
					entry.anchor_string);
			fwts_advice(fw, "%s", warning);
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
