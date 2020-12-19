/*
 * Copyright (C) 2015-2021 Canonical
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

#if defined(FWTS_HAS_ACPI) && !(FWTS_ARCH_AARCH64)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

static fwts_acpi_table_info *table;
acpi_table_init(DBGP, &table)

/*
 *  DBGP Table
 *   see https://msdn.microsoft.com/en-us/library/windows/hardware/dn639130%28v=vs.85%29.aspx
 */
static int dbgp_test1(fwts_framework *fw)
{
	fwts_acpi_table_dbgp *dbgp = (fwts_acpi_table_dbgp *)table->data;
	char *interface_type;
	bool passed = true;
	uint32_t reserved;

	if (!fwts_acpi_table_length_check(fw, "DBGP", table->length, sizeof(fwts_acpi_table_dbgp))) {
		passed = false;
		goto done;
	}

	switch (dbgp->interface_type) {
	case 0:
		interface_type = "Full 16550 interface";
		break;
	case 1:
		interface_type = "16550 subset interface";
		break;
	default:
		interface_type = "Reserved";
		break;
	}

	reserved = dbgp->reserved[0] + ((uint32_t) dbgp->reserved[1] << 8) +
		   ((uint32_t) dbgp->reserved[2] << 16);

	fwts_log_info_verbatim(fw, "DBGP Table:");
	fwts_log_info_verbatim(fw, "  Interface Type            0x%2.2" PRIx8 " (%s)",
		dbgp->interface_type, interface_type);
	fwts_log_info_verbatim(fw, "  Reserved:                 0x%8.8" PRIx32, reserved);
	fwts_log_info_verbatim(fw, "  Base Address:");
	fwts_log_info_verbatim(fw, "    Address Space ID:       0x%2.2" PRIx8, dbgp->base_address.address_space_id);
	fwts_log_info_verbatim(fw, "    Register Bit Width      0x%2.2" PRIx8, dbgp->base_address.register_bit_width);
	fwts_log_info_verbatim(fw, "    Register Bit Offset     0x%2.2" PRIx8, dbgp->base_address.register_bit_offset);
	fwts_log_info_verbatim(fw, "    Access Size             0x%2.2" PRIx8, dbgp->base_address.access_width);
	fwts_log_info_verbatim(fw, "    Address                 0x%16.16" PRIx64, dbgp->base_address.address);
	fwts_log_nl(fw);

	if (dbgp->interface_type > 2) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"DBGPReservedInterfaceType",
			"DBGP Interface Type is 0x%2.2" PRIx8
			" which is a reserved interface type. Expecting "
			" 0x00 (Full 16550) or 0x01 (16550 subset)",
			dbgp->interface_type);
	}

	fwts_acpi_reserved_zero_check(fw, "DBGP", "Reserved", reserved, sizeof(reserved), &passed);

	if (dbgp->base_address.register_bit_width == 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"DBGPBaseAddrBitWidthZero",
			"DBGP Base Address Bit Width is zero.");
	}
	switch (dbgp->base_address.address_space_id) {
	case 0x05 ... 0x09:
	case 0x0b ... 0x7e:
	case 0x80 ... 0xbf:
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"DBGPBaseAddrAddrSpaceID",
			"DBGP Base Address, Address Space ID 0x%" PRIx8
			" which is a reserved value.",
			dbgp->base_address.address_space_id);
		break;
	default:
		break;
	}
done:
	if (passed)
		fwts_passed(fw, "No issues found in DBGP table.");

	return FWTS_OK;
}

static fwts_framework_minor_test dbgp_tests[] = {
	{ dbgp_test1, "DBGP (Debug Port) Table test." },
	{ NULL, NULL }
};

static fwts_framework_ops dbgp_ops = {
	.description = "DBGP (Debug Port) Table test.",
	.init        = DBGP_init,
	.minor_tests = dbgp_tests
};

FWTS_REGISTER("dbgp", &dbgp_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)

#endif
