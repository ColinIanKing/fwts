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
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fwts.h"

static enum firmware_type firmware_type;
static bool firmware_type_valid;

/*
 *  fwts_memory_map_entry_compare()
 *	callback used to sort memory_map entries on start address
 */
int fwts_firmware_detect(void)
{
	struct stat statbuf;

	if (firmware_type_valid)
		return firmware_type;

	if (stat("/sys/firmware/efi", &statbuf)) {
		/* No UEFI, Assume BIOS */
		firmware_type = FWTS_FIRMWARE_BIOS;
	} else {
		firmware_type = FWTS_FIRMWARE_UEFI;
	}

	firmware_type_valid = true;
	return firmware_type;
}

int fwts_firmware_features(void)
{
	int features = 0;

	/* we just have static feature definitions for now */
	switch (fwts_firmware_detect()) {
	case FWTS_FIRMWARE_BIOS:
	case FWTS_FIRMWARE_UEFI:
		features = FWTS_FW_FEATURE_ACPI;
		break;
	default:
		break;
	}

	return features;
}
