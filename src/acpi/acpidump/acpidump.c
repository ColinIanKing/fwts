/*
 * Copyright (C) 2010-2012 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <unistd.h>

#include "fwts.h"

struct fwts_acpidump_field;

typedef void (*fwts_acpidump_field_func)(fwts_framework *fw, struct fwts_acpidump_field *info, void *data, int offset);
typedef char *(*fwts_acpidump_str_func)(uint64_t val);

typedef struct fwts_acpidump_field {
	char *label;
	int   size;
	size_t offset;
	fwts_acpidump_field_func func;
	uint8_t  bit_field_nbits;
	uint8_t  bit_field_shift;
	char  **strings;
	int   strings_len;
	fwts_acpidump_str_func str_func;
} fwts_acpidump_field;

#define OFFSET(type, field)				\
	((void*)&(((type*)(0))->field) - (void*)0)

#define FIELD(text, type, field, func, bit_field_nbits, bit_field_shift, strings, strings_len, str_func) \
	{						\
	text,						\
	sizeof(((type*)0)->field),			\
	OFFSET(type, field),				\
	func,						\
	bit_field_nbits,				\
	bit_field_shift,				\
	strings,					\
	strings_len,					\
	str_func					\
	}

#define FIELD_UINT(text, type, field)			\
	FIELD(text, type, field, acpi_dump_uint, 0, 0, NULL, 0, NULL)

#define FIELD_UINTS(text, type, field)			\
	FIELD(text, type, field, acpi_dump_uints, 0, 0, NULL, 0, NULL)

#define FIELD_STR(text, type, field)			\
	FIELD(text, type, field, acpi_dump_str, 0, 0, NULL, 0, NULL)

#define FIELD_STRS(text, type, field, strings, strings_len)			\
	FIELD(text, type, field, acpi_dump_strings, 0, 0, strings, strings_len, NULL)

#define FIELD_STRF(text, type, field, str_func)			\
	FIELD(text, type, field, acpi_dump_string_func, 0, 0, NULL, 0, str_func)

#define FIELD_GAS(text, type, field)			\
	FIELD(text, type, field, acpi_dump_gas, 0, 0, NULL, 0, NULL)

#define FIELD_BITF(text, type, field, nbits, shift)			\
	FIELD(text, type, field, acpi_dump_uint, nbits, shift, NULL, 0, NULL)

#define FIELD_GUID(text, type, field)			\
	FIELD(text, type, field, acpi_dump_guid, 0, 0, NULL, 0, NULL)

#define FIELD_END { NULL, 0, 0, NULL, 0, 0, NULL, 0, NULL}

static char *acpi_dump_field_info(char *label, int size, int offset)
{
	static char buffer[1024];

	snprintf(buffer, sizeof(buffer), "[0x%3.3x %4.4d %3d] %40.40s:",
		offset, offset, size, label);

	return buffer;
}

static void acpi_dump_str(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{
	fwts_log_info_verbatum(fw, "%s %*.*s",
		acpi_dump_field_info(info->label, info->size, info->offset + offset),
		info->size, info->size,(char*)data);
}

static uint64_t apci_dump_get_uint64_t(fwts_acpidump_field *info, void *data)
{
	uint64_t	ret;
	uint64_t	mask = (1<<info->bit_field_nbits) - 1;
	int i;

	switch (info->size) {
	case 1:
		ret  = (uint64_t)*(uint8_t*)data;
		break;
	case 2:
		ret  = (uint64_t)*(uint16_t*)data;
		break;
	case 4:
		ret  = (uint64_t)*(uint32_t*)data;
		break;
	case 8:
		ret  = (uint64_t)*(uint64_t*)data;
		break;
	default:		
		ret = 0;
		for (i=0;(i<info->size) && (i<8); i++) {
			uint8_t val8 = *(uint8_t*)data++;
			ret = (ret << 8) | val8;
		}
		break;
	}
	return mask ? (ret >> info->bit_field_shift) & mask : ret;
}


static void acpi_dump_uint(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{
	int i;
	int hexdigits = info->size << 1;
	uint64_t val = apci_dump_get_uint64_t(info, data);

	switch (info->size) {
	case 1:
	case 2:
	case 4:
	case 8:
		if (info->bit_field_nbits) {
			hexdigits = (3+info->bit_field_nbits) / 4;
			fwts_log_info_verbatum(fw, "%56.56s: 0x%*.*llx", info->label,
				hexdigits, hexdigits, (unsigned long long)val);
		} else
			fwts_log_info_verbatum(fw, "%s 0x%*.*llx",
				acpi_dump_field_info(info->label, info->size, info->offset + offset),
				hexdigits, hexdigits, (unsigned long long)val);
		break;
	default:		
		for (i=0; i<info->size; i++) {
			uint8_t val8 = *(uint8_t*)data++;
			fwts_log_info_verbatum(fw, "%s 0x%2.2x [%d]", acpi_dump_field_info(info->label, info->size, info->offset + offset), val8, i);
		}
		break;
	}
}

static void acpi_dump_uints(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{
	int n;
	int length = info->size;
	uint8_t *ptr = (uint8_t *)data;

        for (n = 0; n < length; n += 8) {
		int i;
		int j;
		int todo = length - n > 8 ? 8 : length - n;
		char buffer[128];

		for (i = 0, j = 0, *buffer = '\0'; i < todo; i++, ptr++)
                	j += snprintf(buffer + j, sizeof(buffer) - j, "%2.2x ", *ptr);

		fwts_log_info_verbatum(fw, "%s %s",
		 	acpi_dump_field_info(info->label, info->size, info->offset + offset),
			buffer);
	}
}

static void acpi_dump_guid(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{
	char guid[37];

	fwts_guid_buf_to_str(data, guid, sizeof(guid));
	fwts_log_info_verbatum(fw, "%s %s",
		acpi_dump_field_info(info->label, info->size, info->offset + offset), guid);
}

static void acpi_dump_strings(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{
	int hexdigits = info->size << 1;
	uint64_t val = apci_dump_get_uint64_t(info, data);

	fwts_log_info_verbatum(fw, "%s 0x%*.*llx (%s)", acpi_dump_field_info(info->label, info->size, info->offset + offset),
		hexdigits, hexdigits,
		(unsigned long long)val, val > info->strings_len ? "Unknown" : info->strings[val]);
}

static void acpi_dump_string_func(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{
	int hexdigits = info->size << 1;
	uint64_t val = apci_dump_get_uint64_t(info, data);

	fwts_log_info_verbatum(fw, "%s 0x%*.*llx (%s)", acpi_dump_field_info(info->label, info->size, info->offset + offset),
		hexdigits, hexdigits,
		(unsigned long long)val, info->str_func(val));
}

static void acpi_dump_table_fields(fwts_framework *fw, uint8_t *data, fwts_acpidump_field *fields, int offset, size_t table_len)
{
	fwts_acpidump_field *field = fields;

	for (field = fields; field->label != NULL; field++)
		if ((field->offset + field->size) <= table_len)
			field->func(fw, field, data + field->offset, offset);
}

static void __acpi_dump_table_fields(fwts_framework *fw, uint8_t *data, fwts_acpidump_field *fields, size_t offset)
{
	fwts_acpidump_field *field = fields;

	for (field = fields; field->label != NULL; field++)
		field->func(fw, field, data + field->offset, offset);
}

static void acpi_dump_raw_data(fwts_framework *fw, uint8_t *data, size_t length, size_t offset)
{
        int n;

        for (n = 0; n < length; n+=16) {
                int left = length - n;
		char buffer[128];
		fwts_dump_raw_data(buffer, sizeof(buffer), data + n, n + offset, left > 16 ? 16 : left);
		fwts_log_info_verbatum(fw, "%s", buffer);
        }
}

static void acpi_dump_raw_table(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_log_nl(fw);
	acpi_dump_raw_data(fw, (uint8_t *)table->data, table->length, 0);
}

static char *acpi_dump_gas_address_space_id(uint64_t index)
{
	char *txt;

	switch (index) {
	case 0x00:	
		txt = "System Memory";
		break;
	case 0x01:
		txt = "System I/O";
		break;
	case 0x02:
		txt = "PCI Configuration Space";
		break;
	case 0x03:
		txt = "Embedded Controller";
		break;
	case 0x04:
		txt = "SMBus";
		break;
	case 0x05 ... 0x7e:
	case 0x80 ... 0xbf:
		txt = "Reserved";
		break;
	case 0x7f:
		txt = "Functional Fixed Hardware";
		break;
	case 0xc0 ... 0xff:
		txt = "OEM Defined";
		break;
	default:
		txt = "Unknown";
		break;
	}
	
	return txt;
}

static void acpi_dump_gas(fwts_framework *fw, fwts_acpidump_field *info, void *data, int offset)
{	
	static char *access_width[] = {
		"Undefined (legacy reasons)",
		"Byte Access",
		"Word Access",
		"DWord Access",
		"QWord Access",
		"Unknown",
	};

	static fwts_acpidump_field fields[] = {
		FIELD_STRF("  addr_space_id", 	fwts_acpi_gas, address_space_id, acpi_dump_gas_address_space_id),
		FIELD_UINT("  reg_bit_width", 	fwts_acpi_gas, register_bit_width),
		FIELD_UINT("  reg_bit_offset",	fwts_acpi_gas, register_bit_offset),
		FIELD_STRS("  access_width", 	fwts_acpi_gas, access_width, access_width, 6),
		FIELD_UINT("  address", 	fwts_acpi_gas, address),
		FIELD_END
	};

	fwts_log_nl(fw);
	fwts_log_info_verbatum(fw, "%s (Generic Address Structure)",
		acpi_dump_field_info(info->label, info->size, info->offset));
	
	__acpi_dump_table_fields(fw, data, fields, info->offset);
}


static void acpidump_hdr(fwts_framework *fw, fwts_acpi_table_header *hdr, int length)
{
	static fwts_acpidump_field fields[] = {
		FIELD_STR ("Signature", 	fwts_acpi_table_header, signature),
		FIELD_UINT("Length", 		fwts_acpi_table_header, length),
		FIELD_UINT("Revision", 		fwts_acpi_table_header, revision),
		FIELD_UINT("Checksum", 		fwts_acpi_table_header, checksum),
		FIELD_STR ("OEM ID", 		fwts_acpi_table_header, oem_id),
		FIELD_STR ("OEM Table ID", 	fwts_acpi_table_header, oem_tbl_id),
		FIELD_UINT("OEM Revision", 	fwts_acpi_table_header, oem_revision),
		FIELD_STR ("Creator ID", 	fwts_acpi_table_header, creator_id),
		FIELD_UINT("Creator Revision", 	fwts_acpi_table_header, creator_revision),
		FIELD_END
	};

	acpi_dump_table_fields(fw, (uint8_t*)hdr, fields, 0, length);
}

static void acpidump_boot(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	uint8_t cmos_data;
	fwts_acpi_table_boot *boot = (fwts_acpi_table_boot*)data;

	fwts_acpidump_field fields[] = {
		FIELD_UINT("CMOS offset", 	fwts_acpi_table_boot, cmos_index),
		FIELD_END
	};

	if (length < (int)(sizeof(fwts_acpi_table_header) + 4)) {
		fwts_log_info(fw, "Boot table too short\n");
		return;
	}

	acpi_dump_table_fields(fw, data, fields, 0, length);

	/*
	 *  We can only run the CMOS validation if we are sure the
	 *  ACPI table was loaded directly from firmware on this
	 *  machine.  If the firmware came from file or was from
	 *  fwts (as a fixup) then we have to assume it is not from
	 *  this machine to be safe, so in this case, bail out with
	 *  out checking the CMOS settings.
	 */
	if (table->provenance != FWTS_ACPI_TABLE_FROM_FIRMWARE)
		return;

	if (fwts_cmos_read(boot->cmos_index, &cmos_data) == FWTS_OK) {
		fwts_log_info_verbatum(fw, "%56.56s: 0x%x", "Boot Register", cmos_data);
		fwts_log_info_verbatum(fw, "%56.56s: %x",  "PNP-OS", (cmos_data & FWTS_BOOT_REGISTER_PNPOS) ? 1 : 0);
		fwts_log_info_verbatum(fw, "%56.56s: %x",  "Booting", (cmos_data & FWTS_BOOT_REGISTER_BOOTING) ? 1 : 0);
		fwts_log_info_verbatum(fw, "%56.56s: %x",  "Diag", (cmos_data & FWTS_BOOT_REGISTER_DIAG) ? 1 : 0);
		fwts_log_info_verbatum(fw, "%56.56s: %x",  "Suppress", (cmos_data & FWTS_BOOT_REGISTER_SUPPRESS_BOOT_DISPLAY) ? 1 : 0);
		fwts_log_info_verbatum(fw, "%56.56s: %x",  "Parity", (cmos_data & FWTS_BOOT_REGISTER_PARITY) ? 1 : 0);
	} else
		fwts_log_error(fw, "Cannot get read/write permission on I/O ports.");
	
}

static void acpidump_bert(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	fwts_acpi_table_bert *bert = (fwts_acpi_table_bert*)data;

	static char *error_severity[] = {
		"Correctable",
		"Fatal",
		"Corrected",
		"None"
	};

	int n = length - sizeof(fwts_acpi_table_bert);

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Region Length", 	fwts_acpi_table_bert, boot_error_region_length),
		FIELD_UINT("Region Addr", 	fwts_acpi_table_bert, boot_error_region),
		FIELD_UINT("Boot Status", 	fwts_acpi_table_bert, boot_status),
		FIELD_UINT("Raw Data Offset", 	fwts_acpi_table_bert, raw_data_offset),
		FIELD_UINT("Raw Data Length", 	fwts_acpi_table_bert, raw_data_length),
		FIELD_UINT("Error Severity", 	fwts_acpi_table_bert, error_severity),
		FIELD_STRS("Generic Error Data",fwts_acpi_table_bert, generic_error_data, error_severity, 4),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, length, length);
	fwts_log_nl(fw);
	acpi_dump_raw_data(fw, bert->generic_error_data, n, sizeof(fwts_acpi_table_bert));
}

static void acpidump_cpep(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	int i;
	int n = (length - sizeof(fwts_acpi_table_bert)) / sizeof(fwts_acpi_cpep_processor_info);

	for (i=0; i<n; i++) {
		fwts_acpidump_field fields[] = {
			FIELD_UINT("  Type", 		fwts_acpi_table_cpep, cpep_info[i].type),
			FIELD_UINT("  Length", 		fwts_acpi_table_cpep, cpep_info[i].length),
			FIELD_UINT("  Processor ID", 	fwts_acpi_table_cpep, cpep_info[i].processor_id),
			FIELD_UINT("  Processor EID", 	fwts_acpi_table_cpep, cpep_info[i].processor_eid),
			FIELD_UINT("  Poll Interval (ms)",fwts_acpi_table_cpep, cpep_info[i].polling_interval),
			FIELD_END
		};
		fwts_log_info_verbatum(fw, "CPEP #%d\n", i);
		__acpi_dump_table_fields(fw, data, fields, 0);
	};
}

static void acpidump_ecdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	fwts_acpi_table_ecdt *ecdt = (fwts_acpi_table_ecdt*)data;
	int n = length - sizeof(fwts_acpi_table_ecdt);

	static fwts_acpidump_field fields[] = {
		FIELD_GAS ("EC_CONTROL",fwts_acpi_table_ecdt,   ec_control),
		FIELD_GAS ("EC_DATA", 	fwts_acpi_table_ecdt,   ec_data),
		FIELD_UINT("UID", 	fwts_acpi_table_ecdt,	uid),
		FIELD_UINT("GPE_BIT", 	fwts_acpi_table_ecdt,	gpe_bit),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);

	fwts_log_info_verbatum(fw, "EC_ID:");
	fwts_log_nl(fw);
	acpi_dump_raw_data(fw, ecdt->ec_id, n, sizeof(fwts_acpi_table_ecdt));
}

static void acpidump_erst(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	int i;

	static char *serialization_actions[] = {
		"BEGIN_WRITE_OPERATION",
		"BEGIN_READ_OPERATION",
		"BEGIN_CLEAR_OPERATION",
		"END_OPERATION",
		
		"SET_RECORD_OFFSET",
		"EXECUTE_OPERATION",
		"CHECK_BUSY_STATUS",
		"GET_COMMAND_STATUS",

		"GET_RECORD_IDENTIFIER",
		"SET_RECORD_IDENTIFIER",
		"GET_RECOERD_COUNT",		
		"BEGIN_DUMMY_WRITE_OPERATION",
		
		"RESERVED",
		"GET_ERROR_LOG_ADDRESS_RANGE",
		"GET_ERROR_LOG_ADDRESS_RANGE_LENGTH",
		"GET_ERROR_LOG_ADDRESS_RANGE_ATTRIBUTES"
	};

	static char *instructions[] = {
		"READ_REGISTER",
		"READ_REGISTER_VALUE",
		"WRITE_REGISTER",
		"WRITE_REGISTER_VALUE",

		"NOOP",
		"LOAD_VAR1",
		"LOAD_VAR2",
		"STORE_VAR1",	
	
		"ADD",
		"SUBTRACT",
		"ADD_VALUE",
		"SUBTRACT_VALUE",

		"STALL",
		"STALL_WHILE_TRUE",
		"SKIP_NEXT_INSTRUCTION_IF_TRUE",
		"GOTO",

		"SET_SRC_ADDRESS_BASE",
		"SET_DST_ADDRESS_BASE",
		"MOVE_DATA"
	};

	fwts_acpi_table_erst *erst = (fwts_acpi_table_erst*)data;

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Serialization Hdr. Size", fwts_acpi_table_erst, serialization_header_size),
		FIELD_UINT("Insrtuction Entry Count", fwts_acpi_table_erst, instruction_entry_count),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
	
	for (i=0; i<erst->instruction_entry_count; i++) {
		fwts_acpidump_field entry_fields[] = {
			FIELD_STRS("  Action", 		fwts_acpi_table_erst, entries[i].serialization_action, serialization_actions, 16),
			FIELD_STRS("  Instruction", 	fwts_acpi_table_erst, entries[i].instruction, instructions, 19),
			FIELD_UINT("  Flags", 		fwts_acpi_table_erst, entries[i].flags),
			FIELD_GAS ("  Register region", fwts_acpi_table_erst, entries[i].register_region),
			FIELD_UINT("  Value", 		fwts_acpi_table_erst, entries[i].value),
			FIELD_UINT("  Mask", 		fwts_acpi_table_erst, entries[i].mask),
			FIELD_END
		};
		fwts_log_info_verbatum(fw, "Entry #%d", i+1);
		__acpi_dump_table_fields(fw, data, entry_fields, 0);
	}
}

static void acpidump_amlcode(fwts_framework *fw, fwts_acpi_table_info *table)
{
	fwts_log_info_verbatum(fw, "Contains 0x%lx bytes of AML byte code",
		(int long)table->length-sizeof(fwts_acpi_table_header));
}

static void acpidump_facs(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field fields[] = {
		FIELD_STR ("Signature", 	fwts_acpi_table_facs, 	signature),
		FIELD_UINT("Length", 		fwts_acpi_table_facs,	length),
		FIELD_UINT("H/W Signature", 	fwts_acpi_table_facs,	hardware_signature),
		FIELD_UINT("Waking Vector", 	fwts_acpi_table_facs,	firmware_waking_vector),
		FIELD_UINT("Global Lock", 	fwts_acpi_table_facs,	global_lock),
		FIELD_UINT("Flags", 		fwts_acpi_table_facs,	flags),
		FIELD_UINT("X Waking Vector", 	fwts_acpi_table_facs,	x_firmware_waking_vector),
		FIELD_UINT("Version", 		fwts_acpi_table_facs,	version),
		FIELD_UINT("OSPM Flags", 	fwts_acpi_table_facs,	ospm_flags),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
}

static void acpidump_hpet(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Event Timer ID", 	fwts_acpi_table_hpet, event_timer_block_id),
		FIELD_BITF("  Hardware Rev", 	fwts_acpi_table_hpet, event_timer_block_id, 8, 0),
		FIELD_BITF("  Num Comparitors", fwts_acpi_table_hpet, event_timer_block_id, 5, 8),
		FIELD_BITF("  Count Size Cap", 	fwts_acpi_table_hpet, event_timer_block_id, 1, 13),
		FIELD_BITF("  Reserved", 	fwts_acpi_table_hpet, event_timer_block_id, 1, 14),
		FIELD_BITF("  IRQ Routing Cap", fwts_acpi_table_hpet, event_timer_block_id, 1, 15),
		FIELD_BITF("  PCI Vendor ID", 	fwts_acpi_table_hpet, event_timer_block_id, 16, 16),
		FIELD_GAS ("Base Address", 	fwts_acpi_table_hpet, base_address),
		FIELD_UINT("HPET Number", 	fwts_acpi_table_hpet, hpet_number),
		FIELD_UINT("Main Counter Min", 	fwts_acpi_table_hpet, main_counter_minimum),
		FIELD_UINT("Page Prot Attr", 	fwts_acpi_table_hpet, page_prot_and_oem_attribute),
		FIELD_BITF("  Page Protection", fwts_acpi_table_hpet, page_prot_and_oem_attribute, 4, 0),
		FIELD_BITF("  OEM Attr", 	fwts_acpi_table_hpet, page_prot_and_oem_attribute, 4, 4),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
}

static void acpidump_fadt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("FACS Address", 	fwts_acpi_table_fadt, firmware_control),
		FIELD_UINT("DSDT Address", 	fwts_acpi_table_fadt, dsdt),
		FIELD_UINT("Model",		fwts_acpi_table_fadt, reserved),
		FIELD_STRS("PM Profile",	fwts_acpi_table_fadt, preferred_pm_profile, fwts_acpi_fadt_preferred_pm_profile, 8),
		FIELD_UINT("SCI Interrupt", 	fwts_acpi_table_fadt, sci_int),
		FIELD_UINT("SMI Command Port",	fwts_acpi_table_fadt, smi_cmd),
		FIELD_UINT("ACPI Enable Value", fwts_acpi_table_fadt, acpi_enable),
		FIELD_UINT("ACPI Disable Value",fwts_acpi_table_fadt, acpi_disable),
		FIELD_UINT("S4BIOS Command", 	fwts_acpi_table_fadt, s4bios_req),
		FIELD_UINT("P-State Control",	fwts_acpi_table_fadt, pstate_cnt),
		FIELD_UINT("PM1A Event Block", 	fwts_acpi_table_fadt, pm1a_evt_blk),
		FIELD_UINT("PM1B Event Block", 	fwts_acpi_table_fadt, pm1b_evt_blk),
		FIELD_UINT("PM1A Control Block",fwts_acpi_table_fadt, pm1a_cnt_blk),
		FIELD_UINT("PM1B Control Block",fwts_acpi_table_fadt, pm1b_cnt_blk),
		FIELD_UINT("PM2 Control Block", fwts_acpi_table_fadt, pm2_cnt_blk),
		FIELD_UINT("PM Timer Block", 	fwts_acpi_table_fadt, pm_tmr_blk),
		FIELD_UINT("GPE0 Block", 	fwts_acpi_table_fadt, gpe0_blk),
		FIELD_UINT("GPE1 Block", 	fwts_acpi_table_fadt, gpe1_blk),
		FIELD_UINT("PM1 Event Block Length", 			fwts_acpi_table_fadt, pm1_evt_len),
		FIELD_UINT("PM1 Control Block Length", 			fwts_acpi_table_fadt, pm1_cnt_len),
		FIELD_UINT("PM2 Control Block Length", 			fwts_acpi_table_fadt, pm2_cnt_len),
		FIELD_UINT("PM Timer Block Length",			fwts_acpi_table_fadt, pm_tmr_len),
		FIELD_UINT("GPE0 Block Length", 			fwts_acpi_table_fadt, gpe0_blk_len),
		FIELD_UINT("GPE1 Block Length", 			fwts_acpi_table_fadt, gpe1_blk_len),
		FIELD_UINT("_CST Support", 				fwts_acpi_table_fadt, cst_cnt),
		FIELD_UINT("C2 Latency", 				fwts_acpi_table_fadt, p_lvl2_lat),
		FIELD_UINT("C3 Latency", 				fwts_acpi_table_fadt, p_lvl3_lat),
		FIELD_UINT("CPU Cache Size", 				fwts_acpi_table_fadt, flush_size),
		FIELD_UINT("CPU Cache Flush Stride", 			fwts_acpi_table_fadt, flush_stride),
		FIELD_UINT("Duty Cycle Offset", 			fwts_acpi_table_fadt, duty_offset),
		FIELD_UINT("Duty Cycle Width", 				fwts_acpi_table_fadt, duty_width),
		FIELD_UINT("RTC Day Alarm Index", 			fwts_acpi_table_fadt, day_alrm),
		FIELD_UINT("RTC Month Alarm Index", 			fwts_acpi_table_fadt, mon_alrm),
		FIELD_UINT("RTC Century Index", 			fwts_acpi_table_fadt, century),
		FIELD_UINT("IA-PC Boot Flags (see below)", 		fwts_acpi_table_fadt, iapc_boot_arch),
		FIELD_BITF("  Legacy Devices Supported (V2)",		fwts_acpi_table_fadt, iapc_boot_arch, 1, 0),
		FIELD_BITF("  8042 present on ports 60/64 (V2)",	fwts_acpi_table_fadt, iapc_boot_arch, 1, 1),
		FIELD_BITF("  VGA Not Present (V4)",			fwts_acpi_table_fadt, iapc_boot_arch, 1, 2),
		FIELD_BITF("  MSI Not Supported (V4)",			fwts_acpi_table_fadt, iapc_boot_arch, 1, 3),
		FIELD_BITF("  PCIe ASPM Not supported (V4)",		fwts_acpi_table_fadt, iapc_boot_arch, 1, 4),
		FIELD_UINT("Flags (see below)", 			fwts_acpi_table_fadt, flags),
		FIELD_BITF("  WBINVD instruction (V1)", 		fwts_acpi_table_fadt, flags, 1, 0),
		FIELD_BITF("  WBINVD flushes all caches (V1)", 		fwts_acpi_table_fadt, flags, 1, 1),
		FIELD_BITF("  All CPUs support C1 (V1)", 		fwts_acpi_table_fadt, flags, 1, 2),
		FIELD_BITF("  C2 works on MP system (V1)", 		fwts_acpi_table_fadt, flags, 1, 3),
		FIELD_BITF("  Control Method Power Button (V1)", 	fwts_acpi_table_fadt, flags, 1, 4),
		FIELD_BITF("  Control Method Sleep Button (V1)", 	fwts_acpi_table_fadt, flags, 1, 5),
		FIELD_BITF("  RTC wake not in fixed reg space (V1)", 	fwts_acpi_table_fadt, flags, 1, 6),
		FIELD_BITF("  RTC can wake system from S4 (V1)", 	fwts_acpi_table_fadt, flags, 1, 7),
		FIELD_BITF("  32 bit PM timer (V1)", 			fwts_acpi_table_fadt, flags, 1, 8),
		FIELD_BITF("  Docking Capability (V1)", 		fwts_acpi_table_fadt, flags, 1, 9),
		FIELD_BITF("  Reset Register Supported (V2)", 		fwts_acpi_table_fadt, flags, 1, 10),
		FIELD_BITF("  Sealed Case (V3)", 			fwts_acpi_table_fadt, flags, 1, 11),
		FIELD_BITF("  Headless, No Video (V3)", 		fwts_acpi_table_fadt, flags, 1, 12),
		FIELD_BITF("  Use native instr. after SLP_TYPx (V3)",	fwts_acpi_table_fadt, flags, 1, 13),
		FIELD_BITF("  PCI_EXP_WAK bits supported (V4)", 	fwts_acpi_table_fadt, flags, 1, 14),
		FIELD_BITF("  Use patform clock (V4)", 			fwts_acpi_table_fadt, flags, 1, 15),
		FIELD_BITF("  RTC_STS Valid after S4 wake (V4)", 	fwts_acpi_table_fadt, flags, 1, 16),
		FIELD_BITF("  Remote power on capable (V4)", 		fwts_acpi_table_fadt, flags, 1, 17),
		FIELD_BITF("  Use APIC Cluster Model (V4)", 		fwts_acpi_table_fadt, flags, 1, 18),
		FIELD_BITF("  Use APIC Physical Dest. Mode (V4)", 	fwts_acpi_table_fadt, flags, 1, 19),
		FIELD_BITF("  ACPI hardware is not implemented (V5)",	fwts_acpi_table_fadt, flags, 1, 20),
		FIELD_BITF("  S0 power savings are equal or better than S3 (V5)", fwts_acpi_table_fadt, flags, 1, 21),
		FIELD_BITF("  RESERVED", 	fwts_acpi_table_fadt, flags, 10, 22),
		FIELD_GAS ("RESET_REG", 	fwts_acpi_table_fadt, reset_reg),
		FIELD_UINT("RESET_VALUE", 	fwts_acpi_table_fadt, reset_value),
		FIELD_UINT("X_FIRMWARE_CTRL", 	fwts_acpi_table_fadt, x_firmware_ctrl),
		FIELD_UINT("X_DSDT", 		fwts_acpi_table_fadt, x_dsdt),
		FIELD_GAS ("X_PM1a_EVT_BLK", 	fwts_acpi_table_fadt, x_pm1a_evt_blk),
		FIELD_GAS ("X_PM1b_EVT_BLK", 	fwts_acpi_table_fadt, x_pm1b_evt_blk),
		FIELD_GAS ("X_PM1a_CNT_BLK", 	fwts_acpi_table_fadt, x_pm1a_cnt_blk),
		FIELD_GAS ("X_PM1b_CNT_BLK", 	fwts_acpi_table_fadt, x_pm1b_cnt_blk),
		FIELD_GAS ("X_PM2_CNT_BLK", 	fwts_acpi_table_fadt, x_pm2_cnt_blk),
		FIELD_GAS ("X_PM_TMR_BLK", 	fwts_acpi_table_fadt, x_pm_tmr_blk),
		FIELD_GAS ("X_GPE0_BLK", 	fwts_acpi_table_fadt, x_gpe0_blk),
		FIELD_GAS ("X_GPE1_BLK", 	fwts_acpi_table_fadt, x_gpe1_blk),
		FIELD_GAS ("SLEEP_CONTROL_REG", fwts_acpi_table_fadt, sleep_control_reg),
		FIELD_GAS ("SLEEP_STATUS_REG", 	fwts_acpi_table_fadt, sleep_status_reg),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);

}

static void acpidump_rsdp(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field fields[] = {
		FIELD_STR ("Signature", 	fwts_acpi_table_rsdp, signature),
		FIELD_UINT("Checksum", 		fwts_acpi_table_rsdp, checksum),
		FIELD_STR ("OEM ID", 		fwts_acpi_table_rsdp, oem_id),
		FIELD_UINT("Revision", 		fwts_acpi_table_rsdp, revision),
		FIELD_UINT("RsdtAddress", 	fwts_acpi_table_rsdp, rsdt_address),
		FIELD_UINT("Length", 		fwts_acpi_table_rsdp, length),
		FIELD_UINT("XsdtAddress", 	fwts_acpi_table_rsdp, xsdt_address),
		FIELD_UINT("Extended Checksum", fwts_acpi_table_rsdp, extended_checksum),
		FIELD_UINT("Reserved",		fwts_acpi_table_rsdp, reserved),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
}

static void acpidump_rsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	int i;
	int n;
	fwts_acpi_table_rsdt *rsdt = (fwts_acpi_table_rsdt*)data;

	n = (length - sizeof(fwts_acpi_table_header)) / sizeof(uint32_t);
	for (i=0; i<n; i++)  {
		char label[80];
		fwts_acpi_table_info *table;

		if (fwts_acpi_find_table_by_addr(fw, (uint64_t)rsdt->entries[i], &table) == FWTS_OK) {
			char *name = table == NULL ? "unknown" : table->name;
			snprintf(label, sizeof(label), "Entry %2.2d %s", i, name);
			fwts_log_info_verbatum(fw, "%s 0x%8.8x",
				acpi_dump_field_info(label, sizeof(rsdt->entries[i]), OFFSET(fwts_acpi_table_rsdt, entries[i])),
				rsdt->entries[i]);
		}
	}
}

static void acpidump_sbst(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Warn. Energy Level", 	fwts_acpi_table_sbst,	warning_energy_level),
		FIELD_UINT("Low  Energy Level", 	fwts_acpi_table_sbst,	low_energy_level),
		FIELD_UINT("Crit. Energy Level", 	fwts_acpi_table_sbst,	critical_energy_level),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
}

static void acpidump_xsdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	int i;
	int n;
	fwts_acpi_table_xsdt *xsdt = (fwts_acpi_table_xsdt*)data;

	n = (length - sizeof(fwts_acpi_table_header)) / sizeof(uint64_t);
	for (i=0; i<n; i++)  {
		char label[80];
		fwts_acpi_table_info *table;

		if (fwts_acpi_find_table_by_addr(fw, xsdt->entries[i], &table) == FWTS_OK) {
			char *name = table == NULL ? "unknown" : table->name;
			snprintf(label, sizeof(label), "Entry %2.2d %s", i, name);
			fwts_log_info_verbatum(fw, "%s 0x%16.16llx",
				acpi_dump_field_info(label, sizeof(xsdt->entries[i]), OFFSET(fwts_acpi_table_xsdt, entries[i])),
				(unsigned long long)xsdt->entries[i]);
		}
	}
}

static void acpidump_madt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	int i = 0;
	size_t offset = 0;
	
	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Local APIC Address", 	fwts_acpi_table_madt, lapic_address),
		FIELD_UINT("Flags", 			fwts_acpi_table_madt, flags),
		FIELD_BITF("  PCAT_COMPAT", 		fwts_acpi_table_madt, flags, 1, 0),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);
	
	data += sizeof(fwts_acpi_table_madt);	
	length -= sizeof(fwts_acpi_table_madt);

	while (length > (int)sizeof(fwts_acpi_madt_sub_table_header)) {
		int skip = 0;
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);
		offset += sizeof(fwts_acpi_madt_sub_table_header);
		
		fwts_log_nl(fw);
		fwts_log_info_verbatum(fw, "APIC Structure #%d:", ++i);

		switch (hdr->type) {
		case 0: {
				static fwts_acpidump_field fields_processor_local_apic[] = {
					FIELD_UINT("  ACPI CPU ID", fwts_acpi_madt_processor_local_apic, acpi_processor_id),
					FIELD_UINT("  APIC ID",     fwts_acpi_madt_processor_local_apic, apic_id),
					FIELD_UINT("  Flags",       fwts_acpi_madt_processor_local_apic, flags),
					FIELD_BITF("   Enabled",    fwts_acpi_madt_processor_local_apic, flags, 1, 0),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Processor Local APIC:");
				__acpi_dump_table_fields(fw, data, fields_processor_local_apic, offset);
				skip = sizeof(fwts_acpi_madt_processor_local_apic);
			}
			break;
		case 1: {
				static fwts_acpidump_field fields_io_apic[] = {
					FIELD_UINT("  I/O APIC ID", 	fwts_acpi_madt_io_apic, io_apic_id),
					FIELD_UINT("  I/O APIC Addr", 	fwts_acpi_madt_io_apic, io_apic_phys_address),
					FIELD_UINT("  Global IRQ Base", fwts_acpi_madt_io_apic, global_irq_base),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " I/O APIC:");
				__acpi_dump_table_fields(fw, data, fields_io_apic, offset);
				skip = sizeof(fwts_acpi_madt_io_apic);
			}
			break;
		case 2: {
				static fwts_acpidump_field fields_madt_interrupt_override[] = {
					FIELD_UINT("  Bus", 		fwts_acpi_madt_interrupt_override, bus),
					FIELD_UINT("  Source", 		fwts_acpi_madt_interrupt_override, source),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_interrupt_override, gsi),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_interrupt_override, flags),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Interrupt Source Override:");
				__acpi_dump_table_fields(fw, data, fields_madt_interrupt_override, offset);
				skip = sizeof(fwts_acpi_madt_interrupt_override);
			}
			break;
		case 3: {
				static fwts_acpidump_field fields_madt_nmi[] = {
					FIELD_UINT("  Flags", 		fwts_acpi_madt_nmi, flags),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_nmi, gsi),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Non-maskable Interrupt Source (NMI):");
				__acpi_dump_table_fields(fw, data, fields_madt_nmi, offset);
				skip = sizeof(fwts_acpi_madt_nmi);
			}
			break;
		case 4: {
				static fwts_acpidump_field fields_madt_local_apic_nmi[] = {
					FIELD_UINT("  ACPI CPU ID", 	fwts_acpi_madt_local_apic_nmi, acpi_processor_id),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_apic_nmi, flags),
					FIELD_UINT("  Local APIC LINT", fwts_acpi_madt_local_apic_nmi, local_apic_lint),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local APIC NMI:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_apic_nmi, offset);
				skip = sizeof(fwts_acpi_madt_local_apic_nmi);
			}
			break;
		case 5: {
				static fwts_acpidump_field fields_madt_local_apic_addr_override[] = {
					FIELD_UINT("  Local APIC Addr", fwts_acpi_madt_local_apic_addr_override, address),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local APIC Address Override:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_apic_addr_override, offset);
				skip = sizeof(fwts_acpi_madt_local_apic_addr_override);
			}
			break;
		case 6: {
				static fwts_acpidump_field fields_madt_io_sapic[] = {
					FIELD_UINT("  I/O SAPIC ID", 	fwts_acpi_madt_io_sapic, io_sapic_id),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_io_sapic, gsi),
					FIELD_UINT("  I/O SAPIC Addr", 	fwts_acpi_madt_io_sapic, address),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " I/O SAPIC:");
				__acpi_dump_table_fields(fw, data, fields_madt_io_sapic, offset);
				skip = sizeof(fwts_acpi_madt_io_sapic);
			}
			break;
		case 7: {
				fwts_acpi_madt_local_sapic *local_sapic = (fwts_acpi_madt_local_sapic*)data;
				static fwts_acpidump_field fields_madt_local_sapic[] = {
					FIELD_UINT("  ACPI CPU ID", 	fwts_acpi_madt_local_sapic, acpi_processor_id),
					FIELD_UINT("  Local SAPIC ID", 	fwts_acpi_madt_local_sapic, local_sapic_id),
					FIELD_UINT("  Local SAPIC EID",	fwts_acpi_madt_local_sapic, local_sapic_eid),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_sapic, flags),
					FIELD_UINT("  UID Value", 	fwts_acpi_madt_local_sapic, uid_value),
					FIELD_UINT("  UID String", 	fwts_acpi_madt_local_sapic, uid_string),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local SAPIC:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_sapic, offset);
				skip = (sizeof(fwts_acpi_madt_local_sapic) +
					strlen(local_sapic->uid_string) + 1);
			}
			break;
		case 8: {
				static fwts_acpidump_field fields_madt_local_sapic[] = {
					FIELD_UINT("  Flags", 		fwts_acpi_madt_platform_int_source, flags),
					FIELD_UINT("  Type", 		fwts_acpi_madt_platform_int_source, type),
					FIELD_UINT("  Processor ID", 	fwts_acpi_madt_platform_int_source, processor_id),
					FIELD_UINT("  Processor EID", 	fwts_acpi_madt_platform_int_source, processor_eid),
					FIELD_UINT("  IO SAPIC Vector", fwts_acpi_madt_platform_int_source, io_sapic_vector),
					FIELD_UINT("  Gbl Sys Int", 	fwts_acpi_madt_platform_int_source, gsi),
					FIELD_UINT("  PIS Flags", 	fwts_acpi_madt_platform_int_source, pis_flags),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Platform Interrupt Sources:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_sapic, offset);
				skip = (sizeof(fwts_acpi_madt_platform_int_source));
			}
			break;
		case 9: {
				static fwts_acpidump_field fields_madt_local_x2apic[] = {
					FIELD_UINT("  x2APIC ID", 	fwts_acpi_madt_local_x2apic, x2apic_id),
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_x2apic, flags),
					FIELD_UINT("  Processor UID", 	fwts_acpi_madt_local_x2apic, processor_uid),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Processor Local x2APIC:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_x2apic, offset);
				skip = (sizeof(fwts_acpi_madt_local_x2apic));
			}
			break;
		case 10: {
				static fwts_acpidump_field fields_madt_local_x2apic_nmi[] = {
					FIELD_UINT("  Flags", 		fwts_acpi_madt_local_x2apic_nmi, flags),
					FIELD_UINT("  Processor UID", 	fwts_acpi_madt_local_x2apic_nmi, processor_uid),
					FIELD_UINT("  LINT#", 		fwts_acpi_madt_local_x2apic_nmi, local_x2apic_lint),
					FIELD_END
				};
				fwts_log_info_verbatum(fw, " Local x2APIC NMI:");
				__acpi_dump_table_fields(fw, data, fields_madt_local_x2apic_nmi, offset);
				skip = (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			}
			break;
		default:
			fwts_log_info_verbatum(fw, " Reserved for OEM use:");
			skip = 0;
			break;
		}
		data   += skip;
		offset += skip;
		length -= skip;
	}
}

static void acpidump_mcfg(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	fwts_acpi_table_mcfg *mcfg = (fwts_acpi_table_mcfg*)data;
	int n;
	int i;

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Base Address", 	fwts_acpi_table_mcfg, 	base_address),
		FIELD_UINT("Base Reserved", 	fwts_acpi_table_mcfg,	base_reserved),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, 0, length);

	n = length - sizeof(fwts_acpi_table_mcfg);
	fwts_acpi_mcfg_configuration *config = mcfg->configuration;

	for (i=0; i<n/sizeof(fwts_acpi_mcfg_configuration); i++) {
		fwts_acpidump_field fields_config[] = {
			FIELD_UINT("  Base Address", 	fwts_acpi_table_mcfg,	configuration[i].base_address),
			FIELD_UINT("  Base Address", 	fwts_acpi_table_mcfg,	configuration[i].base_address),
			FIELD_UINT("  Base Reserved", 	fwts_acpi_table_mcfg,	configuration[i].base_reserved),
			FIELD_UINT("  PCI Seg Grp Num", fwts_acpi_table_mcfg,	configuration[i].pci_segment_group_number),
			FIELD_UINT("  Start Bus Num", 	fwts_acpi_table_mcfg,	configuration[i].start_bus_number),
			FIELD_UINT("  End Bus Num", 	fwts_acpi_table_mcfg,	configuration[i].end_bus_number),
			FIELD_END
		};
		fwts_log_info_verbatum(fw, "Configuration #%d:", i+1);
		acpi_dump_table_fields(fw, (uint8_t*)config, fields_config, 0, length);
		config++;
	}
}

static void acpidump_slit(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	fwts_acpi_table_slit *slit = (fwts_acpi_table_slit*)data;
	int i;
	int j = 0;
	int k = 0;
	int n = length - sizeof(fwts_acpi_table_slit);
	uint8_t *entry;

	fwts_log_info_verbatum(fw, "# Sys Localities: 0x%llx (%llu)",
				(unsigned long long)slit->num_of_system_localities,
				(unsigned long long)slit->num_of_system_localities);
	if (n < slit->num_of_system_localities * slit->num_of_system_localities) {
		fwts_log_info_verbatum(fw,"Expecting %lld bytes, got only %d",
			(unsigned long long)(slit->num_of_system_localities * slit->num_of_system_localities), n);
	}
	else {
		entry = data + sizeof(fwts_acpi_table_slit);
		for (i=0; i<n; i++) {
			fwts_log_info_verbatum(fw, "Entry[%2.2d][%2.2d]: %2.2x", j, k, *entry++);
			k++;
			if (k >= slit->num_of_system_localities) {
				k = 0;
				j++;
			}
		}
	}
};


static void acpidump_srat(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	uint8_t *ptr;
	int   offset;

	ptr = data + sizeof(fwts_acpi_table_srat);
	offset = sizeof(fwts_acpi_table_srat);

	while (ptr < data + length) {
		switch (*ptr) {
		case 0:	{
				static fwts_acpidump_field fields_lasa[] = {
					FIELD_UINT("  Proximity [7:0]", fwts_acpi_table_slit_local_apic_sapic_affinity, proximity_domain_0),
					FIELD_UINT("  APIC ID", fwts_acpi_table_slit_local_apic_sapic_affinity, apic_id),
					FIELD_UINT("  Flags", fwts_acpi_table_slit_local_apic_sapic_affinity, flags),
					FIELD_UINT("  L_SAPIC_EID", fwts_acpi_table_slit_local_apic_sapic_affinity, local_sapic_eid),
					FIELD_UINT("  Proximity [31:24]",fwts_acpi_table_slit_local_apic_sapic_affinity,  proximity_domain_3),
					FIELD_UINT("  Proximity [23:16]",fwts_acpi_table_slit_local_apic_sapic_affinity,  proximity_domain_2),
					FIELD_UINT("  Proximity [15:8]",fwts_acpi_table_slit_local_apic_sapic_affinity,   proximity_domain_1),
					FIELD_UINT("  Clock Domain:   0x%lx", fwts_acpi_table_slit_local_apic_sapic_affinity, clock_domain),
					FIELD_END,
				};
				fwts_log_info_verbatum(fw, " Processor Local APIC/SAPID Affinity Structure:");
				__acpi_dump_table_fields(fw, ptr, fields_lasa, offset);
				ptr += sizeof(fwts_acpi_table_slit_local_apic_sapic_affinity);
				offset += sizeof(fwts_acpi_table_slit_local_apic_sapic_affinity);
			}
			break;
		case 1:	{
				static fwts_acpidump_field fields_ma[] = {
					FIELD_UINT("  Prox. Domain", fwts_acpi_table_slit_memory_affinity, proximity_domain),
					FIELD_UINT("  Base Addr Lo", fwts_acpi_table_slit_memory_affinity, base_addr_lo),
					FIELD_UINT("  Base Addr Hi", fwts_acpi_table_slit_memory_affinity, base_addr_hi),
					FIELD_UINT("  Length Lo", fwts_acpi_table_slit_memory_affinity, length_lo),
					FIELD_UINT("  Length Hi", fwts_acpi_table_slit_memory_affinity, length_hi),
					FIELD_UINT("  Flags", fwts_acpi_table_slit_memory_affinity, flags),
					FIELD_END,
				};
				fwts_log_info_verbatum(fw, " Memory Affinity Structure:");
				__acpi_dump_table_fields(fw, ptr, fields_ma, offset);
				ptr += sizeof(fwts_acpi_table_slit_memory_affinity);
				offset += sizeof(fwts_acpi_table_slit_memory_affinity);
			}
			break;
		case 2: {
				static fwts_acpidump_field fields_xa[] = {
					FIELD_UINT("  Prox. Domain", fwts_acpi_table_slit_local_x2apic_affinity, proximity_domain),
					FIELD_UINT("  X2APIC ID",    fwts_acpi_table_slit_local_x2apic_affinity, x2apic_id),
					FIELD_UINT("  Flags",        fwts_acpi_table_slit_local_x2apic_affinity, flags),
					FIELD_UINT("  Clock Domain", fwts_acpi_table_slit_local_x2apic_affinity, clock_domain),
					FIELD_END,
				};
				fwts_log_info_verbatum(fw, " Processor Local x2APIC Affinit Structure:");
				__acpi_dump_table_fields(fw, ptr, fields_xa, offset);
				ptr += sizeof(fwts_acpi_table_slit_local_x2apic_affinity);
				offset += sizeof(fwts_acpi_table_slit_local_x2apic_affinity);
			}
			break;
		default:
			ptr = data + length; /* force abort */
			break;
		}
	}
}

static void acpidump_tcpa(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field fields[] = {
		FIELD_UINT("Reserved", 		fwts_acpi_table_tcpa, reserved),
		FIELD_UINT("Log Zone Length", 	fwts_acpi_table_tcpa, log_zone_length),
		FIELD_UINT("Log Zone Address", 	fwts_acpi_table_tcpa, log_zone_addr),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields, length, length);
}

/*
 *  acpidump_asf()
 *	dump out ASF! descruption table
 *	see: http://dmtf.org/documents/asf/alert-standard-format-asf-specification-200
 */
static void acpidump_asf(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	uint8_t *ptr;

	ptr = data + sizeof(fwts_acpi_table_header);

	static fwts_acpidump_field asf_info_fields[] = {
		FIELD_UINT("Watchdog Reset Value", 	fwts_acpi_table_asf_info, watchdog_reset_value),
		FIELD_UINT("Min Sensor Poll Wait Time", fwts_acpi_table_asf_info, min_sensor_poll_wait_time),
		FIELD_UINT("System ID",			fwts_acpi_table_asf_info, id),
		FIELD_UINT("IANA ID",			fwts_acpi_table_asf_info, iana_id),
		FIELD_UINT("Feature Flags",		fwts_acpi_table_asf_info, flags),
		FIELD_UINT("Reserved1",			fwts_acpi_table_asf_info, reserved1),
		FIELD_UINT("Reserved2",			fwts_acpi_table_asf_info, reserved2),
		FIELD_UINT("Reserved3",			fwts_acpi_table_asf_info, reserved3),
		FIELD_END
	};

	static fwts_acpidump_field asf_alrt_fields[] = {
		FIELD_UINT("Assertion Event Bit Mask", 	fwts_acpi_table_asf_alrt, assertion_mask),
		FIELD_UINT("De-assertion Event Bit Mask", fwts_acpi_table_asf_alrt, deassertion_mask),
		FIELD_UINT("Number of Alerts", 		fwts_acpi_table_asf_alrt, number_of_alerts),
		FIELD_UINT("Array Element Length", 	fwts_acpi_table_asf_alrt, array_length),
		FIELD_END
	};

	static fwts_acpidump_field asf_alrt_element_fields[] = {
		FIELD_UINT("Alert Device Address", 	fwts_acpi_table_asf_alrt_element, device_addr),
		FIELD_UINT("Alert Command", 		fwts_acpi_table_asf_alrt_element, command),
		FIELD_UINT("Alert Data Mask", 		fwts_acpi_table_asf_alrt_element, data_mask),
		FIELD_UINT("Alert Compare Value", 	fwts_acpi_table_asf_alrt_element, compare_value),
		FIELD_UINT("Alert Event Sensor Type", 	fwts_acpi_table_asf_alrt_element, sensor_type),
		FIELD_UINT("Alert Event Type", 		fwts_acpi_table_asf_alrt_element, event_type),
		FIELD_UINT("Alert Event Offset", 	fwts_acpi_table_asf_alrt_element, event_offset),
		FIELD_UINT("Alert Event Source Type", 	fwts_acpi_table_asf_alrt_element, event_source_type),
		FIELD_UINT("Alert Event Severity", 	fwts_acpi_table_asf_alrt_element, event_severity),
		FIELD_UINT("Alert Sensor Number", 	fwts_acpi_table_asf_alrt_element, sensor_number),
		FIELD_UINT("Alert Entity", 		fwts_acpi_table_asf_alrt_element, entity),
		FIELD_UINT("Alert Entity Instance", 	fwts_acpi_table_asf_alrt_element, entity_instance),
		FIELD_END
	};

	static fwts_acpidump_field asf_rctl_fields[] = {
		FIELD_UINT("Number of Controls", 	fwts_acpi_table_asf_rctl, number_of_controls),
		FIELD_UINT("Array Element Length", 	fwts_acpi_table_asf_rctl, array_element_length),
		FIELD_UINT("Reserved", 			fwts_acpi_table_asf_rctl, reserved),
		FIELD_END
	};

	static fwts_acpidump_field asf_rctl_element_fields[] = {
		FIELD_UINT("Control Function", 		fwts_acpi_table_asf_rctl_element, control_function),
		FIELD_UINT("Control Device Address", 	fwts_acpi_table_asf_rctl_element, control_device_addr),
		FIELD_UINT("Control Command", 		fwts_acpi_table_asf_rctl_element, control_command),
		FIELD_UINT("Control Value", 		fwts_acpi_table_asf_rctl_element, control_value),
		FIELD_END
	};

	static fwts_acpidump_field asf_rcmp_fields[] = {
		FIELD_UINT("Remote Control Capabilities", fwts_acpi_table_asf_rcmp, remote_control_capabilities),
		FIELD_UINT("RMCP Boot Options Completion Code", fwts_acpi_table_asf_rcmp, rcmp_completion_code),
		FIELD_UINT("RMCP IANA Enterprise ID", 	fwts_acpi_table_asf_rcmp, rcmp_iana),
		FIELD_UINT("RMCP Special Command", 	fwts_acpi_table_asf_rcmp, rcmp_special_command),
		FIELD_UINT("RMCP Special Command Parameter", fwts_acpi_table_asf_rcmp, rcmp_special_command_param),
		FIELD_UINT("RMCP Boot Options", 	fwts_acpi_table_asf_rcmp, rcmp_boot_options),
		FIELD_UINT("RMCP OEM Parameters", 	fwts_acpi_table_asf_rcmp, rcmp_oem_parameters),
		FIELD_END
	};

	static fwts_acpidump_field asf_header_fields[] = {
		FIELD_UINT("Type", fwts_acpi_table_asf_header, type),
		FIELD_UINT("Reserved", fwts_acpi_table_asf_header, reserved),
		FIELD_UINT("Length", fwts_acpi_table_asf_header, length),
	};

	static fwts_acpidump_field asf_addr_fields[] = {
		FIELD_UINT("SEEPROM Address", 		fwts_acpi_table_asf_addr, seeprom_addr),
		FIELD_UINT("Number of Devices", 	fwts_acpi_table_asf_addr, number_of_devices),
		FIELD_END
	};

	static fwts_acpidump_field asf_addr_element_fields[] = {
		FIELD_UINT("Fixed SMBus Address", 	fwts_acpi_table_asf_addr_element, fixed_smbus_addr),
		FIELD_END
	};

	while (ptr < data + length) {
		fwts_acpi_table_asf_header *hdr = (fwts_acpi_table_asf_header*)ptr;
		fwts_acpi_table_asf_alrt *alrt;
		fwts_acpi_table_asf_rctl *rctl;
		fwts_acpi_table_asf_addr *addr;
		uint8_t type = hdr->type;
		uint8_t i;
		uint8_t *asf_ptr = ptr;

		fwts_log_nl(fw);
		__acpi_dump_table_fields(fw, asf_ptr, asf_header_fields, asf_ptr - data);

		asf_ptr += sizeof(fwts_acpi_table_asf_header);

		switch (type & 0x7f) {
		case 0:
			/* Info fields */
			__acpi_dump_table_fields(fw, asf_ptr, asf_info_fields, asf_ptr - data);
			break;
		case 1:
			/* Alert fields */
			alrt = (fwts_acpi_table_asf_alrt *)asf_ptr;
			__acpi_dump_table_fields(fw, asf_ptr, asf_alrt_fields, asf_ptr - data);
			asf_ptr += sizeof(fwts_acpi_table_asf_alrt);
			for (i = 0; i < alrt->number_of_alerts; i++) {
				fwts_log_nl(fw);
				fwts_log_info_verbatum(fw, "ASF Alert Data #%d:\n", (int)i);
				__acpi_dump_table_fields(fw, asf_ptr, asf_alrt_element_fields, asf_ptr - data);
				asf_ptr += alrt->array_length;
			}
			break;
		case 2:
			/* remote control system actions */
			rctl = (fwts_acpi_table_asf_rctl *)asf_ptr;
			__acpi_dump_table_fields(fw, asf_ptr, asf_rctl_fields, asf_ptr - data);
			asf_ptr += sizeof(fwts_acpi_table_asf_rctl);
			for (i = 0; i < rctl->number_of_controls; i++) {
				fwts_log_nl(fw);
				fwts_log_info_verbatum(fw, "ASF Control Data #%d:\n", (int)i);
				__acpi_dump_table_fields(fw, asf_ptr, asf_rctl_element_fields, asf_ptr - data);

				asf_ptr += rctl->array_element_length;
			}
			break;
		case 3:
			/* remote control capabilties */
			__acpi_dump_table_fields(fw, asf_ptr, asf_rcmp_fields, asf_ptr - data);
			break;
		case 4:
			/* fixed SMBus addresses */
			addr = (fwts_acpi_table_asf_addr *)asf_ptr;
			__acpi_dump_table_fields(fw, asf_ptr, asf_addr_fields, asf_ptr - data);
			asf_ptr += sizeof(fwts_acpi_table_asf_addr);
			for (i = 0; i < addr->number_of_devices; i++) {
				__acpi_dump_table_fields(fw, asf_ptr, asf_addr_element_fields, asf_ptr - data);
				asf_ptr += sizeof(fwts_acpi_table_asf_addr_element);
			}
			break;
		default:
			break;	/* Unknown! */
		}

		ptr += hdr->length;	/* Jump to next header */

		if (type & 0x80)	/* Last record indicator, top bit of type field */
			break;
	}
}

/*
 *  acpidump_dmar_device_scope()
 *	dump out DMAR device scope
 */
static void acpidump_dmar_device_scope(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *device_scope,
	size_t device_scope_length)
{
	static fwts_acpidump_field dmar_device_scope_fields[] = {
		FIELD_UINT("Type",			fwts_acpi_table_dmar_device_scope, type),
		FIELD_UINT("Length",			fwts_acpi_table_dmar_device_scope, length),
		FIELD_UINT("Reserved",			fwts_acpi_table_dmar_device_scope, reserved),
		FIELD_UINT("Enumeration ID",		fwts_acpi_table_dmar_device_scope, enumeration_id),
		FIELD_UINT("Start Bus Number",		fwts_acpi_table_dmar_device_scope, start_bus_number),
		FIELD_END
	};

	/* Parse through multiple device scope entries */
	while (device_scope_length > 0) {
		int i;

		fwts_acpi_table_dmar_device_scope *device_scope_entry =
			(fwts_acpi_table_dmar_device_scope *)device_scope;
		__acpi_dump_table_fields(fw, device_scope, dmar_device_scope_fields, device_scope - data);
		/*
		 *  The device scope has a variable length path,
		 *  so just dump this raw data out for now.
		 */
		for (i=0; i < device_scope_entry->length - sizeof(fwts_acpi_table_dmar_device_scope); i++) {
			uint8_t val8 = device_scope_entry->path[i];
			fwts_log_info_verbatum(fw, "%s 0x%2.2x [%d]", acpi_dump_field_info("Path", 1,
				(device_scope - data) + sizeof(fwts_acpi_table_dmar_device_scope) + i),
				val8, i);
		}
		device_scope += device_scope_entry->length;
		device_scope_length -= device_scope_entry->length;
	}
}

/*
 *  acpidump_dmar()
 *	dump out DMAR
 *	http://download.intel.com/technology/computing/vptech/Intel(r)_VT_for_Direct_IO.pdf
 */
static void acpidump_dmar(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	uint8_t *ptr = data;

	static fwts_acpidump_field dmar_fields[] = {
		FIELD_UINT("Host Address Width", 	fwts_acpi_table_dmar, host_addr_width),
		FIELD_UINT("Flags", 			fwts_acpi_table_dmar, flags),
		FIELD_UINT("Reserved", 			fwts_acpi_table_dmar, reserved),
		FIELD_END
	};

	static fwts_acpidump_field dmar_header_fields[] = {
		FIELD_UINT("Type",			fwts_acpi_table_dmar_header, type),
		FIELD_UINT("Length",			fwts_acpi_table_dmar_header, length),
		FIELD_END
	};

	static fwts_acpidump_field dmar_hardware_unit_fields[] = {
		FIELD_UINT("Flags",			fwts_acpi_table_dmar_hardware_unit, flags),
		FIELD_UINT("Reserved",			fwts_acpi_table_dmar_hardware_unit, reserved),
		FIELD_UINT("Segment Number",		fwts_acpi_table_dmar_hardware_unit, segment_number),
		FIELD_UINT("Register Base Address",	fwts_acpi_table_dmar_hardware_unit, register_base_addr),
		FIELD_UINT("Device Scope",		fwts_acpi_table_dmar_hardware_unit, device_scope),
		FIELD_END
	};

	static fwts_acpidump_field dmar_reserved_memory_fields[] = {
		FIELD_UINT("Reserved",			fwts_acpi_table_dmar_reserved_memory, reserved),
		FIELD_UINT("Segment",			fwts_acpi_table_dmar_reserved_memory, segment),
		FIELD_UINT("Base Address",		fwts_acpi_table_dmar_reserved_memory, base_address),
		FIELD_UINT("End Address",		fwts_acpi_table_dmar_reserved_memory, end_address),

		FIELD_END
	};

	static fwts_acpidump_field dmar_atsr_fields[] = {
		FIELD_UINT("Flags",			fwts_acpi_table_dmar_atsr, flags),
		FIELD_UINT("Reserved",			fwts_acpi_table_dmar_atsr, reserved),
		FIELD_UINT("Segment",			fwts_acpi_table_dmar_atsr, segment),
		FIELD_END
	};

	__acpi_dump_table_fields(fw, ptr, dmar_fields, ptr - data);
	ptr += sizeof(fwts_acpi_table_dmar);

	while (ptr < data + length) {
		fwts_acpi_table_dmar_header *header =
			(fwts_acpi_table_dmar_header *)ptr;

		fwts_log_nl(fw);

		switch (header->type) {
		case 0:
			fwts_log_info_verbatum(fw, "Hardware Unit Definition:");
			__acpi_dump_table_fields(fw, ptr, dmar_header_fields, ptr - data);
			__acpi_dump_table_fields(fw, ptr, dmar_hardware_unit_fields, ptr - data);

			acpidump_dmar_device_scope(fw, data,
				ptr + sizeof(fwts_acpi_table_dmar_hardware_unit),
				(size_t)header->length - sizeof(fwts_acpi_table_dmar_hardware_unit));
			break;
		case 1:
			fwts_log_info_verbatum(fw, "Reserved Memory Definition:");
			__acpi_dump_table_fields(fw, ptr, dmar_header_fields, ptr - data);
			__acpi_dump_table_fields(fw, ptr, dmar_reserved_memory_fields, ptr - data);

			acpidump_dmar_device_scope(fw, data,
				ptr + sizeof(fwts_acpi_table_dmar_reserved_memory),
				(size_t)header->length - sizeof(fwts_acpi_table_dmar_reserved_memory));
			break;
		case 2:
			fwts_log_info_verbatum(fw, "Root Port ATS Capability Reporting Structure:");
			__acpi_dump_table_fields(fw, ptr, dmar_header_fields, ptr - data);
			__acpi_dump_table_fields(fw, ptr, dmar_atsr_fields, ptr - data);

			acpidump_dmar_device_scope(fw, data,
				ptr + sizeof(fwts_acpi_table_dmar_atsr),
				(size_t)header->length - sizeof(fwts_acpi_table_dmar_atsr));
			break;
		default:
			/* and anything else */
			break;
		}

		ptr += header->length;
	}
}

/*
 *  acpidump_slic()
 *	dump out SLIC
 */
static void acpidump_slic(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;
	uint8_t *ptr = data;

	static fwts_acpidump_field slic_header_fields[] = {
		FIELD_UINT("Type", 	fwts_acpi_table_slic_header, type),
		FIELD_UINT("Length", 	fwts_acpi_table_slic_header, length),
		FIELD_END
	};

	static fwts_acpidump_field slic_key_fields[] = {
		FIELD_UINT("Key Type", 	fwts_acpi_table_slic_key, key_type),
		FIELD_UINT("Version", 	fwts_acpi_table_slic_key, version),
		FIELD_UINT("Reserved", 	fwts_acpi_table_slic_key, reserved),
		FIELD_UINT("Algorithm", fwts_acpi_table_slic_key, algorithm),
		FIELD_UINT("Magic", 	fwts_acpi_table_slic_key, magic),
		FIELD_UINT("Bit Length",fwts_acpi_table_slic_key, bit_length),
		FIELD_UINTS("Modulus", 	fwts_acpi_table_slic_key, modulus),
		FIELD_END
	};

	static fwts_acpidump_field slic_marker_fields[] = {
		FIELD_UINT("Version", 		fwts_acpi_table_slic_marker, version),
		FIELD_STR("OEM ID", 		fwts_acpi_table_slic_marker, oem_id),
		FIELD_STR("OEM TABLE ID", 	fwts_acpi_table_slic_marker, oem_table_id),
		FIELD_UINTS("Windows Flag",	fwts_acpi_table_slic_marker, windows_flag),
		FIELD_UINT("SLIC Version", 	fwts_acpi_table_slic_marker, slic_version),
		FIELD_UINTS("Reserved",		fwts_acpi_table_slic_marker, reserved),
		FIELD_UINTS("Signature", 	fwts_acpi_table_slic_marker, signature),
		FIELD_END
	};

	ptr += sizeof(fwts_acpi_table_header);

	while (ptr < data + length) {
		fwts_log_nl(fw);
		fwts_acpi_table_slic_header *header =
			(fwts_acpi_table_slic_header *)ptr;

		switch(header->type) {
		case 0:
			__acpi_dump_table_fields(fw, ptr, slic_header_fields, ptr - data);
			__acpi_dump_table_fields(fw, ptr, slic_key_fields, ptr - data);
			break;
		case 1:
			__acpi_dump_table_fields(fw, ptr, slic_header_fields, ptr - data);
			__acpi_dump_table_fields(fw, ptr, slic_marker_fields, ptr - data);
			break;
		default:
			__acpi_dump_table_fields(fw, ptr, slic_header_fields, ptr - data);
			break;
		}

		ptr += header->length;
	}
}

/*
 *  acpidump_uefi()
 *	dump out UEFI tables
 */
static void acpidump_uefi(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	fwts_acpi_table_uefi *uefi = (fwts_acpi_table_uefi *)table->data;

	static fwts_acpidump_field uefi_fields[] = {
		FIELD_GUID("UUID", 	fwts_acpi_table_uefi, uuid),
		FIELD_END
	};

	__acpi_dump_table_fields(fw, data, uefi_fields, 0);
	fwts_log_nl(fw);
	acpi_dump_raw_data(fw, uefi->data, table->length - sizeof(fwts_acpi_table_uefi),
		sizeof(fwts_acpi_table_uefi));
}

/*
 *  acpidump_bgrt()
 *	dump BGRT, see 5.2.22 Boot Graphics Resource Table (BGRT)
 *	of version 5.0 ACPI spec.
 */
static void acpidump_bgrt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field bgrt_fields[] = {
		FIELD_UINT("Version", 		fwts_acpi_table_bgrt, version),
		FIELD_UINT("Status", 		fwts_acpi_table_bgrt, status),
		FIELD_UINT("Image Type", 	fwts_acpi_table_bgrt, image_type),
		FIELD_UINT("Image Address", 	fwts_acpi_table_bgrt, image_addr),
		FIELD_UINT("Image Offset X", 	fwts_acpi_table_bgrt, image_offset_x),
		FIELD_UINT("Image Offset Y", 	fwts_acpi_table_bgrt, image_offset_t),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, bgrt_fields, length, length);
}

/*
 *  acpidump_gtdt()
 *	dump GTDT, see 5.2.24 Generic Timer Description Table (GTDT)
 *	of version 5.0 ACPI spec.
 */
static void acpidump_gtdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field gtdt_fields[] = {
		FIELD_UINT("Physical Address", 		fwts_acpi_table_gtdt, phys_addr),
		FIELD_UINT("Global Flags", 		fwts_acpi_table_gtdt, global_flags),
		FIELD_UINT("Secure PL1 Timer GSIV", 	fwts_acpi_table_gtdt, secure_PL1_timer_GSIV),
		FIELD_UINT("Secure PL1 Timer Flags", 	fwts_acpi_table_gtdt, secure_PL1_timer_flags),
		FIELD_UINT("Non-Secure PL1 Timer GSIV", fwts_acpi_table_gtdt, non_secure_PL1_timer_GSIV),
		FIELD_UINT("Non-Secure PL1 Timer Flags", fwts_acpi_table_gtdt, non_secure_PL1_timer_flags),
		FIELD_UINT("Virtual Timer GSIV", 	fwts_acpi_table_gtdt, virtual_timer_GSIV),
		FIELD_UINT("Virtual Timer flags", 	fwts_acpi_table_gtdt, virtual_timer_flags),
		FIELD_UINT("Non-Secure PL2 Timer GSIV", fwts_acpi_table_gtdt, non_secure_PL2_timer_GSIV),
		FIELD_UINT("Non-Secure PL2 Timer Flags", fwts_acpi_table_gtdt, non_secure_PL2_timer_flags),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, gtdt_fields, length, length);
}

/*
 *  acpidump_fpdt()
 *	dump FPDT, see 5.2.23 Firmware Performance Data Table (FPDT)
 *	of version 5.0 ACPI spec.
 */
static void acpidump_fpdt(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	uint8_t *ptr  = data;
	size_t length = table->length;

	static fwts_acpidump_field fpdt_header_fields[] = {
		FIELD_UINT("Type", 		fwts_acpi_table_fpdt_header, type),
		FIELD_UINT("Length", 		fwts_acpi_table_fpdt_header, length),
		FIELD_UINT("Revision", 		fwts_acpi_table_fpdt_header, revision),
		FIELD_END
	};

	static fwts_acpidump_field fpdt_s3_perf_ptr_fields[] = {
		FIELD_UINT("Reserved", 		fwts_acpi_table_fpdt_s3_perf_ptr, reserved),
		FIELD_UINT("S3PT Pointer", 	fwts_acpi_table_fpdt_s3_perf_ptr, s3pt_addr),
		FIELD_END
	};

	static fwts_acpidump_field fpdt_basic_boot_perf_ptr_fields[] = {
		FIELD_UINT("Reserved", 		fwts_acpi_table_fpdt_basic_boot_perf_ptr, reserved),
		FIELD_UINT("FBPT Pointer", 	fwts_acpi_table_fpdt_basic_boot_perf_ptr, fbpt_addr),
		FIELD_END
	};

	/*
	 *  Currently we just dump out the various pointer records.  A more complete
	 *  implementation should mmap the memory that the records point to and also
	 *  dump these out.  That's an implementation issue for later.
	 */
	while (ptr < data + length) {
		fwts_acpi_table_fpdt_header *fpdt = (fwts_acpi_table_fpdt_header*)ptr;

		__acpi_dump_table_fields(fw, ptr, fpdt_header_fields, ptr - data);
		switch (fpdt->type) {
		case 0:
			fwts_log_info_verbatum(fw, "S3 resume performance record pointer:");
			__acpi_dump_table_fields(fw, ptr, fpdt_s3_perf_ptr_fields, ptr - data);
			break;
		case 1:
			fwts_log_info_verbatum(fw, "S3 suspend performance record pointer:");
			__acpi_dump_table_fields(fw, ptr, fpdt_s3_perf_ptr_fields, ptr - data);
			break;
		case 2:
			fwts_log_info_verbatum(fw, "Boot performance record pointer:");
			__acpi_dump_table_fields(fw, ptr, fpdt_basic_boot_perf_ptr_fields, ptr - data);
			break;
		default:
			break;
		}

		ptr += fpdt->length;
	}
}

/*
 *  acpidump_rasf()
 *	dump RASF, see 5.2.20 ACPI RAS FeatureTable (RASF)
 *	of version 5.0 ACPI spec.
 */
static void acpidump_rasf(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	size_t length = table->length;

	static fwts_acpidump_field rasf_fields[] = {
		FIELD_UINTS("Comm. Channel ID", fwts_acpi_table_rasf, platform_cc_id),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, rasf_fields, length, length);

	/* No idea how to dump rest of table, spec is a rather poor */
	acpi_dump_raw_table(fw, table);
}


typedef struct {
	char *name;
	void (*func)(fwts_framework *fw, fwts_acpi_table_info *table);
	int  standard_header;
} acpidump_table_vec;

/* To be implemented */
#define acpidump_einj		acpi_dump_raw_table
#define acpidump_hest		acpi_dump_raw_table
#define acpidump_msct		acpi_dump_raw_table
#define acpidump_mpst		acpi_dump_raw_table

static acpidump_table_vec table_vec[] = {
	{ "APIC", 	acpidump_madt, 	1 },
	{ "ASF!", 	acpidump_asf, 	1 },
	{ "BERT", 	acpidump_bert, 	1 },
	{ "BGRT", 	acpidump_bgrt, 	1 },
	{ "BOOT", 	acpidump_boot, 	1 },
	{ "CPEP", 	acpidump_cpep, 	1 },
	{ "DSDT", 	acpidump_amlcode, 1 },
	{ "DMAR", 	acpidump_dmar,	1 },
	{ "ECDT", 	acpidump_ecdt, 	1 },
	{ "EINJ", 	acpidump_einj, 	1 },
	{ "ERST", 	acpidump_erst, 	1 },
	{ "FACP", 	acpidump_fadt, 	1 },
	{ "FACS", 	acpidump_facs, 	0 },
	{ "FPDT",	acpidump_fpdt,	1 },
	{ "GTDT", 	acpidump_gtdt, 	1 },
	{ "HEST", 	acpidump_hest, 	1 },
	{ "HPET", 	acpidump_hpet, 	1 },
	{ "MCFG", 	acpidump_mcfg, 	1 },
	{ "MPST",	acpidump_mpst,  1 },
	{ "MSCT", 	acpidump_msct, 	1 },
	{ "PSDT", 	acpidump_amlcode, 1 },
	{ "RASF",	acpidump_rasf,	1 },
	{ "RSDT", 	acpidump_rsdt, 	1 },
	{ "RSD PTR ", 	acpidump_rsdp, 	0 },
	{ "SBST", 	acpidump_sbst,  1 },
	{ "SSDT", 	acpidump_amlcode, 1 },
	{ "SLIT", 	acpidump_slit,  1 },
	{ "SLIC", 	acpidump_slic,  1 },
	{ "SRAT", 	acpidump_srat,  1 },
	{ "TCPA",	acpidump_tcpa,	1 },
	{ "UEFI", 	acpidump_uefi, 	1 },
	{ "XSDT", 	acpidump_xsdt, 	1 },
	{ NULL,		NULL,		0 },
};

static int acpidump_table(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8_t *data = (uint8_t *)table->data;
	fwts_acpi_table_header hdr;
	size_t length = table->length;
	int i;

	for (i=0; table_vec[i].name != NULL; i++) {
		if (strncmp(table_vec[i].name, (char*)data, strlen(table_vec[i].name)) == 0) {
			if (table_vec[i].standard_header) {
				fwts_acpi_table_get_header(&hdr, data);
				acpidump_hdr(fw, &hdr, length);
			}
			table_vec[i].func(fw, table);
			return FWTS_OK;
		}
	}

	/* Cannot find, assume standard table header */
	fwts_acpi_table_get_header(&hdr, data);
	acpidump_hdr(fw, &hdr, length);
	acpi_dump_raw_table(fw, table);

	return FWTS_OK;
}

static int acpidump_test1(fwts_framework *fw)
{
	int i;

	fwts_acpi_table_info *table;

	fwts_infoonly(fw);

	for (i=0; (fwts_acpi_get_table(fw, i, &table) == FWTS_OK) && (table !=NULL); i++) {
		fwts_log_info_verbatum(fw, "%s @ %4.4x (%d bytes)", table->name, (uint32_t)table->addr, (int)table->length);
		fwts_log_info_verbatum(fw, "---------------");
		acpidump_table(fw, table);
		fwts_log_nl(fw);
	}

	return FWTS_OK;
}

static fwts_framework_minor_test acpidump_tests[] = {
	{ acpidump_test1, "Dump ACPI tables." },
	{ NULL, NULL }
};

static fwts_framework_ops acpidump_ops = {
	.description = "Dump ACPI tables.",
	.minor_tests = acpidump_tests
};

FWTS_REGISTER(acpidump, &acpidump_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_UTILS);
