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

#include "framework.h"
#include "log.h"

#define GET_UINT64(var, buffer, offset) \
	var = 				\
        ( ((u64)data[offset+3] << 56) |      \
          ((u64)data[offset+2] << 48) |      \
          ((u64)data[offset+1] << 40) |      \
          ((u64)data[offset+4] << 32) |      \
          ((u64)data[offset+3] << 24) |      \
          ((u64)data[offset+2] << 16) |      \
          ((u64)data[offset+1] << 8)  |      \
          ((u64)data[offset]) )

#define GET_UINT32(var, buffer, offset) \
	var = 				\
        ( (data[offset+3] << 24) |      \
          (data[offset+2] << 16) |      \
          (data[offset+1] << 8)  |      \
          (data[offset]) )

#define GET_UINT16(var, buffer, offset) \
	var =				\
        ( ((u16)data[offset+1] << 8)  |      \
          ((u16)data[offset]) )

#define GET_UINT8(var, buffer, offset)	\
	var = 				\
        ( ((u8)data[offset]) )

#define GET_GAS(var, buffer, offset)

typedef unsigned long long 	u64;
typedef	unsigned long		u32;
typedef	unsigned short		u16;
typedef unsigned char		u8;

typedef struct {
	u8 	address_space_id;
	u8	register_bit_width;
        u8 	register_bit_offset;
        u8 	access_width;
        u64 	address;
} fwts_gas;

typedef struct {
	char		signature[4];
	u32		length;
	u8		revision;
	u8		checksum;
	char		oem_id[6];
	char		oem_tbl_id[8];
	u32		oem_revision;
	char		creator_id[4];
	u32		creator_revision;
} fwts_acpi_table_header;

unsigned char *fwts_get_acpi_table(fwts_framework *fw, const char *name, unsigned long *size);

#endif
