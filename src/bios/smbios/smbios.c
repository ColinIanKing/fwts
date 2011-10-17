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
	uint8_t		intermedate_checksum;
	uint16_t	struct_table_length;
	uint32_t	struct_table_address;
	uint16_t	number_smbios_structures;
	uint8_t		smbios_bcd_revision;
}  __attribute__ ((packed)) fwts_smbios_entry;

void *fwts_smbios_find_entry_in_uefi(fwts_framework *fw, fwts_smbios_entry *entry)
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

void *fwts_smbios_find_entry_in_bios(fwts_framework *fw, fwts_smbios_entry *entry)
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
		    (*(mem+i+3) == '_')) {
			//fwts_smbios_entry *smbios;
			uint8_t checksum = 0;
			int j;

			for (j=i;j<i+16;j++)
				checksum += *(mem+j);

			//smbios = (fwts_smbios_entry*)(mem+i);

			addr = (void*)SMBIOS_REGION_START + i;
			memcpy(entry, (void*)(mem + i), sizeof(fwts_smbios_entry));
			break;
		}
	}

        (void)fwts_munmap(mem, SMBIOS_REGION_SIZE);

	return addr;
}

static void *fwts_smbios_find_entry(fwts_framework *fw, fwts_smbios_entry *entry)
{
	switch (fw->firmware_type) {
	case FWTS_FIRMWARE_BIOS:
		return fwts_smbios_find_entry_in_bios(fw, entry);
		break;
	case FWTS_FIRMWARE_UEFI:
		return fwts_smbios_find_entry_in_uefi(fw, entry);
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
	if ((addr = fwts_smbios_find_entry(fw, &entry)) == NULL)
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SMBIOSNoEntryPoint",
			"Could not find SMBIOS Table Entry Point.");
	else {
		fwts_passed(fw, "Found SMBIOS Table Entry Point at %p", addr);
		/* TODO: Dump out table */
	}

	return FWTS_OK;
}

static fwts_framework_minor_test smbios_tests[] = {
	{ smbios_test1, "Find and SMBIOS Table Entry Point." },
	{ NULL, NULL }
};

static fwts_framework_ops smbios_ops = {
	.description = "Check SMBIOS.",
	.minor_tests = smbios_tests
};

FWTS_REGISTER(smbios, &smbios_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
