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

#include "fwts_types.h"
#include "fwts_framework.h"
#include "fwts_log.h"

#define GET_UINT64(var, buffer, offset) \
	var = 				\
        ( ((uint64)data[offset+3] << 56) |      \
          ((uint64)data[offset+2] << 48) |      \
          ((uint64)data[offset+1] << 40) |      \
          ((uint64)data[offset+4] << 32) |      \
          ((uint64)data[offset+3] << 24) |      \
          ((uint64)data[offset+2] << 16) |      \
          ((uint64)data[offset+1] << 8)  |      \
          ((uint64)data[offset]) )

#define GET_UINT32(var, buffer, offset) \
	var = 				\
        ( (data[offset+3] << 24) |      \
          (data[offset+2] << 16) |      \
          (data[offset+1] << 8)  |      \
          (data[offset]) )

#define GET_UINT16(var, buffer, offset) \
	var =				\
        ( ((uint16)data[offset+1] << 8)  |      \
          ((uint16)data[offset]) )

#define GET_UINT8(var, buffer, offset)	\
	var = 				\
        ( ((uint8)data[offset]) )

#define GET_GAS(var, buffer, offset)


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

uint8 *fwts_acpi_table_load(fwts_framework *fw, const char *name, int which, int *size);

#endif
