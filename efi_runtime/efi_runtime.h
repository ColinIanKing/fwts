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

typedef enum {
	EfiResetCold,
	EfiResetWarm,
	EfiResetShutdown
} EFI_RESET_TYPE;

struct efi_getvariable {
	uint16_t	*VariableName;
	efi_guid_t	*VendorGuid;
	uint32_t	*Attributes;
	uint64_t	*DataSize;
	void		*Data;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_setvariable {
	uint16_t	*VariableName;
	efi_guid_t	*VendorGuid;
	uint32_t	Attributes;
	uint64_t	DataSize;
	void		*Data;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_getnextvariablename {
	uint64_t	*VariableNameSize;
	uint16_t	*VariableName;
	efi_guid_t	*VendorGuid;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_queryvariableinfo {
	uint32_t	Attributes;
	uint64_t	*MaximumVariableStorageSize;
	uint64_t	*RemainingVariableStorageSize;
	uint64_t	*MaximumVariableSize;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_gettime {
	efi_time_t		*Time;
	efi_time_cap_t		*Capabilities;
	uint64_t		*status;
} __attribute__ ((packed));

struct efi_settime {
	efi_time_t		*Time;
	uint64_t		*status;
} __attribute__ ((packed));

struct efi_getwakeuptime {
	uint8_t		*Enabled;
	uint8_t		*Pending;
	efi_time_t	*Time;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_setwakeuptime {
	uint8_t		Enabled;
	efi_time_t	*Time;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_getnexthighmonotoniccount {
	uint32_t	*HighCount;
	uint64_t	*status;
} __attribute__ ((packed));

struct efi_querycapsulecapabilities {
	efi_capsule_header_t	**CapsuleHeaderArray;
	uint64_t		CapsuleCount;
	uint64_t		*MaximumCapsuleSize;
	EFI_RESET_TYPE		*ResetType;
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
