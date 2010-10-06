/*
 * Copyright (C) 2010 Canonical
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

#ifndef __FWTS_ACPI_TABLES_H__
#define __FWTS_ACPI_TABLES_H__

#include "fwts.h"

typedef struct {
	char    name[5];
	void    *data;
	int     length;
	int 	which;
	uint64_t addr;
} fwts_acpi_table_info;

void fwts_acpi_load_tables(fwts_framework *fw);
void fwts_acpi_free_tables(void);

fwts_acpi_table_info *fwts_acpi_find_table(fwts_framework *fw, const char *name, const int which);
fwts_acpi_table_info *fwts_acpi_find_table_by_addr(fwts_framework *fw, const uint64_t addr);
fwts_acpi_table_info *fwts_acpi_get_table(fwts_framework *fw, const int index);

#endif
