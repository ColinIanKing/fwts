/*
 * Copyright (C) 2010-2014 Canonical
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

#if defined(FWTS_ARCH_INTEL) || defined(FWTS_ARCH_AARCH64)
/*
 *  fwts_smbios_find_entry_uefi()
 *	find SMBIOS structure table entry from UEFI systab
 */
static void *fwts_smbios_find_entry_uefi(fwts_framework *fw, fwts_smbios_entry *entry, fwts_smbios_type *type)
{
	void *addr;
	fwts_smbios_entry *mapped_entry;

	if ((addr = fwts_scan_efi_systab("SMBIOS")) != NULL) {
		if ((mapped_entry = fwts_mmap((off_t)addr, sizeof(fwts_smbios_entry))) == FWTS_MAP_FAILED) {
			fwts_log_error(fw, "Cannot mmap SMBIOS entry at %p\n", addr);
			return NULL;
		}
		*entry = *mapped_entry;
		*type  = FWTS_SMBIOS;
		(void)fwts_munmap(mapped_entry, sizeof(fwts_smbios_entry));
	}
	return addr;
}

/*
 *  fwts_smbios_find_entry_bios()
 *	find SMBIOS structure table entry by scanning memory
 */
static void *fwts_smbios_find_entry_bios(fwts_framework *fw, fwts_smbios_entry *entry, fwts_smbios_type *type)
{
	uint8_t *mem;
	void 	*addr = NULL;
	int 	i;

        if ((mem = fwts_mmap(FWTS_SMBIOS_REGION_START, FWTS_SMBIOS_REGION_SIZE)) == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap SMBIOS region.");
		return NULL;
	}

	for (i=0; i<FWTS_SMBIOS_REGION_SIZE; i+= 16) {
		/* SMBIOS entry point */
		if ((*(mem+i)   == '_') &&
		    (*(mem+i+1) == 'S') &&
		    (*(mem+i+2) == 'M') &&
		    (*(mem+i+3) == '_') &&
		    (fwts_checksum(mem + i, 16) == 0)) {
			addr = (void*)FWTS_SMBIOS_REGION_START + i;
			memcpy(entry, (void*)(mem + i), sizeof(fwts_smbios_entry));
			*type  = FWTS_SMBIOS;
			break;
		}
		/* Legacy DMI entry point */
		if ((*(mem+i)   == '_') &&
		    (*(mem+i+1) == 'D') &&
		    (*(mem+i+2) == 'M') &&
		    (*(mem+i+3) == 'I') &&
		    (*(mem+i+4) == '_') &&
		    (fwts_checksum(mem + i, 15) == 0)) {
			memset(entry, 0, 16);
			addr = (void*)FWTS_SMBIOS_REGION_START + i;
			memcpy(16+((void*)entry), (void*)(mem + i), 15);
			*type = FWTS_SMBIOS_DMI_LEGACY;
			break;
		}
	}

        (void)fwts_munmap(mem, FWTS_SMBIOS_REGION_SIZE);

	return addr;
}

/*
 *  fwts_smbios_find_entry()
 *	find SMBIOS structure table entry
 */
void *fwts_smbios_find_entry(fwts_framework *fw,
	fwts_smbios_entry *entry,
	fwts_smbios_type  *type,
	uint16_t	  *version)
{
	void *addr;
	*type = FWTS_SMBIOS_UNKNOWN;

	/* Check EFI first */
	if ((addr = fwts_smbios_find_entry_uefi(fw, entry, type)) == NULL) {
		/* Failed? then scan memory */
		addr = fwts_smbios_find_entry_bios(fw, entry, type);
		if (addr) {
			switch (*type) {
			case FWTS_SMBIOS:
				*version = (entry->major_version << 8) +
				           (entry->minor_version & 0xff);
				break;
			case FWTS_SMBIOS_DMI_LEGACY:
				*version = ((entry->smbios_bcd_revision & 0xf0) << 4) +
				 	    (entry->smbios_bcd_revision & 0x0f);
				break;
			default:
				break;
			}
		}
	}
	return addr;
}

#else
/*
 *  fwts_smbios_find_entry()
 *	find SMBIOS structure table entry, currently void for non-x86 specific code
 */
void *fwts_smbios_find_entry(fwts_framework *fw,
	fwts_smbios_entry *entry,
	fwts_smbios_type  *type,
	uint16_t	  *version)
{
	FWTS_UNUSED(fw);

	/* Return a dummy values */
	memset(entry, 0, sizeof(fwts_smbios_entry));
	*type = FWTS_SMBIOS_UNKNOWN;
	*version = 0;

	return NULL;
}
#endif
