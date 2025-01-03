/*
 * Copyright (C) 2023-2025 Canonical
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
acpi_table_init(iBFT, &table)

static void ibfg_std_dump( fwts_framework *fw, const fwts_acpi_ibft_standard *std)
{
	fwts_log_info_simp_int(fw, "  Structure ID:               ", std->structure_id);
	fwts_log_info_simp_int(fw, "  Version:                    ", std->version);
	fwts_log_info_simp_int(fw, "  Length:                     ", std->length);
	fwts_log_info_simp_int(fw, "  Index:                      ", std->index);
	fwts_log_info_simp_int(fw, "  Flags:                      ", std->flags);
}

static void ibfg_extensions_check(
	fwts_framework *fw,
	uint32_t offset,
	bool *passed)
{

	fwts_log_info_verbatim(fw, "iBFT Extensions Structure:");

	fwts_acpi_ibft_standard *std = (fwts_acpi_ibft_standard *)(table->data + offset);

	if (offset >= table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureOffset",
			"iBFT Extensions structure exceed the table length, "
			"table length 0x%8.8" PRIx32 ", but got 0x%8.8" PRIx32,
			(uint32_t)table->length, offset);
		*passed = false;
		return;
	}

	if (std->length < sizeof(fwts_acpi_ibft_standard)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureLength",
			"iBFT Extensions structure type length too short, "
			"at least 0x%4.4" PRIx16 ", but got 0x%4.4" PRIx16,
			(uint16_t)sizeof(fwts_acpi_ibft_standard), std->length);
		*passed = false;
		return;
	}

	ibfg_std_dump(fw, std);
}

static void ibfg_initiator_check(
	fwts_framework *fw,
	uint32_t offset,
	bool *passed)
{

	fwts_acpi_ibft_initiator *initiator = (fwts_acpi_ibft_initiator *)(table->data + offset);

	if (offset >= table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureOffset",
			"iBFT Initiator structure exceed the table length, "
			"table length 0x%8.8" PRIx32 ", but got 0x%8.8" PRIx32,
			(uint32_t)table->length, offset);
		*passed = false;
		return;
	}

	if (initiator->std.length < sizeof(fwts_acpi_ibft_initiator)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureLength",
			"iBFT Initiator structure type length is not correct, "
			"expecting 0x%4.4" PRIx16 ", but got 0x%4.4" PRIx16,
			(uint16_t)sizeof(fwts_acpi_ibft_initiator), initiator->std.length);
		*passed = false;
		return;
	}

	fwts_log_info_verbatim(fw, "iBFT Initiator Structure:");
	ibfg_std_dump(fw, &initiator->std);
	fwts_acpi_reserved_bits("iBAF", "Flags", initiator->std.flags, 2, 7, passed);
	fwts_log_info_verbatim(fw, "  iSNS Server:");
	fwts_hexdump_data_prefix_all(fw, initiator->isns_server, "    ", sizeof(initiator->isns_server));
	fwts_log_info_verbatim(fw, "  SLP Server:");
	fwts_hexdump_data_prefix_all(fw, initiator->slp_server, "    ", sizeof(initiator->slp_server));
	fwts_log_info_verbatim(fw, "  Primary Radius Server:");
	fwts_hexdump_data_prefix_all(fw, initiator->primary_radius_server, "    ", sizeof(initiator->primary_radius_server));
	fwts_log_info_verbatim(fw, "  Secondary Radius Server:");
	fwts_hexdump_data_prefix_all(fw, initiator->secondary_radius_server, "    ", sizeof(initiator->secondary_radius_server));
	fwts_log_info_simp_int(fw, "  Initiator Name Length:      ", initiator->initiator_name_length);
	fwts_log_info_simp_int(fw, "  Initiator Name Offset:      ", initiator->initiator_name_offset);

	if ((initiator->initiator_name_length + initiator->initiator_name_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT initiator name offset exceed table length");
		*passed = false;
		return;
	}
}

static void ibfg_nic_check(
	fwts_framework *fw,
	uint32_t offset,
	bool *passed)
{
	fwts_acpi_ibft_nic *nic = (fwts_acpi_ibft_nic *)(table->data + offset);

	if (offset >= table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureOffset",
			"iBFT NIC structure exceed the table length, "
			"table length 0x%8.8" PRIx32 ", but got 0x%8.8" PRIx32,
			(uint32_t)table->length, offset);
		*passed = false;
		return;
	}

	if (nic->std.length < sizeof(fwts_acpi_ibft_nic)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureLength",
			"iBFT NIC structure type length is not correct, "
			"expecting 0x%4.4" PRIx16 ", but got 0x%4.4" PRIx16,
			(uint16_t)sizeof(fwts_acpi_ibft_nic), nic->std.length);
		*passed = false;
		return;
	}

	fwts_log_info_verbatim(fw, "iBFT NIC Structure:");
	ibfg_std_dump(fw, &nic->std);
	fwts_acpi_reserved_bits("iBAF", "Flags", nic->std.flags, 3, 7, passed);
	fwts_log_info_verbatim(fw, "  IP Address:");
	fwts_hexdump_data_prefix_all(fw, nic->ip_address, "    ", sizeof(nic->ip_address));
	fwts_log_info_simp_int(fw, "  Subnet Mask Prefix:         ", nic->subnet_mask_prefix);
	fwts_log_info_simp_int(fw, "  Origin:                     ", nic->origin);
	fwts_log_info_verbatim(fw, "  Gateway:");
	fwts_hexdump_data_prefix_all(fw, nic->gateway, "    ", sizeof(nic->gateway));
	fwts_log_info_verbatim(fw, "  Primary DNS:");
	fwts_hexdump_data_prefix_all(fw, nic->primary_dns, "    ", sizeof(nic->primary_dns));
	fwts_log_info_verbatim(fw, "  Secondary DNS:");
	fwts_hexdump_data_prefix_all(fw, nic->secondary_dns, "    ", sizeof(nic->secondary_dns));
	fwts_log_info_verbatim(fw, "  DHCP:");
	fwts_hexdump_data_prefix_all(fw, nic->dhcp, "    ", sizeof(nic->dhcp));
	fwts_log_info_simp_int(fw, "  VLAN:                       ", nic->vlan);
	fwts_log_info_verbatim(fw, "  MAC Address:");
	fwts_hexdump_data_prefix_all(fw, nic->mac_address, "    ", sizeof(nic->mac_address));
	fwts_log_info_simp_int(fw, "  PCI Bus/Dev/Func:           ", nic->pci_bus_dev_func);
	fwts_log_info_simp_int(fw, "  Host Name Length:           ", nic->host_name_length);
	fwts_log_info_simp_int(fw, "  Host Name Offset:           ", nic->host_name_offset);

	if ((nic->host_name_length + nic->host_name_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT host name offset exceed table length");
		*passed = false;
		return;
	}
}

static void ibfg_target_check(
	fwts_framework *fw,
	uint32_t offset,
	bool *passed)
{
	fwts_acpi_ibft_target *target = (fwts_acpi_ibft_target *)(table->data + offset);

	if (offset >= table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureOffset",
			"iBFT Target structure exceed the table length, "
			"table length 0x%8.8" PRIx32 ", but got 0x%8.8" PRIx32,
			(uint32_t)table->length, offset);
		*passed = false;
		return;
	}

	if (target->std.length < sizeof(fwts_acpi_ibft_target)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureLength",
			"iBFT Target structure type length is not correct, "
			"expecting 0x%4.4" PRIx16 ", but got 0x%4.4" PRIx16,
			(uint16_t)sizeof(fwts_acpi_ibft_target), target->std.length);
		*passed = false;
		return;
	}

	fwts_log_info_verbatim(fw, "iBFT Target Structure:");

	ibfg_std_dump(fw, &target->std);
	fwts_acpi_reserved_bits("iBAF", "Flags", target->std.flags, 4, 7, passed);
	fwts_log_info_verbatim(fw, "  Target IP Address:");
	fwts_hexdump_data_prefix_all(fw, target->target_ip_address, "    ", sizeof(target->target_ip_address));
	fwts_log_info_simp_int(fw, "  Target IP Socket:           ", target->target_ip_socket);
	fwts_log_info_simp_int(fw, "  Target Boot LUN:            ", target->target_boot_lun);
	fwts_log_info_simp_int(fw, "  CHAP Type:                  ", target->chap_type);
	fwts_acpi_reserved_type(fw, "iBFT", target->chap_type, 0, 2, passed);

	fwts_log_info_simp_int(fw, "  NIC Association:            ", target->nic_sssociation);
	fwts_log_info_simp_int(fw, "  Target Name Length:         ", target->target_name_length);
	fwts_log_info_simp_int(fw, "  Target Name Offset:         ", target->target_name_offset);
	if ((target->target_name_length + target->target_name_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT target name offset exceed table length");
		*passed = false;
	}

	fwts_log_info_simp_int(fw, "  CHAP Name Length:           ", target->chap_name_length);
	fwts_log_info_simp_int(fw, "  CHAP Name Offset:           ", target->chap_name_offset);
	if ((target->chap_name_length + target->chap_name_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT chap name offset exceed table length");
		*passed = false;
	}

	fwts_log_info_simp_int(fw, "  CHAP Secret Length:         ", target->chap_secret_length);
	fwts_log_info_simp_int(fw, "  CHAP Secret Offset:         ", target->chap_secret_offset);
	if ((target->chap_secret_length + target->chap_secret_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT chat secret offset exceed table length");
		*passed = false;
	}

	fwts_log_info_simp_int(fw, "  Reverse CHAP Name Length:   ", target->reverse_chap_name_length);
	fwts_log_info_simp_int(fw, "  Reverse CHAP Name Offset:   ", target->reverse_chap_name_offset);
	if ((target->reverse_chap_name_length + target->reverse_chap_name_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT resverse chap name offset exceed table length");
		*passed = false;
	}

	fwts_log_info_simp_int(fw, "  Reverse CHAP Secret Length: ", target->reverse_chap_secret_length);
	fwts_log_info_simp_int(fw, "  Reverse CHAP Secret Offset: ", target->reverse_chap_secret_offset);
	if ((target->reverse_chap_secret_length + target->reverse_chap_secret_offset) > table->length) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadOffset",
			"iBFT reverse chap secret offset exceed table length");
		*passed = false;
	}
}

static int ibfg_test1(fwts_framework *fw)
{
	fwts_acpi_ibft_header *header;
	fwts_acpi_ibft_control *control;
	bool passed = true;
	uint32_t offset;

	fwts_log_info_verbatim(fw, "iSCSI Boot Firmware Table:");

	header = (fwts_acpi_ibft_header *)(table->data);
	fwts_acpi_reserved_zero_array(fw, "iBFT", "Reserved", header->reserved, 24, &passed);

	offset = sizeof(fwts_acpi_ibft_header);
	control = (fwts_acpi_ibft_control *)(table->data + offset);

	if (control->std.length < sizeof(fwts_acpi_ibft_control)) {
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureLength",
			"iBFT Control structure type length too short, "
			"at least 0x%4.4" PRIx16 ", but got 0x%4.4" PRIx16,
			(uint16_t)sizeof(fwts_acpi_ibft_control), control->std.length);
		return FWTS_OK;
	}

	if (control->std.structure_id != FWTS_IBFT_CONTROL ) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"iBFTBadStructureType",
			"iBFT first structure type must to be control type(1) , got "
			"0x%2.2" PRIx8 " instead", control->std.structure_id);
		return FWTS_OK;
	}

	fwts_log_info_verbatim(fw, "iBFT Control Structure:");
	ibfg_std_dump(fw, &control->std);
	fwts_acpi_reserved_bits("iBAF", "Flags", control->std.flags, 1, 7, &passed);
	fwts_log_info_simp_int(fw, "  Extensions:                 ", control->extensions);
	if (control->extensions != 0)
		ibfg_extensions_check(fw, control->extensions, &passed);
	fwts_log_info_simp_int(fw, "  Initiator Offset:           ", control->initiator_offset);
	if (control->initiator_offset != 0)
		ibfg_initiator_check(fw, control->initiator_offset, &passed);
	fwts_log_info_simp_int(fw, "  NIC 0 Offset:               ", control->nic_0_offset);
	if (control->nic_0_offset != 0)
		ibfg_nic_check(fw, control->nic_0_offset, &passed);
	fwts_log_info_simp_int(fw, "  Target 0 Offset:            ", control->target_0_offset);
	if (control->target_0_offset != 0)
		ibfg_target_check(fw, control->target_0_offset, &passed);
	fwts_log_info_simp_int(fw, "  NIC 1 Offset:               ", control->nic_1_offset);
	if (control->nic_1_offset != 0)
		ibfg_nic_check(fw, control->nic_1_offset, &passed);
	fwts_log_info_simp_int(fw, "  Target 1 Offset:            ", control->target_1_offset);
	if (control->target_1_offset != 0)
		ibfg_target_check(fw, control->target_1_offset, &passed);
	offset += sizeof(fwts_acpi_ibft_control);

	while (offset < control->std.length) {

		/* Optional Structure Expansion */
		fwts_log_info_verbatim(fw, "iBFT Optional Structure Expansion:");

		fwts_acpi_ibft_standard *std = (fwts_acpi_ibft_standard *)(table + offset);

		switch(std->structure_id) {
			case FWTS_IBFT_INITIATOR:
				ibfg_initiator_check(fw, offset, &passed);
				break;
			case FWTS_IBFT_NIC:
				ibfg_nic_check(fw, offset, &passed);
				break;
			case FWTS_IBFT_TARGET:
				ibfg_target_check(fw, offset, &passed);
				break;
			case FWTS_IBFT_EXTENSIONS:
				ibfg_extensions_check(fw, offset, &passed);
				break;
			case FWTS_IBFT_CONTROL:
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"iBFTBadStructureID",
					"iBFT Control structure shouldn't got the Optional "
					"Structure Expansion for Control structiure ID(1)");
				break;
			case FWTS_IBFT_RESERVED:
			default:
				passed = false;
				fwts_failed(fw, LOG_LEVEL_HIGH,
					"iBFTBadStructureID",
					"iBFT Optional Structure Expansion for Control structiure ID was reserved, got "
					"or unknown got 0x%2.2" PRIx8, std->structure_id);
				break;
		}

		offset += sizeof(uint16_t);
	}

	if (passed)
		fwts_passed(fw, "No issues found in iBFT table.");

	return FWTS_OK;
}

static fwts_framework_minor_test ibfg_tests[] = {
	{ ibfg_test1, "Validate iBFT table." },
	{ NULL, NULL }
};

static fwts_framework_ops ibfg_ops = {
	.description = "iBFT iSCSI Boot Firmware Table test.",
	.init        = iBFT_init,
	.minor_tests = ibfg_tests
};

FWTS_REGISTER("ibft", &ibfg_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_ACPI)

#endif
