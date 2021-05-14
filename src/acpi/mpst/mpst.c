/*
 * Copyright (C) 2016-2021 Canonical
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
acpi_table_init(MPST, &table)

static int mpst_test1(fwts_framework *fw)
{
	fwts_acpi_table_mpst *mpst = (fwts_acpi_table_mpst*) table->data;
	fwts_acpi_table_mpst_power_node_list *node_list;
	fwts_acpi_table_mpst_power_char_list *char_list;
	bool passed = true;
	uint32_t reserved;
	uint32_t node_offset;
	uint32_t j;
	uint16_t i;

	reserved = (uint32_t) mpst->reserved[0] +
		   ((uint32_t) mpst->reserved[1] << 8) +
		   ((uint32_t) mpst->reserved[2] << 16);

	fwts_log_info_verbatim(fw, "MPST Table:");
	fwts_log_info_simp_int(fw, "  Communication Channel ID:        ", mpst->channel_id);
	fwts_log_info_simp_int(fw, "  Reserved:                        ", reserved);

	fwts_acpi_reserved_zero("MPST", "Reserved", reserved, &passed);

	node_list = (fwts_acpi_table_mpst_power_node_list *) (table->data + sizeof(fwts_acpi_table_mpst));
	fwts_log_info_simp_int(fw, "  Memory Power Node Count:         ", node_list->count);
	fwts_log_info_simp_int(fw, "  Reserved:                        ", node_list->reserved);

	fwts_acpi_reserved_zero("MPST", "Reserved", node_list->reserved, &passed);

	node_offset = sizeof(fwts_acpi_table_mpst) + (sizeof(fwts_acpi_table_mpst_power_node_list));
	if (mpst->header.length < node_offset + sizeof(fwts_acpi_table_mpst_power_node) * node_list->count) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"MPSTOutOfBound",
			"MPST's table length is too small to contain all sub-tables");
		goto error;
	}

	for (i = 0; i < node_list->count; i++) {
		fwts_acpi_table_mpst_power_node *power_node = (fwts_acpi_table_mpst_power_node *) (table->data + node_offset);
		uint32_t node_length;

		fwts_log_info_verbatim(fw, "  MPST Power Node:");
		fwts_log_info_simp_int(fw, "    Flags:                         ", power_node->flags);
		fwts_log_info_simp_int(fw, "    Reserved:                      ", power_node->reserved);
		fwts_log_info_simp_int(fw, "    Memory Power Node Id:          ", power_node->node_id);
		fwts_log_info_simp_int(fw, "    Power Node Length:             ", power_node->length);
		fwts_log_info_simp_int(fw, "    Base Address:                  ", power_node->range_address);
		fwts_log_info_simp_int(fw, "    Memory Length:                 ", power_node->range_length);
		fwts_log_info_simp_int(fw, "    Number of Power States:        ", power_node->num_states);
		fwts_log_info_simp_int(fw, "    Number of Physical Components: ", power_node->num_components);

		fwts_acpi_reserved_bits("MPST", "Power Node Flags", power_node->flags, 3, 7, &passed);
		fwts_acpi_reserved_zero("MPST", "Reserved", power_node->reserved, &passed);

		node_length = sizeof(fwts_acpi_table_mpst_power_node) +
			      sizeof(fwts_acpi_table_mpst_power_state) * power_node->num_states +
			      sizeof(fwts_acpi_table_mpst_component) * power_node->num_components;

		if (power_node->length != node_length) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"MPSTBadPowerNodeLength",
				"MPST Power Node Length should be 0x%8.8" PRIx32 ", got "
				"0x%8.8" PRIx32 " instead", node_length, power_node->length);
		}

		node_offset += sizeof(fwts_acpi_table_mpst_power_node);
		if (mpst->header.length < node_offset + sizeof(fwts_acpi_table_mpst_power_state) * power_node->num_states) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"MPSTOutOfBound",
				"MPST's table length is too small to contain all sub-tables");
			goto error;
		}

		for (j = 0; j < power_node->num_states; j++) {
			fwts_acpi_table_mpst_power_state *state = (fwts_acpi_table_mpst_power_state *) (table->data + node_offset);
			fwts_log_info_simp_int(fw, "    Power State Value:             ", state->value);
			fwts_log_info_simp_int(fw, "    Power State Information Index: ", state->info_index);
			node_offset += sizeof(fwts_acpi_table_mpst_power_state);
		}

		if (mpst->header.length < node_offset + sizeof(fwts_acpi_table_mpst_component) * power_node->num_components) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"MPSTOutOfBound",
				"MPST's table length is too small to contain all sub-tables");
			goto error;
		}

		for (j = 0; j < power_node->num_components; j++) {
			fwts_acpi_table_mpst_component *component = (fwts_acpi_table_mpst_component *) (table->data + node_offset);
			fwts_log_info_simp_int(fw, "    Physical Component Id:         ", component->id);
			node_offset +=  sizeof(fwts_acpi_table_mpst_component);
		}
	}

	char_list = (fwts_acpi_table_mpst_power_char_list *) (table->data + node_offset);
	fwts_log_info_simp_int(fw, "  Memory Characteristics Count:    ", char_list->count);
	fwts_log_info_simp_int(fw, "  Reserved:                        ", char_list->reserved);

	fwts_acpi_reserved_zero("MPST", "Reserved", char_list->reserved, &passed);

	node_offset += sizeof(fwts_acpi_table_mpst_power_char_list);
	if (mpst->header.length < node_offset + sizeof(fwts_acpi_table_mpst_power_char) * char_list->count) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"MPSTOutOfBound",
			"MPST's table length is too small to contain all sub-tables");
		goto error;
	}

	for (i = 0; i < char_list->count; i++) {
		fwts_acpi_table_mpst_power_char *power_char = (fwts_acpi_table_mpst_power_char *) (table->data + node_offset);

		fwts_log_info_verbatim(fw, "  MPST Power Characteristics:");
		fwts_log_info_simp_int(fw, "    Power State Structure ID:      ", power_char->structure_id);
		fwts_log_info_simp_int(fw, "    Flags:                         ", power_char->flags);
		fwts_log_info_simp_int(fw, "    Reserved:                      ", power_char->reserved1);
		fwts_log_info_simp_int(fw, "    Average Power Consumed:        ", power_char->average_power);
		fwts_log_info_simp_int(fw, "    Relative Power Saving:         ", power_char->power_saving);
		fwts_log_info_simp_int(fw, "    Exit Latency:                  ", power_char->exit_latency);
		fwts_log_info_simp_int(fw, "    Reserved:                      ", power_char->reserved2);

		if ((power_char->structure_id & 0x3F) != 1) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"MPSTBadPowerCharacteristicsID",
				"MPST Power Characteristics ID must be 1, got "
				"0x%2.2" PRIx8 " instead", power_char->structure_id & 0x3F);
		}

		if (((power_char->structure_id & 0xC0) >> 6) != 1) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"MPSTBadPowerCharacteristicsRevision",
				"MPST Power Characteristics Revision must be 1, got "
				"0x%2.2" PRIx8 " instead", (power_char->structure_id & 0xC0) >> 6);
		}

		fwts_acpi_reserved_bits("MPST", "Power Characteristics Flags", power_char->flags, 3, 7, &passed);
		fwts_acpi_reserved_zero("MPST", "Reserved1", power_char->reserved1, &passed);
		fwts_acpi_reserved_zero("MPST", "Reserved2", power_char->reserved2, &passed);

		node_offset +=  sizeof(fwts_acpi_table_mpst_power_char);
	}

	fwts_log_nl(fw);

	if (passed)
		fwts_passed(fw, "No issues found in MPST table.");

 error:
	return FWTS_OK;
}

static fwts_framework_minor_test mpst_tests[] = {
	{ mpst_test1, "Validate MPST table." },
	{ NULL, NULL }
};

static fwts_framework_ops mpst_ops = {
	.description = "MPST Memory Power State Table test.",
	.init        = MPST_init,
	.minor_tests = mpst_tests
};

FWTS_REGISTER("mpst", &mpst_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
