/*
 * Copyright (C) 2022-2026 Canonical
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
 */
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

static fwts_acpi_table_info *table;
acpi_table_init(AEST, &table)

static void aest_processor_test(
	fwts_framework *fw,
	fwts_acpi_table_aest_processor *entry,
	uint32_t *offset,
	bool *passed)
{
	fwts_log_info_verbatim(fw, "    Node-specific data (Processor):");
	fwts_log_info_simp_int(fw, "    ACPI Processor ID:              ", entry->acpi_processor_id);
	fwts_log_info_simp_int(fw, "    Resource Type:                  ", entry->resource_type);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved);
	fwts_log_info_simp_int(fw, "    Flags:                          ", entry->flags);
	fwts_log_info_simp_int(fw, "    Revision:                       ", entry->revision);
	fwts_log_info_simp_int(fw, "    Processor affinity lvl ind:     ", entry->processor_affinity_level_indicator);

	fwts_acpi_reserved_zero("AEST", "Reserved", entry->reserved, passed);
	fwts_acpi_reserved_bits("AEST", "Flags", entry->flags, 2, 7, passed);

	*offset += sizeof(fwts_acpi_table_aest_processor);
	if (entry->resource_type == FWTS_AEST_RESOURCE_CACHE) {
		fwts_acpi_table_aest_cache_resource_substructure *cache =
			(fwts_acpi_table_aest_cache_resource_substructure *)
			((uint8_t *)entry + sizeof(fwts_acpi_table_aest_processor));
		fwts_log_info_simp_int(fw, "    Cache Reference:                ", cache->cache_reference);
		fwts_log_info_simp_int(fw, "    Reserved:                       ", cache->reserved);
		fwts_acpi_reserved_zero("AEST", "Reserved", cache->reserved, passed);
		*offset += sizeof(fwts_acpi_table_aest_cache_resource_substructure);
	} else if (entry->resource_type == FWTS_AEST_RESOURCE_TLB) {
		fwts_acpi_table_aest_tlb_resource_substructure *tlb =
			(fwts_acpi_table_aest_tlb_resource_substructure *)
			((uint8_t *)entry + sizeof(fwts_acpi_table_aest_processor));
		fwts_log_info_simp_int(fw, "    TLB Reference:                  ", tlb->tlb_reference);
		fwts_log_info_simp_int(fw, "    Reserved:                       ", tlb->reserved);
		fwts_acpi_reserved_zero("AEST", "Reserved", tlb->reserved, passed);
		*offset += sizeof(fwts_acpi_table_aest_tlb_resource_substructure);
	} else if (entry->resource_type == FWTS_AEST_RESOURCE_GENERIC) {
		fwts_acpi_table_aest_generic_resource_substructure *generic =
			(fwts_acpi_table_aest_generic_resource_substructure *)
			((uint8_t *)entry + sizeof(fwts_acpi_table_aest_processor));
		fwts_log_info_simp_int(fw, "    Data:                               ", generic->data);
		*offset += sizeof(fwts_acpi_table_aest_generic_resource_substructure);
	} else
		fwts_acpi_reserved_type(fw, "AEST", entry->resource_type, 0, FWTS_AEST_RESOURCE_RESERVED, passed);

}

static void aest_interface_test(
	fwts_framework *fw,
	uint8_t *interface_data,
	bool *passed)
{
	fwts_acpi_table_aest_interface *entry = (fwts_acpi_table_aest_interface *)interface_data;
	uint32_t gf;  /* Group format multiplier */
	size_t offset;

	fwts_log_info_verbatim(fw, "    Interface:");
	fwts_log_info_simp_int(fw, "    Interface Type:                 ", entry->interface_type);
	fwts_log_info_simp_int(fw, "    Group Format:                   ", entry->group_format);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved);
	fwts_log_info_simp_int(fw, "    Flags:                          ", entry->flags);
	fwts_log_info_simp_int(fw, "    Base Address:                   ", entry->base_address);
	fwts_log_info_simp_int(fw, "    Start Error Record Index:       ", entry->start_error_record_index);
	fwts_log_info_simp_int(fw, "    Number Of Error Records:        ", entry->number_of_error_records);

	/* Calculate gf based on group_format */
	switch (entry->group_format) {
	case 0:  /* 4KiB */
		gf = 1;
		break;
	case 1:  /* 16KiB */
		gf = 4;
		break;
	case 2:  /* 64KiB */
		gf = 14;
		break;
	default:
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"AESTInvalidGroupFormat",
			"AEST Interface Group Format must be 0-2, got 0x%2.2x instead",
			entry->group_format);
		*passed = false;
		/* Cannot continue parsing - offsets are unknown */
		goto validate_fixed_fields;
	}

	/* Read variable-size fields based on gf */
	offset = sizeof(fwts_acpi_table_aest_interface);  /* After fixed fields */

	/* Error record implemented: 8*gf bytes at offset 24 */
	fwts_log_info_verbatim(fw, "    Error Record Implemented:");
	fwts_hexdump_data_prefix_all(fw, interface_data + offset, "      ", 8 * gf);
	offset += 8 * gf;

	/* Status reporting supported: 8*gf bytes */
	fwts_log_info_verbatim(fw, "    Status Reporting Supported:");
	fwts_hexdump_data_prefix_all(fw, interface_data + offset, "      ", 8 * gf);
	offset += 8 * gf;

	/* Addressing mode: 8*gf bytes */
	fwts_log_info_verbatim(fw, "    Addressing Mode:");
	fwts_hexdump_data_prefix_all(fw, interface_data + offset, "      ", 8 * gf);
	offset += 8 * gf;

	/* V2 additional fields at variable offsets */
	/* Offset is now at sizeof(interface) + 24*gf after the three variable fields */
	uint32_t *acpi_error_node_device = (uint32_t *)(interface_data + offset);
	offset += sizeof(uint32_t);
	uint32_t *processor_affinity = (uint32_t *)(interface_data + offset);
	offset += sizeof(uint32_t);
	uint64_t *error_group_register_base = (uint64_t *)(interface_data + offset);
	offset += sizeof(uint64_t);
	uint64_t *fault_injection_register_base = (uint64_t *)(interface_data + offset);
	offset += sizeof(uint64_t);
	uint64_t *interrupt_config_register_base = (uint64_t *)(interface_data + offset);

	fwts_log_info_simp_int(fw, "    ACPI Error Node Device:         ", *acpi_error_node_device);
	fwts_log_info_simp_int(fw, "    Processor Affinity:             ", *processor_affinity);
	fwts_log_info_simp_int(fw, "    Error Group Register Base:      ", *error_group_register_base);
	fwts_log_info_simp_int(fw, "    Fault Injection Register Base:  ", *fault_injection_register_base);
	fwts_log_info_simp_int(fw, "    Interrupt Config Register Base: ", *interrupt_config_register_base);

validate_fixed_fields:
	fwts_acpi_reserved_zero("AEST", "Reserved", entry->reserved, passed);
	fwts_acpi_reserved_bits("AEST", "Flags", entry->flags, 2, 31, passed);
	fwts_acpi_reserved_type(fw, "AEST", entry->interface_type, 0, 2, passed);
}

static void aest_interrupt_test(
	fwts_framework *fw,
	fwts_acpi_table_aest_interrupt *entry,
	bool *passed)
{
	fwts_log_info_simp_int(fw, "    Interrupt Type:                 ", entry->interrupt_type);
	fwts_log_info_simp_int(fw, "    Reserved:                       ", entry->reserved);
	fwts_log_info_simp_int(fw, "    Interrupt Flags:                ", entry->interrupt_flags);
	fwts_log_info_simp_int(fw, "    Interrupt GSIV:                 ", entry->interrupt_gsiv);
	fwts_log_info_simp_int(fw, "    Reserved:                      ", entry->reserved1);

	fwts_acpi_reserved_zero("AEST", "Reserved", entry->reserved, passed);
	fwts_acpi_reserved_bits("AEST", "Interrupt Type", entry->interrupt_type, 2, 7, passed);
	fwts_acpi_reserved_bits("AEST", "Interrupt Flags", entry->interrupt_flags, 2, 31, passed);
	fwts_acpi_reserved_zero("AEST", "Reserved", entry->reserved1, passed);
	fwts_acpi_reserved_type(fw, "AEST", entry->interrupt_type, 0, 2, passed);
}

static int aest_test1(fwts_framework *fw)
{
	fwts_acpi_table_aest_node *node;
	bool passed = true;
	uint32_t offset;


	fwts_log_info_verbatim(fw, "AEST Arm Error Source Table:");

	node = (fwts_acpi_table_aest_node *)(table->data + sizeof(fwts_acpi_table_aest));
	offset = sizeof(fwts_acpi_table_aest);
	while (offset < table->length) {
		fwts_acpi_table_aest_processor *processor = NULL;
		fwts_acpi_table_aest_memory_controller *mem_controller = NULL;
		fwts_acpi_table_aest_smmu *smmu = NULL;
		fwts_acpi_table_aest_vendor_defined *vendor_defined = NULL;
		fwts_acpi_table_aest_gic *gic = NULL;
		fwts_acpi_table_aest_pcie_root_complex *pcie = NULL;
		fwts_acpi_table_aest_proxy *proxy = NULL;

		if (fwts_acpi_structure_length_zero(fw, "AEST", node->length, offset)) {
			passed = false;
			break;
		}

		fwts_log_info_verbatim(fw, "  AEST node structure:");
		fwts_log_info_simp_int(fw, "    Type:                           ", node->type);
		fwts_log_info_simp_int(fw, "    Length:                         ", node->length);
		fwts_log_info_simp_int(fw, "    Reserved:                       ", node->reserved);
		fwts_log_info_simp_int(fw, "    Offset to Node-specific data:   ", node->offset_node_specific_data);
		fwts_log_info_simp_int(fw, "    Offset to Node Interface:       ", node->offset_node_interface);
		fwts_log_info_simp_int(fw, "    Offset to Node Interrupt Array: ", node->offset_node_interrupt_array);
		fwts_log_info_simp_int(fw, "    Node Interrupt Array Size:      ", node->node_interrupt_size);
		fwts_log_info_simp_int(fw, "    Timestamp Rate:                 ", node->timestamp_rate);
		fwts_log_info_simp_int(fw, "    Timebase Offset:                ", node->timebase_offset);
		fwts_log_info_simp_int(fw, "    Error Injection Countdown Rate: ", node->error_injection_countdown_rate);

		fwts_acpi_reserved_zero("AEST", "Reserved", node->reserved, &passed);

		offset += sizeof(fwts_acpi_table_aest_node);

		switch(node->type) {
			case FWTS_AEST_PROCESSOR:
				processor = (fwts_acpi_table_aest_processor *)(table->data + offset);
				aest_processor_test(fw, processor, &offset, &passed);
				break;
			case FWTS_AEST_MEMORY:
				mem_controller = (fwts_acpi_table_aest_memory_controller *)(table->data + offset);
				fwts_log_info_verbatim(fw, "    Node-specific data (Memory Controller):");
				fwts_log_info_simp_int(fw, "    Proximity Domain:               ",
					mem_controller->proximity_domain);
				offset += sizeof(fwts_acpi_table_aest_memory_controller);
				break;
			case FWTS_AEST_SMMU:
				smmu = (fwts_acpi_table_aest_smmu *)(table->data + offset);
				fwts_log_info_verbatim(fw, "    Node-specific data (SMMU):");
				fwts_log_info_simp_int(fw, "    IORT Node Reference:            ",
					smmu->iort_node_reference);
				fwts_log_info_simp_int(fw, "    SMMU-specific Data Subcomponent Reference: ",
					smmu->smmu_specific_data_subcomponent_reference);
				offset += sizeof(fwts_acpi_table_aest_smmu);
				break;
			case FWTS_AEST_VENDOR_DEFINED:
				vendor_defined = (fwts_acpi_table_aest_vendor_defined *)(table->data + offset);
				fwts_log_info_verbatim(fw, "    Node-specific data (Vendor-defined):");
				fwts_log_info_simp_int(fw, "    Hardware ID:                    ",
					vendor_defined->hardware_id);
				fwts_log_info_simp_int(fw, "    Unique ID:                      ",
					vendor_defined->unique_id);
				fwts_log_info_verbatim(fw, "    Vendor-specific data:");
				fwts_hexdump_data_prefix_all(fw, vendor_defined->vendor_specific_data,
					"      ", sizeof(vendor_defined->vendor_specific_data));
				offset += sizeof(fwts_acpi_table_aest_vendor_defined);
				break;
			case FWTS_AEST_GIC:
				gic = (fwts_acpi_table_aest_gic *)(table->data + offset);
				fwts_log_info_verbatim(fw, "    Node-specific data (GIC):");
				fwts_log_info_simp_int(fw, "    Interface Type:                 ",
					gic->interface_type);
				fwts_log_info_simp_int(fw, "    Instance Identifier:            ",
					gic->instance_identifier);
				fwts_acpi_reserved_type(fw, "AEST",
					gic->interface_type,
					0,
					FWTS_AEST_GIC_RESERVED, &passed);
				offset += sizeof(fwts_acpi_table_aest_gic);
				break;
			case FWTS_AEST_PCIE_ROOT_COMPLEX:
				pcie = (fwts_acpi_table_aest_pcie_root_complex *)(table->data + offset);
				fwts_log_info_verbatim(fw, "    Node-specific data (PCIe Root Complex):");
				fwts_log_info_simp_int(fw, "    IORT Node Reference:            ",
					pcie->iort_node_reference);
				offset += sizeof(fwts_acpi_table_aest_pcie_root_complex);
				break;
			case FWTS_AEST_PROXY:
				proxy = (fwts_acpi_table_aest_proxy *)(table->data + offset);
				fwts_log_info_verbatim(fw, "    Node-specific data (Proxy):");
				fwts_log_info_simp_int(fw, "    Node Address:                   ",
					proxy->node_address);
				offset += sizeof(fwts_acpi_table_aest_proxy);
				break;
			default:
				fwts_acpi_reserved_type(fw, "AEST", node->type, 0, FWTS_AEST_RESERVED, &passed);
				break;
		}

		if (node->offset_node_interface >= sizeof(fwts_acpi_table_aest_interface)) {
			uint8_t *interface_data = table->data + offset;
			fwts_acpi_table_aest_interface *interface = (fwts_acpi_table_aest_interface *)interface_data;
			uint32_t gf;
			size_t interface_size;

			/* Calculate gf and interface size based on group_format */
			switch (interface->group_format) {
			case 0:  /* 4KiB */
				gf = 1;
				break;
			case 1:  /* 16KiB */
				gf = 4;
				break;
			case 2:  /* 64KiB */
				gf = 14;
				break;
			default:
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"AESTInvalidGroupFormat",
					"AEST Interface Group Format must be 0-2, got 0x%2.2x instead",
					interface->group_format);
				passed = false;
				fwts_log_info(fw, "Skipping remaining AEST table parsing due to invalid group format.");
				return FWTS_OK;
			}

			/* Only parse if group format is valid */
			/* Total interface size: fixed + variable (24*gf) + V2 fields (32) */
			interface_size = sizeof(fwts_acpi_table_aest_interface) + (24 * gf) + 32;
			aest_interface_test(fw, interface_data, &passed);
			offset += interface_size;
		}

		uint32_t i;

		for (i = 0; i < node->node_interrupt_size; i++) {
			fwts_log_info_verbatim(fw, "    Interrupt Array");
			aest_interrupt_test(fw, (fwts_acpi_table_aest_interrupt *)(table->data + offset), &passed);
			offset += sizeof(fwts_acpi_table_aest_interrupt);
		}

		node = (fwts_acpi_table_aest_node *)(table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in AEST table.");

	return FWTS_OK;
}

static fwts_framework_minor_test aest_tests[] = {
	{ aest_test1, "Validate AEST table." },
	{ NULL, NULL }
};

static fwts_framework_ops aest_ops = {
	.description = "AEST Arm Error Source Table test.",
	.init        = AEST_init,
	.minor_tests = aest_tests
};

FWTS_REGISTER("aest", &aest_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
