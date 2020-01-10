/*
 * Copyright (C) 2010-2020 Canonical
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

#if defined(FWTS_HAS_ACPI)

#define ACPI_MAX_TABLES		(128)

typedef enum {
	FWTS_ACPI_TABLE_FROM_FIRMWARE,	/* directly from firmware */
	FWTS_ACPI_TABLE_FROM_FILE,	/* loaded from file, e.g. from acpidump */
	FWTS_ACPI_TABLE_FROM_FIXUP	/* auto-generated fixup by fwts */
} fwts_acpi_table_provenance;

typedef struct {
	char	   name[5];
	void       *data;
	size_t	   length;
	uint32_t   which;
	uint32_t   index;
	bool	   has_aml;
	uint64_t   addr;
	fwts_acpi_table_provenance provenance;
} fwts_acpi_table_info;

int fwts_acpi_load_tables(fwts_framework *fw);
int fwts_acpi_free_tables(void);

int fwts_acpi_find_table(fwts_framework *fw, const char *name, const uint32_t which, fwts_acpi_table_info **info);
int fwts_acpi_find_table_by_addr(fwts_framework *fw, const uint64_t addr, fwts_acpi_table_info **info);
int fwts_acpi_get_table(fwts_framework *fw, const uint32_t index, fwts_acpi_table_info **info);
bool fwts_acpi_obj_find(fwts_framework *fw, const char *obj_name);

fwts_bool fwts_acpi_is_reduced_hardware(const fwts_acpi_table_fadt *fadt);

void fwts_acpi_reserved_zero_check(fwts_framework *fw, const char *table, const char *field, uint64_t value, uint8_t size, bool *passed);
void fwts_acpi_reserved_zero_array_check(fwts_framework *fw, const char *table, const char *field, uint8_t* data, uint8_t length, bool *passed);
void fwts_acpi_reserved_bits_check(fwts_framework *fw, const char *table, const char *field, uint64_t value, uint8_t size, uint8_t min, uint8_t max, bool *passed);
void fwts_acpi_reserved_type_check(fwts_framework *fw, const char *table, uint8_t value, uint8_t min, uint8_t reserved, bool *passed);
bool fwts_acpi_table_length_check(fwts_framework *fw, const char *table, uint32_t length, uint32_t size);
bool fwts_acpi_structure_length_check(fwts_framework *fw, const char *table, uint8_t subtable_type, uint32_t subtable_length, uint32_t size);

uint32_t fwts_get_acpi_version(fwts_framework *fw);

#endif

#endif
