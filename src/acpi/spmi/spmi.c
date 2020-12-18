/*
 * Copyright (C) 2015-2020 Canonical
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
acpi_table_init(SPMI, &table)

/*
 *  SPMI Service Processor Management Interface Description Table
 *   see http://www.intel.com/content/www/us/en/servers/ipmi/
 *	ipmi-intelligent-platform-mgt-interface-spec-2nd-gen-v2-0-spec-update.html
 *   pages 600-606
 */
static int spmi_test1(fwts_framework *fw)
{
	char *interface_type;
	bool passed = true;
	fwts_acpi_table_spmi *spmi = (fwts_acpi_table_spmi *)table->data;

	if (!fwts_acpi_table_length_check(fw, "SPMI", table->length, sizeof(fwts_acpi_table_spmi))) {
		passed = false;
		goto done;
	}

	switch (spmi->interface_type) {
	case 0x01:
		interface_type = "Keyboard Controller Type (KCS)";
		break;
	case 0x02:
		interface_type = "Server Management Interface Chip (SMIC)";
		break;
	case 0x03:
		interface_type = "Block Transfer (BT)";
		break;
	case 0x04:
		interface_type = "SMBus System Interface (SSIF)";
		break;
	default:
		interface_type = "Reserved";
		break;
	}

	fwts_log_info_verbatim(fw, "SPMI Service Processor Management Interface Description Table:");
	fwts_log_info_verbatim(fw, "  Interface Type:           0x%2.2" PRIx8 " (%s)",
		spmi->interface_type, interface_type);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, spmi->reserved1);
	fwts_log_info_verbatim(fw, "  Specification Revision:   0x%2.2" PRIx8, spmi->specification_revision);
	fwts_log_info_verbatim(fw, "  Interrupt Type:           0x%2.2" PRIx8, spmi->interrupt_type);
	fwts_log_info_verbatim(fw, "  GPE:                      0x%2.2" PRIx8, spmi->gpe);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, spmi->reserved2);
	fwts_log_info_verbatim(fw, "  PCI Device Flag:          0x%2.2" PRIx8, spmi->pci_device_flag);
	fwts_log_info_verbatim(fw, "  Global System Interrupt   0x%8.8" PRIx32, spmi->global_system_interrupt);
	fwts_log_info_verbatim(fw, "  Base Address:");
	fwts_log_info_verbatim(fw, "    Address Space ID:       0x%2.2" PRIx8, spmi->base_address.address_space_id);
	fwts_log_info_verbatim(fw, "    Register Bit Width      0x%2.2" PRIx8, spmi->base_address.register_bit_width);
	fwts_log_info_verbatim(fw, "    Register Bit Offset     0x%2.2" PRIx8, spmi->base_address.register_bit_offset);
	fwts_log_info_verbatim(fw, "    Access Size             0x%2.2" PRIx8, spmi->base_address.access_width);
	fwts_log_info_verbatim(fw, "    Address                 0x%16.16" PRIx64, spmi->base_address.address);
	fwts_log_info_verbatim(fw, "  PCI Segment Group:        0x%2.2" PRIx8, spmi->pci_segment_group_number);
	fwts_log_info_verbatim(fw, "  PCI Bus:                  0x%2.2" PRIx8, spmi->pci_bus_number);
	fwts_log_info_verbatim(fw, "  PCI Device:               0x%2.2" PRIx8, spmi->pci_device_number);
	fwts_log_info_verbatim(fw, "  PCI Function:             0x%2.2" PRIx8, spmi->pci_function_number);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%2.2" PRIx8, spmi->reserved3);
	fwts_log_nl(fw);

	if (spmi->interface_type < 1 || spmi->interface_type > 4) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SPMIInterfaceTypeReserved",
			"SPMI Interface Type is 0x%2.2" PRIx8 " which "
			"is a reserved type and probably incorrect.",
			spmi->interface_type);
	}

	if (spmi->reserved1 != 1) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"SPMIReserved1Not1",
			"SPMI reserved field (offset 37) must be 0x01, got "
			"0x%2.2" PRIx8 " instead", spmi->reserved1);
	}

	fwts_acpi_reserved_bits_check(fw, "SPMI", "Interrupt type", spmi->interrupt_type, sizeof(spmi->interrupt_type), 2, 7, &passed);

	/* Check for zero GPE on specific condition of interrupt type */
	if (((spmi->interrupt_type & 1) == 0) &&
	    (spmi->gpe != 0)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SPMIGPENotZero",
			"SPMI GPE is 0x%2.2" PRIx8 " and should be 0 if the Interrupt "
			"Type bit 0 (SCI triggered through GPE) is set to 0",
			spmi->gpe);
	}
	if (spmi->reserved2 != 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"SPMIReserved1NonZero",
			"SPMI reserved field (offset 42) must be 0x00, got "
			"0x%2.2" PRIx8 " instead", spmi->reserved2);
	}

	fwts_acpi_reserved_bits_check(fw, "SPMI", "PCI device flag", spmi->pci_device_flag, sizeof(spmi->pci_device_flag), 1, 7, &passed);

	if (((spmi->interrupt_type & 2) == 0) &&
	    (spmi->global_system_interrupt != 0)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SPMIGlobalSystemInterruptNonZero",
			"SPMI Global System Interrupt is 0x%" PRIx32 " and "
			"should be zero if bit [1] (I/O APIC/SAPIC interrupt) "
			"of the Interrupt Type is not set",
			spmi->global_system_interrupt);
	}

	/* Base address must be one of 3 types, System Memory, System I/O or SMBUS */
	if ((spmi->base_address.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY) &&
	    (spmi->base_address.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) &&
	    (spmi->base_address.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SMBUS)) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_MEDIUM,
			"SPMIBaseAddressSpaceID",
			"SPMI Base Address Space ID is 0x%2.2" PRIx8 " but should be one of "
			"0x%2.2" PRIx8 " (System Memory), "
			"0x%2.2" PRIx8 " (System I/O) or "
			"0x%2.2" PRIx8 " (SMBUS)",
			spmi->base_address.address_space_id,
			FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY,
			FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO,
			FWTS_GAS_ADDR_SPACE_ID_SMBUS);
	}
	/*
	 * For SSIF: The Address_Space_ID is SMBUS and the address field of the GAS
	 * holds the 7-bit slave address of the BMC on the host SMBus
	 * in the least significant byte. Note that the slave address is
         * stored with the 7-bit slave address in the least significant 7-
         * bits of the byte, and the most significant bit of the byte set to
         */
	if (spmi->interface_type == 0x04) {
		if (spmi->base_address.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SMBUS) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"SPMIInvalidAddressSpaceIDForSSIF",
				"SPMI Base Address Space ID is 0x%2.2" PRIx8
				" but should be 0x%2.2" PRIx8 " (SMBUS) "
				" for a SSIF Interface Type",
				spmi->base_address.address_space_id,
				FWTS_GAS_ADDR_SPACE_ID_SMBUS);
		}
		if (spmi->base_address.address & ~0x7f) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_MEDIUM,
				"SPMIInvalidBaseAddressForSSIF",
				"SPMI Base Address is 0x%16.16" PRIx64
				" but should be 0..127 for a SSIF Interface Type",
				spmi->base_address.address);
		}
	}
	/*
	 *  Specification states that PCI device number bits [7:5]
	 *  are reserved if PCI device flag bit [0] is set, however,
	 *  it does not state if they should be zero or 1, so for
	 *  now don't check these
	 *
	 *  Also, PCI function number bits [7] and [5:3] are
	 *  reserved, again, not sure if these need to be checked
	 *  so ignore these too.
	 */
done:
	if (passed)
		fwts_passed(fw, "No issues found in SPMI table.");

	return FWTS_OK;
}

static fwts_framework_minor_test spmi_tests[] = {
	{ spmi_test1, "SPMI Service Processor Management Interface Description Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops spmi_ops = {
	.description = "SPMI Service Processor Management Interface Description Table test.",
	.init        = SPMI_init,
	.minor_tests = spmi_tests
};

FWTS_REGISTER("spmi", &spmi_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
