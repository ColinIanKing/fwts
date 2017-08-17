/*
 * Copyright (C) 2016-2017 Canonical
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
#include <stdbool.h>

static fwts_acpi_table_info *table;

static int pcct_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "PCCT", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI PCCT table does not exist, skipping test");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static bool subspace_length_equal(fwts_framework *fw, uint8_t type, uint8_t type_size, uint8_t length)
{
	if (type_size != length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTBadSubspaceLength",
			"PCCT Subspace Type %" PRId8 " must have length 0x%2.2" PRIx8 ", "
			"got 0x%2.2" PRIx8 " instead", type, type_size, length);
		return false;
	}

	return true;
}

static void generic_comm_test(fwts_framework *fw, fwts_acpi_table_pcct_subspace_type_0 *entry, bool *passed)
{
	fwts_acpi_gas *gas = &entry->doorbell_register;
	uint64_t reserved;

	reserved = (uint64_t) entry->reserved[0] + ((uint64_t) entry->reserved[1] << 8) +
		   ((uint64_t) entry->reserved[2] << 16) + ((uint64_t) entry->reserved[3] << 24) +
		   ((uint64_t) entry->reserved[4] << 32) + ((uint64_t) entry->reserved[5] << 40);

	fwts_log_info_verbatim(fw, "    Reserved:                    0x%16.16" PRIx64, reserved);
	fwts_log_info_verbatim(fw, "    Base Address:                0x%16.16" PRIx64, entry->base_address);
	fwts_log_info_verbatim(fw, "    Length:                      0x%16.16" PRIx64, entry->length);
	fwts_log_info_verbatim(fw, "    Doorbell Register:");
	fwts_log_info_verbatim(fw, "      Address Space ID           0x%2.2"   PRIx8, gas->address_space_id);
	fwts_log_info_verbatim(fw, "      Register Bit Width         0x%2.2"   PRIx8, gas->register_bit_width);
	fwts_log_info_verbatim(fw, "      Register Bit Offset        0x%2.2"   PRIx8, gas->register_bit_offset);
	fwts_log_info_verbatim(fw, "      Access Size                0x%2.2"   PRIx8, gas->access_width);
	fwts_log_info_verbatim(fw, "      Address                    0x%16.16" PRIx64, gas->address);
	fwts_log_info_verbatim(fw, "    Doorbell Preserve:           0x%16.16" PRIx64, entry->doorbell_preserve);
	fwts_log_info_verbatim(fw, "    Doorbell Write:              0x%16.16" PRIx64, entry->doorbell_write);
	fwts_log_info_verbatim(fw, "    Nominal Latency:             0x%8.8"   PRIx32, entry->nominal_latency);
	fwts_log_info_verbatim(fw, "    Max Periodic Access Rate:    0x%8.8"   PRIx32, entry->max_periodic_access_rate);
	fwts_log_info_verbatim(fw, "    Min Request Turnaround Time: 0x%8.8"   PRIx32, entry->min_request_turnaround_time);

	if ((gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY)) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTSubspaceInvalidAddrSpaceID",
			"PCCT Subspace Type 0 has space ID = 0x%2.2" PRIx8
			" which is not System I/O Space or System Memory Space",
			gas->address_space_id);
	}
}

static void hw_reduced_comm_test_type1(fwts_framework *fw, fwts_acpi_table_pcct_subspace_type_1 *entry, bool *passed)
{
	fwts_acpi_gas *gas = &entry->doorbell_register;
	uint8_t doorbell_int_flags = entry->doorbell_interrupt_flags;

	fwts_log_info_verbatim(fw, "    Doorbell Interrupt:          0x%8.8"   PRIx32, entry->doorbell_interrupt);
	fwts_log_info_verbatim(fw, "    Doorbell Interrupt Flags:    0x%2.2"   PRIx8, entry->doorbell_interrupt_flags);
	fwts_log_info_verbatim(fw, "    Reserved:                    0x%2.2"   PRIx8, entry->reserved);
	fwts_log_info_verbatim(fw, "    Base Address:                0x%16.16" PRIx64, entry->base_address);
	fwts_log_info_verbatim(fw, "    Length:                      0x%16.16" PRIx64, entry->length);
	fwts_log_info_verbatim(fw, "    Doorbell Register:");
	fwts_log_info_verbatim(fw, "      Address Space ID           0x%2.2"   PRIx8, gas->address_space_id);
	fwts_log_info_verbatim(fw, "      Register Bit Width         0x%2.2"   PRIx8, gas->register_bit_width);
	fwts_log_info_verbatim(fw, "      Register Bit Offset        0x%2.2"   PRIx8, gas->register_bit_offset);
	fwts_log_info_verbatim(fw, "      Access Size                0x%2.2"   PRIx8, gas->access_width);
	fwts_log_info_verbatim(fw, "      Address                    0x%16.16" PRIx64, gas->address);
	fwts_log_info_verbatim(fw, "    Doorbell Preserve:           0x%16.16" PRIx64, entry->doorbell_preserve);
	fwts_log_info_verbatim(fw, "    Doorbell Write:              0x%16.16" PRIx64, entry->doorbell_write);
	fwts_log_info_verbatim(fw, "    Nominal Latency:             0x%8.8"   PRIx32, entry->nominal_latency);
	fwts_log_info_verbatim(fw, "    Max Periodic Access Rate:    0x%8.8"   PRIx32, entry->max_periodic_access_rate);
	fwts_log_info_verbatim(fw, "    Min Request Turnaround Time: 0x%8.8"   PRIx32, entry->min_request_turnaround_time);

	if ((gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY) &&
			(gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_FFH)) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTSubspaceInvalidAddrSpaceID",
			"PCCT Subspace Type 1 has space ID = 0x%2.2" PRIx8
			" which is not System I/O Space or System Memory Space",
			gas->address_space_id);
	}

	if (doorbell_int_flags & ~0x3) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTBadSubtypeDoorbellFlags",
			"PCCT Subspace Doorbell Interrupt's bit 2..7 be zero, got "
			"0x%2.2" PRIx8 " instead", doorbell_int_flags);
	}
}

static void hw_reduced_comm_test_type2(fwts_framework *fw, fwts_acpi_table_pcct_subspace_type_2 *entry, bool *passed)
{
	fwts_acpi_gas *gas = &entry->doorbell_register;
	fwts_acpi_gas *gas2 = &entry->doorbell_ack_register;
	uint8_t doorbell_int_flags = entry->doorbell_interrupt_flags;

	fwts_log_info_verbatim(fw, "    Doorbell Interrupt:          0x%8.8"   PRIx32, entry->doorbell_interrupt);
	fwts_log_info_verbatim(fw, "    Doorbell Interrupt Flags:    0x%2.2"   PRIx8, entry->doorbell_interrupt_flags);
	fwts_log_info_verbatim(fw, "    Reserved:                    0x%2.2"   PRIx8, entry->reserved);
	fwts_log_info_verbatim(fw, "    Base Address:                0x%16.16" PRIx64, entry->base_address);
	fwts_log_info_verbatim(fw, "    Length:                      0x%16.16" PRIx64, entry->length);
	fwts_log_info_verbatim(fw, "    Doorbell Register:");
	fwts_log_info_verbatim(fw, "      Address Space ID           0x%2.2"   PRIx8, gas->address_space_id);
	fwts_log_info_verbatim(fw, "      Register Bit Width         0x%2.2"   PRIx8, gas->register_bit_width);
	fwts_log_info_verbatim(fw, "      Register Bit Offset        0x%2.2"   PRIx8, gas->register_bit_offset);
	fwts_log_info_verbatim(fw, "      Access Size                0x%2.2"   PRIx8, gas->access_width);
	fwts_log_info_verbatim(fw, "      Address                    0x%16.16" PRIx64, gas->address);
	fwts_log_info_verbatim(fw, "    Doorbell Preserve:           0x%16.16" PRIx64, entry->doorbell_preserve);
	fwts_log_info_verbatim(fw, "    Doorbell Write:              0x%16.16" PRIx64, entry->doorbell_write);
	fwts_log_info_verbatim(fw, "    Nominal Latency:             0x%8.8"   PRIx32, entry->nominal_latency);
	fwts_log_info_verbatim(fw, "    Max Periodic Access Rate:    0x%8.8"   PRIx32, entry->max_periodic_access_rate);
	fwts_log_info_verbatim(fw, "    Min Request Turnaround Time: 0x%8.8"   PRIx32, entry->min_request_turnaround_time);
	fwts_log_info_verbatim(fw, "    Doorbell Ack Register:");
	fwts_log_info_verbatim(fw, "      Address Space ID           0x%2.2"   PRIx8, gas2->address_space_id);
	fwts_log_info_verbatim(fw, "      Register Bit Width         0x%2.2"   PRIx8, gas2->register_bit_width);
	fwts_log_info_verbatim(fw, "      Register Bit Offset        0x%2.2"   PRIx8, gas2->register_bit_offset);
	fwts_log_info_verbatim(fw, "      Access Size                0x%2.2"   PRIx8, gas2->access_width);
	fwts_log_info_verbatim(fw, "      Address                    0x%16.16" PRIx64, gas2->address);
	fwts_log_info_verbatim(fw, "    Doorbell Ack Preserve:       0x%16.16" PRIx64, entry->doorbell_ack_preserve);
	fwts_log_info_verbatim(fw, "    Doorbell Ack Write:          0x%16.16" PRIx64, entry->doorbell_ack_write);

	if ((gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY) &&
			(gas->address_space_id != FWTS_GAS_ADDR_SPACE_ID_FFH)) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTSubspaceInvalidAddrSpaceID",
			"PCCT Subspace Type 2 has space ID = 0x%2.2" PRIx8
			" which is not System I/O Space or System Memory Space",
			gas->address_space_id);
	}

	if ((gas2->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (gas2->address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY) &&
			(gas2->address_space_id != FWTS_GAS_ADDR_SPACE_ID_FFH)) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTSubspaceInvalidAddrSpaceID",
			"PCCT Subspace Type 2 has space ID = 0x%2.2" PRIx8
			" which is not System I/O Space or System Memory Space",
			gas->address_space_id);
	}

	if (doorbell_int_flags & ~0x3) {
		*passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTBadSubtypeDoorbellFlags",
			"PCCT Subspace Doorbell Interrupt's bit 2..7 be zero, got "
			"0x%2.2" PRIx8 " instead", doorbell_int_flags);
	}
}

static int pcct_test1(fwts_framework *fw)
{
	fwts_acpi_table_pcct *pcct = (fwts_acpi_table_pcct*) table->data;
	fwts_acpi_table_pcct_subspace_header *pcct_sub;
	size_t offset;
	bool passed = true;

	fwts_log_info_verbatim(fw, "PCC Table:");
	fwts_log_info_verbatim(fw, "  Flags:     0x%8.8"   PRIx32, pcct->flags);
	fwts_log_info_verbatim(fw, "  Reserved:  0x%16.16"   PRIx64, pcct->reserved);
	fwts_log_nl(fw);

	if ((pcct->flags & ~0x01) != 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"PCCTReservedNonZero",
			"PCCT flags field's bit 1..31 be zero, got "
			"0x%8.8" PRIx32 " instead", pcct->flags);
		}

	if (pcct->reserved != 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"PCCTReservedNonZero",
			"PCCT reserved field must be zero, got "
			"0x%16.16" PRIx64 " instead", pcct->reserved);
	}

	offset = sizeof(fwts_acpi_table_pcct);
	pcct_sub = (fwts_acpi_table_pcct_subspace_header *) (table->data + offset);

	while (offset < table->length) {
		uint64_t addr_length = 8;

		fwts_log_info_verbatim(fw, "  PCC Subspace Structure:");
		fwts_log_info_verbatim(fw, "    Type:                        0x%2.2"   PRIx8, pcct_sub->type);
		fwts_log_info_verbatim(fw, "    Length:                      0x%2.2"   PRIx8, pcct_sub->length);

		if (pcct_sub->type == 0) {
			fwts_acpi_table_pcct_subspace_type_0 *subspace = (fwts_acpi_table_pcct_subspace_type_0 *) pcct_sub;

			if(!subspace_length_equal(fw, 0, sizeof(fwts_acpi_table_pcct_subspace_type_0), pcct_sub->length)) {
				passed = false;
				break;
			}

			generic_comm_test(fw, subspace, &passed);
			addr_length = subspace->length;

		} else if (pcct_sub->type == 1) {
			fwts_acpi_table_pcct_subspace_type_1 *subspace = (fwts_acpi_table_pcct_subspace_type_1 *) pcct_sub;

			if(!subspace_length_equal(fw, 0, sizeof(fwts_acpi_table_pcct_subspace_type_1), pcct_sub->length)) {
				passed = false;
				break;
			}

			hw_reduced_comm_test_type1(fw, subspace, &passed);
			addr_length = subspace->length;

		} else if (pcct_sub->type == 2) {
			fwts_acpi_table_pcct_subspace_type_2 *subspace = (fwts_acpi_table_pcct_subspace_type_2 *) pcct_sub;

			if(!subspace_length_equal(fw, 0, sizeof(fwts_acpi_table_pcct_subspace_type_2), pcct_sub->length)) {
				passed = false;
				break;
			}

			hw_reduced_comm_test_type2(fw, subspace, &passed);
			addr_length = subspace->length;

		} else {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"PCCTBadSubType",
				"PCCT Subspace Structure supports type 0..2, got "
				"0x%2.2" PRIx8 " instead", pcct_sub->type);
		}

		if (addr_length <= 8) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"PCCTBadSubtypeMemoryLength",
				"PCCT Subspace Structure must have memory length > 8, got "
				"0x%16.16" PRIx64 " instead", addr_length);
		}



		fwts_log_nl(fw);

		offset += pcct_sub->length;
		pcct_sub = (fwts_acpi_table_pcct_subspace_header *) (table->data + offset);
	}

	if (passed)
		fwts_passed(fw, "No issues found in PCC table.");

	return FWTS_OK;
}

static fwts_framework_minor_test pcct_tests[] = {
	{ pcct_test1, "Validate PCC table." },
	{ NULL, NULL }
};

static fwts_framework_ops pcct_ops = {
	.description = "PCCT Platform Communications Channel test.",
	.init        = pcct_init,
	.minor_tests = pcct_tests
};

FWTS_REGISTER("pcct", &pcct_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
