/*
 * Copyright (C) 2010-2015 Canonical
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

#define FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY		(0x00)
#define FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO		(0x01)
#define FWTS_GAS_ADDR_SPACE_ID_PCI_CONFIG		(0x02)
#define FWTS_GAS_ADDR_SPACE_ID_EC			(0x03)
#define FWTS_GAS_ADDR_SPACE_ID_SMBUS			(0x04)
#define FWTS_GAS_ADDR_SPACE_ID_PCC			(0x0a)
#define FWTS_GAS_ADDR_SPACE_ID_FFH			(0x7f)

#include "fwts_types.h"
#include "fwts_framework.h"
#include "fwts_log.h"

extern const char *fwts_acpi_fadt_preferred_pm_profile[];

#define FWTS_ACPI_FADT_PREFERRED_PM_PROFILE(x)		\
	((x) > 8) ? "Reserved" : fwts_acpi_fadt_preferred_pm_profile[x]
#define FWTS_ACPI_FADT_FLAGS_HW_REDUCED_ACPI (1<<20)

/* 5.2.3.1 Generic Address Structure */
typedef struct {
	uint8_t 	address_space_id;
	uint8_t		register_bit_width;
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
} __attribute__ ((packed)) fwts_acpi_table_bert;

typedef struct {
	uint32_t	block_status;
	uint32_t	raw_data_offset;
	uint32_t	raw_data_length;
	uint32_t	data_length;
	uint32_t	error_severity;
	uint8_t		generic_error_data[0];
} __attribute__ ((packed)) fwts_acpi_table_boot_error_region;

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
	uint16_t	arm_boot_flags;
	uint8_t		minor_version;
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
	fwts_acpi_gas	sleep_control_reg;
	fwts_acpi_gas	sleep_status_reg;
} __attribute__ ((packed)) fwts_acpi_table_fadt;

typedef struct {
	uint64_t	base_address;
	uint16_t	pci_segment_group_number;
	uint8_t		start_bus_number;
	uint8_t		end_bus_number;
	uint8_t		reserved[4];
}  __attribute__ ((packed)) fwts_acpi_mcfg_configuration;

typedef struct {
	fwts_acpi_table_header	header;
	uint64_t	reserved;
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
	uint32_t	clock_domain;
} __attribute__ ((packed)) fwts_acpi_table_local_apic_sapic_affinity;

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
} __attribute__ ((packed)) fwts_acpi_table_memory_affinity;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint16_t	reserved1;
	uint32_t	proximity_domain;
	uint32_t	x2apic_id;
	uint32_t	flags;
	uint32_t	clock_domain;
	uint32_t	reserved2;
} __attribute__ ((packed)) fwts_acpi_table_local_x2apic_affinity;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint32_t	proximity_domain;
	uint32_t	acpi_processor_uid;
	uint32_t	flags;
	uint32_t	clock_domain;
} __attribute__ ((packed)) fwts_acpi_table_gicc_affinity;

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
	FWTS_ACPI_MADT_GIC_C_CPU_INTERFACE,
	FWTS_ACPI_MADT_GIC_D_GOC_DISTRIBUTOR,
	FWTS_ACPI_MADT_GIC_V2M_MSI_FRAME,
	FWTS_ACPI_MADT_GIC_R_REDISTRIBUTOR,
        FWTS_ACPI_MADT_RESERVED
} fwts_acpi_madt_type;

/* Type 0, FWTS_ACPI_MADT_LOCAL_APIC */
typedef struct {
	uint8_t		acpi_processor_id;
	uint8_t		apic_id;
	uint32_t	flags;
}  __attribute__ ((packed)) fwts_acpi_madt_processor_local_apic;

/* Type 1, FWTS_ACPI_MADT_IO_APIC */
typedef struct {
	uint8_t		io_apic_id;
	uint8_t		reserved;
	uint32_t	io_apic_phys_address;
	uint32_t	global_irq_base;
} __attribute__ ((packed)) fwts_acpi_madt_io_apic;

/* Type 2, FWTS_ACPI_MADT_INTERRUPT_OVERRIDE */
typedef struct {
	uint8_t		bus;
	uint8_t		source;
	uint32_t	gsi;
	uint16_t	flags;
} __attribute__ ((packed)) fwts_acpi_madt_interrupt_override;

/* Type 3, FWTS_ACPI_MADT_NMI_SOURCE */
typedef struct {
	uint16_t	flags;	
	uint32_t	gsi;
} __attribute__ ((packed)) fwts_acpi_madt_nmi;

/* Type 4, FWTS_ACPI_MADT_LOCAL_APIC_NMI */
typedef struct {
	uint8_t		acpi_processor_id;
	uint16_t	flags;	
	uint8_t		local_apic_lint;
} __attribute__ ((packed)) fwts_acpi_madt_local_apic_nmi;

/* Type 5, FWTS_ACPI_MADT_LOCAL_APIC_OVERRIDE */
typedef struct {
	uint16_t	reserved;
	uint64_t	address;
} __attribute__ ((packed)) fwts_acpi_madt_local_apic_addr_override;

/* Type 6, FWTS_ACPI_MADT_IO_SAPIC */
typedef struct {
	uint8_t		io_sapic_id;
	uint8_t		reserved;
	uint32_t	gsi;
	uint64_t	address;
} __attribute__ ((packed)) fwts_acpi_madt_io_sapic;

/* Type 7, FWTS_ACPI_MADT_LOCAL_SAPIC */
typedef struct {
	uint8_t		acpi_processor_id;
	uint8_t		local_sapic_id;
	uint8_t		local_sapic_eid;
	uint8_t		reserved;
	uint32_t	flags;
	uint32_t	uid_value;
	char		uid_string[0];
} __attribute__ ((packed)) fwts_acpi_madt_local_sapic;

/* Type 8, FWTS_ACPI_MADT_INTERRUPT_SOURCE */
typedef struct {
	uint16_t	flags;
	uint8_t		type;
	uint8_t		processor_id;
	uint8_t		processor_eid;
	uint8_t		io_sapic_vector;
	uint32_t	gsi;
	uint32_t	pis_flags;
} __attribute__ ((packed)) fwts_acpi_madt_platform_int_source;

/* Type 9, FWTS_ACPI_MADT_LOCAL_X2APIC */
typedef struct {
	uint16_t	reserved;
	uint32_t	x2apic_id;
	uint32_t	flags;
	uint32_t	processor_uid;
} __attribute__ ((packed)) fwts_acpi_madt_local_x2apic;

/* Type 10, FWTS_ACPI_MADT_LOCAL_X2APIC_NMI */
typedef struct {
	uint16_t	flags;
	uint32_t	processor_uid;
	uint8_t		local_x2apic_lint;
	uint8_t		reserved[3];
} __attribute__ ((packed)) fwts_acpi_madt_local_x2apic_nmi;

/* Type 11, FWTS_ACPI_MADT_GIC_C_CPU_INTERFACE */
/* New in ACPI 5.0, GIC, section 5.2.12.14 */
typedef struct {
	uint16_t	reserved;
	uint32_t	gic_id;
	uint32_t	processor_uid;
	uint32_t	flags;
	uint32_t	parking_protocol_version;
	uint32_t	performance_interrupt_gsiv;
	uint64_t	parked_address;
	uint64_t	physical_base_address;
	uint64_t	gicv;
	uint64_t	gich;
	uint32_t	vgic;
	uint64_t	gicr_base_address;
	uint64_t	mpidr;
} __attribute__ ((packed)) fwts_acpi_madt_gic;

/* New in ACPI 5.0, GICD, section 5.2.12.15 */
/* Type 12, FWTS_ACPI_MADT_GIC_D_GOC_DISTRIBUTOR */
typedef struct {
	uint16_t	reserved;
	uint32_t	gic_id;
	uint64_t	physical_base_address;
	uint32_t	system_vector_base;
	uint32_t	reserved2;
} __attribute__ ((packed)) fwts_acpi_madt_gicd;

/* New in ACPI 5.1, GIC MSI Frame structure, 5.2.12.16 */
/* Type 13, FWTS_ACPI_MADT_GIC_V2M_MSI_FRAME */
typedef struct {
	uint16_t	reserved;
	uint32_t	frame_id;
	uint64_t	physical_base_address;
	uint32_t	flags;
	uint16_t	spi_count;
	uint16_t	spi_base;
	uint32_t	spi_flags;	/* bit 0 just used at the moment */
} __attribute__ ((packed)) fwts_acpi_madt_gic_msi;

/* New in ACPI 5.1, GICR structure, 5.2.12.17 */
/* Type 14, FWTS_ACPI_MADT_GIC_R_REDISTRIBUTOR */
typedef struct {
	uint16_t	reserved;
	uint64_t	discovery_range_base_address;
	uint32_t	discovery_range_length;
} __attribute__ ((packed)) fwts_acpi_madt_gicr;

/* From http://www.kuro5hin.org/story/2002/10/27/16622/530,
   and also http://www.cl.cam.ac.uk/~rja14/tcpa-faq.html */
typedef struct {
	fwts_acpi_table_header	header;
	uint16_t	platform_class;
	union {
		struct client_hdr {
			uint32_t log_zone_length;
			uint64_t log_zone_addr;
		}  __attribute__ ((packed)) client;
		struct server_hdr {
			uint16_t reserved;
			uint64_t log_zone_length;
			uint64_t log_zone_addr;
			uint16_t spec_revision;
			uint8_t device_flag;
			uint8_t interrupt_flag;
			uint8_t gpe;
			uint8_t reserved2[3];
			uint32_t global_sys_interrupt;
			fwts_acpi_gas base_addr;
			uint32_t reserved3;
			fwts_acpi_gas config_addr;
			uint8_t pci_seg_number;
			uint8_t pci_bus_number;
			uint8_t pci_dev_number;
			uint8_t pci_func_number;
		} __attribute__ ((packed)) server;
	};
}  __attribute__ ((packed)) fwts_acpi_table_tcpa;

/* Following ASF definitions from
   http://dmtf.org/documents/asf/alert-standard-format-asf-specification-200 */
typedef struct {
	uint8_t		type;
	uint8_t		reserved;
	uint16_t	length;
} __attribute__ ((packed)) fwts_acpi_table_asf_header;

typedef struct {
	uint8_t		watchdog_reset_value;
	uint8_t		min_sensor_poll_wait_time;
	uint16_t	id;
	uint32_t	iana_id;
	uint8_t		flags;
	uint8_t		reserved1;
	uint8_t		reserved2;
	uint8_t		reserved3;
} __attribute__ ((packed)) fwts_acpi_table_asf_info;

typedef struct {
	uint8_t		device_addr;
	uint8_t		command;
	uint8_t		data_mask;
	uint8_t		compare_value;
	uint8_t		sensor_type;
	uint8_t		event_type;
	uint8_t		event_offset;
	uint8_t		event_source_type;
	uint8_t		event_severity;
	uint8_t		sensor_number;
	uint8_t		entity;
	uint8_t		entity_instance;
}  __attribute__ ((packed)) fwts_acpi_table_asf_alrt_element;

typedef struct {
	uint8_t		assertion_mask;
	uint8_t		deassertion_mask;
	uint8_t		number_of_alerts;
	uint8_t		array_length;
	uint8_t		device_length[0];
} __attribute__ ((packed)) fwts_acpi_table_asf_alrt;

typedef struct {
	uint8_t		control_function;
	uint8_t		control_device_addr;
	uint8_t		control_command;
	uint8_t		control_value;
} __attribute__ ((packed)) fwts_acpi_table_asf_rctl_element;

typedef struct {
	uint8_t		number_of_controls;
	uint8_t		array_element_length;
	uint16_t	reserved;
	fwts_acpi_table_asf_rctl_element	elements[0];
} __attribute__ ((packed)) fwts_acpi_table_asf_rctl;

typedef struct {
	uint8_t		remote_control_capabilities[7];
	uint8_t		rcmp_completion_code;
	uint8_t		rcmp_iana[4];
	uint8_t		rcmp_special_command;
	uint8_t		rcmp_special_command_param[2];
	uint8_t		rcmp_boot_options[2];
	uint8_t		rcmp_oem_parameters[2];
} __attribute__ ((packed)) fwts_acpi_table_asf_rcmp;

typedef struct {
	uint8_t		fixed_smbus_addr;
} __attribute__ ((packed)) fwts_acpi_table_asf_addr_element;

typedef struct {
	uint8_t		seeprom_addr;
	uint8_t		number_of_devices;
	uint8_t		fwts_acpi_table_asf_addr_element[0];
} __attribute__ ((packed)) fwts_acpi_table_asf_addr;

/*
 *  DMAR
 *  See http://download.intel.com/technology/computing/vptech/Intel(r)_VT_for_Direct_IO.pdf
 */
typedef struct {
	fwts_acpi_table_header header;
	uint8_t		host_addr_width;
	uint8_t		flags;
	uint8_t		reserved[10];
} __attribute__ ((packed)) fwts_acpi_table_dmar;

typedef struct {
	uint16_t	type;
	uint16_t	length;
} __attribute__ ((packed)) fwts_acpi_table_dmar_header;

/* DMA remapping hardware unit definition structure */
typedef struct {
	fwts_acpi_table_dmar_header header;
	uint8_t		flags;
	uint8_t		reserved;
	uint16_t	segment_number;
	uint64_t	register_base_addr;
	uint8_t		device_scope[0];
} __attribute__ ((packed)) fwts_acpi_table_dmar_hardware_unit;

/* Reserved Memory Defininition */
typedef struct {
	fwts_acpi_table_dmar_header header;
	uint16_t	reserved;
	uint16_t	segment;
	uint64_t	base_address;
	uint64_t	end_address;
} __attribute__ ((packed)) fwts_acpi_table_dmar_reserved_memory;

/* Root Port ATS capability reporting structure */
typedef struct {
	fwts_acpi_table_dmar_header header;
	uint8_t		flags;
	uint8_t		reserved;
	uint16_t	segment;
} __attribute__ ((packed)) fwts_acpi_table_dmar_atsr;

/* DMA remapping device scope entries */
typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint8_t		reserved[2];
	uint8_t		enumeration_id;
	uint8_t		start_bus_number;
	uint8_t		path[0];
} __attribute__ ((packed)) fwts_acpi_table_dmar_device_scope;

/* SLIC, see "OEM Activation 2.0 for Windows Vista Operating Systems" */
typedef struct {
	uint32_t	type;
	uint32_t	length;
} __attribute__ ((packed)) fwts_acpi_table_slic_header;

typedef struct {
	fwts_acpi_table_slic_header header;
	uint8_t		key_type;
	uint8_t		version;
	uint16_t	reserved;
	uint32_t	algorithm;
	uint8_t		magic[4];
	uint32_t	bit_length;
	uint8_t		modulus[128];
} __attribute__ ((packed)) fwts_acpi_table_slic_key;

typedef struct {
	fwts_acpi_table_slic_header header;
	uint32_t	version;
	uint8_t		oem_id[6];
	uint8_t		oem_table_id[8];
	uint8_t		windows_flag[8];
	uint32_t	slic_version;
	uint8_t		reserved[16];
	uint8_t		signature[128];
} __attribute__ ((packed)) fwts_acpi_table_slic_marker;

typedef struct {
	fwts_acpi_table_header header;
	uint8_t		uuid[16];
	uint8_t		data[0];
} __attribute__ ((packed)) fwts_acpi_table_uefi;

/* 5.2.22 Boot Graphics Resource Table (BGRT) ACPI 5.0 Spec */
typedef struct {
	fwts_acpi_table_header header;
	uint16_t	version;
	uint8_t		status;
	uint8_t		image_type;
	uint64_t	image_addr;
	uint32_t	image_offset_x;
	uint32_t	image_offset_y;
} __attribute__ ((packed)) fwts_acpi_table_bgrt;

/* 5.2.23 Firmware Performance Data Table (FPDT) ACPI 5.0 spec */
typedef struct {
	uint16_t	type;
	uint8_t		length;
	uint8_t		revision;
} __attribute__ ((packed)) fwts_acpi_table_fpdt_header;

/* 5.2.23.4 S3 Performance Table Pointer Record */
typedef struct {
	fwts_acpi_table_fpdt_header	fpdt;
	uint32_t	reserved;
	uint64_t	s3pt_addr;
} __attribute__ ((packed)) fwts_acpi_table_fpdt_s3_perf_ptr;

/* 5.2.23.5 Firmware Basic Boot Performance Pointer Record */
typedef struct {
	fwts_acpi_table_fpdt_header	fpdt;
	uint32_t	reserved;
	uint64_t	fbpt_addr;
} __attribute__ ((packed)) fwts_acpi_table_fpdt_basic_boot_perf_ptr;

/* 5.2.24 Generic Timer Description Table (GTDT) ACPI 5.0 Spec */
typedef struct {
	fwts_acpi_table_header header;
	uint64_t	cnt_control_base_phys_addr;
	uint32_t	reserved;
	uint32_t	secure_EL1_timer_GSIV;
	uint32_t	secure_EL1_timer_flags;
	uint32_t	non_secure_EL1_timer_GSIV;
	uint32_t	non_secure_EL1_timer_flags;
	uint32_t	virtual_timer_GSIV;
	uint32_t	virtual_timer_flags;
	uint32_t	non_secure_EL2_timer_GSIV;
	uint32_t	non_secure_EL2_timer_flags;
	uint64_t	cnt_read_base_phys_addr;
	uint32_t	platform_timer_count;
	uint32_t	platform_timer_offset;
} __attribute__ ((packed)) fwts_acpi_table_gtdt;

/* 5.2.24 Generic Timer Description Table (GTDT) ACPI 5.0 Spec, table 5-117 */
typedef struct {
	uint32_t	timer_flags;
} __attribute__ ((packed)) fwts_acpi_table_gtdt_platform_timer;

typedef struct {
	uint8_t		frame_number;		/* 0..7 */
	uint8_t		reserved[3];		/* 0 */
	uint64_t	cntbase;
	uint64_t	cntel0base;
	uint32_t	gsiv;
	uint32_t	phys_timer_gsiv;
	uint32_t	phys_timer_flags;
	uint32_t	virt_timer_gsiv;
	uint32_t	virt_timer_flags;
	uint32_t	common_flags;
} __attribute__ ((packed)) fwts_acpi_table_gtdt_block_timer;

/* 5.2.24.1.1 GT Block Structure */
typedef struct {
	uint8_t		type;			/* 0x00 */
	uint16_t	length;			/* 20 + n * 40 */
	uint8_t		reserved;		/* 0x00 */
	uint64_t	physical_address;
	uint32_t	block_timer_count;	/* 0..8 */
	uint32_t	block_timer_offset;
	fwts_acpi_table_gtdt_block_timer	block_timers[0];
} __attribute__ ((packed)) fwts_acpi_table_gtdt_block;

/* 5.2.24.0.1 SBSA Generic Watchdog Structure */
typedef struct {
	uint8_t		type;			/* 0x01 */
	uint16_t	length;			/* 28 */
	uint8_t		reserved;		/* 0x00 */
	uint64_t	refresh_frame_addr;
	uint64_t	watchdog_control_frame_addr;
	uint32_t	watchdog_timer_gsiv;
	uint32_t	watchdog_timer_flags;
} __attribute__ ((packed)) fwts_acpi_table_gtdt_watchdog;

/* 5.2.20 ACPI RAS FeatureTable (RASF) */
typedef struct {
        uint8_t         platform_cc_id[12];
} __attribute__ ((packed)) fwts_acpi_table_rasf;

/* Section 14.1, Platform Communications Channel Table */
typedef struct {
	fwts_acpi_table_header	header;	
	uint32_t	flags;
	uint8_t		reserved[8];
} __attribute__ ((packed)) fwts_acpi_table_pcct;

typedef struct {
	uint8_t		type;
	uint8_t		length;
} __attribute__ ((packed)) fwts_acpi_table_pcct_subspace_header;

typedef struct {
	fwts_acpi_table_pcct_subspace_header	header;
	uint8_t		reserved[6];
	uint64_t	base_address;
	uint64_t	length;
	fwts_acpi_gas	doorbell_register;
	uint64_t	doorbell_preserve;
	uint64_t	doorbell_write;
	uint32_t	nominal_latency;
	uint32_t	max_periodic_access_rate;
	uint16_t	min_request_turnaround_time;
} __attribute__ ((packed)) fwts_acpi_table_pcct_subspace_type_0;

typedef struct {
	fwts_acpi_table_header	header;
	uint8_t		interface_type;
	uint8_t		reserved1[3];
	fwts_acpi_gas	base_address;
	uint8_t		interrupt_type;
	uint8_t		irq;
	uint32_t	gsi;
	uint8_t		baud_rate;
	uint8_t		parity;
	uint8_t		stop_bits;
	uint8_t		flow_control;
	uint8_t		terminal_type;
	uint8_t		reserved2;
	uint16_t	pci_device_id;
	uint16_t	pci_vendor_id;
	uint8_t		pci_bus_number;
	uint8_t		pci_device_number;
	uint8_t		pci_function_number;
	uint32_t	pci_flags;
	uint8_t		pci_segment;
	uint32_t	reserved3;
} __attribute__ ((packed)) fwts_acpi_table_spcr;

typedef struct {
	fwts_acpi_table_header	header;
	uint8_t		interface_type;
	uint8_t		reserved1[3];
	fwts_acpi_gas	base_address;
} __attribute__ ((packed)) fwts_acpi_table_dbgp;

typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	info_offset;
	uint32_t	info_count;
} __attribute__ ((packed)) fwts_acpi_table_dbg2;

typedef struct {
	uint8_t		revision;
	uint16_t	length;
	uint8_t		number_of_regs;
	uint16_t	namespace_length;
	uint16_t	namespace_offset;
	uint16_t	oem_data_length;
	uint16_t	oem_data_offset;
	uint16_t	port_type;
	uint16_t	port_subtype;
	uint16_t	reserved;
	uint16_t	base_address_offset;
	uint16_t	address_size_offset;
} __attribute__ ((packed)) fwts_acpi_table_dbg2_info;

/*
 *  http://www.dmtf.org/standards/published_documents/DSP0256_1.0.0.pdf
 */
typedef struct {
	fwts_acpi_table_header	header;
	uint8_t		interface_type;
	uint8_t		protocol_identifier;
	uint64_t	protocol_data;
	uint8_t		interrupt_type;
	uint8_t		gpe;
	uint8_t		pci_device_flag;
	uint32_t	global_system_interrupt;
	fwts_acpi_gas	base_address;
	uint8_t		pci_segment_group_number;
	uint8_t		pci_bus_number;
	uint8_t		pci_device_number;
	uint8_t		pci_function_number;
} __attribute__ ((packed)) fwts_acpi_table_mchi;

/*
 *   http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-intelligent-platform-mgt-interface-spec-2nd-gen-v2-0-spec-update.html
 *	page 600-606
 */
typedef struct {
	fwts_acpi_table_header	header;
	uint8_t		interface_type;
	uint8_t		reserved1;
	uint16_t	specification_revision;
	uint8_t		interrupt_type;
	uint8_t		gpe;
	uint8_t		reserved2;
	uint8_t		pci_device_flag;
	uint32_t	global_system_interrupt;
	fwts_acpi_gas	base_address;
	uint8_t		pci_segment_group_number;
	uint8_t		pci_bus_number;
	uint8_t		pci_device_number;
	uint8_t		pci_function_number;
	uint8_t		reserved3;
} __attribute__ ((packed)) fwts_acpi_table_spmi;

/*
 *  Hardware Error Source Table (HEST), ACPI section 18.3.2
 */
typedef struct {
	fwts_acpi_table_header	header;
	uint32_t	error_source_count;
} __attribute__ ((packed)) fwts_acpi_table_hest;

/* Section 18.3.2.7, table 18-332 */
typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint16_t	configuration_write_enable;
	uint32_t	poll_interval;
	uint32_t	vector;
	uint32_t	switch_to_polling_threshold_value;
	uint32_t	switch_to_polling_threshold_window;
	uint32_t	error_threshold_value;
	uint32_t	error_threshold_window;
} __attribute__ ((packed)) fwts_acpi_table_hest_hardware_error_notification;

/* Section 18.3.2.1.1 IA-32 Architecture Machine Check Bank Structure */
typedef struct {
	uint8_t		bank_number;
	uint8_t		clear_status_on_initialization;
	uint8_t		status_data_format;
	uint8_t		reserved;
	uint32_t	control_register_msr_address;
	uint64_t	control_init_data;
	uint32_t	status_register_msr_address;
	uint32_t	address_register_msr_address;
	uint32_t	misc_register_msr_address;
} __attribute__ ((packed)) fwts_acpi_table_hest_machine_check_bank;

/* Section 18.3.2.1 Table 18-322, Type 0x00 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	reserved1;
	uint8_t		flags;
	uint8_t		enabled;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	uint64_t	global_capability_init_data;
	uint64_t	global_control_init_data;
	uint8_t		number_of_hardware_banks;
	uint8_t		reserved2[7];
	fwts_acpi_table_hest_machine_check_bank	bank[0];
} __attribute__ ((packed)) fwts_acpi_table_hest_ia32_machine_check_exception;

/* Section 18.3.2.2 Table 18-324, Type 0x01 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	reserved1;
	uint8_t		flags;
	uint8_t		enabled;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	fwts_acpi_table_hest_hardware_error_notification notification;
	uint8_t		number_of_hardware_banks;
	uint8_t		reserved2[3];
	fwts_acpi_table_hest_machine_check_bank	bank[0];
} __attribute__ ((packed)) fwts_acpi_table_hest_ia32_corrected_machine_check;

/* Section 18.3.2.2.1, 18-325, Type 0x02 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	reserved1;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	uint32_t	max_raw_data_length;
} __attribute__ ((packed)) fwts_acpi_table_hest_nmi_error;

/* Section 18.3.2.3, Table 18-326, Type 0x06 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	reserved1;
	uint8_t		flags;
	uint8_t		enabled;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	uint32_t	bus;
	uint16_t	device;
	uint16_t	function;
	uint16_t	device_control;
	uint16_t	reserved2;
	uint32_t	uncorrectable_error_mask;
	uint32_t	uncorrectable_error_severity;
	uint32_t	correctable_error_mask;
	uint32_t	advanced_error_capabilities_and_control;
	uint32_t	root_error_command;
} __attribute__ ((packed)) fwts_acpi_table_hest_pci_express_root_port_aer;

/* Section 18.3.2.4, Table 18-327, Type 0x07 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	reserved1;
	uint8_t		flags;
	uint8_t		enabled;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	uint32_t	bus;
	uint16_t	device;
	uint16_t	function;
	uint16_t	device_control;
	uint16_t	reserved2;
	uint32_t	uncorrectable_error_mask;
	uint32_t	uncorrectable_error_severity;
	uint32_t	correctable_error_mask;
	uint32_t	advanced_error_capabilities_and_control;
} __attribute__ ((packed)) fwts_acpi_table_hest_pci_express_device_aer;

/* Section 18.3.2.5, Table 18-328, Type 0x08 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	reserved1;
	uint8_t		flags;
	uint8_t		enabled;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	uint32_t	bus;
	uint16_t	device;
	uint16_t	function;
	uint16_t	device_control;
	uint16_t	reserved2;
	uint32_t	uncorrectable_error_mask;
	uint32_t	uncorrectable_error_severity;
	uint32_t	correctable_error_mask;
	uint32_t	advanced_error_capabilities_and_control;
	uint32_t	secondary_uncorrectable_error_mask;
	uint32_t	secondary_uncorrectable_error_severity;
	uint32_t	secondary_advanced_error_capabilities_and_control;
} __attribute__ ((packed)) fwts_acpi_table_hest_pci_express_bridge_aer;

/* Section 18.3.2.6, Table 18-329, Type 0x09 */
typedef struct {
	uint16_t	type;
	uint16_t	source_id;
	uint16_t	related_source_id;
	uint8_t		flags;
	uint8_t		enabled;
	uint32_t	number_of_records_to_preallocate;
	uint32_t	max_sections_per_record;
	uint32_t	max_raw_data_length;
	fwts_acpi_gas	error_status_address;
	fwts_acpi_table_hest_hardware_error_notification notification;
	uint32_t	error_status_block_length;
} __attribute__ ((packed)) fwts_acpi_table_hest_generic_hardware_error_source;

void fwts_acpi_table_get_header(fwts_acpi_table_header *hdr, uint8_t *data);

#endif
