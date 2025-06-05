/*
 * Copyright (C) 2016-2025 Canonical
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
acpi_table_init(ASPT, &table)

/*
 *  ASPT Table
 *    (reverse engineered, table is common on AMD machines)
 */
static int aspt_test1(fwts_framework *fw)
{
	bool passed = true;
	uint32_t offset;
	fwts_acpi_table_aspt *aspt = (fwts_acpi_table_aspt *)table->data;

	fwts_log_info_verbatim(fw, "AMD Secure Processor Table:");
	fwts_log_info_simp_int(fw, "  ASP Register Structure Count:    ", aspt->asp_reg_count);

	offset = sizeof(aspt->header) + sizeof(aspt->asp_reg_count);

	for (uint32_t i = 0; i <  aspt->asp_reg_count; i++) {
		if ((offset + sizeof(fwts_aspt_sub_header)) > table->length) {
			fwts_failed(fw, LOG_LEVEL_HIGH,
			"ASPTOutOfRangeOffset",
			"ASPT offset is out of range.");
			return FWTS_OK;
		}
		uint16_t type = ((fwts_aspt_sub_header *)((uint8_t *)table->data + offset))->type;
		switch (type) {
			case 0:
				fwts_acpi_table_asp_global *entry_global;
				entry_global = (fwts_acpi_table_asp_global *)((uint8_t *)table->data + offset);
				fwts_log_info_verbatim(fw, "  ASP Global Registers:");
				fwts_log_info_simp_int(fw, "    Type:                              ", entry_global->header.type);
				fwts_log_info_simp_int(fw, "    Length:                            ", entry_global->header.length);
				fwts_log_info_simp_int(fw, "    Reserved:                          ", entry_global->reserved);
				fwts_acpi_reserved_zero("ASPT", "Reserved", entry_global->reserved, &passed);
				fwts_log_info_simp_int(fw, "    Feature Register Address:          ", entry_global->feature_reg_addr);
				fwts_log_info_simp_int(fw, "    Interrupt Enable Register Address: ", entry_global->interrupt_enable_reg_addr);
				fwts_log_info_simp_int(fw, "    Interrupt Status Register Address: ", entry_global->interrupt_stable_reg_addr);
				offset += entry_global->header.length;
				break;
			case 1:
				fwts_acpi_table_sev_mailbox *entry_sev;
				entry_sev = (fwts_acpi_table_sev_mailbox *)((uint8_t *)table->data + offset);
				fwts_log_info_verbatim(fw, "  SEV Mailbox Registers:");
				fwts_log_info_simp_int(fw, "    Type:                              ", entry_sev->header.type);
				fwts_log_info_simp_int(fw, "    Length:                            ", entry_sev->header.length);
				fwts_log_info_simp_int(fw, "    Mailbox Interrupt ID:              ", entry_sev->mailbox_interrupt_id);
				fwts_acpi_reserved_bits("ASPT", "Mailbox Interrupt ID",  entry_sev->mailbox_interrupt_id, 6, 7, &passed);
				fwts_log_info_verbatim(fw, "    Reserved:");
				fwts_hexdump_data_prefix_all(fw, entry_sev->reserved, "      ", sizeof(entry_sev->reserved));
				fwts_acpi_reserved_zero_array(fw, "ASPT", "Reserved", entry_sev->reserved, sizeof(entry_sev->reserved), &passed);
				fwts_log_info_simp_int(fw, "    CmdResp Register Address:          ", entry_sev->cmdresp_reg_addr);
				fwts_log_info_simp_int(fw, "    CmdBufAddr_Lo Register Address:    ", entry_sev->cmdbufaddr_lo_reg_addr);
				fwts_log_info_simp_int(fw, "    CmdBufAddr_Hi Register Address:    ", entry_sev->cmdbufaddr_hi_reg_addr);
				offset += entry_sev->header.length;
				break;
			case 2:
				fwts_acpi_table_acpi_mailbox *entry_acpi;
				entry_acpi = (fwts_acpi_table_acpi_mailbox *)((uint8_t *)table->data + offset);
				fwts_log_info_verbatim(fw, "  ACPI Mailbox Registers:");
				fwts_log_info_simp_int(fw, "    Type:                              ", entry_acpi->header.type);
				fwts_log_info_simp_int(fw, "    Length:                            ", entry_acpi->header.length);
				fwts_log_info_simp_int(fw, "    Reserved:                          ", entry_acpi->reserved);
				fwts_acpi_reserved_zero("ASPT", "Reserved", entry_acpi->reserved, &passed);
				fwts_log_info_simp_int(fw, "    CmdResp Register Address:          ", entry_acpi->cmdresp_reg_addr);
				fwts_log_info_verbatim(fw, "    Reserved:");
				fwts_hexdump_data_prefix_all(fw, entry_acpi->reserved1, "      ", sizeof(entry_acpi->reserved1));
				fwts_acpi_reserved_zero_array(fw, "ASPT", "Reserved", entry_acpi->reserved1, sizeof(entry_acpi->reserved1), &passed);
				offset += entry_acpi->header.length;
				break;
			default:
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"ASPTBadType",
					"ASPT register structures must have type with 0, 1 "
					"and 2, got %" PRIu16 " instead",
					((fwts_aspt_sub_header *)((uint8_t *)table->data + offset))->type);
				return FWTS_OK;
				break;
			fwts_log_nl(fw);
		}
	}

	if (passed)
		fwts_passed(fw, "No issues found in ASPT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test aspt_tests[] = {
	{ aspt_test1, "ASPT Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops aspt_ops = {
	.description = "ASPT Table test.",
	.init        = ASPT_init,
	.minor_tests = aspt_tests
};

FWTS_REGISTER("aspt", &aspt_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
