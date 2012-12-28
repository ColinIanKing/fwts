/*
 * Copyright (C) 2010-2013 Canonical
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

typedef enum {
	FWTS_ACPI_TABLE_FROM_FIRMWARE,	/* directly from firmware */
	FWTS_ACPI_TABLE_FROM_FILE,	/* loaded from file, e.g. from acpidump */
	FWTS_ACPI_TABLE_FROM_FIXUP,	/* auto-generated fixup by fwts */
} fwts_acpi_table_provenance;

typedef struct {
	char    name[5];
	const void *data;
	size_t	length;
	int 	which;
	uint64_t addr;
	fwts_acpi_table_provenance provenance;
} fwts_acpi_table_info;

int fwts_acpi_load_tables(fwts_framework *fw);
int fwts_acpi_free_tables(void);

int fwts_acpi_find_table(fwts_framework *fw, const char *name, const int which, fwts_acpi_table_info **info);
int fwts_acpi_find_table_by_addr(fwts_framework *fw, const uint64_t addr, fwts_acpi_table_info **info);
int fwts_acpi_get_table(fwts_framework *fw, const int index, fwts_acpi_table_info **info);
uint8_t fwts_acpi_checksum(const uint8_t *data, const int length);

#endif
