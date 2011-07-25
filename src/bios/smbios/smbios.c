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

static int fwts_smbios_find_entry_in_uefi(fwts_framework *fw, fwts_smbios_entry *entry, uint32_t *addr)
{
	fwts_list *systab;
	fwts_list_link *item;
	int ret = FWTS_ERROR;

	*addr = 0;

	if (((systab = fwts_file_open_and_read("/sys/firmware/efi/systab")) == NULL) &&
	    ((systab = fwts_file_open_and_read("/proc/efi/systab")) == NULL)) {
		fwts_log_error(fw, "Cannot read UEFI systab file.");
		return FWTS_ERROR;
	}

	fwts_list_foreach(item, systab) {
		char *str = fwts_list_data(char *, item);
		if (strstr(str, "SMBIOS")) {
			char *ptr = strstr(str, "=");
			if (ptr) {
				*addr = (uint32_t)strtoul(ptr+1, NULL, 0);
				ret = FWTS_OK;
				break;
			}
		}
	}	
	fwts_list_free(systab, free);

	return ret;
}

static int fwts_smbios_find_entry_in_bios(fwts_framework *fw, fwts_smbios_entry *entry, uint32_t *addr)
{
	int ret = FWTS_ERROR;
	uint8_t *mem;
	int i;

        if ((mem = fwts_mmap(SMBIOS_REGION_START, SMBIOS_REGION_SIZE)) == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap SMBIOS region.");
		return FWTS_ERROR;
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

			*addr = SMBIOS_REGION_START + i;
			memcpy(entry, (void*)(mem + i), sizeof(fwts_smbios_entry));
			ret = FWTS_OK;
			break;
		}
	}

        (void)fwts_munmap(mem, SMBIOS_REGION_SIZE);

	return ret;
}

static int fwts_smbios_find_entry(fwts_framework *fw, fwts_smbios_entry *entry, uint32_t *addr)
{
	switch (fw->firmware_type) {
	case FWTS_FIRMWARE_BIOS:
		return fwts_smbios_find_entry_in_bios(fw, entry, addr);
		break;
	case FWTS_FIRMWARE_UEFI:
		return fwts_smbios_find_entry_in_uefi(fw, entry, addr);
		break;
	default:
		return FWTS_ERROR;
		break;
	}
}

static int smbios_test1(fwts_framework *fw)
{
	uint32_t addr = 0;
	fwts_smbios_entry entry;

	fwts_log_info(fw, "This test tries to find and sanity check the SMBIOS data structures.");
	if (fwts_smbios_find_entry(fw, &entry, &addr) != FWTS_OK)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, "SMBIOSNoEntryPoint", "Could not find SMBIOS Table Entry Point.");
	else {
		fwts_passed(fw, "Found SMBIOS Table Entry Point at 0x%8.8x", addr);
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
