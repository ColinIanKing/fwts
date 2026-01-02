/*
 * Copyright (C) 2010-2026 Canonical
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
acpi_table_init(TCPA, &table)

static int tcpa_client_test(fwts_framework *fw, fwts_acpi_table_tcpa *tcpa)
{
	bool passed = true;

	fwts_acpi_fixed_value(fw, LOG_LEVEL_HIGH, "TCPA", "Length", tcpa->header.length, 50, &passed);
	fwts_acpi_revision_check("TCPA", tcpa->header.revision, 2, &passed);

	fwts_log_info_verbatim(fw, "TCPA Table:");
	fwts_log_info_simp_int(fw, "  Platform Class:                  ", tcpa->platform_class);
	fwts_log_info_simp_int(fw, "  Log Area Minimum Length:         ", tcpa->client.log_zone_length);
	fwts_log_info_simp_int(fw, "  Log Area Start Address:          ", tcpa->client.log_zone_addr);

	return passed;
}

static int tcpa_server_test(fwts_framework *fw, fwts_acpi_table_tcpa *tcpa)
{
	bool passed = true;
	uint32_t reserved2;

	fwts_acpi_fixed_value(fw, LOG_LEVEL_HIGH, "TCPA", "Length", tcpa->header.length, 100, &passed);
	fwts_acpi_revision_check("TCPA", tcpa->header.revision, 2, &passed);

	reserved2 = tcpa->server.reserved2[0] + (tcpa->server.reserved2[1] << 4) + (tcpa->server.reserved2[2] << 8);

	fwts_log_info_verbatim(fw, "TCPA Table:");
	fwts_log_info_simp_int(fw, "  Platform Class:                  ", tcpa->platform_class);
	fwts_log_info_simp_int(fw, "  Reserved:                        ", tcpa->server.reserved);
	fwts_log_info_simp_int(fw, "  Log Area Minimum Length:         ", tcpa->server.log_zone_length);
	fwts_log_info_simp_int(fw, "  Log Area Start Address:          ", tcpa->server.log_zone_addr);
	fwts_log_info_simp_int(fw, "  Specification Revision:          ", tcpa->server.spec_revision);
	fwts_log_info_simp_int(fw, "  Device Flags:                    ", tcpa->server.device_flag);
	fwts_log_info_simp_int(fw, "  Interrupt Flags:                 ", tcpa->server.interrupt_flag);
	fwts_log_info_simp_int(fw, "  GPE:                             ", tcpa->server.gpe);
	fwts_log_info_simp_int(fw, "  Reserved:                        ", reserved2);
	fwts_log_info_simp_int(fw, "  Global System Interrupt:         ", tcpa->server.global_sys_interrupt);
	fwts_log_info_verbatim(fw, "  Base Address:");
	fwts_log_info_simp_int(fw, "    Address Space ID:              ", tcpa->server.base_addr.address_space_id);
	fwts_log_info_simp_int(fw, "    Register Bit Width             ", tcpa->server.base_addr.register_bit_width);
	fwts_log_info_simp_int(fw, "    Register Bit Offset            ", tcpa->server.base_addr.register_bit_offset);
	fwts_log_info_simp_int(fw, "    Access Size                    ", tcpa->server.base_addr.access_width);
	fwts_log_info_simp_int(fw, "    Address                        ", tcpa->server.base_addr.address);
	fwts_log_info_simp_int(fw, "  Reserved:                        ", tcpa->server.reserved3);
	fwts_log_info_verbatim(fw, "  Configuration Address:");
	fwts_log_info_simp_int(fw, "    Address Space ID:              ", tcpa->server.config_addr.address_space_id);
	fwts_log_info_simp_int(fw, "    Register Bit Width             ", tcpa->server.config_addr.register_bit_width);
	fwts_log_info_simp_int(fw, "    Register Bit Offset            ", tcpa->server.config_addr.register_bit_offset);
	fwts_log_info_simp_int(fw, "    Access Size                    ", tcpa->server.config_addr.access_width);
	fwts_log_info_simp_int(fw, "    Address                        ", tcpa->server.config_addr.address);
	fwts_log_info_simp_int(fw, "  PCI Segment Group:               ", tcpa->server.pci_seg_number);
	fwts_log_info_simp_int(fw, "  PCI Bus:                         ", tcpa->server.pci_bus_number);
	fwts_log_info_simp_int(fw, "  PCI Device:                      ", tcpa->server.pci_dev_number);
	fwts_log_info_simp_int(fw, "  PCI Function:                    ", tcpa->server.pci_func_number);

	fwts_acpi_reserved_zero("TCPA", "Reserved", tcpa->server.reserved, &passed);
	fwts_acpi_reserved_zero("TCPA", "Reserved2", reserved2, &passed);
	fwts_acpi_reserved_zero("TCPA", "Reserved3", tcpa->server.reserved3, &passed);

	if (tcpa->server.device_flag & 1) {
		if (!(tcpa->server.interrupt_flag & 2)) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"TCPABadInterruptPolarity",
				"TCPA interrupt Polarity should be one, got zero");
		}

		if (tcpa->server.interrupt_flag & 1) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"TCPABadInterruptMode",
				"TCPA interrupt mode should be zero, got one");
		}
	}

	fwts_acpi_space_id(fw, "TCPA", "Base Address", &passed,
			   tcpa->server.base_addr.address_space_id, 2,
			   FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY,
			   FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO);

	fwts_acpi_space_id(fw, "TCPA", "Configuration Address", &passed,
			   tcpa->server.config_addr.address_space_id, 2,
			   FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY,
			   FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO);

	return passed;
}

/*
 * TCPA table
 *   available @ http://www.trustedcomputinggroup.org/files/static_page_files/648D7D46-1A4B-B294-D088037B8F73DAAF/TCG_ACPIGeneralSpecification_1-10_0-37-Published.pdf
 */
static int tcpa_test1(fwts_framework *fw)
{
	fwts_acpi_table_tcpa *tcpa = (fwts_acpi_table_tcpa*)table->data;
	bool passed;

	switch (tcpa->platform_class) {
	case 0:
		passed = tcpa_client_test(fw, tcpa);
		break;
	case 1:
		passed = tcpa_server_test(fw, tcpa);
		break;
	default:
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadPlatformClass",
			"TCPA's platform class must be zero or one, got 0x%" PRIx16,
			tcpa->platform_class);
		break;
	}

	if (passed)
		fwts_passed(fw, "No issues found in TCPA table.");

	return FWTS_OK;
}

static fwts_framework_minor_test tcpa_tests[] = {
	{ tcpa_test1, "Validate TCPA table." },
	{ NULL, NULL }
};

static fwts_framework_ops tcpa_ops = {
	.description = "TCPA Trusted Computing Platform Alliance Capabilities Table test.",
	.init        = TCPA_init,
	.minor_tests = tcpa_tests
};

FWTS_REGISTER("tcpa", &tcpa_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
