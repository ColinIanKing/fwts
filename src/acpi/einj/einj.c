/*
 * Copyright (C) 2016-2023 Canonical
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

#include "fwts_acpi_object_eval.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

static fwts_acpi_table_info *table;
acpi_table_init(EINJ, &table)

/*
 *  EINJ Error Injection Table
 */
static int einj_test1(fwts_framework *fw)
{
	fwts_acpi_table_einj *einj = (fwts_acpi_table_einj *) table->data;
	fwts_acpi_serialization_instruction_entries *entry;
	uint32_t reserved, i;
	bool passed = true;

	reserved = einj->reserved[0] + ((uint32_t) einj->reserved[1] << 8) +
		   ((uint32_t) einj->reserved[2] << 16);

	fwts_log_info_verbatim(fw, "EINJ Error Injection Table:");
	fwts_log_info_simp_int(fw, "  Injection Header Size: ", einj->header_size);
	fwts_log_info_simp_int(fw, "  Injection Flags:       ", einj->flags);
	fwts_log_info_simp_int(fw, "  Reserved:              ", reserved);
	fwts_log_info_simp_int(fw, "  Injection Entry Count: ", einj->count);

	fwts_acpi_reserved_bits("EINJ", "Injection Flags", einj->flags, 0, 31, &passed);
	fwts_acpi_reserved_zero("EINJ", "Reserved", reserved, &passed);

	fwts_log_nl(fw);

	entry = (fwts_acpi_serialization_instruction_entries *)
			(char *)(einj + 48);
	for (i = 0; i < einj->count; i++,  entry++) {
		fwts_acpi_gas gas = entry->register_region;
		fwts_log_info_verbatim(fw, "  Injection Instruction Entry %2.2"
				PRId8, i);
		fwts_log_info_verbatim(fw, "    Injection Action    : 0x%2.2"
				PRIx8, entry->serialization_action);
		fwts_log_info_verbatim(fw, "    Instruction         : 0x%2.2"
				PRIx8, entry->instruction);
		fwts_log_info_verbatim(fw, "    Flags               : 0x%2.2"
				PRIx8, entry->flags);
		fwts_log_info_verbatim(fw, "    Reserved            : 0x%2.2"
				PRIx8, entry->reserved);
		fwts_log_info_verbatim(fw, "    Address Space ID    : 0x%2.2"
				PRIx8, gas.address_space_id);
		fwts_log_info_verbatim(fw, "    Register Bit Width  : 0x%2.2"
				PRIx8, gas.register_bit_width);
		fwts_log_info_verbatim(fw, "    Register Bit Offset : 0x%2.2"
				PRIx8, gas.register_bit_offset);
		fwts_log_info_verbatim(fw, "    Access Size         : 0x%2.2"
				PRIx8, gas.access_width);
		fwts_log_info_verbatim(fw, "    Address             : 0x%16.16"
				PRIx64, gas.address);
		fwts_log_info_verbatim(fw, "    Value               : 0x%16.16"
				PRIx64, entry->value);
		fwts_log_info_verbatim(fw, "    Mask                : 0x%16.16"
				PRIx64, entry->mask);

		if (einj->header.revision >= 2) {
			if (entry->serialization_action > 0x11 &&
			    entry->serialization_action != 0xFF) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					    "EINJBadInjectionAction",
					    "EINJ Injection Action must be within 0~9 "
					    "or 0xFF got 0x%" PRIx8 " instead",
					    entry->serialization_action);
				passed = false;
			}
		} else {
			if (entry->serialization_action > 0x9 &&
			    entry->serialization_action != 0xFF) {
				fwts_failed(fw, LOG_LEVEL_MEDIUM,
					    "EINJBadInjectionAction",
					    "EINJ Injection Action must be within 0~9 "
					    "or 0xFF got 0x%" PRIx8 " instead",
					    entry->serialization_action);
				passed = false;
			}
		}

		if (entry->instruction > 0x4) {
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				    "EINJBadInstruction",
				    "EINJ Instruction must be within 0~4, got "
				    "0x%" PRIx8 " instead", entry->instruction);
			passed = false;
		}

		fwts_acpi_reserved_zero("EINJ", "Reserved", entry->reserved, &passed);
		fwts_acpi_space_id(fw, "EINJ", "Register Region", &passed, gas.address_space_id, 2,
				   FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY, FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO);

		fwts_log_nl(fw);
	}

	if (passed)
		fwts_passed(fw, "No issues found in EINJ table.");

	return FWTS_OK;
}

static fwts_framework_minor_test einj_tests[] = {
	{ einj_test1, "EINJ Error Injection Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops einj_ops = {
	.description = "EINJ Error Injection Table test.",
	.init        = EINJ_init,
	.minor_tests = einj_tests
};

FWTS_REGISTER("einj", &einj_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
