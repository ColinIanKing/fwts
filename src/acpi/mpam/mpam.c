/*
 * Copyright (C) 2023 Canonical
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
acpi_table_init(MPAM, &table)

static int mpam_test1(fwts_framework *fw)
{
	fwts_acpi_mpam_msc_node *node;
	bool passed = true;
	uint32_t offset;

	fwts_log_info_verbatim(fw, "MPAM Memory System Resource Partitioning And Monitoring Table:");

	offset = sizeof(fwts_acpi_table_mpam);
	node = (fwts_acpi_mpam_msc_node *)(table->data + offset);
	while (offset < table->length) {

		if (fwts_acpi_structure_length_zero(fw, "MPAM", node->length, offset)) {
			passed = false;
			break;
		}

		fwts_log_info_verbatim(fw, "MPAM MSC node:");
		fwts_log_info_simp_int(fw, "  Length:                              ", node->length);
		fwts_log_info_simp_int(fw, "  Interface type:                      ", node->interface_type);
		fwts_log_info_simp_int(fw, "  Reserved:                            ", node->reserved);
		fwts_log_info_simp_int(fw, "  Identifier:                          ", node->identifier);
		fwts_log_info_simp_int(fw, "  Base address:                        ", node->base_address);
		fwts_log_info_simp_int(fw, "  MMIO size:                           ", node->mmio_size);
		fwts_log_info_simp_int(fw, "  Overflow interrupt:                  ", node->overflow_interrupt);
		fwts_log_info_simp_int(fw, "  Overflow interrupt flags:            ", node->overflow_interrupt_flags);
		fwts_log_info_simp_int(fw, "  Reserved1:                           ", node->reserved1);
		fwts_log_info_simp_int(fw, "  Overflow interrupt affinity:         ", node->overflow_interrupt_affinity);
		fwts_log_info_simp_int(fw, "  Error interrupt:                     ", node->error_interrupt);
		fwts_log_info_simp_int(fw, "  Error interrupt flags:               ", node->error_interrupt_flags);
		fwts_log_info_simp_int(fw, "  Reserved2:                           ", node->reserved2);
		fwts_log_info_simp_int(fw, "  Error interrupt affinity:            ", node->error_interrupt_affinity);
		fwts_log_info_simp_int(fw, "  MAX_NRDY_USEC:                       ", node->max_nrdy_usec);
		fwts_log_info_simp_int(fw, "  Hardware ID of linked device:        ", node->hardware_id_linked_device);
		fwts_log_info_simp_int(fw, "  Instance ID of linked device:        ", node->instance_id_linked_device);
		fwts_log_info_simp_int(fw, "  Number of resource nodes:            ", node->num_resouce_nodes);

		if (node->interface_type != 0 && node->interface_type != 0x0a) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"MPAMBadInterfaceType",
				"MPAM MSC node interface type must have value with 0 or 0x0a, got "
				"0x%2.2" PRIx8 " instead", node->interface_type);
		}

		fwts_acpi_reserved_zero("MPAM", "Reserved", node->reserved, &passed);
		fwts_acpi_reserved_bits("MPAM", "Overflow interrupt flags", node->overflow_interrupt_flags, 1, 2, &passed);
		fwts_acpi_reserved_bits("MPAM", "Overflow interrupt flags", node->overflow_interrupt_flags, 5, 31, &passed);
		fwts_acpi_reserved_zero("MPAM", "Reserved1", node->reserved1, &passed);
		fwts_acpi_reserved_bits("MPAM", "Error interrupt flags", node->error_interrupt_flags, 1, 2, &passed);
		fwts_acpi_reserved_bits("MPAM", "Error interrupt flags", node->error_interrupt_flags, 5, 31, &passed);
		fwts_acpi_reserved_zero("MPAM", "Reserved2", node->reserved2, &passed);

		uint32_t msc_offset = sizeof(fwts_acpi_mpam_msc_node);

		for (uint32_t i = 0; i < node->num_resouce_nodes; i++) {
			fwts_acpi_mpam_resource_node *res_node = (fwts_acpi_mpam_resource_node *)(table->data + offset + msc_offset);
			fwts_log_info_verbatim(fw, "  List of resource nodes: ");
			fwts_log_info_simp_int(fw, "    Identifier:                        ", res_node->identifier);
			fwts_log_info_simp_int(fw, "    RIS Index:                         ", res_node->ris_index);
			fwts_log_info_simp_int(fw, "    Reserved1:                         ", res_node->reserved1);
			fwts_log_info_simp_int(fw, "    Locator type:                      ", res_node->locator_type);
			fwts_log_info_verbatim(fw, "    Locator:");

			fwts_interconnect_locator_descriptor *intc_loc_des = NULL;
			fwts_interconnect_descriptor_table *intc_des_table = NULL;

			switch(res_node->locator_type) {
				case FWTS_MPAM_PROCESSOR_CACHE:
					fwts_log_info_verbatim(fw, "      Processor cache locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					fwts_acpi_reserved_zero_array(fw, "MPAM", "Locator", res_node->locator + 8, 4, &passed);
					break;
				case FWTS_MPAM_MEMORY:
					fwts_log_info_verbatim(fw, "      Memory locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					fwts_acpi_reserved_zero_array(fw, "MPAM", "Locator", res_node->locator + 8, 4, &passed);
					break;
				case FWTS_MPAM_SMMU:
					fwts_log_info_verbatim(fw, "      SMMU locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					fwts_acpi_reserved_zero_array(fw, "MPAM", "Locator", res_node->locator + 8, 4, &passed);
					break;
				case FWTS_MPAM_MEMORY_CACHE:
					fwts_log_info_verbatim(fw, "      Memory-side cache locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					fwts_acpi_reserved_zero_array(fw, "MPAM", "Locator", res_node->locator, 7, &passed);
					break;
				case FWTS_MPAM_ACPI_DEVICE:
					fwts_log_info_verbatim(fw, "      ACPI device locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					break;
				case FWTS_MPAM_INTERCONNECT:
					intc_loc_des = (fwts_interconnect_locator_descriptor *)res_node->locator;
					intc_des_table = (table->data + intc_loc_des->intc_des_tbl_offset);
					fwts_log_info_verbatim(fw, "      Interconnect locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					fwts_acpi_reserved_zero_array(fw, "MPAM", "Locator", res_node->locator + 8, 4, &passed);
					fwts_log_info_verbatim(fw, "      Interconnect descriptor table:");
					fwts_log_info_verbatim(fw, "        Signature:");
					fwts_hexdump_data_prefix_all(fw, intc_des_table->signature, "          ", sizeof(intc_des_table->signature));
					fwts_log_info_simp_int(fw, "        Number of descriptors:         ", intc_des_table->num_of_descriptors);
					fwts_acpi_reserved_zero_array(fw, "MPAM", "reserved",
						(uint8_t *)intc_des_table + sizeof(fwts_interconnect_descriptor_table) + 9,
						3, &passed);
					break;
				case FWTS_MPAM_UNKNOWN:
					fwts_log_info_verbatim(fw, "      Unknown locator:");
					fwts_hexdump_data_prefix_all(fw, res_node->locator, "        ", sizeof(res_node->locator));
					break;
				default:
					passed = false;
					fwts_failed(fw, LOG_LEVEL_HIGH,
						"MPAMBadLocaterType",
						"MPAM MPAM MSC locator type must not have the value 0x06..0xfe, got "
						"0x%2.2" PRIx8 "instead", res_node->locator_type);
					break;
			}

			fwts_log_info_simp_int(fw, "    Number of functional dependencies: ", res_node->num_functional_deps);

			fwts_acpi_reserved_zero("MPAM", "Reserved1", res_node->reserved1, &passed);

			msc_offset += sizeof(fwts_acpi_mpam_resource_node);

			for (uint32_t j = 0; j < res_node->num_functional_deps; j++) {
				fwts_acpi_mpam_func_deps *fun_deps = (fwts_acpi_mpam_func_deps *)(table->data + offset + msc_offset);
				fwts_log_info_verbatim(fw, "    Functional dependency descriptor: ");
				fwts_log_info_simp_int(fw, "      Producer:			       ", fun_deps->producer);
				fwts_log_info_simp_int(fw, "      Reserved:			       ", fun_deps->reserved);

				fwts_acpi_reserved_zero("MPAM", "Reserved1", fun_deps->reserved, &passed);

				msc_offset += sizeof(fwts_acpi_mpam_func_deps);
			}
		}

		if (node->length > msc_offset) {
			fwts_log_info_verbatim(fw, "  Resource-specific data: ");
			fwts_hexdump_data_prefix_all(fw, (uint8_t *)(table->data + offset + msc_offset), "    ", node->length - msc_offset);
		}

		offset += node->length;

		node = (fwts_acpi_mpam_msc_node *)(table->data + offset);
		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in MPAM table.");

	return FWTS_OK;
}

static fwts_framework_minor_test mpam_tests[] = {
	{ mpam_test1, "Validate MPAM table." },
	{ NULL, NULL }
};

static fwts_framework_ops mpam_ops = {
	.description = "MPAM Memory System Resource Partitioning And Monitoring Table test.",
	.init        = MPAM_init,
	.minor_tests = mpam_tests
};

FWTS_REGISTER("mpam", &mpam_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
