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

#ifndef __FWTS_ACPI_H__
#define __FWTS_ACPI_H__

#define FWTS_ACPI_TABLES_PATH   "/sys/firmware/acpi/tables"

#include "fwts_types.h"
#include "fwts_framework.h"
#include "fwts_log.h"

#define FWTS_GET_UINT64(var, buffer, offset) 	\
	var = 					\
        ( ((uint64)data[offset+7] << 56) |      \
          ((uint64)data[offset+6] << 48) |      \
          ((uint64)data[offset+5] << 40) |      \
          ((uint64)data[offset+4] << 32) |      \
          ((uint64)data[offset+3] << 24) |      \
          ((uint64)data[offset+2] << 16) |      \
          ((uint64)data[offset+1] << 8)  |      \
          ((uint64)data[offset]) )

#define FWTS_GET_UINT32(var, buffer, offset) 	\
	var = 					\
        ( (data[offset+3] << 24) |      	\
          (data[offset+2] << 16) |      	\
          (data[offset+1] << 8)  |      	\
          (data[offset]) )

#define FWTS_GET_UINT16(var, buffer, offset) 	\
	var =					\
        ( ((uint16)data[offset+1] << 8)  |      \
          ((uint16)data[offset]) )

#define FWTS_GET_UINT8(var, buffer, offset)	\
	var = 					\
        ( ((uint8)data[offset]) )

#define FWTS_GET_GAS(var, buffer, offset)					\
	do { 									\
		FWTS_GET_UINT8(var.address_space_id, buffer, offset);		\
		FWTS_GET_UINT8(var.register_bit_width, buffer, (offset+1));	\
		FWTS_GET_UINT8(var.register_bit_offset, buffer, (offset+2));	\
		FWTS_GET_UINT8(var.access_width, buffer, (offset+3));		\
		FWTS_GET_UINT64(var.address, buffer, (offset+4));		\
	} while (0)

typedef struct {
	uint8 	address_space_id;
	uint8	register_bit_width;
        uint8 	register_bit_offset;
        uint8 	access_width;
        uint64 	address;
} fwts_gas;

typedef struct {
	char		signature[4];
	uint32		length;
	uint8		revision;
	uint8		checksum;
	char		oem_id[6];
	char		oem_tbl_id[8];
	uint32		oem_revision;
	char		creator_id[4];
	uint32		creator_revision;
} fwts_acpi_table_header;

typedef struct {
	uint8		type;
	uint8		length;
} fwts_acpi_sub_table_header;

typedef struct {
	fwts_acpi_table_header	header;
	uint32		lapic_address;
	uint32		flags;
} fwts_acpi_table_madt;

typedef enum {
        FWTS_ACPI_MADT_LOCAL_APIC = 0,
        FWTS_ACPI_MADT_IO_APIC,
        FWTS_ACPI_MADT_INTERRUPT_OVERRIDE,
        FWTS_ACPI_MADT_NMI_SOURCE,
        FWTS_ACPI_MADT_LOCAL_APIC_NMI,
        FWTS_ACPI_MADT_LOCAL_APIC_OVERRIDE,
        FWTS_ACPI_MADT_IO_SAPIC,
        FWTS_ACPI_MADT_LOCAL_SAPIC,
        FWTS_ACPI_MADT_INTERRUPT_SOURCE,
        FWTS_ACPI_MADT_LOCAL_X2APIC,
        FWTS_ACPI_MADT_LOCAL_X2APIC_NMI,
        FWTS_ACPI_MADT_RESERVED
} fwts_acpi_madt_type;

typedef struct {
	fwts_acpi_sub_table_header	header;
	uint8		processor_id;
	uint8		lapic_id;
	uint32		lapic_flags;
} fwts_acpi_madt_local_apic;

typedef struct {
	fwts_acpi_sub_table_header	header;
	uint8		io_apic_id;
	uint8		reserved;
	uint32		apic_phys_address;
	uint32		global_irq_base;
} fwts_acpi_madt_io_apic;

typedef struct {
	fwts_acpi_sub_table_header	header;
	uint8		bus;
	uint8		source_irq;
	uint32		global_irq;
	uint16		int_flags;
} fwts_acpi_madt_interrupt_override;

uint8 *fwts_acpi_table_load(fwts_framework *fw, const char *name, const int which, int *size);

#endif
