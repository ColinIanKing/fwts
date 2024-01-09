/*
 * Copyright (C) 2024 Canonical
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
acpi_table_init(VIOT, &table)

static int viot_test1(fwts_framework *fw)
{
	fwts_acpi_viot_node_hdr *node_hdr;
	bool passed = true;
	uint32_t offset;
	fwts_acpi_table_viot *viot = (fwts_acpi_table_viot *)table->data;

	fwts_log_info_verbatim(fw, "VIOT Virtual I/O Translation Table:");
	fwts_log_info_simp_int(fw, "  Node Count:           ", viot->node_count);
	fwts_log_info_simp_int(fw, "  Node Offset:          ", viot->node_offset);

	if (viot->node_offset == 0) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"VIOTinvalidOffset",
			"VIOT Offset for the first node should not be 0.");
		return FWTS_OK;
	}

	fwts_log_info_simp_int(fw, "  Reserved:             ", viot->reserved);
	fwts_acpi_reserved_zero("VIOT", "Reserved", viot->reserved, &passed);

	offset = viot->node_offset;

	if (offset >= table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
		"VIOTOutOfRangeOffset",
		"VIOT Node Data Offset is out of range.");
		return FWTS_OK;
	}

	node_hdr = (fwts_acpi_viot_node_hdr *)(table->data + offset);

	for (uint32_t i = 0; i < viot->node_count; i++) {

		if (fwts_acpi_structure_length_zero(fw, "VIOT", node_hdr->length, offset)) {
			passed = false;
			break;
		}

		if ((offset + node_hdr->length) > table->length) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"VIOTOutOfRangeOffset",
			"VIOT Node Data Offset is out of range.");
			return FWTS_OK;
		}

		switch(node_hdr->type) {
			case 1:
				fwts_acpi_viot_pci_rng_node *pci_rng = (fwts_acpi_viot_pci_rng_node *)(table->data + offset);
				fwts_log_info_verbatim(fw, "  PCI Range Node Structure:");
				fwts_log_info_simp_int(fw, "    Type:               ", pci_rng->hdr.type);
				fwts_log_info_simp_int(fw, "    Reserved:           ", pci_rng->hdr.reserved);
				fwts_acpi_reserved_zero("VIOT", "Reserved", pci_rng->hdr.reserved, &passed);
				fwts_log_info_simp_int(fw, "    length:             ", pci_rng->hdr.length);
				fwts_log_info_simp_int(fw, "    Endpoint Start:     ", pci_rng->endpoint_start);
				fwts_log_info_simp_int(fw, "    PCI Segment Start:  ", pci_rng->pci_seg_start);
				fwts_log_info_simp_int(fw, "    PCI Segment End:    ", pci_rng->pci_seg_end);
				fwts_log_info_simp_int(fw, "    PCI BDF Start:      ", pci_rng->pci_bdf_start);
				fwts_log_info_simp_int(fw, "    PCI BDF End:        ", pci_rng->pci_bdf_end);
				fwts_log_info_simp_int(fw, "    PCI Segment End:    ", pci_rng->output_node);
				fwts_hexdump_data_prefix_all(fw, pci_rng->reserved, "      ", sizeof(pci_rng->reserved));
				fwts_acpi_reserved_zero_array(fw, "VIOT", "Reserved", pci_rng->reserved, 6, &passed);
				offset += sizeof(fwts_acpi_viot_pci_rng_node);
				break;
			case 2:
				fwts_acpi_viot_mmio_ep_node *mmio_ep = (fwts_acpi_viot_mmio_ep_node *)(table->data + offset);
				fwts_log_info_verbatim(fw, "  Single MMIO Endpoint Node Structure:");
				fwts_log_info_simp_int(fw, "    Type:               ", mmio_ep->hdr.type);
				fwts_log_info_simp_int(fw, "    Reserved:           ", mmio_ep->hdr.reserved);
				fwts_acpi_reserved_zero("VIOT", "Reserved", mmio_ep->hdr.reserved, &passed);
				fwts_log_info_simp_int(fw, "    length:             ", mmio_ep->hdr.length);
				fwts_log_info_simp_int(fw, "    Endpoint ID:        ", mmio_ep->endpoint_id);
				fwts_log_info_simp_int(fw, "    Base Address:       ", mmio_ep->base_addr);
				fwts_log_info_simp_int(fw, "    Output Node:        ", mmio_ep->output_node);
				fwts_hexdump_data_prefix_all(fw, mmio_ep->reserved, "      ", sizeof(mmio_ep->reserved));
				fwts_acpi_reserved_zero_array(fw, "VIOT", "Reserved", mmio_ep->reserved, 6, &passed);
				offset += sizeof(fwts_acpi_viot_mmio_ep_node);
				break;
			case 3:
				fwts_acpi_viot_pci_iommu_node *pci_iommu = (fwts_acpi_viot_pci_iommu_node *)(table->data + offset);
				fwts_log_info_verbatim(fw, "  Virtio-iommu based on virtio-pci Node Structure:");
				fwts_log_info_simp_int(fw, "    Type:               ", pci_iommu->hdr.type);
				fwts_log_info_simp_int(fw, "    Reserved:           ", pci_iommu->hdr.reserved);
				fwts_acpi_reserved_zero("VIOT", "Reserved", pci_iommu->hdr.reserved, &passed);
				fwts_log_info_simp_int(fw, "    length:             ", pci_iommu->hdr.length);
				fwts_log_info_simp_int(fw, "    PCI Segment:        ", pci_iommu->pci_seg);
				fwts_log_info_simp_int(fw, "    PCI BDF Number:     ", pci_iommu->pci_bdf_num);
				fwts_log_info_simp_int(fw, "    Reserved:           ", pci_iommu->reserved);
				fwts_acpi_reserved_zero("VIOT", "Reserved", pci_iommu->reserved, &passed);
				offset += sizeof(fwts_acpi_viot_pci_iommu_node);
				break;
			case 4:
				fwts_acpi_viot_mmio_iommu_node *mmio_iommu = (fwts_acpi_viot_mmio_iommu_node *)(table->data + offset);
				fwts_log_info_verbatim(fw, "  Virtio-iommu based on virtio-pci Node Structure:");
				fwts_log_info_simp_int(fw, "    Type:               ", mmio_iommu->hdr.type);
				fwts_log_info_simp_int(fw, "    Reserved:           ", mmio_iommu->hdr.reserved);
				fwts_acpi_reserved_zero("VIOT", "Reserved", mmio_iommu->hdr.reserved, &passed);
				fwts_log_info_simp_int(fw, "    length:             ", mmio_iommu->hdr.length);
				fwts_log_info_simp_int(fw, "    Reserved:           ", mmio_iommu->reserved);
				fwts_acpi_reserved_zero("VIOT", "Reserved", mmio_iommu->reserved, &passed);
				fwts_log_info_simp_int(fw, "    Base Address:       ", mmio_iommu->base_addr);
				offset += sizeof(fwts_acpi_viot_mmio_iommu_node);
				break;
			case 0:
			default:
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"VIOTBadNodeType",
					"VIOT node structure types must not have the value 0, 0x05..0xff, got "
					"0x%2.2" PRIx8 " instead", node_hdr->type);
				offset += node_hdr->length;
				break;
		}

		node_hdr = (fwts_acpi_viot_node_hdr *)(table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in VIOT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test viot_tests[] = {
	{ viot_test1, "Validate VIOT table." },
	{ NULL, NULL }
};

static fwts_framework_ops viot_ops = {
	.description = "VIOT Virtual I/O Translation Table test.",
	.init        = VIOT_init,
	.minor_tests = viot_tests
};

FWTS_REGISTER("viot", &viot_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
