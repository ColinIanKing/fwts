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

#ifndef __FWTS_ACPI_H__
#define __FWTS_ACPI_H__

#define FWTS_ACPI_TABLES_PATH   "/sys/firmware/acpi/tables"

#define FWTS_FACP_UNSPECIFIED		(0x00)
#define FWTS_FACP_DESKTOP		(0x01)
#define FWTS_FACP_MOBILE		(0x02)
#define FWTS_FACP_WORKSTATION		(0x03)
#define FWTS_FACP_ENTERPRISE_SERVER	(0x04)
#define FWTS_FACP_SOHO_SERVER		(0x05)
#define FWTS_FACP_APPLIANCE_PC		(0x06)
#define FWTS_FACP_PERFORMANCE_SERVER	(0x07)
#define FWTS_FACP_TABLET		(0x08)

#define FWTS_FACP_IAPC_BOOT_ARCH_LEGACY_DEVICES		(0x0001)
#define FWTS_FACP_IAPC_BOOT_ARCH_8042			(0x0002)
#define FWTS_FACP_IAPC_BOOT_ARCH_VGA_NOT_PRESENT	(0x0004)
#define FWTS_FACP_IAPC_BOOT_ARCH_MSI_NOT_SUPPORTED	(0x0008)
#define FWTS_FACP_IAPC_BOOT_ARCH_PCIE_ASPM_CONTROLS	(0x0010)

#include "fwts_types.h"
#include "fwts_framework.h"
#include "fwts_log.h"

extern char *fwts_acpi_fadt_preferred_pm_profile[];

#define FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(x)		\
	((x) > 7) ? "Reserved" : fwts_acpi_fadt_preferred_pm_profile[x]

/* 5.2.3.1 Generic Address Structure */
typedef struct {
	uint8_t 	address_space_id;
	uint8_t	register_bit_width;
        uint8_t 	register_bit_offset;
        uint8_t 	access_width;
        uint64_t 	address;
} __attribute__ ((packed)) fwts_acpi_gas;

typedef struct {
	char		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	char		oem_id[6];
	char		oem_tbl_id[8];
	uint32_t	oem_revision;
	char		creator_id[4];
	uint32_t	creator_revision;
} __attribute__ ((packed)) fwts_acpi_table_header;

typedef struct {
	fwts_acpi_table_header	header;	
	uint8_t		cmos_index;
	uint8_t		reserved[3];
} __attribute__ ((packed)) fwts_acpi_table_boot;

typedef struct {
	fwts_acpi_table_header	header;	
	uint32_t	boot_error_region_length;
	uint64_t	boot_error_region;
	uint32_t	boot_status;
	uint32_t	raw_data_offset;
	uint32_t	raw_data_length;
	uint32_t	data_length;
	uint32_t	error_severity;
	uint8_t		generic_error_data[0];
} __attribute__ ((packed)) fwts_acpi_table_bert;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint8_t		processor_id;
	uint8_t		processor_eid;
	uint32_t	polling_interval;
} __attribute__ ((packed)) fwts_acpi_cpep_processor_info;

typedef struct {
	fwts_acpi_table_header	header;	
	uint8_t		reserved[8];
	fwts_acpi_cpep_processor_info	cpep_info[0];
} __attribute__ ((packed)) fwts_acpi_table_cpep;

typedef struct {
	fwts_acpi_table_header	header;	
	fwts_acpi_gas	ec_control;
	fwts_acpi_gas	ec_data;
	uint32_t	uid;
	uint8_t		gpe_bit;
	uint8_t		ec_id[0];
} __attribute__ ((packed)) fwts_acpi_table_ecdt;

typedef enum {
	FWTS_BOOT_REGISTER_PNPOS	= 0x01,
	FWTS_BOOT_REGISTER_BOOTING	= 0x02,
	FWTS_BOOT_REGISTER_DIAG		= 0x04,
	FWTS_BOOT_REGISTER_SUPPRESS_BOOT_DISPLAY = 0x08,
	FWTS_BOOT_REGISTER_PARITY	= 0x80
} ftws_acpi_cmos_boot_register;

typedef struct {
	char		signature[4];
	uint32_t	length;
	uint32_t	hardware_signature;
	uint32_t	firmware_waking_vector;
	uint32_t	global_lock;
	uint32_t	flags;
	uint64_t	x_firmware_waking_vector;
	uint8_t		version;
	uint8_t		reserved[3];
	uint32_t	ospm_flags;
	uint8_t		reserved2[24];
} __attribute__ ((packed)) fwts_acpi_table_facs;

typedef struct {
	char		signature[8];
	uint8_t		checksum;
	char		oem_id[6];
	uint8_t		revision;
	uint32_t	rsdt_address;
	uint32_t	length;
	uint64_t	xsdt_address;
	uint8_t		extended_checksum;
	uint8_t		reserved[3];
} __attribute__ ((packed)) fwts_acpi_table_rsdp;

typedef struct {
	fwts_acpi_table_header header;
	uint64_t	entries[0];
} __attribute__ ((packed)) fwts_acpi_table_xsdt;

typedef struct {
	fwts_acpi_table_header header;
	uint32_t	entries[0];
} __attribute__ ((packed)) fwts_acpi_table_rsdt;

typedef struct {
	fwts_acpi_table_header header;
	uint32_t	warning_energy_level;
	uint32_t	low_energy_level;
	uint32_t	critical_energy_level;
} __attribute__ ((packed)) fwts_acpi_table_sbst;
/*
 *  From ACPI Spec, section 5.2.9 Fixed ACPI Description Field
 */
typedef struct {
	fwts_acpi_table_header	header;	
	uint32_t	firmware_control;
	uint32_t	dsdt;
	uint8_t		reserved;
	uint8_t		preferred_pm_profile;
	uint16_t	sci_int;
	uint32_t	smi_cmd;
	uint8_t		acpi_enable;
	uint8_t		acpi_disable;
	uint8_t		s4bios_req;
	uint8_t		pstate_cnt;
	uint32_t	pm1a_evt_blk;
	uint32_t	pm1b_evt_blk;
	uint32_t	pm1a_cnt_blk;
	uint32_t	pm1b_cnt_blk;
	uint32_t	pm2_cnt_blk;
	uint32_t	pm_tmr_blk;
	uint32_t	gpe0_blk;
	uint32_t	gpe1_blk;
	uint8_t		pm1_evt_len;
	uint8_t		pm1_cnt_len;
	uint8_t		pm2_cnt_len;
	uint8_t		pm_tmr_len;
	uint8_t		gpe0_blk_len;
	uint8_t		gpe1_blk_len;
	uint8_t		gpe1_base;
	uint8_t		cst_cnt;
	uint16_t	p_lvl2_lat;
	uint16_t	p_lvl3_lat;
	uint16_t	flush_size;
	uint16_t	flush_stride;
	uint8_t		duty_offset;
	uint8_t		duty_width;
	uint8_t		day_alrm;
	uint8_t		mon_alrm;
	uint8_t		century;
	uint16_t	iapc_boot_arch;
	uint8_t		reserved1;
	uint32_t	flags;
	fwts_acpi_gas	reset_reg;
	uint8_t		reset_value;
	uint8_t		reserved2[3];
	uint64_t	x_firmware_ctrl;
	uint64_t	x_dsdt;
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
	uint32_t	base_address;
	uint32_t	base_reserved;
	uint16_t	pci_segment_group_number;
	uint8_t		start_bus_number;
	uint8_t		end_bus_number;
	uint8_t		reserved[4];
}  __attribute__ ((packed)) fwts_acpi_mcfg_configuration;

typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	base_address;
	uint32_t	base_reserved;
	fwts_acpi_mcfg_configuration configuration[0];
} __attribute__ ((packed)) fwts_acpi_table_mcfg;

typedef struct {
	fwts_acpi_table_header	header;
	uint64_t	num_of_system_localities;
	/* matrix follows */
} __attribute__ ((packed)) fwts_acpi_table_slit;

typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	reserved1;
	uint64_t	reserved2;
} __attribute__ ((packed)) fwts_acpi_table_srat;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint8_t		proximity_domain_0;
	uint8_t		apic_id;
	uint32_t	flags;
	uint8_t		local_sapic_eid;
	uint8_t		proximity_domain_1;
	uint8_t		proximity_domain_2;
	uint8_t		proximity_domain_3;
	uint32_t		clock_domain;
} __attribute__ ((packed)) fwts_acpi_table_slit_local_apic_sapic_affinity;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint32_t	proximity_domain;
	uint16_t	reserved1;
	uint32_t	base_addr_lo;
	uint32_t	base_addr_hi;
	uint32_t	length_lo;
	uint32_t	length_hi;
	uint32_t	reserved2;
	uint32_t	flags;
	uint64_t	reserved3;
} __attribute__ ((packed)) fwts_acpi_table_slit_memory_affinity;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint16_t	reserved1;
	uint32_t	proximity_domain;
	uint32_t	x2apic_id;
	uint32_t	flags;
	uint32_t	clock_domain;
	uint32_t	reserved2;
} __attribute__ ((packed)) fwts_acpi_table_slit_local_x2apic_affinity;

/* from 3.2.4 The ACPI 2.0 HPET Description Table (HPET) http://www.intel.com/hardwaredesign/hpetspec_1.pdf */

typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	event_timer_block_id;
	fwts_acpi_gas	base_address;
	uint8_t		hpet_number;
	uint16_t	main_counter_minimum;
	uint8_t		page_prot_and_oem_attribute;
} __attribute__ ((packed)) fwts_acpi_table_hpet;


typedef struct {
	uint8_t		serialization_action;
	uint8_t		instruction;
	uint8_t		flags;
	uint8_t		reserved;
	fwts_acpi_gas	register_region;
	uint64_t	value;
	uint64_t	mask;
} __attribute__ ((packed)) fwts_acpi_serialization_instruction_entries;

typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	serialization_header_size;
	uint32_t	reserved;
	uint32_t	instruction_entry_count;
	fwts_acpi_serialization_instruction_entries	entries[0];
} __attribute__ ((packed)) fwts_acpi_table_erst;


/* MADT, Section 5.2.12 of ACPI spec, Multiple APIC Description Table */

typedef struct {
	uint8_t		type;
	uint8_t		length;
} __attribute__ ((packed)) fwts_acpi_madt_sub_table_header;

typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	lapic_address;
	uint32_t	flags;
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
	uint8_t		acpi_processor_id;
	uint8_t		apic_id;
	uint32_t	flags;
}  __attribute__ ((packed)) fwts_acpi_madt_processor_local_apic;

typedef struct {
	uint8_t		io_apic_id;
	uint8_t		reserved;
	uint32_t	io_apic_phys_address;
	uint32_t	global_irq_base;
} __attribute__ ((packed)) fwts_acpi_madt_io_apic;

typedef struct {
	uint8_t		bus;
	uint8_t		source;
	uint32_t	gsi;
	uint16_t	flags;
} __attribute__ ((packed)) fwts_acpi_madt_interrupt_override;

typedef struct {
	uint16_t	flags;	
	uint32_t	gsi;
} __attribute__ ((packed)) fwts_acpi_madt_nmi;

typedef struct {
	uint8_t		acpi_processor_id;
	uint16_t	flags;	
	uint8_t		local_apic_lint;
} __attribute__ ((packed)) fwts_acpi_madt_local_apic_nmi;

typedef struct {
	uint16_t	reserved;
	uint64_t	address;
} __attribute__ ((packed)) fwts_acpi_madt_local_apic_addr_override;

typedef struct {
	uint8_t		io_sapic_id;
	uint8_t		reserved;
	uint32_t	gsi;
	uint64_t	address;
} __attribute__ ((packed)) fwts_acpi_madt_io_sapic;

typedef struct {
	uint8_t		acpi_processor_id;
	uint8_t		local_sapic_id;
	uint8_t		local_sapic_eid;
	uint8_t		reserved;
	uint32_t	flags;
	uint32_t	uid_value;
	char		uid_string[0];
} __attribute__ ((packed)) fwts_acpi_madt_local_sapic;

typedef struct {
	uint16_t	flags;
	uint8_t		type;
	uint8_t		processor_id;
	uint8_t		processor_eid;
	uint8_t		io_sapic_vector;
	uint32_t	gsi;
	uint32_t	pis_flags;
} __attribute__ ((packed)) fwts_acpi_madt_platform_int_source;

typedef struct {
	uint16_t	reserved;
	uint32_t	x2apic_id;
	uint32_t	flags;
	uint32_t	processor_uid;
} __attribute__ ((packed)) fwts_acpi_madt_local_x2apic;

typedef struct {
	uint16_t	flags;
	uint32_t	processor_uid;
	uint8_t		local_x2apic_lint;
	uint8_t		reserved[3];
} __attribute__ ((packed)) fwts_acpi_madt_local_x2apic_nmi;

/* From http://www.kuro5hin.org/story/2002/10/27/16622/530,
   and also http://www.cl.cam.ac.uk/~rja14/tcpa-faq.html */
typedef struct {
	fwts_acpi_table_header	header;
	uint16_t	reserved;
	uint32_t	log_zone_length;
	uint64_t	log_zone_addr;
}  __attribute__ ((packed)) fwts_acpi_table_tcpa;

void fwts_acpi_table_get_header(fwts_acpi_table_header *hdr, uint8_t *data);

#endif
