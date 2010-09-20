/*
 * Copyright (C) 2010 Canonical
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
#include <sys/io.h> 

#include "fwts.h"

static int acpidump_init(fwts_framework *fw)
{
	if (fwts_check_root_euid(fw))
		return FWTS_ERROR;

	return FWTS_OK;
}


static char *acpidump_headline(void)
{
	return "Check ACPI table acpidump.";
}

struct fwts_acpidump_field;

typedef void (*fwts_acpidump_field_func)(fwts_framework *fw, struct fwts_acpidump_field *info, void *data);

typedef struct fwts_acpidump_field {
	char *label;
	int   size;
	int   offset;
	fwts_acpidump_field_func func;
	uint8  bit_field_nbits;
	uint8  bit_field_shift;
	char  **strings;
	int   strings_len;
} fwts_acpidump_field;

#define OFFSET(type, field)				\
	(int)&(((type*)(0))->field)

#define FIELD(text, type, field, func, bit_field_nbits, bit_field_shift, strings, strings_len) \
	{						\
	text,						\
	sizeof(((type*)0)->field),			\
	OFFSET(type, field),				\
	func,						\
	bit_field_nbits,				\
	bit_field_shift,				\
	strings,					\
	strings_len					\
	}

#define FIELD_UINT(text, type, field)			\
	FIELD(text, type, field, acpi_dump_uint, 0, 0, NULL, 0)

#define FIELD_STR(text, type, field)			\
	FIELD(text, type, field, acpi_dump_str, 0, 0, NULL, 0)

#define FIELD_STRS(text, type, field, strings, strings_len)			\
	FIELD(text, type, field, acpi_dump_strings, 0, 0, strings, strings_len)

#define FIELD_GAS(text, type, field)			\
	FIELD(text, type, field, __acpi_dump_gas, 0, 0, NULL, 0)

#define FIELD_BITF(text, type, field, nbits, shift)			\
	FIELD(text, type, field, acpi_dump_uint, nbits, shift, NULL, 0)

#define FIELD_END { NULL, 0, 0, NULL, 0, 0, NULL, 0}

static char *acpi_dump_field_info(char *label, int size, int offset)
{
	static char buffer[1024];

	snprintf(buffer, sizeof(buffer), "[%3.3x %4.4d %2d] %25.25s: ",
		offset, offset, size, label);

	return buffer;
}

static void acpi_dump_str(fwts_framework *fw, fwts_acpidump_field *info, void *data)
{
	fwts_log_info_verbatum(fw, "%s %*.*s", 
		acpi_dump_field_info(info->label, info->size, info->offset), info->size, info->size,(char*)data);
}

static void acpi_dump_uint(fwts_framework *fw, fwts_acpidump_field *info, void *data)
{
	uint8	val8, mask8;
	uint16	val16,mask16;
	uint32	val32,mask32;
	uint64  val64,mask64;
	int i;

	switch (info->size) {
	case 1:
		val8 = *(uint8*)data;
		mask8 = (1<<info->bit_field_nbits) - 1;
		fwts_log_info_verbatum(fw, "%s 0x%2.2x", acpi_dump_field_info(info->label, info->size, info->offset), 
			info->bit_field_nbits ? (val8 >> info->bit_field_shift) & mask8 : val8);
		break;
	case 2:
		val16 = *(uint16*)data;
		mask16 = (1<<info->bit_field_nbits) - 1;
		fwts_log_info_verbatum(fw, "%s 0x%4.4x", acpi_dump_field_info(info->label, info->size, info->offset),
			info->bit_field_nbits ? (val16 >> info->bit_field_shift) & mask16 : val16);
		break;
	case 4:
		val32 = *(uint32*)data;
		mask32 = (1<<info->bit_field_nbits) - 1;
		fwts_log_info_verbatum(fw, "%s 0x%8.8x", acpi_dump_field_info(info->label, info->size, info->offset),
			info->bit_field_nbits ? (val32 >> info->bit_field_shift) & mask32 : val32);
		break;
	case 8:
		val64 = *(uint64*)data;
		mask64 = (1<<info->bit_field_nbits) - 1;
		fwts_log_info_verbatum(fw, "%s 0x%16.16llx", acpi_dump_field_info(info->label, info->size, info->offset),
			info->bit_field_nbits ? (val64 >> info->bit_field_shift) & mask64 : val64);
		break;
	default:		
		for (i=0; i<info->size; i++) {
			val8 = *(uint8*)data;
			fwts_log_info_verbatum(fw, "%s 0x%2.2x [%d]", acpi_dump_field_info(info->label, info->size, info->offset), val8, i);
			data++;
		}
		break;
	}
}

static void acpi_dump_strings(fwts_framework *fw, fwts_acpidump_field *info, void *data)
{
	uint8	val8;
	uint16	val16;
	uint32	val32;
	uint64  val64;

	switch (info->size) {
	case 1:
		val8 = *(uint8*)data;
		fwts_log_info_verbatum(fw, "%s 0x%2.2x (%s)", acpi_dump_field_info(info->label, info->size, info->offset), val8,
			val8 > info->strings_len ? "unknown" : info->strings[val8]);
		break;
	case 2:
		val16 = *(uint16*)data;
		fwts_log_info_verbatum(fw, "%s 0x%4.4x (%s)", acpi_dump_field_info(info->label, info->size, info->offset), val16,
			val16 > info->strings_len ? "unknown" : info->strings[val16]);
		break;
	case 4:
		val32 = *(uint32*)data;
		fwts_log_info_verbatum(fw, "%s 0x%8.8x (%s)", acpi_dump_field_info(info->label, info->size, info->offset), val32,
			val32 > info->strings_len ? "unknown" : info->strings[val32]);
		break;
	case 8:
		val64 = *(uint64*)data;
		fwts_log_info_verbatum(fw, "%s 0x%16.16x (%s)", acpi_dump_field_info(info->label, info->size, info->offset), val64,
			val64 > info->strings_len ? "unknown" : info->strings[val64]);
		break;
	}
}

static void acpi_dump_table_fields(fwts_framework *fw, uint8 *data, fwts_acpidump_field *fields)
{
	fwts_acpidump_field *field = fields;

	for (field = fields; field->label != NULL; field++)
		field->func(fw, field, data + field->offset);
}

#if 0
static void acpi_dump_fill_table(void *table, const uint *data, 
				 const int given_length, const int expected_length)
{
	memset(table, 0, expected_length);
	memcpy(table, data, given_length);
}
#endif

static void acpi_dump_raw_table(fwts_framework *fw, uint8 *data, int length)
{
        int n;

	fwts_log_nl(fw);

        for (n = 0; n < length; n+=16) {
                int left = length - n;
		char buffer[128];
		fwts_dump_raw_data(buffer, sizeof(buffer), data + n, n, left > 16 ? 16 : left);
		fwts_log_info_verbatum(fw, "%s", buffer);
        }
}


static void acpi_dump_gas(fwts_framework *fw, const char *str, const fwts_acpi_gas *gas)
{	
	char *txt;
	fwts_log_info_verbatum(fw, "%s:", str);

	switch (gas->address_space_id) {
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
	fwts_log_info_verbatum(fw, "  addr_space_id:  0x%x (%s)", gas->address_space_id, txt);
	fwts_log_info_verbatum(fw, "  reg_bit_width:  0x%x", gas->register_bit_width);
	fwts_log_info_verbatum(fw, "  reg_bit_offset: 0x%x", gas->register_bit_offset);
	switch (gas->access_width) {
	case 0x00:
		txt = "Undefined (legacy reasons)";
		break;
	case 0x01:
		txt = "Byte Access";
		break;
	case 0x02:
		txt = "Word Access";
		break;
	case 0x03:
		txt = "DWord Access";
		break;
	case 0x04:
		txt = "QWord Access";
		break;
	default:
		txt = "Unknown";
		break;
	}
	fwts_log_info_verbatum(fw, "  access_width:   0x%x (%s)", gas->access_width, txt);
	fwts_log_info_verbatum(fw, "  address:        0x%llx", gas->address);
}

static void __acpi_dump_gas(fwts_framework *fw, fwts_acpidump_field *info, void *data)
{
	acpi_dump_gas(fw, info->label, (fwts_acpi_gas *)data);
}

static void acpidump_hdr(fwts_framework *fw, fwts_acpi_table_header *hdr)
{
	fwts_acpidump_field fields[] = {
		FIELD_STR ("Signature", 	fwts_acpi_table_header, signature),
		FIELD_UINT("Length", 		fwts_acpi_table_header, length),
		FIELD_UINT("Revision", 		fwts_acpi_table_header, revision),
		FIELD_UINT("Checksum", 		fwts_acpi_table_header, checksum),
		FIELD_STR ("OEM ID", 		fwts_acpi_table_header, oem_id),
		FIELD_UINT("OEM Table ID", 	fwts_acpi_table_header, oem_tbl_id),
		FIELD_UINT("OEM Revision", 	fwts_acpi_table_header, oem_revision),
		FIELD_STR ("Creator ID", 	fwts_acpi_table_header, creator_id),
		FIELD_UINT("Creator Revision", 	fwts_acpi_table_header, creator_revision),
		FIELD_END
	};

	acpi_dump_table_fields(fw, (uint8*)hdr, fields);
}

static void acpidump_boot(fwts_framework *fw, uint8 *data, int length)
{
	uint8 cmos_data;
	fwts_acpi_table_boot *boot = (fwts_acpi_table_boot*)data;

	fwts_acpidump_field fields[] = {
		FIELD_UINT("CMOS offset", 	fwts_acpi_table_boot, cmos_index),
		FIELD_END
	};

	if (length < (sizeof(fwts_acpi_table_header) + 4)) {
		fwts_log_info(fw, "Boot table too short\n");
		return;
	}

	acpi_dump_table_fields(fw, data, fields);

	cmos_data = fwts_cmos_read(boot->cmos_index);
	fwts_log_info_verbatum(fw, "Boot Register: 0x%x", cmos_data);
	fwts_log_info_verbatum(fw, "  PNP-OS:   %x", (cmos_data & FWTS_BOOT_REGISTER_PNPOS) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Booting:  %x", (cmos_data & FWTS_BOOT_REGISTER_BOOTING) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Diag:     %x", (cmos_data & FWTS_BOOT_REGISTER_DIAG) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Suppress: %x", (cmos_data & FWTS_BOOT_REGISTER_SUPPRESS_BOOT_DISPLAY) ? 1 : 0);
	fwts_log_info_verbatum(fw, "  Parity:   %x", (cmos_data & FWTS_BOOT_REGISTER_PARITY) ? 1 : 0);
}

static void acpidump_bert(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_bert *bert = (fwts_acpi_table_bert*)data;
	static char *error_severity[] = {
		"Correctable",
		"Fatal",
		"Corrected",
		"None"
	};

	int n = length - sizeof(fwts_acpi_table_bert);

	fwts_acpidump_field fields[] = {
		FIELD_UINT("Region Length", 	fwts_acpi_table_bert, boot_error_region_length),
		FIELD_UINT("Region Addr", 	fwts_acpi_table_bert, boot_error_region),
		FIELD_UINT("Boot Status", 	fwts_acpi_table_bert, boot_status),
		FIELD_UINT("Raw Data Offset", 	fwts_acpi_table_bert, raw_data_offset),
		FIELD_UINT("Raw Data Length", 	fwts_acpi_table_bert, raw_data_length),
		FIELD_UINT("Error Severity", 	fwts_acpi_table_bert, error_severity),
		FIELD_STRS("Generic Error Data",fwts_acpi_table_bert, generic_error_data, error_severity, 4),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields);

	acpi_dump_raw_table(fw, bert->generic_error_data, n);
}

static void acpidump_cpep(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_cpep *cpep = (fwts_acpi_table_cpep*)data;
	int i;
	int n = (length - sizeof(fwts_acpi_table_bert)) / sizeof(fwts_acpi_cpep_processor_info);

	for (i=0; i<n; i++) {
		fwts_log_info_verbatum(fw, "CPEP #%d\n", i);
		fwts_log_info_verbatum(fw, "  Type:           0x%x", cpep->cpep_info[i].type);
		fwts_log_info_verbatum(fw, "  Length:         0x%x", cpep->cpep_info[i].length);
		fwts_log_info_verbatum(fw, "  Processor ID:   0x%x", cpep->cpep_info[i].processor_id);
		fwts_log_info_verbatum(fw, "  Processor EID:  0x%x", cpep->cpep_info[i].processor_eid);
		fwts_log_info_verbatum(fw, "  Poll Interval   0x%lx ms", cpep->cpep_info[i].polling_interval);
	}
}

static void acpidump_ecdt(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_ecdt *ecdt = (fwts_acpi_table_ecdt*)data;
	int n = length - sizeof(fwts_acpi_table_ecdt);


	fwts_acpidump_field fields[] = {
		FIELD_GAS ("EC_CONTROL", fwts_acpi_table_ecdt,   ec_control),
		FIELD_GAS ("EC_DATA", 	fwts_acpi_table_ecdt,   ec_data),
		FIELD_UINT("UID", 	fwts_acpi_table_ecdt,	uid),
		FIELD_UINT("GPE_BIT", 	fwts_acpi_table_ecdt,	gpe_bit),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields);

	fwts_log_info_verbatum(fw, "EC_ID:");
	acpi_dump_raw_table(fw, ecdt->ec_id, n);
}

static void acpidump_erst(fwts_framework *fw, uint8 *data, int length)
{
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

	fwts_log_info_verbatum(fw, "Ser. Hdr. Size:   0x%lx", erst->serialization_header_size);
	fwts_log_info_verbatum(fw, "Insr. Entry Count:0x%lx", erst->instruction_entry_count);
	
	for (i=0; i<erst->instruction_entry_count; i++) {
		fwts_log_info_verbatum(fw, "Entry #%d%", i+1);
		fwts_log_info_verbatum(fw, "  Action:         0x%x (%s)", 
			erst->entries[i].serialization_action,
			erst->entries[i].serialization_action > 0x10 ? "Unknown" :
				serialization_actions[erst->entries[i].serialization_action]);
		fwts_log_info_verbatum(fw, "  Instruction::   0x%x (%s)", 
			erst->entries[i].instruction,
			erst->entries[i].instruction > 0x12 ? "Unknown" :
				instructions[erst->entries[i].instruction]);
		fwts_log_info_verbatum(fw, "  Flags:          0x%x", erst->entries[i].flags);
		acpi_dump_gas(fw, "Resgister region", &erst->entries[i].register_region);
		fwts_log_info_verbatum(fw, "  Value:          0x%llx", erst->entries[i].value);
		fwts_log_info_verbatum(fw, "  Mask:           0x%llx", erst->entries[i].mask);
	}
}

static void acpidump_amlcode(fwts_framework *fw, uint8 *data, int length)
{
	fwts_log_info_verbatum(fw, "Contains 0x%x byes of AML byte code", length-sizeof(fwts_acpi_table_header));
}

static void acpidump_facs(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpidump_field fields[] = {
		FIELD_STR ("Signature", 	fwts_acpi_table_facs, 	signature),
		FIELD_UINT("Length", 		fwts_acpi_table_facs,	length),
		FIELD_UINT("H/W Signature", 	fwts_acpi_table_facs,	hardware_signature),
		FIELD_UINT("Waking Vector", 	fwts_acpi_table_facs,	firmware_waking_vector),
		FIELD_UINT("Global Lock", 	fwts_acpi_table_facs,	global_lock),
		FIELD_UINT("Flags", 		fwts_acpi_table_facs,	flags),
		FIELD_UINT("X Waking Vector:", 	fwts_acpi_table_facs,	x_firmware_waking_vector),
		FIELD_UINT("Version:", 		fwts_acpi_table_facs,	version),
		FIELD_UINT("OSPM Flags", 	fwts_acpi_table_facs,	ospm_flags),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields);
}

static void acpidump_hpet(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpidump_field fields[] = {
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

	acpi_dump_table_fields(fw, data, fields);
}

static void acpidump_fadt(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpidump_field fields[] = {
		FIELD_UINT("FACS Address", 	fwts_acpi_table_fadt, firmware_control),
		FIELD_UINT("DSDT Address", 	fwts_acpi_table_fadt, dsdt),
		FIELD_UINT("Model",		fwts_acpi_table_fadt, reserved),
		FIELD_STRS("PM Profile",	fwts_acpi_table_fadt, preferred_pm_profile, fwts_acpi_fadt_preferred_pm_profile, 8),
		FIELD_UINT("SCI_INT, IRQ 0x", 	fwts_acpi_table_fadt, sci_int),
		FIELD_UINT("SMI_CMD", 		fwts_acpi_table_fadt, smi_cmd),
		FIELD_UINT("ACPI_ENABLE", 	fwts_acpi_table_fadt, acpi_enable),
		FIELD_UINT("ACPI_DISABLE", 	fwts_acpi_table_fadt, acpi_disable),
		FIELD_UINT("S4BIOS_REQ", 	fwts_acpi_table_fadt, s4bios_req),
		FIELD_UINT("PSTATE_CNT", 	fwts_acpi_table_fadt, pstate_cnt),
		FIELD_UINT("PM1a_EVT_BLK", 	fwts_acpi_table_fadt, pm1a_evt_blk),
		FIELD_UINT("PM1b_EVT_BLK", 	fwts_acpi_table_fadt, pm1b_evt_blk),
		FIELD_UINT("PM1a_CNT_BLK", 	fwts_acpi_table_fadt, pm1a_cnt_blk),
		FIELD_UINT("PM1b_CNT_BLK", 	fwts_acpi_table_fadt, pm1b_cnt_blk),
		FIELD_UINT("PM2_CNT_BLK", 	fwts_acpi_table_fadt, pm2_cnt_blk),
		FIELD_UINT("PM_TMR_BLK", 	fwts_acpi_table_fadt, pm_tmr_blk),
		FIELD_UINT("GPE0_BLK", 		fwts_acpi_table_fadt, gpe0_blk),
		FIELD_UINT("GPE1_BLK", 		fwts_acpi_table_fadt, gpe1_blk),
		FIELD_UINT("PM1_EVT_LEN", 	fwts_acpi_table_fadt, pm1_evt_len),
		FIELD_UINT("PM1_CNT_LEN", 	fwts_acpi_table_fadt, pm1_cnt_len),
		FIELD_UINT("PM2_CNT_LEN", 	fwts_acpi_table_fadt, pm2_cnt_len),
		FIELD_UINT("GPE0_BLK_LEN", 	fwts_acpi_table_fadt, gpe0_blk_len),
		FIELD_UINT("GPE1_BLK_LEN", 	fwts_acpi_table_fadt, gpe1_blk_len),
		FIELD_UINT("_CST Support", 	fwts_acpi_table_fadt, cst_cnt),
		FIELD_UINT("C2 Latency", 	fwts_acpi_table_fadt, p_lvl2_lat),
		FIELD_UINT("C3 Latency", 	fwts_acpi_table_fadt, p_lvl3_lat),
		FIELD_UINT("FLUSH_SIZE", 	fwts_acpi_table_fadt, flush_size),
		FIELD_UINT("FLUSH_STRIDE", 	fwts_acpi_table_fadt, flush_stride),
		FIELD_UINT("DUTY_OFFSET", 	fwts_acpi_table_fadt, duty_offset),
		FIELD_UINT("DUTY_WIDTH", 	fwts_acpi_table_fadt, duty_width),
		FIELD_UINT("DAY_ALRM", 		fwts_acpi_table_fadt, day_alrm),
		FIELD_UINT("MON_ALRM", 		fwts_acpi_table_fadt, mon_alrm),
		FIELD_UINT("CENTURY", 		fwts_acpi_table_fadt, century),
		FIELD_UINT("IAPC_BOOT_ARCH", 	fwts_acpi_table_fadt, iapc_boot_arch),
		FIELD_UINT("Flags", 		fwts_acpi_table_fadt, flags),
		FIELD_BITF("  WBINVD", 		fwts_acpi_table_fadt, flags, 1, 0),
		FIELD_BITF("  WBINVD_FLUSH", 	fwts_acpi_table_fadt, flags, 1, 1),
		FIELD_BITF("  PROC_C1", 	fwts_acpi_table_fadt, flags, 1, 2),
		FIELD_BITF("  P_LVL2_UP", 	fwts_acpi_table_fadt, flags, 1, 3),
		FIELD_BITF("  PWR_BUTTON", 	fwts_acpi_table_fadt, flags, 1, 4),
		FIELD_BITF("  SLP_BUTTON", 	fwts_acpi_table_fadt, flags, 1, 5),
		FIELD_BITF("  FIX_RTC", 	fwts_acpi_table_fadt, flags, 1, 6),
		FIELD_BITF("  RTC_S4", 		fwts_acpi_table_fadt, flags, 1, 7),
		FIELD_BITF("  TMR_VAL_EXT", 	fwts_acpi_table_fadt, flags, 1, 8),
		FIELD_BITF("  DCK_CAP", 	fwts_acpi_table_fadt, flags, 1, 9),
		FIELD_BITF("  RESET_REG_SUP", 	fwts_acpi_table_fadt, flags, 1, 10),
		FIELD_BITF("  SEALED_CASE", 	fwts_acpi_table_fadt, flags, 1, 11),
		FIELD_BITF("  HEADLESS", 	fwts_acpi_table_fadt, flags, 1, 12),
		FIELD_BITF("  CPU_WS_SLP", 	fwts_acpi_table_fadt, flags, 1, 13),
		FIELD_BITF("  PCI_EXP_WAK", 	fwts_acpi_table_fadt, flags, 1, 14),
		FIELD_BITF("  USE_PLATFORM_CLOCK", fwts_acpi_table_fadt, flags, 1, 15),
		FIELD_BITF("  S4_RTC_STS_VALID", fwts_acpi_table_fadt, flags, 1, 16),
		FIELD_BITF("  REMOTE_POWER_ON_CAPABLE", fwts_acpi_table_fadt, flags, 1, 17),
		FIELD_BITF("  FORCE_APIC_CLUSTER_MODEL", fwts_acpi_table_fadt, flags, 1, 18),
		FIELD_BITF("  FORCE_APIC_PYS_DEST_MODE", fwts_acpi_table_fadt, flags, 1, 19),
		FIELD_BITF("  RESERVED", 	fwts_acpi_table_fadt, flags, 12, 20),
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
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields);

}

static void acpidump_rsdp(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpidump_field fields[] = {
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

	acpi_dump_table_fields(fw, data, fields);
}

static void acpidump_rsdt(fwts_framework *fw, uint8 *data, int length)
{
	int i;
	int n;
	fwts_acpi_table_rsdt *rsdt = (fwts_acpi_table_rsdt*)data;

	n = (length - sizeof(fwts_acpi_table_header)) / sizeof(uint32);
	for (i=0; i<n; i++)  {
		char label[80];
		fwts_acpi_table_info *table = fwts_acpi_find_table_by_addr((uint64)rsdt->entries[i]);
		char *name = table == NULL ? "unknown" : table->name;
		snprintf(label, sizeof(label), "Entry %2.2d %s", i, name);
		fwts_log_info_verbatum(fw, "%s 0x%8.8x", 
			acpi_dump_field_info(label, sizeof(rsdt->entries[i]), OFFSET(fwts_acpi_table_rsdt, entries[i])), 
			rsdt->entries[i]);
	}
}

static void acpidump_sbst(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpidump_field fields[] = {
		FIELD_UINT("Warn. Energy Level", 	fwts_acpi_table_sbst,	warning_energy_level),
		FIELD_UINT("Low  Energy Level", 	fwts_acpi_table_sbst,	low_energy_level),
		FIELD_UINT("Crit. Energy Level", 	fwts_acpi_table_sbst,	critical_energy_level),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields);
}

static void acpidump_xsdt(fwts_framework *fw, uint8 *data, int length)
{
	int i;
	int n;
	fwts_acpi_table_xsdt *xsdt = (fwts_acpi_table_xsdt*)data;

	n = (length - sizeof(fwts_acpi_table_header)) / sizeof(uint64);
	for (i=0; i<n; i++)  {
		fwts_acpi_table_info *table = fwts_acpi_find_table_by_addr(xsdt->entries[i]);
		char *name = table == NULL ? "unknown" : table->name;
		fwts_log_info_verbatum(fw, "Entry %2.2d          0x%16.16llx (%s)", i, xsdt->entries[i], name);
	}
}

static void acpidump_madt(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_madt *madt = (fwts_acpi_table_madt*)data;
	int i = 0;
	int n;

	fwts_log_info_verbatum(fw, "Local APIC Addr:  0x%lx", madt->lapic_address);
	fwts_log_info_verbatum(fw, "Flags:            0x%lx", madt->flags);
	fwts_log_info_verbatum(fw, "  PCAT_COMPAT     0x%1.1x", madt->flags & 1);
	
	data += sizeof(fwts_acpi_table_madt);	
	length -= sizeof(fwts_acpi_table_madt);

	while (length > sizeof(fwts_acpi_madt_sub_table_header)) {
		fwts_acpi_madt_sub_table_header *hdr = (fwts_acpi_madt_sub_table_header*)data;
		fwts_acpi_madt_processor_local_apic *proc_local_apic;
		fwts_acpi_madt_io_apic              *io_apic;
		fwts_acpi_madt_interrupt_override   *interrupt_override;
		fwts_acpi_madt_nmi                  *nmi;
		fwts_acpi_madt_local_apic_nmi       *local_apic_nmi;
		fwts_acpi_madt_local_apic_addr_override *local_apic_addr_override;
		fwts_acpi_madt_io_sapic 	    *io_sapic;
		fwts_acpi_madt_local_sapic	    *local_sapic;
		fwts_acpi_madt_platform_int_source  *platform_int_source;
		fwts_acpi_madt_local_x2apic         *local_x2apic;
		fwts_acpi_madt_local_x2apic_nmi	    *local_x2apic_nmi;
		

		data += sizeof(fwts_acpi_madt_sub_table_header);
		length -= sizeof(fwts_acpi_madt_sub_table_header);
		
		fwts_log_info_verbatum(fw, "APIC Structure #%d:", ++i);
		/*
		fwts_log_info_verbatum(fw, " Type:            0x%2.2x", hdr->type);
		fwts_log_info_verbatum(fw, " Size:            0x%2.2x", hdr->length);
		*/
		switch (hdr->type) {
		case 0: 
			proc_local_apic = (fwts_acpi_madt_processor_local_apic*)data;
			fwts_log_info_verbatum(fw, " Processor Local APIC:");
			fwts_log_info_verbatum(fw, "  ACPI CPU ID:    0x%2.2x", proc_local_apic->acpi_processor_id);
			fwts_log_info_verbatum(fw, "  APIC ID:        0x%2.2x", proc_local_apic->apic_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", proc_local_apic->flags);
			fwts_log_info_verbatum(fw, "   Enabled:       0x%x", proc_local_apic->flags & 1);
			data += sizeof(fwts_acpi_madt_processor_local_apic);
			length -= sizeof(fwts_acpi_madt_processor_local_apic);
			break;
		case 1:
			io_apic = (fwts_acpi_madt_io_apic*)data;
			fwts_log_info_verbatum(fw, " I/O APIC:");
			fwts_log_info_verbatum(fw, "  I/O APIC ID:    0x%2.2x", io_apic->io_apic_id);
			fwts_log_info_verbatum(fw, "  I/O APIC Addr:  0x%lx", io_apic->io_apic_phys_address);
			fwts_log_info_verbatum(fw, "  Global IRQ Base:0x%lx", io_apic->io_apic_phys_address);
			data += sizeof(fwts_acpi_madt_io_apic);
			length -= sizeof(fwts_acpi_madt_io_apic);
			break;
		case 2:
			interrupt_override = (fwts_acpi_madt_interrupt_override*)data;
			fwts_log_info_verbatum(fw, " Interrupt Source Override:");
			fwts_log_info_verbatum(fw, "  Bus:            0x%2.2x", interrupt_override->bus);
			fwts_log_info_verbatum(fw, "  Source:         0x%2.2x", interrupt_override->source);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", interrupt_override->gsi);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", interrupt_override->flags);
			break;
		case 3:
			nmi = (fwts_acpi_madt_nmi*)data;
			fwts_log_info_verbatum(fw, " Non-maskable Interrupt Source (NMI):");
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", nmi->flags);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", nmi->gsi);
			data += sizeof(fwts_acpi_madt_nmi);
			length -= sizeof(fwts_acpi_madt_nmi);
			break;
		case 4:
			local_apic_nmi = (fwts_acpi_madt_local_apic_nmi *)data;
			fwts_log_info_verbatum(fw, " Local APIC NMI:");
			fwts_log_info_verbatum(fw, "  ACPI CPU ID:    0x%x", local_apic_nmi->acpi_processor_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", local_apic_nmi->flags);
			fwts_log_info_verbatum(fw, "  Local APIC LINT:0x%x", local_apic_nmi->local_apic_lint);
	
			data += sizeof(fwts_acpi_madt_local_apic_nmi);
			length -= sizeof(fwts_acpi_madt_local_apic_nmi);
			break;
		case 5:
			local_apic_addr_override = (fwts_acpi_madt_local_apic_addr_override*)data;
			fwts_log_info_verbatum(fw, " Local APIC Address Override:");
			fwts_log_info_verbatum(fw, "  Local APIC Addr:0x%x", local_apic_addr_override->address);
			data += sizeof(fwts_acpi_madt_local_apic_addr_override);
			length -= sizeof(fwts_acpi_madt_local_apic_addr_override);
			
			break;
		case 6:
			io_sapic = (fwts_acpi_madt_io_sapic *)data;
			fwts_log_info_verbatum(fw, " I/O SAPIC:");
			fwts_log_info_verbatum(fw, "  I/O SAPIC ID:   0x%x", io_sapic->io_sapic_id);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", io_sapic->gsi);
			fwts_log_info_verbatum(fw, "  I/O SAPIC Addr: 0x%llx", io_sapic->address);
			data += sizeof(fwts_acpi_madt_io_sapic);
			length -= sizeof(fwts_acpi_madt_io_sapic);
			break;
		case 7:
			local_sapic = (fwts_acpi_madt_local_sapic*)data;
			fwts_log_info_verbatum(fw, " Local SAPIC:");
			fwts_log_info_verbatum(fw, "  ACPI CPU ID:    0x%x", local_sapic->acpi_processor_id);
			fwts_log_info_verbatum(fw, "  Local SAPIC ID: 0x%x", local_sapic->local_sapic_id);
			fwts_log_info_verbatum(fw, "  Local SAPIC EID:0x%x", local_sapic->local_sapic_eid);
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", local_sapic->flags);
			fwts_log_info_verbatum(fw, "  UID Value:      0x%x", local_sapic->uid_value);
		
			fwts_log_info_verbatum(fw, "  UID String:     %s", local_sapic->uid_string);
			
			n = strlen(local_sapic->uid_string) + 1;
			data += (sizeof(fwts_acpi_madt_io_sapic) + n);
			length -= (sizeof(fwts_acpi_madt_io_sapic) + n);
			break;
		case 8:
			platform_int_source = (fwts_acpi_madt_platform_int_source*)data;
			fwts_log_info_verbatum(fw, " Platform Interrupt Sources:");
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", platform_int_source->flags);
			fwts_log_info_verbatum(fw, "  Type:           0x%x", platform_int_source->type);
			fwts_log_info_verbatum(fw, "  Processor ID:   0x%x", platform_int_source->processor_id);
			fwts_log_info_verbatum(fw, "  Processor EID:  0x%x", platform_int_source->processor_eid);
			fwts_log_info_verbatum(fw, "  IO SAPIC Vector:0x%lx", platform_int_source->io_sapic_vector);
			fwts_log_info_verbatum(fw, "  Gbl Sys Int:    0x%lx", platform_int_source->gsi);
			fwts_log_info_verbatum(fw, "  PIS Flags:      0x%x", platform_int_source->pis_flags);
			data += (sizeof(fwts_acpi_madt_platform_int_source));
			length += (sizeof(fwts_acpi_madt_platform_int_source));
			break;
		case 9:
			local_x2apic = (fwts_acpi_madt_local_x2apic*)data;
			fwts_log_info_verbatum(fw, " Processor Local x2APIC:");
			fwts_log_info_verbatum(fw, "  x2APIC ID       0x%lx", local_x2apic->x2apic_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", local_x2apic->flags);
			fwts_log_info_verbatum(fw, "  Processor UID:  0x%lx", local_x2apic->processor_uid);
			data += (sizeof(fwts_acpi_madt_local_x2apic));
			length += (sizeof(fwts_acpi_madt_local_x2apic));
			break;
		case 10:
			local_x2apic_nmi = (fwts_acpi_madt_local_x2apic_nmi*)data;
			fwts_log_info_verbatum(fw, " Local x2APIC NMI:");
			fwts_log_info_verbatum(fw, "  Flags:          0x%x", local_x2apic_nmi->flags);
			fwts_log_info_verbatum(fw, "  Processor UID:  0x%lx", local_x2apic_nmi->processor_uid);
			fwts_log_info_verbatum(fw, "  LINT#           :0x%lx", local_x2apic_nmi->local_x2apic_lint);
			data += (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			length += (sizeof(fwts_acpi_madt_local_x2apic_nmi));
			break;
		default:
			fwts_log_info_verbatum(fw, " Reserved for OEM use:");
			break;
		}
	}
}

static void acpidump_mcfg(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_mcfg *mcfg = (fwts_acpi_table_mcfg*)data;
	int n;
	int i;

	fwts_acpidump_field fields[] = {
		FIELD_UINT("Base Address", 	fwts_acpi_table_mcfg, 	base_address),
		FIELD_UINT("Base Reserved", 	fwts_acpi_table_mcfg,	base_reserved),
		FIELD_END
	};

	acpi_dump_table_fields(fw, data, fields);

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
		acpi_dump_table_fields(fw, (uint8*)config, fields_config);
		config++;
	}
}

static void acpidump_slit(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_slit *slit = (fwts_acpi_table_slit*)data;
	int i;
	int j = 0;
	int k = 0;
	int n = length - sizeof(fwts_acpi_table_slit);
	uint8 *entry;

	fwts_log_info_verbatum(fw, "# Sys Localities: 0x%lx (%lu)", slit->num_of_system_localities, 
								    slit->num_of_system_localities);
	if (n < slit->num_of_system_localities * slit->num_of_system_localities) {
		fwts_log_info_verbatum(fw,"Expecting %d bytes, got only %d", 
			slit->num_of_system_localities * slit->num_of_system_localities, n);
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


static void acpidump_srat(fwts_framework *fw, uint8 *data, int length)
{
	fwts_acpi_table_slit_local_apic_sapic_affinity *lasa;
	fwts_acpi_table_slit_memory_affinity	       *ma;
	fwts_acpi_table_slit_local_x2apic_affinity     *xa;
	uint8 *ptr;

	ptr = data + sizeof(fwts_acpi_table_srat);

	while (ptr < data + length) {
		switch (*ptr) {
		case 0:
			lasa = (fwts_acpi_table_slit_local_apic_sapic_affinity*)ptr;
			fwts_log_info_verbatum(fw, " Processor Local APIC/SAPID Affinity Structure:");
			fwts_log_info_verbatum(fw, "  Proximity [7:0] 0x%2.2x", lasa->proximity_domain_0);
			fwts_log_info_verbatum(fw, "  APIC ID:        0x%2.2x", lasa->apic_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", lasa->flags);
			fwts_log_info_verbatum(fw, "  L_SAPIC_EID:    0x%2.2x", lasa->local_sapic_eid);
			fwts_log_info_verbatum(fw, "  Proximity [31:8]0x%2.2x%2.2x%2.2x", 
				lasa->proximity_domain_1, lasa->proximity_domain_2, lasa->proximity_domain_3);
			fwts_log_info_verbatum(fw, "  Clock Domain:   0x%lx", lasa->clock_domain);
			ptr += sizeof(fwts_acpi_table_slit_local_apic_sapic_affinity);
			break;
		case 1:
			ma = (fwts_acpi_table_slit_memory_affinity*)ptr;
			fwts_log_info_verbatum(fw, " Memory Affinity Structure:");
			fwts_log_info_verbatum(fw, "  Prox. Domain:   0x%lx", ma->proximity_domain);
			fwts_log_info_verbatum(fw, "  Base Addr Lo:   0x%lx", ma->base_addr_lo);
			fwts_log_info_verbatum(fw, "  Base Addr Hi:   0x%lx", ma->base_addr_hi);
			fwts_log_info_verbatum(fw, "  Length Lo:      0x%lx", ma->length_lo);
			fwts_log_info_verbatum(fw, "  Length Hi:      0x%lx", ma->length_hi);
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", ma->flags);
			ptr += sizeof(fwts_acpi_table_slit_memory_affinity);
			break;
		case 2:
			xa = (fwts_acpi_table_slit_local_x2apic_affinity*)ptr;
			fwts_log_info_verbatum(fw, " Processor Local x2APIC Affinit Structure:");
			fwts_log_info_verbatum(fw, "  Prox. Domain:   0x%lx", xa->proximity_domain);
			fwts_log_info_verbatum(fw, "  X2APIC ID:      0x%lx", xa->x2apic_id);
			fwts_log_info_verbatum(fw, "  Flags:          0x%lx", xa->flags);
			fwts_log_info_verbatum(fw, "  Clock Domain:   0x%lx", xa->clock_domain);
			ptr += sizeof(fwts_acpi_table_slit_local_x2apic_affinity);
			break;
		default:
			ptr = data + length; /* force abort */
			break;
		}
	}
}


typedef struct {
	char *name;
	void (*func)(fwts_framework *fw, uint8 *data, int length);
	int  standard_header;
} acpidump_table_vec;

/* To be implemented */
#define acpidump_einj		acpi_dump_raw_table
#define acpidump_hest		acpi_dump_raw_table
#define acpidump_msct		acpi_dump_raw_table

acpidump_table_vec table_vec[] = {
	{ "APIC", 	acpidump_madt, 	1 },
	{ "BERT", 	acpidump_bert, 	1 },
	{ "BOOT", 	acpidump_boot, 	1 },
	{ "CPEP", 	acpidump_cpep, 	1 },
	{ "DSDT", 	acpidump_amlcode, 1 },
	{ "ECDT", 	acpidump_ecdt, 	1 },
	{ "EINJ", 	acpidump_einj, 	1 },
	{ "ERST", 	acpidump_erst, 	1 },
	{ "FACP", 	acpidump_fadt, 	1 },
	{ "FACS", 	acpidump_facs, 	0 },
	{ "HEST", 	acpidump_hest, 	1 },
	{ "HPET", 	acpidump_hpet, 	1 },
	{ "MCFG", 	acpidump_mcfg, 	1 },
	{ "MSCT", 	acpidump_msct, 	1 },
	{ "PSDT", 	acpidump_amlcode, 1 },
	{ "RSDT", 	acpidump_rsdt, 	1 },
	{ "RSD PTR ", 	acpidump_rsdp, 	0 },
	{ "SBST", 	acpidump_sbst,  1 },
	{ "SSDT", 	acpidump_amlcode, 1 },
	{ "SLIT", 	acpidump_slit,  1 },
	{ "SRAT", 	acpidump_srat,  1 },
	{ "XSDT", 	acpidump_xsdt, 	1 },
	{ NULL,		NULL,		0 },
};

static int acpidump_table(fwts_framework *fw, fwts_acpi_table_info *table)
{
	uint8 *data;
	fwts_acpi_table_header hdr;
	int length;
	int i;

	data = table->data;
	length = table->length;

	for (i=0; table_vec[i].name != NULL; i++) {
		if (strncmp(table_vec[i].name, (char*)data, strlen(table_vec[i].name)) == 0) {
			if (table_vec[i].standard_header) {
				fwts_acpi_table_get_header(&hdr, data);
				acpidump_hdr(fw, &hdr);
			}
			table_vec[i].func(fw, data, length);
			return FWTS_OK;
		}
	}

	/* Cannot find, assume standard table header */
	fwts_acpi_table_get_header(&hdr, data);
	acpidump_hdr(fw, &hdr);
	acpi_dump_raw_table(fw, data, length);

	return FWTS_OK;
}

static int acpidump_test1(fwts_framework *fw)
{
	int i;

	fwts_acpi_table_info *table;

	for (i=0; (table = fwts_acpi_get_table(i)) !=NULL; i++) {
		fwts_log_info_verbatum(fw, "%s @ %lx", table->name, table->addr);
		fwts_log_info_verbatum(fw, "-----------------------");
		acpidump_table(fw, table);
		fwts_log_nl(fw);
	}

	return FWTS_OK;
}


static fwts_framework_tests acpidump_tests[] = {
	acpidump_test1,
	NULL
};

static fwts_framework_ops acpidump_ops = {
	acpidump_headline,
	acpidump_init,
	NULL,
	acpidump_tests
};

FWTS_REGISTER(acpidump, &acpidump_ops, FWTS_TEST_ANYTIME, FWTS_BATCH_EXPERIMENTAL);
