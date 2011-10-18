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

#ifndef __FWTS_SMBIOS_H__
#define __FWTS_SMBIOS_H__

#include <stdint.h>
#include "fwts.h"

#define FWTS_SMBIOS_REGION_START     (0x000e0000)
#define FWTS_SMBIOS_REGION_END       (0x000fffff)
#define FWTS_SMBIOS_REGION_SIZE      (FWTS_SMBIOS_REGION_END - FWTS_SMBIOS_REGION_START)

typedef enum {
	FWTS_SMBIOS_UNKNOWN = -1,
	FWTS_SMBIOS_DMI_LEGACY = 0,
	FWTS_SMBIOS = 1,
} fwts_smbios_type;

/*
 * From System Management BIOS (SMBIOS) Reference Specification
 * http://dmtf.org/standards/smbios
 */
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

void *fwts_smbios_find_entry(fwts_framework *fw, fwts_smbios_entry *entry, fwts_smbios_type *type, uint16_t *version);

#endif
