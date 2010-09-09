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

extern char *fwts_acpi_fadt_preferred_pm_profile[];

#define FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(x)		\
	((x) > 7) ? "Reserved" : fwts_acpi_fadt_preferred_pm_profile[x]

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

/* 5.2.3.1 Generic Address Structure */
typedef struct {
	uint8 	address_space_id;
	uint8	register_bit_width;
        uint8 	register_bit_offset;
        uint8 	access_width;
        uint64 	address;
} __attribute__ ((packed)) fwts_acpi_gas;

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
} __attribute__ ((packed)) fwts_acpi_table_header;

typedef struct {
	fwts_acpi_table_header	header;	
	uint8		cmos_index;
	uint8		reserved[3];
} __attribute__ ((packed)) fwts_acpi_table_boot;

typedef struct {
	fwts_acpi_table_header	header;	
	uint32		boot_error_region_length;
	uint64		boot_error_region;
	uint32		boot_status;
	uint32		raw_data_offset;
	uint32		raw_data_length;
	uint32		data_length;
	uint32		error_severity;
	uint8		generic_error_data[0];
} __attribute__ ((packed)) fwts_acpi_table_bert;

typedef struct {
	uint8		type;
	uint8		length;
	uint8		processor_id;
	uint8		processor_eid;
	uint32		polling_interval;
} __attribute__ ((packed)) fwts_acpi_cpep_processor_info;

typedef struct {
	fwts_acpi_table_header	header;	
	uint8		reserved[8];
	fwts_acpi_cpep_processor_info	cpep_info[0];
} __attribute__ ((packed)) fwts_acpi_table_cpep;

typedef enum {
	FTWS_BOOT_REGISTER_PNPOS	= 0x01,
	FWTS_BOOT_REGISTER_BOOTING	= 0x02,
	FWTS_BOOT_REGISTER_DIAG		= 0x04,
	FWTS_BOOT_REGISTER_SUPPRESS_BOOT_DISPLAY = 0x08,
	FWTS_BOOT_REGISTER_PARITY	= 0x80
} ftws_acpi_cmos_boot_register;

typedef struct {
	char		signature[4];
	uint32		length;
	uint32		hardware_signature;
	uint32		firmware_waking_vector;
	uint32		global_lock;
	uint32		flags;
	uint64		x_firmware_waking_vector;
	uint8		version;
	uint8		reserved[3];
	uint32		ospm_flags;
	uint8		reserved2[24];
} __attribute__ ((packed)) fwts_acpi_table_facs;

typedef struct {
	char		signature[8];
	uint8		checksum;
	char		oem_id[6];
	uint8		revision;
	uint32		rsdt_address;
	uint32		length;
	uint32		xsdt_address;
	uint8		extended_checksum;
	uint8		reserved[3];
} __attribute__ ((packed)) fwts_acpi_table_rsdp;

typedef struct {
	fwts_acpi_table_header header;
	uint64		entries[0];
} __attribute__ ((packed)) fwts_acpi_table_xsdt;

typedef struct {
	fwts_acpi_table_header header;
	uint32		entries[0];
} __attribute__ ((packed)) fwts_acpi_table_rsdt;

/*
 *  From ACPI Spec, section 5.2.9 Fixed ACPI Description Field
 */
typedef struct {
	fwts_acpi_table_header	header;	
	uint32		firmware_control;
	uint32		dsdt;
	uint8		reserved;
	uint8		preferred_pm_profile;
	uint16		sci_int;
	uint32		smi_cmd;
	uint8		acpi_enable;
	uint8		acpi_disable;
	uint8		s4bios_req;
	uint8		pstate_cnt;
	uint32		pm1a_evt_blk;
	uint32		pm1b_evt_blk;
	uint32		pm1a_cnt_blk;
	uint32		pm1b_cnt_blk;
	uint32		pm2_cnt_blk;
	uint32		pm_tmr_blk;
	uint32		gpe0_blk;
	uint32		gpe1_blk;
	uint8		gpe1_base;
	uint8		pm1_evt_len;
	uint8		pm1_cnt_len;
	uint8		pm2_cnt_len;
	uint8		pm_tmr_len;
	uint8		gpe0_blk_len;
	uint8		gpe1_blk_len;
	uint8		cst_cnt;
	uint16		p_lvl2_lat;
	uint16		p_lvl3_lat;
	uint16		flush_size;
	uint16		flush_stride;
	uint8		duty_offset;
	uint8		duty_width;
	uint8		day_alrm;
	uint8		mon_alrm;
	uint8		century;
	uint16		iapc_boot_arch;
	uint8		reserved1;
	uint32		flags;
	fwts_acpi_gas	reset_reg;
	uint8		reset_value;
	uint8		reserved2[3];
	uint64		x_firmware_ctrl;
	uint64		x_dsdt;
	fwts_acpi_gas	x_pm1a_evt_blk;
	fwts_acpi_gas	x_pm1b_evt_blk;
	fwts_acpi_gas	x_pm1a_cnt_blk;
	fwts_acpi_gas	x_pm1b_cnt_blk;
	fwts_acpi_gas	x_pm2_cnt_blk;
	fwts_acpi_gas	x_pm_tmr_blk;
	fwts_acpi_gas	x_gpe0_blk;
	fwts_acpi_gas	x_gpe1_blk;
} __attribute__ ((packed)) fwts_acpi_table_fadt;

typedef struct {
	uint32		base_address;
	uint32		base_reserved;
	uint16		pci_segment_group_number;
	uint8		start_bus_number;
	uint8		end_bus_number;
	uint8		reserved[4];
}  __attribute__ ((packed)) fwts_acpi_mcfg_configuration;

typedef struct {
	fwts_acpi_table_header	header;
	uint32		base_address;
	uint32		base_reserved;
	fwts_acpi_mcfg_configuration configuration[0];
} __attribute__ ((packed)) fwts_acpi_table_mcfg;


/* from 3.2.4 The ACPI 2.0 HPET Description Table (HPET) http://www.intel.com/hardwaredesign/hpetspec_1.pdf */

typedef struct {
	fwts_acpi_table_header	header;
	uint32		event_timer_block_id;
	fwts_acpi_gas	base_address;
	uint8		hpet_number;
	uint16		main_counter_minimum;
	uint8		page_prot_and_oem_attribute;
} __attribute__ ((packed)) fwts_acpi_table_hpet;


/* MADT, Section 5.2.12 of ACPI spec, Multiple APIC Description Table */

typedef struct {
	uint8		type;
	uint8		length;
} __attribute__ ((packed)) fwts_acpi_madt_sub_table_header;

typedef struct {
	fwts_acpi_table_header	header;
	uint32		lapic_address;
	uint32		flags;
} __attribute__ ((packed)) fwts_acpi_table_madt;

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
	uint8		acpi_processor_id;
	uint8		apic_id;
	uint32		flags;
}  __attribute__ ((packed)) fwts_acpi_madt_processor_local_apic;

typedef struct {
	uint8		io_apic_id;
	uint8		reserved;
	uint32		io_apic_phys_address;
	uint32		global_irq_base;
} __attribute__ ((packed)) fwts_acpi_madt_io_apic;

typedef struct {
	uint8		bus;
	uint8		source;
	uint32		gsi;
	uint16		flags;
} __attribute__ ((packed)) fwts_acpi_madt_interrupt_override;

typedef struct {
	uint16		flags;	
	uint32		gsi;
} __attribute__ ((packed)) fwts_acpi_madt_nmi;

typedef struct {
	uint8		acpi_processor_id;
	uint16		flags;	
	uint8		local_apic_lint;
} __attribute__ ((packed)) fwts_acpi_madt_local_apic_nmi;

typedef struct {
	uint16		reserved;
	uint64		address;
} __attribute__ ((packed)) fwts_acpi_madt_local_apic_addr_override;

typedef struct {
	uint8		io_sapic_id;
	uint8		reserved;
	uint32		gsi;
	uint64		address;
} __attribute__ ((packed)) fwts_acpi_madt_io_sapic;

typedef struct {
	uint8		acpi_processor_id;
	uint8		local_sapic_id;
	uint8		local_sapic_eid;
	uint8		reserved;
	uint32		flags;
	uint32		uid_value;
	char		uid_string[0];
} __attribute__ ((packed)) fwts_acpi_madt_local_sapic;

typedef struct {
	uint16		flags;
	uint8		type;
	uint8		processor_id;
	uint8		processor_eid;
	uint8		io_sapic_vector;
	uint32		gsi;
	uint32		pis_flags;
} __attribute__ ((packed)) fwts_acpi_madt_platform_int_source;

typedef struct {
	uint16		reserved;
	uint32		x2apic_id;
	uint32		flags;
	uint32		processor_uid;
} __attribute__ ((packed)) fwts_acpi_madt_local_x2apic;

typedef struct {
	uint16		flags;
	uint32		processor_uid;
	uint8		local_x2apic_lint;
	uint8		reserved[3];
} __attribute__ ((packed)) fwts_acpi_madt_local_x2apic_nmi;

void fwts_acpi_table_get_header(fwts_acpi_table_header *hdr, uint8 *data);

#endif
