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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "fwts.h"

#ifdef FWTS_ARCH_INTEL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define DMI_VERSION 			(0x0207)
#define VERSION_MAJOR(v)		((v) >> 8)
#define VERSION_MINOR(v)		((v) & 0xff)

#define SMBIOS_END_OF_TABLE		(127)

#define DMI_NO_TABLE			"DMINoTable"
#define DMI_NO_TABLE_HEADER		"DMINoTableHeader"
#define DMI_BAD_TABLE_LENGTH		"DMIBadTableLength"
#define DMI_BAD_UUID			"DMIBadUUID"
#define DMI_STRUCT_COUNT		"DMIStructCount"
#define DMI_VALUE_OUT_OF_RANGE		"DMIValueOutOfRange"
#define DMI_STRING_INDEX_OUT_OF_RANGE	"DMIStringIndexOutOfRange"
#define DMI_ILLEGAL_MAPPED_ADDR_RANGE	"DMIIllegalMappedAddrRange"
#define DMI_MGMT_CTRL_HOST_TYPE		"DMIMgmtCtrlHostType"
#define DMI_INVALID_ENTRY_LENGTH	"DMIInvalidEntryLength"

#define GET_UINT16(x) (uint16_t)(*(const uint16_t *)(x))
#define GET_UINT32(x) (uint32_t)(*(const uint32_t *)(x))
#define GET_UINT64(x) (uint64_t)(*(const uint64_t *)(x))

typedef struct {
	const char *label;
	const char *field;
	const char *value;
} fwts_dmi_pattern;

typedef struct {
	uint16_t   old;
	uint16_t   new;
} fwts_dmi_version;

static const fwts_dmi_pattern dmi_patterns[] = {
	{ "DMISerialNumber",	"Serial Number", 	"0123456789" },
	{ "DMIAssetTag",	"Asset Tag",		"1234567890" },
	{ "DMIBadDefault",	NULL,			"To Be Filled By O.E.M." },
	{ NULL,			NULL,			NULL }
};

static const char *uuid_patterns[] = {
	"0A0A0A0A-0A0A-0A0A-0A0A-0A0A0A0A0A0A",
	NULL,
};

/* Remapping table from buggy version numbers to correct values */
static const fwts_dmi_version dmi_versions[] = {
	{ 0x021f, 0x0203 },
	{ 0x0221, 0x0203 },
	{ 0x0233, 0x0206 },
	{ 0, 0 }
};

static uint16_t dmi_remap_version(fwts_framework *fw, uint16_t old)
{
	int i;

	for (i=0; dmi_versions[i].old != 0; i++) {
		if (old == dmi_versions[i].old) {
			uint16_t new = dmi_versions[i].new;
			fwts_warning(fw,
				"Detected a buggy DMI version number %u.%u, remapping to %u.%u",
				VERSION_MAJOR(old), VERSION_MINOR(old),
				VERSION_MAJOR(new), VERSION_MINOR(new));
			return new;
		}
	}

	/* All OK, return original */
	return old;
}

static void dmi_min_max_uint8_check(fwts_framework *fw,
	const char *table,
	uint32_t addr,
	const char *field,
	const fwts_dmi_header *hdr,
	uint8_t offset,
	uint8_t min,
	uint8_t max)
{
	uint8_t val = hdr->data[offset];
	if ((val < min) || (val > max))
		fwts_failed(fw, LOG_LEVEL_HIGH,
			DMI_VALUE_OUT_OF_RANGE,
			"Out of range value 0x%2.2x (range allowed 0x%2.2x..0x%2.2x) "
			"while accessing entry '%s' @ 0x%8.8x, field '%s', offset 0x%2.2x",
			val, min, max, table, addr, field, offset);
}

static void dmi_min_max_mask_uint8_check(fwts_framework *fw,
	const char *table,
	uint32_t addr,
	const char *field,
	const fwts_dmi_header *hdr,
	uint8_t offset,
	uint8_t min,
	uint8_t max,
	uint8_t shift,
	uint8_t mask)
{
	uint8_t val = (hdr->data[offset] >> shift) & mask;

	if ((val < min) || (val > max))
		fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
			"Out of range value 0x%2.2x (range allowed 0x%2.2x..0x%2.2x) "
			"while accessing entry '%s' @ 0x%8.8x, field '%s', offset 0x%2.2x",
			val, min, max, table, addr, field, offset);
}

static void dmi_str_check_index(fwts_framework *fw,
	const char *table,
	uint32_t addr,
	const char *field,
	const fwts_dmi_header *hdr,
	uint8_t offset,
	uint8_t index)
{
	char 	*data = (char *)hdr->data;
	uint8_t	i = index;

	if (i > 0) {
		int 	j;
		int	failed = -1;

		data += hdr->length;
		while (i > 1 && *data) {
			data += strlen(data) + 1;
			i--;
		}
		/* Sanity checks */
		if (*data == '\0') {
			fwts_failed(fw, LOG_LEVEL_HIGH, DMI_STRING_INDEX_OUT_OF_RANGE,
				"Out of range string index 0x%2.2x while accessing entry '%s' "
				"@ 0x%8.8x, field '%s', offset 0x%2.2x",
				index, table, addr, field, offset);
			return;
		}

		/* Scan for known BIOS defaults that vendors forget to set */
		for (j=0; dmi_patterns[j].label != NULL; j++) {
			if (dmi_patterns[j].field &&
				(strcmp(dmi_patterns[j].field, field) == 0) &&
				(strcmp(dmi_patterns[j].value, data) == 0)) {
				failed = j;
				break;
			} else if (strcmp(dmi_patterns[j].value, data) == 0) {
				failed = j;
				break;
			}
		}
		if (failed != -1) {
			fwts_failed(fw, LOG_LEVEL_LOW, dmi_patterns[j].label,
				"String index 0x%2.2x in table entry '%s' @ 0x%8.8x, field '%s', "
				"offset 0x%2.2x has a default value '%s' and probably has "
				"not been updated by the BIOS vendor.",
				index, table, addr, field, offset, data);
		}
	}
}

static inline void dmi_str_check(fwts_framework *fw,
	const char *table,
	uint32_t addr,
	const char *field,
	const fwts_dmi_header *hdr,
	uint8_t offset)
{
	dmi_str_check_index(fw, table, addr, field, hdr, offset, hdr->data[offset]);
}

static void dmi_uuid_check(fwts_framework *fw,
	const char *table,
	uint32_t addr,
	const char *field,
	const fwts_dmi_header *hdr,
	uint8_t offset,
	uint16_t version)
{
	char guid_str[37];
	int i;

	fwts_guid_buf_to_str(hdr->data + offset, guid_str, sizeof(guid_str));

	for (i=0; uuid_patterns[i] != NULL; i++) {
		if (strcmp(guid_str, uuid_patterns[i]) == 0) {
			fwts_failed(fw, LOG_LEVEL_LOW, DMI_BAD_UUID,
				"UUID in table entry '%s' @ 0x%8.8x, field '%s', "
				"offset 0x%2.2x has a default value '%s' and probably has "
				"not been updated by the BIOS vendor.",
				table, addr, field, offset, guid_str);
		}
	}
}

static void dmi_decode_entry(fwts_framework *fw,
	uint32_t addr,
	const fwts_dmi_header *hdr,
	uint16_t version)
{
	uint8_t *ptr;
	uint8_t count;
	uint8_t val;
	uint8_t *data = hdr->data;
	char	tmp[64];
	char	*table;
	int	i;
	int 	len;
	int	failed_count = fw->minor_tests.failed;

	switch (hdr->type) {
		case 0: /* 7.1 */
			table = "BIOS Information (Type 0)";
			if (hdr->length < 0x12)
				break;
			dmi_str_check(fw, table, addr, "Vendor", hdr, 0x4);
			dmi_str_check(fw, table, addr, "BIOS Version", hdr, 0x5);
			dmi_str_check(fw, table, addr, "Release Date", hdr, 0x8);
			break;

		case 1: /* 7.2 */
			table = "System Information (Type 1)";
			if (hdr->length < 0x08)
				break;
			dmi_str_check(fw, table, addr, "Manufacturer", hdr, 0x4);
			dmi_str_check(fw, table, addr, "Product Name", hdr, 0x5);
			dmi_str_check(fw, table, addr, "Version", hdr, 0x6);
			dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x7);
			if (hdr->length < 0x19)
				break;
			dmi_uuid_check(fw, table, addr, "UUID", hdr, 0x8, version);
			dmi_min_max_uint8_check(fw, table, addr, "Wakeup Type", hdr, 0x18, 0x0, 0x08);
			if (hdr->length < 0x1b)
				break;
			dmi_str_check(fw, table, addr, "SKU Number", hdr, 0x19);
			dmi_str_check(fw, table, addr, "Family", hdr, 0x1a);
			break;

		case 2: /* 7.3 */
			table = "Base Board Information (Type 2)";
			if (hdr->length < 0x08)
				break;
			dmi_str_check(fw, table, addr, "Manufacturer", hdr, 0x4);
			dmi_str_check(fw, table, addr, "Product", hdr, 0x5);
			dmi_str_check(fw, table, addr, "Version", hdr, 0x6);
			dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x7);
			if (hdr->length < 0x09)
				break;
			dmi_str_check(fw, table, addr, "Asset Tag", hdr, 0x8);
			if (hdr->length < 0x0f)
				break;
			dmi_str_check(fw, table, addr, "Location In Chassis", hdr, 0xa);
			dmi_min_max_uint8_check(fw, table, addr, "Type", hdr, 0xd, 0x1, 0xd);
			break;

		case 3: /* 7.4 */
			table = "Chassis Information (Type 3)";
			if (hdr->length < 0x09)
				break;
			dmi_str_check(fw, table, addr, "Manufacturer", hdr, 0x4);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Chassis Type", hdr, 0x5, 0x1, 0x1d, 0x0, 0x7f);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Chassis Lock", hdr, 0x5, 0x0, 0x1, 0x7, 0x1);
			dmi_str_check(fw, table, addr, "Version", hdr, 0x6);
			dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x7);
			dmi_str_check(fw, table, addr, "Asset Tag", hdr, 0x8);
			dmi_min_max_uint8_check(fw, table, addr, "Boot-up State", hdr, 0x9, 0x1, 0x6);
			dmi_min_max_uint8_check(fw, table, addr, "Power Supply State", hdr, 0xa, 0x1, 0x6);
			dmi_min_max_uint8_check(fw, table, addr, "Thermal State", hdr, 0xb, 0x1, 0x6);
			dmi_min_max_uint8_check(fw, table, addr, "Security Status", hdr, 0xc, 0x1, 0x5);
			if (hdr->length < 0x15 + data[0x13] * data[0x14])
				break;
			ptr = data + 0x15;
			len = data[0x14];
			if (len >= 0x3) {
				for (i = 0; i < data[0x13]; i++) {
					val = ptr[i * len] & 0x7f;
					if (ptr[i * len] & 0x80) {
						if (val > 0x42)
							fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
								"Out of range value 0x%2.2x "
								"(range allowed 0x00..0x42) "
								"while accessing entry '%s' @ 0x%8.8x, field "
								"'SMBIOS Structure Type %d', offset 0x%2.2x",
								val, table, addr, i, 0x15 + (i*len));
					} else {
						if ((val < 0x1) || (val > 0xd))
							snprintf(tmp, sizeof(tmp), "Base Board Type %d", i);
							fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
								"Out of range value 0x%2.2x "
								"(range allowed 0x00..0x42) "
								"while accessing entry '%s' @ 0x%8.8x, field "
								"'Base Board Type %d', offset 0x%2.2x",
								val, table, addr, i, 0x15 + (i*len));
					}
				}
			}
			if (hdr->length < 0x16 + data[0x13] * data[0x14])
				break;
			dmi_str_check(fw, table, addr, "SKU Number", hdr, 0x15 + data[0x13] * data[0x14]);
			break;

		case 4: /* 7.5 */
			table = "Processor Information (Type 4)";
			if (hdr->length < 0x1a)
				break;
			dmi_str_check(fw, table, addr, "Socket Designation", hdr, 0x4);
			dmi_min_max_uint8_check(fw, table, addr, "Processor Type", hdr, 0x5, 0x1, 0x6);
			dmi_str_check(fw, table, addr, "Processor Manufacturer", hdr, 0x7);
			dmi_str_check(fw, table, addr, "Processor Version", hdr, 0x10);
			dmi_min_max_uint8_check(fw, table, addr, "Upgrade", hdr, 0x19, 0x1, 0x2a);
			if (hdr->length < 0x23)
				break;
			dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x20);
			dmi_str_check(fw, table, addr, "Asset Tag", hdr, 0x21);
			dmi_str_check(fw, table, addr, "Part Number", hdr, 0x22);
			break;

		case 5: /* 7.6 */
			table = "Memory Controller Information (Type 5)";
			if (hdr->length < 0x0f)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Error Detecting Method", hdr, 0x4, 0x1, 0x8);
			dmi_min_max_uint8_check(fw, table, addr, "Supported Interleave", hdr, 0x6, 0x1, 0x7);
			dmi_min_max_uint8_check(fw, table, addr, "Current Interleave", hdr, 0x7, 0x1, 0x7);
			break;

		case 6: /* 7.7 */
			table = "Memory Module Information (Type 6)";
			if (hdr->length < 0x0c)
				break;
			dmi_str_check(fw, table, addr, "Socket Designation", hdr, 0x4);
			break;

		case 7: /* 7.8 */
			table = "Cache Information (Type 7)";
			if (hdr->length < 0x0f)
				break;
			dmi_str_check(fw, table, addr, "Socket Designation", hdr, 0x4);
			if (((GET_UINT16(data + 0x05) >> 5) & 0x0003) == 0x2)
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value %x4.4x "
					"bits 5..6 set to illegal value 0x2, only allowed"
					"0x0, 0x1, 0x3 while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					GET_UINT16(data + 0x05),
					table, addr, "Cache Location", 0x5);
			if (hdr->length < 0x13)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Error Correction Type", hdr, 0x10, 0x1, 0x6);
			dmi_min_max_uint8_check(fw, table, addr, "System Cache Type", hdr, 0x11, 0x1, 0x5);
			dmi_min_max_uint8_check(fw, table, addr, "Associativity", hdr, 0x12, 0x1, 0xe);
			break;

		case 8: /* 7.9 */
			table = "Port Connector Information (Type 8)";
			if (hdr->length < 0x09)
				break;
			dmi_str_check(fw, table, addr, "Internal Reference Designator", hdr, 0x4);
			if (!((data[0x5] <= 0x22) ||
 			      (data[0x5] == 0xff) ||
 			      ((data[0x5] >= 0xa0) && (data[0x5] <= 0xa4))))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x00..0x22, 0xa0..0xa4, 0xff) "
					"while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					data[0x5], table, addr, "Internal Connector Type", 0x5);
			dmi_str_check(fw, table, addr, "External Reference Designator", hdr, 0x6);
			if (!((data[0x7] <= 0x22) ||
 			      (data[0x7] == 0xff) ||
 			      ((data[0x7] >= 0xa0) && (data[0x7] <= 0xa4))))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x00..0x22, 0xa0..0xa4, 0xff) "
					"while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					data[0x7], table, addr, "Internal Connector Type", 0x7);

			if (!((data[0x8] <= 0x21) ||
 			      (data[0x8] == 0xff) ||
 			      ((data[0x8] >= 0xa0) && (data[0x8] <= 0xa1))))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x00..0x21, 0xa0..0xa1, 0xff) "
					"while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					data[0x8], table, addr, "Port Type", 0x8);
			break;

		case 9: /* 7.10 */
			table = "System Slot Information (Type 9)";
			if (hdr->length < 0x0c)
				break;
			dmi_str_check(fw, table, addr, "Slot Designation", hdr, 0x4);
			if (!(((data[0x5] >= 0x01) && (data[0x5] <= 0x13)) ||
			      ((data[0x5] >= 0xa0) && (data[0x5] <= 0xb6))))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x01..0x08, 0xa0..0xa2) "
					"while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					data[0x5], table, addr, "Slot Type", 0x5);
			dmi_min_max_uint8_check(fw, table, addr, "Slot Data Bus Width", hdr, 0x6, 0x1, 0xe);
			dmi_min_max_uint8_check(fw, table, addr, "Current Usage", hdr, 0x7, 0x1, 0x4);
			dmi_min_max_uint8_check(fw, table, addr, "Slot Length", hdr, 0x8, 0x1, 0x4);
			break;

		case 10: /* 7.11 */
			table = "On Board Devices (Type 10)";
			count = (hdr->length - 4) / 2;
			for (i = 0; i < count; i++) {
				snprintf(tmp, sizeof(tmp), "Type (Device #%d)", i);
				dmi_min_max_mask_uint8_check(fw, table, addr, tmp, hdr, 4 + (2 * i), 0x1, 0xa, 0x0, 0x7f);
				snprintf(tmp, sizeof(tmp), "Description (Device #%d)", i);
				dmi_str_check(fw, table, addr, tmp, hdr, 5 + (2 * i));
			}
			break;

		case 11: /* 7.12 */
			table = "OEM Strings (Type 11)";
			if (hdr->length < 0x5)
				break;
			for (i = 1; i <= hdr->data[4]; i++) {
				snprintf(tmp, sizeof(tmp), "String %d", i);
				dmi_str_check_index(fw, table, addr, tmp, hdr, 0x4, i);
			}
			break;

		case 12: /* 7.13 */
			table = "System Configuration Options (Type 12)";
			if (hdr->length < 0x5)
				break;
			for (i = 1; i <= hdr->data[4]; i++) {
				snprintf(tmp, sizeof(tmp), "Option %d", i);
				dmi_str_check_index(fw, table, addr, tmp, hdr, 0x4, i);
			}
			break;

		case 13: /* 7.14 */
			table = "BIOS Language Information (Type 13)";
			if (hdr->length < 0x16)
				break;
			for (i = 1; i <= hdr->data[4]; i++) {
				snprintf(tmp, sizeof(tmp), "BIOS Language String %d", i);
				dmi_str_check_index(fw, table, addr, tmp, hdr, 0x4, i);
			}
			dmi_str_check(fw, table, addr, "Currently Installed Language", hdr, 0x15);
			break;

		case 14: /* 7.15 */
			table = "Group Associations (Type 14)";
			if (hdr->length < 0x05)
				break;
			dmi_str_check(fw, table, addr, "Name", hdr, 0x4);
			break;

		case 15: /* 7.16 */
			table = "System Event Log (Type 15)";
			if (hdr->length < 0x14)
				break;
			val = hdr->data[0x0a];
			if (!(((val >= 0x00) && (val <= 0x04)) ||
			      ((val >= 0x80) && (val <= 0xff)))) {
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x (range allowed 0x00..0x01, "
					"0x80..0xff) while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					val, table, addr, "Access Method", 0x0a);
			}
			if (hdr->length < 0x17)
				break;
			val = hdr->data[0x14];
			if (!(((val >= 0x00) && (val <= 0x01)) ||
			      ((val >= 0x80) && (val <= 0xff)))) {
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x (range allowed 0x00..0x01, "
					"0x80..0xff) while accessing entry '%s' @ 0x%8.8x, "
					"field '%s', offset 0x%2.2x",
					val, table, addr, "Log Header Format", 0x14);
			}
			if (hdr->length < 0x17 + data[0x15] * data[0x16])
				break;
			if (data[0x16] >= 0x02) {
				uint8_t *ptr = data + 0x17;
				int i;
				for (i = 0; i < data[0x15]; i++) {
					int j = data[0x16] * i;
					val = ptr[j];
					if (!(((val >= 0x01) && (val <= 0x0e)) ||
					      ((val >= 0x10) && (val <= 0x17)) ||
					      ((val >= 0x80) && (val <= 0xff)))) {
						fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
							"Out of range value 0x%2.2x "
							"(range allowed 0x01..0x0e, 0x10..0x17, "
							"0x80..0xff) while accessing entry '%s' @ 0x%8.8x, "
							"field '%s', item %d",
							val, table, addr, "Log Descriptor Type", i);
					}
					val = ptr[j + 1];
					if ((val > 0x06) && (val < 0x80)) {
						fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
							"Out of range value 0x%2.2x "
							"(range allowed 0x00..0x06, 0x80..0xff) "
							"while accessing entry '%s' @ 0x%8.8x, "
							"field '%s', item %d",
							val, table, addr, "Log Descriptor Format", i);
					}
				}
			}
			break;

		case 16: /* 7.17 */
			table = "Physical Memory Array (Type 16)";
			if (hdr->length < 0x0f)
				break;
			if (!(((data[0x4] >= 0x01) && (data[0x4] <= 0x0a)) ||
			      ((data[0x4] >= 0xa0) && (data[0x4] <= 0xa3))))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x01..0x0a, 0xa0..0xa3) "
					"while accessing entry '%s' @ 0x%8.8x, field '%s', offset 0x%2.2x",
					data[0x4], table, addr, "Location", 0x4);
			dmi_min_max_uint8_check(fw, table, addr, "Use", hdr, 0x5, 0x1, 0x7);
			dmi_min_max_uint8_check(fw, table, addr, "Error Corrrection Type", hdr, 0x6, 0x1, 0x7);
			break;

		case 17: /* 7.18 */
			table = "Memory Device (Type 17)";
			if (hdr->length < 0x15)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Form Factor", hdr, 0xe, 0x1, 0xf);
			dmi_str_check(fw, table, addr, "Locator", hdr, 0x10);
			dmi_str_check(fw, table, addr, "Bank Locator", hdr, 0x11);
			dmi_min_max_uint8_check(fw, table, addr, "Memory Type", hdr, 0x12, 0x1, 0x19);
			if (hdr->length < 0x1b)
				break;
			dmi_str_check(fw, table, addr, "Manufacturer", hdr, 0x17);
			dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x18);
			dmi_str_check(fw, table, addr, "Asset Tag", hdr, 0x19);
			dmi_str_check(fw, table, addr, "Part Number", hdr, 0x1a);
			break;

		case 18: /* 7.19 */
			table = "32-bit Memory Error Information (Type 18)";
			if (hdr->length < 0x17)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Error Type", hdr, 0x4, 0x1, 0xe);
			dmi_min_max_uint8_check(fw, table, addr, "Error Granularity", hdr, 0x5, 0x1, 0x4);
			dmi_min_max_uint8_check(fw, table, addr, "Error Operation", hdr, 0x6, 0x1, 0x5);
			break;

		case 19: /* 7.20 */
			table = "Memory Array Mapped Address (Type 19)";
			if (hdr->length < 0x0F)
				break;
			if (hdr->length >= 0x1F && GET_UINT32(data + 0x04) == 0xFFFFFFFF) {
				uint64_t start, end;
				start = GET_UINT64(data + 0x0F);
				end = GET_UINT64(data + 0x17);
				if (start == end)
					fwts_failed(fw, LOG_LEVEL_HIGH, DMI_ILLEGAL_MAPPED_ADDR_RANGE,
						"Extended Start and End addresses are identical "
						"while accessing entry '%s' @ 0x%8.8x, "
						"fields 'Extended Starting Address' and 'Extended Ending Address'",
						table, addr);
			} else {
				if (GET_UINT32(data + 0x08) - GET_UINT32(data + 0x04) + 1 == 0)
					fwts_failed(fw, LOG_LEVEL_HIGH, DMI_ILLEGAL_MAPPED_ADDR_RANGE,
						"Illegal zero mapped address range "
						"for entry '%s' @ 0x%8.8x", table, addr);
			}
			break;

		case 20: /* 7.21 */
			table = "Memory Device Mapped Address (Type 20)";
			if (hdr->length < 0x13)
				break;
			if (hdr->length >= 0x23 && GET_UINT32(data + 0x04) == 0xFFFFFFFF) {
				uint64_t start, end;
				start = GET_UINT64(data + 0x13);
				end = GET_UINT64(data + 0x1B);
				if (start == end)
					fwts_failed(fw, LOG_LEVEL_HIGH, DMI_ILLEGAL_MAPPED_ADDR_RANGE,
						"Extended Start and End addresses are identical "
						"while accessing entry '%s' @ 0x%8.8x, "
						"fields 'Extended Starting Address' and 'Extended Ending Address'",
						table, addr);
			} else {
				if (GET_UINT32(data + 0x08) - GET_UINT32(data + 0x04) + 1 == 0)
					fwts_failed(fw, LOG_LEVEL_HIGH, DMI_ILLEGAL_MAPPED_ADDR_RANGE,
						"Illegal zero mapped address range "
						"for entry '%s' @ 0x%8.8x", table, addr);
			}
			if (data[0x10] == 0)
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_ILLEGAL_MAPPED_ADDR_RANGE,
					"Illegal row position %2.2x, "
					"while accessing entry '%s' @ 0x%8.8x, field '%s', offset 0x%2.2x",
					data[0x10], table, addr, "Partial Row Position", 0x10);
			break;

		case 21: /* 7.22 */
			table = "Built-in Pointing Device (Type 21)";
			if (hdr->length < 0x07)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Type", hdr, 0x4, 0x1, 0x9);
			if (!(((data[0x5] >= 0x01) && (data[0x5] <= 0x08)) ||
			      ((data[0x5] >= 0xa0) && (data[0x5] <= 0xa2)))) {
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x01..0x08, 0xa0..0xa2) "
					"while accessing '%s', field '%s', offset 0x%2.2x",
					data[0x5], table, "Interface", 0x5);
}
			break;

		case 22: /* 7.23 */
			table = "Portable Battery (Type 22)";
			if (hdr->length < 0x10)
				break;
			dmi_str_check(fw, table, addr, "Location", hdr, 0x4);
			dmi_str_check(fw, table, addr, "Manufacturer", hdr, 0x5);
			if (data[0x06] || hdr->length < 0x1A)
				dmi_str_check(fw, table, addr, "Manufacturer Date", hdr, 0x6);
			if (data[0x07] || hdr->length < 0x1A)
				dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x7);
			dmi_str_check(fw, table, addr, "Device Name", hdr, 0x8);
			if (data[0x09] != 0x02 || hdr->length < 0x1A)
				dmi_str_check(fw, table, addr, "Device Chemistry", hdr, 0x9);

			dmi_str_check(fw, table, addr, "SBDS Version Number", hdr, 0xe);
			if (hdr->length < 0x1A)
				break;
			if (data[0x09] == 0x02)
				dmi_str_check(fw, table, addr, "SBDS Device Chemistry", hdr, 0x14);
			break;

		case 23: /* 7.24 */
			table = "System Reset (Type 23)";
			if (hdr->length < 0x0D)
				break;
			if (!(data[0x04] & (1 << 5)))
				break;
			dmi_min_max_mask_uint8_check(fw, table, addr, "Capabilities (bits 1..2)", hdr, 0x4, 0x1, 0x3, 1, 0x3);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Capabilities (bits 3..4)", hdr, 0x4, 0x1, 0x3, 3, 0x3);
			break;

		case 24: /* 7.25 */
			table = "Hardware Security (Type 24)";
			/* if (hdr->length < 0x05)
				break; */
			break;

		case 25: /* 7.26 */
			table = "System Power Controls (Type 25)";
			/* if (hdr->length < 0x9)
				break; */
			break;

		case 26: /* 7.27 */
			table = "Voltage Probe (Type 26)";
			if (hdr->length < 0x14)
				break;
			dmi_str_check(fw, table, addr, "Description", hdr, 0x4);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Location (bits 0..4)", hdr, 0x5, 0x1, 0xb, 0, 0x1f);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Status (bits 5..7)", hdr, 0x5, 0x1, 0x6, 5, 0x7);
			break;

		case 27: /* 7.28 */
			table = "Cooling Device (Type 27)";
			if (hdr->length < 0xc)
				break;
			val = data[0x06] & 0x1f;
			if (!(((val >= 0x01) && (val <= 0x09)) ||
			      ((val >= 0x10) && (val <= 0x11))))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x01..0x09, 0x10..0x11) "
					"while accessing entry '%s' @ 0x%8.8x, field '%s', "
					"offset 0x%2.2x, mask 0x%2.2x",
					data[0x6], table, addr, "Device Type", 0x6, 0x1f);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Status (bits 5..7)", hdr, 0x6, 0x1, 0x6, 5, 0x7);
			if (hdr->length < 0x0f)
				break;
			dmi_str_check(fw, table, addr, "Description", hdr, 0xe);
			break;

		case 28: /* 7.29 */
			table = "Temperature Probe (Type 28)";
			if (hdr->length < 0x14)
				break;
			dmi_str_check(fw, table, addr, "Description", hdr, 0x4);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Location (bits 0..4)", hdr, 0x5, 0x1, 0xf, 0, 0x1f);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Status (bits 5..7)", hdr, 0x5, 0x1, 0x6, 5, 0x7);
			break;

		case 29: /* 7.30 */
			table = "Electrical Current Probe (Type 29)";
			if (hdr->length < 0x14)
				break;
			dmi_str_check(fw, table, addr, "Description", hdr, 0x4);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Location (bits 0..4)", hdr, 0x5, 0x1, 0xb, 0, 0x1f);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Status (bits 5..7)", hdr, 0x5, 0x1, 0x6, 5, 0x7);
			break;

		case 30: /* 7.31 */
			table = "Out-of-band Remote Access (Type 30)";
			if (hdr->length < 0x06)
				break;
			dmi_str_check(fw, table, addr, "Manufacturer Name", hdr, 0x4);
			break;

		case 31: /* 7.32 */
			table = "Boot Integrity Services Entry Point (Type 31)";
			/*
			if (hdr->length < 0x1c)
				break;
			*/
			break;

		case 32: /* 7.33 */
			table = "System Boot Information (Type 32)";
			if (hdr->length < 0xb)
				break;
			if ((data[0xa] > 0x8) && (data[0xa] < 128))
				fwts_failed(fw, LOG_LEVEL_HIGH, DMI_VALUE_OUT_OF_RANGE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x00..0x08, 0x80..0xff) "
					"while accessing entry '%s' @ 0x%8.8x, field '%s', offset 0x%2.2x",
					data[0xa], table, addr, "Boot Status", 0xa);
			break;

		case 33: /* 7.34 */
			table = "64-bit Memory Error Information (Type 33)";
			if (hdr->length < 0x1f)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Error Type", hdr, 0x4, 0x1, 0xe);
			dmi_min_max_uint8_check(fw, table, addr, "Error Granularity", hdr, 0x5, 0x1, 0x4);
			dmi_min_max_uint8_check(fw, table, addr, "Error Operation", hdr, 0x6, 0x1, 0x5);
			break;

		case 34: /* 7.35 */
			table = "Management Device (Type 34)";
			if (hdr->length < 0x0b)
				break;
			dmi_str_check(fw, table, addr, "Description", hdr, 0x4);
			dmi_min_max_uint8_check(fw, table, addr, "Type", hdr, 0x5, 0x1, 0xd);
			dmi_min_max_uint8_check(fw, table, addr, "Address Type", hdr, 0xa, 0x1, 0x5);
			break;

		case 35: /* 7.36 */
			table = "Management Device Component (Type 35)";
			if (hdr->length < 0x0b)
				break;
			dmi_str_check(fw, table, addr, "Description", hdr, 0x4);
			break;

		case 36: /* 7.37 */
			table = "Management Device Threshold Data (Type 36)";
			/*
			if (hdr->length < 0x10)
				break;
			*/
			break;

		case 37: /* 7.38 */
			table = "Memory Channel (Type 37)";
			if (hdr->length < 0x07)
				break;
			dmi_min_max_uint8_check(fw, table, addr, "Type", hdr, 0x4, 0x1, 0x4);
			break;

		case 38: /* 7.39 */
			table = "IPMI Device Information (Type 38)";
			dmi_min_max_uint8_check(fw, table, addr, "Interface Type", hdr, 0x4, 0x0, 0x4);
			break;

		case 39: /* 7.40 */
			table = "System Power Supply (Type 39)";
			if (hdr->length < 0x10)
				break;
			dmi_str_check(fw, table, addr, "Location", hdr, 0x5);
			dmi_str_check(fw, table, addr, "Device Name", hdr, 0x6);
			dmi_str_check(fw, table, addr, "Manufacturer", hdr, 0x7);
			dmi_str_check(fw, table, addr, "Serial Number", hdr, 0x8);
			dmi_str_check(fw, table, addr, "Asset Tag", hdr, 0x9);
			dmi_str_check(fw, table, addr, "Model Part Number", hdr, 0xa);
			dmi_str_check(fw, table, addr, "Revision Level", hdr, 0xb);
			break;

		case 40: /* 7.41 */
			table = "Additional Information (Type 40)";
			break;

		case 41: /* 7.42 */
			table = "Onboard Device (Type 41)";
			if (hdr->length < 0xb)
				break;
			dmi_str_check(fw, table, addr, "Reference Designation", hdr, 0x4);
			dmi_min_max_mask_uint8_check(fw, table, addr, "Device Type", hdr, 0x5, 0x1, 0xa, 0, 0x7f);
			break;

		case 42: /* 7.43 */
			table = "Management Controller Host Interface (Type 42)";
			if (hdr->length < 0x05)
				break;
			if (!((data[0x04] >= 0x02 && data[0x04] <= 0x08) ||
			      (data[0x04] == 0xF0)))
				fwts_failed(fw, LOG_LEVEL_MEDIUM, DMI_MGMT_CTRL_HOST_TYPE,
					"Out of range value 0x%2.2x "
					"(range allowed 0x02..0x08, 0xf0) "
					"while accessing entry '%s' @ 0x%8.8x, field '%s', offset 0x%2.2x",
					data[0x4], table, addr, "Reference Designation", 0x4);
			break;

		case 126: /* 7.44 */
			table = "Inactive (Type 126)";
			break;
		case SMBIOS_END_OF_TABLE: /* 7.45 */
			table = "End of Table (Type 127)";
			break;
		default:
			snprintf(tmp, sizeof(tmp), "Unknown (Type %d)", hdr->type);
			table = tmp;
			break;
	}
	if (fw->minor_tests.failed == failed_count)
		fwts_passed(fw, "Entry @ 0x%8.8x '%s'", addr, table);	
	else if (hdr->type <= 42) 
		fwts_advice(fw,
			"It may be worth checking against section 7.%d of the "
			"System Management BIOS (SMBIOS) Reference Specification "
			"(see http://www.dmtf.org/standards/smbios).", hdr->type+1);
}

static int dmi_version_check(fwts_framework *fw, uint16_t version)
{
	if (version > DMI_VERSION) {
		fwts_warning(fw,
			"SMBIOS version %u.%u is not supported by the dmi_decode "
			"test. This test only supports SMBIOS version %u.%u and lower.",
			VERSION_MAJOR(version), VERSION_MINOR(version),
			VERSION_MAJOR(DMI_VERSION), VERSION_MINOR(DMI_VERSION));
		return FWTS_ERROR;
	}
	return FWTS_OK;
}

static void dmi_scan_tables(fwts_framework *fw,
	fwts_smbios_entry *entry,
	uint8_t  *table,
	uint16_t version)
{
	uint8_t *entry_data = table;
	uint16_t table_length;
	uint16_t struct_count;
	int i = 0;

	table_length = entry->struct_table_length;
	struct_count = entry->number_smbios_structures;

	while ((i < struct_count) &&
	       (entry_data <= (table + table_length - 4))) {
 		uint32_t addr = entry->struct_table_address + (entry_data - table);
		fwts_dmi_header hdr;
		uint8_t *next_entry;

		hdr.type   = entry_data[0];
		hdr.length = entry_data[1];
		hdr.handle = GET_UINT16(entry_data + 2);
		hdr.data   = entry_data;

		/* Sanity check */
		if (hdr.length < 4) {
			fwts_failed(fw, LOG_LEVEL_HIGH, DMI_INVALID_ENTRY_LENGTH,
				"Invald header length of entry #%d, length was 0x%2.2x.",
				i, (unsigned int)hdr.length);
			fwts_advice(fw,
				"DMI entry header lengths must be 4 or more bytes long "
				"so this error indicates that the DMI table is unreliable "
				"and DMI table checking has been aborted at entry #%d.", i);
			break;
		}

		/* Real Physical Address */
		next_entry = entry_data + hdr.length;

		/* Look for structure terminator, ends in two zero bytes */
		while (((next_entry - table + 1) < table_length) &&
		       ((next_entry[0] != 0) || (next_entry[1] != 0))) {
			next_entry++;
		}

		/* Skip over terminating two zero bytes, see section 6.1 of spec */
		next_entry += 2;

		if ((next_entry - table) <= table_length)
			dmi_decode_entry(fw, addr, &hdr, version);

		i++;
		entry_data = next_entry;
	}

	if (table_length != (entry_data - table))
		fwts_failed(fw, LOG_LEVEL_MEDIUM, DMI_BAD_TABLE_LENGTH,
			"DMI table length was %d bytes (as specified by "
			"the SMBIOS header) but the DMI entries used %d bytes.",
			table_length, (int)(entry_data - table));

	if (struct_count != i)
		fwts_failed(fw, LOG_LEVEL_MEDIUM, DMI_STRUCT_COUNT,
			"DMI table was DMI %d entries in size (as specified by "
			"the SMBIOS header) but only %d DMI entries were found.",
				struct_count, i);
}

static int dmi_decode_test1(fwts_framework *fw)
{
	void *addr;
	fwts_smbios_entry entry;
	fwts_smbios_type  type;
	uint16_t version = 0;
	uint8_t  *table;

	addr = fwts_smbios_find_entry(fw, &entry, &type, &version);
	if (addr == NULL) {
		fwts_failed(fw, LOG_LEVEL_HIGH, DMI_NO_TABLE_HEADER,
			"Cannot find SMBIOS or DMI table entry.");
		return FWTS_ERROR;
	}
	if (type == FWTS_SMBIOS_UNKNOWN) {
		fwts_failed(fw, LOG_LEVEL_HIGH, DMI_NO_TABLE,
			"Cannot find a valid SMBIOS or DMI table.");
		return FWTS_ERROR;
	}

	/* Fix broken version numbers found in the wild */
	version = dmi_remap_version(fw, version);
	if (dmi_version_check(fw, version) != FWTS_OK)
		return FWTS_SKIP;

	table = fwts_mmap((off_t)entry.struct_table_address,
		         (size_t)entry.struct_table_length);
	if (table == FWTS_MAP_FAILED) {
		fwts_log_error(fw, "Cannot mmap SMBIOS tables from %8.8x..%8.8x.",
			entry.struct_table_address,
			entry.struct_table_address + entry.struct_table_length);
		return FWTS_ERROR;
	}

	dmi_scan_tables(fw, &entry, table, version);

	(void)fwts_munmap(table, (size_t)entry.struct_table_length);

	return FWTS_OK;
}

static fwts_framework_minor_test dmi_decode_tests[] = {
	{ dmi_decode_test1, "Test DMI/SMBIOS tables for errors." },
	{ NULL, NULL }
};

static fwts_framework_ops dmi_decode_ops = {
	.description = "Test DMI/SMBIOS tables for errors.",
	.minor_tests = dmi_decode_tests
};

FWTS_REGISTER(dmi_decode, &dmi_decode_ops, FWTS_TEST_ANYTIME, FWTS_BATCH | FWTS_ROOT_PRIV);

#endif
