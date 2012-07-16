/*
 * Copyright (C) 2010-2012 Canonical
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "fwts.h"

char *fwts_acpi_fadt_preferred_pm_profile[] = {
	"Unspecified",
	"Desktop",
	"Mobile",
	"Workstation",
	"Enterprise Server",
	"SOHO Server",
	"Appliance PC",
	"Performance Server",
	"Tablet",
};

/*
 *  fwts_acpi_table_get_header()
 * 	copy into ACPI table header from raw data
 */
void fwts_acpi_table_get_header(fwts_acpi_table_header *hdr, uint8_t *data)
{
	memcpy(hdr, data, sizeof(fwts_acpi_table_header));
}
