/*
 * Copyright (C) 2010-2015 Canonical
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

#define FWTS_SMBIOS_CHASSIS_INVALID			(0x00)
#define FWTS_SMBIOS_CHASSIS_OTHER			(0X01)
#define FWTS_SMBIOS_CHASSIS_UNKNOWN			(0x02)
#define FWTS_SMBIOS_CHASSIS_DESKTOP			(0x03)
#define FWTS_SMBIOS_CHASSIS_LOW_PROFILE_DESKTOP		(0x04)
#define FWTS_SMBIOS_CHASSIS_PIZZA_BOX			(0x05)
#define FWTS_SMBIOS_CHASSIS_MINI_TOWER			(0x06)
#define FWTS_SMBIOS_CHASSIS_TOWER			(0x07)
#define FWTS_SMBIOS_CHASSIS_PORTABLE			(0x08)
#define FWTS_SMBIOS_CHASSIS_LAPTOP			(0x09)
#define FWTS_SMBIOS_CHASSIS_NOTEBOOK			(0x0A)
#define FWTS_SMBIOS_CHASSIS_HANDHELD			(0x0B)
#define FWTS_SMBIOS_CHASSIS_DOCKING_STATION		(0x0C)
#define FWTS_SMBIOS_CHASSIS_ALL_IN_ONE			(0x0D)
#define FWTS_SMBIOS_CHASSIS_SUB_NOTEBOOK		(0x0E)
#define FWTS_SMBIOS_CHASSIS_SPACE_SAVING		(0x0F)
#define FWTS_SMBIOS_CHASSIS_LUNCH_BOX			(0x10)
#define FWTS_SMBIOS_CHASSIS_MAIN_SERVER_CHASSIS		(0x11)
#define FWTS_SMBIOS_CHASSIS_EXPANISON_CHASSIS		(0x12)
#define FWTS_SMBIOS_CHASSIS_SUB_CHASSIS			(0x13)
#define FWTS_SMBIOS_CHASSIS_BUS_EXPANSION_CHASSIS	(0x14)
#define FWTS_SMBIOS_CHASSIS_PERIPHERAL_CHASSIS		(0x15)
#define FWTS_SMBIOS_CHASSIS_RAID_CHASSIS		(0x16)
#define FWTS_SMBIOS_CHASSIS_RACK_MOUNT_CHASSIS		(0x17)
#define FWTS_SMBIOS_CHASSIS_SEALED_CASE_PC		(0x18)
#define FWTS_SMBIOS_CHASSIS_MULTI_SYSTEM_CHASSIS	(0x19)
#define FWTS_SMBIOS_CHASSIS_COMPACT_PCI			(0x1A)
#define FWTS_SMBIOS_CHASSIS_ADVANCED_TCA		(0x1B)
#define FWTS_SMBIOS_CHASSIS_BLADE			(0x1C)
#define FWTS_SMBIOS_CHASSIS_BLADE_ENCLOSURE		(0x1D)
#define FWTS_SMBIOS_CHASSIS_TABLET			(0x1E)
#define FWTS_SMBIOS_CHASSIS_CONVERTIBLE			(0x1F)
#define FWTS_SMBIOS_CHASSIS_DETACHABLE			(0x20)

typedef enum {
	FWTS_SMBIOS_UNKNOWN = -1,
	FWTS_SMBIOS_DMI_LEGACY = 0,
	FWTS_SMBIOS = 1
} fwts_smbios_type;

typedef struct {
	uint8_t  type;
	uint8_t  length;
	uint16_t handle;
	uint8_t  *data;
} fwts_dmi_header;

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

/*
 * From System Management BIOS (SMBIOS) Reference Specification 3.0.0
 */
typedef struct {
	uint8_t		signature[5];
	uint8_t		checksum;
	uint8_t		length;
	uint8_t		major_version;
	uint8_t		minor_version;
	uint8_t		docrev;
	uint8_t		revision;
	uint8_t		reserved;
	uint32_t	struct_table_max_size;
	uint64_t	struct_table_address;
}  __attribute__ ((packed)) fwts_smbios30_entry;

void *fwts_smbios_find_entry(fwts_framework *fw, fwts_smbios_entry *entry, fwts_smbios_type *type, uint16_t *version);

#endif
