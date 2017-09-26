/*
 * Copyright (C) 2015-2017 Canonical
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
#include "fwts.h"

#if defined(FWTS_HAS_ACPI)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static fwts_acpi_table_info *table;

static int iort_init(fwts_framework *fw)
{

	if (fwts_acpi_find_table(fw, "IORT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot read ACPI tables.");
		return FWTS_ERROR;
	}
	if (table == NULL || (table && table->length == 0)) {
		fwts_log_error(fw, "ACPI IORT table does not exist, skipping test");
		return FWTS_SKIP;
	}

	return FWTS_OK;
}

/*
 *  iort_node_dump()
 *	dump IORT Node header, common to all IORT node types
 */
static void iort_node_dump(
	fwts_framework *fw,
	const char *node_name,
	fwts_acpi_table_iort_node *node)
{
	fwts_log_info_verbatim(fw, "%s:", node_name);
	fwts_log_info_verbatim(fw, "  Type:                     0x%2.2" PRIx8, node->type);
	fwts_log_info_verbatim(fw, "  Length:                   0x%4.4" PRIx16, node->length);
	fwts_log_info_verbatim(fw, "  Revision:                 0x%2.2" PRIx8, node->revision);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%8.8" PRIx32, node->reserved);
	fwts_log_info_verbatim(fw, "  Number of ID mappings:    0x%8.8" PRIx32, node->id_mappings_count);
	fwts_log_info_verbatim(fw, "  Reference to ID Array:    0x%8.8" PRIx32, node->id_array_offset);
}

/*
 *  iort_node_check()
 *	sanity check IORT Node header
 */
static void iort_node_check(
	fwts_framework *fw,
	uint8_t *data,
	bool no_id_mappings,
	bool no_id_array,
	bool *passed)
{
	fwts_acpi_table_iort_node *node = (fwts_acpi_table_iort_node *)data;

	if (node->revision != 0) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"IORTNodeRevisionNonZero",
			"IORT Node Revision field is 0x%2.2" PRIx8
			" and should be zero.",
			node->revision);
	}

	fwts_acpi_reserved_zero_check(fw, "IORT", "Node Reserved", node->reserved, sizeof(node->reserved), passed);

	if (no_id_mappings && node->id_mappings_count) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"IORTNodeMappingsCountNonZero",
			"IORT Node Number of ID Mappings field is 0x0%8.8" PRIx32
			" and should be zero for this node type.",
			node->id_mappings_count);
	}
	if (no_id_array && node->id_array_offset) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"IORTNodeRefToIdArrayNonZero",
			"IORT Node Referenced to ID Array field is 0x0%8.8" PRIx32
			" and should be zero for this node type.",
			node->id_array_offset);
	}
}


/*
 *  IORT ID mappings
 */

/*
 *  iort_id_mapping_dump()
 *	dump IORT ID mapping
 */
static void iort_id_mapping_dump(
	fwts_framework *fw,
	uint32_t i,
	fwts_acpi_table_iort_id_mapping *id_mapping)
{
	fwts_log_info_verbatim(fw, "ID Mapping %" PRIu32, i);
	fwts_log_info_verbatim(fw, "  Input Base:               0x%8.8" PRIx32, id_mapping->input_base);
	fwts_log_info_verbatim(fw, "  ID Count:                 0x%8.8" PRIx32, id_mapping->id_count);
	fwts_log_info_verbatim(fw, "  Output Base:              0x%8.8" PRIx32, id_mapping->output_base);
	fwts_log_info_verbatim(fw, "  Output Reference:         0x%8.8" PRIx32, id_mapping->output_reference);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, id_mapping->flags);
}

/*
 *  iort_id_mappings_dump()
 *	dump array of ID mappings
 */
static void iort_id_mappings_dump(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *data_end)
{
	uint32_t i;
	fwts_acpi_table_iort_node *node = (fwts_acpi_table_iort_node *)data;
	fwts_acpi_table_iort_id_mapping *id_mapping;

	id_mapping = (fwts_acpi_table_iort_id_mapping *)(data + node->id_array_offset);

	for (i = 0; i < node->id_mappings_count; i++, id_mapping++) {
		if (sizeof(*id_mapping) + (uint8_t *)id_mapping > data_end)
			break;
		iort_id_mapping_dump(fw, i, id_mapping);
	}
}

/*
 *  iort_id_mappings_check()
 *	snaity check array of ID mappings
 */
static void iort_id_mappings_check(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *data_end,
	bool *passed)
{
	uint32_t i;
	fwts_acpi_table_iort_node *node = (fwts_acpi_table_iort_node *)data;
	fwts_acpi_table_iort_id_mapping *id_mapping;
	char field[80];

	id_mapping = (fwts_acpi_table_iort_id_mapping *)(data + node->id_array_offset);

	for (i = 0; i < node->id_mappings_count; i++, id_mapping++) {
		if (sizeof(*id_mapping) + (uint8_t *)id_mapping > data_end) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"IORTIdMappingOutsideTable",
				"IORT ID Mapping %" PRIu32 " is outside the "
				"IORT ACPI table. Either the offset is incorrect "
				"or the IORT table size or the node is too small.", i);
			break;
		}

		snprintf(field, sizeof(field), "ID Mapping %" PRIu32 " flags", i);
		fwts_acpi_reserved_bits_check(fw, "IORT", field, id_mapping->flags, sizeof(id_mapping->flags), 1, 31, passed);
	}
}

/*
 *  IORT SMMU interrupts
 */

/*
 *  iort_smmu_interrupt_dump()
 *	dump array of SMMU interrupts
 */
static void iort_smmu_interrupt_dump(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *data_end,
	uint32_t offset,
	uint32_t count)
{
	uint32_t i;

	fwts_acpi_table_iort_smmu_interrupt *intr =
		(fwts_acpi_table_iort_smmu_interrupt *)(data + offset);

	for (i = 0; i < count; i++, intr++) {
		if (sizeof(*intr) + (uint8_t *)intr > data_end)
			break;
		fwts_log_info_verbatim(fw, "  GSIV:                     0x%8.8" PRIx32, intr->gsiv);
		fwts_log_info_verbatim(fw, "  Interrupt Flags:          0x%8.8" PRIx32, intr->flags);

	}
}

/*
 *  iort_smmu_interrupt_flags_check()
 *	sanity check SMMU interrupt flag bits
 */
static void iort_smmu_interrupt_flags_check(
	fwts_framework *fw,
	char *name,
	uint32_t flags,
	bool *passed)
{
	fwts_acpi_reserved_bits_check(fw, "IORT", name, flags, sizeof(flags), 1, 31, passed);
}

/*
 *  iort_smmu_interrupt_check()
 *	sanity check array of SMMU interrupts
 */
static void iort_smmu_interrupt_check(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *data_end,
	uint32_t offset,
	uint32_t count,
	bool *passed)
{
	uint32_t i;

	fwts_acpi_table_iort_smmu_interrupt *intr =
		(fwts_acpi_table_iort_smmu_interrupt *)(data + offset);

	for (i = 0; i < count; i++, intr++) {
		if (sizeof(*intr) + (uint8_t *)intr > data_end) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"IORTSmmuInterruptsOutsideTable",
				"IORT SMMU Interrupt %" PRIu32 " is outside the "
				"IORT ACPI table. Either the offset is incorrect "
				"or the IORT table size is too small.", i);
			break;
		}
		iort_smmu_interrupt_flags_check(fw, "SMMU_GSV",
			intr->flags, passed);
	}
}

/*
 * IORT Global Interrupts
 */

/*
 *  iort_smmu_global_interrupt_dump()
 *	dump SMMU global interrupts
 */
static void iort_smmu_global_interrupt_dump(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *data_end,
	uint32_t offset)
{
	fwts_acpi_table_iort_smmu_global_interrupt_array *intr =
		(fwts_acpi_table_iort_smmu_global_interrupt_array *)(data + offset);

	if (sizeof(*intr) + (uint8_t*)intr <= data_end) {
		fwts_log_info_verbatim(fw, "  SMMU_NSgIrpt:             0x%8.8" PRIx32, intr->smmu_nsgirpt);
		fwts_log_info_verbatim(fw, "  SMMU_NSgIrpt Flags:       0x%8.8" PRIx32, intr->smmu_nsgirpt_flags);
		fwts_log_info_verbatim(fw, "  SMMU_NSgCfgIrpt:          0x%8.8" PRIx32, intr->smmu_nsgcfgirpt);
		fwts_log_info_verbatim(fw, "  SMMU_NSgCfgIrpt Flags:    0x%8.8" PRIx32, intr->smmu_nsgcfgirpt_flags);
	}
}

/*
 *  iort_smmu_global_interrupt_check()
 *	sanity check SMMU global interrupts
 */
static void iort_smmu_global_interrupt_check(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *data_end,
	uint32_t offset,
	bool *passed)
{
	fwts_acpi_table_iort_smmu_global_interrupt_array *intr =
		(fwts_acpi_table_iort_smmu_global_interrupt_array *)(data + offset);
	uint8_t *intr_end = data + offset +
			sizeof(fwts_acpi_table_iort_smmu_global_interrupt_array);

	if (sizeof(*intr) + (uint8_t *)intr_end > data_end) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTSmmuGlobalInterruptsOutsideTable",
			"IORT SMMU Global Interrupt is outside the "
			"IORT ACPI table. The offset is incorrect "
			"or the IORT table size is too small.");
		return;
	}
	iort_smmu_interrupt_flags_check(fw, "SMMU_NsgIrpt",
		intr->smmu_nsgirpt_flags, passed);
	iort_smmu_interrupt_flags_check(fw, "SMMU_NsgCfgIrpt",
		intr->smmu_nsgcfgirpt_flags, passed);
}

/*
 *  IORT Node checks
 */

/*
 *  Check IORT ITS Group Node
 */
static void iort_check_its_group(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *node_end,
	bool *passed)
{
	fwts_acpi_table_iort_its_group_node *node =
		(fwts_acpi_table_iort_its_group_node *)data;
	uint32_t *its_id = node->identifier_array;
	size_t its_id_array_size = node->its_count * sizeof(*its_id);

	iort_node_dump(fw, "IORT ITS Group Node", (fwts_acpi_table_iort_node *)data);
	fwts_log_info_verbatim(fw, "  Number of ITSs:           0x%8.8" PRIx32, node->its_count);


	/* Array too big? */
	if (its_id_array_size + (uint8_t*)its_id > node_end) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTItsIdArrayOutsideTable",
			"IORT ITS Identifier Array end is outside the "
			"IORT ACPI table. Either the Number of ID mappings is "
			"too large or the IORT table size or ITS Group Node is too small.");
	} else {
		uint32_t i;

		for (i = 0; i < node->its_count; i++, its_id++) {
			fwts_log_info_verbatim(fw, "  GIC ITS Identifier:       0x%8.8" PRIx32, *its_id);
		}
	}
	iort_node_check(fw, data, true, true, passed);
	fwts_log_nl(fw);
}

/*
 *  iort_memory_access_properties_check()
 *	sanity check memory access properties
 */
static void iort_memory_access_properties_check(
	fwts_framework *fw,
	const char *name,
	fwts_acpi_table_iort_properties *properties,
	bool *passed)
{
	uint8_t cca, cpm, dacs;
	char field[80];

	if (properties->cache_coherent > 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTCacheCoherentAttrInvalid",
			"IORT %s Coherent Attribute "
			"is 0x%" PRIx32 " and should be either "
			"0 (device is fully coherent) or 1 (device is "
			"not coherent).",
			name, properties->cache_coherent);
	}

	snprintf(field, sizeof(field), "%s Allocation Hints", name);
	fwts_acpi_reserved_bits_check(fw, "IORT", field, properties->allocation_hints,
		sizeof(properties->allocation_hints), 4, 7, passed);

	snprintf(field, sizeof(field), "%s Reserved", name);
	fwts_acpi_reserved_zero_check(fw, "IORT", field, properties->reserved,
		sizeof(properties->reserved), passed);

	snprintf(field, sizeof(field), "%s  Memory Access Flags", name);
	fwts_acpi_reserved_bits_check(fw, "IORT", field, properties->memory_access_flags,
		sizeof(properties->memory_access_flags), 2, 7, passed);

	cca = properties->cache_coherent & 1;
	cpm = properties->memory_access_flags & 1;
	dacs = (properties->memory_access_flags >> 1) & 1;

	if ((cca == 1) && (cpm == 0)) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTMemAttrInvalid",
			"IORT %s Memory Attributes are illegal, "
			"CCA cannot be 1 if CPM is 0.", name);
	}
	if ((cca == 0) && (cpm == 1) && (dacs == 1)) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTMemAttrInvalid",
			"IORT %s Memory Attributes are illegal, "
			"CCA cannot be 0 if CPM and DACS are 1. If the device "
			"has a coherent path to memory and natively outputs "
			"IWB-OWB-ISH then CCA must be 1.", name);
	}
}

/*
 *  Check IORT Named Component Node
 */
static void iort_check_named_component(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *node_end,
	bool *passed)
{
	fwts_acpi_table_iort_named_component_node *node =
		(fwts_acpi_table_iort_named_component_node *)data;
	uint8_t *obj_name;

	iort_node_dump(fw, "IORT Named Component Node", (fwts_acpi_table_iort_node *)data);
	fwts_log_info_verbatim(fw, "  Node Flags:               0x%8.8" PRIx32, node->flags);
	fwts_log_info_verbatim(fw, "  Cache Coherent Attribute: 0x%8.8" PRIx32, node->properties.cache_coherent);
	fwts_log_info_verbatim(fw, "  Allocation Hints:         0x%2.2" PRIx8, node->properties.allocation_hints);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, node->properties.reserved);
	fwts_log_info_verbatim(fw, "  Memory Access Flags       0x%2.2" PRIx8, node->properties.memory_access_flags);
	fwts_log_info_verbatim(fw, "  Device Memory Addr. Size: 0x%2.2" PRIx8, node->device_memory_address_size);

	/* Is object name sane, zero terminated and inside the table? */
	for (obj_name = node->device_object_name; *obj_name; obj_name++) {
		if (obj_name > node_end) {
			*passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"IORTBadNamedComponentDeviceObjectName",
				"IORT Named Component Device Object Name string "
				"does not contain a null byte string terminator "
				"inside the IORT ACPI table.  Either the null "
				"terminator is missing, or the IORT ACPI table "
				"is truncated.");
			/* Need to skip dumping it out as it is probably garbage */
			goto next;
		}
	}
	fwts_log_info_verbatim(fw, "  Device Object Name:       %s", (char *)node->device_object_name);

next:
	iort_id_mappings_dump(fw, data, node_end);
	iort_node_check(fw, data, false, false, passed);

	if (node->flags) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTNamedComponentNodeFlagsNonZero",
			"IORT Named Component Node Flags is 0x%" PRIx32 " and is "
			"reserved and should be zero.", node->flags);
	}

	iort_memory_access_properties_check(fw, "Named Component Node", &node->properties, passed);
	iort_id_mappings_check(fw, data, node_end, passed);
	fwts_log_nl(fw);
}

/*
 *  Check IORT PCI Root Complex Node
 */
static void iort_check_pci_root_complex(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *node_end,
	bool *passed)
{
	fwts_acpi_table_iort_pci_root_complex_node *node =
		(fwts_acpi_table_iort_pci_root_complex_node *)data;

	iort_node_dump(fw, "IORT PCI Root Complex Node", (fwts_acpi_table_iort_node *)data);
	fwts_log_info_verbatim(fw, "  Cache Coherent Attribute: 0x%8.8" PRIx32, node->properties.cache_coherent);
	fwts_log_info_verbatim(fw, "  Allocation Hints:         0x%2.2" PRIx8, node->properties.allocation_hints);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx16, node->properties.reserved);
	fwts_log_info_verbatim(fw, "  Memory Access Flags       0x%4.4" PRIx16, node->properties.memory_access_flags);
	fwts_log_info_verbatim(fw, "  ATS Attribute:            0x%8.8" PRIx32, node->ats_attribute);
	fwts_log_info_verbatim(fw, "  PCI Segment Number:       0x%8.8" PRIx32, node->pci_segment_number);

	iort_id_mappings_dump(fw, data, node_end);
	iort_node_check(fw, data, false, false, passed);

	if (node->ats_attribute > 1) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTCPCIRootComplexAtsAttrInvalid",
			"IORT PCI Root Complex Node ATS Attribute is 0x%" PRIx32
			" and was expecting either 0 (root complex supports ATS) "
			"or 1 (root complex does not support ATS).",
			node->ats_attribute);
	}

	iort_memory_access_properties_check(fw, "PCI Root Complex Node", &node->properties, passed);
	iort_id_mappings_check(fw, data, node_end, passed);
	fwts_log_nl(fw);
}

/*
 *  Check IORT SMMUv1, SMMUv2 node
 */
static void iort_check_smmu(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *node_end,
	bool *passed)
{
	fwts_acpi_table_iort_smmu_node *node =
		(fwts_acpi_table_iort_smmu_node *)data;

	iort_node_dump(fw, "IORT SMMU node", (fwts_acpi_table_iort_node *)data);
	fwts_log_info_verbatim(fw, "  Base Address:             0x%16.16" PRIx64, node->base_address);
	fwts_log_info_verbatim(fw, "  Span:                     0x%16.16" PRIx64, node->span);
	fwts_log_info_verbatim(fw, "  Model:                    0x%8.8" PRIx32, node->model);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, node->flags);
	fwts_log_info_verbatim(fw, "  Global Intr. Offset:      0x%8.8" PRIx32, node->global_interrupt_array_offset);
	fwts_log_info_verbatim(fw, "  Number of Context Intr.:  0x%8.8" PRIx32, node->context_interrupt_count);
	fwts_log_info_verbatim(fw, "  Context Intr. Offset:     0x%8.8" PRIx32, node->context_interrupt_array_offset);
	fwts_log_info_verbatim(fw, "  Number of PMU Intr.:      0x%8.8" PRIx32, node->pmu_interrupt_count);
	fwts_log_info_verbatim(fw, "  PMU Intr. Offset:         0x%8.8" PRIx32, node->pmu_interrupt_array_offset);

	fwts_log_info_verbatim(fw, "Global Interrupt Array:");
	iort_smmu_global_interrupt_dump(fw, data, node_end,
		node->global_interrupt_array_offset);
	fwts_log_info_verbatim(fw, "Context Interrupt Array:");
	iort_smmu_interrupt_dump(fw, data, node_end,
		node->context_interrupt_array_offset, node->context_interrupt_count);
	fwts_log_info_verbatim(fw, "PMU Interrupt Array:");
	iort_smmu_interrupt_dump(fw, data, node_end,
		node->pmu_interrupt_array_offset, node->pmu_interrupt_count);
	iort_id_mappings_dump(fw, data, node_end);

	iort_node_check(fw, data, false, false, passed);
	iort_smmu_global_interrupt_check(fw, data, node_end,
		node->global_interrupt_array_offset, passed);
	iort_smmu_interrupt_check(fw, data, node_end,
		node->context_interrupt_array_offset, node->context_interrupt_count, passed);
	iort_smmu_interrupt_check(fw, data, node_end,
		node->pmu_interrupt_array_offset, node->pmu_interrupt_count, passed);
	iort_id_mappings_check(fw, data, node_end, passed);

	if (node->model > 3) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTSmmuInvalidModel",
			"IORT SMMU Model is 0x%" PRIx32 " and was expecting "
			"a model value 0 to 3.", node->model);
	}

	fwts_acpi_reserved_bits_check(fw, "IORT", "SMMU Reserved Flags", node->flags, sizeof(node->flags), 2, 31, passed);
	fwts_log_nl(fw);
}

/*
 *  Check IORT SMMUv3
 */
static void iort_check_smmuv3(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *node_end,
	bool *passed)
{
	fwts_acpi_table_iort_smmuv3_node *node =
		(fwts_acpi_table_iort_smmuv3_node *)data;

	iort_node_dump(fw, "IORT SMMUv3 node", (fwts_acpi_table_iort_node *)data);
	fwts_log_info_verbatim(fw, "  Base Address:             0x%16.16" PRIx64, node->base_address);
	fwts_log_info_verbatim(fw, "  Flags:                    0x%8.8" PRIx32, node->flags);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%8.8" PRIx32, node->reserved);
	fwts_log_info_verbatim(fw, "  VATOS Address:            0x%16.16" PRIx64, node->vatos_address);
	fwts_log_info_verbatim(fw, "  Model:                    0x%8.8" PRIx32, node->model);
	fwts_log_info_verbatim(fw, "  Event:                    0x%8.8" PRIx32, node->event);
	fwts_log_info_verbatim(fw, "  PRI:                      0x%8.8" PRIx32, node->pri);
	fwts_log_info_verbatim(fw, "  GERR:                     0x%8.8" PRIx32, node->gerr);
	fwts_log_info_verbatim(fw, "  Sync:                     0x%8.8" PRIx32, node->sync);

	iort_id_mappings_dump(fw, data, node_end);

	iort_node_check(fw, data, false, false, passed);
	iort_id_mappings_check(fw, data, node_end, passed);

	if (node->model > 0) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTSmmuv3InvalidModel",
			"IORT SMMUv3 Model is 0x%" PRIx32 " and was expecting "
			"a model value of 0.", node->model);
	}

	fwts_acpi_reserved_bits_check(fw, "IORT", "SMMUv3 Reserved Flags", node->flags, sizeof(node->flags), 2, 31, passed);
	fwts_log_nl(fw);
}

/*
 *  Check IORT Performance Monitoring Counter Group (PMCG)
 */
static void iort_check_pmcg(
	fwts_framework *fw,
	uint8_t *data,
	uint8_t *node_end,
	bool *passed)
{
	fwts_acpi_table_iort_pmcg_node *node =
		(fwts_acpi_table_iort_pmcg_node *)data;

	iort_node_dump(fw, "IORT PMCG node", (fwts_acpi_table_iort_node *)data);
	fwts_log_info_verbatim(fw, "  Base Address:             0x%16.16" PRIx64, node->base_address);
	fwts_log_info_verbatim(fw, "  Overflow interrupt GSIV:  0x%8.8" PRIx32, node->gsiv);
	fwts_log_info_verbatim(fw, "  Node reference:           0x%8.8" PRIx32, node->node_ref);

	iort_id_mappings_dump(fw, data, node_end);

	iort_node_check(fw, data, false, false, passed);
	iort_id_mappings_check(fw, data, node_end, passed);

	fwts_log_nl(fw);
}

/*
 *  IORT Remapping Table
 *     http://infocenter.arm.com/help/topic/com.arm.doc.den0049a/DEN0049A_IO_Remapping_Table.pdf
 */
static int iort_test1(fwts_framework *fw)
{
	bool passed = true;
	fwts_acpi_table_iort *iort = (fwts_acpi_table_iort *)table->data;
        uint8_t *data = (uint8_t *)table->data;
	uint8_t *data_end;
	uint32_t count;

	if (table->length < (size_t)iort->header.length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTTooShort",
			"IORT table incorrectly sized, IORT "
			"header reports it is %" PRIu32 " bytes, "
			"instead got %zu bytes",
			iort->header.length, table->length);
		goto done;
	}

	if (table->length < sizeof(fwts_acpi_table_iort)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"IORTTooShort",
			"IORT table too short, expecting %zu bytes, "
			"instead got %zu bytes",
			sizeof(fwts_acpi_table_iort), table->length);
		goto done;
	}

	data_end = data + table->length;

	fwts_log_info_verbatim(fw, "IORT IO Remapping Table test");
	fwts_log_info_verbatim(fw, "  Number of IORT Nodes:     0x%4.4" PRIx8, iort->io_rt_nodes_count);
	fwts_log_info_verbatim(fw, "  IORT Node Array Offset:   0x%4.4" PRIx8, iort->io_rt_offset);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%4.4" PRIx8, iort->reserved);
	fwts_log_nl(fw);

	data = (uint8_t *)table->data + iort->io_rt_offset;

	for (count = 0; count < iort->io_rt_nodes_count; count++) {
		fwts_acpi_table_iort_node *node = (fwts_acpi_table_iort_node *)data;
		uint8_t *node_end;

		if (data + node->length > data_end) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"IORTTooShort",
				"IORT table too short, parsing IORT node %" PRIu32
				" and the end of this node falls outside "
				"the IORT table.", count);
			break;
		}
		node_end = data + node->length;

		switch (node->type) {
		case 0x00:
			iort_check_its_group(fw, data, node_end, &passed);
			break;
		case 0x01:
			iort_check_named_component(fw, data, node_end, &passed);
			break;
		case 0x02:
			iort_check_pci_root_complex(fw, data, node_end, &passed);
			break;
		case 0x03:
			iort_check_smmu(fw, data, node_end, &passed);
			break;
		case 0x04:
			iort_check_smmuv3(fw, data, node_end, &passed);
			break;
		case 0x05:
			iort_check_pmcg(fw, data, node_end, &passed);
			break;
		default:
			/* reserved */
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"IORTInvalidType",
				"IORT Type 0x%2.2" PRIx8 " is invalid, aborting check",
				node->type);
			data = data_end;	/* Forces termination */
			break;
		}
		data += node->length;
	}
	if (passed)
		fwts_passed(fw, "No issues found in IORT.");

done:
	return FWTS_OK;
}

static fwts_framework_minor_test iort_tests[] = {
	{ iort_test1, "IORT IO Remapping Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops iort_ops = {
	.description = "IORT IO Remapping Table test.",
	.init        = iort_init,
	.minor_tests = iort_tests
};

FWTS_REGISTER("iort", &iort_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
