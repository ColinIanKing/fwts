/*
 * Copyright (C) 2010-2021 Canonical
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

#define fwts_acpi_revision_check(table, actual, must_be, passed) \
	fwts_acpi_fixed_value_check(fw, LOG_LEVEL_HIGH, table, "Revision", actual, must_be, passed)

#define fwts_acpi_reserved_bits_check(table, field, value, min, max, passed) \
	fwts_acpi_reserved_bits_check_(fw, table, field, value, sizeof(value), min, max, passed)

#define fwts_acpi_reserved_zero_check(table, field, value, passed) \
	fwts_acpi_reserved_zero_check_(fw, table, field, value, sizeof(value), passed)

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

int acpi_table_generic_init(fwts_framework *fw, char *name, fwts_acpi_table_info **table);

#define acpi_table_init(name, table)				\
static int name ## _init (fwts_framework *fw)			\
{								\
	return acpi_table_generic_init(fw, # name, table);	\
}

int fwts_acpi_load_tables(fwts_framework *fw);
int fwts_acpi_free_tables(void);

int fwts_acpi_find_table(fwts_framework *fw, const char *name, const uint32_t which, fwts_acpi_table_info **info);
int fwts_acpi_find_table_by_addr(fwts_framework *fw, const uint64_t addr, fwts_acpi_table_info **info);
int fwts_acpi_get_table(fwts_framework *fw, const uint32_t index, fwts_acpi_table_info **info);
bool fwts_acpi_obj_find(fwts_framework *fw, const char *obj_name);

fwts_bool fwts_acpi_is_reduced_hardware(fwts_framework *fw);

void fwts_acpi_reserved_zero_check_(fwts_framework *fw, const char *table, const char *field, uint64_t value, uint8_t size, bool *passed);
void fwts_acpi_reserved_zero_array_check(fwts_framework *fw, const char *table, const char *field, uint8_t* data, uint8_t length, bool *passed);
void fwts_acpi_reserved_bits_check_(fwts_framework *fw, const char *table, const char *field, uint64_t value, uint8_t size, uint8_t min, uint8_t max, bool *passed);
void fwts_acpi_reserved_type_check(fwts_framework *fw, const char *table, uint8_t value, uint8_t min, uint8_t reserved, bool *passed);
bool fwts_acpi_table_length_check(fwts_framework *fw, const char *table, uint32_t length, uint32_t size);
bool fwts_acpi_structure_length_check(fwts_framework *fw, const char *table, uint8_t subtable_type, uint32_t subtable_length, uint32_t size);
bool fwts_acpi_structure_length_zero_check(fwts_framework *fw, const char *table, uint16_t length, uint32_t offset);
bool fwts_acpi_structure_range_check(fwts_framework *fw, const char *table, uint32_t table_length, uint32_t offset);
void fwts_acpi_fixed_value_check(fwts_framework *fw, fwts_log_level level, const char *table, const char *field, uint8_t actual, uint8_t must_be, bool *passed);
void fwts_acpi_space_id_check(fwts_framework *fw, const char *table, const char *field, bool *passed, uint8_t actual, uint8_t num_type, ...);

uint32_t fwts_get_acpi_version(fwts_framework *fw);

#endif

#endif
