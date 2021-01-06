/*
 * Copyright (C) 2010-2021 Canonical
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

	if (tcpa->header.length != 50) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadSize",
			"TCPA size is incorrect, expecting 0x32 bytes, "
			"instead got 0x%8.8" PRIx32 " bytes",
			tcpa->header.length);
	}

	if (tcpa->header.revision != 2) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadRevision",
			"TCPA revision is incorrect, expecting 0x02, "
			"instead got 0x%2.2" PRIx8,
			tcpa->header.revision);
	}

	fwts_log_info_verbatim(fw, "TCPA Table:");
	fwts_log_info_verbatim(fw, "  Platform Class:                  0x%4.4"   PRIx16, tcpa->platform_class);
	fwts_log_info_verbatim(fw, "  Log Area Minimum Length:         0x%8.8"   PRIx32, tcpa->client.log_zone_length);
	fwts_log_info_verbatim(fw, "  Log Area Start Address:          0x%16.16" PRIx64, tcpa->client.log_zone_addr);

	return passed;
}

static int tcpa_server_test(fwts_framework *fw, fwts_acpi_table_tcpa *tcpa)
{
	bool passed = true;
	uint32_t reserved2;

	if (tcpa->header.length != 100) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadSize",
			"TCPA size is incorrect, expecting 0x64 bytes, "
			"instead got 0x%8.8" PRIx32 " bytes",
			tcpa->header.length);
	}

	if (tcpa->header.revision != 2) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadRevision",
			"TCPA revision is incorrect, expecting 0x02, "
			"instead got 0x%2.2" PRIx8,
			tcpa->header.revision);
	}

	reserved2 = tcpa->server.reserved2[0] + (tcpa->server.reserved2[1] << 4) + (tcpa->server.reserved2[2] << 8);

	fwts_log_info_verbatim(fw, "TCPA Table:");
	fwts_log_info_verbatim(fw, "  Platform Class:                  0x%4.4"   PRIx16, tcpa->platform_class);
	fwts_log_info_verbatim(fw, "  Reserved:                        0x%4.4"   PRIx16, tcpa->server.reserved);
	fwts_log_info_verbatim(fw, "  Log Area Minimum Length:         0x%16.16" PRIx64, tcpa->server.log_zone_length);
	fwts_log_info_verbatim(fw, "  Log Area Start Address:          0x%16.16" PRIx64, tcpa->server.log_zone_addr);
	fwts_log_info_verbatim(fw, "  Specification Revision:          0x%4.4"   PRIx16, tcpa->server.spec_revision);
	fwts_log_info_verbatim(fw, "  Device Flags:                    0x%2.2"   PRIx16, tcpa->server.device_flag);
	fwts_log_info_verbatim(fw, "  Interrupt Flags:                 0x%2.2"   PRIx16, tcpa->server.interrupt_flag);
	fwts_log_info_verbatim(fw, "  GPE:                             0x%2.2"   PRIx16, tcpa->server.gpe);
	fwts_log_info_verbatim(fw, "  Reserved:                        0x%8.8"   PRIx32, reserved2);
	fwts_log_info_verbatim(fw, "  Global System Interrupt:         0x%8.8"   PRIx32, tcpa->server.global_sys_interrupt);
	fwts_log_info_verbatim(fw, "  Base Address:");
	fwts_log_info_verbatim(fw, "    Address Space ID:              0x%2.2"   PRIx8, tcpa->server.base_addr.address_space_id);
	fwts_log_info_verbatim(fw, "    Register Bit Width             0x%2.2"   PRIx8, tcpa->server.base_addr.register_bit_width);
	fwts_log_info_verbatim(fw, "    Register Bit Offset            0x%2.2"   PRIx8, tcpa->server.base_addr.register_bit_offset);
	fwts_log_info_verbatim(fw, "    Access Size                    0x%2.2"   PRIx8, tcpa->server.base_addr.access_width);
	fwts_log_info_verbatim(fw, "    Address                        0x%16.16" PRIx64, tcpa->server.base_addr.address);
	fwts_log_info_verbatim(fw, "  Reserved:                        0x%8.8"   PRIx32, tcpa->server.reserved3);
	fwts_log_info_verbatim(fw, "  Configuration Address:");
	fwts_log_info_verbatim(fw, "    Address Space ID:              0x%2.2"   PRIx8, tcpa->server.config_addr.address_space_id);
	fwts_log_info_verbatim(fw, "    Register Bit Width             0x%2.2"   PRIx8, tcpa->server.config_addr.register_bit_width);
	fwts_log_info_verbatim(fw, "    Register Bit Offset            0x%2.2"   PRIx8, tcpa->server.config_addr.register_bit_offset);
	fwts_log_info_verbatim(fw, "    Access Size                    0x%2.2"   PRIx8, tcpa->server.config_addr.access_width);
	fwts_log_info_verbatim(fw, "    Address                        0x%16.16" PRIx64, tcpa->server.config_addr.address);
	fwts_log_info_verbatim(fw, "  PCI Segment Group:               0x%2.2"   PRIx8, tcpa->server.pci_seg_number);
	fwts_log_info_verbatim(fw, "  PCI Bus:                         0x%2.2"   PRIx8, tcpa->server.pci_bus_number);
	fwts_log_info_verbatim(fw, "  PCI Device:                      0x%2.2"   PRIx8, tcpa->server.pci_dev_number);
	fwts_log_info_verbatim(fw, "  PCI Function:                    0x%2.2"   PRIx8, tcpa->server.pci_func_number);

	fwts_acpi_reserved_zero_check(fw, "TCPA", "Reserved", tcpa->server.reserved, sizeof(tcpa->server.reserved), &passed);
	fwts_acpi_reserved_zero_check(fw, "TCPA", "Reserved2", reserved2, sizeof(reserved2), &passed);
	fwts_acpi_reserved_zero_check(fw, "TCPA", "Reserved3", tcpa->server.reserved3, sizeof(tcpa->server.reserved3), &passed);

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

	if (tcpa->server.base_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY &&
	    tcpa->server.base_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadAddressID",
			"TCPA base address ID must be 1 or zero, got "
			"0x%2.2" PRIx8 " instead", tcpa->server.base_addr.address_space_id);
	}

	if (tcpa->server.config_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY &&
	    tcpa->server.config_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadAddressID",
			"TCPA configuration address ID must be 1 or zero, got "
			"0x%2.2" PRIx8 " instead", tcpa->server.config_addr.address_space_id);
	}

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

FWTS_REGISTER("tcpa", &tcpa_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
