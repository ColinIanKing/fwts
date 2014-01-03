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
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fwts.h"

/*
 *  fwts_memory_map_entry_compare()
 *	callback used to sort memory_map entries on start address
 */
int fwts_firmware_detect(void)
{
	struct stat statbuf;

	if (stat("/sys/firmware/efi", &statbuf))
		return FWTS_FIRMWARE_BIOS;		/* No UEFI, Assume BIOS */
	else
		return FWTS_FIRMWARE_UEFI;
}
