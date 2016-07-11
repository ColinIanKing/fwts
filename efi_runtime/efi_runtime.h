/*
 * EFI Runtime driver
 *
 * Copyright(C) 2012-2016 Canonical Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _EFI_RUNTIME_H_
#define _EFI_RUNTIME_H_

#include <linux/efi.h>

struct efi_getvariable {
	uint16_t	*variable_name;
	efi_guid_t	*vendor_guid;
	uint32_t	*attributes;
	uint64_t	*data_size;
	void		*data;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_setvariable {
	uint16_t	*variable_name;
	efi_guid_t	*vendor_guid;
	uint32_t	attributes;
	uint64_t	data_size;
	void		*data;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_getnextvariablename {
	uint64_t	*variable_name_size;
	uint16_t	*variable_name;
	efi_guid_t	*vendor_guid;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_queryvariableinfo {
	uint32_t	attributes;
	uint64_t	*maximum_variable_storage_size;
	uint64_t	*remaining_variable_storage_size;
	uint64_t	*maximum_variable_size;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_gettime {
	efi_time_t		*time;
	efi_time_cap_t		*capabilities;
	uint64_t		*status;
} __attribute__ ((packed));

struct efi_settime {
	efi_time_t		*time;
	uint64_t		*status;
} __attribute__ ((packed));

struct efi_getwakeuptime {
	uint8_t		*enabled;
	uint8_t		*pending;
	efi_time_t	*time;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_setwakeuptime {
	uint8_t		enabled;
	efi_time_t	*time;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_getnexthighmonotoniccount {
	uint32_t	*high_count;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_querycapsulecapabilities {
	efi_capsule_header_t	**capsule_header_array;
	uint64_t		capsule_count;
	uint64_t		*maximum_capsule_size;
	int			*reset_type;
	uint64_t		*status;
} __attribute__ ((packed));

/* ioctl calls that are permitted to the /dev/efi_runtime interface. */
#define EFI_RUNTIME_GET_VARIABLE \
	_IOWR('p', 0x01, struct efi_getvariable)
#define EFI_RUNTIME_SET_VARIABLE \
	_IOW('p', 0x02, struct efi_setvariable)

#define EFI_RUNTIME_GET_TIME \
	_IOR('p', 0x03, struct efi_gettime)
#define EFI_RUNTIME_SET_TIME \
	_IOW('p', 0x04, struct efi_settime)

#define EFI_RUNTIME_GET_WAKETIME \
	_IOR('p', 0x05, struct efi_getwakeuptime)
#define EFI_RUNTIME_SET_WAKETIME \
	_IOW('p', 0x06, struct efi_setwakeuptime)

#define EFI_RUNTIME_GET_NEXTVARIABLENAME \
	_IOWR('p', 0x07, struct efi_getnextvariablename)

#define EFI_RUNTIME_QUERY_VARIABLEINFO \
	_IOR('p', 0x08, struct efi_queryvariableinfo)

#define EFI_RUNTIME_GET_NEXTHIGHMONOTONICCOUNT \
	_IOR('p', 0x09, struct efi_getnexthighmonotoniccount)

#define EFI_RUNTIME_QUERY_CAPSULECAPABILITIES \
	_IOR('p', 0x0A, struct efi_querycapsulecapabilities)

#endif /* _EFI_RUNTIME_H_ */
